/*----------------------------------------------------------------------------------------------------------------------
Seven Segment 4 Digit 74HC595 Arduino Library 
Copyright 2017 Irfan Indra Kurniawan >> http://wwww.Fans-Electronics.com <<

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

CHANGELOG
 > V1.0 - 7 February 2017
----------------------------------------------------------------------------------------------------------------------*/


#include <SFE_SevSeg595.h>

// SevSeg595
//-------------------------------------------------------------------------tr---------------------------------------------
SevSeg595::SevSeg595(byte pinData, byte pinClock, byte pinLatch, byte pinDig1, byte pinDig2, byte pinDig3, byte pinDig4, bool divice)
{
  if(divice){
    for(uint8_t i=0; i<10; i++) {
      binSevSeg[i] = Trainer_binSevSeg[i];
    }
  }
  if(!divice){
    for(uint8_t i=0; i<10; i++) {
      binSevSeg[i] = Module_binSevSeg[i];
    }
  }
  Data = pinData;
  Clock = pinClock;
  Latch = pinLatch;
  Dig1 = pinDig1;
  Dig2 = pinDig2;
  Dig3 = pinDig3;
  Dig4 = pinDig4;
	pinMode(Data,OUTPUT);
	pinMode(Clock,OUTPUT);
	pinMode(Latch,OUTPUT);
	pinMode(Dig1,OUTPUT);
	pinMode(Dig2,OUTPUT);
	pinMode(Dig3,OUTPUT);
	pinMode(Dig4,OUTPUT);
}

// Void setNumber
//----------------------------------------------------------------------------------------------------------------------
void SevSeg595::setNumber(int numToShow)
{
	int d1,d2,d3,d4; // Temporary values for thousands, hundreds, tens and units
  if (numToShow > 9999)
    numToShow = 9999;  // Do some bounds checks to avoid strangeness
  if (numToShow < 0)
  	numToShow = 0;
  d1 = (int) (numToShow / 1000);    // Get thousands
  numToShow = numToShow - (d1 * 1000);
  d2 = (int) (numToShow / 100);     // Get hundreds
  numToShow = numToShow - (d2 * 100);
  d3 = (int) (numToShow / 10);      // Get tens
  d4 = numToShow - (d3* 10);        // Get units
  displayDigit(Dig1,byte(binSevSeg[d1]));  		// Show thousands
  displayDigit(Dig2,byte(binSevSeg[d2]));  		// Show hundreds
  displayDigit(Dig3,byte(binSevSeg[d3]));  		// Show tens
  displayDigit(Dig4,byte(binSevSeg[d4]));  		// Show units

}

// Void displayDigit
//----------------------------------------------------------------------------------------------------------------------
void SevSeg595::displayDigit(byte numDisplay, byte digitToDisplay){
  digitalWrite(Latch, LOW); 
  clear();  										// Turn off all cathode drivers.
  shiftOut(Data, Clock, LSBFIRST, digitToDisplay);  		// shift the bits out:
  digitalWrite(Latch, HIGH);  								// Set latch high to set segments. 
  digitalWrite(numDisplay, HIGH);  							// Drive one cathode low to turn on display. 
  delay(1);  											// Wait a bit for POV
}

// Void allDsiplayOff
//----------------------------------------------------------------------------------------------------------------------
void SevSeg595::clear(){
  // Matikan Semua Common Digit
  digitalWrite(Dig1, LOW);
  digitalWrite(Dig2, LOW);
  digitalWrite(Dig3, LOW);
  digitalWrite(Dig4, LOW);
}