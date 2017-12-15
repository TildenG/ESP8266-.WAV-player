# ESP8266-.WAV-player

The .wav player is a work in progress !!.

Supports 8 bit 16khz .wav files from spiffs or SD card

Plays .wav files directly to a pin so It can be connected to a speaker.

Supports timer 0 and timer 1, just edit in the .h (header) file.

If you use SPIFFS increase the maxBufferSize variable in TGpcm.cpp to improve sound quality.