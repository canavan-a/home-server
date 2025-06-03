#include <WiFi.h>
#include <esp_now.h>

#define BUTTON_PIN 16

uint8_t peerAddress[] = {0xCC, 0x7B, 0x5C, 0xA7, 0x85, 0x5C};

void setup()
{
    Serial.begin(115200);
    pinMode(BUTTON_PIN, INPUT);
    WiFi.mode(WIFI_STA);
    esp_now_init();

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, peerAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
}

void publishAction()
{
    const char *msg = "trigger";
    esp_now_send(peerAddress, (uint8_t *)msg, strlen(msg));
}

bool state = false;

void loop()
{
    bool updatedState = digitalRead(BUTTON_PIN);

    if (updatedState == true && state == false)
    {
        publishAction();
    }

    state = updatedState;
}
