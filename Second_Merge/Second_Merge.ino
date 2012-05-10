//Second attempt at basic building block of HauntBox control
//Trying to write code for a row in the GUI, not for a specific input.
//Code written for 6 row control from HB
//12-01-20 (YY-MM-DD)


#include <SPI.h>
#include <Ethernet.h>
#include <Flash.h>
#include <SD.h>
#include <TinyWebServer.h>

int d = 0;  //Delay used in testing of code.  Set to 0 if not testing code.

//--------------------Define/get variables from GUI-------------------------------------
int guiFlag = 0;  //GUI Flag tells us when there are new definitions from the GUI

//THESE VALUES ARE SET ARBITRARILY TO TEST CODE.  WILL NEED TO CHANGE IN FINAL VERSION!
byte inputArray[] =       {1, 6, 6, 6, 6, 6};    //which input (0-6) is read by which row ({row0, row1, ...})
                                                //0 = no input, 1 = input #1, 2 = input #2, etc

byte inputHiLowArray[] =  {1, 1, 1, 1, 1, 0};    //What signal level is considered "on" for input # ({input1, input2, ...})
                                                //1 = High, 0 = Low 
                                                
byte outputArray[] =      {1, 2, 3, 4, 5, 6};    //which outputs (0-6) are controlled by which row ({row0, row1, ...})
                                                //0 = no output, 1 = output #1, 2 = output #2, etc
                                                
byte outputHiLowArray[] = {1, 1, 1, 1, 1, 1};    //Output considered on when High (1) or Low (0)
unsigned int DelayRow[] = {0, 2000, 3000, 4000, 5000, 6000};     //Time in millis  
unsigned int DurationRow[] = {1000, 6000, 6000, 6000, 6000, 6000};  //Time in millis  //TURN INTO AN ARRAY FOR FINAL CODE


//----------------------Define variables in code-----------------------------
int rn = 6;  //number of rows
int stateRow[6];                       //array that defines each row's state. Gets initialized in initializeFunction called from main function
int trigState[6];                      //gets initialized immediately before it is used
unsigned long timeStampDelayRow[6];    //gets initialized immediately before it is used
unsigned long timeStampDurationRow[6]; //gets initialized immediately before it is used
unsigned long nowTime;
unsigned long netTime;
int ini = 0;      //Initialization variable
unsigned long tempStampA;
unsigned long tempStampB;

//Define I/O pins
int pinIn1 = 0;  //Analog pin
int pinIn2 = 1;  //Analog pin
int pinIn3 = 2;  //Analog pin
int pinIn4 = 3;  //Analog pin
int pinIn5 = 4;  //Analog pin
int pinIn6 = 5;  //Analog pin

//int CS_pin = 4;  //Pin to SD card  IS THIS THE SAME AS PIN 10, BUT FOR THE SHIELD?
int pinOut1 = 7; //Digital pin
int pinOut2 = 6; //Digital pin
int pinOut3 = 5; //Digital pin
int pinOut4 = 8; //Digital pin
int pinOut5 = 3; //Digital pin
int pinOut6 = 2; //Digital pin
// Pin0 unused
// Pin1 unused
// Pin9 unused

//Reserved digital pins for Arduino Ethernet Module SPI
// Pin 10 (SS)  This one is being called out below in the SD card setup
// Pin 11 (MOSI)
// Pin 12 (MISO)
// Pin 13 (SCK)


/****************VALUES YOU CHANGE*************/
// pin 4 is the SPI select pin for the SDcard
const int SD_CS = 4;

// pin 10 is the SPI select pin for the Ethernet
const int ETHER_CS = 10;

// Don't forget to modify the IP to an available one on your home network
byte ip[] = { 192, 168, 1, 9 };
//byte ip[] = {10,0,0,10 };

char StorageString[100];

/*********************************************/
static uint8_t mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
char* browser_header = "HTTP/1.0 200 OK\nContent-Type: text/html\n";  //2 line header including mandatory blank line to signify data below

//--------------------------- Define Handlers ----------------------------

boolean file_handler(TinyWebServer& web_server);
boolean index_handler(TinyWebServer& web_server);
boolean test_handler(TinyWebServer& web_server);
boolean program_handler(TinyWebServer& web_server);
boolean settings_handler(TinyWebServer& web_server);
boolean export_handler(TinyWebServer& web_server);

TinyWebServer::PathHandler handlers[] = {
  // Work around Arduino's IDE preprocessor bug in handling /* inside
  // strings.
  //
  // `put_handler' is defined in TinyWebServer
 // {"/", TinyWebServer::GET, &index_handler },
  {"/", TinyWebServer::GET, &index_handler },
  {"/ram", TinyWebServer::GET, &ram_handler },
  {"/status", TinyWebServer::GET, &status_handler },
  {"/program", TinyWebServer::GET, &program_handler },
  {"/testget", TinyWebServer::POST, &testget_handler },
  {"/settings", TinyWebServer::GET, &settings_handler },
  {"/export", TinyWebServer::GET, &export_handler },
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
    TinyWebServer::MimeType mime_type
      = TinyWebServer::get_mime_type_from_filename(filename);
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
  send_file_name(web_server, "gui.htm");
  return true;
}


// -------------------- testing the bridge handler -------------------- 

boolean testget_handler(TinyWebServer& web_server){
  
  char* col[6];
  char* tok;
  byte input_arr[6];
  byte in_onoff[6];
  unsigned int ondelay[6];
  byte out_arr[6];
  byte out_onoff[6];
  unsigned int duration[6];
  
  byte i = 0;
  
  // String readString = String(); //string for fetching data from address

  // We're using a character array instead of the built-in String class because
  // that's what the strtok function needs.  Plus, using Strings in this case
  // wouldn't provide us a lot of benefit anyway.
  // You need to decide how many bytes your server could possibly send.  This
  // is a nice round number as a placeholder.  Also note that this is the
  // maximum size that the variable "byte i" will index.  If you need a bigger
  // array, make i an int.  

  char readString[100];
  char ch;
  Client& client = web_server.get_client();
  Serial.print("Free RAM: ");
  Serial.println(FreeRam());

  while (client.connected()) {
    //Serial.print("1: con = ");
    //Serial.print(client.connected());
    //Serial.print(", avail = ");
    //Serial.println(client.available());
    
    if (client.available() > 0) {      //if there are incoming bytes
      //Serial.print("2: con = ");
      //Serial.print(client.connected());
      //Serial.print(", avail = ");
      //Serial.println(client.available());
      
      ch = client.read();
      
      //if (readString.length() < 100) {//need to find out what this limit does
      // Read a byte at a time and concatenate it onto the end of the character array.
      readString[i] = ch;
      StorageString[i] = ch;
      i++;
      //}
          
    }else if (client.available() == 0) {
      //Serial.print("3: con = ");
      //Serial.print(client.connected());
      //Serial.print(", avail = ");
      //Serial.println(client.available());
      
      Serial.println(readString);
      //StorageString = readString;
      	
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
      for (i = 1; i < 6; i++) {
        if (tok == NULL)
          break;
        tok = strtok(NULL, ";");
        col[i] = tok;
      }
      
      // Now turn the array of strings into arrays of numbers.  Each array is
      // named for the column it represents.  The values are separated by
      // commas.  atoi is used to convert the stringified numbers back into
      // integers.  The values returned by atoi, which are ints, are cast
      // into the appropriate data type.  (That's what the (byte) before
      // atoi(tok) is doing.)  It would be more graceful to create a function
      // to do this, rather than repeat it 6 times.
      // input_arr
      tok = strtok(col[0], ",");
      for (i = 0; i < 6; i++) {
        if (tok == NULL)
          break;
        input_arr[i] = (byte)atoi(tok);
        tok = strtok(NULL, ",");
      }
      // in_onoff
      tok = strtok(col[1], ",");
      for (i = 0; i < 6; i++) {
        if (tok == NULL)
          break;
        in_onoff[i] = (byte)atoi(tok);
        tok = strtok(NULL, ",");
      }
      // ondelay
      tok = strtok(col[2], ",");
      for (i = 0; i < 6; i++) {
        if (tok == NULL)
          break;
        ondelay[i] = (unsigned int)atoi(tok);
        tok = strtok(NULL, ",");
      }
      // out_arr
      tok = strtok(col[3], ",");
      for (i = 0; i < 6; i++) {
        if (tok == NULL)
          break;
        out_arr[i] = (byte)atoi(tok);
        tok = strtok(NULL, ",");
      }
      // out_onoff
      tok = strtok(col[4], ",");
      for (i = 0; i < 6; i++) {
        if (tok == NULL)
          break;
        out_onoff[i] = (byte)atoi(tok);
        tok = strtok(NULL, ",");
      }
      // duration
      tok = strtok(col[5], ",");
      for (i = 0; i < 6; i++) {
        if (tok == NULL)
          break;
        duration[i] = (unsigned int)atoi(tok);
        tok = strtok(NULL, ",");
      }
      
      // Now print out all the values to make sure it all worked
      Serial.print("Input array: ");
      for (i = 0; i < 6; i++)
        Serial.print(input_arr[i]); Serial.print(" ");
      
      Serial.print("\nInput on/off: ");
      for (i = 0; i < 6; i++)
        Serial.print(in_onoff[i]); Serial.print(" ");
      
      Serial.print("\nOn delay: ");
      for (i = 0; i < 6; i++)
        Serial.print(ondelay[i]); Serial.print(" ");
      
      Serial.print("\nOutput array: ");
      for (i = 0; i < 6; i++)
        Serial.print(out_arr[i]); Serial.print(" ");
      
      Serial.print("\nOutput on/off: ");
      for (i = 0; i < 6; i++)
        Serial.print(out_onoff[i]); Serial.print(" ");
      
      Serial.print("\nDuration: ");
      for (i = 0; i < 6; i++)
        Serial.print(duration[i]); Serial.print(" ");
      
      client.println(browser_header);
      //send some sample data back to the broswer
      client.print("Free RAM: ");
      client.print(FreeRam());
      client.print(", millis: ");
      client.println(millis());
      
      //Data sending protocol (Arduino --> Browser)
      //Input array;Input on/off;On delay;Output array;Output on/off;Duration;status code;Free RAM;
      //0,0,0,0,0,0;1,1,1,1,1,1;0,0,0,0,0,0;0,0,0,0,0,0;1,1,1,1,1,1;0,0,0,0,0,0;0;182;
      //status codes
      //0 = all good
      //1 = 
      //2 = 
      //3 = 
      //4 = 
      client.print("Input array: ");
      for (i = 0; i < 6; i++)
        client.print(input_arr[i]); client.print(" ");
        
      client.print("\nInput on/off: ");
      for (i = 0; i < 6; i++)
        client.print(in_onoff[i]); client.print(" ");
      
      client.print("\nOn delay: ");
      for (i = 0; i < 6; i++)
        client.print(ondelay[i]); client.print(" ");
      
      client.print("\nOutput array: ");
      for (i = 0; i < 6; i++)
        client.print(out_arr[i]); client.print(" ");
      
      client.print("\nOutput on/off: ");
      for (i = 0; i < 6; i++)
        client.print(out_onoff[i]); client.print(" ");
      
      client.print("\nDuration: ");
      for (i = 0; i < 6; i++)
        client.print(duration[i]); client.print(" ");
      
      client.print("\n");
      
      client.stop();

      //Serial.print("4: con = ");
      //Serial.print(client.connected());
      //Serial.print(", avail = ");
      //Serial.println(client.available());
      
    }//client.available()
    
    //Serial.print("5: con = ");
    //Serial.print(client.connected());
    //Serial.print(", avail = ");
    //Serial.println(client.available());
    
  }//while client.connected()
  
    //Serial.print("6: con = ");
    //Serial.print(client.connected());
    //Serial.print(", avail = ");
    //Serial.println(client.available());
              
  return true; //exit the handler 
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

// -------------------- export handler -------------------- 
boolean export_handler(TinyWebServer& web_server) {
  Client& client = web_server.get_client();
  client.println(browser_header);
  
  client.print("Date, Description\nSettings:0,1,0,0,1,0,0,0,1,0,0,0;input1,input2,input3,input4,input5,input6;output1,output2,output3,output4,output5,output6;103,246,492,103,246,492,103,246,492,103,246,492;1,2,3,4,5,6;\nProgram: 0,2,3,4,5,6;0,1,0,1,0,1;0,1,10,100,500,900;6,5,4,3,2,1;0,1,0,1,0,1;0,1,10,100,500,900;\n");
  client.stop();
  return true;
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



// -------------------- status handler -------------------- 

boolean status_handler(TinyWebServer& web_server) {
  web_server.send_error_code(200);
  web_server.send_content_type("text/plain");
  web_server.end_headers();
  Client& client = web_server.get_client();
  client.print(stateRow[0], DEC);
  client.print(",");
    client.print(stateRow[1], DEC);
  client.print(",");
    client.print(stateRow[2], DEC);
  client.print(",");
    client.print(stateRow[3], DEC);
  client.print(",");
    client.print(stateRow[4], DEC);
  client.print(",");
    client.println(stateRow[5], DEC);
  return true;
}


// -------------------- file uploader -------------------- 
byte tempGuiFlag = 0;

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
	Serial << F("Creating ") << fname << "\n";
	file.open(&root, fname, O_CREAT | O_WRITE | O_TRUNC);

        //IF PROGRAM.TXT WAS UPDATED, TRIGGER UPDATE TO PROGRAM ARRAYS
        Serial.println(fname);
        if (strcmp(fname,"PROGRAM.TXT") == 0){    //strcmp compares the pointer to the string 0 means equal.
          tempGuiFlag = 1;
        }
        Serial.println("case START");
        Serial.print("tempGuiFlag = ");
        Serial.println(tempGuiFlag);
        
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
    Serial << F("Wrote ") << file.fileSize() << F(" bytes in ")
	   << millis() - start_time << F(" millis (received ")
           << total_size << F(" bytes)\n");
    file.close();
    if (tempGuiFlag == 1){
      guiFlag = 1;
      tempGuiFlag = 0;
      Serial.println("case END");
      Serial.print("tempGuiFlag = ");
      Serial.println(tempGuiFlag);
    }
  }
}




// -------------------- begin firmware -------------------- 

void setup() {
  Serial.begin(115200);
  Serial << F("Free RAM: ") << FreeRam() << "\n";

//  pinMode(LEDPIN, OUTPUT);
//  setLedEnabled(false);

  pinMode(SS_PIN, OUTPUT);	// set the SS pin as an output
                                // (necessary to keep the board as
                                // master and not SPI slave)
  digitalWrite(SS_PIN, HIGH);	// and ensure SS is high

  // Ensure we are in a consistent state after power-up or a reset
  // button These pins are standard for the Arduino w5100 Rev 3
  // ethernet board They may need to be re-jigged for different boards
  pinMode(ETHER_CS, OUTPUT); 	// Set the CS pin as an output
  digitalWrite(ETHER_CS, HIGH); // Turn off the W5100 chip! (wait for
                                // configuration)
  pinMode(SD_CS, OUTPUT);       // Set the SDcard CS pin as an output
  digitalWrite(SD_CS, HIGH); 	// Turn off the SD card! (wait for
                                // configuration)

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

  // Initialize the Ethernet.
  Serial << F("Setting up the Ethernet card...\n");
  Ethernet.begin(mac, ip);

  // Start the web server.
  Serial << F("Web server starting...\n");
  web.begin();

  Serial << F("Ready to accept HTTP requests.\n");
  
  
  
   pinMode(pinOut1, OUTPUT);
   pinMode(pinOut2, OUTPUT);
   pinMode(pinOut3, OUTPUT);
   pinMode(pinOut4, OUTPUT);
   pinMode(pinOut5, OUTPUT);
   pinMode(pinOut6, OUTPUT);
}
//----- AA1b -- Initialize Output Function -----
// Function that initializes the outputs states to "off"
void initializeFunction() 
{
//m   Serial.println(" ");
   delay(d*3);
//m   Serial.println(" ");
//m   Serial.println("Initializing Variables");
   for(int a = 0; a < 7; a++) {
     outputSelectFunction(a, 0);  //start with all pins off
   }
   ini = 1;
   delay(d*3);
//m   Serial.println("Initialization Complete");
//m   Serial.println(" ");
}

//----- Section AA4b -----
// Function that pairs the input pin state to the row that is listening for it
// Returns int 1 for "on" or 0 for "off"
// This actually reads the sensor and interperets what the value means
int inputSelectFunction(int rowNumber) {
  int trig;
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
  y = inputHiLowArray[x-1];  //Returns High/Low definition for corresponding input
  if((y == 1) && (val >= 512)) {trig = 1;}    // if high and supposed to be, trigger on
  if((y == 0) && (val >= 512)) {trig = 0;}    // if low and supposed to be high, don't trigger
  if((y == 1) && (val < 512)) {trig = 0;}
  if((y == 0) && (val < 512)) {trig = 1;}
  return trig;
}

//  Function to write messages to gui            CURRENTLY WRITTEN FOR SERIAL, NOT GUI!
void guiMess(int n) {
  if (n==1) {Serial.println("Definitions don't make sense");}
  if (n==2) {Serial.println("No SD Card anymore");}
  if (n==3) {Serial.println("Problem opening file on SD card");}
  if (n==4) {Serial.println("Corrupted SD file -Read/Write fail");}
  if (n==5) {Serial.println("Successfully updated definitions");}
  if (n==6) {Serial.println("Finished writing to .csv file");}
  
}

//----- Section AA4c -----
// Function that pairs the output pins to the row that is controlling for it
void outputSelectFunction(int rowNumber, int onOff) {
  int x;      //local variable
  int y;
  x = outputArray[rowNumber]; //
  y = outputHiLowArray[x-1];// lookup which value is considered "on"
  if(onOff == 1) {         // turn output on
    if(y == 1) {           // "on" means turn output high
      if(x == 0) {return;} //Do nothing for "N/A" case and break out of function
      if(x == 1) {digitalWrite(pinOut1, HIGH);}
      if(x == 2) {digitalWrite(pinOut2, HIGH);}
      if(x == 3) {digitalWrite(pinOut3, HIGH);}
      if(x == 4) {digitalWrite(pinOut4, HIGH);}
      if(x == 5) {digitalWrite(pinOut5, HIGH);}
      if(x == 6) {digitalWrite(pinOut6, HIGH);} 
      }
    if(y == 0) {           // "on" means turn output low
      if(x == 0) {return;} //Do nothing for "N/A" case and break out of function
      if(x == 1) {digitalWrite(pinOut1, LOW);}
      if(x == 2) {digitalWrite(pinOut2, LOW);}
      if(x == 3) {digitalWrite(pinOut3, LOW);}
      if(x == 4) {digitalWrite(pinOut4, LOW);}
      if(x == 5) {digitalWrite(pinOut5, LOW);}
      if(x == 6) {digitalWrite(pinOut6, LOW);} }
    return; }
  if(onOff == 0) {         //turn output off
    if(y == 1) {           // "off" means turn output low
      if(x == 0) {return;} //Do nothing for "N/A" case and break out of function
      if(x == 1) {digitalWrite(pinOut1, LOW);}
      if(x == 2) {digitalWrite(pinOut2, LOW);}
      if(x == 3) {digitalWrite(pinOut3, LOW);}
      if(x == 4) {digitalWrite(pinOut4, LOW);}
      if(x == 5) {digitalWrite(pinOut5, LOW);}
      if(x == 6) {digitalWrite(pinOut6, LOW);} }
    if(y == 0) {           // "off" means turn output high
      if(x == 0) {return;} //Do nothing for "N/A" case and break out of function
      if(x == 1) {digitalWrite(pinOut1, HIGH);}
      if(x == 2) {digitalWrite(pinOut2, HIGH);}
      if(x == 3) {digitalWrite(pinOut3, HIGH);}
      if(x == 4) {digitalWrite(pinOut4, HIGH);}
      if(x == 5) {digitalWrite(pinOut5, HIGH);}
      if(x == 6) {digitalWrite(pinOut6, HIGH);} }
    return; }
  }

//-------------------- Main Function ------------------------
void loop(){
  
  delay(d/2);  //For debug use only
  
  //----- Section AA1a -----
  if(ini == 0) {  //If the states have not been initialized, do so.
    initializeFunction();
  }
  
  //----- Section AA4a ----- Reading Inputs and Writing to Outputs -----
  tempStampA = millis();
  for(int z = 0; z < rn; z++) {              //runs loop for each row
    if(stateRow[z] == 1) {                   //STATE 1 = Waiting for a trigger
      trigState[z] = inputSelectFunction(z); //Call function and pass(Row number) to see if input is on or off
      if(trigState[z] == 1) {                //If input is "on"
        stateRow[z] = 2;                     //Moves to next state
      }
      if(trigState[z] == 0) {               //If input is off
      //Do nothing
      }
    }
    else if(stateRow[z] == 2) {             //STATE 2 = Trigger message just received
      timeStampDelayRow[z] = millis();      //Gets time stamp
      stateRow[z] = 3;                      //Moves on to next state
    }
    else if(stateRow[z] == 3) {             //STATE 3 = Delay vs. timeStamp
      nowTime = millis();
      netTime = nowTime - timeStampDelayRow[z];
      if(netTime >= DelayRow[z] * 1000) {          //Tests to see if time > delay
        stateRow[z] = 4;                    //If we've met our delay, go to next state
      }
    }
    else if(stateRow[z] == 4) {             //STATE 4 = Turn output 1 on
      outputSelectFunction(z, 1);           //(Row, On)
      timeStampDurationRow[z] = millis();   //Get timestamp
      stateRow[z] = 5;                      //Moves on to next state
    }
    else if(stateRow[z] == 5) {             //STATE 5 = Duration of output "on"
      nowTime = millis();
      netTime = nowTime - timeStampDurationRow[z];
      if(netTime >= DurationRow[z] * 1000) {      
        outputSelectFunction(z, 0);        //Turn output off after duration is over
        stateRow[z] = 1;                   //Moves on to trigger-waiting state
      }
    }
    else {                                 //if state is not 1-5, set to 1 (waiting)
      stateRow[z] = 1;                     // this is to increase robustness
//m      Serial.print("Initializing Row State ");
//m      Serial.println(z);
      delay (d);
    }
  }
    
  //----- Section AA5 ----- Update the Status LEDs on shield -----
  
  
  
  
  
  
  
  
  //----- Section AA6 ----- Update the Status variables for GUI to read
  
  
  
  
  
  
  

  //----- Section AA9 ----- See if GUI definitions have changed
  if(guiFlag == 0) {  //No new GUI definitions
    //return;  //break out of main funtion if no new definitions have come in.  POSSIBLY REMOVE THE HARD RETURN?
  }
  if(guiFlag == 1) {  //If there are new GUI definitions...
    Serial.println("Program.txt updated");
    
    //----- Section AB1 -----
    // PUT FUNCTION HERE TO READ GUI DEFINITIONS
    char* newvar = open_file("program.txt");    //store the file in a var
    
    Serial.println(newvar);                      //print the file out
    convert(newvar);                            //convert the file to arrays
    
    //----- Section AB2 -----
    //FUNCTION TO MAKE SURE GUI DEFINITIONS MAKE SENSE
     
    //----- Section AB3 -----
    //FUNCTION TO SAVE DEFINITIONS TO SD CARD

    //CHECK TO SEE IF SD CARD IS THERE 
    
    //----- Section AB4 -----
    //FUNCTION TO READ DEFINITIONS ON SD CARD
    
    //COMPARE TO VALUES FROM AB1
    
    //Sends GUI confirmation message
    guiMess(5);  //GUI message #5
    
    guiFlag = 0;                                //reset guiFlag
  }
  
 
 
   if (has_filesystem) {  //This tiny section runs the entire web server. Must be in void loop()
    web.process();
  }
}// end void loop


//----- Section open SD file for conversion to arrays -----
char* open_file(char* input_file){
  char storage[150];                    //used to store read stuff
  char ch;                              //used to store incoming byte
  byte i = 0;                           //used as counter for building string
  char* fail = "";                      //the failure return
  
  Serial.print("SD.begin = ");
  Serial.print(SD.begin(SD_CS));
  Serial.print(" ,has_filesystem = ");
  Serial.println(has_filesystem);
  
  //THIS BOTHERSOME SECTION SEEMS TO BEHAVE INCONSISTENTLY AT BEST. COMMENTING OUT TO KEEP THE BRIDGE WORKING. IT IS ASSUMED THAT THE SD CARD IS WORKING AT THIS POINT.
  //if (!SD.begin(SD_CS)) {
  //if (!has_filesystem){                  //hijack the already check SD from TinyWebServer library
  //  Serial.println("SD.begin failed");
  //  return fail;          
  //}
  //Serial.println("ready");

  
  File file = SD.open(input_file);
  
  Serial.print("input_file = ");
  Serial.print(input_file);
  Serial.print(", file read = ");
  Serial.println(file);
  
  if (file) {                           //if there's a file
    Serial.println(input_file);
    while (file.available()) {          //if there are unread bytes in the file
      ch = file.read();                 //read one
      storage[i] = ch;                  //append it to storage
      i ++;                             //inc counter
    }
    file.close();                      //close the file
    return storage;                    //return the read bytes
  }else{                              //no file
    Serial.print("no file");          //error
    return fail;                      //return w/ fail
  }
}//end open_file


//----- Section Convert cupcake string from SD/Web to arrays -----
char convert(char* readString){
  char* col[6];
  char* tok;
  //byte input_arr[6]; inputArray
  //byte in_onoff[6];
  //unsigned int ondelay[6];
  //byte out_arr[6];
  //byte out_onoff[6];
  //unsigned int duration[6];
  
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
      for (i = 1; i < 6; i++) {
        if (tok == NULL)
          break;
        tok = strtok(NULL, ";");
        col[i] = tok;
      }
      
      // Now turn the array of strings into arrays of numbers.  Each array is
      // named for the column it represents.  The values are separated by
      // commas.  atoi is used to convert the stringified numbers back into
      // integers.  The values returned by atoi, which are ints, are cast
      // into the appropriate data type.  (That's what the (byte) before
      // atoi(tok) is doing.)  It would be more graceful to create a function
      // to do this, rather than repeat it 6 times.
      // input_arr
      tok = strtok(col[0], ",");
      for (i = 0; i < 6; i++) {
        if (tok == NULL)
          break;
        inputArray[i] = (byte)atoi(tok);
        tok = strtok(NULL, ",");
      }
      // in_onoff
      tok = strtok(col[1], ",");
      for (i = 0; i < 6; i++) {
        if (tok == NULL)
          break;
        inputHiLowArray[i] = (byte)atoi(tok);
        tok = strtok(NULL, ",");
      }
      // ondelay
      tok = strtok(col[2], ",");
      for (i = 0; i < 6; i++) {
        if (tok == NULL)
          break;
        DelayRow[i] = (unsigned int)atoi(tok);
        tok = strtok(NULL, ",");
      }
      // out_arr
      tok = strtok(col[3], ",");
      for (i = 0; i < 6; i++) {
        if (tok == NULL)
          break;
        outputArray[i] = (byte)atoi(tok);
        tok = strtok(NULL, ",");
      }
      // out_onoff
      tok = strtok(col[4], ",");
      for (i = 0; i < 6; i++) {
        if (tok == NULL)
          break;
        outputHiLowArray[i] = (byte)atoi(tok);
        tok = strtok(NULL, ",");
      }
      // duration
      tok = strtok(col[5], ",");
      for (i = 0; i < 6; i++) {
        if (tok == NULL)
          break;
        DurationRow[i] = (unsigned int)atoi(tok);
        tok = strtok(NULL, ",");
      }
      
      // Now print out all the values to make sure it all worked
      Serial.print("Input array: ");
      for (i = 0; i < 6; i++){
        Serial.print(inputArray[i]);
        Serial.print(" ");
      }
      
      Serial.print("\ninputHiLowArray: ");
      for (i = 0; i < 6; i++){
        Serial.print(inputHiLowArray[i]);
        Serial.print(" ");
      }
      
      Serial.print("\nOn delay: ");
      for (i = 0; i < 6; i++){
        Serial.print(DelayRow[i]);
        Serial.print(" ");
      }
      
      Serial.print("\nOutput array: ");
      for (i = 0; i < 6; i++){
        Serial.print(outputArray[i]);
        Serial.print(" ");
      }
      
      Serial.print("\nOutput on/off: ");
      for (i = 0; i < 6; i++){
        Serial.print(outputHiLowArray[i]);
        Serial.print(" ");
      }
      
      Serial.print("\nDuration: ");
      for (i = 0; i < 6; i++){
        Serial.print(DurationRow[i]);
        Serial.print(" ");
      }
}//end convert cupcake string to arrays function
