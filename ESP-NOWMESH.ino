#include "esp-nowmesh.h"

void message_received(NowMeshPacket &packet){
  return;
  Serial.print("NEW MSG | ");
  Serial.println(millis());
  Serial.print("SRC: "); Serial.println(packet.SOURCE);
  Serial.print("DST: "); Serial.println(packet.DESTINATION);
  Serial.print("UID: "); Serial.println(packet.UID);
  Serial.print("ACK: "); Serial.println(packet.ACK);
  Serial.print("SOS: "); Serial.println(packet.SOS);
  Serial.print("MNG: "); Serial.println(packet.MNG);
  Serial.print("LEN: "); Serial.println(packet.SIZE);
  Serial.println((char *)packet.DATA);
  Serial.println("= = = = = = = = =");
}

uint8_t button_state=1;
void setup() {
  Serial.begin(115200);
  delay(50);
  // put your setup code here, to run once:
NowMesh.begin(3);
NowMesh.setOnReceive(message_received);
NowMesh.subscribe("FF:FF:FF:FF:FF:FF");
pinMode(0, INPUT); //boot button
}

void loop() {
  if(digitalRead(0)!=button_state){
    if (digitalRead(0)==0){
      Serial.println("SENDING-START");
      NowMesh.send(NowMesh.ID(), "FF:FF:FF:FF:FF:FFh", (uint8_t*)"tocna", 6);
      Serial.println("SENDING-DONE");
    }
    button_state=digitalRead(0);
  }
}

