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
#pragma GCC optimize ("-O2")
#ifndef TGpcm_h
#define TGpcm_h

//Switches
#define useSD
//#define useSPIFFS
#define Diag
//#define useTimer1
#define useTimer0

#ifdef useSPIFFS
#include <FS.h>
#else
#define useSD
#endif

 #ifdef useSD
	#include <SPI.h>
	#include <SD.h>
#endif


class TGpcm
{
	public:
	TGpcm(byte speakerPin_);
	boolean isPlaying();
	boolean play(String filename);
	unsigned long getTV();
	void stop();
	void increaseVolume();
	void decreaseVolume();
	
	private:
	unsigned long readBytes(int pos,byte numberOfBytes);
	boolean waveInfo(String filename);
	void disableTimer();
	boolean seek( unsigned long pos );
	
	unsigned long fPosition();
	unsigned long currentPos;										// current byte position in file
	unsigned long fileLength = 0;
};
ICACHE_RAM_ATTR void T1IntHandler();
void fastDigitalWrite(int pin,bool State);
void checkBuffers(byte);
void stopPlaying();
void checkBuffer();
unsigned long readBufferBytes(char * buff,int pos,byte numberOfBytes);
#endif
