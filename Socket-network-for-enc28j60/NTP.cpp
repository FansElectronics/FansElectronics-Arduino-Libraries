// NTP look-up functions based on the udp client
//
// Author: Dov Rosenebrg <dovrose@gmail.com>
// Date: 01/05/2013
//
// Copyright: GPL V2
// See http://www.gnu.org/licenses/gpl.html
#include "net.h"
#include "enc28j60.h"
#include "Network.h"

#define SECONDS_IN_DAY          86400
#define START_YEAR              1900
static int8_t TIME_ZONE = 3;
static uint8_t days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
const char ntpreqhdr[] PROGMEM = { 0xE3,0,4,0xFA,0,1,0,0,0,1 };
const char iphdr[] PROGMEM = { 0x45,0,0,0x82,0,0,0x40,0,0x20 };

byte Network::ntpServer[4];
byte Network::ntpPort;
bool Network::using_NTP = false;
uint32_t Network::timeStamp;

NtpCallback Callback;


bool Network::ntpBegin(NtpCallback callbackFunction, byte *ntpServerIp){
	net.copyIp(ntpServer, ntpServerIp);
	Callback = callbackFunction;
	ntpPort = 0;
	using_NTP = true;
	ntpRequest(true);
	return true;
}


boolean isLeapYear(unsigned int year) {
  
  return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}


void Network::sendTimeAndDate() {
  
  unsigned int year = START_YEAR;
  timeStamp = timeStamp + 3600 * TIME_ZONE;
  while(1) {
    uint32_t seconds;
    if(isLeapYear(year)) seconds = SECONDS_IN_DAY * 366;
    else seconds = SECONDS_IN_DAY * 365;
    if(timeStamp >= seconds) {
      timeStamp -= seconds;
      year++;
    } else break;
  }
  
  unsigned int month = 0;
  while(1) {    
    uint32_t seconds = SECONDS_IN_DAY * days_in_month[month];
    if(isLeapYear(year) && month == 1) seconds = SECONDS_IN_DAY * 29;
    if(timeStamp >= seconds) {
      timeStamp -= seconds;
      month++;
    } else break;
  }  
  month++;
  
  unsigned int day = 1;
  while(1) {
    if(timeStamp >= SECONDS_IN_DAY) {
      timeStamp -= SECONDS_IN_DAY;
      day++;
    } else break;
  }  

  uint8_t hour = timeStamp / 3600;
  //uint8_t minute = (timeStamp - (uint32_t)hour * 3600) / 60;
 // uint8_t second = (timeStamp - (uint32_t)hour * 3600) - minute * 60;
  Callback((timeStamp - (uint32_t)hour * 3600) - ((timeStamp - (uint32_t)hour * 3600) / 60) * 60, (timeStamp - (uint32_t)hour * 3600) / 60, hour, day, month, year);

}

void Network::setDLT(int32_t zone){
	TIME_ZONE = zone;
}



