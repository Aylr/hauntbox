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
int guiFlag = 1;  //GUI Flag tells us when there are new definitions from the GUI

//THESE VALUES ARE SET ARBITRARILY TO TEST CODE.  WILL NEED TO CHANGE IN FINAL VERSION!
int inputArray[] =       {6, 6, 6, 6, 6, 6};    //which input (0-6) is read by which row ({row0, row1, ...})
                                                //0 = no input, 1 = input #1, 2 = input #2, etc

int inputHiLowArray[] =  {1, 1, 1, 1, 1, 0};    //What signal level is considered "on" for input # ({input1, input2, ...})
                                                //1 = High, 0 = Low 
                                                
int outputArray[] =      {1, 2, 3, 4, 5, 6};    //which outputs (0-6) are controlled by which row ({row0, row1, ...})
                                                //0 = no output, 1 = output #1, 2 = output #2, etc
                                                
int outputHiLowArray[] = {1, 1, 1, 1, 1, 1};    //Output considered on when High (1) or Low (0)
int DelayRow[] = {1000, 2000, 3000, 4000, 5000, 6000};     //Time in millis  
int DurationRow[] = {6000, 6000, 6000, 6000, 6000, 6000};  //Time in millis  //TURN INTO AN ARRAY FOR FINAL CODE


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

int CS_pin = 4;  //Pin to SD card  IS THIS THE SAME AS PIN 10, BUT FOR THE SHIELD?
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

boolean file_handler(TinyWebServer& web_server);          // FIND OUT WHAT THIS IS/MEANS!!!
//boolean blink_led_handler(TinyWebServer& web_server);
//boolean led_status_handler(TinyWebServer& web_server);
boolean index_handler(TinyWebServer& web_server);

TinyWebServer::PathHandler handlers[] = {
  // Work around Arduino's IDE preprocessor bug in handling /* inside
  // strings.
  //
  // `put_handler' is defined in TinyWebServer
 // {"/", TinyWebServer::GET, &index_handler },
  {"/", TinyWebServer::GET, &index_handler },
  {"/gui", TinyWebServer::GET, &gui_handler },
    {"/ram", TinyWebServer::GET, &ram_handler },
  {"/status", TinyWebServer::GET, &status_handler },
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


// -------------------- Settings you should modify -------------------- 

static uint8_t mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
// Don't forget to modify the IP to an available one on your home network
byte ip[] = { 192, 168, 1, 9 };


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
  send_file_name(web_server, "INDEX.HTM");
  return true;
}


// -------------------- gui handler -------------------- 

boolean gui_handler(TinyWebServer& web_server) {
    send_file_name(web_server, "GUI.HTM");
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

void file_uploader_handler(TinyWebServer& web_server,
			   TinyWebPutHandler::PutAction action,
			   char* buffer, int size) {
  static uint32_t start_time;
  static uint32_t total_size;

  switch (action) {
  case TinyWebPutHandler::START:
    start_time = millis();
    total_size = 0;
    if (!file.isOpen()) {
      // File is not opened, create it. First obtain the desired name
      // from the request path.
      char* fname = web_server.get_file_from_path(web_server.get_path());
      if (fname) {
	Serial << F("Creating ") << fname << "\n";
	file.open(&root, fname, O_CREAT | O_WRITE | O_TRUNC);
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
  }
}




// -------------------- begin firmware -------------------- 

void setup() {
   Serial.begin(9600);
   Serial << F("Free RAM: ") << FreeRam() << "\n";
    // -------------------- initialize SD card --------------------
  Serial << F("Setting up SD card...\n");
  pinMode(10, OUTPUT); // set the SS pin as an output (necessary!)
  digitalWrite(10, HIGH); // but turn off the W5100 chip!
  if (!card.init(SPI_FULL_SPEED, 4)) {
    Serial << F("card failed\n");
    has_filesystem = false;
  }
  // initialize a FAT volume
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
//m  if (n==1) {Serial.println("Definitions don't make sense");}
//m  if (n==2) {Serial.println("No SD Card anymore");}
//m  if (n==3) {Serial.println("Problem opening file on SD card");}
//m  if (n==4) {Serial.println("Corrupted SD file -Read/Write fail");}
//m  if (n==5) {Serial.println("Successfully updated definitions");}
//m  if (n==6) {Serial.println("Finished writing to .csv file");}
  
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
      if(netTime >= DelayRow[z]) {          //Tests to see if time > delay
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
      if(netTime >= DurationRow[z]) {      
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
  if(guiFlag = 0) {  //No new GUI definitions
    return;  //break out of main funtion if no new definitions have come in.  POSSIBLY REMOVE THE HARD RETURN?
  }
  if(guiFlag = 1) {  //New GUI definitions
  //----- Section AB1 -----
  // PUT FUNCTION HERE TO READ GUI DEFINITIONS
  //  int inputArray[] =       
  //  int inputHiLowArray[] =                     
  //  int outputArray[] =                          
  //  int outputHiLowArray[] = 
  //  int DelayRow[] = 
  //  int DurationRow[] = 
  
  
  
  //----- Section AB2 -----
  //FUNCTION TO MAKE SURE GUI DEFINITIONS MAKE SENSE
 
 
 
 //----- Section AB3 -----
 //FUNCTION TO SAVE DEFINITIONS TO SD CARD


 
  
  //CHECK TO SEE IF SD CARD IS THERE
  
  //Open file to write to
  File dataFile = SD.open("definitions.csv", FILE_WRITE);
  if (dataFile) {
    //FUNCTION HERE TO WRITE TO FILE
    dataFile.close();
    guiMess(6);  //GUI message #6 
  }
  else {         //If unable to open SD file.
    guiMess(3);  //GUI message #3
  }
  
  
  
  
  //----- Section AB4 -----
  //FUNCTION TO READ DEFINITIONS ON SD CARD
  
  //COMPARE TO VALUES FROM AB1
  
  //Sends GUI confirmation message
  guiMess(5);  //GUI message #5
  
}  
  
 
 
   if (has_filesystem) {
    web.process();
  }
}