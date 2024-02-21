#include "VeryLongRangeLoRa.h"
#include "HT_SSD1306Wire.h"
#include <esp_now.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

bool indicatorModule = false;

const char *apiURL = "https://api.ondarra.rellum.com.ar/weight";

/*
const char *ssid = "Fibertel WiFi012 2.4GHz";
const char *password = "0041613442";
 */

// /*
const char *ssid = "Ondarra";
const char *password = "riverplate2012";
// */

uint8_t stickMac[] = { 0x3C, 0xE9, 0x0E, 0x86, 0xB4, 0xA4 };
esp_now_peer_info_t stickPeerInfo;
uint8_t myMacAddress[6];

VeryLongRangeLoRa loraMessaging;
long lastMessageSentMillis;
Message message;
int counter;

const byte STX2 = 0x02;
char STX = (char)2;
long lastWeightSentMillis;
const byte CR = 0x0D;
const byte LF = 0x0A;

short weight = 0;
bool movement = false;

SSD1306Wire factory_display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);  // addr , freq , i2c group , resolution , rst

typedef struct weight_reading_message {
  byte msgType;
  short weight;
  bool movement;
} weight_reading_message;

typedef struct weight_sending_message {
  char tagId[16];
  short weight;
} weight_sending_message;

weight_reading_message weightData;
weight_sending_message animalData;
byte httpResponseStatus;
StaticJsonDocument<200> requestJson;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  factory_display.clear();
  factory_display.display();
  factory_display.drawString(0, 0, String(weightData.weight));
  factory_display.drawString(0, 10, (weightData.movement ? "MV" : "EQ"));
  if (status == ESP_NOW_SEND_SUCCESS) {
    factory_display.drawString(0, 20, "Stick received");
  } else {
    factory_display.drawString(0, 20, "Stick failed");
  }
  factory_display.display();
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&animalData, incomingData, sizeof(animalData));
  Serial.println("Message received");
  Serial.println(animalData.tagId);
  Serial.println(animalData.weight);
  Serial.println("Sending weight through LoRa");
  message.id = 1;
  memcpy(&message.payload, incomingData, sizeof(animalData));
  //loraMessaging.sendMessage(0xF0406AFA12F4, message);
  loraMessaging.sendMessage(0x6C5ACE7554DC, message);
}

void sendPostRequest() {

  String jsonBody = "";
  requestJson["tagId"] = animalData.tagId;
  requestJson["weight"] = animalData.weight;

  factory_display.clear();
  factory_display.drawString(0, 0, "Realizando POST");
  factory_display.drawString(0, 10, animalData.tagId);
  factory_display.display();

  HTTPClient http;
  http.begin(apiURL);
  http.addHeader("Content-Type", "application/json");

  serializeJson(requestJson, jsonBody);
  Serial.println(jsonBody);

  int httpResponseCode = http.POST(jsonBody);

  if (httpResponseCode > 0) {
    factory_display.clear();
    factory_display.drawString(0, 0, "POST Exitoso");
    factory_display.display();
    Serial.print("Solicitud POST exitosa. Código de respuesta: ");
    Serial.println(httpResponseCode);
    if (httpResponseCode == 200) {
      httpResponseStatus = 0;
    } else {
      httpResponseStatus = 2;
    }

  } else {
    factory_display.clear();
    factory_display.drawString(0, 0, "POST Error");
    factory_display.display();
    Serial.print("Error en la solicitud POST. Código de respuesta: ");
    Serial.println(httpResponseCode);
    httpResponseStatus = 1;
  }

  http.end();
}

void onMessageReceived(uint64_t senderID, Message newMessage, int16_t rssi) {
  if (indicatorModule) {
    if (newMessage.id == 2) {
      Serial.println("Upload confirmation received");
      memcpy(&httpResponseStatus, newMessage.payload, sizeof(httpResponseStatus));
      if (httpResponseStatus == 0) {
        weightData.msgType = 2;
      } else if (httpResponseStatus == 1) {
        weightData.msgType = 3;
      } else if (httpResponseStatus == 2) {
        weightData.msgType = 4;
      }
      lastWeightSentMillis = millis();
      esp_err_t result = esp_now_send(stickMac, (uint8_t *)&weightData, sizeof(weightData));

      if (result != ESP_OK) {
        factory_display.clear();
        factory_display.display();
        factory_display.drawString(0, 0, String(weightData.weight));
        factory_display.drawString(0, 10, (weightData.movement ? "MV" : "EQ"));
        factory_display.drawString(0, 20, "Stick conection error");
        factory_display.display();
      }
    }
  } else {
    if (newMessage.id == 1) {
      Serial.println("Uploading weight...");
      memcpy(&animalData, newMessage.payload, sizeof(animalData));
      Serial.println(animalData.tagId);
      Serial.println(animalData.weight);
      sendPostRequest();
      if (httpResponseStatus == 1) sendPostRequest();
      message.id = 2;
      memcpy(&message.payload, &httpResponseStatus, sizeof(httpResponseStatus));

      loraMessaging.sendMessage(0x74C6CE7554DC, message);
    }
  }
}

void onMessageSent(uint64_t receiverID, byte messageID, bool received, byte intents) {
  factory_display.clear();
  factory_display.drawString(0, 0, "SENDING ReceiverID:");
  factory_display.drawString(0, 10, String(receiverID));
  factory_display.drawString(0, 40, "Status");
  if (received) {
    factory_display.drawString(0, 50, "RECEIVED");
  } else {
    factory_display.drawString(0, 50, "SENT");
  }
  factory_display.drawString(40, 40, "Intents:");
  factory_display.drawString(80, 40, String(intents));
  factory_display.display();
  if (indicatorModule && received) {
    weightData.msgType = 1;
    lastWeightSentMillis = millis();
    esp_err_t result = esp_now_send(stickMac, (uint8_t *)&weightData, sizeof(weightData));

    if (result != ESP_OK) {
      factory_display.clear();
      factory_display.display();
      factory_display.drawString(0, 0, String(weightData.weight));
      factory_display.drawString(0, 10, (weightData.movement ? "MV" : "EQ"));
      factory_display.drawString(0, 20, "Stick conection error");
      factory_display.display();
    }
  } else if (indicatorModule && intents >= 6) {
    weightData.msgType = 5;
    lastWeightSentMillis = millis();
    esp_err_t result = esp_now_send(stickMac, (uint8_t *)&weightData, sizeof(weightData));

    if (result != ESP_OK) {
      factory_display.clear();
      factory_display.display();
      factory_display.drawString(0, 0, String(weightData.weight));
      factory_display.drawString(0, 10, (weightData.movement ? "MV" : "EQ"));
      factory_display.drawString(0, 20, "Stick conection error");
      factory_display.display();
    }
  }
}

void setup() {
  if (indicatorModule) {
    Serial.begin(9600);
    WiFi.mode(WIFI_STA);
    Serial.println(WiFi.macAddress());
    factory_display.init();
    factory_display.clear();
    factory_display.display();
    factory_display.drawString(0, 0, "Iniciando...");
    factory_display.display();
    esp_read_mac(myMacAddress, ESP_MAC_WIFI_STA);

    if (esp_now_init() != ESP_OK) {
      factory_display.clear();
      factory_display.display();
      factory_display.drawString(0, 0, "Error ESP-NOW");
      factory_display.display();
      return;
    }

    esp_now_register_recv_cb(OnDataRecv);
    esp_now_register_send_cb(OnDataSent);

    memcpy(stickPeerInfo.peer_addr, stickMac, 6);
    stickPeerInfo.channel = 0;
    stickPeerInfo.encrypt = false;

    if (esp_now_add_peer(&stickPeerInfo) != ESP_OK) {
      factory_display.clear();
      factory_display.display();
      factory_display.drawString(0, 0, "STICK PEER FAIL");
      factory_display.display();
      return;
    }

  } else {
    Serial.begin(115200);
    factory_display.init();
    factory_display.clear();
    factory_display.display();
    factory_display.drawString(0, 0, "Conectando a WiFi...");
    factory_display.display();
    WiFi.mode(WIFI_STA);
    WiFi.setTxPower(WIFI_POWER_11dBm);
    WiFi.begin(ssid, password);
    Serial.print("Connecting to Wifi");
    byte intents;

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      intents++;
      if (intents >= 60) {
        factory_display.clear();
        factory_display.display();
        factory_display.drawString(0, 0, "Error Conectando WiFi");
        factory_display.display();
        delay(3000);
        ESP.restart();
      }
    }

    Serial.println("");
    Serial.println("Connected to Wifi");
    factory_display.clear();
    factory_display.display();
    factory_display.drawString(0, 0, "Conectado");
    factory_display.display();
  }

  loraMessaging.init(915000000, 20, 0);
  loraMessaging.setOnMessageReceived(onMessageReceived);
  loraMessaging.setOnMessageSent(onMessageSent);
}

void loop() {
  if (indicatorModule) {
    static boolean recvInProgress = false;
    static short index = 0;
    static byte buffer[13];
    while (Serial.available() > 0) {
      if (!recvInProgress) {
        if (STX == Serial.read()) {
          recvInProgress = true;
          index = 0;
        }
      } else {
        index++;
        buffer[index] = Serial.read();
        if (index >= 12) {
          recvInProgress = false;
          parseMessage(buffer);
        }
      }
    }
  } else {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.print(millis());
      Serial.println("Reconnecting to WiFi...");
      factory_display.clear();
      factory_display.display();
      factory_display.drawString(0, 0, "Reconectando al WiFi...");
      factory_display.display();
      delay(3000);
      if (WiFi.status() != WL_CONNECTED) {
        ESP.restart();
      }
      factory_display.clear();
      factory_display.display();
      factory_display.drawString(0, 0, "Conectado");
      factory_display.display();
    }
  }
  loraMessaging.process();
}

void parseMessage(byte buffer[]) {
  char weightStr[9];
  strncpy(weightStr, (char *)&buffer[2], 8);
  weightStr[8] = '\0';
  weight = atof(weightStr);

  char status = buffer[11];
  movement = (status == 'M');

  weightData.msgType = 0;
  weightData.weight = weight;
  weightData.movement = movement;
  if (millis() - lastWeightSentMillis >= 500) {
    lastWeightSentMillis = millis();
    esp_err_t result = esp_now_send(stickMac, (uint8_t *)&weightData, sizeof(weightData));

    if (result != ESP_OK) {
      factory_display.clear();
      factory_display.display();
      factory_display.drawString(0, 0, String(weightData.weight));
      factory_display.drawString(0, 10, (weightData.movement ? "MV" : "EQ"));
      factory_display.drawString(0, 20, "Stick conection error");
      factory_display.display();
    }
  }
}
