#include <U8g2lib.h>
#include <esp_now.h>
#include <WiFi.h>
#define buzzerPIN 25
#define buttonPIN 26
#define screenDrawGap 50

/*
ESP  | DISPLAY
3.3v   3.3v
GND    GND
27     RES
05     CS
14     DC
18     D6
23     D7
*/

//DC:54:75:CE:C6:74
uint8_t indicatorMAC[] = { 0xDC, 0x54, 0x75, 0xCE, 0xC6, 0x74 };
esp_now_peer_info_t indicatorPeerInfo;

U8G2_ST7565_64128N_F_4W_SW_SPI u8g2(U8G2_MIRROR_VERTICAL, /* clock=*/18, /* data=*/23, /* cs=*/5, /* dc=*/14, /* reset=*/27);
uint8_t myMacAddress[6];
long lastMessageReceived;
byte messageStatus = 0;
bool buttonPressed;
long lastButtonPressed;
long lastMessageSent;
long lastDisplayDrawed;
bool buzzed;
bool buzzedReceived;
short prevWeight;
int spashTimeout = 15000;
bool booting = true;
bool showingSplash = true;
char STX = (char)2;

char lastTagScanned[16];

typedef struct weight_reading_message {
  byte msgType;
  short weight;
  bool movement;
} weight_reading_message;

typedef struct weight_sending_message {
  char tagId[16];
  short weight;
} weight_sending_message;

weight_reading_message receivedData;
weight_sending_message animalData;


void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  //Serial.print("\r\nLast Packet Send Status:\t");
  if (status != ESP_NOW_SEND_SUCCESS) {
    Serial.println("Message not received");
    messageStatus = 5;
    drawDisplay();
  } else {
    Serial.println("Message received");
  }
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&receivedData, incomingData, sizeof(receivedData));
  if (receivedData.msgType == 1) {
    messageStatus = 2;  // Message Received
    if (!buzzedReceived) {
      buzzedReceived = true;
      buzz(50);
    }
  } else if (receivedData.msgType == 2) {
    messageStatus = 3;  // Weight Uploaded
  } else if (receivedData.msgType == 3) {
    messageStatus = 6;  // POST Request Failed -> No connection
  } else if (receivedData.msgType == 4) {
    messageStatus = 7;  // POST Request Error
  } else if (receivedData.msgType == 5) {
    messageStatus = 8;  // LoRa Message Failed
  }
  if (receivedData.weight == 0 && prevWeight == 0) {
    animalData.weight = 0;
  } else if (receivedData.weight != 0) {
    memcpy(&animalData.weight, &receivedData.weight, sizeof(receivedData.weight));
  }
  prevWeight = receivedData.weight;

  lastMessageReceived = millis();
  if ((messageStatus >= 1) && millis() - lastDisplayDrawed >= screenDrawGap) {
    delay(millis() - lastDisplayDrawed + 10);
    drawDisplay();
  } else {
    drawDisplay();
  }
}

void drawDisplay() {
  if (!booting && millis() - lastDisplayDrawed >= screenDrawGap) {
    lastDisplayDrawed = millis();
    showingSplash = false;
    char s[4];
    sprintf(s, "%03d", receivedData.weight);
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_fub42_tn);
    u8g2.drawStr(0, 43, s);
    u8g2.setFont(u8g2_font_crox4hb_tf);
    u8g2.drawStr(100, 43, "KG");
    if (receivedData.movement) u8g2.drawStr(100, 20, "M!");
    u8g2.drawLine(0, 45, 128, 45);
    u8g2.setFont(u8g2_font_crox1hb_tf);
    if (messageStatus == 0) {
      u8g2.drawStr(0, 64, "ID:");
      u8g2.drawStr(20, 64, animalData.tagId);
    } else if (messageStatus == 1) {
      u8g2.drawStr(0, 64, "ENVIANDO...");
    } else if (messageStatus == 2) {
      u8g2.drawStr(0, 64, "GUARDANDO...");
    } else if (messageStatus == 3) {
      u8g2.drawStr(0, 64, "PESO GUARDADO");
    } else if (messageStatus == 4) {
      u8g2.drawStr(0, 60, "Error 2: Enviando");
    } else if (messageStatus == 5) {
      u8g2.drawStr(0, 60, "Error 1: Muy Lejos");
    } else if (messageStatus == 6) {
      u8g2.drawStr(0, 60, "Error 3: WiFi Apagado");
    } else if (messageStatus == 7) {
      u8g2.drawStr(0, 64, "PESO/TAG INVALIDO");
    } else if (messageStatus == 8) {
      u8g2.drawStr(0, 64, "Error 4: LoRa");
    }

    u8g2.sendBuffer();
    lastDisplayDrawed = millis();
  }
  if (!buzzed && messageStatus == 3) {
    buzzed = true;
    buzz(50);
    delay(50);
    buzz(250);
  } else if (!buzzed && messageStatus >= 4) {
    buzzed = true;
    buzz(500);
  }
}

void showSplash() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_crox5hb_tf);
  u8g2.drawStr(1, 30, "ONDARRA");
  u8g2.setFont(u8g2_font_crox1hb_tf);
  u8g2.drawStr(10, 52, "RFID FDX-B / HDX");
  u8g2.drawStr(30, 64, "ISO 11784/5");
  u8g2.sendBuffer();
  showingSplash = true;
  receivedData.weight = 0;
  lastDisplayDrawed = millis();
  delay(2000);
  booting = false;
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N2);
  pinMode(buzzerPIN, OUTPUT);
  pinMode(buttonPIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(buttonPIN), buttonReleaseInterrupt, FALLING);
  delay(500);
  u8g2.begin();
  WiFi.mode(WIFI_STA);
  Serial.println(WiFi.macAddress());
  esp_read_mac(myMacAddress, ESP_MAC_WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
  esp_now_register_send_cb(OnDataSent);

  memcpy(indicatorPeerInfo.peer_addr, indicatorMAC, 6);
  indicatorPeerInfo.channel = 0;
  indicatorPeerInfo.encrypt = false;
  if (esp_now_add_peer(&indicatorPeerInfo) != ESP_OK) {
    Serial.println("Error adding indicator peer");
  }
  Serial.println("ESP Now initialized");
  showSplash();
}

void loop() {
  if (!showingSplash && messageStatus == 0 && (millis() - lastMessageReceived > spashTimeout)) {
    showSplash();
  }
  static boolean recvInProgress = false;
  static short index = 0;
  static byte buffer[30];
  while (Serial2.available() > 0) {
    if (!recvInProgress) {
      if (STX == Serial2.read()) {
        recvInProgress = true;
        index = 0;
      }
    } else {
      index++;
      buffer[index] = Serial2.read();
      if (index >= 29) {
        recvInProgress = false;
        parseMessage(buffer);
      }
    }
  }

  if (buttonPressed) {
    buttonPressed = false;
    esp_err_t result = esp_now_send(indicatorMAC, (uint8_t *)&animalData, sizeof(animalData));
    if (result != ESP_OK) {
      messageStatus = 4;  // Error sending data
    } else {
      messageStatus = 1;
      lastMessageSent = millis();
      buzz(100);
    }
    buzzed = false;
    buzzedReceived = false;
    drawDisplay();
  }

  if ((messageStatus == 1 || messageStatus == 2) && millis() - lastMessageSent >= 45000) {
    messageStatus = 4;
    drawDisplay();
  }
}

void parseMessage(byte buffer[]) {
  char tagID[11];
  for (int i = 10; i >= 1; i--) {
    tagID[10 - i] = (char)buffer[i];
  }
  tagID[10] = '\0';
  char countryID[5];
  for (int j = 14; j >= 11; j--) {
    countryID[14 - j] = (char)buffer[j];
  }
  countryID[4] = '\0';
  long countryIDDec = strtol(countryID, NULL, 16);
  uint64_t tagIDDec = strtoull(tagID, NULL, 16);
  sprintf(animalData.tagId, "%ld%llu", countryIDDec, tagIDDec);
  buzz(150);
  Serial.println(animalData.tagId);
  lastMessageReceived = millis();
  if ((messageStatus == 0 || messageStatus >= 3) && (millis() - lastDisplayDrawed >= screenDrawGap)) {
    messageStatus = 0;
    drawDisplay();
  } else if ((messageStatus == 0 || messageStatus >= 3)) {
    delay(millis() - lastDisplayDrawed + 10);
    drawDisplay();
  }
}

void buzz(short delayMillis) {
  digitalWrite(buzzerPIN, HIGH);
  delay(delayMillis);
  digitalWrite(buzzerPIN, LOW);
}

void buttonReleaseInterrupt() {
  if ((messageStatus == 0 || messageStatus >= 3) && (millis() - lastButtonPressed >= 250)) {
    lastButtonPressed = millis();
    lastMessageReceived = millis();
    buttonPressed = true;
  }
}