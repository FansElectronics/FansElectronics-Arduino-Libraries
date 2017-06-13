/*-------------------------------------------
 * >> Examples Display Number
 * >> By Irfan Indra Kurniawan
--------------------------------------------*/

#include <SFE_SevSeg595.h>
// konfigurasi pin Modul
// SevSeg595 Segmen(data,clock,latch,dig1,dig2,dig3,dig4,DIVICE);
SevSeg595 Segmen(6,7,8,5,4,3,2,TRAINER);

void setup() {

}
void loop() {
  for(int i = 0; i < 9999; i++){
    for (int j=0; j<50; j++) // delay counter untuk geser data.
    // Tampilkan Angka, wajib looping
    Segmen.setNumber(i);
    // Perhatian, jangan gunakan Delay..
  }  
}