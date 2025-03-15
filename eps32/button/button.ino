#include <HTTPClient.h>
#include <WiFi.h>
#include "config.h" 


const char* serverName = "http://192.168.1.166:5000/center";

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



  pinMode(33, INPUT);
}

int prevValue = 0;
void loop() {
  int touchState = digitalRead(33);  // Read touch state
  
  if (touchState != prevValue){
    if (touchState){
      Serial.println("button pressed");
      HTTPClient http;

      http.begin(serverName);

      int httpCode = http.GET();

      String payload = http.getString();

      Serial.println(payload);

      http.end();
    }
  }

  prevValue= touchState;
  
}
