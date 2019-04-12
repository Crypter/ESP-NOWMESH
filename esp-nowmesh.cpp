
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
    memset(data, 0, this->SIZE+16);
    stringToMac(this->DESTINATION, data);
    stringToMac(this->SOURCE, data+6);
    data[12] = TTL;
    data[13] = UID>>8;
    data[14] = UID & 8 ;
    data[15] = (ACK&1)<<7 | (SOS&1)<<6 | (MNG&1)<<5;
    memcpy(data+16, this->DATA, this->SIZE);
    return this->SIZE+16;
  }
  
  uint8_t NowMeshPacket::sizeRAW(){
    return this->SIZE+16;
  }
  
  uint8_t NowMeshPacket::fromRAW(const uint8_t data[], uint8_t size){
    DESTINATION = macToString(data);
    SOURCE = macToString(data+6);
    TTL = data[12];
    UID = (uint16_t)data[13]<<8 | data[14];
    ACK = (data[15]>>7) & 1;
    SOS = (data[15]>>6) & 1;
    MNG = (data[15]>>5) & 1;
    SIZE = size-16;
    DATA = (uint8_t*)malloc(SIZE);
    memcpy(DATA, data+16, SIZE);
    
    return this->SIZE;
  }
  
  NowMeshPacket::~NowMeshPacket(){
    if (this->DATA) free(this->DATA);
  }


// END NOWMESH PACKET

NOWMESH_class::ReceivedDataFunction NOWMESH_class::on_received = 0;
NOWMESH_class::SentDataFunction NOWMESH_class::on_sent = 0;
uint8_t NOWMESH_class::channel = 0;

LinkedList<NowMeshPacket> NOWMESH_class::history;
LinkedList<String> NOWMESH_class::subscribed;

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
  NOWMESH_class::channel = channel;
  brcst.channel = channel;
  brcst.ifidx = ESP_IF_WIFI_STA;
  esp_now_add_peer(&brcst);
  
}

void NOWMESH_class::setOnReceive(ReceivedDataFunction function) {
  on_received = function;
}

void NOWMESH_class::setOnSend(SentDataFunction function) {
  on_sent = function;
}

void NOWMESH_class::send(String source, String destination, uint8_t data[], uint8_t size, uint8_t ACK, uint8_t SOS){
  NowMeshPacket new_packet;
  new_packet.SIZE = size;
  new_packet.ACK = ACK;
  new_packet.SOS = SOS;
  new_packet.MNG = 0;
  new_packet.SOURCE = source;
  new_packet.DESTINATION = destination;
  new_packet.UID=esp_random(); //TODO NTP ?
  new_packet.TTL= (new_packet.SOS ? 255:15); //Should I really define this?
  new_packet.DATA = data;
  
  uint8_t broadcastAddr[6]={0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  uint8_t packetData[new_packet.sizeRAW()];
  new_packet.toRAW(packetData);
  
  esp_now_send(broadcastAddr, packetData, new_packet.sizeRAW());
  new_packet.DATA = 0; //DATA not needed
  new_packet.TIMESTAMP = esp_timer_get_time();
  history.add(new_packet);
  
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

void NOWMESH_class::packet_repeat(NowMeshPacket &packet){
  if (packet.TTL==0) return;
  Serial.print("REPEATING:"); Serial.println(packet.UID);
//  Serial.print("SRC:  "); Serial.println(packet.SOURCE);
//  Serial.print("DST:  "); Serial.println(packet.DESTINATION);
//  Serial.print("TTL:"); Serial.println(packet.TTL); 
//  Serial.print("DATA: "); Serial.println((char*)packet.DATA);
  packet.TTL--;
    
  uint8_t broadcastAddr[6]={0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  uint8_t packetData[packet.sizeRAW()];
  packet.toRAW(packetData);
  
  esp_now_send(broadcastAddr, packetData, packet.sizeRAW());
  
  packet.TTL++;
}
void NOWMESH_class::receive_data(const uint8_t *mac, const uint8_t *data, uint8_t len){
  NowMeshPacket new_packet;
  new_packet.fromRAW(data, len);
  

  Serial.println("NEW PACKET!!!");
  Serial.print("TTL: "); Serial.println(new_packet.TTL);
  Serial.print("UID: "); Serial.println(new_packet.UID);
  Serial.print("TIME: "); Serial.println(new_packet.TIMESTAMP);
  Serial.println("END OF ENTRY");

Serial.print("HISTORY SIZE: ");
Serial.println(history.size());
  uint16_t i;
  for (i=0; i<history.size(); i++){
//  Serial.print("HISTORY POSITION: "); Serial.println(i);
//  Serial.print("TTL: "); Serial.println(history.get(i).TTL);
//  Serial.print("UID: "); Serial.println(history.get(i).UID);
//  Serial.print("TIME: "); Serial.println(history.get(i).TIMESTAMP);
//  Serial.println("END OF ENTRY");
  
    while (i<history.size() && esp_timer_get_time() - history.get(i).TIMESTAMP > 20000) {
      Serial.print("REMOVING: "); Serial.println(history.get(i).UID);
      history.remove(i);
    }
    if ( i<history.size() && 
      history.get(i).UID == new_packet.UID &&
      history.get(i).SOURCE == new_packet.SOURCE &&
      history.get(i).DESTINATION == new_packet.DESTINATION &&
      history.get(i).ACK == new_packet.ACK &&
      history.get(i).SOS == new_packet.SOS &&
      history.get(i).MNG == new_packet.MNG){
        Serial.print("DUPLICATE: "); Serial.println(history.get(i).UID);
          return; //duplicate packet, ignore silently
      }
      
  }
  
  uint8_t* newData = new_packet.DATA;
  new_packet.DATA=0;
  new_packet.TIMESTAMP = esp_timer_get_time();
  history.add(new_packet); //Without DATA
  new_packet.DATA=newData;
  packet_repeat(new_packet);

  for (i=0; i<subscribed.size(); i++)
    if (subscribed.get(i).equals(new_packet.DESTINATION)){
      if (on_received) on_received(new_packet);
      break;
    }
//TODO:    if (new_packet.ACK) send_ACK(new_packet);
  
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
  String temp = mac;
  temp.replace(":","");
  temp.replace("-","");
  temp.trim();
  temp.toUpperCase();
  for (int i=0; i<6; i++) {
      result[i] = ((temp[i*2] > '9') ? (temp[i*2]-'A'+0xA):(temp[i*2]-'0')) << 4 | ((temp[i*2+1] > '9') ? (temp[i*2+1]-'A'+0xA):(temp[i*2+1]-'0'));
  }
}

NOWMESH_class NowMesh;
