
#if defined(ESP32)
#include <WiFi.h>
#include <esp_now.h>
#else
#error "This library supports ESP32 only."
#endif

#include "esp-nowmesh.h"


// START NOWMESH PACKET

  NowMeshPacket::NowMeshPacket(){
  }

  String NowMeshPacket::DATAasString(){
    uint8_t temp[this->SIZE+1];
    memcpy(temp, DATA, SIZE);
    temp[SIZE]='\0';
    return String((char *)temp);
  }
  
  uint8_t NowMeshPacket::toRAW(uint8_t data[]){
    memset(data, 0, this->SIZE+17);
    stringToMac(this->DESTINATION, data);
    stringToMac(this->SOURCE, data+6);
    data[12] = TTL;
    data[13] = UID>>8;
    data[14] = UID & 8 ;
    data[15] = (ACK&1)<<7 | (SOS&1)<<6 | (MANAGEMENT&1)<<7 ;
    memcpy(data+16, this->DATA, this->SIZE);
    return this->SIZE+17;
  }
  
  uint8_t NowMeshPacket::sizeRAW(){
    return this->SIZE+17;
  }
  
  uint8_t NowMeshPacket::fromRAW(const uint8_t data[], uint8_t size){
    this->DESTINATION = macToString(data);
    this->SOURCE = macToString(data+6);
    TTL = data[12];
    UID = (uint16_t)data[13]<<8 | data[14];
    ACK = (data[15]>>7) & 1;
    SOS = (data[15]>>6) & 1;
    MANAGEMENT = (data[15]>>5) & 1;
    this->SIZE = size-17;
    this->DATA = (uint8_t*)malloc(this->SIZE);
    memcpy(this->DATA, data+16, this->SIZE);
    return this->SIZE;
  }
  
  NowMeshPacket::~NowMeshPacket(){
    if (this->DATA) free(this->DATA);
  }


// END NOWMESH PACKET






NOWMESH_class::NOWMESH_class(){
  this->on_receive=0;
  this->on_send=0;
  this->channel=0;
}

void NOWMESH_class::begin(uint8_t channel) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_now_init();
  esp_now_register_send_cb(reinterpret_cast<esp_now_send_cb_t>(&NOWMESH_class::send_data));
  esp_now_register_recv_cb(reinterpret_cast<esp_now_recv_cb_t>(&NOWMESH_class::receive_data));

  uint8_t broadcastAddr[6]={0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_peer_info_t brcst;
  memset(&brcst, 0, sizeof(brcst));
  memcpy(brcst.peer_addr, broadcastAddr, ESP_NOW_ETH_ALEN);
  brcst.channel = this->channel;
  brcst.ifidx = ESP_IF_WIFI_STA;
  esp_now_add_peer(&brcst);
  
}

void NOWMESH_class::setOnReceive(void (*function)(String source, String destination, uint8_t size, uint8_t data[])) {
  this->on_receive = function;
}

void NOWMESH_class::setOnSend(void (*function)(String source, String destination, uint8_t status)) {
  this->on_send = function;
}

void NOWMESH_class::send(String source, String destination, uint8_t data[], uint8_t size, uint8_t ACK, uint8_t SOS){
  NowMeshPacket new_packet;
  new_packet.SIZE = size;
  new_packet.ACK = ACK;
  new_packet.SOS = SOS;
  new_packet.SOURCE = source;
  new_packet.DESTINATION = destination;
  new_packet.UID=esp_random(); //TODO NTP ?
  new_packet.TTL= (new_packet.SOS ? 255:16); //Should I really define this?
  new_packet.DATA = data;
  
  uint8_t broadcastAddr[6]={0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  uint8_t packetData[new_packet.sizeRAW()];
  new_packet.toRAW(packetData);
  
  esp_now_send(broadcastAddr, packetData, new_packet.sizeRAW());
  new_packet.DATA=0; //DATA not needed
  this->history.add(new_packet);
  
}

void NOWMESH_class::subscribe(String address){
  uint16_t i;
  for (i=0; i<subscribed.size(); i++)
    if (subscribed.get(i).equals(address)) break;
  
  if (subscribed.size() == i)
    subscribed.add(address);
}

String NOWMESH_class::ID() {
    return WiFi.macAddress();
}

void NOWMESH_class::receive_data(const uint8_t *mac, const uint8_t *data, uint8_t len){
  NowMeshPacket new_packet;
  new_packet.fromRAW(data, len);
  
  uint8_t* newData = new_packet.DATA;
  new_packet.DATA=0;
  history.add(new_packet); //Without DATA
  new_packet.DATA=newData;

  uint16_t i;
  for (i=0; i<subscribed.size(); i++)
    if (subscribed.get(i).equals(new_packet.DESTINATION)) break;
  
  if (subscribed.size() != i) {
//TODO:    if (new_packet.ACK) send_ACK(new_packet);
    if (this->on_receive) on_receive(new_packet.SOURCE, new_packet.DESTINATION, new_packet.SIZE, new_packet.DATA);
  } else {
//TODO:    this->forward(new_packet);
  }
    
}
void NOWMESH_class::send_data(const uint8_t *mac, uint8_t status){
  
}


String macToString(const uint8_t* mac) {
    char buf[20];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(buf);
}

void stringToMac(const String mac, uint8_t result[]){
  for (int i=0; i<6; i++) {
      result[i] = ((mac[i*2] > '9') ? (mac[i*2]-'A'+0xA):(mac[i*2]-'0')) << 4 | ((mac[i*2+1] > '9') ? (mac[i*2+1]-'A'+0xA):(mac[i*2+1]-'0'));
  }
}

NOWMESH_class NowMesh;
