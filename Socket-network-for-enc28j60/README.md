##Arduino TCP/IP SOCKET
I had to create a project with TCP/IP socket connection. All other projects I found was a connection to Web sites, but the link was dropped after the connection.
I added the ability to keep the connection open, but I didn't tried to connect to WEB pages (I don't need it).

##About the ENC28J60
ENC28J60 Ethernet Module utilizes the new Microchip ENC28J60 Stand-Alone Ethernet Controller IC featuring a host of features to handle most of the network protocol requirements. The board connects directly to most microcontrollers with a standard SPI interface with a transfer speed of up to 20MHz.

##What we have?
**We can do the following:**

1. Open socket connection to socket server from arduino.
2. Send UDP packets
3. Listen to UDP.
4. Get NTP.
5. Connect with DHCP.
6. Connect to server with DNS name.

##How to use
###Set the ethernet you need to set MAC address:
`static uint8_t mymac[6] = {0x54,0x55,0x58,0x10,0x00,0x24};`

###Set the socket:
`static uint16_t my_port = 11200;     // client port`

`static uint16_t server_port = 1000;   // server port`

`static uint8_t dest_ip[4]={192,168,1,105};  //server IP`

###Start socket - in setup
`net.socketSetup(tcpSerialPrint,my_port,dest_ip,server_port);`
> the _tcpSerialPrint_ is callback function on receiving data.

`while (!net.connect()) {}`

`Serial.println("socket connected");`

### keep the socket alive - in the loop
`net.process();`

`net.available()`


##Next steps
1. Add the ability to connect web sits.
2. keep alive connection.
3. Re-transmission packets.
4. Connect to network with static address.

##Authors and Contributors - Thank you
To create this, I had help from the following:
* [ethercard](https://github.com/jcw/ethercard) - Jean-Claude Wippler
* [Arduino 1.0 with enc28j60 Ethernet Shield V1.1](http://trollmaker.com/article11/arduino-1-0-with-enc28j60-ethernet-shield-v1-1) - Virtualmix
*   The help to understand [Arduino Goodness : Ethernet Shield](http://www.codetorment.com/2009/10/24/arduino-goodness-ethernet-shield/#postcomment) - Tom Decreton
