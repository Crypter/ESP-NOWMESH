
#include <WiFi.h>
#include "esp-nowmesh.h"

void message_received(NowMeshPacket &packet){
#if defined (NOWMESH_DEBUG)
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
#endif
Serial.printf("RECV (%04X): ", packet.UID);
for (uint8_t i=0; i<packet.SIZE-1; i++){
    Serial.printf("%c", packet.DATA[i]);
  }
Serial.println();
}

void message_acked(String source, uint16_t UID){
#if defined (NOWMESH_DEBUG)
  Serial.println("NEW ACK ===");
  Serial.print("UID: "); Serial.printf("%04X", UID); Serial.println();
  Serial.print("SRC: "); Serial.println(source);
  Serial.println("= = = = = = = = =");
#endif
  Serial.printf("SEEN (%04X): %s\r\n", UID, source.c_str());
}

uint8_t button_state=1;
void setup() {
  
  Serial.begin(115200);
  delay(200);
  Serial.print("ID: ");
  Serial.println(NowMesh.ID());

//  if (NowMesh.ID().equals( "B4:E6:2D:B2:C6:0D")) WiFi.begin("Gajtanless", "google.com.mk");
//  WiFi.begin("Gajtanless", "google.com.mk");
  
  NowMesh.begin(11);
  NowMesh.setOnReceive(message_received);
  NowMesh.setOnACK(message_acked);
  NowMesh.subscribe("FF:FF:FF:FF:FF:FF");
  pinMode(0, INPUT); //boot button
}

void loop() {
String input;
input = Serial.readStringUntil('\n');

if (input.length()){
  Serial.printf("SENT (%04X): %s", NowMesh.send("FF:FF:FF:FF:FF:FF", (uint8_t*)input.c_str(), input.length()+1, true), input.c_str());
  Serial.println();
}
}
