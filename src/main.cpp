#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

#include "app/hardware_tests/mac_discovery_helper.h"
#include "drivers/communication/espnow_transport_driver.h"

const char* ssid = "Cyber32-Setup";
const char* password = "12345678";
const int TEMP_ESPNOW_RX_TEST_CHANNEL = 1;

WebServer server(80);
Servo servo1;
Cyber32::EspNowTransportDriver espnowRxTestDriver;

const int SERVO_PIN = 18;

void handleRoot() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Cyber32 Core OS</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            font-family: Arial;
            background: #111;
            color: white;
            text-align: center;
            padding-top: 40px;
        }
        button {
            width: 120px;
            height: 60px;
            margin: 10px;
            font-size: 18px;
            border-radius: 10px;
            border: none;
            background: #00aaff;
            color: white;
        }
        input {
            width: 80%;
        }
    </style>
</head>
<body>
    <h1>Cyber32 Core OS</h1>

    <h2>Servo 222</h2>
    <input type="range" min="0" max="180" value="90"
       onchange="fetch('/servo?angle=' + this.value)">

    <br><br>

    <button onclick="fetch('/forward')">FORWARD</button><br>

    <button onclick="fetch('/left')">LEFT</button>
    <button onclick="fetch('/stop')">STOP</button>
    <button onclick="fetch('/right')">RIGHT</button><br>

    <button onclick="fetch('/backward')">BACKWARD</button>
</body>
</html>
)rawliteral";

    server.send(200, "text/html", html);
}

void setup() {
    Serial.begin(115200);

    // Temporary hardware-test MAC discovery for first ESP-NOW base-node setup.
    Cyber32::MacDiscoveryHelper macDiscoveryHelper;
    macDiscoveryHelper.begin();
    macDiscoveryHelper.printBaseNodeMacToSerial();

    servo1.setPeriodHertz(50);
    servo1.attach(SERVO_PIN, 500, 2400);
    servo1.write(90);

    // Temporary hardware-test AP+STA channel alignment for ESP-NOW receive visibility.
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ssid, password, TEMP_ESPNOW_RX_TEST_CHANNEL);

    Serial.println("WiFi AP Started");
    Serial.print("Cyber32 WiFi channel: ");
    Serial.println(TEMP_ESPNOW_RX_TEST_CHANNEL);
    Serial.print("IP: ");
    Serial.println(WiFi.softAPIP());

    // Temporary hardware-test ESP-NOW receive hook for first sender packet visibility.
    if (espnowRxTestDriver.begin()) {
        Serial.println("Cyber32 ESP-NOW RX test started");
    } else {
        Serial.println("Cyber32 ESP-NOW RX test failed");
    }

    server.on("/", handleRoot);

    server.on("/servo", []() {
        if (server.hasArg("angle")) {
            int angle = server.arg("angle").toInt();
            angle = constrain(angle, 0, 180);
            servo1.write(angle);

            Serial.print("Servo angle: ");
            Serial.println(angle);
        }

        server.send(200, "text/plain", "OK");
    });

    server.on("/forward", []() {
        Serial.println("FORWARD");
        server.send(200, "text/plain", "OK");
    });

    server.on("/backward", []() {
        Serial.println("BACKWARD");
        server.send(200, "text/plain", "OK");
    });

    server.on("/left", []() {
        Serial.println("LEFT");
        server.send(200, "text/plain", "OK");
    });

    server.on("/right", []() {
        Serial.println("RIGHT");
        server.send(200, "text/plain", "OK");
    });

    server.on("/stop", []() {
        Serial.println("STOP");
        server.send(200, "text/plain", "OK");
    });

    server.begin();
    Serial.println("Web server started");
}

void loop() {
    server.handleClient();
}
