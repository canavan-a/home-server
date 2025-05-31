#include <WiFi.h>
#include <esp_now.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include "config.h" // un and pw

#define RELAY_PIN 33
#define MAG_PIN 16

WebServer server(80);

void handleRoot()
{
    server.send(200, "text/plain", "Hello, Garage!");
}

void handleIp()
{
    server.send(200, "text/plain", WiFi.localIP().toString());
}

void handleMac()
{
    server.send(200, "text/plain", String(WiFi.macAddress()));
}

void handleGarage()
{
    server.send(200, "text/plain", String(digitalRead(MAG_PIN)));
}

void openGarage()
{
    digitalWrite(RELAY_PIN, HIGH);
    delay(100);
    digitalWrite(RELAY_PIN, LOW);
}

void handleTriggerStateChange()
{
    openGarage();
    server.send(200, "text/plain", "done");
}

void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    Serial.print("Received from: ");
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             info->src_addr[0], info->src_addr[1], info->src_addr[2],
             info->src_addr[3], info->src_addr[4], info->src_addr[5]);
    Serial.println(macStr);

    Serial.print("Data: ");
    Serial.write(data, len);
    Serial.println();
    openGarage();
}

void setup()
{
    Serial.begin(115200);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    WiFi.setHostname("Garage Module");

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".\n");
        delay(500);
    }
    Serial.println(WiFi.localIP());

    server.on("/", handleRoot);
    server.on("/ip", handleIp);
    server.on("/mac", handleMac);
    server.on("/garage", handleGarage);

    server.on("/change", handleTriggerStateChange);
    server.begin();
    ArduinoOTA.setPassword("aidan");
    ArduinoOTA.begin();

    pinMode(MAG_PIN, INPUT_PULLUP);

    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);

    esp_now_init();
    esp_now_register_recv_cb(onReceive);
}

void loop()
{
    server.handleClient();
    ArduinoOTA.handle();
}