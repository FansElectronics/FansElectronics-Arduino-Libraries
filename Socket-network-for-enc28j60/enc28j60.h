// Microchip ENC28J60 Ethernet Interface Driver
// Author: Pascal Stang 
// Modified by: Guido Socher
// Copyright: GPL V2
// 
// Changed by: Dov Rosnberg <dovrose@gmail.com>
// Date: 01/05/2013
// This driver provides initialization and transmit/receive
// functions for the Microchip ENC28J60 10Mb Ethernet Controller and PHY.
// This chip is novel in that it is a full MAC+PHY interface all in a 28-pin
// chip, using an SPI interface to the host processor.
//
// 2010-05-20 <jc@wippler.nl>

#ifndef ENC28J60_H
#define ENC28J60_H
#if ARDUINO >= 100
#include <Arduino.h> // Arduino 1.0
#else
#include <Wprogram.h> // Arduino 0022
#endif

  extern uint8_t buffer[];
  extern uint16_t bufferSize;
  
  extern uint8_t* tcpOffset ();

  extern void initSPI ();
  extern uint8_t initialize (const uint16_t size, const uint8_t* macaddr,
                             uint8_t csPin =8);
  extern bool isLinkUp ();
  
  extern void enc28j60PacketSend(uint16_t len, uint8_t* packet);
  extern uint16_t enc28j60PacketReceive(uint16_t maxlen, uint8_t* packet);
  
  extern void copyout (uint8_t page, const uint8_t* data);
  extern void copyin (uint8_t page, uint8_t* data);
  extern uint8_t peekin (uint8_t page, uint8_t off);

  extern void powerDown();  // contrib by Alex M.
  extern void powerUp();    // contrib by Alex M.
  
  extern void enableBroadcast();
  extern void disableBroadcast();

  extern uint8_t doBIST(uint8_t csPin =8);


#endif
