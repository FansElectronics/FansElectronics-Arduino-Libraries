/*----------------------------------------------------------------------------------------------------------------------
Multriplexer Module Arduino Library 
Copyright 2017 Irfan Indra Kurniawan >> http://wwww.Fans-Electronics.com <<

CHANGELOG
 > V1.0 - 8 February 2017
----------------------------------------------------------------------------------------------------------------------*/

#ifndef MuxLib_h
#define MuxLib_h

#define ANALOG_MODE false
#define DIGITAL_MODE true

#include <arduino.h>

class MUX {
	public:
		MUX(uint8_t _Output, uint8_t _pinA, uint8_t _pinB, uint8_t _pinC, bool _Mode);
		uint16_t read(uint8_t numInput);

	private:
		void activeInput(uint8_t A, uint8_t B, uint8_t C);
		void analogInput();
		void digitalInput();
		uint8_t pinA, pinB, pinC, Output;
		bool Mode;
		uint16_t inputValue[8];
} ;

#endif // MuxLib_h