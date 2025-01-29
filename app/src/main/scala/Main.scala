import cats.Eq
import cats.effect.*
import cats.effect.std.Console
import cats.implicits.*
import com.comcast.ip4s.*
import com.kubukoz.hid4s.*
import fs2.{Pipe, Stream}
import org.http4s.*
import org.http4s.Method.GET
import org.http4s.dsl.io.*
import org.http4s.ember.server.EmberServerBuilder
import org.http4s.implicits.*
import org.http4s.server.Server
import org.http4s.server.websocket.WebSocketBuilder2
import org.http4s.websocket.WebSocketFrame
import scodec.bits.*

import java.util.concurrent.TimeUnit
import scala.concurrent.duration.*
import scala.util.chaining.*

object Main extends IOApp {

  private def routes(ws: WebSocketBuilder2[IO]): HttpRoutes[IO] =
    HttpRoutes.of[IO] { case GET -> Root / "ws" =>
      ws.build(
        send = processControllerInput.map(msg => WebSocketFrame.Text(msg)).repeat,
        receive = in => in.evalMap(frameIn => IO(println(s"Received: ${frameIn.data}"))),
      )
    }

  private val serverResource: Resource[IO, Server] =
    EmberServerBuilder
      .default[IO]
      .withHost(ipv4"0.0.0.0")
      .withPort(port"12345")
      .withHttpWebSocketApp(ws => routes(ws).orNotFound)
      .build

  private def retryExponentially[F[_]: {Temporal, Console}, A]: Pipe[F, A, A] = {
    val factor = 1.2

    def go(stream: Stream[F, A], attemptsRemaining: Int, currentDelay: FiniteDuration): Stream[F, A] =
      if (attemptsRemaining <= 1) stream
      else
        Stream.suspend {
          val newDelay = FiniteDuration((currentDelay * factor).toMillis, TimeUnit.MILLISECONDS)

          val showRetrying = Stream.exec(
            Console[F].errorln(
              s"Device not available, retrying ${attemptsRemaining - 1} more times in $newDelay..."
            )
          )

          stream.handleErrorWith(_ => showRetrying ++ go(stream, attemptsRemaining - 1, newDelay).delayBy(currentDelay))
        }

    go(_, 10, 1.second)
  }

  private enum DeviceInfo(val vendorId: Int, val productId: Int) {
    case Dualsense extends DeviceInfo(0x54c, 0xce6)
    case DS4 extends DeviceInfo(0x54c, 0x9cc)
  }

  private def hidapi(device: DeviceInfo): Stream[IO, BitVector] = Stream
    .resource(HID.instance[IO])
    .flatMap(_.getDevice(device.vendorId, device.productId).pipe(Stream.resource).pipe(retryExponentially))
    .flatMap(_.read(64))

  private def processControllerInput: Stream[IO, String] = {
    val device = DeviceInfo.DS4
    val pollingRate = 5.millis
    val input = hidapi(device).metered(pollingRate)

    implicit val eqBitVector: Eq[BitVector] = Eq.fromUniversalEquals

    val usb = 1 // 0x01
    val bluetooth = 17 // 0x11

    // handle counter overflow
    def normalize(i: Int): Int =
      if (i < 0) { 255 + i }
      else i

    val loop: fs2.Pipe[IO, BitVector, String] =
      _.map { bitVector =>
        val byteVector = bitVector.bytes

         byteVector(0).toInt match {
          case `bluetooth` if byteVector.length >= 12 =>
            val leftY = normalize(byteVector(4).toInt)
            val rightY = normalize(byteVector(6).toInt)
            s"$leftY $rightY"

          case `usb` if byteVector.length >= 10 =>
            val leftY = normalize(byteVector(2).toInt & 0xff)
            val rightY = normalize(byteVector(4).toInt & 0xff)
            s"$leftY $rightY"

          case `usb` | `bluetooth` =>
            "Error: Report is too short for the expected mode"

          case _ =>
            "Error: Unknown report type"
        }
      }
        .changes
        .debug()

    input.through(loop)
  }

  def run(args: List[String]): IO[ExitCode] =
    IO.println("Starting server") >> serverResource.useForever

}
