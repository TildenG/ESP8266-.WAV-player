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
#pragma GCC optimize ("-O2")
#include "src/TGpcm.h"
//#include <ESP8266WiFi.h>

extern "C" {
	#include "user_interface.h"
}
TGpcm myPlayer(D1);

void setup(){
	//system_update_cpu_freq(80);
	//WiFi.mode(WIFI_OFF);
	Serial.begin(2000000);
	//String filename = "/fireflies.wav";
	//String filename = "/TNGM.wav";
	//String filename = "/alert04.wav";
	
	//String filename = "/WAV/ALERT04.WAV";  // 32000 HZ 8 bit
	//String filename = "/WAV/FIREFL~1.WAV"; // 44110 HZ 8 Bit
	//String filename = "/WAV/FIREFL~2.WAV"; // 44110 HZ 16 Bit Broken
	//String filename = "/WAV/FF816K.WAV";	// 16000 HZ 8 bit
	String filename = "/WAV/IMPMAR.WAV"; // 32000 HZ 8 bit
	//String filename = "/WAV/TNGM.wav"; // 16000 HZ 8 bit
	//String filename = "/WAV/MerryG8.WAV"; // 44100HZ 8 bit
	//String filename = "/WAV/KnowItCC.wav"; // 16000HZ 8 bit
	myPlayer.play(filename);
	//myPlayer.play("/WAV/KnowItCC.wav");
}

void loop(){
	if (myPlayer.isPlaying())Serial.println(myPlayer.getTV() + String("%"));
	if (!myPlayer.isPlaying()){
	delay(10000);
	String filename = "/WAV/TNGM.wav";
	myPlayer.play(filename);
	}
	delay(5100);
}
