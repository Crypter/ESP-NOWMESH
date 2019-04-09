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
  uint8_t SOS;
  uint8_t MANAGEMENT;
  String SOURCE;
  String DESTINATION;
  
  String DATAasString();
  
  uint8_t toRAW(uint8_t data[]);
  uint8_t sizeRAW();
  uint8_t fromRAW(const uint8_t data[], uint8_t size);
};

class NOWMESH_class {

public:
  NOWMESH_class();
  void begin(uint8_t channel=6);
  void setOnReceive(void (*function)(String source, String destination, uint8_t size, uint8_t data[]));
  void setOnSend(void (*function)(String source, String destination, uint8_t status));
  void send(String source, String destination, uint8_t data[], uint8_t size, uint8_t ACK=0, uint8_t SOS=0);
  void subscribe(String address);
  String ID();
  
private:
  void (*on_receive)(String source, String destination, uint8_t size, uint8_t data[]);
  void (*on_send)(String source, String destination, uint8_t status);

  void receive_data(const uint8_t *mac, const uint8_t *data, uint8_t len);
  void send_data(const uint8_t *mac, uint8_t status);
  
  uint8_t channel;
  

  LinkedList<NowMeshPacket> history;
  LinkedList<String> subscribed;
  
  
};

extern NOWMESH_class NowMesh;
#endif
