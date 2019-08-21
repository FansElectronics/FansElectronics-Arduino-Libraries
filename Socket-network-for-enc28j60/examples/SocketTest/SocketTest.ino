//test for the network lib
#include <Network.h>
#include <MemoryFree.h>
#include <SPI.h>

static uint8_t mymac[6] = {0x54,0x55,0x58,0x10,0x00,0x24}; 
//static uint8_t myip[4] = {192,168,1,106};
//static uint8_t mygw[4] = {192,168,1,1};
//static uint8_t mydns[4] = {192,168,1,1};

char ntpSite[] PROGMEM = "timeserver.iix.net.il";
//static byte ntpServer[4] = {192,114,62,250};


static uint16_t my_port = 11200;     // client port
static uint16_t server_port = 1000;
static uint8_t dest_ip[4]={192,168,1,105};

//ntp stuff
uint32_t timeStamp;

#define INTERVAL                10000
unsigned long lastTime = 0;

void tcpSerialPrint(uint16_t port,byte ip[4], const char *data, word len) {
  Serial.print("port: ");Serial.print(port);
  
  Serial.print(ip[0]);Serial.print(":");
  Serial.print(ip[1]);Serial.print(":");
  Serial.print(ip[2]);Serial.print(":");
  Serial.println(ip[3]);
  Serial.println(data);
  Serial.println(len);
}

void ntpSerialPrint(uint8_t second,uint8_t minute,uint8_t hour,uint8_t day,uint8_t month,uint16_t year){
	 Serial.print(hour);Serial.print(":");
	 Serial.print(minute);Serial.print(":");
	 Serial.println(second);
	 Serial.print(day);Serial.print("/");
	 Serial.print(month);Serial.print("/");
	 Serial.println(year);
}


void setup(){
	Serial.begin(57600);
	Serial.println("Started");
	
	net.begin(mymac);
	net.dhcpSetup ();
	
	//Start socket
	net.socketSetup(tcpSerialPrint,my_port,dest_ip,server_port);
	while (!net.connect()) {}
	Serial.println("socket connected");
	
	//check for DNS
	if (!net.dnsLookup(ntpSite))
		Serial.println("DNS failed");
	
	//start NTP
	net.ntpBegin(ntpSerialPrint,net.hisip);
	Serial.println("Idel");

}

int count = 0;
long lastchange = 0;

void loop(){
	net.process();
    
	if (net.available()) {
		count++;
	}
	
	if (count == 5) {
		uint8_t temp2[] = "Mad_Men";
		net.send(temp2,7);
		count = 0;
		net.sendUdp(temp2,7,1001,dest_ip,11201);
	}
	
	if(millis()-lastchange>30000) {
		Serial.print("free Memory()="); Serial.println(freeMemory());

		Serial.println("send..... ");
		lastchange=millis();
		uint8_t temp[] = "Hello";
		net.send(temp,5);
	}
}

