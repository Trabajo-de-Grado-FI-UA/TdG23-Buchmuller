#ifndef VeryLongRangeLoRa_h
#define VeryLongRangeLoRa_h

#define MESSAGE_PAYLOAD_SIZE 18

#include "Arduino.h"

struct Message {
  byte id;
  byte payload[MESSAGE_PAYLOAD_SIZE];
};

struct Package {
  uint64_t receiverID;
  uint64_t senderID;
  uint8_t payload[MESSAGE_PAYLOAD_SIZE + 1];
};

struct ConfirmationPackage {
  uint64_t receiverID;
};

class VeryLongRangeLoRa {
public:
  VeryLongRangeLoRa();

  void init(long frequency, byte txPower, byte bandwidth);
  void sendMessage(uint64_t receiverID, Message message);
  void setOnMessageReceived(void (*callback)(uint64_t senderID, Message message, int16_t rssi));
  void setOnMessageSent(void (*callback)(uint64_t receiverID, byte messageID, bool received, byte intents));
  void process();
};

#endif