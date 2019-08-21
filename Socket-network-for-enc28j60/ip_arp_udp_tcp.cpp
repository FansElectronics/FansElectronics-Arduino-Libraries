/*********************************************
 * vim:sw=8:ts=8:si:et
 * To use the above modeline in vim you must have "set modeline" in your .vimrc
 * Changed by: Dov Rosenberg
 * Date: 01/05/2013
 * Author: Guido Socher 
 * Copyright: GPL V2
 * See http://www.gnu.org/licenses/gpl.html
 *
 * IP, Arp, UDP and TCP functions.
 *
 * The TCP implementation uses some size optimisations which are valid
 * only if all data can be sent in one single packet. This is however
 * not a big limitation for a microcontroller as you will anyhow use
 * small web-pages. The TCP stack is therefore a SDP-TCP stack (single data packet TCP).
 *
 * Chip type           : ATMEGA88 with ENC28J60
 *********************************************/
 /*********************************************
 * Modified: nuelectronics.com -- Ethershield for Arduino
 *********************************************/
#include <avr/io.h>
#include <avr/pgmspace.h>
//#include "avr_compat.h"
#include "net.h"

#include "enc28j60.h"

#include "Network.h"
static uint8_t wwwport=80;
#define macaddr net.mymac
//static uint8_t macaddr[6];
//static uint8_t ipaddr[4];
#define ipaddr net.myip
#define WGW_ACCEPT_ARP_REPLY 8
static int16_t INTERVAL = 43200;
//#define INTERVAL 43200
static unsigned long lastTime = 0;
static bool requestSent = true;

static int16_t info_hdr_len=0;
static int16_t info_data_len=0;
static uint8_t seqnum=0xa; // my initial tcp sequence number
static uint8_t temp[4];
static byte gwmacaddr[6];

const byte allOnes[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
const char iphdr[] PROGMEM = { 0x45,0,0,0x82,0,0,0x40,0,0x20 };
const char arpreqhdr[] PROGMEM = { 0,1,8,0,6,4,0,1 };
const char ntpreqhdr[] PROGMEM = { 0xE3,0,4,0xFA,0,1,0,0,0,1 };


static uint8_t ack[4];
static uint8_t seq[4];

static uint8_t lastack[4];
static uint8_t lastseq[4];
static uint8_t sendData = 0;


enum SOCKET_STATE{ACK,ACK_PSH,SYN,SYN_ACK,FIN,ACK_FIN};
static SOCKET_STATE state;

// The Ip checksum is calculated over the ip header only starting
// with the header length field and a total length of 20 bytes
// unitl ip.dst
// You must set the IP checksum field to zero before you start
// the calculation.
// len for ip is 20.
//
// For UDP/TCP we do not make up the required pseudo header. Instead we 
// use the ip.src and ip.dst fields of the real packet:
// The udp checksum calculation starts with the ip.src field
// Ip.src=4bytes,Ip.dst=4 bytes,Udp header=8bytes + data length=16+len
// In other words the len here is 8 + length over which you actually
// want to calculate the checksum.
// You must set the checksum field to zero before you start
// the calculation.
// len for udp is: 8 + 8 + data length
// len for tcp is: 4+4 + 20 + option len + data length
//
// For more information on how this algorithm works see:
// http://www.netfor2.com/checksum.html
// http://www.msc.uky.edu/ken/cs471/notes/chap3.htm
// The RFC has also a C code example: http://www.faqs.org/rfcs/rfc1071.html
uint16_t checksum(uint8_t *buf, uint16_t len,uint8_t type){
        // type 0=ip 
        //      1=udp
        //      2=tcp
        uint32_t sum = 0;

        //if(type==0){
        //        // do not add anything
        //}
        if(type==1){
                sum+=IP_PROTO_UDP_V; // protocol udp
                // the length here is the length of udp (data+header len)
                // =length given to this function - (IP.scr+IP.dst length)
                sum+=len-8; // = real tcp len
        }
        if(type==2){
                sum+=IP_PROTO_TCP_V; 
                // the length here is the length of tcp (data+header len)
                // =length given to this function - (IP.scr+IP.dst length)
                sum+=len-8; // = real tcp len
        }
        // build the sum of 16bit words
        while(len >1){
                sum += 0xFFFF & (*buf<<8|*(buf+1));
                buf+=2;
                len-=2;
        }
        // if there is a byte left then add it (padded with zero)
        if (len){
                sum += (0xFF & *buf)<<8;
        }
        // now calculate the sum over the bytes in the sum
        // until the result is only 16bit long
        while (sum>>16){
                sum = (sum & 0xFFFF)+(sum >> 16);
        }
        // build 1's complement:
        return( (uint16_t) sum ^ 0xFFFF);
}

// you must call this function once before you use any of the other functions:
void init_ip_arp_udp_tcp(uint8_t *mymac,uint8_t *myip,uint8_t wwwp){
        uint8_t i=0;
        wwwport=wwwp;
        while(i<4){
                ipaddr[i]=myip[i];
                i++;
        }
        i=0;
        while(i<6){
                macaddr[i]=mymac[i];
                i++;
        }
}

uint8_t Network::eth_type_is_arp_and_my_ip(uint8_t *buf,uint16_t len){
        uint8_t i=0;
        //  
        if (len<41){
                return(0);
        }
        if(buf[ETH_TYPE_H_P] != ETHTYPE_ARP_H_V || 
           buf[ETH_TYPE_L_P] != ETHTYPE_ARP_L_V){
                return(0);
        }
        while(i<4){
                if(buf[ETH_ARP_DST_IP_P+i] != ipaddr[i]){
                        return(0);
                }
                i++;
        }
        return(1);
}

uint8_t Network::eth_type_is_ip_and_my_ip(uint8_t *buf,uint16_t len){
        uint8_t i=0;
		uint8_t myPacket = 1;
        //eth+ip+udp header is 42
        if (len<42){
                return(0);
        }
        if(buf[ETH_TYPE_H_P]!=ETHTYPE_IP_H_V || 
           buf[ETH_TYPE_L_P]!=ETHTYPE_IP_L_V){
                return(0);
        }
        if (buf[IP_HEADER_LEN_VER_P]!=0x45){
                // must be IP V4 and 20 byte header
                return(0);
        }
        while(i<4){
                if(buf[IP_DST_P+i]!=ipaddr[i]){
                        return(0);
                }
                i++;
        }
        return(1);
}
// make a return eth header from a received eth packet
void make_eth(uint8_t *buf)
{
        uint8_t i=0;
        //
        //copy the destination mac from the source and fill my mac into src
        while(i<6){
                buf[ETH_DST_MAC +i]=buf[ETH_SRC_MAC +i];
                buf[ETH_SRC_MAC +i]=macaddr[i];
                i++;
        }
}

// make a new eth header for IP packet
void make_eth_ip_new(uint8_t *buf, uint8_t* dst_mac)
{
        uint8_t i=0;
        //
        //copy the destination mac from the source and fill my mac into src
        while(i<6){
                buf[ETH_DST_MAC +i]=dst_mac[i];
                buf[ETH_SRC_MAC +i]=macaddr[i];
                i++;
        }
		
		buf[ ETH_TYPE_H_P ] = ETHTYPE_IP_H_V;
		buf[ ETH_TYPE_L_P ] = ETHTYPE_IP_L_V;
}


void fill_ip_hdr_checksum(uint8_t *buf)
{
        uint16_t ck;
        // clear the 2 byte checksum
        buf[IP_CHECKSUM_P]=0;
        buf[IP_CHECKSUM_P+1]=0;
        buf[IP_FLAGS_P]=0x40; // don't fragment
        buf[IP_FLAGS_P+1]=0;  // fragement offset
        buf[IP_TTL_P]=64; // ttl
        // calculate the checksum:
        ck=checksum(&buf[IP_P], IP_HEADER_LEN,0);
        buf[IP_CHECKSUM_P]=ck>>8;
        buf[IP_CHECKSUM_P+1]=ck& 0xff;
}

static uint16_t ip_identifier = 1;

// make a new ip header for tcp packet

// make a return ip header from a received ip packet
void make_ip_tcp_new(uint8_t *buf, uint16_t len,uint8_t *dst_ip)
{
    uint8_t i=0;
		
	// set ipv4 and header length
	buf[ IP_P ] = IP_V4_V | IP_HEADER_LENGTH_V;

	// set TOS to default 0x00
	buf[ IP_TOS_P ] = 0x00;

	// set total length
	buf[ IP_TOTLEN_H_P ] = (len >>8)& 0xff;
	buf[ IP_TOTLEN_L_P ] = len & 0xff;
	
	// set packet identification
	buf[ IP_ID_H_P ] = (ip_identifier >>8) & 0xff;
	buf[ IP_ID_L_P ] = ip_identifier & 0xff;
	ip_identifier++;
	
	// set fragment flags	
	buf[ IP_FLAGS_H_P ] = 0x00;
	buf[ IP_FLAGS_L_P ] = 0x00;
	
	// set Time To Live
	buf[ IP_TTL_P ] = 128;
	
	// set ip packettype to tcp/udp/icmp...
	buf[ IP_PROTO_P ] = IP_PROTO_TCP_V;
	
	// set source and destination ip address
        while(i<4){
                buf[IP_DST_P+i]=dst_ip[i];
                buf[IP_SRC_P+i]=ipaddr[i];
                i++;
        }
        fill_ip_hdr_checksum(buf);
}


// make a return ip header from a received ip packet
void make_ip(uint8_t *buf)
{
        uint8_t i=0;
        while(i<4){
                buf[IP_DST_P+i]=buf[IP_SRC_P+i];
                buf[IP_SRC_P+i]=ipaddr[i];
                i++;
        }
        fill_ip_hdr_checksum(buf);
}

// make a return tcp header from a received tcp packet
// rel_ack_num is how much we must step the seq number received from the
// other side. We do not send more than 255 bytes of text (=data) in the tcp packet.
// If mss=1 then mss is included in the options list
//
// After calling this function you can fill in the first data byte at TCP_OPTIONS_P+4
// If cp_seq=0 then an initial sequence number is used (should be use in synack)
// otherwise it is copied from the packet we received
void make_tcphead(uint8_t *buf,uint16_t rel_ack_num,uint8_t mss,uint8_t cp_seq)
{
        uint8_t i=0;
        uint8_t tseq;
        while(i<2){
                buf[TCP_DST_PORT_H_P+i]=buf[TCP_SRC_PORT_H_P+i];
                buf[TCP_SRC_PORT_H_P+i]=0; // clear source port
                i++;
        }
        // set source port  (http):
        buf[TCP_SRC_PORT_L_P]=wwwport;
        i=4;
        // sequence numbers:
        // add the rel ack num to SEQACK
        while(i>0){
                rel_ack_num=buf[TCP_SEQ_H_P+i-1]+rel_ack_num;
                tseq=buf[TCP_SEQACK_H_P+i-1];
                buf[TCP_SEQACK_H_P+i-1]=0xff&rel_ack_num;
                if (cp_seq){
                        // copy the acknum sent to us into the sequence number
                        buf[TCP_SEQ_H_P+i-1]=tseq;
                }else{
                        buf[TCP_SEQ_H_P+i-1]= 0; // some preset vallue
                }
                rel_ack_num=rel_ack_num>>8;
                i--;
        }
        if (cp_seq==0){
                // put inital seq number
                buf[TCP_SEQ_H_P+0]= 0;
                buf[TCP_SEQ_H_P+1]= 0;
                // we step only the second byte, this allows us to send packts 
                // with 255 bytes or 512 (if we step the initial seqnum by 2)
                buf[TCP_SEQ_H_P+2]= seqnum; 
                buf[TCP_SEQ_H_P+3]= 0;
                // step the inititial seq num by something we will not use
                // during this tcp session:
                seqnum+=2;
        }
        // zero the checksum
        buf[TCP_CHECKSUM_H_P]=0;
        buf[TCP_CHECKSUM_L_P]=0;

        // The tcp header length is only a 4 bit field (the upper 4 bits).
        // It is calculated in units of 4 bytes. 
        // E.g 24 bytes: 24/4=6 => 0x60=header len field
        //buf[TCP_HEADER_LEN_P]=(((TCP_HEADER_LEN_PLAIN+4)/4)) <<4; // 0x60
        if (mss){
                // the only option we set is MSS to 1408:
                // 1408 in hex is 0x580
                buf[TCP_OPTIONS_P]=2;
                buf[TCP_OPTIONS_P+1]=4;
                buf[TCP_OPTIONS_P+2]=0x05; 
                buf[TCP_OPTIONS_P+3]=0x80;
                // 24 bytes:
                buf[TCP_HEADER_LEN_P]=0x60;
        }else{
                // no options:
                // 20 bytes:
                buf[TCP_HEADER_LEN_P]=0x50;
        }
}




void Network::make_arp_answer_from_request(uint8_t *buf)
{
        uint8_t i=0;
        //
        make_eth(buf);
        buf[ETH_ARP_OPCODE_H_P]=ETH_ARP_OPCODE_REPLY_H_V;
        buf[ETH_ARP_OPCODE_L_P]=ETH_ARP_OPCODE_REPLY_L_V;
        // fill the mac addresses:
        while(i<6){
                buf[ETH_ARP_DST_MAC_P+i]=buf[ETH_ARP_SRC_MAC_P+i];
                buf[ETH_ARP_SRC_MAC_P+i]=macaddr[i];
                i++;
        }
        i=0;
        while(i<4){
                buf[ETH_ARP_DST_IP_P+i]=buf[ETH_ARP_SRC_IP_P+i];
                buf[ETH_ARP_SRC_IP_P+i]=ipaddr[i];
                i++;
        }
        // eth+arp is 42 bytes:
        enc28j60PacketSend(42,buf); 
}

void Network::make_echo_reply_from_request(uint8_t *buf,uint16_t len)
{
        make_eth(buf);
        make_ip(buf);
        buf[ICMP_TYPE_P]=ICMP_TYPE_ECHOREPLY_V;
        // we changed only the icmp.type field from request(=8) to reply(=0).
        // we can therefore easily correct the checksum:
        if (buf[ICMP_CHECKSUM_P] > (0xff-0x08)){
                buf[ICMP_CHECKSUM_P+1]++;
        }
        buf[ICMP_CHECKSUM_P]+=0x08;
        //
        enc28j60PacketSend(len,buf);
}

// you can send a max of 220 bytes of data
void make_udp_reply_from_request(uint8_t *buf,char *data,uint8_t datalen,uint16_t port)
{
        uint8_t i=0;
        uint16_t ck;
        make_eth(buf);
        if (datalen>220){
                datalen=220;
        }
        // total length field in the IP header must be set:
        buf[IP_TOTLEN_H_P]=0;
        buf[IP_TOTLEN_L_P]=IP_HEADER_LEN+UDP_HEADER_LEN+datalen;
        make_ip(buf);
        buf[UDP_DST_PORT_H_P]=port>>8;
        buf[UDP_DST_PORT_L_P]=port & 0xff;
        // source port does not matter and is what the sender used.
        // calculte the udp length:
        buf[UDP_LEN_H_P]=0;
        buf[UDP_LEN_L_P]=UDP_HEADER_LEN+datalen;
        // zero the checksum
        buf[UDP_CHECKSUM_H_P]=0;
        buf[UDP_CHECKSUM_L_P]=0;
        // copy the data:
        while(i<datalen){
                buf[UDP_DATA_P+i]=data[i];
                i++;
        }
        ck=checksum(&buf[IP_SRC_P], 16 + datalen,1);
        buf[UDP_CHECKSUM_H_P]=ck>>8;
        buf[UDP_CHECKSUM_L_P]=ck& 0xff;
        enc28j60PacketSend(UDP_HEADER_LEN+IP_HEADER_LEN+ETH_HEADER_LEN+datalen,buf);
}

void make_tcp_synack_from_syn(uint8_t *buf)
{
        uint16_t ck;
        make_eth(buf);
        // total length field in the IP header must be set:
        // 20 bytes IP + 24 bytes (20tcp+4tcp options)
        buf[IP_TOTLEN_H_P]=0;
        buf[IP_TOTLEN_L_P]=IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+4;
        make_ip(buf);
        buf[TCP_FLAG_P]=TCP_FLAGS_SYNACK_V;
        make_tcphead(buf,1,1,0);
        // calculate the checksum, len=8 (start from ip.src) + TCP_HEADER_LEN_PLAIN + 4 (one option: mss)
        ck=checksum(&buf[IP_SRC_P], 8+TCP_HEADER_LEN_PLAIN+4,2);
        buf[TCP_CHECKSUM_H_P]=ck>>8;
        buf[TCP_CHECKSUM_L_P]=ck& 0xff;
        // add 4 for option mss:
        enc28j60PacketSend(IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+4+ETH_HEADER_LEN,buf);
}

// get a pointer to the start of tcp data in buf
// Returns 0 if there is no data
// You must call init_len_info once before calling this function
uint16_t Network::get_tcp_data_pointer(void)
{
        if (info_data_len){
                return((uint16_t)TCP_SRC_PORT_H_P+info_hdr_len);
        }else{
                return(0);
        }
}

// do some basic length calculations and store the result in static varibales
void Network::init_len_info(uint8_t *buf)
{
        info_data_len=(buf[IP_TOTLEN_H_P]<<8)|(buf[IP_TOTLEN_L_P]&0xff);
        info_data_len-=IP_HEADER_LEN;
        info_hdr_len=(buf[TCP_HEADER_LEN_P]>>4)*4; // generate len in bytes;
        info_data_len-=info_hdr_len;
        if (info_data_len<=0){
                info_data_len=0;
        }
}

// fill in tcp data at position pos. pos=0 means start of
// tcp data. Returns the position at which the string after
// this string could be filled.
uint16_t fill_tcp_data_p(uint8_t *buf,uint16_t pos, const prog_char *progmem_s)
{
        char c;
        // fill in tcp data at position pos
        //
        // with no options the data starts after the checksum + 2 more bytes (urgent ptr)
        while ((c = pgm_read_byte(progmem_s++))) {
                buf[TCP_CHECKSUM_L_P+3+pos]=c;
                pos++;
        }
        return(pos);
}

// fill in tcp data at position pos. pos=0 means start of
// tcp data. Returns the position at which the string after
// this string could be filled.
uint16_t Network::fill_tcp_data(uint8_t *buf,uint16_t pos, const char *s)
{
        // fill in tcp data at position pos
        //
        // with no options the data starts after the checksum + 2 more bytes (urgent ptr)
        while (*s) {
                buf[TCP_CHECKSUM_L_P+3+pos]=*s;
                pos++;
                s++;
        }
        return(pos);
}

uint16_t fill_tcp_data2(uint8_t *buf, uint16_t pos, const char *s, uint8_t length) {
    uint8_t i = 0;
    while (i < length) {
        buf[TCP_CHECKSUM_L_P + 3 + pos] = s[i];
        pos++;
        i++;
    }
    return(pos);
}

// Make just an ack packet with no tcp data inside
// This will modify the eth/ip/tcp header 
void make_tcp_ack_from_any(uint8_t *buf)
{
        uint16_t j;
        make_eth(buf);
        // fill the header:
        buf[TCP_FLAG_P]=TCP_FLAG_ACK_V;
        if (info_data_len==0){
                // if there is no data then we must still acknoledge one packet
                make_tcphead(buf,1,0,1); // no options
        }else{
                make_tcphead(buf,info_data_len,0,1); // no options
        }

        // total length field in the IP header must be set:
        // 20 bytes IP + 20 bytes tcp (when no options) 
        j=IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN;
        buf[IP_TOTLEN_H_P]=j>>8;
        buf[IP_TOTLEN_L_P]=j& 0xff;
        make_ip(buf);
        // calculate the checksum, len=8 (start from ip.src) + TCP_HEADER_LEN_PLAIN + data len
        j=checksum(&buf[IP_SRC_P], 8+TCP_HEADER_LEN_PLAIN,2);
        buf[TCP_CHECKSUM_H_P]=j>>8;
        buf[TCP_CHECKSUM_L_P]=j& 0xff;
        enc28j60PacketSend(IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+ETH_HEADER_LEN,buf);
}

// you must have called init_len_info at some time before calling this function
// dlen is the amount of tcp data (http data) we send in this packet
// You can use this function only immediately after make_tcp_ack_from_any
// This is because this function will NOT modify the eth/ip/tcp header except for
// length and checksum
void make_tcp_ack_with_data(uint8_t *buf,uint16_t dlen)
{
        uint16_t j;
        // fill the header:
        // This code requires that we send only one data packet
        // because we keep no state information. We must therefore set
        // the fin here:
        buf[TCP_FLAG_P]=TCP_FLAG_ACK_V|TCP_FLAG_PUSH_V|TCP_FLAG_FIN_V;

        // total length field in the IP header must be set:
        // 20 bytes IP + 20 bytes tcp (when no options) + len of data
        j=IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+dlen;
        buf[IP_TOTLEN_H_P]=j>>8;
        buf[IP_TOTLEN_L_P]=j& 0xff;
        fill_ip_hdr_checksum(buf);
        // zero the checksum
        buf[TCP_CHECKSUM_H_P]=0;
        buf[TCP_CHECKSUM_L_P]=0;
        // calculate the checksum, len=8 (start from ip.src) + TCP_HEADER_LEN_PLAIN + data len
        j=checksum(&buf[IP_SRC_P], 8+TCP_HEADER_LEN_PLAIN+dlen,2);
        buf[TCP_CHECKSUM_H_P]=j>>8;
        buf[TCP_CHECKSUM_L_P]=j& 0xff;
        enc28j60PacketSend(IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+dlen+ETH_HEADER_LEN,buf);
}


/* new functions for web client interface */
void Network::make_arp_request(uint8_t *buf, uint8_t *server_ip)
{
       uint8_t i=0;

       while(i<6)
       {
            buf[ETH_DST_MAC +i]=0xff;
			buf[ETH_SRC_MAC +i]=macaddr[i];
			i++;
		}
		
		buf[ ETH_TYPE_H_P ] = ETHTYPE_ARP_H_V;
		buf[ ETH_TYPE_L_P ] = ETHTYPE_ARP_L_V;

	// generate arp packet
	buf[ARP_OPCODE_H_P]=ARP_OPCODE_REQUEST_H_V;
	buf[ARP_OPCODE_L_P]=ARP_OPCODE_REQUEST_L_V;

	// fill in arp request packet
	// setup hardware type to ethernet 0x0001
       buf[ ARP_HARDWARE_TYPE_H_P ] = ARP_HARDWARE_TYPE_H_V;
       buf[ ARP_HARDWARE_TYPE_L_P ] = ARP_HARDWARE_TYPE_L_V;

       // setup protocol type to ip 0x0800
       buf[ ARP_PROTOCOL_H_P ] = ARP_PROTOCOL_H_V;
       buf[ ARP_PROTOCOL_L_P ] = ARP_PROTOCOL_L_V;

       // setup hardware length to 0x06
       buf[ ARP_HARDWARE_SIZE_P ] = ARP_HARDWARE_SIZE_V;

       // setup protocol length to 0x04
       buf[ ARP_PROTOCOL_SIZE_P ] = ARP_PROTOCOL_SIZE_V;

       // setup arp destination and source mac address
       for ( i=0; i<6; i++)
       {
            buf[ ARP_DST_MAC_P + i ] = 0x00;
            buf[ ARP_SRC_MAC_P + i ] = macaddr[i];
       }

       // setup arp destination and source ip address
       for ( i=0; i<4; i++)
       {
            buf[ ARP_DST_IP_P + i ] = server_ip[i];
            buf[ ARP_SRC_IP_P + i ] = ipaddr[i];
       }
         // eth+arp is 42 bytes:
		enc28j60PacketSend(42,buf);

}


uint8_t Network::arp_packet_is_myreply_arp ( uint8_t *buf )
{
       uint8_t i;

       // if packet type is not arp packet exit from function
       if( buf[ ETH_TYPE_H_P ] != ETHTYPE_ARP_H_V || buf[ ETH_TYPE_L_P ] != ETHTYPE_ARP_L_V)
               return 0;
       // check arp request opcode
       if ( buf[ ARP_OPCODE_H_P ] != ARP_OPCODE_REPLY_H_V || buf[ ARP_OPCODE_L_P ] != ARP_OPCODE_REPLY_L_V )
               return 0;
       // if destination ip address in arp packet not match with avr ip address
        for(i=0; i<4; i++){
            if(buf[ETH_ARP_DST_IP_P+i] != ipaddr[i]){
                return 0;
            }
        }
       return 1;
}


uint16_t tcp_get_dlength ( uint8_t *buf )
{
	int dlength, hlength;

	dlength = ( buf[ IP_TOTLEN_H_P ] <<8 ) | ( buf[ IP_TOTLEN_L_P ] );
	dlength -= IP_HEADER_LEN;
	hlength = (buf[ TCP_HEADER_LEN_P ]>>4) * 4; // generate len in bytes;
	dlength -= hlength;
	if ( dlength <= 0 )
		dlength=0;
	
	return ((uint16_t)dlength);
}

void saveSEQ(uint8_t *buf){
	lastseq[0] = buf[TCP_SEQ_H_P];
	lastseq[1] = buf[TCP_SEQ_H_P+1];
	lastseq[2] = buf[TCP_SEQ_H_P+2];
	lastseq[3] = buf[TCP_SEQ_H_P+3];
}

void saveACK(uint8_t *buf){
	lastack[0] = buf[TCP_SEQACK_H_P];
	lastack[1] = buf[TCP_SEQACK_H_P+1];
	lastack[2] = buf[TCP_SEQACK_H_P+2];
	lastack[3] = buf[TCP_SEQACK_H_P+3];
}

void loadACK(uint8_t *buf){
	//sendData = 0;
	int n = 4;
	while(n>0){
		buf[TCP_SEQACK_H_P + n-1] = ack[n-1];
		n--;
	}
}

void loadSEQ(uint8_t *buf){
	//sendData = 0;
	int n = 4;
	while(n>0){
		buf[TCP_SEQ_H_P + n-1] = seq[n-1];
		n--;
	}
}

void setSocketState(SOCKET_STATE s){
	state = s;
}

uint8_t getSocketState(){
	return state;
}


void addTonumber(uint8_t *num, uint8_t numToAdd){
	for (int i=0; i<numToAdd;i++)
	{
	  if (++num[3] == 0x00)
		if (++num[2] == 0x00)
		  if (++num[1] == 0x00)
			++num[0];
	}
}
void Network::tcp_client_send_packet(uint8_t *buf,uint16_t dest_port, uint16_t src_port, uint8_t flags, uint16_t dlength, uint8_t *dest_mac, uint8_t *dest_ip)
{
       uint8_t i=0;
       uint8_t tseq;
       uint16_t ck;
	   if (gwmacaddr != 0)
		make_eth_ip_new(buf, gwmacaddr);
	   else
		make_eth_ip_new(buf, dest_mac);

       buf[TCP_DST_PORT_H_P]= (uint8_t) ( (dest_port>>8) & 0xff);
       buf[TCP_DST_PORT_L_P]= (uint8_t) (dest_port & 0xff);

       buf[TCP_SRC_PORT_H_P]= (uint8_t) ( (src_port>>8) & 0xff);
       buf[TCP_SRC_PORT_L_P]= (uint8_t) (src_port & 0xff);

       // sequence numbers:
       // add the rel ack num to SEQACK
		
		
		if (flags == TCP_FLAG_SYN_V) {
		// initial tcp sequence number,require to setup for first transmit/receive = max_segment_size
			// put inital seq number
			buf[TCP_SEQ_H_P+0]= 0;
			buf[TCP_SEQ_H_P+1]= 0;
			// we step only the second byte, this allows us to send packts
			// with 255 bytes or 512 (if we step the initial seqnum by 2)
			buf[TCP_SEQ_H_P+2]= seqnum;
			buf[TCP_SEQ_H_P+3]= 0;
			// step the inititial seq num by something we will not use
			// during this tcp session:
			seqnum+=2;

			// setup maximum segment size
			buf[TCP_OPTIONS_P]=2;
			buf[TCP_OPTIONS_P+1]=4;
			buf[TCP_OPTIONS_P+2]=0x05;
			buf[TCP_OPTIONS_P+3]=0x80;
			// 24 bytes:
			buf[TCP_HEADER_LEN_P]=0x60;

			dlength +=4;
		//end initial tcp sequence
			
	   // clear sequence ack numer before send tcp SYN packet = clear_seqack
			buf[TCP_SEQACK_P] = 0;
			buf[TCP_SEQACK_P+1] = 0;
			buf[TCP_SEQACK_P+2] = 0;
			buf[TCP_SEQACK_P+3] = 0;
		// end clear sequence ack number
		
			saveSEQ(buf);
			saveACK(buf);
			state = SYN;
		} else if (flags & TCP_FLAG_ACK_V) {
			if (flags & TCP_FLAG_PUSH_V) {
				if (state == ACK_PSH){
					copy4byte(net.buf + TCP_SEQ_H_P, lastack);
					copy4byte(net.buf + TCP_SEQACK_H_P, lastseq);
				} else if (state == ACK) {
					copy4byte(net.buf + TCP_SEQ_H_P, lastseq);
					copy4byte(net.buf + TCP_SEQACK_H_P, lastack);
				}
				
				saveSEQ(buf);
				saveACK(buf);
				
				state = ACK_PSH;
			}else if (flags & TCP_FLAG_SYN_V) {
				copy4byte(net.buf + TCP_SEQ_H_P, lastack);

				copy4byte(net.buf + TCP_SEQACK_H_P, lastseq);

				saveSEQ(buf);
				saveACK(buf);
				
				state = ACK;

			}else {
				// in SYN state and recived ACK due to SYN|ACK recived
				if (state == SYN){
					saveSEQ(buf);
					saveACK(buf);
					copy4byte(net.buf + TCP_SEQ_H_P, lastack);

					addTonumber(lastseq,1);
					copy4byte(net.buf + TCP_SEQACK_H_P, lastseq);

					saveSEQ(buf);
					saveACK(buf);
					state = ACK;
				} else if ((state == ACK) || (state == ACK_PSH)){
					saveSEQ(buf);
					saveACK(buf);
					copy4byte(net.buf + TCP_SEQ_H_P, lastack);

					addTonumber(lastseq,tcp_get_dlength(buf));
					copy4byte(net.buf + TCP_SEQACK_H_P, lastseq);

					saveSEQ(buf);
					saveACK(buf);
					state = ACK;
				}
				
			}
			// no options - 20 bytes:
			buf[TCP_HEADER_LEN_P]=0x50;
		} else if (flags & TCP_FLAG_RST_V) {
			saveSEQ(buf);
			saveACK(buf);
			copy4byte(net.buf + TCP_SEQ_H_P, lastack);
			
			addTonumber(lastseq,1);
			copy4byte(net.buf + TCP_SEQACK_H_P, lastseq);
		
			saveSEQ(buf);
			saveACK(buf);
			state = ACK;
			buf[TCP_HEADER_LEN_P]=0x50;
		} 

				
  
	   // most stay
	   make_ip_tcp_new(buf,IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+dlength, dest_ip);

       // zero the checksum
       buf[TCP_CHECKSUM_H_P]=0;
       buf[TCP_CHECKSUM_L_P]=0;

       // set up flags
       buf[TCP_FLAG_P] = flags;
       // setup maximum windows size
       buf[ TCP_WINDOWSIZE_H_P ] = ((600 - IP_HEADER_LEN - ETH_HEADER_LEN)>>8) & 0xff;
       buf[ TCP_WINDOWSIZE_L_P ] = (600 - IP_HEADER_LEN - ETH_HEADER_LEN) & 0xff;

       // setup urgend pointer (not used -> 0)
       buf[ TCP_URGENT_PTR_H_P ] = 0;
       buf[ TCP_URGENT_PTR_L_P ] = 0;

      // check sum
       ck=checksum(&buf[IP_SRC_P], 8+TCP_HEADER_LEN_PLAIN+dlength,2);
       buf[TCP_CHECKSUM_H_P]=ck>>8;
       buf[TCP_CHECKSUM_L_P]=ck& 0xff;
       // add 4 for option mss:
       enc28j60PacketSend(IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+dlength+ETH_HEADER_LEN+4,buf);
}

void Network::save_ACKSEQ_from_send(uint8_t *buf){
	saveACK(buf);
	saveSEQ(buf);
}

// from here all is the new tcpip
static void setMACs (const byte *mac) {
  net.copyMac(net.buf + ETH_DST_MAC, mac);
  net.copyMac(net.buf + ETH_SRC_MAC, Network::mymac);
}

static void setMACandIPs (const byte *mac, const byte *dst) {
  setMACs(mac);
  net.copyIp(net.buf + IP_DST_P, dst);
  net.copyIp(net.buf + IP_SRC_P, Network::myip);
}


void Network::udpPrepare (word sport, byte *dip, word dport) {
	setMACandIPs(gwmacaddr, dip);
	// see http://tldp.org/HOWTO/Multicast-HOWTO-2.html
	// multicast or broadcast address, https://github.com/jcw/ethercard/issues/59
	if ((dip[0] & 0xF0) == 0xE0 || *((long*) dip) == 0xFFFFFFFF)
		net.copyMac(buf + ETH_DST_MAC, allOnes);
	net.buf[ETH_TYPE_H_P] = ETHTYPE_IP_H_V;
	net.buf[ETH_TYPE_L_P] = ETHTYPE_IP_L_V;
	memcpy_P(net.buf + IP_P,iphdr,9);
	buf[IP_TOTLEN_H_P] = 0;
	buf[IP_PROTO_P] = IP_PROTO_UDP_V;
	buf[UDP_DST_PORT_H_P] = (dport>>8);
	buf[UDP_DST_PORT_L_P] = dport; 
	buf[UDP_SRC_PORT_H_P] = (sport>>8);
	buf[UDP_SRC_PORT_L_P] = sport; 
	buf[UDP_LEN_H_P] = 0;
	buf[UDP_CHECKSUM_H_P] = 0;
	buf[UDP_CHECKSUM_L_P] = 0;
}

void Network::udpTransmit (word datalen) {
	buf[IP_TOTLEN_H_P] = (IP_HEADER_LEN+UDP_HEADER_LEN+datalen) >> 8;
	buf[IP_TOTLEN_L_P] = IP_HEADER_LEN+UDP_HEADER_LEN+datalen;
	fill_ip_hdr_checksum(buf);
	buf[UDP_LEN_H_P] = (UDP_HEADER_LEN+datalen) >>8;
	buf[UDP_LEN_L_P] = UDP_HEADER_LEN+datalen;
	//fill_checksum(UDP_CHECKSUM_H_P, IP_SRC_P, 16 + datalen,1);
	uint16_t ck=checksum(&buf[IP_SRC_P], 16 + datalen,1);
	buf[UDP_CHECKSUM_H_P]=ck>>8;
	buf[UDP_CHECKSUM_L_P]=ck& 0xff;
	enc28j60PacketSend(UDP_HEADER_LEN+IP_HEADER_LEN+ETH_HEADER_LEN+datalen,buf);
}

// make a arp request
void Network::client_arp_whohas(byte *ip_we_search) {
	setMACs(allOnes);
	buf[ETH_TYPE_H_P] = ETHTYPE_ARP_H_V;
	buf[ETH_TYPE_L_P] = ETHTYPE_ARP_L_V;
	memcpy_P(buf + ETH_ARP_P,arpreqhdr,8);
	memset(buf + ETH_ARP_DST_MAC_P, 0, 6);
	net.copyMac(buf + ETH_ARP_SRC_MAC_P, Network::mymac);
	net.copyIp(buf + ETH_ARP_DST_IP_P, ip_we_search);
	net.copyIp(buf + ETH_ARP_SRC_IP_P, Network::myip);
	waitgwmac |= WGW_ACCEPT_ARP_REPLY;
	enc28j60PacketSend(42,buf);
}

byte Network::client_store_gw_mac() {
	if (memcmp(buf + ETH_ARP_SRC_IP_P, net.gwip, 4) != 0)
		return 0;
	net.copyMac(gwmacaddr, buf + ETH_ARP_SRC_MAC_P);
	return 1;
}

void Network::ntpRequest (bool firstTime) {
	if (net.ntpServer != 0) {
		unsigned long time = millis();
		if(time - lastTime > INTERVAL || firstTime) {
			Serial.println("Send request for NTP");
			lastTime = time;
			requestSent = true;
			setMACandIPs(gwmacaddr, net.ntpServer);
			buf[ETH_TYPE_H_P] = ETHTYPE_IP_H_V;
			buf[ETH_TYPE_L_P] = ETHTYPE_IP_L_V;
			memcpy_P(buf + IP_P,iphdr,9);
			buf[IP_TOTLEN_L_P] = 0x4c;
			buf[IP_PROTO_P] = IP_PROTO_UDP_V;
			fill_ip_hdr_checksum(buf);
			buf[UDP_DST_PORT_H_P] = 0;
			buf[UDP_DST_PORT_L_P] = 0x7b; // ntp = 123
			buf[UDP_SRC_PORT_H_P] = 10;
			buf[UDP_SRC_PORT_L_P] = net.ntpPort; // lower 8 bit of src port
			buf[UDP_LEN_H_P] = 0;
			buf[UDP_LEN_L_P] = 56; // fixed len
			buf[UDP_CHECKSUM_H_P] = 0;
			buf[UDP_CHECKSUM_L_P] = 0;
			memset(buf + UDP_DATA_P, 0, 48);
			memcpy_P(buf + UDP_DATA_P,ntpreqhdr,10);
			//fill_checksum(UDP_CHECKSUM_H_P, IP_SRC_P, 16 + 48,1);
			uint16_t ck=checksum(&buf[IP_SRC_P], 16 + 48,1);
			buf[UDP_CHECKSUM_H_P]=ck>>8;
			buf[UDP_CHECKSUM_L_P]=ck& 0xff;
			enc28j60PacketSend(90,buf);
		}
	}
}

bool Network::ntpProcessAnswer (uint32_t *time) {
	if (ntpServer != 0 && requestSent) { 
		if ((net.ntpPort && buf[UDP_DST_PORT_L_P]!=net.ntpPort) || buf[UDP_LEN_H_P]!=0 ||
			buf[UDP_LEN_L_P]!=56 || buf[UDP_SRC_PORT_L_P]!=0x7b)
				return false;
		  ((byte*) time)[3] = buf[0x52];
		  ((byte*) time)[2] = buf[0x53];
		  ((byte*) time)[1] = buf[0x54];
		  ((byte*) time)[0] = buf[0x55];
		requestSent = false;
		return true;
	}
	return false;
}

void Network::sendUdp (uint8_t *data,byte datalen,word sport, byte *dip, word dport) {
  udpPrepare(sport, dip, dport);
  if (datalen>220)
    datalen = 220;
  memcpy(buf + UDP_DATA_P, data, datalen);
  udpTransmit(datalen);
}

void Network::showIPsettings(void){
	Serial.print("My ip: ");Serial.print(net.myip[0]);Serial.print(".");
	Serial.print(net.myip[1]);Serial.print(".");
	Serial.print(net.myip[2]);Serial.print(".");
	Serial.println(net.myip[3]);
	Serial.print("My mask: ");Serial.print(net.mymask[0]);Serial.print(".");
	Serial.print(net.mymask[1]);Serial.print(".");
	Serial.print(net.mymask[2]);Serial.print(".");
	Serial.println(net.mymask[3]);
	Serial.print("My gateway: ");Serial.print(net.gwip[0]);Serial.print(".");
	Serial.print(net.gwip[1]);Serial.print(".");
	Serial.print(net.gwip[2]);Serial.print(".");
	Serial.println(net.gwip[3]);
	
	Serial.print("My gateway MAC: ");Serial.print(gwmacaddr[0]);Serial.print(":");
	Serial.print(gwmacaddr[1]);Serial.print(":");
	Serial.print(gwmacaddr[2]);Serial.print(":");
	Serial.print(gwmacaddr[3]);Serial.print(":");
	Serial.print(gwmacaddr[4]);Serial.print(":");
	Serial.println(gwmacaddr[5]);
	
	Serial.print("My NTP server ");Serial.print(net.ntpServer[0]);Serial.print(".");
	Serial.print(net.ntpServer[1]);Serial.print(".");
	Serial.print(net.ntpServer[2]);Serial.print(".");
	Serial.println(net.ntpServer[3]);

}



/* end of ip_arp_udp.c */
