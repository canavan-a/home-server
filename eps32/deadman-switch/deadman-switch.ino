#include "Arduino.h"
#include "WebServer.h"
#include "WiFi.h"
#include "constants.h"

#define TRIGGER_PIN 25

WebServer server(80);

void handleTrigger()
{
    int newState = !digitalRead(TRIGGER_PIN);
    digitalWrite(TRIGGER_PIN, newState);

    String body = "toggled, " + String(newState);
    server.send(200, "text/plain", body);
}

void setup()
{
    Serial.begin(115200);
    pinMode(TRIGGER_PIN, OUTPUT);
    digitalWrite(TRIGGER_PIN, LOW);

    WiFi.begin(env::SSID, env::PASSWORD);

    server.on("/trigger", HTTP_GET, handleTrigger);
    server.begin();
}

void loop()
{
    server.handleClient();
    Serial.println(WiFi.localIP());
}