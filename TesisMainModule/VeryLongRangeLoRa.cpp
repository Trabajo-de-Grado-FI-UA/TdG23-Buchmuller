#include "VeryLongRangeLoRa.h"
#include "LoRaWan_APP.h"

#define LORA_SPREADING_FACTOR 10  // [SF7..SF12]
#define LORA_CODINGRATE 1
#define LORA_PREAMBLE_LENGTH 8  // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT 0   // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define SEND_INTERVAL 3000
#define MAX_SEND_INTENTS 6

void (*onMessageReceived)(uint64_t senderID, Message message, int16_t rssi);
void (*onMessageSent)(uint64_t receiverID, byte messageID, bool received, byte intents);

uint64_t myDeviceID;
int16_t rssi, rxSize;

Package rxPackage;
Package txPackage;
Message rxMessage;

bool sendPackage;
bool sendNow;
bool sendingConfirmationPackage;
byte sendIntentsDone;
long lastSendIntentMillis;
long lastPackageReceivedMillis;
bool packageReceived;

static RadioEvents_t RadioEvents;

VeryLongRangeLoRa::VeryLongRangeLoRa() {
  // Constructor
  pinMode(LED, OUTPUT);
}

void OnTxDone(void) {
  Serial.println("TX done");
  Radio.Rx(0);
  if (!sendingConfirmationPackage) {
    sendIntentsDone++;
    lastSendIntentMillis = millis();
    if (sendIntentsDone < MAX_SEND_INTENTS) {
      sendPackage = true;
    } else {
      digitalWrite(LED, LOW);
    }
    onMessageSent(txPackage.receiverID, txPackage.payload[0], false, sendIntentsDone);
  } else {
    Serial.println("Confirmation sent");
  }
}

void OnTxTimeout(void) {
  Serial.println("TX Timeout......");
}

uint8_t* packageToByteArray(const Package& package) {
  uint8_t* byteArray = new uint8_t[MESSAGE_PAYLOAD_SIZE + 17];
  memcpy(byteArray, &package, MESSAGE_PAYLOAD_SIZE + 17);
  return byteArray;
}

uint8_t* createConfirmationPackage(const uint64_t& senderID) {
  uint8_t* byteArray = new uint8_t[8];
  memcpy(byteArray, &senderID, 8);
  return byteArray;
}

void OnRxDone(uint8_t* payload, uint16_t size, int16_t rssi, int8_t snr) {
  rssi = rssi;
  rxSize = size;
  memcpy(&rxPackage, payload, size);
  if (size == 8) {
    Serial.print("Confirmation received. Is it mine? ");
    if (!packageReceived && rxPackage.receiverID == myDeviceID) {
      digitalWrite(LED, LOW);
      packageReceived = true;
      sendPackage = false;
      Serial.println("YES");
      onMessageSent(txPackage.receiverID, txPackage.payload[0], true, sendIntentsDone);
    } else {
      Serial.println("NO");
    }
  } else if (rxPackage.receiverID == myDeviceID) {
    Serial.println("Received Message");
    lastPackageReceivedMillis = millis();
    sendingConfirmationPackage = true;
    Serial.println("Sending confirmation...");
    Radio.Send(createConfirmationPackage(rxPackage.senderID), 8);
    memcpy(&rxMessage, &rxPackage.payload, MESSAGE_PAYLOAD_SIZE + 1);
    onMessageReceived(rxPackage.senderID, rxMessage, rssi);
  }
}

void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void VeryLongRangeLoRa::init(long frequency, byte txPower, byte bandwidth) {
  Serial.println("Initializing VLRLoRa...");
  VextON();
  Mcu.begin();
  myDeviceID = ESP.getEfuseMac();
  randomSeed(analogRead(0));
  Serial.printf("My device ID=%04X", (uint16_t)(myDeviceID >> 32));  //print High 2 bytes
  Serial.printf("%08X\n", (uint32_t)myDeviceID);
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  RadioEvents.RxDone = OnRxDone;
  Radio.Init(&RadioEvents);
  Radio.SetChannel(frequency);
  Radio.SetTxConfig(MODEM_LORA, txPower, 0, bandwidth,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                    true, 0, 0, LORA_IQ_INVERSION_ON, 3000);
  Radio.SetRxConfig(MODEM_LORA, bandwidth, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true);
  Radio.Rx(0);
  Serial.println("VLRLoRa Initialized");
}

void VeryLongRangeLoRa::sendMessage(uint64_t receiverID, Message message) {
  txPackage.receiverID = receiverID;
  txPackage.senderID = myDeviceID;
  memcpy(&txPackage.payload, &message, MESSAGE_PAYLOAD_SIZE + 1);
  sendPackage = true;
  sendIntentsDone = 0;
  packageReceived = false;
  sendNow = true;
}

void VeryLongRangeLoRa::setOnMessageReceived(void (*callback)(uint64_t senderID, Message message, int16_t rssi)) {
  Serial.println("Setting setOnMessageReceived");
  onMessageReceived = callback;
}

void VeryLongRangeLoRa::setOnMessageSent(void (*callback)(uint64_t receiverID, byte messageID, bool received, byte intents)) {
  Serial.println("Setting setOnMessageSent");
  onMessageSent = callback;
}

void VeryLongRangeLoRa::process() {
  Radio.IrqProcess();
  if (!packageReceived && sendPackage && (sendNow || (millis() - lastSendIntentMillis >= SEND_INTERVAL)) && (millis() - lastPackageReceivedMillis >= SEND_INTERVAL * 1.5)) {
    digitalWrite(LED, HIGH);
    Serial.println("Enviando...");
    Serial.print("Intento: ");
    Serial.println(sendIntentsDone);
    sendPackage = false;
    sendingConfirmationPackage = false;
    sendNow = false;
    Radio.Send(packageToByteArray(txPackage), MESSAGE_PAYLOAD_SIZE + 17);
  }
}
