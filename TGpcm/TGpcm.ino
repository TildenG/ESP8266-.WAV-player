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
extern "C" {
	#include "user_interface.h"
}
TGpcm myPlayer(D1);

void setup(){
	//system_update_cpu_freq(160);
	Serial.begin(2000000);
	//String filename = "/fireflies.wav";
	//String filename = "/TNGM.wav";
	//String filename = "/alert04.wav";
	String filename = "/WAV/ALERT04.WAV";
	//String filename = "/WAV/FIREFL~1.WAV";
	//String filename = "/WAV/FIREFL~2.WAV";
	//String filename = "/WAV/FF816K.WAV";
	myPlayer.play(filename);
}

void loop(){
	if (myPlayer.isPlaying())Serial.println(myPlayer.getTV() + String("%"));
	if (!myPlayer.isPlaying()){
	delay(10000);
	String filename = "/WAV/FF816K.WAV";
	myPlayer.play(filename);
	}
	delay(1100);
}
