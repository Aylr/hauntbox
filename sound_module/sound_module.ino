// Hauntbox Sound Module

// This file is based on Adafruit's Waveshield demo files
// from http://www.ladyada.net/make/waveshield/download.html

// It is CRITICAL to note that for the official hauntbox sound module
// you use a MODIFIED version of Adafruit's WaveHC library.
// The only change is a single pin definition.

#include <FatReader.h>
#include <SdReader.h>
#include <avr/pgmspace.h>
#include "WaveUtil.h"
#include "WaveHC.h"


// ---------------------------------- Mode Variables ----------------------------------
bool ambient_mode = false;          // true if ambient sound should play
int current_trigger_sound = 0;      // keeps track of current trigger sound
bool random_mode = false;           // true if it should play random trigger sounds
bool SD_failed = true;              // true if SD card has failed, false if successful

// ---------------------------------- Misc Variables ----------------------------------
char* ambient_wav_filename = "ambient.wav";     // ambient wave file name
char* trigger_wav_filename = "alarm.wav";
int OC_trigger_pin = 5;                         // OC trigger pin
int logic_trigger_pin = 4;                      // Logic trigger pin
int trigger_threshold = 200;                    
int trigger_LED_pin = 7;                        // indicator LED when triggered
int reset_LED_pin = 8;                          // indicator board needs to be reset

// ---------------------------------- Objects ----------------------------------
SdReader card;    // This object holds the information for the card
FatVolume vol;    // This holds the information for the partition on the card
FatReader root;   // This holds the information for the filesystem on the card
FatReader f;      // This holds the information for the file we're play
WaveHC wave;      // This is the only wave (audio) object, since we will only play one at a time


// ---------------------------------- Utility Functions ----------------------------------

int freeRam(void){          // return the number of bytes currently free in RAM, great for debugging!
  extern int  __bss_end; 
  extern int  *__brkval; 
  int free_memory; 
  if((int)__brkval == 0) {
    free_memory = ((int)&free_memory) - ((int)&__bss_end); 
  }
  else {
    free_memory = ((int)&free_memory) - ((int)__brkval); 
  }
  return free_memory; 
}

void sdErrorCheck(void){
  if (!card.errorCode()) return;
  Serial.print(F("\n\rSD I/O error: "));
  Serial.print(card.errorCode(), HEX);
  Serial.print(F(", "));
  Serial.println(card.errorData(), HEX);
  SD_failed = true;
}

void IO_pin_setup(){
    // Set the output pins for the DAC control.
    // These pins are defined in the library
    pinMode(2, OUTPUT);
    pinMode(3, OUTPUT);
    pinMode(4, OUTPUT);
    pinMode(5, OUTPUT);

    //Set the LED output pins
    pinMode(trigger_LED_pin, OUTPUT);
    pinMode(reset_LED_pin, OUTPUT);
    // pinMode(OC_trigger_pin, INPUT);
    // pinMode(logic_trigger_pin, INPUT);
}

bool setup_SD_card(){
    //  if (!card.init(true)) { //play with 4 MHz spi if 8MHz isn't working for you
    if (!card.init()) {         //play with 8 MHz spi (default faster!)  
        Serial.println(F("SD init failed!"));  	// Something went wrong, lets print out why
        sdErrorCheck();
        SD_failed = true;							// set global mode variable
    }

    card.partialBlockRead(true);					// enable optimize read - some cards may timeout. Disable if you're having problems

    // Now we will look for a FAT partition!
    uint8_t part;
    for (part = 0; part < 5; part++) {     	// we have up to 5 slots to look in
        if (vol.init(card, part)) 
        break;                             	// we found one, lets bail
    }
    if (part == 5) {                       	// if we ended up not finding one  :(
        Serial.println(F("No FAT partition!"));
        sdErrorCheck(); 					// Something went wrong, lets print out why
        SD_failed = true;					// set global mode variable
    }

    // Lets tell the user about what we found
    Serial.print(F("Using partition "));
    Serial.print(part, DEC);
    Serial.print(F(", type is FAT"));
    Serial.println(vol.fatType(),DEC);     			// FAT16 or FAT32?

    // Try to open the root directory
    if (!root.openRoot(vol)) {
        Serial.println(F("Can't open root dir!")); 	// Something went wrong
        SD_failed = true;							// set global mode variable
        return false;
    }

    SD_failed = false;                        // Important to set the SD_faled to success!

    Serial.println(F("SD Ready"));
}

void SD_failure_alert(){
	// flash leds or something
    Serial.println(F("SD failed."));
}

bool does_file_exist(char *name) {   // Should only really run at startup or if SD card is reset
  if (wave.isplaying) {     // if wave object is already playing something, stop it!
    wave.stop();        // stop it
  }
  if (!f.open(root, name)) {  // look in the root directory and open the file
    Serial.print(F("Couldn't open "));
    Serial.println(name);
    return false;
  }
  if (!wave.create(f)) {    // OK read the file and turn it into a wave object
    Serial.println(F("Not a valid WAV"));
    return false;
  }
  return true;            // true if you made it this far without failing
}

// ---------------------------------- Sound Functions ----------------------------------

// Plays a full file from beginning to end with no pause.
void playcomplete(char *name) {
  // call our helper to find and play this name
  playfile(name);
  while (wave.isplaying) {
  // do nothing while its playing
  }
  // now its done playing
}

void playfile(char *name) {		// Plays w/ possiblity to be stopped
  if (wave.isplaying) { 		// if wave object is already playing something, stop it!
    wave.stop(); 				// stop it
  }
  if (!f.open(root, name)) {	// look in the root directory and open the file
    Serial.print(F("Couldn't open "));
    Serial.println(name);
    return;
  }
  if (!wave.create(f)) {		// OK read the file and turn it into a wave object
    Serial.println(F("Not a valid WAV")); return;
  }
  wave.play();					// ok time to play! start playback
}

// Plays the ambient background sound while waiting for a trigger
void playbackground(char *name) {
  playfile(name);					// call our helper to find and play this name
  while (wave.isplaying) {			// While the ambient background track is playing watch for triggers
    if (analogRead(OC_trigger_pin) > trigger_threshold) {
      Serial.print(F("Trigger: "));
      Serial.println(analogRead(OC_trigger_pin));
      digitalWrite(trigger_LED_pin,HIGH);   	// turn on trigger indicator LED
      playcomplete(trigger_wav_filename);			// Play complete trigger wav
    }
  }
  // now its done playing
  digitalWrite(trigger_LED_pin,LOW);    		// turn off trigger indicator LED
}


// ---------------------------------- Main Functions ----------------------------------
void setup(){
    Serial.begin(9600);                                 // set up serial port
    Serial.print("Hauntbox Sound Module. Free RAM: ");  // Running out of RAM is bad
    Serial.println(freeRam());                          // if this is under 150 bytes it may spell trouble!
    IO_pin_setup();             						//setup all IO pins required
    setup_SD_card();									//setup SD card

    //SD card logic
    if (SD_failed) {                                    // if the SD failed
        SD_failure_alert();								// alert user
    }else{                                              // if the SD succeeded
        //sound file logic
        ambient_mode = does_file_exist(ambient_wav_filename);
        Serial.print(F("ambient = "));
        Serial.println(ambient_mode);

        // if (trig*.wav exists)
        //     trigger_sounds[] = count of trig1.wav ... trign.wav
        // if (file random.txt exists)
        //     random_mode = true;
    }
}

void loop(){
    if (!SD_failed){                                    // if the SD succeeded
        if (ambient_mode) {
            playbackground(ambient_wav_filename);                // play background ambient listening for triggers
        }
        // if (triggered) {
        //     digitalWrite(LED, ON);
        //     if (trigger_sounds exists){
        //         if (random_mode){                                                   // if in random_mode play random sound
        //             play trigger_sounds[random number <= trigger_sounds.length];
        //         }else{                                                              //if not in random_mode, play next sound in sequence
        //             play trigger_sounds[current_trigger_sound];
    
        //             if (current_trigger_sound > trigger_sounds.length){
        //                 current_trigger_sound = 0;                                  // back to the beginning
        //             }else{
        //                 current_trigger_sound ++;
        //             }
        //         }
        //     }
        // }
        // digitalWrite(LED,OFF)
    }else{                                              // if the SD failed
    	// setup_SD_card();								// try to setup SD card again
    	// if (SD_failed)									// if it failed after another attempt
	        // SD_failure_alert();							// alert user
    }
}
