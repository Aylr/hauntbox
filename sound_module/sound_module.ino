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

#define MAJOR_VER 0
#define MINOR_VER 4
#define MAX_SOUNDS 30
#define FNAME_LGTH 12


// ---------------------------------- Mode Variables ----------------------------------
bool ambient_mode = false;          // true if ambient sound should play
int current_trigger_sound = 0;      // keeps track of current trigger sound
bool random_mode = false;           // true if it should play random trigger sounds
bool SD_failed = true;              // true if SD card has failed, false if successful

// ---------------------------------- Misc Variables ----------------------------------
char *ambient_wav_filename = "AMBIENT.WAV";     // ambient wave file name
// char *trigger_wav_filename = "ALARM.WAV";
char *random_mode_filename = "RANDOM.TXT";
char trigger_sounds[MAX_SOUNDS][FNAME_LGTH];
byte trigger_sound_count = 0;                   // keeps track of how many trigger sounds you have on the SD
int OC_trigger_pin = 5;                         // OC trigger pin
int logic_trigger_pin = 4;                      // Logic trigger pin
int OC_trigger_threshold = 300;                 // OC trigger threshold that it must go below
int log_trigger_threshold = 200;                // logic trigger thresh that it must go above
int trigger_LED_pin = 7;                        // indicator LED when triggered
int reset_LED_pin = 8;                          // indicator board needs to be reset

// ---------------------------------- Objects ----------------------------------
SdReader card;    // This object holds the information for the card
FatVolume vol;    // This holds the information for the partition on the card
FatReader root;   // This holds the information for the filesystem on the card
FatReader f;      // This holds the information for the file we're playing
dir_t dirBuf;     // buffer for directory reads
WaveHC wave;      // This is the only wave (audio) object, since we will only play one at a time


// ---------------------------------- Utility Functions ----------------------------------

// Print device info to the serial port
void print_info(void)
{
  byte i;

  Serial.print(F("\r\nHauntbox Sound Module\r\nVersion "));
  Serial.print(MAJOR_VER);
  Serial.print(F("."));
  Serial.println(MINOR_VER);

  Serial.print(F("\r\nAmbient mode = "));
  Serial.println(ambient_mode);
  Serial.print(F("Random mode = "));
  Serial.println(random_mode);

  // Also play the trigger sounds
  Serial.print(trigger_sound_count);
  Serial.println(F(" trigger sounds found:"));
  for (i = 0; i < trigger_sound_count; i++) {
    Serial.println(trigger_sounds[i]);
    playcomplete(trigger_sounds[i]);
  }
  Serial.print(F("Current trigger index = "));
  Serial.println(current_trigger_sound);
}



int freeRam(void) {         // return the number of bytes currently free in RAM, great for debugging!
  extern int  __bss_end;
  extern int  *__brkval;
  int free_memory;
  if ((int)__brkval == 0) {
    free_memory = ((int)&free_memory) - ((int)&__bss_end);
  }
  else {
    free_memory = ((int)&free_memory) - ((int)__brkval);
  }
  return free_memory;
}

void sdErrorCheck(void) {
  if (!card.errorCode()) return;
  Serial.print(F("\r\nSD I/O error: 0x"));
  Serial.print(card.errorCode(), HEX);
  Serial.print(F(", 0x"));
  Serial.println(card.errorData(), HEX);
  SD_failed = true;
}

void IO_pin_setup() {
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

bool setup_SD_card() {
  //  if (!card.init(true)) { //play with 4 MHz spi if 8MHz isn't working for you
  if (!card.init()) {         //play with 8 MHz spi (default faster!)
    Serial.println(F("SD init failed!"));   // Something went wrong, lets print out why
    sdErrorCheck();
    SD_failed = true;             // set global mode variable
    return false;
  }

  card.partialBlockRead(true);          // enable optimize read - some cards may timeout. Disable if you're having problems

  // Now we will look for a FAT partition!
  uint8_t part;
  for (part = 0; part < 5; part++) {      // we have up to 5 slots to look in
    if (vol.init(card, part))
      break;                              // we found one, lets bail
  }
  if (part == 5) {                        // if we ended up not finding one  :(
    Serial.println(F("No FAT partition!"));
    sdErrorCheck();           // Something went wrong, lets print out why
    SD_failed = true;         // set global mode variable
  }

  // Lets tell the user about what we found
  Serial.print(F("Using partition "));
  Serial.print(part, DEC);
  Serial.print(F(", type is FAT"));
  Serial.println(vol.fatType(),DEC);          // FAT16 or FAT32?

  // Try to open the root directory
  if (!root.openRoot(vol)) {
    Serial.println(F("Can't open root dir!"));  // Something went wrong
    SD_failed = true;             // set global mode variable
    return false;
  }

  SD_failed = false;                        // Important to set the SD_faled to success!

  Serial.println(F("SD Ready"));
}

void SD_failure_alert() {
  // flash leds or something
  digitalWrite(reset_LED_pin,HIGH);
  Serial.println(F("SD failed."));
  digitalWrite(reset_LED_pin,LOW);
}


// print dir_t name field. The output is 8.3 format, so like SOUND.WAV or FILENAME.DAT
// void printEntryName(dir_t &dir)
// {
//   for (uint8_t i = 0; i < 11; i++) {     // 8.3 format has 8+3 = 11 letters in it
//     if (dir.name[i] == ' ')
//         continue;         // dont print any spaces in the name
//     if (i == 8)
//         Serial.print('.');           // after the 8th letter, place a dot
//     Serial.print(dir.name[i]);      // print the n'th digit
//   }
//   if (DIR_IS_SUBDIR(dir))
//     Serial.print('/');       // directories get a / at the end
// }


// Like strcmp but compare sequences of digits numerically -- natural sort
int strcmpbynum(const char *s1, const char *s2) {
  for (;;) {
    if (*s2 == '\0')
      return *s1 != '\0';
    else if (*s1 == '\0')
      return 1;
    else if (!(isdigit(*s1) && isdigit(*s2))) {
      if (*s1 != *s2)
        return (int)*s1 - (int)*s2;
      else
        (++s1, ++s2);
    } else {
      char *lim1, *lim2;
      unsigned long n1 = strtoul(s1, &lim1, 10);
      unsigned long n2 = strtoul(s2, &lim2, 10);
      if (n1 > n2)
        return 1;
      else if (n1 < n2)
        return -1;
      s1 = lim1;
      s2 = lim2;
    }
  }
}


// String compare function for qsort
int compare(void const *a, void const *b)
{
  char const *aa = (char const *)a;
  char const *bb = (char const *)b;
  return strcmpbynum(aa, bb);
}


// List the files in the directory
void get_files(FatReader &d)
{
  unsigned char c, skip;
  char name[FNAME_LGTH];
  char *s = "";
  char *pch;

  trigger_sound_count = 0;
  while ((d.readDir(dirBuf)) > 0) {     // read the next file in the directory
    // skip subdirs . and .. as well as hidden _filenames
    if (dirBuf.name[0] == '.' || dirBuf.name[0] == '_')
      continue;

    // Get the name of the next file
    dirName(dirBuf, name);
    // Set random mode if random.txt is found
    if (strcmp(name, random_mode_filename) == 0)
      random_mode = true;
    // Only save the .wav files.  Add each one to the trigger sounds array.
    pch = strstr(name, ".WAV");
    if (pch != NULL) {
      // Don't include ambient.wav.  It's special.
      if (strcmp(name, ambient_wav_filename) != 0) {
        // Make sure the file is a real .wav before adding it to the list
        if (does_file_exist(name)) {
          strcpy(trigger_sounds[trigger_sound_count], name);
          trigger_sound_count++;
        }
      }
    }

    // Stop reading the directory when we've reached the max # of sounds
    if (trigger_sound_count == MAX_SOUNDS)
      break;
  }

  // Sort the array alphabetically
  qsort(trigger_sounds, trigger_sound_count, FNAME_LGTH, compare);
  // Serial.println(F("Sorted:"));
  // for (c = 0; c < trigger_sound_count; c++) {
  //   Serial.println(trigger_sounds[c]);
  // }

  sdErrorCheck();                  // are we doign OK?
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


bool trigger_check() {
  if (analogRead(OC_trigger_pin) <= OC_trigger_threshold || analogRead(logic_trigger_pin) >= log_trigger_threshold) {
    Serial.print(F("OC: "));
    Serial.print(analogRead(OC_trigger_pin));
    Serial.print(F(" log: "));
    Serial.println(analogRead(logic_trigger_pin));
    return true;
  }
  return false;
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

void playfile(char *name) {   // Plays w/ possiblity to be stopped
  if (wave.isplaying) {     // if wave object is already playing something, stop it!
    wave.stop();        // stop it
  }
  if (!f.open(root, name)) {  // look in the root directory and open the file
    Serial.print(F("Couldn't open "));
    Serial.println(name);
    sdErrorCheck();
    return;
  }
  if (!wave.create(f)) {    // OK read the file and turn it into a wave object
    Serial.println(F("Not a valid WAV"));
    return;
  }
  wave.play();          // ok time to play! start playback
}

// Plays the ambient background sound while waiting for a trigger
void playbackground(char *name) {
  playfile(name);         // call our helper to find and play this name
  while (wave.isplaying) {      // While the ambient background track is playing watch for triggers
    // Check for user input first
    check_serial();
    if (trigger_check()) {
      play_next_trigger();
    }
  }
  // now it's done playing
}

void play_next_trigger() {
  if (!SD_failed) {                                                // if files exist
    digitalWrite(trigger_LED_pin,HIGH);     // turn on trigger LED
    if (random_mode) {                                              // if in random mode
      playcomplete(trigger_sounds[random(0, trigger_sound_count)]); // play a random sound
    } else {
      playcomplete(trigger_sounds[current_trigger_sound]);          // play the trigger sound
      if (current_trigger_sound < (trigger_sound_count - 1)) {      // if we've reached the end
        current_trigger_sound++;                                  // back to the beginning
      } else {
        current_trigger_sound = 0;
      }
    }
    digitalWrite(trigger_LED_pin,LOW);      // turn off trigger LED
  }
}

// void helper(char *number) {
//   char temp[10] = {};
//   strcat(temp,number);
//   strcat(temp,".wav");
//   playcomplete(temp);
// }


// Monitor the serial port for commands
void check_serial(void) {
  // Check for user input
  if (Serial.available() > 0) {
    c = Serial.read();
      if (c == 104 || c == 105) { // 'h' or 'i'
        print_info();
    }
  }
}


// ---------------------------------- Main Functions ----------------------------------
void setup() {
  Serial.begin(9600);                                 // set up serial port
  Serial.print(F("Hauntbox Sound Module. Free RAM: "));  // Running out of RAM is bad
  Serial.println(freeRam());                          // if this is under 150 bytes it may spell trouble!
  IO_pin_setup();                         //setup all IO pins required
  setup_SD_card();                  //setup SD card

  //SD card logic
  if (SD_failed) {                                    // if the SD failed
    SD_failure_alert();               // alert user
  } else {                                              // if the SD succeeded

    // Whew! We got past the tough parts.
    // Get all of the usable files on the SD card.
    get_files(root);

    // sound file logic -- make sure these are usable wav files
    ambient_mode = does_file_exist(ambient_wav_filename);
    Serial.print(F("ambient = "));
    Serial.println(ambient_mode);

    Serial.println(F("'i' for info"));

    // for (byte i = 0; i < trigger_sound_count; i++) {
    //   Serial.println(trigger_sounds[i]);
    //   playcomplete(trigger_sounds[i]);
    // }
  }
}


void loop() {
  unsigned char c;

  if (!SD_failed) {                                   // if the SD succeeded
    // Check for user input
    check_serial();

    if (ambient_mode) {
      playbackground(ambient_wav_filename);                // play background ambient listening for triggers
    }
    else {
      // Simply check for triggers
      if (trigger_check()) {
        play_next_trigger();
      }
    }
  } else {                                             // if the SD failed
    SD_failure_alert();             // alert user
  }
}

