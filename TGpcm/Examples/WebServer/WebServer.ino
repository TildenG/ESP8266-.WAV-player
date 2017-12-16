/*
Example file for new Wave player for ESP8266.
upload the webpage to the ESP8266 using https://github.com/esp8266/arduino-esp8266fs-plugin
add you own wave files to the SD card.

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

#include "src/WebFiles.h"
#include "src/TGpcm.h"
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

void handleRoot(){
  Serial.println("Enter Index page");
  if (server.hasArg("B1")){
	myPlayer.play("/WAV/FF816K.WAV");
  }else if (server.hasArg("B2")){
	myPlayer.play("/WAV/IMPMAR.WAV");
  }else if (server.hasArg("B3")){
	myPlayer.play("/WAV/MerryG8.WAV");
  }else if (server.hasArg("B4")){
	myPlayer.play("/WAV/TNGM.WAV");
  }else if (server.hasArg("stop")){
	myPlayer.stop();
  }
	sendFile(server.uri(),&server);
}

void handleNotFound(){
	sendFile(server.uri(),&server);
}

void setup(){
	//system_update_cpu_freq(80);
	Serial.begin(2000000);
	setupWiFi();
	DNSTicker.attach_ms(20,DNS);
	Serial.println(F("\r\nType Music.com into your web browser"));
}

void loop(){
	if (myPlayer.isPlaying()) Serial.println(myPlayer.getTV() + String("%"));
	delay(1000);
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
	dnsServer.start(DNS_PORT, "*", apIP);
	server.on("/", handleRoot);
	server.onNotFound(handleNotFound);
	server.begin();
}
