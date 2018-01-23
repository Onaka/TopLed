
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "Testing.h"
#include "CommandStore.h"
#include "TopLedUtilityFunctions.h"



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

