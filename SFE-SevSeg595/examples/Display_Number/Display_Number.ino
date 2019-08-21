/*-------------------------------------------
 * >> Examples Display Number
 * >> By Irfan Indra Kurniawan
--------------------------------------------*/

#include<SFE_SevSeg595.h>
// konfigurasi pin Modul
// SevSeg595 Segmen(data,clock,latch,dig1,dig2,dig3,dig4,DIVICE);
SevSeg595 Segmen(5,7,6,1,2,3,4,MODULE);

void setup() { 

}
void loop() {
  // Tampilkan Angka, wajib looping
  Segmen.setNumber(1412);
  // Perhatian, jangan gunakan Delay..
}
