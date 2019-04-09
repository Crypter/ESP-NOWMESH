#include "esp-nowmesh.h"

void setup() {
  // put your setup code here, to run once:
NowMesh.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
NowMesh.send("test", "ing", (uint8_t*)"poraka", 7);
delay(20);
}

