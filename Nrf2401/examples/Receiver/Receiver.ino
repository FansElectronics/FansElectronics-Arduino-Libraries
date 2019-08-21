/*
 *  Nrf2401 library example 1 - RX
 *  March 2009 | jesse dot tane at gmail dot com
 *
 *  Listen for incoming messages targeting the device's local address
 *  of 3 byte length, whose contents equal 22, 33 and 44 respectively.
 *  If such a message is encountered, blink an LED on pin 13 for half a second.
 *
 */
 
#include "Nrf2401.h"
 
Nrf2401 Radio;
 
void setup(void)
{
  pinMode(13, OUTPUT);
  Radio.localAddress = 1;
  Radio.rxMode(3);
}
 
void loop(void)
{
  while(!Radio.available());
  Radio.read();
  if(Radio.data[0] == 22 && Radio.data[1] == 33 && Radio.data[2] == 44)
  {
    digitalWrite(13, HIGH);
    delay(500);
    digitalWrite(13, LOW);
  }
}
