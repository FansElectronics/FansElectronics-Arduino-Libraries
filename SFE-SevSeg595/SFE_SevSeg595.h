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

#ifndef SFE_SevSeg595_h
#define SFE_SevSeg595_h
#define TRAINER true
#define MODULE false
#include <arduino.h>

class SevSeg595 {
	public:
		SevSeg595(byte pinData, byte pinClock, byte pinLatch, byte pinDig1, byte pinDig2, byte pinDig3, byte pinDig4, bool divice);
		void clear();
		void setNumber(int numToShow);

	private:

		void displayDigit(byte digitNumber, byte digitToDisplay);
		int Data;
		int Clock;
		int Latch;
		int Dig1;
		int Dig2;
		int Dig3;
		int Dig4;

		byte binSevSeg[10];
		// Data Bit Untuk SFE 7Segmen Module
		byte Module_binSevSeg[10] = { B00010100, B11110110, B00011010, B01010010, B11110000, B01010001, B00010001, B01110110, B00010000, B01010000,};
        // Data Bit Untuk SFE Trainer Arduino				  
        byte Trainer_binSevSeg[10] = { B00000011, B10011111, B00100101, B00001101, B10011001, B01001001, B01000001, B00011111, B00000001, B00001001,};
       	
} ;

#endif // SFE_SevSeg595_h