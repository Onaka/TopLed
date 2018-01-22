
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "Testing.h"
#include "CommandStore.h"
#include "TopLedUtilityFunctions.h"

void dumpCommands(IPAddress server, WiFiClient client, WiFiUDP udp, String id)
{
  if (client.connect(server, 1500)) {   // If there's a successful connection to the IP address on port 80
    bool inResponse = false;
    // Make a HTTP request
    String getRequest = "GET ";
    getRequest.concat("/index.php?id=");
    getRequest.concat(id);
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
    udp.beginPacket(server, 1500);
    sprintf(buf, "x: %x rx: %x c: %c cr: %c ru: %u u: %u \n", uint8_t(readBuf[0]), uint8_t(reversed), readBuf[0], reversed, uint8_t(reversed), uint8_t(readBuf[0]));
    udp.write(buf);
    udp.write(readBuf[0]);
    udp.write(" ");
    udp.write(reversed);
    udp.write('\n');
    udp.endPacket();
      delay(100);
    } 
  } else {
  client.stop(); 
  
  }
  
}


void testGarbage(IPAddress server, WiFiClient client)
{
  if (client.connect(server, 80)) {   // If there's a successful connection to the IP address on port 80
    unsigned long timeout = millis();
    
    for (int i = 0; i < 255; i++)
    {
    //Block while waiting for the response from the server
    //also figure out if we've timed out

    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
        Particle.publish("Status", "Timed out");
        client.stop();
        return;   
   }
    }
    String line;
    //Read the client until there's no more data
    while (client.available()) {
    
    }
    }
    }
}
