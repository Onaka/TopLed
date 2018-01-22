
#include <Arduino.h>
#include <math.h>

#include <ESP8266WiFi.h>
//#include <Task.h>
#include <WiFiUdp.h>

#include "vector3.cpp"
#define numLeds 1
//UDP Object for sending data
WiFiUDP Udp;

//TCP client, for making GET requests
WiFiClient client;

IPAddress other(192, 168, 1, 95);
String id = "2";

//IP address of the web and timesync server
IPAddress server(192, 168, 1, 3);


//TaskManager taskManager;

//TaskGetCommands taskGetCommands(MsToTaskTime(100));

//#include "AsyncGET.h"

// First, we're going to make some variables.
// This is our "shorthand" that we'll use throughout the program:

//Used for converting character arrays into unsigned longs
//note that the timesync server's endianness is opposite of the ESP
union CharLong {
  int i;
  unsigned long l;
  char str[4];
};

//Enum for checking what the current action is
enum programType {
  synctest = 'S',
  gradient = 'G',
  twinkle = 't',
  timeSet = 'T'
};

//Offset between server time and ESP time
long millisOffset = 0;
//Has the server time -> ESP time offset been set?
bool millisOffsetSet = false;

//Calculate what server time is
unsigned long serverMillis()
{
  return millis() + millisOffset;
}

#include "CommandStore.h"

//Buffers for the commands we've received from the server
//Improvement note: CRC or something for command array integrity?

ChangeInstruction commandArray[166];
int ledArray[numLeds][3];

void writeVector(int led, Vector3 vec)
{
  analogWrite(ledArray[led][0], int(vec.x));
  analogWrite(ledArray[led][1], int(vec.y));
  analogWrite(ledArray[led][2], int(vec.z));
}

void processNextCommand(int commandArray = 0)
{
}

// local UDP port to listen on
unsigned int localUdpPort = 1500;
// buffer for incoming packets
char incomingPacket[548];

//Define which pin LEDs are in
int led1 = 2;

bool LED = false;



programType currentProgram = synctest;

//Time counter, used for detecting when to poll webserver for data
unsigned long timeCounter = 0;
//How much time to wait between attempts to poll the webserver
unsigned long timeBetweenGets = 1000;

//Used to record what the server time was when the time was first synched
unsigned long startupTime = 0;

//Define which pins the buttons are in
int plusButton = 9;
int minusButton = 8;


//Initializing variable for storing brigthness of LED
int brightness = 128;

//Lookup table for integer sine function
uint8_t isinTable8[] = {
  0, 4, 9, 13, 18, 22, 27, 31, 35, 40, 44,
  49, 53, 57, 62, 66, 70, 75, 79, 83, 87,
  91, 96, 100, 104, 108, 112, 116, 120, 124, 128,

  131, 135, 139, 143, 146, 150, 153, 157, 160, 164,
  167, 171, 174, 177, 180, 183, 186, 190, 192, 195,
  198, 201, 204, 206, 209, 211, 214, 216, 219, 221,

  223, 225, 227, 229, 231, 233, 235, 236, 238, 240,
  241, 243, 244, 245, 246, 247, 248, 249, 250, 251,
  252, 253, 253, 254, 254, 254, 255, 255, 255, 255,
};
//integer sine function
int isin(long x)
{
  boolean pos = true;  // positive - keeps an eye on the sign.
  if (x < 0)
  {
    x = -x;
    pos = !pos;
  }
  if (x >= 360) x %= 360;
  if (x > 180)
  {
    x -= 180;
    pos = !pos;
  }
  if (x > 90) x = 180 - x;
  if (pos) return isinTable8[x] / 2 ;
  return -isinTable8[x] / 2 ;
}



//Check to see if there have been any UDP packets incoming
//returns true if it reads a packet
bool receiveUDP() {
  //Parse the socket, allows for all the other operations required to read the contents
  int packetSize = Udp.parsePacket();
  if (packetSize)
  {
    //Read 548 bytes from the socket and place it in incomingPacket
    int len = Udp.read(incomingPacket, 548);
    if (len > 0)
    {
      //0 terminate the character array
      incomingPacket[len] = 0;
    }
    return true;
  }
  return false;
}

//Attempts to synchronize server time to ESP time
unsigned long timeSync(IPAddress server, unsigned int port )
{
  //Used for calculating the Round-Trip Time between the server and the ESP
  unsigned long RTTStart = micros();
  //Begin a packet to the server, write a request for the time and (s)end the packet
  Udp.beginPacket(server, port);
  Udp.write("T:?");
  Udp.endPacket();

  //Calculate Round-Trip Time for the purposes of our checking loop
  unsigned long RTT = micros() - RTTStart;
  while (!receiveUDP() && RTT <= 750000)
  {
    RTT = micros() - RTTStart;
    //If the incoming packet starts with "T:", break out of the loop
    if (incomingPacket[0] == 'T' && incomingPacket[1] == ':')
    {
      break;
    }
  }

  //Union for converting stream of bytes into an unsigned long
  union CharLong timestamp;

  //Read the incoming packet's bytes into the timestamp union
  //in reverse order due to opposite endianness
  for ( int i = 0; i < 4; i++)
  {
    timestamp.str[i] = incomingPacket[5 - i];
  }

  //Calculate the offset between ESP and server, as per Cristian's algorithm
  millisOffset = timestamp.l + (RTT / 2000) - millis();
  //Let the rest of the program know that serverMillis() is accurate
  millisOffsetSet = true;

  /* Publish an event on the Particle cloud for the servertime at time of sync
    char buffa[50];
    sprintf(buffa, "%d", timestamp.l);
    Particle.publish("Servertime", buffa);*/

  return millisOffset;
}

union byteInt {
  byte b;
  uint8_t i;
};

uint8_t byteToInt(byte input) {
  union byteInt converter;
  converter.b = input;
  return converter.i;
}
byte reverseBitsByte(byte x) {
  int intSize = 8;
  byte y=0;
  for(int position=intSize-1; position>0; position--){
    y+=((x&1)<<position);
    x >>= 1;
  }
  return y;
}
void dumpCommands()
{
  if (client.connect(server, 1500)) {   // If there's a successful connection to the IP address on port 80
    bool inResponse = false;
    // Make a HTTP request
    String getRequest = "GET ";
    getRequest.concat("/index.php?id=1");
    getRequest.concat(" HTTP/1.1\r\nHost: ");
    getRequest.concat(server.toString());
    getRequest.concat("\r\nAccept: application/octet-stream\r\n");
    getRequest.concat("\r\nConnection: close\r\n\r\n");
    client.print(getRequest);


    unsigned long timeout = millis();
    //Block while waiting for the response from the server
    //also figure out if we've timed out
    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
        Particle.publish("Status", "Timed out");
        client.stop();
      }
    }
    bool done = false;
    byte readBuf[1];
    byte reversed = 0;
        //debug
    char buf[255];
    
    while (client.available()){
   
    client.readBytes(readBuf, (size_t)1);
    reversed = reverseBitsByte(readBuf[0]);
    Udp.beginPacket(server, 1500);
    sprintf(buf, "x: %x rx: %x c: %c cr: %c ru: %u u: %u \n", uint8_t(readBuf[0]), uint8_t(reversed), readBuf[0], reversed, uint8_t(reversed), uint8_t(readBuf[0]));
    Udp.write(buf);
    Udp.write(readBuf[0]);
    Udp.write(" ");
    Udp.write(reversed);
    Udp.write('\n');
    Udp.endPacket();
      delay(100);
    } 
  } else {
  client.stop(); 
  
  }
  
}


bool parseCommands(IPAddress connectTo, String url, int commandBufferSize = 166, int commandArraySelect = 1)
{
  union byteInt converter;
  String response = "";
  if (client.connect(connectTo, 80)) {   // If there's a successful connection to the IP address on port 80
    bool inResponse = false;
    // Make a HTTP request
    String getRequest = "GET ";
    getRequest.concat(url);
    getRequest.concat(" HTTP/1.1\r\nHost: ");
    getRequest.concat(server.toString());
    getRequest.concat("Accept: application/octet-stream\r\n");
    getRequest.concat("\r\nConnection: close\r\n\r\n");
    client.print(getRequest);
    //debug
    char buf[255];

    unsigned long timeout = millis();
    //Block while waiting for the response from the server
    //also figure out if we've timed out
    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
        Particle.publish("Status", "Timed out");
        client.stop();
        return false;
      }
    }
    int bufferIndex = 0;
    int commandIndex = 0;
    byte buff[50];
    String line;
    //Read the client until there's no more data
    unsigned long startDataWait = millis();
    while (client.available()) {



      if (!inResponse)
      {
        line = client.readStringUntil('\r');
        response.concat(line);
              //The HTTP response header ends in two newlines in a row, so if we find a line with
      //nothing but a newline character, we've gotten to the start of the payload
      if (line == "\n")
      {
        inResponse = true;
      }
      }


      if (inResponse) {

        buff[bufferIndex] = client.read();
        
        Udp.beginPacket(server, 1500);
        sprintf(buf, "x: %x c: %c i: %i u: %u buf: %i", byteToInt(buff[bufferIndex]), buff[bufferIndex], byteToInt(buff[bufferIndex]), byteToInt(buff[bufferIndex]), bufferIndex);
        Udp.write(buf);
        Udp.write((char *)(buff));
        Udp.endPacket();
        delay(50);
        switch ((programType)byteToInt(buff[0])) {
          case synctest:
            for (int i = 0; i < 4; i++)
            {
              bufferIndex++;
              if (bufferIndex >= sizeof(buff))
              {
                bufferIndex = 0;
                Particle.publish("Error", "Buffer length exceeded in synctest command");
                break;
              }

              buff[bufferIndex] = client.read();
            }
            commandArray[commandIndex] = ChangeInstruction(buff);

            sprintf(buf, "cx:%i cy:%i cz:%u bx:%i by:%i bz:%i", int(commandArray[commandIndex].gradientStart.x), int(commandArray[commandIndex].gradientStart.y), int(commandArray[commandIndex].gradientStart.z), (int)buff[1], (int)buff[2], (int)buff[3]);
            Udp.beginPacket(server, 1500);
            Udp.write(buf);
            Udp.endPacket();
            delay(50);
            commandIndex++;
            bufferIndex = 0;
            break;

          case gradient:
            for (int i = 0; i < 15; i++)
            {
              bufferIndex++;
              if (bufferIndex >= sizeof(buff))
              {
                bufferIndex = 0;
                Particle.publish("Error", "Buffer length exceeded in gradient command");
                break;
              }

              buff[bufferIndex] = client.read();
            }
            for (; buff[bufferIndex] != (byte)255; buff[bufferIndex] = client.read())
            {
              bufferIndex++;
            }

            commandArray[commandIndex] = ChangeInstruction(buff);
            sprintf(buf, "cx:%i cy:%i cz:%u bx:%i by:%i bz:%i", int(commandArray[commandIndex].gradientStart.x), int(commandArray[commandIndex].gradientStart.y), int(commandArray[commandIndex].gradientStart.z), (int)buff[1], (int)buff[2], (int)buff[3]);
            Udp.beginPacket(server, 1500);
            Udp.write(buf);
            Udp.endPacket();
            delay(50);
            commandIndex++;
            bufferIndex = 0;

            break;

          case twinkle:
            break;
          default:
            for (int h = 0; h <= bufferIndex; h++)
            {
              buff[h] = '\0';
            }
            bufferIndex = 0;
            break;

        };
        if (commandIndex >= commandBufferSize - 1)
        {
          return true;
        }
      }
    }

    /*Publish the last line of the payload to Particle
      char buffy[624];
      line.toCharArray(buffy, 624);
       Particle.publish("lastline", buffy, 60, PRIVATE);*/
  } else {
    // If you didn't get a connection to the server:
    Particle.publish("Status", "connection failed");
    return false;
  }
  client.stop();
  return true;
}

void testGarbage()
{
  if (client.connect(server, 80)) {   // If there's a successful connection to the IP address on port 80
    unsigned long timeout = millis();
    
    for (int = 0; i < 255; i++)
    {
    //Block while waiting for the response from the server
    //also figure out if we've timed out

    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
        Particle.publish("Status", "Timed out");
        client.stop();
        return "timeout";
   
   }
    }
    String line;
    //Read the client until there's no more data
    while (client.available()) {
    
    }
    }
    }
}



//This might be too slow to actually use, I just didn't want to reinvent the wheel
//so I googled a solution -Matti
// https://stackoverflow.com/questions/9072320/split-string-into-string-array
String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? String(data.substring(strIndex[0], strIndex[1])) : String("");
}

//Sends a get request to a given IP address, with the URL provided
//url should begin with a /
String sendGET(IPAddress server, String url)
{
  String response = "";
  if (client.connect(server, 80)) {   // If there's a successful connection to the IP address on port 80
    bool inResponse = false;
    // Make a HTTP request
    String getRequest = "GET ";
    getRequest.concat(url);
    getRequest.concat(" HTTP/1.1\r\nHost: ");
    getRequest.concat(server.toString());
    getRequest.concat("\r\nConnection: close\r\n\r\n");
    client.print(getRequest);

    unsigned long timeout = millis();
    //Block while waiting for the response from the server
    //also figure out if we've timed out
    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
        Particle.publish("Status", "Timed out");
        client.stop();
        return "timeout";
   
   }
    }
    String line;
    //Read the client until there's no more data
    while (client.available()) {
      line = client.readStringUntil('\r');
      //The HTTP response header ends in two newlines in a row, so if we find a line with
      //nothing but a newline character, we've gotten to the start of the payload
      if (line == "\n")
      {
        inResponse = true;
      }

      if (inResponse) {
        response.concat(line);
      }
    }

    /*Publish the last line of the payload to Particle
      char buffy[624];
      line.toCharArray(buffy, 624);
      Particle.publish("lastline", buffy, 60, PRIVATE);
    */
  }
  else {
    // If you didn't get a connection to the server:
    Particle.publish("Status", "connection failed");
    client.stop();
    return "Connection failed";
  }
  client.stop();
  //Remove the two leading newlines that are prepended to the response payload
  response.remove(0, 2);
  return response;
}

int iterator = 0;


void setup() {
  //Important for the TCP and UDP libraries
  WiFi.begin();
  //Start listening for UDP packets
  Udp.begin(localUdpPort);


  //pinMode(led1, OUTPUT);
  pinMode(plusButton, INPUT_PULLUP);
  pinMode(minusButton, INPUT_PULLUP);
  ledArray[0][0] = 2;
  ledArray[0][1] = 3;
  ledArray[0][2] = 4;
  for (int i = 0; i < numLeds; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      if (ledArray[i][j] != -1)
      {
        pinMode(ledArray[i][j], OUTPUT);
      }
    }
  }

  //Synchronize the time with the server
  while (!millisOffsetSet)
  {
    timeSync(server, 1500);
    delay(1000);
  }
  //parseCommands(server, String("/index.php?id=") + id);

  /* Publish an event to Particle telling what the offset ended up being
    char bufferizer[20];
    sprintf(bufferizer, "%d", millisOffset);
    Particle.publish("timesync", bufferizer);
  */

  //Store roughly what the time was when the synch was done
  startupTime = serverMillis();
}

// Next we have the loop function, the other essential part of a microcontroller program.
// This routine gets repeated over and over, as quickly as possible and as many times as possible, after the setup function is called.
// Note: Code that blocks for too long (like more than 5 seconds), can make weird things happen (like dropping the network connection).  The built-in delay function shown below safely interleaves required background activity, so arbitrarily long delays can safely be done if you need them.

void loop() {
  //Check for UDP messages, and clear the buffer
  receiveUDP();

  //delay(50); //magic number
  //timeCounter += 50; //magic number
  /*TCP Synch code for web server
     Randomize time between gets a little so that not every device is connecting at
     the exact same instant or frequency
     if (timeCounter >= timeBetweenGets + random(0, 200))
    {
    //Send a GET request with our ID
    String response = sendGET(IPAddress(192, 168, 1, 3), "/index.php?id=" + id);
    //Check if someone else set the brightness last, and check to see if we've set the
    //brightness, if we haven't, we can safely grab the value from the server
    if (id != getValue(response, ',', 0) && oldBrightness == brightness)
    {
      brightness = getValue(response, ',', 1).toInt();
    }
    //If we've changed the brightness locally, send it to the server
    if (oldBrightness != brightness) {
      String response = sendGET(IPAddress(192, 168, 1, 3), "/index.php?id=" + id + "&bri=" + String(brightness));
    }
    //Reset time between checks
    timeCounter = 0;
    oldBrightness = brightness;
    }
  */
  /* Read the status of the buttons and change the brightness
    if (digitalRead(plusButton))
    {
    brightness += 10;
    delay(50); //magic number
    timeCounter += 50; //magic number
    }
    if (digitalRead(minusButton))
    {
    brightness -= 10;
    delay(50); //magic number
    timeCounter += 50; //magic number
    }
  */

  //Resynch server time, for purposes of testing
  if (!digitalRead(plusButton))
  {
    timeSync(server, 1500);
    //parseCommands(server, String("/index.php?id=") + id);
    dumpCommands();
    delay(500); //magic number
  }

  if (!digitalRead(minusButton))
  {
    timeSync(server, 1500);
    //parseCommands(server, String("/index.php?id=") + id);
    dumpCommands();
    delay(500); //magic number
  }


  //Clamp the brigthness to a reasonable range
  if (brightness > 255)
  {
    brightness = 255;
  } else if (brightness < 0)
  {
    brightness = 0;
  }


  /*  switch (currentProgram)
    {
      case gradient:

        break;
      case synctest:
        analogWrite(led1, 127 + isin(serverMillis()));
        break;
      default:
        bool is1000 = false;
        //Square wave function that takes the server time and outputs true or false (hopefully)
        if (pow(-1, floor(float(serverMillis() / 1000))) < 0)
        {
          is1000 = false;
        } else {
          is1000 = true;
        }
        digitalWrite(led1, is1000);
        break;
    }*/
  delay(1);

  for (int m = 0; m < 165; m++)
  {
    commandArray[m].doThing(serverMillis());
  }

  //analogWrite(led1, serverMillis() % 255);
  //writeVector((char)0, Vector3(float(random(0, 127)), float(random(0, 127)), float(random(0, 127))));
  if (serverMillis() % 3000 == 0)
  {
    char buf[255];
    sprintf(buf, "cx:%u cy:%u cz:%u sx:%u sy:%u sz:%u ex:%u ey:%u ez:%u", int(commandArray[iterator].gradientChange.x), int(commandArray[iterator].gradientChange.y), int(commandArray[iterator].gradientChange.z), commandArray[iterator].gradientStart.x, commandArray[iterator].gradientStart.y, commandArray[iterator].gradientStart.z), commandArray[iterator].gradientEnd.x, commandArray[iterator].gradientEnd.y, commandArray[iterator].gradientEnd.z;
    iterator++;
    if (iterator > 164)
    {
      iterator = 0;
    }
    //Send what we think is the server time
    //to the server for comparison and skew experimentation
    Udp.beginPacket(server, 1500);
    Udp.write(buf);
    Udp.endPacket();
  }
  /* Brightness setting directly through UDP
     doesn't exactly work, race condition makes LEDs very blinky
    if (oldBrightness != brightness)
    {
    Udp.beginPacket(IPAddress(192, 168, 1, 119), 1500);
    Udp.write((char)brightness);
    Udp.endPacket();
    oldBrightness = brightness;
    }
  */


  /* Report to cloud what the GET response is.

    delay(3000);
    String response2 = sendGET(IPAddress(192, 168, 1, 3), "/index.php?id=2");
    char buffy2[624];
    response2.toCharArray(buffy2, 624);
    Particle.publish("response", buffy2, 60, PRIVATE);
  */
}
