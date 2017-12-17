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
volatile boolean pinState = false;
volatile byte speakerPin = 255;
volatile byte speakerPin2 = 255;
#define maxBufferSize 512 // 256
volatile unsigned long preProcessedBuffer[2][(maxBufferSize * 2)];
Ticker bufferTicker;
volatile boolean whichBuffer = 0;
volatile boolean isBufferEmpty[2];
volatile unsigned long clockCyclesPerMs;
volatile unsigned int bufferPos;
volatile unsigned long range;
volatile unsigned int bufferLevel[2];
unsigned int SAMPLE_RATE;
boolean playing = false;
volatile unsigned long resolution;
volatile unsigned long timingOverhead = 0;
byte cpuFrequency = 0;
volatile unsigned int bytesPerSample;
volatile byte maxVolume = 10;
volatile byte volume = maxVolume;

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
	fastDigitalWrite(speakerPin,LOW);
	cpuFrequency = system_get_cpu_freq();
}
TGpcm::TGpcm(byte speakerPin_, byte speakerPin2_){
	speakerPin = speakerPin_;
	speakerPin2 = speakerPin2_;
	#ifdef useSPIFFS
		SPIFFS.begin();
	#else
	if (!SD.begin(D8)) {
		Serial.println("initialization failed!");
		return;
	}
	#endif
	pinMode(speakerPin,OUTPUT);
	pinMode(speakerPin2,OUTPUT);
	fastDigitalWrite(speakerPin,LOW);
	fastDigitalWrite(speakerPin2,LOW);
	cpuFrequency = system_get_cpu_freq();
}
boolean TGpcm::play(String filename){
	if (playing)stopPlaying();
	if (!waveInfo(filename)){
		return false;
	}
	playing = true;
	timingOverhead = 0;
	#ifdef useTimer1
	if (system_get_cpu_freq() == 80)timingOverhead = 230;
	if (system_get_cpu_freq() == 160)timingOverhead = 1030;
 	if (system_get_cpu_freq() == 80 && SAMPLE_RATE == 44100){// needs to run at 160Mhz
		system_update_cpu_freq(160);
		timingOverhead = 1030;
	}
	#else
	if (system_get_cpu_freq() == 80)timingOverhead = 121;
	if (system_get_cpu_freq() == 160)timingOverhead = 122;
 	if (system_get_cpu_freq() == 80 && SAMPLE_RATE == 44100){// needs to run at 160Mhz
		system_update_cpu_freq(160);
		timingOverhead = 122;
	}
	#endif
	clockCyclesPerMs = system_get_cpu_freq()*1000000; // update in-case of cpu frequency change
	// fill buffers
	for (int a=0;a<2;a++){
	isBufferEmpty[a]=true;
	}
	checkBuffer();
	whichBuffer = 0;
	bufferPos = 0;
	bufferTicker.attach_ms(1,checkBuffer);
	Serial.println("");
	pinState = true;
	#ifdef useTimer1
		timer1_disable();
		timer1_detachInterrupt();
		timer1_isr_init();
		timer1_attachInterrupt(T1IntHandler);
		timer1_enable(TIM_DIV1, TIM_EDGE, TIM_SINGLE);
		timer1_write(clockCyclesPerMs / SAMPLE_RATE); // ticks before interrupt fires, maximum ticks 8388607
	#else
		timer0_detachInterrupt();
		timer0_isr_init();
		timer0_attachInterrupt(T1IntHandler);
		timer0_write( (ESP.getCycleCount() + (clockCyclesPerMs / SAMPLE_RATE)) );
	#endif
	return playing;
}
boolean TGpcm::waveInfo(String filename){ // read header
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
	delay(100); // give serial time to start
	Serial.println("");
	Serial.print(F("File name: \t\t\t"));
	Serial.println(filename);
	#endif
	seek(8); // check to see if file is compatible
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
	bytesPerSample = readBytes(32,2);
	resolution = pow(2,readBytes(34,2)) -1;
	SAMPLE_RATE = readBytes(24,4);
	range = resolution * SAMPLE_RATE;
	currentPos = 43+1; // next read will read 44
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

#ifdef useSPIFFS
	boolean TGpcm::seek( unsigned long pos ){
		return sFile.seek(pos,SeekSet);
	}
	unsigned long TGpcm::fPosition(){
		return sFile.position();
	}
#else
	boolean TGpcm::seek(unsigned long pos ){
		return sFile.seek(pos);
	}
	unsigned long TGpcm::fPosition(){
		return sFile.position();
	}
#endif

boolean TGpcm::isPlaying(){
	return playing;
}

void TGpcm::disableTimer(){
	#ifdef useTimer1
		timer1_disable();
	#else
		timer0_detachInterrupt();
	#endif
}
void TGpcm::stop(){
	stopPlaying();
}
void stopPlaying(){
	#ifdef useTimer1
		timer1_disable();
		timer1_detachInterrupt();
	#else
		timer0_detachInterrupt();
	#endif
	bufferTicker.detach();
	fastDigitalWrite(speakerPin,LOW);
	playing = false;
	sFile.close();
	pinState = false;
	if (cpuFrequency != system_get_cpu_freq())system_update_cpu_freq(cpuFrequency);
}
ICACHE_RAM_ATTR void T1IntHandler(){
	#ifdef useTimer1
		timer1_write(preProcessedBuffer[whichBuffer][bufferPos]);
	#else
		timer0_write((ESP.getCycleCount() + preProcessedBuffer[whichBuffer][bufferPos]));
	#endif
	fastDigitalWrite(speakerPin,pinState);
	if(speakerPin2!=255)fastDigitalWrite(speakerPin2,(!pinState));
	bufferPos++;
	pinState = !pinState;
	
	if (bufferPos == bufferLevel[whichBuffer] * 2){
		isBufferEmpty[whichBuffer] = true;
		bufferPos = 0;
		if ((bufferLevel[whichBuffer] < maxBufferSize) || (isBufferEmpty[0] && isBufferEmpty[1])){ // end of file
			stopPlaying();
			return;
		}
		whichBuffer = !whichBuffer;
	}
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
void TGpcm::increaseVolume(){
	if (volume < (maxVolume + maxVolume * 1.5))volume++; // alow overdrive
}
void TGpcm::decreaseVolume(){
	if (volume > 1)volume--;
}
void checkBuffers(volatile byte a){
		if (isBufferEmpty[a]){
		char buffer[maxBufferSize*bytesPerSample];
		volatile unsigned long bytesLeft = (sFile.size() - sFile.position())/bytesPerSample; // samples left
 			#ifdef useSPIFFS
				#ifdef useTimer1
					timer1_disable();//TODO: try detaching the callback to see if it cleans up the audio on timer 1
				#else
				#endif
				fastDigitalWrite(speakerPin,LOW);
			#endif
			if(bytesLeft)isBufferEmpty[a] = false;
			if (bytesLeft > maxBufferSize){
				sFile.readBytes(buffer, maxBufferSize);
				bufferLevel[a] = maxBufferSize;
			}
			else
			{
				sFile.readBytes(buffer, bytesLeft);
				bufferLevel[a] = bytesLeft;
			}
 			#ifdef useSPIFFS
				#ifdef useTimer1
					timer1_enable(TIM_DIV1, TIM_EDGE, TIM_SINGLE);
				#else
				#endif
			#endif 
		volatile int newBufferPos = 0;
		for (volatile int b=0;b<bufferLevel[a]*bytesPerSample;b = b + bytesPerSample){
			volatile unsigned long bufferSample = readBufferBytes(buffer,b,bytesPerSample);
			if (bufferSample > 0 && bufferSample < resolution){
				preProcessedBuffer[a][newBufferPos] = (clockCyclesPerMs / ( range / bufferSample)) - timingOverhead;
				//Serial.println(preProcessedBuffer[a][newBufferPos]);
				newBufferPos++;
				preProcessedBuffer[a][newBufferPos] = (clockCyclesPerMs / ( range / (resolution - bufferSample) )) - timingOverhead;
				//Serial.println(preProcessedBuffer[a][newBufferPos]);
				newBufferPos++;
			}else if (bufferSample == 0){
				preProcessedBuffer[a][newBufferPos] = (clockCyclesPerMs / range ) - timingOverhead;
				newBufferPos++;
				preProcessedBuffer[a][newBufferPos] = (clockCyclesPerMs / (range / resolution)) - timingOverhead;
				newBufferPos++;
			}else {
				preProcessedBuffer[a][newBufferPos] = (clockCyclesPerMs / (range / resolution )) - timingOverhead;
				newBufferPos++;
				preProcessedBuffer[a][newBufferPos] = (clockCyclesPerMs / range ) - timingOverhead;
				newBufferPos++;
			}
			#ifdef useTimer1
				if (preProcessedBuffer[a][newBufferPos - 2] < 31)preProcessedBuffer[a][newBufferPos - 2] = 31; // stop popping sound
				else if (preProcessedBuffer[a][newBufferPos - 2] > 8388606)preProcessedBuffer[a][newBufferPos - 2] = 31; // avoid exceeding max ticks 8388606
				if (preProcessedBuffer[a][newBufferPos - 1] < 31)preProcessedBuffer[a][newBufferPos - 1] = 31; // stop popping sound
				else if (preProcessedBuffer[a][newBufferPos - 1] > 8388606)preProcessedBuffer[a][newBufferPos - 1] = 31;
			#else
				if (preProcessedBuffer[a][newBufferPos - 2] < 101)preProcessedBuffer[a][newBufferPos - 2] = 101; // stop popping sound
				else if (preProcessedBuffer[a][newBufferPos - 2] > 8388606)preProcessedBuffer[a][newBufferPos - 2] = 101; // avoid exceeding max ticks 8388606
				if (preProcessedBuffer[a][newBufferPos - 1] < 101)preProcessedBuffer[a][newBufferPos - 1] = 101; // stop popping sound
				else if (preProcessedBuffer[a][newBufferPos - 1] > 8388606)preProcessedBuffer[a][newBufferPos - 1] = 101;
			#endif
		}
	}
}
void checkBuffer(){
	checkBuffers(0);
	checkBuffers(1);
}
unsigned long readBufferBytes(char * buff,int pos,byte numberOfBytes){
		volatile uint16_t dataBytes = buff[pos];
		/* Serial.print("Bytes ");
		Serial.print(byte(buff[pos]));
		Serial.print(" ");
		Serial.println(byte(buff[pos+1])); */
	    for (byte i =8; i<numberOfBytes*8; i+=8){
			pos++;
			dataBytes = buff[pos] << i | dataBytes;
		}
		//Serial.print("long ");
		//Serial.println(dataBytes);
		if (SAMPLE_RATE != 32000) return dataBytes * float(volume / 10.0); //TODO: fix volume not working on 32K samples
		return dataBytes;
}