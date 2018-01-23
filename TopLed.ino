#include <Arduino.h>
#include <math.h>

#include <ESP8266WiFi.h>
//#include <Task.h>
#include <WiFiUdp.h>

//UDP Object for sending data
WiFiUDP udp;

//TCP client, for making GET requests
WiFiClient client;

#include "Enums.h"
#include "TopLedUtilityFunctions.h"
#include "CommandStore.h"
#include "vector3.h"


#include "Testing.h"
ChangeInstruction commandArray[commandArraySize];




IPAddress other(192, 168, 1, 95);
String id = "2";

//IP address of the web and timesync server
IPAddress server(192, 168, 1, 3);


//TaskManager taskManager;

//TaskGetCommands taskGetCommands(MsToTaskTime(100));

//#include "AsyncGET.h"


int ledArray[1][3];




//Time counter, used for detecting when to poll webserver for data, deprecated
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











//Sends a get request to a given IP address, with the URL provided
//url should begin with a /


int iterator = 0;


void setup() {
  //Initialize the serial port
  Serial.begin(115200, SERIAL_8N1);
  //Important for the TCP and UDP libraries
  WiFi.begin();
  //Start listening for UDP packets
  udp.begin(localUdpPort);


  //pinMode(led1, OUTPUT);
  pinMode(plusButton, INPUT_PULLUP);
  pinMode(minusButton, INPUT_PULLUP);
  ledArray[0][0] = 5;
  ledArray[0][1] = 6;
  ledArray[0][2] = 7;
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
  while (serverMillis() == 0)
  {
    serverMillis(timeSync(server, udp, 1500));
    delay(1000);
  }
  parseCommands(server, String("/index.php?id="), client, udp, commandArray);

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
  char packet[255];
  //Check for UDP messages, and clear the buffer
  receiveUDP(udp, packet, 255);

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
    serverMillis(timeSync(server, udp, 1500));
    parseCommands(server, String("/index.php?id="), client, udp, commandArray);
    //dumpCommands(server, client, udp);
    delay(500); //magic number
  }

  if (!digitalRead(minusButton))
  {
    serverMillis(timeSync(server, udp, 1500));
    parseCommands(server, String("/index.php?id=1"), client, udp, commandArray);
    //dumpCommands(server, client, udp);
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
  long long timeNow = serverMillis();
  for (int m = 0; m < 165; m++)
  {
    commandArray[m].doThing(timeNow, ledArray);
  }

  if (serverMillis() % 3000 == 0)
  {
    Serial.printf("server time:%lu\n", timeNow);
    char timeBuf[50];
    udp.beginPacket(server, 1500);
    sprintf(timeBuf, "%lu\n", timeNow);
    udp.write(timeBuf);
    udp.endPacket();

    /*
      Serial.printf("cx:%i cy:%i cz:%i sx:%i sy:%i sz:%i ex:%i ey:%i ez:%i",
                    int(commandArray[iterator].gradientChange.x), int(commandArray[iterator].gradientChange.y), int(commandArray[iterator].gradientChange.z),
                    int(commandArray[iterator].gradientStart.x), int(commandArray[iterator].gradientStart.y), int(commandArray[iterator].gradientStart.z),
                    int(commandArray[iterator].gradientEnd.x), int(commandArray[iterator].gradientEnd.y), int(commandArray[iterator].gradientEnd.z),
                    int(commandArray[iterator].gradientChange.z), int(commandArray[iterator].gradientStart.x), int(commandArray[iterator].gradientStart.y),
                    int(commandArray[iterator].gradientStart.z), int(commandArray[iterator].gradientEnd.x), int(commandArray[iterator].gradientEnd.y));
      Serial.printf(" start: %u duration: %u \n", commandArray[iterator].startTime, commandArray[iterator].duration);
      */
      Serial.printf("Currently active commands: %i\n", countActiveCommands(commandArray,timeNow));
      /*
      iterator++;
      if (iterator > 164)
      {
        iterator = 0;
      }
    */
  }
  /* Brightness setting directly through UDP
     doesn't exactly work, race condition makes LEDs very blinky
    if (oldBrightness != brightness)
    {
    udp.beginPacket(IPAddress(192, 168, 1, 119), 1500);
    udp.write((char)brightness);
    udp.endPacket();
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

