#include <WiFi.h>
#include "config.h" 
#include <WebServer.h>

WebServer server(80); 


int prevValue = 0;

int state = 0;

void handleToggle(){
    if (state == 0){
        state = 1;
      } else{
        state = 0; 
    }
    server.send(200, "text/plain", "toggled");
}

void handleToggleEndpoint(){
    if (state == 0){
        state = 1;
      } else{
        state = 0; 
    }
    server.send(200, "text/plain", "toggled");
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

  server.on("/toggle", handleToggleEndpoint);

  pinMode(33, INPUT);
  pinMode(14, OUTPUT);
}





void loop() {
  int touchState = digitalRead(33);  // Read touch state
  
  if (touchState != prevValue){
    if (touchState){
      Serial.println("button pressed");
     
      Serial.println(WiFi.localIP());

      handleToggle();
    } 
  }

  if (state == 0){
      digitalWrite(14, HIGH);
  } else{
      digitalWrite(14, LOW);
  }

  server.handleClient(); 

  prevValue= touchState;
}
