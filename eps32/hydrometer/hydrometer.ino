#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include "config.h" // un and pw


WebServer server(80); // Listen on port 80 (HTTP)


void handleRoot() {
  server.send(200, "text/plain", "Hello, ESP32!");
}

void handleHydrometerLevel() {
  server.send(200, "text/plain", String(analogRead(34)));
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".\n");
    delay(500);
  }

  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/hydrometer", handleHydrometerLevel);
  server.begin(); // Start server

  ArduinoOTA.begin();
}

void loop() {
  server.handleClient(); // Listen for incoming requests
  ArduinoOTA.handle();
}
