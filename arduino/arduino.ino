#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>
#include "WiFiS3.h"
#include <WebSocketsClient.h>

// Network configuration
#define WIFI_SSID "wroclawski-koks"
#define WIFI_PASS "krzysinek"
#define WEBSOCKET_HOST "192.168.1.20"
#define WEBSOCKET_PORT 12345
#define WEBSOCKET_PATH "/ws"

// Motor pin definitions
#define MOTOR_LEFT_PWM 3
#define MOTOR_RIGHT_PWM 9
#define MOTOR_LEFT_IN1 8
#define MOTOR_LEFT_IN2 7
#define MOTOR_RIGHT_IN1 4
#define MOTOR_RIGHT_IN2 2

// Status LED pin
#define STATUS_LED LED_BUILTIN

class MotorController {
private:
    int leftPWM, rightPWM;
    int leftIn1, leftIn2;
    int rightIn1, rightIn2;
    
public:
    MotorController(int left_pwm, int right_pwm, 
                   int left_in1, int left_in2, 
                   int right_in1, int right_in2) {
        leftPWM = left_pwm;
        rightPWM = right_pwm;
        leftIn1 = left_in1;
        leftIn2 = left_in2;
        rightIn1 = right_in1;
        rightIn2 = right_in2;
    }

    void begin() {
        pinMode(leftPWM, OUTPUT);
        pinMode(rightPWM, OUTPUT);
        pinMode(leftIn1, OUTPUT);
        pinMode(leftIn2, OUTPUT);
        pinMode(rightIn1, OUTPUT);
        pinMode(rightIn2, OUTPUT);
        stop();
    }

    void stop() {
        digitalWrite(leftIn1, LOW);
        digitalWrite(leftIn2, LOW);
        digitalWrite(rightIn1, LOW);
        digitalWrite(rightIn2, LOW);
        analogWrite(leftPWM, 0);
        analogWrite(rightPWM, 0);
    }

    void forward() {
        digitalWrite(leftIn2, LOW);
        digitalWrite(leftIn1, HIGH);
        digitalWrite(rightIn1, HIGH);
        digitalWrite(rightIn2, LOW);
    }

    void backward() {
        digitalWrite(leftIn2, HIGH);
        digitalWrite(leftIn1, LOW);
        digitalWrite(rightIn1, LOW);
        digitalWrite(rightIn2, HIGH);
    }

    void left() {
        digitalWrite(leftIn2, LOW);
        digitalWrite(leftIn1, HIGH);
        digitalWrite(rightIn1, LOW);
        digitalWrite(rightIn2, HIGH);
    }

    void right() {
        digitalWrite(leftIn2, HIGH);
        digitalWrite(leftIn1, LOW);
        digitalWrite(rightIn1, HIGH);
        digitalWrite(rightIn2, LOW);
    }

    void setPower(int left, int right) {
        analogWrite(leftPWM, left);
        analogWrite(rightPWM, right);
    }
};

class NetworkManager {
private:
    WebSocketsClient webSocket;
    void (*messageCallback)(int left, int right);

public:
    NetworkManager(void (*callback)(int, int)) {
        messageCallback = callback;
    }

    void begin() {
        setupWiFi();
        setupWebSocket();
    }

    void loop() {
        webSocket.loop();
    }

private:
    void setupWiFi() {
        if (WiFi.status() == WL_NO_MODULE) {
            Serial.println("Communication with WiFi module failed!");
            while (true);
        }

        String fv = WiFi.firmwareVersion();
        if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
            Serial.println("Please upgrade the firmware");
        }

        Serial.println("[Wifi]: Connecting");
        while (WiFi.status() != WL_CONNECTED) {
            Serial.printf("[Wifi]: Attempting to connect to %s\n", WIFI_SSID);
            WiFi.begin(WIFI_SSID, WIFI_PASS);
            delay(1000);
        }

        Serial.print("Connected! IP: ");
        Serial.println(WiFi.localIP());
    }

    void setupWebSocket() {
        webSocket.begin(WEBSOCKET_HOST, WEBSOCKET_PORT, WEBSOCKET_PATH);
        webSocket.onEvent([this](WStype_t type, uint8_t * payload, size_t length) {
            handleWebSocketEvent(type, payload, length);
        });
        webSocket.setReconnectInterval(5000);
    }

    void handleWebSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
        switch (type) {
            case WStype_DISCONNECTED:
                Serial.println("[WSc] Disconnected!");
                break;
            case WStype_CONNECTED:
                Serial.println("[WSc] Connected!");
                webSocket.sendTXT("Connected");
                break;
            case WStype_TEXT: {
                Serial.printf("[WSc] Received: %s\n", (char *)payload);
                int left, right;
                if (sscanf((char *)payload, "%d %d", &left, &right) == 2) {
                    messageCallback(left, right);
                }
                break;
            }
            default:
                break;
        }
    }
};

// Global objects
MotorController motors(MOTOR_LEFT_PWM, MOTOR_RIGHT_PWM,
                      MOTOR_LEFT_IN1, MOTOR_LEFT_IN2,
                      MOTOR_RIGHT_IN1, MOTOR_RIGHT_IN2);
NetworkManager* network;

void handleMotorCommand(int left, int right) {
    if (left < 128 && right < 128) {
        motors.backward();
    } else if (left > 127 && right > 127) {
        motors.forward();
    } else if (left < 128 && right > 127) {
        motors.left();
    } else if (left > 127 && right < 128) {
        motors.right();
    }

    int leftPower = abs(left - 128);
    int rightPower = abs(right - 128);
    motors.setPower(leftPower, rightPower);
}

void setup() {
    Serial.begin(115200);
    while (!Serial);
    pinMode(STATUS_LED, OUTPUT);
    
    motors.begin();
    network = new NetworkManager(handleMotorCommand);
    network->begin();
}

void loop() {
    network->loop();
}
