/* 
  Remote debug library 
  Copyright (C) 2019 Ricardo Arroyo & Javier Zaldo

  This program allows to remotely monitoring any ESP device using a telnet connection.
  
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, version 3 of the License.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details at: <http://www.gnu.org/licenses/>. 
*/

/////////////////////////////////////////////////////
// rdebug.h
// Usage: Include these definitions in your sketch
// 
//#define DEBUG  //Allows standard COMx debug
#define RDEBUG  //Allows remote debug
// Select only one option 
// Both option commented for final release (no debug)
//
// Available functions (use as standard COMx):
// DebugStart()
// DebugLn(s)
// Debug(s)
//
// #include <ESP8266WiFi.h>
// #include "rdebug.h"
/////////////////////////////////////////////////////

#ifndef _RDEBUG_H 
#define _RDEBUG_H 

#define DEFAULT_PORT  8888
#define DEFAULT_SPEED 115200
#define WELCOME "Rdebug: Welcome!"

#ifdef DEBUG
# define DebugStart() {Serial.begin(DEFAULT_SPEED); while (!Serial){} }
# define DebugLn(s) Serial.println((s))
# define Debug(s) Serial.print((s))
#define _DEBUG
#else
#ifdef RDEBUG
# define DebugStart() remoteserial.begin(DEFAULT_SPEED, DEFAULT_PORT)
# define DebugLn(s) remoteserial.println((s))
# define Debug(s) remoteserial.print((s))
#define _DEBUG
#else
# define DebugStart() //
# define DebugLn(s) //
# define Debug(s) //
#endif
#endif

  
class Rserial {
  public:
  int port = DEFAULT_PORT;  //Port number
  WiFiServer *server;
  WiFiClient client;
  bool clientconnected;

  void begin(int spd, int p) {
    port = p;
    server = new WiFiServer(p);
    server->begin();
    Serial.begin(spd);
    while (!Serial);
    clientconnected = false;
  };
  
 void begin(int spd) {
    begin(spd, DEFAULT_PORT);
  };
  
  void print(const char *s) {
    Serial.print(s);
    if (clientAvailable())
      client.print(s);
  };
  void print(String s) {
    Serial.print(s);
    if (clientAvailable())
      client.print(s);
  };
  void print(int s) {
    Serial.print(s);
    if (clientAvailable())
      client.print(s);
  };  
  void print(IPAddress s) {
    Serial.print(s);
    if (clientAvailable())
      client.print(s);
  };  
  void println(const char *s) {
    Serial.println(s);
    if (clientAvailable())
      client.println(s);
  };
  void println(String s) {
    Serial.println(s);
    if (clientAvailable())
      client.println(s);
  };
  void println(int s) {
    Serial.println(s);
    if (clientAvailable())
      client.println(s);
  };
  void println(IPAddress s) {
    Serial.println(s);
    if (clientAvailable())
      client.println(s);
  };  

  bool clientAvailable() {
    if (!clientconnected) {
      client = server->available();
      if(client.connected()){
        Serial.println("Client Connected");
        client.println(WELCOME);
        clientconnected = true;
      }
      else {
        clientconnected = false;
      }
    }
    else {
      if(client.connected()){
        clientconnected = true;
      }
      else {
        client.stop();
        Serial.println("Client Disconnected");
        clientconnected = false;
      }
    }
    return clientconnected;
  };
  
};
#ifdef RDEBUG
Rserial     remoteserial;
#endif
#endif
