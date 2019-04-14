
#include <WiFi.h>
#include "esp-nowmesh.h"

void message_received(NowMeshPacket &packet){
  Serial.println("NEW MSG ===");
  Serial.print("SRC: "); Serial.println(packet.SOURCE);
  Serial.print("DST: "); Serial.println(packet.DESTINATION);
  Serial.print("UID: "); Serial.printf("%04X", packet.UID); Serial.println();
  Serial.print("ACK: "); Serial.println(packet.ACK);
  Serial.print("ANS: "); Serial.println(packet.ANS);
  Serial.print("MNG: "); Serial.println(packet.MNG);
  Serial.print("TTL: "); Serial.println(packet.TTL);
  Serial.print("TME: "); Serial.println(packet.TIMESTAMP);
  Serial.print("LEN: "); Serial.println(packet.SIZE);
  Serial.println("DATA: = = = =");
  for (uint8_t i=0; i<packet.SIZE-1; i++){
    Serial.printf("%02X ", packet.DATA[i]);
  }
    Serial.printf("%02X", packet.DATA[packet.SIZE-1]);
    Serial.println();
  Serial.println("= = = = = = = = =");
}

void message_acked(String source, uint16_t UID){
  Serial.println("NEW ACK ===");
  Serial.print("UID: "); Serial.printf("%04X", UID); Serial.println();
  Serial.print("SRC: "); Serial.println(source);
  Serial.println("= = = = = = = = =");
}

uint8_t button_state=1;
void setup() {
  
  Serial.begin(115200);
  delay(200);
  Serial.print("ID: ");
  Serial.println(NowMesh.ID());

  //if (NowMesh.ID().equals( "B4:E6:2D:B2:C6:0D")) WiFi.begin("Gajtanless", "google.com.mk");
  
  NowMesh.begin(6);
  NowMesh.setOnReceive(message_received);
  NowMesh.setOnACK(message_acked);
  NowMesh.subscribe("FF:FF:FF:FF:FF:FF");
  pinMode(0, INPUT); //boot button
}

void loop() {
  if(digitalRead(0)!=button_state){
    if (digitalRead(0)==0){
      Serial.print("SENT: ");
      Serial.printf("%04X", NowMesh.send("FF:FF:FF:FF:FF:FF", (uint8_t*)"01234", 5, true));
      Serial.println();
    }
    button_state=digitalRead(0);
  }
}
