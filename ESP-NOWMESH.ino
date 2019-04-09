#include "esp-nowmesh.h"

void message_received(NowMeshPacket &packet){
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


void setup() {
  Serial.begin(115200);
  delay(50);
  // put your setup code here, to run once:
NowMesh.begin();
NowMesh.setOnReceive(message_received);
NowMesh.subscribe("FF:FF:FF:FF:FF:FF");
}

void loop() {
  // put your main code here, to run repeatedly:
NowMesh.send(NowMesh.ID(), "FF:FF:FF:FF:FA:FE", (uint8_t*)"utnata", 7);
NowMesh.send(NowMesh.ID(), "FF:FF:FF:FF:FF:FF", (uint8_t*)"tocna", 6);
delay(esp_random()%1000);
}

