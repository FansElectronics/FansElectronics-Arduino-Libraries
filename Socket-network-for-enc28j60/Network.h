// This code created from the EHTERSHIELD_H and EHTERCARD_H which are GPL v2.
// 		Modified by: Dov Rosenberg.
//		Created date: 28-04-2013
//		email: dovrose@gmail.com
//		This is GPL v2
//		See below the other pepole work.
// This code slightly follows the conventions of, but is not derived from:
//      EHTERSHIELD_H library for Arduino etherShield
//      Copyright (c) 2008 Xing Yu.  All right reserved. (this is LGPL v2.1)
// It is however derived from the enc28j60 and ip code (which is GPL v2)
//      Author: Pascal Stang 
//      Modified by: Guido Socher
//      DHCP code: Andrew Lindsay
// Hence: GPL V2
//
// 2010-05-19 <jc@wippler.nl>
//
// Changed by: Dov Rosenberg <dovrose@gmail.com>
// Date: 01/05/2013
//
// PIN Connections (Using Arduino UNO):
//   VCC -   3.3V
//   GND -    GND
//   SCK - Pin 13
//   SO  - Pin 12
//   SI  - Pin 11
//   CS  - Pin 10
//
#define __PROG_TYPES_COMPAT__

#ifndef Network_h
#define Network_h

#include <Arduino.h> // Arduino 1.0

#include <avr/pgmspace.h>
#include "enc28j60.h"
#include "net.h"
#define BUFFER_SIZE 400

typedef void (*ServerCallback)(
	uint16_t dest_port,	// the port the packet was sent to
	uint8_t src_ip[4],	// the ip of the sender
	const char *data,			// the data
	uint16_t len);		// the length of the data
	
typedef void (*NtpCallback)(
	uint8_t second,	
	uint8_t minute,	
	uint8_t hour,	
	uint8_t day,	
	uint8_t month,	
	uint16_t year	
);		
	
	
class Network {
public:
	static uint8_t buf[]; //buffer
	static uint8_t mymac[6];  // my MAC address
	static uint8_t myip[4];   // my ip address
	static uint8_t mymask[4]; // my net mask
	static uint8_t gwip[4];   // gateway
	static uint8_t dhcpip[4]; // dhcp server
	static uint8_t dnsip[4];  // dns server
	static uint8_t hisip[4];  // dns result
	//TCP socket
	static uint16_t my_port;   // socket port
	static uint8_t destinationIp[4];
	static uint8_t destinationMac[6];
	static uint16_t destinationPort;
	enum CLIENT_STATE{NO_STATE, GOT_MAC, ARP_REQUEST_SENT, TCP_SYN_SENT, SOCK_ESTABLISHED} clientState;
	static uint16_t packetLength;
	static uint8_t sendPacketLength;
	static uint8_t bytesToRead;
	bool using_dhcp;
	static byte waitgwmac;
	static byte ntpServer[4];
	static byte ntpPort;
	static bool using_NTP;
	static uint32_t timeStamp;

	
	// Network.cpp
	void begin (const uint8_t* macaddr);  
	void process ( void );
  	void copyMac (byte *dst, const byte *src);
	void copyIp (byte *dst, const byte *src);
	void copyPort (byte *dst, const byte *src);
	void setGwIp (const byte *gwipaddr);
	void copy4byte (byte *dst, const byte *src);
	byte clientWaitingGw ();

	// ip_arp_udp_tcp.cpp
	uint8_t eth_type_is_arp_and_my_ip(uint8_t *buf,uint16_t len);
	uint8_t arp_packet_is_myreply_arp ( uint8_t *buf );
	void make_arp_answer_from_request(uint8_t *buf);
	uint8_t eth_type_is_ip_and_my_ip(uint8_t *buf,uint16_t len);
	void make_echo_reply_from_request(uint8_t *buf,uint16_t len);
	void tcp_client_send_packet(uint8_t *buf,uint16_t dest_port, uint16_t src_port, uint8_t flags, uint16_t dlength, uint8_t *dest_mac, uint8_t *dest_ip);
	void init_len_info(uint8_t *buf);
	uint16_t get_tcp_data_pointer(void);
	void save_ACKSEQ_from_send(uint8_t *buf);
	void make_arp_request(uint8_t *buf, uint8_t *server_ip);
	uint16_t fill_tcp_data(uint8_t *buf,uint16_t pos, const char *s);
	void udpPrepare(word sport, byte *dip, word dport);
	void udpTransmit (word datalen);
	void client_arp_whohas(byte *ip_we_search);
	byte client_store_gw_mac();
	void sendUdp (uint8_t *data,byte datalen,word sport, byte *dip, word dport);
	void showIPsettings(void);
	bool ntpProcessAnswer (uint32_t *time);
	void ntpRequest(bool firstTime);
	
	//socket.cpp
	void socketSetup(ServerCallback callback,uint16_t myPort,uint8_t *serverIp, uint16_t serverPort);
	bool connect();
	uint16_t send(const uint8_t *bufferToSend, uint16_t length);
	uint16_t send(String s);
	uint16_t available();

	//UDPlistening
	void udpServerListenOnPort(ServerCallback callback, uint16_t port);
	bool udpServerHasProcessedPacket();
	bool udpServerListening();
	
	//DHCP
	bool dhcpSetup ();
	void DhcpStateMachine (word len);
	
	//NTP
	bool ntpBegin(NtpCallback callbackFunction, byte *ntpServerIp);
	void sendTimeAndDate();
	void setDLT(int32_t zone);
	
	//DNS
	bool dnsLookup (prog_char* name, bool fromRam=false);


};

extern Network net;

#endif
