#include "TopLedUtilityFunctions.h"
#include "CommandStore.h"


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

int isin(long x)
{
  //Lookup table for integer sine function
static const uint8_t isinTable8[] = {
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


bool receiveUDP(WiFiUDP udp) {
  char incomingPacket[800];
  //Parse the socket, allows for all the other operations required to read the contents
  int packetSize = udp.parsePacket();
  if (packetSize)
  {
    //Read 799 bytes from the socket and place it in incomingPacket
    int len = udp.read(incomingPacket, 799);
    if (len > 0)
    {
      //0 terminate the character array
      incomingPacket[len] = 0;
    }
    return true;
  }
  return false;
}

unsigned long serverMillis(long setOffset)
{
  static long millisOffset = 0;
  if ( setOffset != 0)
  {
    millisOffset = setOffset;
  }
  return millis() + millisOffset;
}

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

//Attempts to synchronize server time to ESP time
unsigned long timeSync(IPAddress server, WiFiUDP udp, unsigned int port )
{
  long millisOffset = 0;
  char incomingPacket[255];
  //Used for calculating the Round-Trip Time between the server and the ESP
  unsigned long RTTStart = micros();
  //Begin a packet to the server, write a request for the time and (s)end the packet
  udp.beginPacket(server, port);
  udp.write("T:?");
  udp.endPacket();

  //Calculate Round-Trip Time for the purposes of our checking loop
  unsigned long RTT = micros() - RTTStart;
  while (!receiveUDP(udp) && RTT <= 750000)
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

  /* Publish an event on the Particle cloud for the servertime at time of sync
    char buffa[50];
    sprintf(buffa, "%d", timestamp.l);
    Particle.publish("Servertime", buffa);*/

  return millisOffset;
}

String sendGET(IPAddress server, WiFiClient client, String url)
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

bool parseCommands(IPAddress connectTo, String url, WiFiClient client, WiFiUDP udp, ChangeInstruction* commandArray, int commandBufferSize, int commandArraySelect)
{
  union byteInt converter;
  String response = "";
  if (client.connect(connectTo, 80)) {   // If there's a successful connection to the IP address on port 80
    bool inResponse = false;
    // Make a HTTP request
    String getRequest = "GET ";
    getRequest.concat(url);
    getRequest.concat(" HTTP/1.1\r\nHost: ");
    getRequest.concat(connectTo.toString());
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
        
        udp.beginPacket(connectTo, 1500);
        sprintf(buf, "x: %x c: %c i: %i u: %u buf: %i", byteToInt(buff[bufferIndex]), buff[bufferIndex], byteToInt(buff[bufferIndex]), byteToInt(buff[bufferIndex]), bufferIndex);
        udp.write(buf);
        udp.write((char *)(buff));
        udp.endPacket();
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
            udp.beginPacket(connectTo, 1500);
            udp.write(buf);
            udp.endPacket();
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
            udp.beginPacket(connectTo, 1500);
            udp.write(buf);
            udp.endPacket();
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
