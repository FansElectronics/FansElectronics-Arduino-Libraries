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

#include <Network.h>
#define BUFFER_SIZE 400

Network net;
uint8_t Network::buf[400];
uint8_t Network::mymac[6];  // my MAC address
uint8_t Network::myip[4];   // my ip address
uint8_t Network::mymask[4]; // my net mask
uint8_t Network::gwip[4];   // gateway
uint8_t Network::dhcpip[4]; // dhcp server
uint8_t Network::dnsip[4];  // dns server
uint8_t Network::hisip[4];  // dns result
uint16_t Network::my_port;   // socket port
uint8_t Network::destinationIp[4];
uint8_t Network::destinationMac[6];
uint16_t Network::destinationPort;
uint16_t Network::packetLength;
uint8_t Network::sendPacketLength;
uint8_t Network::bytesToRead;

byte Network::waitgwmac; // 0=wait, 1=first req no anser, 2=have gwmac, 4=refeshing but have gw mac, 8=accept an arp reply
#define WGW_INITIAL_ARP 1
#define WGW_HAVE_GW_MAC 2
#define WGW_REFRESHING 4
#define WGW_ACCEPT_ARP_REPLY 8


//uint16_t Network::hisport = 1000; // tcp port to browse to


void Network::begin (const uint8_t* macaddr) {
	copyMac(mymac, macaddr);
	//enc28j60Init(mymac);
	initialize(400,mymac,10);
	//enc28j60clkout(4); // change clkout from 6.25MHz to 12.5MHz
}

void Network::copyMac (byte *dst, const byte *src) {
    memcpy(dst, src, 6);
}

void Network::copyIp (byte *dst, const byte *src) {
    memcpy(dst, src, 4);
}

void Network::copyPort (byte *dst, const byte *src) {
    memcpy(dst, src, 4);
}

void Network::copy4byte (byte *dst, const byte *src) {
    memcpy(dst, src, 4);
}

void Network::setGwIp (const byte *gwipaddr) {
  waitgwmac = WGW_INITIAL_ARP; // causes an arp request in the process loop
  copyIp(gwip, gwipaddr);
}

byte Network::clientWaitingGw () {
  return !(waitgwmac & WGW_HAVE_GW_MAC);
}

void Network::process ( void )
{
	if (waitgwmac & WGW_INITIAL_ARP || waitgwmac & WGW_REFRESHING)
      client_arp_whohas(gwip);
	if (!(packetLength = enc28j60PacketReceive(BUFFER_SIZE, buf))) { //No packet available for reading!
		return;
    }
	//Recivong NTP
	if (ntpProcessAnswer(&timeStamp)) {
		sendTimeAndDate();
        return;
    }
	if(using_dhcp){
		net.DhcpStateMachine(packetLength);
	}
    if (eth_type_is_arp_and_my_ip(buf, packetLength)) {
		//this 'if' is for the socket connect
        if (arp_packet_is_myreply_arp(buf)) {
			if ((waitgwmac & WGW_ACCEPT_ARP_REPLY) && (buf[ETH_ARP_OPCODE_L_P]==ETH_ARP_OPCODE_REPLY_L_V) && client_store_gw_mac()){
				waitgwmac = WGW_HAVE_GW_MAC;
			}
			else{
				uint8_t i;
				// Received ARP reply
				for (i = 0; i < 6; i++) {
					destinationMac[i] = buf[ETH_SRC_MAC + i];
				}
				clientState = GOT_MAC;
			}
        }
        else {
			//Answering ARP request
            make_arp_answer_from_request(buf);

        }
		//if (buf[ETH_ARP_OPCODE_L_P]==ETH_ARP_OPCODE_REQ_L_V)
		//	make_arp_answer_from_request(buf);
		
	   
	}
	//Ignoring packet not for me
    else if (!eth_type_is_ip_and_my_ip(buf, packetLength)) {
		Serial.print(".");
        return;
    }
	//Replying ICMP ECHO REQUEST
    else if (buf[IP_PROTO_P] == IP_PROTO_ICMP_V &&
            buf[ICMP_TYPE_P] == ICMP_TYPE_ECHOREQUEST_V) {
        make_echo_reply_from_request(buf, packetLength);
    }
	// Recived UDP packet
	else if (net.udpServerListening() && buf[IP_PROTO_P]==IP_PROTO_UDP_V) {
		if(net.udpServerHasProcessedPacket())
			return;
	}
    else if (buf[IP_PROTO_P] == IP_PROTO_TCP_V) {
		//Received TCP packet

		//It is TCP SYNACK, sending ACK
        if (buf[TCP_FLAGS_P] == (TCP_FLAG_SYN_V | TCP_FLAG_ACK_V)) {
			tcp_client_send_packet(buf, destinationPort, my_port, TCP_FLAG_ACK_V, 0, destinationMac,destinationIp);
			clientState = SOCK_ESTABLISHED;
			
            //TODO: verify if I'm waiting for this SYN+ACK
            return;
        }
		//It is TCP SYN, sending SYNACK
		else if (buf[TCP_FLAGS_P] & TCP_FLAGS_SYN_V) {
			tcp_client_send_packet(buf, destinationPort, my_port,  TCP_FLAG_SYN_V|TCP_FLAG_ACK_V, 0, destinationMac,destinationIp);
			return;
        }
		//It is ACK
        else if (buf[TCP_FLAGS_P] & TCP_FLAGS_ACK_V) {
            uint16_t data;
            init_len_info(buf);
            data = get_tcp_data_pointer();
			//It is ACK with no data
            if (!data) {
				//It is ACKFIN, closing connection
				if (buf[TCP_FLAGS_P] & TCP_FLAGS_FIN_V) {
					tcp_client_send_packet(buf, destinationPort, my_port, TCP_FLAG_ACK_V, 1, destinationMac,destinationIp);
					sendPacketLength = 0;
                }

				//I send data And recived ack
				else if (sendPacketLength != 0) {
					save_ACKSEQ_from_send(buf);
					sendPacketLength = 0;
				} else {
					//need to reset the connection
					tcp_client_send_packet(buf, destinationPort, my_port, TCP_FLAG_RST_V, 0, destinationMac,destinationIp);
					sendPacketLength = 0;
					if (clientState == SOCK_ESTABLISHED){
						while(!connect()){};
					}
				}

                return;
            }
            else if (buf[TCP_FLAGS_P] & TCP_FLAG_PUSH_V){
				//It is ACK with data, ACK sent
                // send ACK to answer PSHACK from server
				tcp_client_send_packet(buf, destinationPort, my_port, TCP_FLAG_ACK_V, 0, destinationMac,destinationIp);
				int i, dataSize;
                dataSize = packetLength - (&buf[data] - buf);
                bytesToRead = dataSize;
				
                return;
            }
        } 
		else { 
		}
	}
	if (using_NTP) {
		ntpRequest(false);
	}
}


