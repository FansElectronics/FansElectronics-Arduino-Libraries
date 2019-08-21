/*
 *  Nrf2401.h
 *  A simplistic interface for using Sparkfun's Nrf2401A breakout boards with Arduino
 *  Original code for http://labs.ideo.com by Jesse Tane March 2009
 *
 *  License:
 *  --------
 *  This is free software. You can redistribute it and/or modify it under
 *  the terms of Creative Commons Attribution 3.0 United States License. 
 *  To view a copy of this license, visit http://creativecommons.org/licenses/by/3.0/us/ 
 *  or send a letter to Creative Commons, 171 Second Street, Suite 300, San Francisco, California, 94105, USA.
 *
 *  Notes:
 *  ------
 *  For documentation on how to use this library, please visit http://www.arduino.cc/playground/Main/InterfacingWithHardware
 *  Pin connections should be as follows for Arduino:
 *
 *  DR1 = 2  (digital pin 2)
 *  CE  = 3
 *  CS  = 4
 *  CLK = 5
 *  DAT = 6
 *
 */

#include <avr/io.h>
#include <util/delay.h>

#define NRF2401_BUFFER_SIZE 25

#define DR1 4
#define CE  8
#define CS  16
#define CLK 32
#define DAT 64

#define SELECT_CHIP   PORTD |= CS
#define DESELECT_CHIP PORTD &= ~CS
#define ENABLE_CHIP   PORTD |= CE
#define DISABLE_CHIP  PORTD &= ~CE
#define CYCLE_CLOCK   PORTD |= CLK, _delay_us(1), PORTD &= ~CLK
#define TX_DATA_HI    PORTD |= DAT
#define TX_DATA_LO    PORTD &= ~DAT
#define RX_DATA_HI    (PIND & DAT)
#define DATA_READY    (PIND & DR1)

class Nrf2401
{
  public:
  
  // properties
  
  volatile unsigned char data[NRF2401_BUFFER_SIZE];
  volatile unsigned int remoteAddress;
  volatile unsigned int localAddress;
  volatile unsigned char dataRate;
  volatile unsigned char channel;
  volatile unsigned char power;
  volatile unsigned char mode;
  
  // methods
  
  Nrf2401(void);
  void rxMode(unsigned char messageSize=0);
  void txMode(unsigned char messageSize=0);
  void write(unsigned char dataByte);
  void write(unsigned char* dataBuffer=0);
  void read(unsigned char* dataBuffer=0);
  bool available(void);

  // you shouldn't need to use anything below this point..
  
  volatile unsigned char payloadSize;
  volatile unsigned char configuration[15];
  void configure(void);
  void loadConfiguration(bool modeSwitchOnly=false);
  void loadByte(unsigned char byte);
};
