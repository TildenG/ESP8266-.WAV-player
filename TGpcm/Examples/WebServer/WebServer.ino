/*
Example file for new Wave player for ESP8266
Copyright 2017, Tilden Groves.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "src/TGpcm.h"
#include "src/WebFiles.h"
#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>

extern "C" {
	#include "user_interface.h"
}
TGpcm myPlayer(D1);
Ticker DNSTicker;
ESP8266WebServer server(80);
DNSServer dnsServer;

void handleSent(){
  server.send(200, "text/plain", "Request sent, get ready.");
  Serial.println(server.arg("NEWTEXT"));
}

void handleRoot(){
  Serial.println("Enter Index page");
  if (server.hasArg("NEWTEXT") && server.arg("NEWTEXT") !=""){
	Serial.println(server.arg("NEWTEXT")); // TODO: send text to display function here
	String header = F("HTTP/1.1 301 OK\r\nLocation: /sent\r\nCache-Control: no-cache\r\n\r\n");
    server.sendContent(header);
  }else
  {
	sendFile(server.uri(),&server);
  }
}

void handleNotFound(){
	//myPlayer.stop();
	sendFile(server.uri(),&server);
}
void setup(){
	system_update_cpu_freq(80);
	Serial.begin(2000000);
	//SPIFFS.begin();
	setupWiFi();
	String filename = "/WAV/FF816K.WAV";
	DNSTicker.attach_ms(20,DNS);
}

void loop(){
	if (myPlayer.isPlaying())Serial.println(myPlayer.getTV() + String("%"));
/* 	if (!myPlayer.isPlaying()){
	delay(10000);
	String filename = "/WAV/TNGM.wav";
	myPlayer.play(filename);
	} */
	//delay(5100);
 	//dnsServer.processNextRequest();      // update DNS requests
	server.handleClient();
}

void DNS(){
	dnsServer.processNextRequest();      // update DNS requests
}

void setupWiFi(){ // setup AP, start DNS server, start Web server
	const char *AP_NameChar = "Music Player";
	WiFi.mode(WIFI_AP);
	int channel = random(1, 13 + 1);
	const byte DNS_PORT = 53;
	IPAddress subnet(255, 255, 255, 0);
	IPAddress apIP(192, 168, 1, 1);
	WiFi.softAPConfig(apIP, apIP, subnet);
	const char *pw = "";
	WiFi.softAP(AP_NameChar, pw, channel, 0);
	dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
	dnsServer.start(DNS_PORT, "*", apIP);
	server.on("/", handleRoot);
	server.on("/sent", handleSent);
	server.onNotFound(handleNotFound);
	server.begin();
}
/*
#define FS_NO_GLOBALS //allow spiffs to coexist with SD card, define BEFORE including FS.h
#include <FS.h> //spiff file system

//accessing SPIFFS files, add 'fs::' in front of the file declaration, the rest is done as usual:
   fs::File file = SPIFFS.open(path, "r");  //add the namespace 
   size_t sent = server.streamFile(file, contentType); //stream file to webclient
   file.close();

//accessing SD card files is done the usual way:
 File dataFile = SD.open(filename, FILE_WRITE); //create the file
      if (dataFile) {
        String header = "Logfile created at "  + getTimeString();
        dataFile.println(header);
      }
      dataFile.close();


*/