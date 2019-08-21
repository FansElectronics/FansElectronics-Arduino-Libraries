/*
 *  Nrf2401 library example 1 - TX
 *  March 2009 | jesse dot tane at gmail dot com
 *
 *  Send a 3 byte message every 2 seconds to the device at address 1.
 *
 */
 
#include "Nrf2401.h"
 
Nrf2401 Radio;
 
void setup(void)
{
  Radio.remoteAddress = 1;
  Radio.txMode(3);
  Radio.data[0] = 22;
  Radio.data[1] = 33;
  Radio.data[2] = 44;
}
 
void loop(void)
{
  Radio.write();
  delay(2000);
}
