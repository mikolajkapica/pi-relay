#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>

#include "WiFiS3.h"
#include <SocketIOclient.h>
#include <WebSockets.h>
#include <WebSocketsClient.h>
#include <WebSocketsServer.h>
#include <WebSocketsVersion.h>

#define WIFI_SSID "wroclawski-koks"
#define WIFI_PASS "krzysinek"

#define MOTOR_AIN1 8
#define MOTOR_AIN2 7
#define MOTOR_BIN1 4
#define MOTOR_BIN2 2

WebSocketsClient webSocket;
int l2, r2;

void setupMotors();
void motor_config_forward();
void motor_config_left();
void motor_config_right();

void setupMotors() {
    pinMode(MOTOR_AIN1, OUTPUT);
    pinMode(MOTOR_AIN2, OUTPUT);
    pinMode(MOTOR_BIN1, OUTPUT);
    pinMode(MOTOR_BIN2, OUTPUT);

    digitalWrite(MOTOR_AIN1, LOW);
    digitalWrite(MOTOR_AIN2, LOW);
    digitalWrite(MOTOR_BIN1, LOW);
    digitalWrite(MOTOR_BIN2, LOW);
}

void motor_config_forward() {
    digitalWrite(MOTOR_AIN2, LOW); 
    digitalWrite(MOTOR_AIN1, HIGH);
    digitalWrite(MOTOR_BIN1, HIGH);
    digitalWrite(MOTOR_BIN2, LOW);
}

void motor_config_left() {
    digitalWrite(MOTOR_AIN2, LOW);
    digitalWrite(MOTOR_AIN1, HIGH);
    digitalWrite(MOTOR_BIN1, LOW);
    digitalWrite(MOTOR_BIN2, HIGH);
}

void motor_config_right() {
    digitalWrite(MOTOR_AIN2, HIGH);
    digitalWrite(MOTOR_AIN1, LOW);
    digitalWrite(MOTOR_BIN1, HIGH);
    digitalWrite(MOTOR_BIN2, LOW);
}

void parseNumbers(const char *input, int *l2, int *r2) {
    if (sscanf(input, "l2=%d, r2=%d", l2, r2) != 2) {
        printf("Error: Input format is incorrect\n");
    }
}

void webSocketEvent(int type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("[WSc] Disconnected!");
      break;
    case WStype_CONNECTED:
      Serial.println("[WSc] Connected!");
      webSocket.sendTXT("Connected");
      break;
    case WStype_TEXT:
      Serial.print("[WSc] get text:");
      Serial.println((char *)payload);
      parseNumbers((const char*)payload, &l2, &r2);
      break;
    case WStype_BIN:
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      break;
  }
}



void setup() {
  setupMotors();
  pinMode(3, OUTPUT);
  pinMode(5, OUTPUT);

  Serial.begin(115200);
  while (!Serial);

  Serial.println();
  Serial.println();
  Serial.println();

  for (uint8_t t = 4; t > 0; t--) {
    Serial.println("[SETUP] BOOT WAIT ...");
    Serial.flush();
    delay(1000);
  }

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  Serial.println("[Wifi]: Connecting");

  int status = WL_IDLE_STATUS;

  while (status != WL_CONNECTED) {
    Serial.print("[Wifi]: Attempting to connect to SSID: ");
    Serial.println(WIFI_SSID);
    status = WiFi.begin(WIFI_SSID, WIFI_PASS);
    delay(1000);
  }

  Serial.println("Connected!");

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  webSocket.begin("192.168.1.20", 12345, "/ws");

  webSocket.onEvent(webSocketEvent);

  webSocket.setReconnectInterval(5000);
}

void loop() {
  webSocket.loop();
  motor_config_forward();
  analogWrite(3, l2);
  analogWrite(9, r2);
}
