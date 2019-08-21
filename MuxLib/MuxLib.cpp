/*----------------------------------------------------------------------------------------------------------------------
Multriplexer Module Arduino Library 
Copyright 2017 Irfan Indra Kurniawan >> http://wwww.Fans-Electronics.com <<

CHANGELOG
 > V1.0 - 8 February 2017
----------------------------------------------------------------------------------------------------------------------*/
#include <MuxLib.h>

// MUX
//----------------------------------------------------------------------------------------------------------------------
MUX::MUX(uint8_t pinOutput, uint8_t _pinA, uint8_t _pinB, uint8_t _pinC, bool _Mode) {
	Output = pinOutput;
	Mode = _Mode;
	pinA = _pinA;
	pinB = _pinB;
	pinC = _pinC;

	pinMode(pinA, OUTPUT);
	pinMode(pinB, OUTPUT);
	pinMode(pinC, OUTPUT);

}
// Read
//----------------------------------------------------------------------------------------------------------------------
uint16_t MUX::read(uint8_t numInput) {
	analogInput();
	inputValue[numInput];
}
// activeInput
//----------------------------------------------------------------------------------------------------------------------
void MUX::activeInput(uint8_t A, uint8_t B, uint8_t C) {
	digitalWrite(pinA,A);
	digitalWrite(pinB,B);
	digitalWrite(pinC,C);
}

// SelectInput
//----------------------------------------------------------------------------------------------------------------------
void MUX::analogInput() {
		activeInput(1,0,1);
  	inputValue[0] = analogRead(Output);
  	activeInput(1,1,1);
  	inputValue[1] = analogRead(Output);
  	activeInput(0,1,1);
  	inputValue[2] = analogRead(Output);
  	activeInput(0,1,0);
  	inputValue[3] = analogRead(Output);
  	activeInput(1,0,0);
  	inputValue[4] = analogRead(Output);
  	activeInput(1,0,0);
  	inputValue[5] = analogRead(Output);
  	activeInput(0,0,0);
  	inputValue[6] = analogRead(Output);
  	activeInput(1,1,0);
  	inputValue[7] = analogRead(Output);
}

void MUX::digitalInput() {
		activeInput(1,0,1);
  	inputValue[0] = digitalRead(Output);
  	activeInput(1,1,1);
  	inputValue[1] = digitalRead(Output);
  	activeInput(0,1,1);
  	inputValue[2] = digitalRead(Output);
  	activeInput(0,1,0);
  	inputValue[3] = digitalRead(Output);
  	activeInput(0,0,1);
  	inputValue[4] = digitalRead(Output);
  	activeInput(1,0,0);
  	inputValue[5] = digitalRead(Output);
  	activeInput(0,0,0);
  	inputValue[6] = digitalRead(Output);
  	activeInput(1,1,0);
  	inputValue[7] = digitalRead(Output);
}