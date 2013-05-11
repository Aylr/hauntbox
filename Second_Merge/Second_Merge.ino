//Hauntbox Firmware

//#include <avr/wdt.h>    //watchdog timer. Proably not neededa
#include <SPI.h>
#include <Ethernet.h>
#include <Flash.h>
#include <SD.h>
#include <TinyWebServer.h>
#include <EthernetBonjour.h>

//Serial debugging options. Uncomment a row to enable each section as needed.
// #define DEBUG_STATES true             //prints states to serial
// #define DEBUG_FILES true              //prints file conversion details to serial
// #define DEBUG_FILES_BY_CHARACTER true //prints file conversion details character by character to serial
// #define DEBUG_PUT_HANDLER true        //prints file upload details to serial
// #define DEBUG_OUTPUTS true            //prints outputSelect details to serial
// #define DEBUG_INPUTS true             //prints input details to serial
// #define DEBUG_TRIGGERS true           //prints trigger details to serial
// #define DEBUG_BRIDGE true             //prints bridge details to serial
// #define DEBUG_MANUAL true             //prints manual mode details to serial
// #define DEBUG_BOUNJOUR_NAME true      //details regarding custom bonjour naming
// #define DEBUG_IP_ADDRESS  true        //details about static IP address
#define DEBUG_DECIPHER_INPUT_SENSOR true  //details about the inner workings of the decipherIntputSensor() function

#define MAXROWS 20                            //Maximum # of rows. Please adjust MAX_FILE_LENGTH up accordingly
#define MINROWS 1                             //minimum # of rows
#define MIN_BONJOUR_NAME_LENGTH 3             //minimum length of bonjour name. Unreliable below 3.
#define MAX_BONJOUR_NAME_LENGTH 16            //maximum length of bonjour name
#define MAX_FILE_LENGTH (MAXROWS * 34) + 1    //maximum length of program/settings file (major impact on memory)
                                              //should be at least 34 x MAXROWS, as that is the largest input.
                                              // longest valid program string: 34*MAXROWS characters
                                              // longest valid settings string: 318 characters
#define HYSTERESIS_BUFFER_PERCENT 10.0        // percent buffer to add/subtrack to the trigger  low ---|--- buffer --|---- high

//"Program" arrays. Note these arrays are not limited by the 6 IO pins, but the number of rows
bool enableDisableArray[MAXROWS] = {1,1,1,1,1,1};       //if a row is enabled or disabled
byte inputArray[MAXROWS] =       {1, 2, 3, 4, 5, 6};    //which input (0-6) is selected (0 = none, 1 = input #1, 2 = input #2, ...)
bool inputOnOffArray[MAXROWS] =  {1, 1, 1, 1, 1, 1};    //when input is on/off
unsigned long delayArray[MAXROWS] = {0, 0, 0, 0, 0, 0}; //actual delay in milliseconds
byte outputArray[MAXROWS] =      {1, 2, 3, 4, 5, 6};    //which outputs (0-6) is selected (0 = none, 1 = output #1, 2 = output #2, ...)
byte outputOnOffToggleArray[MAXROWS] = {1,1,1,1,1,1};   //What the output should do (on/off/toggle)
byte durationTypeArray[MAXROWS] = {0,1,2,0,1,2};        //The type of duration (0 = until further notice, 1 = while input active, 2 = for ...)
unsigned long durationArray[MAXROWS] = {1000, 6000, 6000, 6000, 6000, 6000};  //actual effect duration in milliseconds
FLASH_STRING(default_program, "1,1;1,2;1,1;0,0;1,2;1,1;2,2;1000,1000;");

//"Settings" arrays. Note these arrays are limited to 6 physical IO pins
bool inputActiveHiLowArray[6] =  {1, 1, 1, 1, 1, 1};                     //What signal level is considered "on" for each input (1 = High, 0 = Low)
bool outputActiveHiLowArray[6] = {1, 1, 1, 1, 1, 1};                     //Output considered on when High (1) or Low (0)
int inputTriggerThresholdArray[6] = {103,103,103,103,103,103};           //input trigger thresholds
unsigned long inputRetriggerDelayArray[6] = {100,100,100,100,100,100};   //retrigger time in milliseconds
FLASH_STRING(default_settings, "1,1,1,1,1,1,1,1,1,1,1,1;garage,my room,hall,cemetery,cornfield,swamp;UV,light,strobe,sound,air horn,zombie;103,103,103,103,103,103;100,100,100,100,100,100;");

// Other arrays
byte inputLEDArray [6] =  {39,32,33,34,35,36};   //Array of arduino pins that correspond to the LEDs that indicate an input is triggered
byte outputLEDArray [6] = {47,46,45,44,43,42};   //Array of arduino pins that correspond to the LEDs that indicate an output is on

// Misc variables
char bonjourName[MAX_BONJOUR_NAME_LENGTH] = "hauntbox";     //default bonjour name if not set in bounjour.txt on SD card
bool guiFlag = true;                          //GUI Flag tells us when there is a new program.txt/settings.txt from the GUI. Start as 1 to load the inital program/settings.
byte currentRowCount = 6;                     //current number of rows (starts at 6 and modified by gui)
byte newCurrentRowCount = 0;                  //used in the bridge to count what the new row count should be
bool automaticMode = true;                    //keeps track of auto/manual override mode
bool networkServicesDisabled = false;         //true if ethernet doesn't start due to IP issue or unplugged CAT5. Still runs state machine, and skips any web.process and bonjour stuff.
bool outputState[6] = {0,0,0,0,0,0};          //array to hold on/off (1/0) state of every given output. Manipulated by any/multiple rules
                                              //***only 6 outputs!!!
byte stateRow[MAXROWS];                       //array that defines each row's state. Gets initialized in initializeFunction called from main function
bool trigState[MAXROWS];                      //gets initialized immediately before it is used
unsigned long delayTimeStamp[MAXROWS];        //gets initialized immediately before it is used
unsigned long timeStampDurationRow[MAXROWS];  //gets initialized immediately before it is used
unsigned long nowTime;                        //used to keep track of now.
unsigned long netTime;                        //used to measure difference between now and delay time
unsigned long netTimeRetrigger;               //single temp variable used to measure retrigger delay
unsigned long nowTimeRetrigger;               //single temp variable used to measure retrigger delay
unsigned long tempRetriggerDelay;             //single temp variable used to measure retrigger delay

//Define I/O pins
int pinIn1 = 10;  //Analog pin
int pinIn2 = 11;  //Analog pin
int pinIn3 = 12;  //Analog pin
int pinIn4 = 13;  //Analog pin
int pinIn5 = 14;  //Analog pin
int pinIn6 = 15;  //Analog pin
int inputPinArray[] = {pinIn1, pinIn2, pinIn3, pinIn4, pinIn5, pinIn6};

int pinOut1 = 49; //Digital pin
int pinOut2 = 48; //Digital pin
int pinOut3 = 38; //Digital pin
int pinOut4 = 41; //Digital pin
int pinOut5 = 40; //Digital pin
int pinOut6 = 37; //Digital pin
int outputPinArray[] = {pinOut1, pinOut2, pinOut3, pinOut4, pinOut5, pinOut6};


//Reserved digital pins for Arduino Ethernet Module SPI
// Pin 4  (SD card chip select)
// Pin 10 (Ethernet Chip select)
// Pin 11 (MOSI)
// Pin 12 (MISO)
// Pin 13 (SCK)

const int SD_CS = 4;                          // pin 4 is the SPI select pin for the SDcard
const int ETHER_CS = 10;                      // pin 10 is the SPI select pin for the Ethernet
byte ip[] = { 192, 168, 0, 100 };             // Static fallback IP if not set in ip.txt on SD card
byte mac[] = { 222, 173, 190, 239, 254, 238}; // static fallback MAC address if not set in uniqueID.txt on SD card

FLASH_STRING(html_browser_header, "HTTP/1.0 200 OK\nContent-Type: text/html\n");  //2 line header including mandatory blank line to signify data below
// Store frequently used strings in eprom flash to save on ram.
FLASH_STRING(prefixSTATUS, "STATUS: ");
FLASH_STRING(prefixDEBUG_FILES, "DEBUG_FILES: ");
FLASH_STRING(prefixDEBUG_BONJOUR_NAME, "DEBUG_BOUNJOUR_NAME: ");
FLASH_STRING(prefixDEBUG_BRIDGE, "DEBUG_BRIDGE:");
FLASH_STRING(prefixDEBUG_MANUAL, "DEBUG_MANUAL: ");
FLASH_STRING(prefixDEBUG_STATES, "DEBUG_STATES: ");
FLASH_STRING(prefixDEBUG_OUTPUTS, "DEBUG_OUTPUTS: ");
//below is the bare bones html for when there's no SD card
FLASH_STRING(sd_fail_html,"<html><body><h1>SD Card Failed</h1><ol><li>Verify your SD card works</li><li>Remove and reseat it in SD slot in hauntbox</li><li>Reset hauntbox</li><li>Reload this page in 30 seconds</li></ol></body></html>");


//--------------------------- Define Handlers ----------------------------
boolean file_handler(TinyWebServer& web_server);
boolean index_handler(TinyWebServer& web_server);
boolean program_handler(TinyWebServer& web_server);
boolean settings_handler(TinyWebServer& web_server);

TinyWebServer::PathHandler handlers[] = {
  // Work around Arduino's IDE preprocessor bug in handling /* inside strings.
  //
  // `put_handler' is defined in TinyWebServer
  // {"/", TinyWebServer::GET, &index_handler },
  {"/", TinyWebServer::GET, &index_handler },
  {"/ram", TinyWebServer::GET, &ram_handler },
  {"/status", TinyWebServer::GET, &status_handler },
  {"/program", TinyWebServer::GET, &program_handler },
  {"/settings", TinyWebServer::GET, &settings_handler },
  {"/manual", TinyWebServer::POST, &manual_handler },
  {"/all_off", TinyWebServer::GET, &all_off_handler },
  {"/all_on", TinyWebServer::GET, &all_on_handler },
  {"/trigger_all", TinyWebServer::GET, &trigger_all_handler },
  {"/automatic_on", TinyWebServer::GET, &automatic_on_handler },
  {"/automatic_off", TinyWebServer::GET, &automatic_off_handler },
  {"/mode", TinyWebServer::GET, &mode_handler },
  {"/trigger", TinyWebServer::POST, &trigger_handler },
  {"/upload/" "*", TinyWebServer::PUT, &TinyWebPutHandler::put_handler },
  {"/" "*", TinyWebServer::GET, &file_handler },
  {NULL},
};

const char* headers[] = {
  "Content-Length",
  NULL
};

TinyWebServer web = TinyWebServer(handlers, headers);

boolean has_filesystem = true;
Sd2Card card;
SdVolume volume;
SdFile root;
SdFile file;


// -------------------- send file handler -------------------- 
void send_file_name(TinyWebServer& web_server, const char* filename) {
  if (!filename) {
    web_server.send_error_code(404);
    web_server << F("Could not parse URL");
  } else {
    TinyWebServer::MimeType mime_type = TinyWebServer::get_mime_type_from_filename(filename);
    if (file.open(&root, filename, O_READ)) {
      web_server.send_error_code(200);
      web_server.send_content_type(mime_type);
      web_server.end_headers();

      Serial << F("Read file "); Serial.println(filename);
      web_server.send_file(file);
      file.close();
    } else {
      web_server.send_error_code(404);
      web_server << F("Could not find file: ") << filename << "\n";
    }
  }
}


// -------------------- file handler -------------------- 
boolean file_handler(TinyWebServer& web_server) {
  char* filename = TinyWebServer::get_file_from_path(web_server.get_path());
  send_file_name(web_server, filename);
  free(filename);
  return true;
}


// -------------------- index handler -------------------- 
boolean index_handler(TinyWebServer& web_server) {
  if(has_filesystem){                             //if SD is working send main gui file
    send_file_name(web_server, "gui.htm");
  }else{                                          //if SD has failed, send an informative help page
    Client& client = web_server.get_client();
    client << html_browser_header;
    client << sd_fail_html;                       //the actual SD fail html page is stored in eprom flash
  }
  return true;
}


// -------------------- send program + status handler -------------------- 
boolean program_handler(TinyWebServer& web_server){
  send_file_name(web_server, "program.txt");
  Client& client = web_server.get_client();
  client << "0;" << FreeRam() << ";M" << (bool)automaticMode << ";\n";
  client.stop();
  return true; //exit the handler 
}

// -------------------- send settings + status handler -------------------- 
boolean settings_handler(TinyWebServer& web_server){
  send_file_name(web_server, "settings.txt");
  Client& client = web_server.get_client();
  client << "0;" << FreeRam() << ";M" << (bool)automaticMode << ";\n";
  client.stop();
  return true; //exit the handler 
}


// -------------------- ram handler -------------------- 
boolean ram_handler(TinyWebServer& web_server) {
  web_server.send_error_code(200);
  web_server.send_content_type("text/plain");
  web_server.end_headers();
  Client& client = web_server.get_client();
  client.println(FreeRam());
  client.stop();
  return true;
}


// -------------------- manual handler -------------------- 
boolean manual_handler(TinyWebServer& web_server) {
  web_server.send_error_code(200);
  web_server.send_content_type("text/plain");
  web_server.end_headers();
  Client& client = web_server.get_client();

  if (client.available()) {
    bool tempOnOff = 0;             //keeps track of if we want it on or off
    byte tempOutput = 0;            //keeps track of which input
    
    char ch = (char)client.read();  //throw away the first character in "a=11"
    ch = (char)client.read();       //throw away the second character in "a=11"
    ch = (char)client.read();       //now get the first integer 1
    char ch2 = (char)client.read(); //now get the second integer 1
    
    // tempOnOff = atoi(&ch2);
    if (ch2 == '1'){                //this could be cleaned probably by using atoi
      tempOnOff = 1;
    }else{
      tempOnOff = 0;
    }
    
    // tempOutput = atoi(&ch);        //convert char address to an integer
    if (ch == '1'){                   //this could be cleaned probably by using atoi
      tempOutput = 1;
    }else if (ch == '2'){
      tempOutput = 2;
    }else if (ch == '3'){
      tempOutput = 3;
    }else if (ch == '4'){
      tempOutput = 4;
    }else if (ch == '5'){
      tempOutput = 5;
    }else if (ch == '6'){
      tempOutput = 6;
    }else{                      //if you get something besides 1-6, exit the handler
      #ifdef DEBUG_MANUAL
        Serial << prefixDEBUG_MANUAL << F("Bad data\n");
      #endif
      return true;
    }

    outputState[tempOutput-1] = tempOnOff;        //set map, remembering to shift by minus 1
    actuallyChangeOutput(tempOutput, tempOnOff);  // set reality

    #ifdef DEBUG_MANUAL
      Serial << prefixDEBUG_MANUAL << F("Raw: ") << ch << " " << ch2 << F(" Converted: ") << tempOutput << " " << tempOnOff << F("\n");
      Serial << prefixDEBUG_MANUAL << "outputState[tempOutput-1]=" << outputState[tempOutput-1] << " tempOutput=" << tempOutput << " tempOutput-1=" << tempOutput-1 << F("\n");
    #endif
  }
  return true;
}

// -------------------- trigger handler -------------------- 
boolean trigger_handler(TinyWebServer& web_server) {
  web_server.send_error_code(200);
  web_server.send_content_type("text/plain");
  web_server.end_headers();
  Client& client = web_server.get_client();

  if (client.available()) {
    int tempInput = 0;             //keeps track of which input
    
    char ch = (char)client.read();  //throw away the first character in "a=1"
    ch = (char)client.read();       //throw away the second character in "a=1"
    ch = (char)client.read();       //now get the first integer 1
    
    tempInput = atoi(&ch);          //convert char address to an integer

    if (tempInput < 1 || tempInput > 6){    //if bad data, exit handler
      #ifdef DEBUG_MANUAL
        Serial << prefixDEBUG_MANUAL << F("Bad data\n");
      #endif
      return true;                          //exit handler
    }
    
    #ifdef DEBUG_MANUAL
      Serial << prefixDEBUG_MANUAL << F("Raw: ") << ch << F(" Converted: ") << tempInput << F("\n");
    #endif

    //loop through each row to see if it is controlled by the triggered input
    for (int i=0; i < currentRowCount; i++){
      if (inputArray[i] == tempInput){      //if the row is controlld by the input
        stateRow[i] = 2;                    //set as triggered in the state machine
      }
    }
    
  }
  return true;
}


// -------------------- trigger_all handler -------------------- 
boolean trigger_all_handler(TinyWebServer& web_server) {   //triggers all inputs on
  web_server.send_error_code(200);
  web_server.send_content_type("text/plain");
  web_server.end_headers();
  
  #ifdef DEBUG_MANUAL
    Serial << prefixDEBUG_MANUAL << F("ALL\n");
  #endif
  
  for (byte i=0;i<=currentRowCount;i++){
    stateRow[i] = 2;          //set all to state 2
  }
  return true;
}


// -------------------- trigger_all handler -------------------- 
boolean automatic_on_handler(TinyWebServer& web_server) {   //turns on automatic mode
  web_server.send_error_code(200);
  web_server.send_content_type("text/plain");
  web_server.end_headers();
  #ifdef DEBUG_MANUAL
    Serial << prefixDEBUG_MANUAL << F("Automatic Mode ON\n");
  #endif

  automaticMode = true;
  return true;
}


// -------------------- trigger_all handler -------------------- 
boolean automatic_off_handler(TinyWebServer& web_server) {   //turns off automatic mode, ignoring physical input
  web_server.send_error_code(200);
  web_server.send_content_type("text/plain");
  web_server.end_headers();
  #ifdef DEBUG_MANUAL
    Serial << prefixDEBUG_MANUAL << F("Automatic Mode OFF\n");
  #endif
    
  automaticMode = false;
  return true;
}


// -------------------- mode handler -------------------- 
boolean mode_handler(TinyWebServer& web_server) {
  web_server.send_error_code(200);
  web_server.send_content_type("text/plain");
  web_server.end_headers();
  Client& client = web_server.get_client();
  client << "M" << (bool)automaticMode << F("\n");
  client.stop();
  return true;
}


// -------------------- all_off handler -------------------- 
boolean all_off_handler(TinyWebServer& web_server) {  //turns all outputs off
  web_server.send_error_code(200);
  web_server.send_content_type("text/plain");
  web_server.end_headers();
  
  #ifdef DEBUG_MANUAL
    Serial << prefixDEBUG_MANUAL << F("All Off\n");
  #endif
  
  for (byte i=1;i<=6;i++){
    outputState[i-1] = 1;        //set map (shift -1)
    actuallyChangeOutput(i, 0);  //set reality (takes actual output #, not shifted)
  }
  return true;
}


// -------------------- all_on handler -------------------- 
boolean all_on_handler(TinyWebServer& web_server) {   //turns all outputs on
  web_server.send_error_code(200);
  web_server.send_content_type("text/plain");
  web_server.end_headers();
  
  #ifdef DEBUG_MANUAL
    Serial << prefixDEBUG_MANUAL << F("All On\n");
  #endif
  
  for (byte i=1;i<=6;i++){
    outputState[i-1] = 1;        //set map (shift -1)
    actuallyChangeOutput(i, 1);  //set reality (takes actual output #, not shifted)
  }
  return true;
}


// -------------------- status handler -------------------- 
boolean status_handler(TinyWebServer& web_server) {   //returns trigState[];stateRow[];outputState[];
  web_server.send_error_code(200);
  web_server.send_content_type("text/plain");
  web_server.end_headers();
  Client& client = web_server.get_client();

  for (int i = 0; i < currentRowCount; i++){
    client.print(trigState[i], DEC);
    if(i < (currentRowCount - 1)){
      client.print(",");
    }
  }
  client.print(";");
  for (int i = 0; i < currentRowCount; i++){
    client.print(stateRow[i], DEC);
    if(i < (currentRowCount - 1)){
      client.print(",");
    }
  }
  client.print(";");
  for (int i = 0; i < currentRowCount; i++){
    client.print(outputState[i], DEC);
    if(i < (currentRowCount - 1)){
      client.print(",");
    }
  }
  client.print(";");
  return true;
}


// -------------------- file uploader -------------------- 
byte tempGuiFlag = 0;   //remembers guiFlag state in between loops (this is called 3 times: start, write, end)
                        //so we have to only set the real guiFlag after all 3 steps
void file_uploader_handler(TinyWebServer& web_server, TinyWebPutHandler::PutAction action, char* buffer, int size) {
  static uint32_t start_time;
  static uint32_t total_size;
  char* fname;


  switch (action) {
  case TinyWebPutHandler::START:
    start_time = millis();
    total_size = 0;
    if (!file.isOpen()) {
      // File is not opened, create it. First obtain the desired name
      // from the request path.
      fname = web_server.get_file_from_path(web_server.get_path());
      if (fname) {
        #ifdef DEBUG_PUT_HANDLER
        	Serial << F("DEBUG_PUT_HANDLER: Creating ") << fname << F("\n");
      	#endif

        file.open(&root, fname, O_CREAT | O_WRITE | O_TRUNC);

        //IF PROGRAM.TXT OR SETTINGS.TXT WERE UPDATED, TRIGGER UPDATE ARRAYS
        if ((strcmp(fname,"PROGRAM.TXT") == 0) || (strcmp(fname,"SETTINGS.TXT") == 0)){    //strcmp compares the pointer to the string 0 means equal.
          tempGuiFlag = 1;
        }
        #ifdef DEBUG_PUT_HANDLER
          Serial << F("DEBUG_PUT_HANDLER: case START tempGuiFlag=") << tempGuiFlag << F("\n");
        #endif
        
	     free(fname);
      }
    }
    break;

  case TinyWebPutHandler::WRITE:
    if (file.isOpen()) {
      file.write(buffer, size);
      total_size += size;
    }
    break;

  case TinyWebPutHandler::END:
    file.sync();
    #ifdef DEBUG_PUT_HANDLER
      Serial << F("DEBUG_PUT_HANDLER: Wrote ") << file.fileSize() << F(" bytes in ") << millis() - start_time << F(" millis (received ") << total_size << F(" bytes)\n");
    #endif

    file.close();
    if (tempGuiFlag == 1){
      guiFlag = 1;
      tempGuiFlag = 0;
      #ifdef DEBUG_PUT_HANDLER
        Serial << F("DEBUG_PUT_HANDLER: case END tempGuiFlag = ") << tempGuiFlag << F("\n");
      #endif
    }
  }
}


// -------------------- begin firmware -------------------- 
void setup() {
  /*
  if ((MCUSR & _BV(PORF)) && (MCUSR & _BV(EXTRF))) {
    Serial.println("Down with the ship!!");
    MCUSR = 0;
  }
  */

  //This portion detects how it was reset and resets if it was a PORF (power on reset flag)
  /*
  if (MCUSR & _BV(WDRF)) {          //if the watchdog reset flag is true
    Serial.println("WDRF true");
    Serial.println(MCUSR, HEX);
    MCUSR = 0;                      //reset register
    wdt_disable();                  //disable watchdog timer, because we already had a reset
    Serial.println(MCUSR, HEX);
  }else if (MCUSR & _BV(PORF)) {
    Serial.println("enabling reset");
    Serial.println(MCUSR, HEX);
    wdt_enable(WDTO_1S);            //reset atmel after 1 second
  } else {
    Serial.println("something else");
    Serial.println(MCUSR, HEX);
    MCUSR = 0;                      //reset register
  }
  */
    
  Serial.begin(115200);
  Serial << F("Starting up Hauntbox. Free RAM: ") << FreeRam() << "\n";
  int i;

  pinMode(SS_PIN, OUTPUT);	// set the SS pin as an output (necessary to keep the board as master and not SPI slave)
  digitalWrite(SS_PIN, HIGH);	// and ensure SS is high

  // Ensure we are in a consistent state after power-up or a reset button These pins are standard for the Arduino w5100 Rev 3 ethernet board They may need to be re-jigged for different boards
  pinMode(ETHER_CS, OUTPUT); 	// Set the CS pin as an output
  digitalWrite(ETHER_CS, HIGH); // Turn off the W5100 chip! (wait for configuration)
  pinMode(SD_CS, OUTPUT);       // Set the SDcard CS pin as an output
  digitalWrite(SD_CS, HIGH); 	// Turn off the SD card! (wait for configuration)

  //Set up arduino pins as outputs
  pinMode(pinOut1, OUTPUT);
  pinMode(pinOut2, OUTPUT);
  pinMode(pinOut3, OUTPUT);
  pinMode(pinOut4, OUTPUT);
  pinMode(pinOut5, OUTPUT);
  pinMode(pinOut6, OUTPUT);

  for (int i = 0;i <=5;i++){  // loop through all inputLEDs & outputLEDS and set as outputs
    pinMode(inputLEDArray[i], OUTPUT);  //set inputLED as output
    pinMode(outputLEDArray[i], OUTPUT); //set outoutLED as output
  }

  directionalLEDFlasher(1,1,30,0);

  //--------------------------------- Set up SD card ---------------------------------------
  Serial << F("Setting up SD card...\n");

  if (!card.init(SPI_HALF_SPEED, SD_CS)) {      //Pass over the speed and Chip select for the SD card
  // if (!card.init(SPI_FULL_SPEED, SD_CS)) {
    Serial << F("SD card failed\n");
    has_filesystem = false;
  }
  if (!volume.init(&card)) {                  //initialize a FAT volume.
    Serial << F("vol.init failed!\n");
    has_filesystem = false;
  }
  if (!root.openRoot(&volume)) {              //open the volume
    Serial << F("openRoot failed");
    has_filesystem = false;
  }
  
  if (has_filesystem) {
    TinyWebPutHandler::put_handler_fn = file_uploader_handler;   // Assign our function to `upload_handler_fn'.

    //----------------------------------- load uniqueID.txt -----------------------------------
    char* mac_addr_temp = open_file("uniqueID.txt");    //try to open uniqueID.txt
    if (mac_addr_temp != ""){  
      char* tok;
      char* val[6];
      
      tok = strtok(mac_addr_temp, ":.-_, \r\n");   //separate string using ":" as a delimiter
      val[0] = tok;
      for (i = 1; i < 6; i++) {
        if (tok == NULL)
          break;
        tok = strtok(NULL, ":.-_, \r\n");
        val[i] = tok;
      }
  
      for (i = 0; i < 6; i++) {
        Serial << (val[i]) << ' ';
      }
      Serial << F("Changed mac from ");
      int vb;
      for (vb = 0; vb < 6; vb++)
        Serial << mac[vb] << ' ';
      Serial << F(" to ");
      for (i = 0; i < 6; i++) {
        mac[i] = (byte)atoi(val[i]);
      }
      for (vb = 0; vb < 6; vb++)
        Serial << mac[vb] << ' ';
      Serial << F("!\n");
    }else{      //use default mac address specified above.
      statusMessage(11);
    }// end if uniqueID.txt exists
  	
    //----------------------------------- load bonjour.txt -----------------------------------
    #ifdef DEBUG_BOUNJOUR_NAME
      Serial << prefixDEBUG_BONJOUR_NAME << F("Trying to open bonjour.txt\n");
    #endif

    char* bonjour_file = open_file("bonjour.txt");    //try to open bonjour.txt

    if (bonjour_file !=""){                           //if there is data in the file
      #ifdef DEBUG_BOUNJOUR_NAME
        Serial << prefixDEBUG_BONJOUR_NAME << F("found bonjour.txt\n");
      #endif

      char* tok;                                //Get rid of extraneous characters
      tok = strtok(bonjour_file, ".:-',?*&^#@!()\n\r ");   //separate string using ".:- " or newline as a delimiter
      strcpy(bonjourName, tok);                 //copy the read string into bonjourName (Note you can't just say bonjourName = bonjourFile)

    }else{                                      //if no file or not there use default set initially
      #ifdef DEBUG_BOUNJOUR_NAME
        Serial << prefixDEBUG_BONJOUR_NAME << F("No bonjour.txt file exists\n");
      #endif
    }// end if bonjour.txt exists

    //----------------------------------- load ip.txt -----------------------------------
    char* ip_temp = open_file("ip.txt");    //try to open ip.txt
    if (ip_temp != ""){  
      char* tok;
      char* val[4];
      
      tok = strtok(ip_temp, ".");   //separate string using "." as a delimiter
      val[0] = tok;
    	for (i = 1; i < 4; i++) {
    	  if (tok == NULL)
    	    break;
    	  tok = strtok(NULL, ".");
    	  val[i] = tok;
    	}

      #ifdef DEBUG_IP_ADDRESS
        Serial << F("DEBUG_IP_ADDRESS: ");
        for (i = 0; i < 4; i++) {
          Serial << val[i] << ".";
        }
        Serial.println();
      #endif

      for (i = 0; i < 4; i++) {
        ip[i] = (byte)atoi(val[i]);
      }
      #ifdef DEBUG_IP_ADDRESS
        Serial << F("DEBUG_IP_ADDRESS: Static IP: ") << ip_temp << F("\n");
      #endif
      Ethernet.begin(mac,ip);             //set up with static address
    }else{                                //if there is not a static ip specified... use DHCP
      Serial << F("Setting up the Ethernet card...\n");
      if (Ethernet.begin(mac) == 0) {     // Initialize ethernet with DHCP
         disableNetworkServices();        // disable any network services like web.process and bonjour
      }
    }
  }else{                                                    //If there is NO FILESYSTEM
    Serial << F("Setting up the Ethernet card...\n");       //still setup ethernet
    if (Ethernet.begin(mac) == 0) {                         // Initialize ethernet with DHCP
      disableNetworkServices();       // disable any network services like web.process and bonjour
    }

    statusMessage(12);
    LEDFlasher(10,200,200);  //visually alert user that something is awry by flashing all LEDs
  }//end if has filesystem
  
  if (!networkServicesDisabled){           //if the hauntbox doesn't appear to be on a network, disable network services
    Serial << F("Web server starting...\n");
    web.begin();                 // Start the web server.
  
    // Start the bonjour/zeroconf service
    EthernetBonjour.begin(bonjourName);                       //Set the advertised name
    char bonjourServiceRecord[MAX_BONJOUR_NAME_LENGTH + 6];   //declare char array to hold bonjourServiceRecord
    strcpy(bonjourServiceRecord, bonjourName);                //copy user-changeable name from http://stackoverflow.com/questions/2218290/concatenate-char-array-in-c
    strcat(bonjourServiceRecord, "._http");                   //copy required postfix (needs to have the "._http" at the end)

    #ifdef DEBUG_BOUNJOUR_NAME
      Serial << prefixDEBUG_BONJOUR_NAME << F("bonjourName: ") << bonjourName << F(" bonjourServiceRecord: ") << bonjourServiceRecord << F("\n");
    #endif

    EthernetBonjour.addServiceRecord(bonjourServiceRecord, 80, MDNSServiceTCP);   //Set the advertised port/service

    Serial << F("Ready to accept web requests at http://") << bonjourName << F(".local or at http://");
    Serial.println(Ethernet.localIP());
  }

  directionalLEDFlasher(0,1,0,30);                            //turn bootup LEDs off to indicate complete bootup!
} // end setup()


//----- AA1b -- Initialize Output Function -----
// Function that initializes the outputs states to "off"
void initializeOutputs() {
  for(int i = 1; i <= 6; i++) {
    outputState[i-1] = 0;     //start with all pins off.
    actuallyChangeOutput(i, outputState[i-1]);  //set output according to outputState[] map
  }
}

//----- Section AA4b -----
// Function that reads sensor, interprets what the value means (active hi/low) and 
//returns int 1 for "triggered" or 0 for "not triggered"
bool decipherInputSensor(byte x) {
  // It is very important to note that this takes an input # from 1-6 and
  // NOT 0-5. In this firmware an input = 0 is considered not active, or NO input.
  // This is why you see some of the indexes shifted + or - 1.
  bool trig = false;                      //returns 1 if input is considered triggered, 0 if not
  int val;
  int y;
  float thresholdWithBuffer;

  if(x == 0 || x > 6) {return 0;}         //Do nothing for "N/A" case and break out of function
  if(x == 1) {val = analogRead(pinIn1);}
  if(x == 2) {val = analogRead(pinIn2);}
  if(x == 3) {val = analogRead(pinIn3);}
  if(x == 4) {val = analogRead(pinIn4);}
  if(x == 5) {val = analogRead(pinIn5);}
  if(x == 6) {val = analogRead(pinIn6);}

  y = inputActiveHiLowArray[x-1];         // Returns Acvive High/Low definition for corresponding input
      // x-1 because in the inputArray we are using 0 as "none"
      // whereas in inputActiveHiLowArray & inputTriggerThresholdArray we are not
  if (y == 1) thresholdWithBuffer = (float)inputTriggerThresholdArray[x-1] * ((100.0 + HYSTERESIS_BUFFER_PERCENT) / 100.0);     //do a little maths to add a hysteresis buffer
  if (y == 0) thresholdWithBuffer = (float)inputTriggerThresholdArray[x-1] * ((100.0 - HYSTERESIS_BUFFER_PERCENT) / 100.0);


  if((y == 1) && (val >= thresholdWithBuffer)) {trig = 1;}    // if high and supposed to be, consider input "triggered"
  if((y == 0) && (val >= thresholdWithBuffer)) {trig = 0;}    // if low and supposed to be high, don't
  if((y == 1) && (val <  thresholdWithBuffer)) {trig = 0;}
  if((y == 0) && (val <  thresholdWithBuffer)) {trig = 1;}    // if low and supposed to be, consider input "triggered"

  #ifdef DEBUG_DECIPHER_INPUT_SENSOR
    if (trig == 1)  Serial << "DEBUG_DECIPHER_INPUT_SENSOR: x=" << x << " val=" << val << " inputTriggerThresholdArray[x-1]=" << inputTriggerThresholdArray[x-1] <<  " thresholdWithBuffer=" << thresholdWithBuffer << " trig=" << trig << "\n";
  #endif

  return trig;
}


//function that matches up the input state (triggered or not) with
//the GUI dropdown "When input is [on/off]" to see if action should be taken
  //The logic is deceptively simple since we are using booleans.
  //If triggered (1) == on (1)  --> take action (1)
  //if triggered (1) != off (0) --> don't
  //if not trig (0)  == off (0) --> take action (1)
  //if not trig (0)  != on (1)  --> don't
  //Therefore, only take action when they == each other
bool inputTakeAction(byte rowNumber) {
  bool shouldItTakeAction;
  if ( decipherInputSensor(inputArray[rowNumber]) == inputOnOffArray[rowNumber] ){
    shouldItTakeAction = 1;
  }else{
    shouldItTakeAction = 0;
  }
  return shouldItTakeAction;
}

//  Function to write messages to gui            CURRENTLY WRITTEN FOR SERIAL, NOT GUI!
void statusMessage(byte n) {
  if (n==1) {Serial << prefixSTATUS << F("Definitions don't make sense\n"); return;}
  if (n==2) {Serial << prefixSTATUS << F("No SD Card anymore\n"); return;}
  if (n==3) {Serial << prefixSTATUS << F("Problem opening file on SD card\n"); return;}
  if (n==4) {Serial << prefixSTATUS << F("Corrupted SD file -Read/Write fail\n"); return;}
  if (n==5) {Serial << prefixSTATUS << F("Successfully updated programming\n"); return;}
  if (n==6) {Serial << prefixSTATUS << F("Finished writing to .txt file\n"); return;}
  if (n==7) {Serial << prefixSTATUS << F("Default settings used.\n"); return;}
  if (n==8) {Serial << prefixSTATUS << F("Default program used.\n"); return;}
  if (n==9) {Serial << prefixSTATUS << F("Error creating new default file. Check SD and reset.\n"); return;}
  if (n==10) {Serial << prefixSTATUS << F("Ethernet failed. Check network connections and reset Hauntbox. Proceeding without network services.\n"); return;}
  if (n==11) {Serial << prefixSTATUS << F("No uniqueID.txt file. Using default MAC address.\n"); return;}
  if (n==11) {Serial << prefixSTATUS << F("****** Warning: SD not working. Check card, formatting and reset.\n"); return;}
}


//----- Section AA4c -----
// Function that pairs the output pins to the row that is controlling for it
void outputSelectFunction(int rowNumber, bool action){
  // Looks up an output from a given row number and passes along the action
  // to the actuallyChangeOutput() function that enacts the change
  #ifdef DEBUG_OUTPUTS
    Serial << prefixDEBUG_OUTPUTS << F("outputSelectFunction: rowNumber=") << rowNumber << F(" mapped output=") << outputArray[rowNumber] << "\n";
  #endif
  actuallyChangeOutput(outputArray[rowNumber],action);    // make the change by passing the actual output # (1-6 on the PCB) and action
}

// void actuallyChangeOutput(byte outputNumber, bool action) {    // takes an output number
//   // from 1-6 corresponding with outputs on Hauntbox labeled 1-6.
//   // If output 0 is passed, it is ignored.
//   //takes an outputNumber and an action: 0 = off, 1 = on
//   #ifdef DEBUG_OUTPUTS
//     Serial << prefixDEBUG_OUTPUTS << F("actuallyChangeOutput outputNumber=") << outputNumber << F(" action=") << action << F("\n");
//   #endif

//   byte x = outputNumber;                 // change to x for compact readability
//   bool y = outputActiveHiLowArray[x-1];  // lookup which value is considered "on" (shifted by -1)
  
//   if(action == 1) {        // turn output on
//     if(y == 1) {           // "on" means turn output high
//       if(x == 0) {return;} //Do nothing for "N/A" case and break out of function
//       if(x == 1) {digitalWrite(pinOut1, HIGH); digitalWrite(outputLEDArray[0], HIGH); }
//       if(x == 2) {digitalWrite(pinOut2, HIGH); digitalWrite(outputLEDArray[1], HIGH); }
//       if(x == 3) {digitalWrite(pinOut3, HIGH); digitalWrite(outputLEDArray[2], HIGH); }
//       if(x == 4) {digitalWrite(pinOut4, HIGH); digitalWrite(outputLEDArray[3], HIGH); }
//       if(x == 5) {digitalWrite(pinOut5, HIGH); digitalWrite(outputLEDArray[4], HIGH); }
//       if(x == 6) {digitalWrite(pinOut6, HIGH); digitalWrite(outputLEDArray[5], HIGH); } }
//     if(y == 0) {           // "on" means turn output low
//       if(x == 0) {return;} //Do nothing for "N/A" case and break out of function
//       if(x == 1) {digitalWrite(pinOut1, LOW); digitalWrite(outputLEDArray[0], HIGH);}
//       if(x == 2) {digitalWrite(pinOut2, LOW); digitalWrite(outputLEDArray[1], HIGH);}
//       if(x == 3) {digitalWrite(pinOut3, LOW); digitalWrite(outputLEDArray[2], HIGH);}
//       if(x == 4) {digitalWrite(pinOut4, LOW); digitalWrite(outputLEDArray[3], HIGH);}
//       if(x == 5) {digitalWrite(pinOut5, LOW); digitalWrite(outputLEDArray[4], HIGH);}
//       if(x == 6) {digitalWrite(pinOut6, LOW); digitalWrite(outputLEDArray[5], HIGH);} }
//     return; }
//   if(action == 0) {        //turn output off
//     if(y == 1) {           // "off" means turn output low
//       if(x == 0) {return;} //Do nothing for "N/A" case and break out of function
//       if(x == 1) {digitalWrite(pinOut1, LOW); digitalWrite(outputLEDArray[0], LOW);}
//       if(x == 2) {digitalWrite(pinOut2, LOW); digitalWrite(outputLEDArray[1], LOW);}
//       if(x == 3) {digitalWrite(pinOut3, LOW); digitalWrite(outputLEDArray[2], LOW);}
//       if(x == 4) {digitalWrite(pinOut4, LOW); digitalWrite(outputLEDArray[3], LOW);}
//       if(x == 5) {digitalWrite(pinOut5, LOW); digitalWrite(outputLEDArray[4], LOW);}
//       if(x == 6) {digitalWrite(pinOut6, LOW); digitalWrite(outputLEDArray[5], LOW);} }
//     if(y == 0) {           // "off" means turn output high
//       if(x == 0) {return;} //Do nothing for "N/A" case and break out of function
//       if(x == 1) {digitalWrite(pinOut1, HIGH); digitalWrite(outputLEDArray[0], LOW); }
//       if(x == 2) {digitalWrite(pinOut2, HIGH); digitalWrite(outputLEDArray[1], LOW); }
//       if(x == 3) {digitalWrite(pinOut3, HIGH); digitalWrite(outputLEDArray[2], LOW); }
//       if(x == 4) {digitalWrite(pinOut4, HIGH); digitalWrite(outputLEDArray[3], LOW); }
//       if(x == 5) {digitalWrite(pinOut5, HIGH); digitalWrite(outputLEDArray[4], LOW); }
//       if(x == 6) {digitalWrite(pinOut6, HIGH); digitalWrite(outputLEDArray[5], LOW); } }
//     return; }
//   }

void actuallyChangeOutput(byte outputNumber, bool action) {    // takes an output number
  // from 1-6 corresponding with outputs on Hauntbox labeled 1-6.
  // If output 0 is passed, it is ignored.
  //takes an outputNumber and an action: 0 = off, 1 = on
  
  bool y = outputActiveHiLowArray[outputNumber-1];
  int highOrLow;

  if (outputNumber == 0) return;  //break out of function for case "n/a" (represented as output 0)
  if (action == y) highOrLow = HIGH;
  if (action == !y) highOrLow = LOW;

  #ifdef DEBUG_OUTPUTS
    Serial << prefixDEBUG_OUTPUTS << F("actuallyChangeOutput outputNumber=") << outputNumber << F(" action=") << action << F(". Arduino pin=") << outputPinArray[outputNumber-1] << F(" HIGH/LOW=") << highOrLow << F("\n");
  #endif

  digitalWrite(outputPinArray[outputNumber-1],highOrLow);   // set the output HIGH or LOW (depending on what that means)
  digitalWrite(outputLEDArray[outputNumber-1],action);      // set the outputLED according to the action "on" or "off"
}

//-------------------- Main Function ------------------------
void loop(){  
  //----- Section AA9 ----- See if program or settings have changed
  if(guiFlag == 1) {            // If there are new program/settings. Or upon first boot.
    loadProgramAndSettings();   // load the new program/settings from the GUI
    initializeOutputs();        // initialize the outputs according to the new program/settings
    guiFlag = 0;                // reset guiFlag
  }
  

  //----- Section AA4a ----- Main State Machine: Reading Inputs and Writing to Outputs -----
  for(int z = 0; z < currentRowCount; z++) {              //runs loop for each row
    #ifdef DEBUG_STATES
      Serial << prefixDEBUG_STATES << F("") << millis() << F("\n");
    #endif
    if(enableDisableArray[z] == 1) {   //only run the state machine for a row that's enabled
      if(stateRow[z] == 1) {                              //STATE 1 = Waiting for a trigger
        trigState[z] = inputTakeAction(z);                //Call function and pass(Row number) to see if input is triggered
        if(trigState[z] == 1 && automaticMode == true) {  //If triggered AND in automaticMode

          //loop through each row to see if it is controlled by the triggered input
          for (int i=0; i < currentRowCount; i++){
            if (inputArray[i] == inputArray[z]){      //if the row is controlled by the currently triggered input
              trigState[i] = 1;                       //set trigState as triggered
              stateRow[i] = 2;                        //advance to next state
            }
          }

          stateRow[z] = 2;                     //Moves to next state
        }else{//if(trigState[z] == 0) {        //If not triggered (or not in automaticMode)
          //Do nothing
        }

      }else if(stateRow[z] == 2) {              //STATE 2 = Trigger message just received
        delayTimeStamp[z] = millis();           //Gets time stamp
        
        #ifdef DEBUG_TRIGGERS
          Serial << F("DEBUG_TRIGGERS: Trigger Row ") << z+1 << F(": ") << delayTimeStamp[z] << F("\n");     // z+1 is the row number starting from 1 for the user, not starting from 0 for the array
        #endif
        stateRow[z] = 3;                        //Moves on to next state

      }else if(stateRow[z] == 3) {              //STATE 3 = Delay vs. timeStamp
        nowTime = millis();
        netTime = nowTime - delayTimeStamp[z];
        if(netTime >= delayArray[z]) {   //Tests to see if time > delay
          stateRow[z] = 4;                    //If we've met our delay, go to next state
        }

      }else if(stateRow[z] == 4) {             //STATE 4 = Change output (make it on/off/toggle)
        if (outputOnOffToggleArray[z] == 0){                              // if it should be off
          outputState[outputArray[z]-1] = 0;                                // turn it off
        }else if(outputOnOffToggleArray[z] == 1){                         // if it should be on
          outputState[outputArray[z]-1] = 1;                                // turn it on
        }else if(outputOnOffToggleArray[z] == 2){                         // if it should toggle
          outputState[outputArray[z]-1] = !outputState[outputArray[z]-1];   // toggle = flip the bit
        }
        outputSelectFunction(z, outputState[outputArray[z]-1]);  // enact the change
        timeStampDurationRow[z] = millis();       // Get timestamp
        stateRow[z] = 5;                          // Moves on to next state

      }else if(stateRow[z] == 5) {             //STATE 5 = Duration of output "on"
        #ifdef DEBUG_STATES
          printState(z);
        #endif

        //switch for 3 different duration types
        if (durationTypeArray[z] == 0) {        //"until further notice"
          stateRow[z] = 6;                            //move to next state (retrigger delay)
        }else if(durationTypeArray[z] == 1) {   //"while input triggered"
          trigState[z] = inputTakeAction(z);          //Call function and pass(Row number) to see if input is triggered
          if(trigState[z] == 0){                      //trigger has stopped active
            if (outputOnOffToggleArray[z] == 1) {     //if on, turn back off
              outputState[outputArray[z]-1] = 0;
              outputSelectFunction(z, 0);
            }else if(outputOnOffToggleArray[z] == 0){ //if off, turn back on
              outputState[outputArray[z]-1] = 1;
              outputSelectFunction(z, 1);
            }else if(outputOnOffToggleArray[z] == 2){ //if toggle
              outputState[outputArray[z]-1] = !outputState[outputArray[z]-1];       //toggle: flip the bit
              outputSelectFunction(z, outputState[outputArray[z]-1]);
            }
            
            stateRow[z] = 6;                          //move to next state
          }
        }else if (durationTypeArray[z] == 2) {  //"for...seconds"
          nowTime = millis();
          netTime = nowTime - timeStampDurationRow[z];
          if(netTime >= durationArray[z]) {
            if (outputOnOffToggleArray[z] == 1) {       //if on, turn back off
              outputState[outputArray[z]-1] = 0;
              outputSelectFunction(z, 0);
            }else if(outputOnOffToggleArray[z] == 0){   //if off, turn back on
              outputState[outputArray[z]-1] = 1;
              outputSelectFunction(z, 1);
            }else if(outputOnOffToggleArray[z] == 2){   //if toggle
              outputState[outputArray[z]-1] = !outputState[outputArray[z]-1];         //change the current state
              outputSelectFunction(z, outputState[outputArray[z]-1]);
            }

            stateRow[z] = 6;    //move to next state (retrigger delay)
          }
        }

      }else if(stateRow[z] == 6) {             //STATE 6 = retrigger delay holding state (kind of like a lobby)
        #ifdef DEBUG_STATES
          printState(z);
        #endif

        tempRetriggerDelay = inputRetriggerDelayArray[(inputArray[z] - 1)];
        nowTimeRetrigger = millis();
        netTimeRetrigger = nowTimeRetrigger - delayTimeStamp[z];        // subtract now - original trigger time
        if(netTimeRetrigger >= tempRetriggerDelay){                 // if you've met the retrigger time
          stateRow[z] = 1;                    // Go back to the beginning!
        }
      }else {                                 //STATE = ??? if state is not 1-6, set to 1 (waiting)
        stateRow[z] = 1;
      }
    }
  }
    
  //----- Section AA5 ----- Update the input status LEDs on shield -----
  #ifdef DEBUG_INPUTS
    char tempInputLetterArray[] = "ABCDEF";   //letter prefix for serial debugging
  #endif
  for(int j=0;j<=5;j++){                    //loop through each input
    if(decipherInputSensor(j+1) == 1){      //if input is (on). Shift +1 to account for inputArray[0] = no input
      digitalWrite(inputLEDArray[j],HIGH);  //turn on LED

      #ifdef DEBUG_INPUTS
        Serial << F("DEBUG_INPUTS: input ") << tempInputLetterArray[j] << F(": actual/threshold: ") << analogRead(inputPinArray[j]) << F("/") << inputTriggerThresholdArray[j];
        Serial.println();
      #endif
    }else{                                  //else (off)
      digitalWrite(inputLEDArray[j],LOW);   //turn off LED
    }
  }
    
  
  //----- Section AA6 ----- Update the Status variables for GUI to read
  
  
  
  
  
  
  

  
  //This tiny section runs the entire web server. Must be in void loop()
  //Please note the main line "web.process()" is the actual functioning block.
  //It should normally be wrapped in a statement protecting it from running
  //without an SD card, but in this case we want it to run even if there is no SD card
  //so we can serve up a custom html page from index_handler
  // LEDFlasher(1,200,200);    // call the LEDFlasher function to visually alert user of SD issue
  if (!networkServicesDisabled) {     // if not in networkServicesDisabled
    web.process();          // run web process
    EthernetBonjour.run();  // run zeroconf/bonjour
  }
}// end void loop


void loadProgramAndSettings() {
  #ifdef DEBUG_BRIDGE
    Serial << prefixDEBUG_BRIDGE << F("New info from gui received\n");
  #endif
  //----- Section AB1 -----  READ program.txt and settings.txt
  char* newvar = open_file("program.txt");  //store the program.txt in a var
  #ifdef DEBUG_BRIDGE
    Serial << prefixDEBUG_BRIDGE << F("program.txt=") << newvar << F("\n");                 //print the file out
  #endif

  if (newvar != 0 && newvar != ""){
    convert(newvar,1);                        //convert the file to arrays
    #ifdef DEBUG_BRIDGE
      Serial << prefixDEBUG_BRIDGE << F("program.txt converted\n");
    #endif
  } else {  // if we've gotten here we need to remove, then create & populate a new settings.txt file
    createDefaultFile("program.txt");
  }

  newvar = 0;                               //erase the newvar
  newvar = open_file("settings.txt");       //store the settings.txt in a var

  #ifdef DEBUG_BRIDGE
    Serial << prefixDEBUG_BRIDGE << F("settings.txt=") << newvar << F("\n");
  #endif
  if (newvar != 0 && newvar != ""){
    convert(newvar,0);                        //convert the file to arrays
    #ifdef DEBUG_BRIDGE
      Serial << prefixDEBUG_BRIDGE << F("settings.txt converted\n");
    #endif
  } else {  // if we've gotten here we need to remove, then create & populate a new settings.txt file
    createDefaultFile("settings.txt");
  }

  statusMessage(5);  //GUI message #5 Sends GUI confirmation message
} // end void loadProgramAndSettings()



//----- Section open SD file for conversion to arrays -----
char* open_file(char* input_file){
  #ifdef DEBUG_FILES
    Serial << prefixDEBUG_FILES << F("open_file(") << input_file << F(")\n");
  #endif
  char storage[MAX_FILE_LENGTH] = {0};                //used to store read stuff
  char ch;                                            //used to store incoming byte
  int i = 0;                                          //used as counter for building string
  char* fail = "";                                    //the failure return
  unsigned int tempMaxFileLength = MAX_FILE_LENGTH;   //largest file to use (to protect limited memory)
  byte tempMinFileLength = 10;                        //smallest file to consider valid (used in open_file() testing file.available())

  
  int tempSDStatus = SD.begin(SD_CS);
  File file = SD.open(input_file);

  #ifdef DEBUG_FILES
    Serial << prefixDEBUG_FILES << F("SD.begin=") << tempSDStatus << F(", has_filesystem=") << has_filesystem << F(" input_file=") << input_file << F(" file read=") << file << F("\n");
  #endif
  
  if (file) {                           //if there's a file
    #ifdef DEBUG_FILES
      Serial << prefixDEBUG_FILES << F("File.available()=") << file.available() << F("\n");
    #endif

    // ------ input sanitization section -------
    // This important section validates the user-changeable files on the SD card
    // such as bonjour.txt, ip.txt, etc. Based on which file is opened, min/max
    // lengths are set to prevent bad data from getting in and overflowing the cpu.
    // If the input fails, defaults are used.
    if (input_file == "bonjour.txt") {
      tempMinFileLength = MIN_BONJOUR_NAME_LENGTH;
      tempMaxFileLength = MAX_BONJOUR_NAME_LENGTH;
    }else if (input_file == "ip.txt") {
      tempMinFileLength = 7;                  //min possibl IP n.n.n.n
      tempMaxFileLength = 15;                 //max possible IP nnn.nnn.nnn.nnn
    }else if (input_file == "uniqueID.txt"){
      tempMinFileLength = 11;                 //min possible MAC n:n:n:n:n:n
      tempMaxFileLength = 23;                 //max possible MAC nnn:nnn:nnn:nnn:nnn:nnn
    }else{
      // else if (input_file == "program.txt"){
      //   tempMinFileLength = ;
      //   tempMaxFileLength = MAX_FILE_LENGTH;
      // }
      //else if (unt)
      //use defaults of min & max specified at top of function
    }

    if (file.available() < tempMinFileLength){
      Serial << F("File.avail() < ") << tempMinFileLength << F("\n");
      return fail;
    }else if (file.available() > tempMaxFileLength){    //tests to see if the file has more than the minimum chars to be considered legitimate
      Serial << F("File.avail() > ") << tempMaxFileLength << F("\n");
      return fail;            //exit the function and return the fail string ("")
    }

    while (file.available()) {          //if there are unread bytes in the file
      ch = file.read();                 //read one
      storage[i] = ch;                  //append it to storage
      #ifdef DEBUG_FILES_BY_CHARACTER
        Serial << F("DEBUG_FILES_BY_CHARACTER: ") << i << F(" ") << ch << F("\n");
      #endif

      i ++;                             //inc counter
    }
    file.close();                      //close the file
    return storage;                    //return the read bytes
  }else{                              //no file
    statusMessage(3);
    // Serial << F("No file: ") << input_file;
    // Serial.println("");
    return fail;                      //return w/ fail
  }
}//end open_file


//----- Section Convert cupcake string from SD/Web to arrays -----
char convert(char* readString, bool type){
  //converts readString (data) into program or settings arrays
  //type: 0 = settings, 1 = program

  char* col[8];
  char* tok;
  byte i = 0;
 
  // We're using a character array instead of the built-in String class because
  // that's what the strtok function needs.  Plus, using Strings in this case
  // wouldn't provide us a lot of benefit anyway.
  // You need to decide how many bytes your server could possibly send.  This
  // is a nice round number as a placeholder.  Also note that this is the
  // maximum size that the variable "byte i" will index.  If you need a bigger
  // array, make i an int.  
  
  // Split readString into columns with comma-separated values.  Columns
  // are separated by semicolons.  The first time you call strtok, you
  // pass the string (char array) you want to work on.  It returns a
  // a string with everything up to the delimiter you set.  For
  // subsequent calls, you pass NULL, and the function keeps working on
  // the original string.  Since we've set up the client to send 6
  // columns, we call the function 6 times.  The check for tok == NULL is
  // there in case there's a problem and we don't get the full packet.
  // This may help it fail more gracefully.
  tok = strtok(readString, ";");
  col[0] = tok;
  for (i = 1; i < 8; i++) {
    if (tok == NULL)
      break;
    tok = strtok(NULL, ";");
    col[i] = tok;
  }

  if (type == 0) {  //convert settings arrays here
    Serial << F("converting settings.txt\n");
    //inputActiveHiLow + outputActiveHiLow ; names ; names ; inputThreshold + outputVoltage (if we care) ; retriggerTime
    //0,1,1,1,1,1,0,1,1,1,1,1;garage,my room,hall,cemetery,cornfield,swamp;UV,light,strobe,sound,air horn,zombie;492,246,103,103,103,103,103,103,103,103,103,103;1,1,1,1,1,0; 
    //col[0] = inputActiveHiLow + outputActiveHiLow (need both)
    //col[1] = input names (don't care)
    //col[2] = output names  (don't care)
    //col[3] = inputThreshold (need) + outputVoltage (if we care)
    //col[4] = retriggerTime (need)

    // Now turn the array of strings into arrays of numbers.  Each array is
    // named for the column it represents.  The values are separated by
    // commas.  atoi is used to convert the stringified numbers back into
    // integers.  The values returned by atoi, which are ints, are cast
    // into the appropriate data type.  (That's what the (byte) before
    // atoi(tok) is doing.)  It would be more graceful to create a function
    // to do this, rather than repeat it 6 times.

    // inputActiveHiLowArray & outputActiveHiLowArray
    tok = strtok(col[0], ",");
    for (i = 0; i < 12; i++) {
      if (tok == NULL)
        break;
      if (i < 6){                     //since this is in a string of 12, do first 6 as input last 6 as output
        inputActiveHiLowArray[i] = (bool)atoi(tok);
      }else{
        outputActiveHiLowArray[i-6] = (bool)atoi(tok);
      }
      tok = strtok(NULL, ",");
    }
    // inputTriggerThresholdArray
    tok = strtok(col[3], ",");
    for (i = 0; i < 6; i++) {
      if (tok == NULL)
        break;
      inputTriggerThresholdArray[i] = (int)atoi(tok);
      tok = strtok(NULL, ",");
    }
    // inputRetriggerDelayArray
    tok = strtok(col[4], ",");
    for (i = 0; i < 6; i++) {
      if (tok == NULL)
        break;
      inputRetriggerDelayArray[i] = strtoul(tok, NULL, 0);      //string to unsigned long
      tok = strtok(NULL, ",");
    }
    
    #ifdef DEBUG_BRIDGE
      // Now print out all the values to make sure it all worked
      Serial.print("inputActiveHiLowArray: ");
      for (i = 0; i < 6; i++){
        Serial.print(inputActiveHiLowArray[i]);
        Serial.print(" ");
      }
      
      Serial.print("\noutputActiveHiLowArray: ");
      for (i = 0; i < 6; i++){
        Serial.print(outputActiveHiLowArray[i]);
        Serial.print(" ");
      }
      
      Serial.print("\ninputTriggerThresholdArray: ");
      for (i = 0; i < 6; i++){
        Serial.print(inputTriggerThresholdArray[i]);
        Serial.print(" ");
      }
      
      Serial.print("\ninputRetriggerDelayArray: ");
      for (i = 0; i < 6; i++){
        Serial.print(inputRetriggerDelayArray[i]);
        Serial.print(" ");
      }
      Serial.println("");
    #endif

  }else if (type == 1){ //convert program arrays here
    Serial.println("converting program.txt");
    #ifdef DEBUG_BRIDGE
      Serial << prefixDEBUG_BRIDGE << F("Free RAM: ") << FreeRam() << "\n";
    #endif
    // Now turn the array of strings into arrays of numbers.  Each array is
    // named for the column it represents.  The values are separated by
    // commas.  atoi is used to convert the stringified numbers back into
    // integers.  The values returned by atoi, which are ints, are cast
    // into the appropriate data type.  (That's what the (byte) before
    // atoi(tok) is doing.)  It would be more graceful to create a function
    // to do this, rather than repeat it 6 times.

    newCurrentRowCount = 0; //reset to zero, then we'll count them on the first array
    // inputArray
    tok = strtok(col[0], ",");
    for (i = 0; i < MAXROWS; i++) {
      if (tok == NULL)
        break;
      enableDisableArray[i] = (bool)atoi(tok);
    
      newCurrentRowCount = i + 1;

      #ifdef DEBUG_BRIDGE
        Serial << prefixDEBUG_BRIDGE << F("i: ") << i << F(" newCurrentRowCount: ") << newCurrentRowCount << F("\n");
      #endif
      
      tok = strtok(NULL, ",");
    }

    currentRowCount = newCurrentRowCount;
    #ifdef DEBUG_BRIDGE
      Serial << prefixDEBUG_BRIDGE << F("currentRowCount: ") << currentRowCount << F(" newCurrentRowCount: ") << newCurrentRowCount <<  F("\n");
    #endif

    tok = strtok(col[1], ",");
    for (i = 0; i < currentRowCount; i++) {
      if (tok == NULL)
        break;
      inputArray[i] = (byte)atoi(tok);
      tok = strtok(NULL, ",");
    }
    // inputOnOffArray
    tok = strtok(col[2], ",");
    for (i = 0; i < currentRowCount; i++) {
      if (tok == NULL)
        break;
      inputOnOffArray[i] = (bool)atoi(tok);
      tok = strtok(NULL, ",");
    }
    // delayArray
    tok = strtok(col[3], ",");
    for (i = 0; i < currentRowCount; i++) {
      if (tok == NULL)
        break;
      delayArray[i] = strtoul(tok, NULL, 0);      //string to unsigned long
      tok = strtok(NULL, ",");
    }
    // outputArray
    tok = strtok(col[4], ",");
    for (i = 0; i < currentRowCount; i++) {
      if (tok == NULL)
        break;
      outputArray[i] = (byte)atoi(tok);
      tok = strtok(NULL, ",");
    }
    // outputOnOffToggleArray
    tok = strtok(col[5], ",");
    for (i = 0; i < currentRowCount; i++) {
      if (tok == NULL)
        break;
      outputOnOffToggleArray[i] = (byte)atoi(tok);
      tok = strtok(NULL, ",");
    }
    // durationTypeArray
    tok = strtok(col[6], ",");
    for (i = 0; i < currentRowCount; i++) {
      if (tok == NULL)
        break;
      durationTypeArray[i] = (byte)atoi(tok);
      tok = strtok(NULL, ",");
    }
    // durationArray
    tok = strtok(col[7], ",");
    for (i = 0; i < currentRowCount; i++) {
      if (tok == NULL)
        break;
      durationArray[i] = strtoul(tok, NULL, 0);      //string to unsigned long
      tok = strtok(NULL, ",");
    }
    
    // Now print out all the values to make sure it all worked
    #ifdef DEBUG_BRIDGE
      Serial.print("enableDisableArray: ");
      for (i = 0; i < currentRowCount; i++){
        Serial.print(enableDisableArray[i]);
        Serial.print(" ");
      }
      Serial.print("\ninputArray: ");
      for (i = 0; i < currentRowCount; i++){
        Serial.print(inputArray[i]);
        Serial.print(" ");
      }
      Serial.print("\ninputOnOffArray: ");
      for (i = 0; i < currentRowCount; i++){
        Serial.print(inputOnOffArray[i]);
        Serial.print(" ");
      }
      Serial.print("\ndelayArray: ");
      for (i = 0; i < currentRowCount; i++){
        Serial.print(delayArray[i]);
        Serial.print(" ");
      }
      Serial.print("\noutputArray: ");
      for (i = 0; i < currentRowCount; i++){
        Serial.print(outputArray[i]);
        Serial.print(" ");
      }
      Serial.print("\noutputOnOffToggleArray: ");
      for (i = 0; i < currentRowCount; i++){
        Serial.print(outputOnOffToggleArray[i]);
        Serial.print(" ");
      }
      Serial.print("\ndurationTypeArray: ");
      for (i = 0; i < currentRowCount; i++){
        Serial.print(durationTypeArray[i]);
        Serial.print(" ");
      }
      Serial.print("\ndurationArray: ");
      for (i = 0; i < currentRowCount; i++){
        Serial.print(durationArray[i]);
        Serial.print(" ");
      }
      Serial.println();
      Serial << F("Free RAM: ") << FreeRam() << "\n";
    #endif

    Serial.println("Convert: done converting program");
  }

}//end convert cupcake string to arrays function

void LEDFlasher (int flashes, int timeOn, int timeOff){    //used to flash all input/output LEDs as a warning something is awry
  for (int i=0;i<flashes;i++){
    //turn all on
    for (int j=0;j<=5;j++){
      digitalWrite(inputLEDArray[j],HIGH);
      digitalWrite(outputLEDArray[j],HIGH);
    }
    delay(timeOn);
    //turn all off
    for (int j=0;j<=5;j++){
      digitalWrite(inputLEDArray[j],LOW);
      digitalWrite(outputLEDArray[j],LOW);
    }
    delay(timeOff);
  }//flashing loop
}//end LEDFlasher function


void directionalLEDFlasher (int tempOnOff, int cycles, int timeOn, int timeOff){    //used to flash all input/output LEDs as a warning something is awry
  for (int i=0;i<cycles;i++){
    if (tempOnOff == 1){                    //if on
      for (int j=0;j<=5;j++){
        digitalWrite(inputLEDArray[j],HIGH);
        digitalWrite(outputLEDArray[j],HIGH);
        if (j<5) delay(timeOn);
      }
    }else{
      for (int j=0;j<=5;j++){                 //if off
        digitalWrite(inputLEDArray[j],LOW);
        digitalWrite(outputLEDArray[j],LOW);
        if (j<5) delay(timeOff);
      }
    }
  }// end one cycle
}//end directionalLEDFlasher function


void printState(int row){
  Serial << prefixDEBUG_STATES << F("Row=") << row << F(" State=") << stateRow[row] << "\n";
}

void disableNetworkServices(){    //disables network services (bonjour, web.process, etc)
  networkServicesDisabled = true;
  statusMessage(10);
}

void createDefaultFile(char* tempFileName){ //creates a new program/settings.txt file based on defaults
  byte tempStatus = 0;
  File myFile;
  bool tempConvertType;
  SD.remove(tempFileName);            // just in case, delete the file to prevent bad data
  myFile = SD.open(tempFileName, FILE_WRITE);   //create new

  if (myFile) {                   // if the file opened okay, write to it:
    Serial << F("Creating new ") << tempFileName << F(" file.\n");
    if (tempFileName == "settings.txt") {
      myFile << default_settings;
      tempConvertType = 0;
      tempStatus = 7;
    }
    if (tempFileName == "program.txt") {
      myFile << default_program;
      tempConvertType = 1;
      tempStatus = 8;
    }
    myFile.close();             // close the file:
  } else {         // if the file didn't open, print an error:
    statusMessage(9);
  }

  convert(open_file(tempFileName),tempConvertType);
  statusMessage(tempStatus);
}
