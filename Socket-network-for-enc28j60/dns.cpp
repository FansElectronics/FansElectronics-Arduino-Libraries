// DNS look-up functions based on the udp client
// Changed by: Dov Rosenebrg <dovrose@gmail.com>
// Date: 01/05/2013
// Author: Guido Socher 
// Copyright: GPL V2
//
// 2010-05-20 <jc@wippler.nl>

#include "Network.h"
#include "net.h"

#define buf net.buf

static byte dnstid_l; // a counter for transaction ID
#define DNSCLIENT_SRC_PORT_H 0xE0 

static void dnsRequest (const prog_char *progmem_hostname, bool fromRam) {
  ++dnstid_l; // increment for next request, finally wrap
  if (net.dnsip[0] == 0)
    memset(net.dnsip, 8, 4); // use 8.8.8.8 Google DNS as default
  net.udpPrepare((DNSCLIENT_SRC_PORT_H << 8) | dnstid_l,
                                                net.dnsip, 53);
  memset(buf + UDP_DATA_P, 0, 12);
  
  byte *p = buf + UDP_DATA_P + 12;
  char c;
  do {
    byte n = 0;
    for(;;) {
      c = fromRam ? *progmem_hostname : pgm_read_byte(progmem_hostname);
      ++progmem_hostname;
      if (c == '.' || c == 0)
        break;
      p[++n] = c;
    }
    *p++ = n;
    p += n;
  } while (c != 0);
  
  *p++ = 0; // terminate with zero, means root domain.
  *p++ = 0;
  *p++ = 1; // type A
  *p++ = 0; 
  *p++ = 1; // class IN
  byte i = p - buf - UDP_DATA_P;
  buf[UDP_DATA_P] = i;
  buf[UDP_DATA_P+1] = dnstid_l;
  buf[UDP_DATA_P+2] = 1; // flags, standard recursive query
  buf[UDP_DATA_P+5] = 1; // 1 question
  net.udpTransmit(i);
}

static void checkForDnsAnswer (uint16_t plen) {
  byte *p = buf + UDP_DATA_P;
  if (plen < 70 || buf[UDP_SRC_PORT_L_P] != 53 ||
                   buf[UDP_DST_PORT_H_P] != DNSCLIENT_SRC_PORT_H ||
                   buf[UDP_DST_PORT_L_P] != dnstid_l ||
                   p[1] != dnstid_l ||
                   (p[3] & 0x0F) != 0) 
    return;

  p += *p; // we encoded the query len into tid
  for (;;) {
    if (*p & 0xC0)
      p += 2;
    else
      while (++p < buf + plen) {
        if (*p == 0) {
          ++p;
          break;
        }
      }
    if (p + 14 > buf + plen)
      break;
    if (p[1] == 1 && p[9] == 4) { // type "A" and IPv4
      net.copyIp(net.hisip, p + 10);
      break;
    }
    p += p[9] + 10;
  }
}

// use during setup, as this discards all incoming requests until it returns
bool Network::dnsLookup (prog_char* name, bool fromRam) {
  word start = millis();
  while (clientWaitingGw()) {
    net.process();
    if ((word) (millis() - start) >= 30000)
      return false;
  }
    
  memset(net.hisip, 0, 4);
  dnsRequest(name, fromRam);

  start = millis();
  while (net.hisip[0] == 0) {
    if ((word) (millis() - start) >= 30000)
      return false;
    word len = enc28j60PacketReceive(BUFFER_SIZE, buf);
    if (len > 0 )
      checkForDnsAnswer(len);
  }

  return true;
}
