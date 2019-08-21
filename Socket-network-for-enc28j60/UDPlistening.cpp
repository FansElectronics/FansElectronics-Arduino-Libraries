// Simple UDP listening server
//
// Author: Brian Lee
//
// Changed By: Dov Rosenberg <dovrose@gmail.com>
// Date: 01/05/2013
//
// Copyright: GPL V2
// See http://www.gnu.org/licenses/gpl.html

#include <Arduino.h>
#include "Network.h"
#include "net.h"

#define buf net.buf
#define UDPSERVER_MAXLISTENERS 2	//the maximum number of port listeners.

typedef struct {
	ServerCallback callback;
	uint16_t port;
	bool listening;
} UdpServerListener;

UdpServerListener listeners[UDPSERVER_MAXLISTENERS];
byte numListeners = 0;

void Network::udpServerListenOnPort(ServerCallback callback, uint16_t port) {
	if(numListeners < UDPSERVER_MAXLISTENERS)
	{
		listeners[numListeners] = (UdpServerListener){callback, port, true};
		numListeners++;
	}
}

bool Network::udpServerListening() {
	return numListeners > 0;
}

bool Network::udpServerHasProcessedPacket() {
	bool packetProcessed = false;
	for(uint8_t i = 0; i < numListeners; i++)
	{
		if(buf[UDP_DST_PORT_H_P] == (listeners[i].port >> 8) && buf[UDP_DST_PORT_L_P] == ((byte) listeners[i].port) && listeners[i].listening)
		{
			uint16_t datalen = (uint16_t) (buf[UDP_LEN_H_P] << 8)  + buf[UDP_LEN_L_P] - UDP_HEADER_LEN;
			listeners[i].callback(
				listeners[i].port,
				buf + IP_SRC_P,
				(const char *) (buf + UDP_DATA_P),
				datalen);
			packetProcessed = true;
		}
	}
	return packetProcessed;
}

