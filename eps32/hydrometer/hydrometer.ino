#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "config.h" // un and pw


#define ONE_WIRE_BUS 32  // Change this to the GPIO you're using

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

WebServer server(80); // Listen on port 80 (HTTP)



void handleRoot() {
  server.send(200, "text/plain", "Hello, ESP32!");
}

void handleHydrometerLevel() {
  server.send(200, "text/plain", String(analogRead(34)));
}

void handleTemperature(){
  sensors.requestTemperatures();
  float temperatureC = sensors.getTempCByIndex(0);

  if (temperatureC != DEVICE_DISCONNECTED_C) {
    float temperatureF = (temperatureC * 9.0 / 5.0) + 32.0;
    server.send(200, "text/plain", String(temperatureF));
  } else {
    Serial.println("Error: Could not read temperature");
    server.send(400, "invalid reading");
  }


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


  sensors.begin(); 

  server.on("/", handleRoot);
  server.on("/hydrometer", handleHydrometerLevel);
  server.on("/temp", handleTemperature);
  server.begin(); // Start server
  ArduinoOTA.setPassword("aidan");
  ArduinoOTA.begin();
}

void loop() {
  server.handleClient(); // Listen for incoming requests
  ArduinoOTA.handle();
}
