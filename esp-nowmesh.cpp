
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
  
  uint8_t NowMeshPacket::toRAW(uint8_t data[]){
    memset(data, 0, this->SIZE+16);
    stringToMac(this->DESTINATION, data);
    stringToMac(this->SOURCE, data+6);
    data[12] = TTL;
    data[13] = UID >> 8;
    data[14] = UID & 0xFF;
    data[15] = (ACK&1)<<7 | (ANS&1)<<6 | (MNG&1)<<5;
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
    UID = (uint16_t) data[13]<<8 | data[14];
    ACK = (data[15]>>7) & 1;
    ANS = (data[15]>>6) & 1;
    MNG = (data[15]>>5) & 1;
    SIZE = size-16;
    DATA = (uint8_t*)malloc(SIZE);
    memcpy(DATA, data+16, SIZE);
    return this->SIZE;
  }
  
  NowMeshPacket::~NowMeshPacket(){
//    if (this->DATA) free(this->DATA);
  }


// END NOWMESH PACKET

NOWMESH_class::ReceivedDataFunction NOWMESH_class::on_received = 0;
NOWMESH_class::SentDataFunction NOWMESH_class::on_sent = 0;
NOWMESH_class::ACKDataFunction NOWMESH_class::on_ack = 0;
uint8_t NOWMESH_class::channel = 0;

LinkedList<NowMeshPacket> NOWMESH_class::history;
LinkedList<String> NOWMESH_class::subscribed;

void NOWMESH_class::begin(uint8_t channel) {
  WiFi.enableSTA(true);
//  WiFi.mode(WIFI_STA);
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
  
	xTaskCreatePinnedToCore(
		reinterpret_cast<TaskFunction_t>(&NOWMESH_class::timesync),   /* Function to implement the task */
		"timesync", /* Name of the task */
		5000,      /* Stack size in words */
		0,       /* Task input parameter */
		1,          /* Priority of the task */
		NULL,       /* Task handle. */
		APP_CPU_NUM);  /* Core where the task should run */
  
}

void NOWMESH_class::timesync( void * parameter ) {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
	for (;;){
      uint64_t millisepoch;
      struct timeval tv;
      gettimeofday(&tv, NULL);
      while(tv.tv_sec<1500000000){
        delay(5000);
        gettimeofday(&tv, NULL);
      }
      millisepoch = ((uint64_t)tv.tv_sec * 1000 + (tv.tv_usec / 1000));
//      Serial.println(tv.tv_sec);
//      Serial.printf("%08X%08X\n", (uint32_t)((uint64_t)millisepoch>>32), (uint32_t)(millisepoch)&0xFFFFFFFF);
    NowMeshPacket new_packet;
    new_packet.SIZE = 9;
    new_packet.ACK = 0;
    new_packet.ANS = 0;
    new_packet.MNG = 1;
    new_packet.SOURCE = NOWMESH_class::ID();
    new_packet.DESTINATION = "FF:FF:FF:FF:FF:FF";
    new_packet.UID = esp_random();
    new_packet.TTL = 15;

    uint8_t data[9];
    data[0]='s'; //sync
    for (int i=0; i<8; i++){
      data[i+1] = millisepoch>>(i*8);
    }
    new_packet.DATA = data;
  
    NOWMESH_class::packet_send(new_packet);
    delay(60000);
	}
}

void NOWMESH_class::setOnReceive(ReceivedDataFunction function) {
  on_received = function;
}

void NOWMESH_class::setOnSend(SentDataFunction function) {
  on_sent = function;
}

void NOWMESH_class::setOnACK(ACKDataFunction function) {
  on_ack = function;
}

uint16_t NOWMESH_class::send(String service, uint8_t data[], uint8_t size, uint8_t ACK){
  NowMeshPacket new_packet;
  new_packet.SIZE = size;
  new_packet.ACK = ACK;
  new_packet.ANS = 0;
  new_packet.MNG = 0;
  new_packet.SOURCE = NOWMESH_class::ID();
  new_packet.DESTINATION = service;
  new_packet.UID = esp_random();
  new_packet.TTL = 15;
  new_packet.DATA = data;

  NOWMESH_class::packet_send(new_packet);
  return new_packet.UID;
}

void NOWMESH_class::packet_send(NowMeshPacket &packet){

	packet.TIMESTAMP = millis();

	uint8_t *data_temp;
	data_temp = packet.DATA;
	packet.DATA = 0;
	history.add(packet);
	packet.DATA = data_temp;

	uint8_t broadcastAddr[6]={0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	uint8_t packetData[packet.sizeRAW()];
	packet.toRAW(packetData);

	esp_now_send(broadcastAddr, packetData, packet.sizeRAW());
}


void NOWMESH_class::subscribe(String address){
  uint16_t i;
  for (i=0; i<subscribed.size(); i++)
    if (subscribed.get(i).equals(address)) break;
  
  if (subscribed.size() == i)
    subscribed.add(address);
}

String NOWMESH_class::ID() {
    char mac[18];
    uint64_t fuseMac = ESP.getEfuseMac();
    sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X", (uint8_t)(fuseMac>>0)&0xFF, (uint8_t)(fuseMac>>8)&0xFF, (uint8_t)(fuseMac>>16)&0xFF, (uint8_t)(fuseMac>>24)&0xFF, (uint8_t)(fuseMac>>32)&0xFF, (uint8_t)(fuseMac>>40)&0xFF );
    return mac;
}

void NOWMESH_class::packet_repeat(NowMeshPacket &packet){
  if (packet.TTL==0 || packet.DESTINATION == NOWMESH_class::ID()) return;
  Serial.printf("RPT: %04X\n", packet.UID);
  packet.TTL--;
    
  NOWMESH_class::packet_send(packet);
  
  packet.TTL++;
}


void NOWMESH_class::packet_ack(NowMeshPacket &packet){
  if (packet.ACK==0 || packet.ANS==1) return;

  if (packet.ACK==1 && packet.ANS==0){
    Serial.printf("ACK: %04X\n", packet.UID);
    
    NowMeshPacket response;
    response.SIZE = 2;
    response.ACK = 1;
    response.ANS = 1;
    response.MNG = 0;
    response.SOURCE = NOWMESH_class::ID();
    response.DESTINATION = packet.SOURCE;
    response.UID = esp_random();
    response.TTL = 15; //Should I really define this?
    response.TIMESTAMP = millis();
    
	uint8_t data[2];
	data[0] = packet.UID >> 8;
	data[1] = packet.UID & 0xFF;
    response.DATA = data;
    
      NOWMESH_class::packet_send(response);

  }
  
}
void NOWMESH_class::packet_mng(NowMeshPacket &packet){
   //TODO
}

void NOWMESH_class::receive_data(const uint8_t *mac, const uint8_t *data, uint8_t len){
  NowMeshPacket new_packet;
  new_packet.fromRAW(data, len);
  
  Serial.println();
  Serial.printf("NEW: %04X\n", new_packet.UID);

  uint16_t i;
  for (i=0; i<history.size(); i++){
      
    while (i<history.size() && millis() - history.get(i).TIMESTAMP > 20000) {
  Serial.printf("REM: %04X\n", history.get(i).UID);
      history.remove(i);
    }
      Serial.printf("CMP: %04X | %04X\n", new_packet.UID, history.get(i).UID);

    if ( i<history.size() && 
      history.get(i).UID == new_packet.UID &&
      history.get(i).SOURCE == new_packet.SOURCE &&
      history.get(i).DESTINATION == new_packet.DESTINATION &&
      history.get(i).ACK == new_packet.ACK &&
      history.get(i).ANS == new_packet.ANS &&
      history.get(i).MNG == new_packet.MNG){
          Serial.printf("DUP: %04X\n", history.get(i).UID);
          return; //duplicate packet, ignore silently
      }
      
  }
  
  uint8_t* newData = new_packet.DATA;
  new_packet.DATA=0;
  new_packet.TIMESTAMP = millis();
  history.add(new_packet); //Without DATA
  new_packet.DATA=newData;

  packet_repeat(new_packet);

  if (new_packet.MNG) {
    packet_mng(new_packet);
  } else if (new_packet.ANS==1 && new_packet.SIZE==2){
      if (on_ack){
        uint16_t new_UID = (uint16_t) new_packet.DATA[0]<<8 | new_packet.DATA[1];
        for (i=0; i<history.size(); i++){
          if (history.get(i).UID == new_UID) on_ack(new_packet.SOURCE, new_UID);
        }
    }
  } else {
    for (i=0; i<subscribed.size(); i++)
      if (subscribed.get(i).equals(new_packet.DESTINATION)){
		packet_ack(new_packet);
//        if (on_received) on_received(new_packet);
        break;
      }
  }
  on_received(new_packet); //DEBUG MODE
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
