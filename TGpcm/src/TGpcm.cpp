/*
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

#include "TGpcm.h"
#include <Ticker.h>

extern "C" {
	#include "user_interface.h"
}
File sFile;
volatile unsigned long testVariable = 0;
volatile unsigned long nextFrequency;
volatile boolean pinState = false;
volatile byte speakerPin = 255; 
#define maxBufferSize 12550 // 255
char buffer[2][maxBufferSize+10];
Ticker bufferTicker;
volatile boolean whichBuffer = 0;
volatile boolean isBufferEmpty[2];
volatile unsigned long clockCyclesPerMs;
volatile unsigned int bufferPos;
volatile unsigned long range;
volatile unsigned int bufferLevel[2];
volatile boolean sampleHalf; // 0 = first half of sample 1 = second half of sample
unsigned int SAMPLE_RATE;
boolean playing = false;
volatile unsigned long resolution;


TGpcm::TGpcm(byte speakerPin_){
	speakerPin = speakerPin_;
	#ifdef useSPIFFS
		SPIFFS.begin();
	#else
	if (!SD.begin(D8)) {
		Serial.println("initialization failed!");
		return;
	}
	#endif
	pinMode(speakerPin,OUTPUT);
}
boolean TGpcm::play(String filename){
	if (!waveInfo(filename)){
		return false;
	}
	clockCyclesPerMs = clockCyclesPerMicrosecond() * 500000; // update incase of cpu frequency change
	if (system_get_cpu_freq() == 160)clockCyclesPerMs *= 1.3; //1.23
	playing = true;
	frequency = SAMPLE_RATE;
	// fill buffers
	for (int a=0;a<2;a++){ //TODO: make sure file size isn't bigger than buffers
		sFile.readBytes(buffer[a], maxBufferSize);
		isBufferEmpty[a]=false;
		bufferLevel[a] = maxBufferSize;
	}
	whichBuffer = 0;
	bufferPos = 0;
	bufferTicker.attach_ms(10,checkBuffer);
	sampleHalf = 0;
	Serial.println("");
	timer1_disable();
	timer1_isr_init();
	timer1_attachInterrupt(T1IntHandler);
	timer1_enable(TIM_DIV1, TIM_EDGE, TIM_SINGLE);
	timer1_write(clockCyclesPerMs / frequency); // ticks before interrupt fires, maximum ticks 8388607
}
boolean TGpcm::waveInfo(String filename){ // read headder
#ifdef useSPIFFS
	sFile = SPIFFS.open(filename,"r");
#else
	sFile = SD.open(filename,O_READ);
#endif
	if (!sFile){
	#ifdef Diag
		Serial.println(F("Failed to open file"));
	#endif
		return false;
	}
	#ifdef Diag
	Serial.print(F("File name: \t\t\t"));
	Serial.println(filename);
	#endif
	seek(8); // check to see if file is compatable
	char wavStr[] = {'W','A','V','E'};
	for (byte i =0; i<4; i++){
		if(sFile.read() != wavStr[i]){
			#ifdef Diag
		  	Serial.println("WAV ERROR");
			return false;
			#endif
		}
	}
	#ifdef Diag
	Serial.print(F("File length:\t\t\t"));
	Serial.println(readBytes(4,4) + String(" Bytes"));
	Serial.print(F("Length of format data:\t\t"));
	Serial.println(readBytes(16,4));
	Serial.print(F("File Type:\t\t\t"));
	if (readBytes(20,2) == 1)Serial.println(F("PCM"));
	else if (readBytes(20,2) == 6)Serial.println(F("mulaw"));
	else if (readBytes(20,2) == 7)Serial.println(F("alaw"));
	else if (readBytes(20,2) == 257)Serial.println(F("IBM Mu-Law"));
	else if (readBytes(20,2) == 258)Serial.println(F("ADPCM"));
	else Serial.println(F("Unknown")); // TODO add other file types
	Serial.print(F("Number of channels: \t\t"));
	Serial.println(readBytes(22,2));
 	Serial.print(F("Sample Rate:\t\t\t"));
	Serial.println(readBytes(24,4) + String(" Samples per second"));
	Serial.print(F("(Sample Rate * BitsPerSample * Channels) / 8: "));
	Serial.println(readBytes(28,4));
	Serial.print(F("(BitsPerSample * Channels) / 8.1 - 8 bit mono2 - 8 bit stereo/16 bit mono4 - 16 bit stereo: "));
	Serial.println(readBytes(32,2));
	Serial.print(F("Bits per sample:\t\t"));
	Serial.println(readBytes(34,2) + String(" Bits"));
	Serial.print(F("data only file size:\t\t"));
	Serial.println(readBytes(40,4) + String(" Bytes"));
	delay(5);
	#endif
	fileLength = readBytes(4,4);
	resolution = pow(2,readBytes(34,2)) -1;
	SAMPLE_RATE = readBytes(24,4);
	range = (resolution + 1) * SAMPLE_RATE;
	currentPos = 43; // next read will read 44
	seek(currentPos);
	return true;
}
unsigned long TGpcm::readBytes(int pos,byte numberOfBytes){
		seek(pos);
		unsigned long dataBytes = sFile.read();
	    for (byte i =8; i<numberOfBytes*8; i+=8){
			dataBytes = sFile.read() << i | dataBytes;
		}
		return dataBytes;
}

/**************************************************
This section used for translation of functions between
 SDFAT library and regular SD library
Prevents a whole lot more #if defined statements */

#ifdef useSPIFFS

	boolean TGpcm::seek( unsigned long pos ){
		return sFile.seek(pos,SeekSet);
	}

	unsigned long TGpcm::fPosition(){
		return sFile.position();
	}

/* 	boolean TGpcm::ifOpen(){
		if(sFile){ return 1;}
	} */

#else

	boolean TGpcm::seek(unsigned long pos ){
		return sFile.seek(pos);
	}

	unsigned long TGpcm::fPosition(){
		return sFile.position();
	}

/* 	boolean TGpcm::ifOpen(){
		return sFile.isOpen();
	} */

#endif

boolean TGpcm::isPlaying(){
	return playing;
}

void TGpcm::disableTimer(){
	timer1_disable();
}
void TGpcm::stop(){
	stopPlaying();
}
void stopPlaying(){
	timer1_disable();
	bufferTicker.detach();
	fastDigitalWrite(speakerPin,LOW);
	playing = false;
	sFile.close();
}
ICACHE_RAM_ATTR void T1IntHandler(){
/* unsigned long mod = buffer[whichBuffer][bufferPos] - 100;
	mod = constrain(mod,0, resolution -1);
	buffer[whichBuffer][bufferPos] = mod; */
	if (!sampleHalf){ // first half
		if (buffer[whichBuffer][bufferPos] >0){
			nextFrequency = range/buffer[whichBuffer][bufferPos];
			//nextFrequency = range - (buffer[whichBuffer][bufferPos] * SAMPLE_RATE);
		}
		sampleHalf = 1;
		pinState = true;
		if (!buffer[whichBuffer][bufferPos]){ // off for whole sample
			sampleHalf = 0;
			pinState = false;
			nextFrequency = SAMPLE_RATE;
			bufferPos++;
		}
		if (buffer[whichBuffer][bufferPos] == resolution){// on for whole sample
			sampleHalf = 0;
			pinState = true;
			nextFrequency = SAMPLE_RATE;
			bufferPos++;
		}
/* 		Serial.print(nextFrequency);
		Serial.print(" ");
		Serial.println(byte(buffer[whichBuffer][bufferPos])); */
	}else{
		pinState = true;
		nextFrequency = range / (resolution - buffer[whichBuffer][bufferPos]);
		//nextFrequency = range - (range - (buffer[whichBuffer][bufferPos] * SAMPLE_RATE));
		//Serial.println(nextFrequency);
		sampleHalf = 0;
		pinState = false;
		bufferPos++;
/* 		Serial.print(nextFrequency);
		Serial.print(" ");
		Serial.println(byte(buffer[whichBuffer][bufferPos])); */
	}
	if (bufferPos == bufferLevel[whichBuffer]){
		isBufferEmpty[whichBuffer] = true;
		bufferPos = 0;
		if (bufferLevel[whichBuffer] < maxBufferSize){ // end of file
			stopPlaying();
			return;
		}
		//fastDigitalWrite(speakerPin,LOW);
		//checkBuffers(whichBuffer);
		whichBuffer = !whichBuffer;
	}
	fastDigitalWrite(speakerPin,pinState);
	nextFrequency = clockCyclesPerMs / nextFrequency + 562.0;// *1.30;
	timer1_write(nextFrequency);
	timer1_enable(TIM_DIV1, TIM_EDGE, TIM_SINGLE);
}

void fastDigitalWrite(int pin,bool State){
	if (State){
		GPOS = (1 << pin); // HIGH
		return;
	}
	GPOC = (1 << pin); // LOW
}
unsigned long TGpcm::getTV(){
	return (double(sFile.position()) / double(fileLength)) * 100.0;//testVariable;
}

void checkBuffers(byte a){
	//for (int a=0;a<2;a++){
		unsigned long bytesLeft = sFile.size() - sFile.position();
		if (isBufferEmpty[a]){
 			#ifdef useSPIFFS
				timer1_disable();
				fastDigitalWrite(speakerPin,LOW);
			#endif
			if(bytesLeft)isBufferEmpty[a] = false;
			if (bytesLeft > maxBufferSize){
				sFile.readBytes(buffer[a], maxBufferSize);
				bufferLevel[a] = maxBufferSize;
			}
			else
			{
				sFile.readBytes(buffer[a], bytesLeft);
				bufferLevel[a] = bytesLeft;
			}
			//if(bytesLeft)isBufferEmpty[a] = false;
 			#ifdef useSPIFFS
				timer1_enable(TIM_DIV1, TIM_EDGE, TIM_SINGLE);
			#endif 
		}
	//}
}
void checkBuffer(){
	checkBuffers(0);
	checkBuffers(1);
}