#ifndef ESPNOWMESH_H
#define ESPNOWMESH_H

#include <stdint.h>
#include <WString.h>
#include "LinkedList.h"

String macToString(const uint8_t* mac);
void stringToMac(const String mac, uint8_t result[]);

class NowMeshPacket {
  public:
  NowMeshPacket();
  ~NowMeshPacket();
  uint8_t *DATA;
  uint8_t SIZE;
  uint8_t TTL;
  uint16_t UID;
  uint8_t ACK;
  uint8_t ANS;
  uint8_t MNG;
  String SOURCE;
  String DESTINATION;

  uint32_t TIMESTAMP;
  
  uint8_t toRAW(uint8_t data[]);
  uint8_t sizeRAW();
  uint8_t fromRAW(const uint8_t data[], uint8_t size);
};

class NOWMESH_class {

public:
  typedef void (*ReceivedDataFunction)(NowMeshPacket &packet);
  typedef void (*SentDataFunction)(String source, String destination, uint8_t status);
  typedef void (*ACKDataFunction)(String source, uint16_t UID);

  static void begin(uint8_t channel=6);
  static void setOnReceive(ReceivedDataFunction function);
  static void setOnSend(SentDataFunction function);
  static void setOnACK(ACKDataFunction function);
  static void packet_send(NowMeshPacket &packet);
  static uint16_t send(String service, uint8_t data[], uint8_t size, uint8_t ACK = 0);
  static void subscribe(String address);
  static String ID();
  static uint8_t channel;
  
private:
  static ReceivedDataFunction on_received;
  static SentDataFunction on_sent;
  static ACKDataFunction on_ack;

  static void receive_data(const uint8_t *mac, const uint8_t *data, uint8_t len);
  static void send_data(const uint8_t *mac, uint8_t status);
  static void packet_repeat(NowMeshPacket &packet);
  static void packet_ack(NowMeshPacket &packet);
  static void packet_mng(NowMeshPacket &packet);
  static void timesync( void * parameter );
  
//  static uint8_t channel;

  static LinkedList<NowMeshPacket> history;
  static LinkedList<String> subscribed;
  
  
};

extern NOWMESH_class NowMesh;
#endif
