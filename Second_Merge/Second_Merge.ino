//Second attempt at basic building block of HauntBox control
//Trying to write code for a row in the GUI, not for a specific input.
//Code written for 6 row control from HB
//12-01-20 (YY-MM-DD)


#include <SPI.h>
#include <Ethernet.h>
#include <Flash.h>
#include <SD.h>
#include <TinyWebServer.h>
#include <EthernetBonjour.h>

//Serial debugging options. Uncomment a row to enable each section as needed.
//#define DEBUG_STATES true           //prints states to serial
#define DEBUG_FILES true            //prints file conversion details to serial
#define DEBUG_PUT_HANDLER true      //prints file upload details to serial
//#define DEBUG_OUTPUTS true          //prints outputSelect details to serial
#define DEBUG_TRIGGERS true           //prints trigger details to serial
#define DEBUG_BRIDGE true             //prints bridge details to serial
//#define DEBUG_MANUAL true           //prints manual mode details to serial

#define MAXROWS 20
#define MINROWS 6

char* bonjourName = "hauntbox";                  //bonjour name
char* bonjourServiceRecord = "hauntbox._http";  //bonjour name (needs to have the "._http" at the end)

int d = 0;  //Delay used in testing of code.  Set to 0 if not testing code.

//--------------------Define/get variables from GUI-------------------------------------
int guiFlag = 0;  //GUI Flag tells us when there is a new program.txt/settings.txt from the GUI
int currentRowCount = 6;  //current number of rows (starts at 6 and modified by gui)
int newCurrentRowCount = 0;
bool automaticMode = true;  //keeps track of auto/manual override mode


//"Program" arrays
bool enableDisableArray[MAXROWS] = {1,1,1,1,1,1};       //if a row is enabled or disabled
byte inputArray[MAXROWS] =       {1, 2, 3, 4, 5, 6};    //which input (0-6) is selected (0 = none, 1 = input #1, 2 = input #2, ...)
byte inputOnOffArray[MAXROWS] =  {1, 1, 1, 1, 1, 1};    //when input is on/off
unsigned long delayArray[MAXROWS] = {0, 0, 0, 0, 0, 0}; //actual delay in milliseconds
byte outputArray[MAXROWS] =      {1, 2, 3, 4, 5, 6};    //which outputs (0-6) is selected (0 = none, 1 = output #1, 2 = output #2, ...)
byte outputOnOffToggleArray[MAXROWS] = {1,1,1,1,1,1};   //What the output should do (on/off/toggle)
byte durationTypeArray[MAXROWS] = {0,1,2,0,1,2};        //The type of duration (0 = until further notice, 1 = while input active, 2 = for ...)
unsigned long durationArray[MAXROWS] = {1000, 6000, 6000, 6000, 6000, 6000};  //actual effect duration in milliseconds

//"Settings" arrays
byte inputActiveHiLowArray[] =  {1, 1, 1, 1, 1, 0};         //What signal level is considered "on" for each input (1 = High, 0 = Low)
byte outputActiveHiLowArray[] = {1, 1, 1, 1, 1, 1};         //Output considered on when High (1) or Low (0)
unsigned int inputTriggerThresholdArray[] = {100,100,100,100,100,100};  //input trigger thresholds
unsigned long inputRetriggerDelayArray[] = {0,0,0,0,0,0};               //retrigger time in milliseconds


// Other arrays
//---------intput & output indicator LEDs near screw terminals
byte inputLEDArray [] =  {39,32,33,34,35,36};   //Array of arduino pins that correspond to the LEDs that indicate an input is triggered
byte outputLEDArray [] = {47,46,45,44,43,42};   //Array of arduino pins that correspond to the LEDs that indicate an output is on


//----------------------Define variables in code-----------------------------
bool outputState[6] = {0,0,0,0,0,0};   //array to hold on/off (1/0) state of every given output. Manipulated by any/multiple rules
                //***only 6 outputs!!!
int stateRow[MAXROWS];              //array that defines each row's state. Gets initialized in initializeFunction called from main function
int trigState[6];                      //gets initialized immediately before it is used
unsigned long delayTimeStamp[MAXROWS];    //gets initialized immediately before it is used
unsigned long timeStampDurationRow[MAXROWS]; //gets initialized immediately before it is used
unsigned long nowTime;
unsigned long netTime;                       //used to measure difference between now and delay time
//unsigned long retriggerNetTime;              //used to measure difference between now and retrigger time
int statesInitialized = 0;      //keeps track if the states have been initialized

//Define I/O pins
int pinIn1 = 10;  //Analog pin
int pinIn2 = 11;  //Analog pin
int pinIn3 = 12;  //Analog pin
int pinIn4 = 13;  //Analog pin
int pinIn5 = 14;  //Analog pin
int pinIn6 = 15;  //Analog pin

int pinOut1 = 49; //Digital pin
int pinOut2 = 48; //Digital pin
int pinOut3 = 38; //Digital pin
int pinOut4 = 41; //Digital pin
int pinOut5 = 40; //Digital pin
int pinOut6 = 37; //Digital pin
// Pin0 unused
// Pin1 unused
// Pin9 unused

//Reserved digital pins for Arduino Ethernet Module SPI
// Pin 10 (SS)  This one is being called out below in the SD card setup
// Pin 11 (MOSI)
// Pin 12 (MISO)
// Pin 13 (SCK)


const int SD_CS = 4;      // pin 4 is the SPI select pin for the SDcard
const int ETHER_CS = 10;  // pin 10 is the SPI select pin for the Ethernet
byte ip[] = { 192, 168, 0, 100 }; // Static fallback IP
static uint8_t mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEE };
char* html_browser_header = "HTTP/1.0 200 OK\nContent-Type: text/html\n";  //2 line header including mandatory blank line to signify data below

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
  {"/row_status", TinyWebServer::GET, &row_status_handler },
  {"/status", TinyWebServer::GET, &status_handler },
  {"/output_status", TinyWebServer::GET, &output_status_handler },
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
    web_server.send_error_code(200);
    web_server.send_content_type(mime_type);
    web_server.end_headers();
    if (file.open(&root, filename, O_READ)) {
      Serial << F("Read file "); Serial.println(filename);
      web_server.send_file(file);
      file.close();
    } else {
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
    client.println(html_browser_header);
    client.println("<html><body><h1>SD Card Failed</h1><ol><li>Verify your SD card works</li><li>Remove and reseat it in SD slot in hauntbox</li><li>Reset hauntbox</li><li>Reload this page in 30 seconds</li></ol></body></html>");
  }
  return true;
}


// -------------------- send program + status handler -------------------- 
boolean program_handler(TinyWebServer& web_server){
  send_file_name(web_server, "program.txt");
  Client& client = web_server.get_client();
  client.print("0;");    //send status code = 0
  client.print(FreeRam());
  client.print(";");
  client.print("\n");
  client.stop();
  return true; //exit the handler 
}

// -------------------- send settings + status handler -------------------- 
boolean settings_handler(TinyWebServer& web_server){
  send_file_name(web_server, "settings.txt");
  Client& client = web_server.get_client();
  client.print("0;");    //send status code = 0
  client.print(FreeRam());
  client.print(";");
  client.print("\n");
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
  return true;
}


// -------------------- manual handler -------------------- 
boolean manual_handler(TinyWebServer& web_server) {
  web_server.send_error_code(200);
  //web_server.send_content_type("text/plain");
  //web_server.end_headers();

  Client& client = web_server.get_client();
  if (client.available()) {
    bool tempOnOff = 0;             //keeps track of if we want it on or off
    int tempOutput = 0;             //keeps track of which input
    
    char ch = (char)client.read();  //throw away the first two characters in "a=11"
    ch = (char)client.read();       //throw away the first two characters in "a=11"
    ch = (char)client.read();       //now get the first integer 1
    char ch2 = (char)client.read(); //now get the second integer 1
    
    if (ch2 == '1'){                //this could be cleaned probably by using atoi
      tempOnOff = 1;
    }else{
      tempOnOff = 0;
    }
    
    if (ch == '1'){                 //this could be cleaned probably by using atoi
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
    }
    
    #ifdef DEBUG_MANUAL
      Serial << F("****** Manual Control raw: ") << ch << " " << ch2;
      Serial << F(" Converted: ") << tempOutput << " " << tempOnOff << F("\n");
    #endif

    outputState[tempOutput-1] = tempOnOff;          //set map, remembering to shift by minus 1
    outputSelectFunction(tempOutput-1, tempOnOff);  //set reality, remembering to shift by minus 1
  }
  return true;
}

// -------------------- trigger handler -------------------- 
boolean trigger_handler(TinyWebServer& web_server) {
  web_server.send_error_code(200);
  //web_server.send_content_type("text/plain");
  //web_server.end_headers();

  Client& client = web_server.get_client();
  if (client.available()) {
    int tempInput = 0;             //keeps track of which input
    
    char ch = (char)client.read();  //throw away the first two characters in "a=11"
    ch = (char)client.read();       //throw away the first two characters in "a=11"
    ch = (char)client.read();       //now get the first integer 1
    
    if (ch == '1'){                 //this could be cleaned probably by using atoi
      tempInput = 1;
    }else if (ch == '2'){
      tempInput = 2;
    }else if (ch == '3'){
      tempInput = 3;
    }else if (ch == '4'){
      tempInput = 4;
    }else if (ch == '5'){
      tempInput = 5;
    }else if (ch == '6'){
      tempInput = 6;
    }
    
    #ifdef DEBUG_MANUAL
      Serial << F("****** Manual trigger raw: ") << ch;
      Serial << F(" Converted: ") << tempInput << F("\n");
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
boolean trigger_all_handler(TinyWebServer& web_server) {   //turns all outputs on
  web_server.send_error_code(200);
  //web_server.send_content_type("text/plain");
  //web_server.end_headers();
  
  #ifdef DEBUG_MANUAL
    Serial << F("****** Manual Trigger: ALL");
  #endif
  
  for (byte i=0;i<=currentRowCount;i++){
    //set all to state 2
    stateRow[i] = 2;
  }
  return true;
}


// -------------------- trigger_all handler -------------------- 
boolean automatic_on_handler(TinyWebServer& web_server) {   //turns all outputs on
  web_server.send_error_code(200);
  //web_server.send_content_type("text/plain");
  //web_server.end_headers();
  #ifdef DEBUG_MANUAL
    Serial << F("****** Automatic Mode ON");
  #endif

  automaticMode = true;
  return true;
}


// -------------------- trigger_all handler -------------------- 
boolean automatic_off_handler(TinyWebServer& web_server) {   //turns all outputs on
  web_server.send_error_code(200);
  //web_server.send_content_type("text/plain");
  //web_server.end_headers();
  #ifdef DEBUG_MANUAL
    Serial << F("****** Automatic Mode OFF");
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
  client.println((bool)automaticMode);
  return true;
}


// -------------------- all_off handler -------------------- 
boolean all_off_handler(TinyWebServer& web_server) {  //turns all outputs off
  web_server.send_error_code(200);
  //web_server.send_content_type("text/plain");
  //web_server.end_headers();
  
  #ifdef DEBUG_MANUAL
    Serial << F("****** Manual Control: All Off");
  #endif
  
  for (byte i=0;i<=5;i++){
    outputState[i] = 0;          //set map
    outputSelectFunction(i, 0);  //set reality
  }
  return true;
}


// -------------------- all_on handler -------------------- 
boolean all_on_handler(TinyWebServer& web_server) {   //turns all outputs on
  web_server.send_error_code(200);
  //web_server.send_content_type("text/plain");
  //web_server.end_headers();
  
  #ifdef DEBUG_MANUAL
    Serial << F("****** Manual Control: All On");
  #endif
  
  for (byte i=0;i<=5;i++){
    outputState[i] = 1;          //set map
    outputSelectFunction(i, 1);  //set reality
  }
  return true;
}


// -------------------- row status handler -------------------- 
boolean row_status_handler(TinyWebServer& web_server) {   //returns stateRow[]
  web_server.send_error_code(200);
  web_server.send_content_type("text/plain");
  web_server.end_headers();
  
  Client& client = web_server.get_client();
  for (int i = 0; i < currentRowCount; i++){
    client.print(stateRow[0], DEC);
    if(i < (currentRowCount - 1)){
      client.print(",");
    }
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


// -------------------- output status handler -------------------- 
boolean output_status_handler(TinyWebServer& web_server) {  //returns outputState[]
  web_server.send_error_code(200);
  web_server.send_content_type("text/plain");
  web_server.end_headers();
  Client& client = web_server.get_client();
  
  client.print("Current output states:\n");
  for (int i = 0; i < 6; i++){
    client.print(outputState[i], DEC);
    if (i < 5){
      client.print(",");
    }
  }
  return true;
}


// -------------------- file uploader -------------------- 
byte tempGuiFlag = 0;   //remembers guiFlag state in between loops (this is called 3 times: start, write, end)
                        //so we have to only set the real guiFlag after all 3 steps

void file_uploader_handler(TinyWebServer& web_server,
			   TinyWebPutHandler::PutAction action,
			   char* buffer, int size) {
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
        	Serial << F("Creating ") << fname << "\n";
      	#endif

        file.open(&root, fname, O_CREAT | O_WRITE | O_TRUNC);

        //IF PROGRAM.TXT OR SETTINGS.TXT WERE UPDATED, TRIGGER UPDATE ARRAYS
        if ((strcmp(fname,"PROGRAM.TXT") == 0) || (strcmp(fname,"SETTINGS.TXT") == 0)){    //strcmp compares the pointer to the string 0 means equal.
          tempGuiFlag = 1;
        }
        #ifdef DEBUG_PUT_HANDLER
          Serial.println("case START");
          Serial.print("tempGuiFlag = ");
          Serial.println(tempGuiFlag);
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
      Serial << F("Wrote ") << file.fileSize() << F(" bytes in ") << millis() - start_time << F(" millis (received ") << total_size << F(" bytes)\n");
    #endif

    file.close();
    if (tempGuiFlag == 1){
      guiFlag = 1;
      tempGuiFlag = 0;
      #ifdef DEBUG_PUT_HANDLER
        Serial.println("case END");
        Serial.print("tempGuiFlag = ");
        Serial.println(tempGuiFlag);
      #endif
    }
  }
}




// -------------------- begin firmware -------------------- 

void setup() {
  int i;
  
  Serial.begin(115200);
  Serial << F("Starting up Hauntbox. Free RAM: ") << FreeRam() << "\n";

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
  for (int i = 0;i <=5;i++){  //quickly loop through inputLEDs and outoutLEDS
    pinMode(inputLEDArray[i], OUTPUT);  //set inputLED as output
    pinMode(outputLEDArray[i], OUTPUT); //set outoutLED as output
  }

  // initialize the SD card.
  Serial << F("Setting up SD card...\n");
  // Pass over the speed and Chip select for the SD card
  if (!card.init(SPI_FULL_SPEED, SD_CS)) {
    Serial << F("card failed\n");
    has_filesystem = false;
  }
  // initialize a FAT volume.
  if (!volume.init(&card)) {
    Serial << F("vol.init failed!\n");
    has_filesystem = false;
  }
  if (!root.openRoot(&volume)) {
    Serial << F("openRoot failed");
    has_filesystem = false;
  }

  if (has_filesystem) {
   // Assign our function to `upload_handler_fn'.
   TinyWebPutHandler::put_handler_fn = file_uploader_handler;
  }

  if (has_filesystem) {
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

      for (i = 0; i < 4; i++) {
        Serial << (val[i]) << ' ';
      }
      for (i = 0; i < 4; i++) {
        ip[i] = (byte)atoi(val[i]);
      }
      
      Serial << F("Static IP: ") << ip_temp << F("\n");
      Ethernet.begin(mac,ip);                                 //setup with static address
    }else{      //if there is not a static ip specified... use DHCP
      Serial << F("Setting up the Ethernet card...\n");
      if (Ethernet.begin(mac) == 0) {                          // Initialize ethernet with DHCP
         Serial << F("DHCP failed\n");
      }
    }
  }else{
    Serial << F("Setting up the Ethernet card...\n");
    
    if (Ethernet.begin(mac) == 0) {                          // Initialize ethernet with DHCP
      Serial << F("DHCP failed\n");
    }

    Serial << F("****** Warning: SD not working\n");
    LEDFlasher(10,200,200);  //visually alert user that something is awry by flashing all LEDs
  }//end if has filesystem
  
  // Start the web server.
  Serial << F("Web server starting...\n");
  web.begin();
  
  // Start the bonjour/zeroconf service
  EthernetBonjour.begin(bonjourName);        //Set the advertised name
  EthernetBonjour.addServiceRecord(bonjourServiceRecord, 80, MDNSServiceTCP);   //Set the advertised port/service

  Serial << F("Ready to accept HTTP requests at ") << bonjourName << F(".local or at http://");
  //Serial.print("IP Address: ");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {  // print the value of each byte of the IP address:
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print("."); 
  }
  Serial.print("\n");

}


//----- AA1b -- Initialize Output Function -----
// Function that initializes the outputs states to "off"
void initializeFunction() {
  // load settings and program from SD card

  //if they aren't there, create an intelligent default
  //delay(d*3);

  for(int a = 0; a < 6; a++) {
    outputState[a] = 0;     //start with all pins off.
    outputSelectFunction(a, outputState[a]);  //set output according to outputState[] map
  }
  statesInitialized = 1;
  //delay(d*3);
}

//----- Section AA4b -----
// Function that reads sensor, interprets what the value means (active hi/low) and 
//returns int 1 for "triggered" or 0 for "not triggered"
int decipherInputSensor(int rowNumber) {
  int trig; //returns 1 if input is considered triggered, 0 if not
  int val;
  int x;
  int y;
  x = inputArray[rowNumber];  //Returns input pin # (0-6) 
  if(x == 0) {return 0;} //Do nothing for "N/A" case and break out of function
  if(x == 1) {val = analogRead(pinIn1);}
  if(x == 2) {val = analogRead(pinIn2);}
  if(x == 3) {val = analogRead(pinIn3);}
  if(x == 4) {val = analogRead(pinIn4);}
  if(x == 5) {val = analogRead(pinIn5);}
  if(x == 6) {val = analogRead(pinIn6);}

  y = inputActiveHiLowArray[x-1];  //Returns Acvive High/Low definition for corresponding input
      //x-1 because in the inputArray we are using 0 as "none"
      //wherease in inputActiveHiLowArray we are not

  if((y == 1) && (val >= inputTriggerThresholdArray[rowNumber])) {trig = 1;}    // if high and supposed to be, consider input "triggered"
  if((y == 0) && (val >= inputTriggerThresholdArray[rowNumber])) {trig = 0;}    // if low and supposed to be high, don't
  if((y == 1) && (val <  inputTriggerThresholdArray[rowNumber])) {trig = 0;}
  if((y == 0) && (val <  inputTriggerThresholdArray[rowNumber])) {trig = 1;}    // if low and supposed to be, consider input "triggered"
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
bool inputTakeAction(int rowNumber) {
  bool shouldItTakeAction;
  if ( decipherInputSensor(rowNumber) == inputOnOffArray[rowNumber] ){
    shouldItTakeAction = 1;
  }else{
    shouldItTakeAction = 0;
  }
  return shouldItTakeAction;
}

//  Function to write messages to gui            CURRENTLY WRITTEN FOR SERIAL, NOT GUI!
void statusMessage(int n) {
  if (n==1) {Serial << F("Definitions don't make sense\n");}
  if (n==2) {Serial << F("No SD Card anymore\n");}
  if (n==3) {Serial << F("Problem opening file on SD card\n");}
  if (n==4) {Serial << F("Corrupted SD file -Read/Write fail\n");}
  if (n==5) {Serial << F("Successfully updated programming\n");}
  if (n==6) {Serial << F("Finished writing to .txt file\n");}
}

//----- Section AA4c -----
// Function that pairs the output pins to the row that is controlling for it
void outputSelectFunction(int outputNumber, bool action) {
  
  #ifdef DEBUG_OUTPUTS
    Serial << F("outputSelectFunction ") << outputNumber << F(" ") << action << F("\n");
  #endif

  //takes an output (outputNumber) and an action:
    //0 = off
    //1 = on
  int x;      //local variable
  int y;
  x = outputArray[outputNumber]; //lookup which output is being controlled
  y = outputActiveHiLowArray[x-1];// lookup which value is considered "on"
  
  if(action == 1) {         // turn output on
    if(y == 1) {           // "on" means turn output high
      if(x == 0) {return;} //Do nothing for "N/A" case and break out of function
      if(x == 1) {digitalWrite(pinOut1, HIGH); digitalWrite(outputLEDArray[0], HIGH); }
      if(x == 2) {digitalWrite(pinOut2, HIGH); digitalWrite(outputLEDArray[1], HIGH); }
      if(x == 3) {digitalWrite(pinOut3, HIGH); digitalWrite(outputLEDArray[2], HIGH); }
      if(x == 4) {digitalWrite(pinOut4, HIGH); digitalWrite(outputLEDArray[3], HIGH); }
      if(x == 5) {digitalWrite(pinOut5, HIGH); digitalWrite(outputLEDArray[4], HIGH); }
      if(x == 6) {digitalWrite(pinOut6, HIGH); digitalWrite(outputLEDArray[5], HIGH); } }
    if(y == 0) {           // "on" means turn output low
      if(x == 0) {return;} //Do nothing for "N/A" case and break out of function
      if(x == 1) {digitalWrite(pinOut1, LOW); digitalWrite(outputLEDArray[0], LOW);}
      if(x == 2) {digitalWrite(pinOut2, LOW); digitalWrite(outputLEDArray[1], LOW);}
      if(x == 3) {digitalWrite(pinOut3, LOW); digitalWrite(outputLEDArray[2], LOW);}
      if(x == 4) {digitalWrite(pinOut4, LOW); digitalWrite(outputLEDArray[3], LOW);}
      if(x == 5) {digitalWrite(pinOut5, LOW); digitalWrite(outputLEDArray[4], LOW);}
      if(x == 6) {digitalWrite(pinOut6, LOW); digitalWrite(outputLEDArray[5], LOW);} }
    return; }
  if(action == 0) {         //turn output off
    if(y == 1) {           // "off" means turn output low
      if(x == 0) {return;} //Do nothing for "N/A" case and break out of function
      if(x == 1) {digitalWrite(pinOut1, LOW); digitalWrite(outputLEDArray[0], LOW);}
      if(x == 2) {digitalWrite(pinOut2, LOW); digitalWrite(outputLEDArray[1], LOW);}
      if(x == 3) {digitalWrite(pinOut3, LOW); digitalWrite(outputLEDArray[2], LOW);}
      if(x == 4) {digitalWrite(pinOut4, LOW); digitalWrite(outputLEDArray[3], LOW);}
      if(x == 5) {digitalWrite(pinOut5, LOW); digitalWrite(outputLEDArray[4], LOW);}
      if(x == 6) {digitalWrite(pinOut6, LOW); digitalWrite(outputLEDArray[5], LOW);} }
    if(y == 0) {           // "off" means turn output high
      if(x == 0) {return;} //Do nothing for "N/A" case and break out of function
      if(x == 1) {digitalWrite(pinOut1, HIGH); digitalWrite(outputLEDArray[0], HIGH); }
      if(x == 2) {digitalWrite(pinOut2, HIGH); digitalWrite(outputLEDArray[1], HIGH); }
      if(x == 3) {digitalWrite(pinOut3, HIGH); digitalWrite(outputLEDArray[2], HIGH); }
      if(x == 4) {digitalWrite(pinOut4, HIGH); digitalWrite(outputLEDArray[3], HIGH); }
      if(x == 5) {digitalWrite(pinOut5, HIGH); digitalWrite(outputLEDArray[4], HIGH); }
      if(x == 6) {digitalWrite(pinOut6, HIGH); digitalWrite(outputLEDArray[5], HIGH); } }
    return; }
  }

//-------------------- Main Function ------------------------
void loop(){
  
  //delay(d/2);  //For debug use only
  
  //----- Section AA1a -----
  if(statesInitialized == 0) {  //If the states have not been initialized, do so.
    initializeFunction();
    guiFlag = 1;                //force loading of settings and program
  }

  //----- Section AA9 ----- See if program or settings have changed
  if(guiFlag == 1) {  //If there are new program/settings ...
    #ifdef DEBUG_BRIDGE
      Serial << F("new info from gui received\n");
    #endif

    //----- Section AB1 -----
    //READ program.txt and settings.txt
    char* newvar = open_file("program.txt");  //store the program.txt in a var
    #ifdef DEBUG_BRIDGE
      Serial.println(newvar);                   //print the file out
    #endif
    convert(newvar,1);                        //convert the file to arrays
    #ifdef DEBUG_BRIDGE
      Serial << F("made it through program.txt conversion\n");
    #endif
    newvar = 0;                               //erase the newvar
    //Serial.println("reset newvar to 0");
    
    newvar = open_file("settings.txt");       //store the settings.txt in a var

    #ifdef DEBUG_BRIDGE
      Serial << F("opening settings.txt\n");
      Serial.println(newvar);                   //print the file out
      Serial << F("printed settings.txt\n");
    #endif
    convert(newvar,0);                        //convert the file to arrays
    #ifdef DEBUG_BRIDGE
      Serial << F("made it through conversion of settings.txt\n");
    #endif

    
    //----- Section AB2 -----
    //FUNCTION TO MAKE SURE GUI DEFINITIONS MAKE SENSE
    
    //----- Section AB3 -----
    //FUNCTION TO SAVE DEFINITIONS TO SD CARD
    //saved to disk with the put handler

    //CHECK TO SEE IF SD CARD IS THERE
    //already done with has_filesystem
    
    //----- Section AB4 -----
    //FUNCTION TO READ DEFINITIONS ON SD CARD
    
    //COMPARE TO VALUES FROM AB1
    
    //Sends GUI confirmation message
    statusMessage(5);  //GUI message #5
    
    guiFlag = 0;                                //reset guiFlag
  }

  
  //----- Section AA4a ----- Main State Machine: Reading Inputs and Writing to Outputs -----
  for(int z = 0; z < currentRowCount; z++) {              //runs loop for each row
    #ifdef DEBUG_STATES
      Serial.println(millis());
    #endif
    if(enableDisableArray[z] == 1) {   //only run the state machine for a row that's enabled
      if(stateRow[z] == 1) {                   //STATE 1 = Waiting for a trigger
        trigState[z] = inputTakeAction(z); //Call function and pass(Row number) to see if input is triggered
        if(trigState[z] == 1 && automaticMode == true) {   //If triggered AND in automaticMode
          stateRow[z] = 2;                     //Moves to next state
        }else{//if(trigState[z] == 0) {               //If not triggered (or not in automaticMode)
          //Do nothing
        }
      }else if(stateRow[z] == 2) {             //STATE 2 = Trigger message just received
        delayTimeStamp[z] = millis();      //Gets time stamp
        
        #ifdef DEBUG_TRIGGERS
          Serial << F("Trigger ") << z << F(": ") << delayTimeStamp[z] << F("\n");
        #endif
        stateRow[z] = 3;                      //Moves on to next state
      }else if(stateRow[z] == 3) {             //STATE 3 = Delay vs. timeStamp
        nowTime = millis();
        netTime = nowTime - delayTimeStamp[z];
        if(netTime >= delayArray[z]) {   //Tests to see if time > delay
          stateRow[z] = 4;                    //If we've met our delay, go to next state
        }
      }else if(stateRow[z] == 4) {             //STATE 4 = Change output (make it on/off/toggle)
        if (outputOnOffToggleArray[z] == 0){       //if it should be off
          outputState[z] = 0;
        }else if(outputOnOffToggleArray[z] == 1){  //if it should be on
          outputState[z] = 1;
        }else if(outputOnOffToggleArray[z] == 2){  //if it should toggle
          outputState[z] = !outputState[z]; //flip the bit
        }
        outputSelectFunction(z, outputState[z]);    //enact the change
        timeStampDurationRow[z] = millis();   //Get timestamp
        stateRow[z] = 5;                      //Moves on to next state
      }else if(stateRow[z] == 5) {             //STATE 5 = Duration of output "on"
        #ifdef DEBUG_STATES
          printState(z);
        #endif

        //switch for 3 different duration types
        if (durationTypeArray[z] == 0) {  //"until further notice"
          stateRow[z] = 6;  //move to next state (retrigger delay)
        }else if(durationTypeArray[z] == 1) {  //"while input triggered"
          if(trigState[z] == 0){  //trigger has stopped active
            if (outputOnOffToggleArray[z] == 1) //if on, turn back off
            {
              outputState[z] = 0;
              outputSelectFunction(z, 0);
            }
            else if(outputOnOffToggleArray[z] == 0){ //if off, turn back on
              outputState[z] = 1;
              outputSelectFunction(z, 1);
            }
            else if(outputOnOffToggleArray[z] == 2){  //if toggle
              outputState[z] = !outputState[z]; //change the current state
              outputSelectFunction(z, outputState[z]);
            }
            
            stateRow[z] = 6; //move to next state (retrigger delay)
          }
        }else if (durationTypeArray[z] == 2) {    //"for...seconds"
          nowTime = millis();
          netTime = nowTime - timeStampDurationRow[z];
          if(netTime >= durationArray[z]) {
            if (outputOnOffToggleArray[z] == 1) { //if on, turn back off
              outputState[z] = 0;
              outputSelectFunction(z, 0);
            }else if(outputOnOffToggleArray[z] == 0){ //if off, turn back on
              outputState[z] = 1;
              outputSelectFunction(z, 1);
            }else if(outputOnOffToggleArray[z] == 2){  //if toggle
              outputState[z] = !outputState[z]; //change the current state
              outputSelectFunction(z, outputState[z]);
            }

            stateRow[z] = 6;    //move to next state (retrigger delay)
            
          }
        }
      }else if(stateRow[z] == 6) {             //STATE 6 = retrigger delay holding state (kind of like a lobby)
        
        #ifdef DEBUG_STATES
          printState(z);
        #endif

        unsigned long net;
        unsigned long tempDelay = inputRetriggerDelayArray[(inputArray[z] - 1)];
        unsigned long now = millis();

        net = now - delayTimeStamp[z];


        if(net >= tempDelay){
          stateRow[z] = 1;
        }
      }else {                                  //STATE = ??? if state is not 1-6, set to 1 (waiting)
        stateRow[z] = 1;
        delay (d);
      }
    }
  }
    
  //----- Section AA5 ----- Update the input status LEDs on shield -----
  for(int z = 0; z < 5; z++) {              //runs loop for each input
    trigState[z] = decipherInputSensor(z); //Call function and pass(Row number) to see if input is on or off
      if(trigState[z] == 1) {                //If input is "on"
        digitalWrite(inputLEDArray[z],HIGH);  //turn on appropriate input LED
      }
      if(trigState[z] == 0) {               //If input is "off"
        digitalWrite(inputLEDArray[z],LOW);  //turn off appropriate input LED
      }
  }
  
  
  //----- Section AA6 ----- Update the Status variables for GUI to read
  
  
  
  
  
  
  

  
  //This tiny section runs the entire web server. Must be in void loop()
  //Please not the main line "web.process()" is the actual functioning block.
  //It should normally be wrapped in a statement protecting it from running
  //without an SD card, but in this case we want it to run even if there is no SD card
  //so we can serve up a custom html page from index_handler
  if (has_filesystem) {
  //  web.process();
  }else{
    LEDFlasher(1,200,200);    //call the LEDFlasher function to visually alert user of SD issue
  }
  web.process();
  
  EthernetBonjour.run();  //Runs zeroconf/bonjour. Must be in void loop()
}// end void loop


//----- Section open SD file for conversion to arrays -----
char* open_file(char* input_file){
  #ifdef DEBUG_FILES
    Serial.print("\nopen_file(");
    Serial.print(input_file);
    Serial.print(") called\n");
  #endif
  char storage[400] = {0};             //used to store read stuff
  char ch;                             //used to store incoming byte
  int i = 0;                           //used as counter for building string
  char* fail = "";                     //the failure return
  
  #ifdef DEBUG_FILES
    Serial << F("SD.begin = ") << SD.begin(SD_CS) << F(", has_filesystem = ") << has_filesystem << F("\n");
  #endif
  
  File file = SD.open(input_file);
  
  #ifdef DEBUG_FILES
    Serial << F("input_file = ") << input_file << F(", file read = ") << file << F("\n");
  #endif
  
  if (file) {                           //if there's a file

    #ifdef DEBUG_FILES
      Serial << F("*****did we make it? File.available() = ") << file.available() << F("\n");
      Serial << input_file << F("we made it!\n");
    #endif

    while (file.available()) {          //if there are unread bytes in the file
      ch = file.read();                 //read one
      storage[i] = ch;                  //append it to storage
      #ifdef DEBUG_FILES
        Serial << i << F(" ") << ch << F("\n");
      #endif

      i ++;                             //inc counter
    }
    file.close();                      //close the file
    return storage;                    //return the read bytes
  }else{                              //no file
    Serial.println("no file");          //error
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
        inputActiveHiLowArray[i] = (byte)atoi(tok);
      }else{
        outputActiveHiLowArray[i-6] = (byte)atoi(tok);
      }
      tok = strtok(NULL, ",");
    }
    // inputTriggerThresholdArray
    tok = strtok(col[3], ",");
    for (i = 0; i < 6; i++) {
      if (tok == NULL)
        break;
      inputTriggerThresholdArray[i] = (byte)atoi(tok);
      tok = strtok(NULL, ",");
    }
    // inputRetriggerDelayArray
    tok = strtok(col[4], ",");
    for (i = 0; i < 6; i++) {
      if (tok == NULL)
        break;
      inputRetriggerDelayArray[i] = (unsigned long)atoi(tok);
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
    #endif

  }else if (type == 1){ //convert program arrays here
    Serial.println("converting Program.txt");
    #ifdef DEBUG_BRIDGE
      Serial << F("Free RAM: ") << FreeRam() << "\n";
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
      enableDisableArray[i] = (byte)atoi(tok);
    
      newCurrentRowCount = i + 1;

      #ifdef DEBUG_BRIDGE
        Serial << F("i: ") << i << F(" new: ") << newCurrentRowCount << F("\n");
      #endif
      
      tok = strtok(NULL, ",");
    }

    currentRowCount = newCurrentRowCount;
    #ifdef DEBUG_BRIDGE
      Serial << F("Done w/ first loop. currentRowCount: ") << currentRowCount << F(" newCurrentRowCount: ") << newCurrentRowCount <<  F("\n");
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
      inputOnOffArray[i] = (byte)atoi(tok);
      tok = strtok(NULL, ",");
    }
    // delayArray
    tok = strtok(col[3], ",");
    for (i = 0; i < currentRowCount; i++) {
      if (tok == NULL)
        break;
      delayArray[i] = (unsigned int)atoi(tok);
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
      durationArray[i] = (unsigned int)atoi(tok);
      tok = strtok(NULL, ",");
    }
    
    // Now print out all the values to make sure it all worked
    #ifdef DEBUG_BRIDGE
      Serial.print("enableDisableArray: ");
      for (i = 0; i < currentRowCount; i++){
        Serial.print(enableDisableArray[i]);
        Serial.print(" ");
      }
      Serial.print("inputArray: ");
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
    #endif
    Serial.println("convert: done converting program");
    Serial << F("Free RAM: ") << FreeRam() << "\n";

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

void printState(int row){
  Serial << F("Row ") << row << F(" State: ") << stateRow[row] << "\n";
}

