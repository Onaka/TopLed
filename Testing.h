#ifndef TESTING_H
#define TESTING_H

//Dumps some info about commands over UDP
void dumpCommands (IPAddress server, WiFiClient client, WiFiUDP udp, String id = "1");


void testGarbage (IPAddress server);


#endif
