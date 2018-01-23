#ifndef TOPLEDUTILITYFUNCTIONS_H
#define TOPLEDUTILITYFUNCTIONS_H
//What port to communicate on over UDP
#define localUdpPort 1500
#define numLeds 1
#define commandArraySize 166

#include "vector3.h"
#include "CommandStore.h"

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

//Separates string into pieces delimited by character
//This might be too slow to actually use, I just didn't want to reinvent the wheel
//so I googled a solution
// https://stackoverflow.com/questions/9072320/split-string-into-string-array
String getValue(String data, char separator, int index);

//Calculate what server time is
long long serverMillis(long setOffset = 0);

//integer sine function
int isin(long x);



//Check to see if there have been any UDP packets incoming
//returns true if it reads a packet, places said packet in packetBuffer
bool receiveUDP(WiFiUDP udp, char *packetBuffer, int bufferLength);

long timeSync(IPAddress server, WiFiUDP udp, unsigned int port );


//Convert byte to integer
uint8_t byteToInt(byte input);

//Reverses the order of bits in a byte
byte reverseBitsByte(byte x);


//Send a GET reuquest to the server and with the given URL relative to the root
String sendGET(IPAddress server, WiFiClient client, String url);

//Process command data into command containers
bool parseCommands(IPAddress connectTo, String url, WiFiClient client, WiFiUDP udp, ChangeInstruction* commandArray, int commandBufferSize = commandArraySize, int commandArraySelect = 1);

//Check to see if the current command should be executed
bool isCommandActive(ChangeInstruction command, unsigned long serverTime);

//Count how many commands are currently in an actionable state
int countActiveCommands(ChangeInstruction* commandArray,unsigned long serverTime);

//Prints a float over the serial port
void serialPrintDouble( double val, unsigned int precision);

#endif

