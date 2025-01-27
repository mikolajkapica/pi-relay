import bindgen.interface.Binding

inThisBuild(
  List(
    tlBaseVersion := "0.2",
    organization := "pwr",
    startYear := Some(2025),
    licenses := List("Apache-2.0" -> url("http://www.apache.org/licenses/LICENSE-2.0")),
    developers := List(tlGitHubDev("mikolajkapica", "Mikołaj Kapica")),
  )
)

Global / onChangedBuildSource := ReloadOnSourceChanges

val commonSettings = Seq(
  scalaVersion := "3.6.3",
  scalacOptions --= Seq("-Xfatal-warnings"),
  scalacOptions ++= Seq(
    "-Wunused:all"
  ),
  resolvers += Resolver.mavenLocal,
  Compile / doc / sources := Nil,
)

val hidapi =
  project
    .settings(commonSettings)
    .enablePlugins(ScalaNativePlugin)
    .settings(
      bindgenBindings := Seq(
        Binding(file(sys.env("HIDAPI_PATH")), "libhidapi")
      ),
      bindgenBinary := file(sys.env("BINDGEN_PATH")),
      scalacOptions += "-Wconf:msg=unused import:s",
    )
    .enablePlugins(BindgenPlugin)

val http4sVersion = "0.23.30"

val app = project
  .in(file("app"))
  .enablePlugins(ScalaNativePlugin)
  .settings(commonSettings)
  .settings(
    libraryDependencies ++= Seq(
      "org.typelevel" %%% "cats-effect" % "3.5.2",
      "co.fs2" %%% "fs2-io" % "3.9.3",
      "org.scodec" %%% "scodec-core" % "2.2.2",
      "org.scodec" %%% "scodec-cats" % "1.2.0",
      "org.http4s" %%% "http4s-ember-client" % http4sVersion,
      "org.http4s" %%% "http4s-ember-server" % http4sVersion,
      "org.http4s" %%% "http4s-dsl" % http4sVersion,
      "io.chrisdavenport" %%% "crossplatformioapp" % "0.1.0",
      "com.monovore" %%% "decline-effect" % "2.4.1",
      "com.armanbilge" %%% "epollcat" % "0.1.6",
      "com.kubukoz.dualshock4s" %%% "dualshock4s" % "0.1.0",
    ),
    nativeLinkingOptions ++= {
      val isLinux = {
        import sys.process.*
        "uname".!!.trim == "Linux"
      }

      val hidapiLinkName =
        if (isLinux) "hidapi-hidraw"
        else "hidapi"

      Seq(s"-l$hidapiLinkName")
    },
  )
  .dependsOn(hidapi)

val root = project
  .in(file("."))
  .settings(name := "pi-robot")
  .aggregate(app, hidapi)
