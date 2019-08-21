// Socket connection based on TCP/IP.
//
// Author: Dov Rosenebrg <dovrose@gmail.com>
// Date: 01/05/2013
//
// Copyright: GPL V2
// See http://www.gnu.org/licenses/gpl.html



#include <Arduino.h>
#include "Network.h"
#include "net.h"
#define MAX_ITERATIONS      1000 //need to chage to time out
#define buf net.buf
#define my_port net.my_port
#define destinationIp net.destinationIp
#define destinationPort net.destinationPort
#define clientState net.clientState
#define destinationMac net.destinationMac
#define clientState net.clientState
#define bytesToRead net.bytesToRead
#define sendPacketLength net.sendPacketLength

ServerCallback socketCallback;


enum CLIENT_STATE{NO_STATE, GOT_MAC, ARP_REQUEST_SENT, TCP_SYN_SENT, SOCK_ESTABLISHED};

void Network::socketSetup(ServerCallback callback, uint16_t myPort,uint8_t *serverIp, uint16_t serverPort){
	socketCallback = callback;
	my_port = myPort;
	net.copyIp(destinationIp,serverIp);
	destinationPort = serverPort;
}

bool Network::connect(){
	uint16_t i;
	//Sent ARP request.
//	net.make_arp_request(buf, destinationIp);
//	for (i = 0;clientState != GOT_MAC && i < MAX_ITERATIONS; i++) {
//		net.process(); //it'll fill dest_mac on socket struct
//		delay(10);
//	}
//	if (clientState != GOT_MAC) {
//		return 0;
//	}
	//MAC received, sending TCP SYN
	Serial.println("Send [SYN]");
	net.tcp_client_send_packet(buf, destinationPort, my_port, TCP_FLAG_SYN_V, 0, 0,destinationIp);
	clientState = net.TCP_SYN_SENT;
	//TCP SYN sent
	for (i = 0;clientState != SOCK_ESTABLISHED && i < MAX_ITERATIONS; i++) {
		net.process();
		delay(10);
	}

	return clientState == SOCK_ESTABLISHED;
	//TODO: Maybe use a timeout instead of MAX_ITERATIONS to receive SYN+ACK
}

uint16_t Network::send(const uint8_t *bufferToSend, uint16_t length) {
	//int packetLen= net.fill_tcp_data(buf,0, PSTR ( " " ) );
	uint16_t packetLen = 0;
	for(uint8_t j=0; j<length; j++){
		buf[TCP_DATA_P+j]=bufferToSend[j];
		packetLen++;
	}
	net.tcp_client_send_packet(buf, destinationPort, my_port, TCP_FLAG_ACK_V | TCP_FLAG_PUSH_V, packetLen, 0,destinationIp);
	sendPacketLength = packetLen;
   //client_process();
	return packetLen;
}

uint16_t Network::send(String s) {
	//int packetLen= net.fill_tcp_data(buf,0, PSTR ( " " ) );
	uint8_t len = s.length();
	char out[len];
	s.toCharArray(out,len);
	uint16_t packetLen = 0;
	for(uint8_t j=0; j<len; j++){
		buf[TCP_DATA_P+j]=out[j];
		packetLen++;
	}
	net.tcp_client_send_packet(buf, destinationPort, my_port, TCP_FLAG_ACK_V | TCP_FLAG_PUSH_V, packetLen, 0,destinationIp);
	sendPacketLength = packetLen;
   //client_process();
	return packetLen;
}



uint16_t Network::available() {
	uint16_t len = bytesToRead;
	if (bytesToRead != 0){
		socketCallback(destinationPort, buf + IP_SRC_P,(const char *) (buf + TCP_DATA_P),len);
		bytesToRead = 0;
		return len;
	}
	return 0;
}


 