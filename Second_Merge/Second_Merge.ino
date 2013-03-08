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

#define MAXROWS 6


int d = 0;  //Delay used in testing of code.  Set to 0 if not testing code.

//--------------------Define/get variables from GUI-------------------------------------
int guiFlag = 0;  //GUI Flag tells us when there are new definitions from the GUI

//THESE VALUES ARE SET ARBITRARILY TO TEST CODE.  WILL NEED TO CHANGE IN FINAL VERSION!
byte inputArray[] =       {1, 6, 6, 6, 6, 6};    //which input (0-6) is read by which row ({row0, row1, ...})
                                                //0 = no input, 1 = input #1, 2 = input #2, etc

byte inputHiLowArray[] =  {1, 1, 1, 1, 1, 0};    //What signal level is considered "on" for input # ({input1, input2, ...})
                                                //1 = High, 0 = Low
byte inputLEDArray [] =  {39,32,33,34,35,36};
                                                
byte outputArray[] =      {1, 2, 3, 4, 5, 6};    //which outputs (0-6) are controlled by which row ({row0, row1, ...})
                                                //0 = no output, 1 = output #1, 2 = output #2, etc
                                                
byte outputHiLowArray[] = {1, 1, 1, 1, 1, 1};    //Output considered on when High (1) or Low (0)
unsigned int DelayRow[] = {0, 2000, 3000, 4000, 5000, 6000};     //Time in millis
unsigned int DurationRow[] = {1000, 6000, 6000, 6000, 6000, 6000};  //Time in millis  //TURN INTO AN ARRAY FOR FINAL CODE
int durationType[] = {0,1,2,0,1,2};  //The type of duration 0 = until further notice, 1 = while input active, 2 = for ...


//----------------------Define variables in code-----------------------------
int rn = 6;  //number of rows
bool outputState[6] = {0,0,0,0,0,0};   //array to hold on/off (1/0) state of every given output. Manipulated by any/multiple rules
                //***only 6 outputs!!!
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


/****************VALUES YOU CHANGE*************/
// pin 4 is the SPI select pin for the SDcard if using an ethernet shield
const int SD_CS = 4;//31;//4;        //****CHANGE TO PIN 31 FOR REAL HAUNTBOX RATHER THAN STACK'O'SHIELDS

// pin 10 is the SPI select pin for the Ethernet if using an ethernet shield
const int ETHER_CS = 10;//53;//10;      //****** CHANGE TO PIN 53 FOR REAL HAUNTBOX RATHER THAN STACK'O'SHIELDS

// Don't forget to modify the IP to an available one on your home network
byte ip[] = { 192, 168, 0, 100 };


/*********************************************/
char StorageString[100];
static uint8_t mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEE };
char* browser_header = "HTTP/1.0 200 OK\nContent-Type: text/html\n";  //2 line header including mandatory blank line to signify data below

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
  {"/output_status", TinyWebServer::GET, &output_status_handler },
  {"/program", TinyWebServer::GET, &program_handler },
  {"/settings", TinyWebServer::GET, &settings_handler },
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


// -------------------- row status handler -------------------- 
boolean row_status_handler(TinyWebServer& web_server) {
  web_server.send_error_code(200);
  web_server.send_content_type("text/plain");
  web_server.end_headers();
  
  Client& client = web_server.get_client();
  for (int i = 0; i < 6; i++){
    client.print(stateRow[0], DEC);
    if(i < 5){
      client.print(",");
    }
  }
  return true;
}

// -------------------- output status handler -------------------- 
boolean output_status_handler(TinyWebServer& web_server) {
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
  int i;
  
  Serial.begin(115200);
  Serial << F("Free RAM: ") << FreeRam() << "\n";

  pinMode(SS_PIN, OUTPUT);	// set the SS pin as an output (necessary to keep the board as master and not SPI slave)
  digitalWrite(SS_PIN, HIGH);	// and ensure SS is high

  // Ensure we are in a consistent state after power-up or a reset button These pins are standard for the Arduino w5100 Rev 3 ethernet board They may need to be re-jigged for different boards
  pinMode(ETHER_CS, OUTPUT); 	// Set the CS pin as an output
  digitalWrite(ETHER_CS, HIGH); // Turn off the W5100 chip! (wait for configuration)                              
  pinMode(SD_CS, OUTPUT);       // Set the SDcard CS pin as an output
  digitalWrite(SD_CS, HIGH); 	// Turn off the SD card! (wait for configuration)  

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
      if (ip_temp != "") {
        
        char* tok;
        char* val[4];
        
        tok = strtok(ip_temp, ".");
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
        
        //Placeholders for actual code that should validate the IP address
        //test for real IP which should look like this: byte ip[] = { 192, 168, 0, 100 };
        
        Serial << F("Static IP on\n");
        Serial << ip_temp;
        Ethernet.begin(mac,ip);                                 //setup with static address

        
      }else{      //if there is not a static ip specified... use DHCP
        
        Serial << F("Setting up the Ethernet card...\n");
        if (Ethernet.begin(mac) == 0) {                          // Initialize ethernet with DHCP
           Serial << F("DHCP failed\n");
           
        }
      }
   }//end if has filesystem
   
   //print IP address to serial
   Serial.print("IP Address: ");
   for (byte thisByte = 0; thisByte < 4; thisByte++) {                   // print the value of each byte of the IP address:
          Serial.print(Ethernet.localIP()[thisByte], DEC);
          Serial.print("."); 
        }
    Serial.print("\n");
  
  // Start the web server.
  Serial << F("Web server starting...\n");
  web.begin();
  
  // Start the bonjour/zeroconf service
  EthernetBonjour.begin("hauntbox");                                        //Set the advertised name
  EthernetBonjour.addServiceRecord("Hauntbox._http", 80, MDNSServiceTCP);   //Set the advertised port/service

  Serial << F("Ready to accept HTTP requests.\n");
  
   pinMode(pinOut1, OUTPUT);
   pinMode(pinOut2, OUTPUT);
   pinMode(pinOut3, OUTPUT);
   pinMode(pinOut4, OUTPUT);
   pinMode(pinOut5, OUTPUT);
   pinMode(pinOut6, OUTPUT);
   
   //---------indicator LEDs
   //1:23, 2:25, 3:27, 4:29, 5:31, 6:33
   //A:22, B:24, C:26, D:28, E:30, F:32
   // temporarlily disable a few to prevent overcurrent
   
   //pinMode(22, OUTPUT);
   //1: pinMode(23, OUTPUT);
   pinMode(24, OUTPUT);
   pinMode(25, OUTPUT);
   pinMode(26, OUTPUT);
   pinMode(27, OUTPUT);
   pinMode(28, OUTPUT);
   //pinMode(29, OUTPUT);
   //pinMode(30, OUTPUT);
   //pinMode(31, OUTPUT);
   //pinMode(32, OUTPUT);
   //6: pinMode(33, OUTPUT);
}
//----- AA1b -- Initialize Output Function -----
// Function that initializes the outputs states to "off"
void initializeFunction() 
{
//m   Serial.println(" ");
   delay(d*3);
//m   Serial.println(" ");
//m   Serial.println("Initializing Variables");
   for(int a = 0; a < 6; a++) {
      outputState[a] = 0;
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
  if (n==5) {Serial.println("Successfully updated programming");}
  if (n==6) {Serial.println("Finished writing to .csv file");}
  
}

//----- Section AA4c -----
   //---------indicator LEDs
   //1:23, 2:25, 3:27, 4:29, 5:31, 6:33
   //A:22, B:24, C:26, D:28, E:30, F:32
// Function that pairs the output pins to the row that is controlling for it
void outputSelectFunction(int outputNumber, bool action) {
  //takes an output (outputNumber) and an action:
    //0 = off
    //1 = on
  int x;      //local variable
  int y;
  x = outputArray[outputNumber]; //lookup which output is being controlled
  y = outputHiLowArray[x-1];// lookup which value is considered "on"
  
  if(action == 1) {         // turn output on
    if(y == 1) {           // "on" means turn output high
      if(x == 0) {return;} //Do nothing for "N/A" case and break out of function
      if(x == 1) {digitalWrite(pinOut1, HIGH); }//digitalWrite(23, HIGH); }
      if(x == 2) {digitalWrite(pinOut2, HIGH); }//digitalWrite(25, HIGH); }
      if(x == 3) {digitalWrite(pinOut3, HIGH); }//digitalWrite(27, HIGH); }
      if(x == 4) {digitalWrite(pinOut4, HIGH); }//digitalWrite(29, HIGH); }
      if(x == 5) {digitalWrite(pinOut5, HIGH); }//digitalWrite(31, HIGH); }
      if(x == 6) {digitalWrite(pinOut6, HIGH); }}//digitalWrite(33, HIGH); } }
    if(y == 0) {           // "on" means turn output low
      if(x == 0) {return;} //Do nothing for "N/A" case and break out of function
      if(x == 1) {digitalWrite(pinOut1, LOW); }//digitalWrite(23, LOW);}
      if(x == 2) {digitalWrite(pinOut2, LOW); }//digitalWrite(25, LOW);}
      if(x == 3) {digitalWrite(pinOut3, LOW); }//digitalWrite(27, LOW);}
      if(x == 4) {digitalWrite(pinOut4, LOW); }//digitalWrite(29, LOW);}
      if(x == 5) {digitalWrite(pinOut5, LOW); }//digitalWrite(31, LOW);}
      if(x == 6) {digitalWrite(pinOut6, LOW); }}//digitalWrite(33, LOW);} }
    return; }
  if(action == 0) {         //turn output off
    if(y == 1) {           // "off" means turn output low
      if(x == 0) {return;} //Do nothing for "N/A" case and break out of function
      if(x == 1) {digitalWrite(pinOut1, LOW); }//digitalWrite(23, LOW);}
      if(x == 2) {digitalWrite(pinOut2, LOW); }//digitalWrite(25, LOW);}
      if(x == 3) {digitalWrite(pinOut3, LOW); }//digitalWrite(27, LOW);}
      if(x == 4) {digitalWrite(pinOut4, LOW); }//digitalWrite(29, LOW);}
      if(x == 5) {digitalWrite(pinOut5, LOW); }//digitalWrite(31, LOW);}
      if(x == 6) {digitalWrite(pinOut6, LOW); }}//digitalWrite(33, LOW);} }
    if(y == 0) {           // "off" means turn output high
      if(x == 0) {return;} //Do nothing for "N/A" case and break out of function
      if(x == 1) {digitalWrite(pinOut1, HIGH); }//digitalWrite(23, HIGH); }
      if(x == 2) {digitalWrite(pinOut2, HIGH); }//digitalWrite(25, HIGH); }
      if(x == 3) {digitalWrite(pinOut3, HIGH); }//digitalWrite(27, HIGH); }
      if(x == 4) {digitalWrite(pinOut4, HIGH); }//digitalWrite(29, HIGH); }
      if(x == 5) {digitalWrite(pinOut5, HIGH); }//digitalWrite(31, HIGH); }
      if(x == 6) {digitalWrite(pinOut6, HIGH); }}//digitalWrite(33, HIGH); } }
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
      if(trigState[z] == 1) {                //If triggered
        stateRow[z] = 2;                     //Moves to next state
      }
      if(trigState[z] == 0) {               //If not triggered
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
    else if(stateRow[z] == 4) {             //STATE 4 = Change output (make it on/off/toggle)
      if (outputHiLowArray[z] == 0)       //if it should be off
      {
        outputState[z] = 0;
      }
      else if(outputHiLowArray[z] == 1){  //if it should be on
        outputState[z] = 1;
      }
      else if(outputHiLowArray[z] == 2){  //if it should toggle
        outputState[z] = !outputState[z]; //flip the bit
      }
      outputSelectFunction(z, outputState[z]);    //enact the change
      timeStampDurationRow[z] = millis();   //Get timestamp
      stateRow[z] = 5;                      //Moves on to next state
    }
    else if(stateRow[z] == 5) {             //STATE 5 = Duration of output "on"
      //switch for 3 different duration types
      if (durationType[z] == 0) {  //"until further notice"
        stateRow[z] = 1;  //reset row state to waiting for trigger
      }
      else if(durationType[z] == 1) {  //"while input triggered"
        if(trigState[z] == 0){  //trigger has stopped active
          if (outputHiLowArray[z] == 1) //if on, turn back off
          {
            outputState[z] = 0;
            outputSelectFunction(z, 0);
          }
          else if(outputHiLowArray[z] == 0){ //if off, turn back on
            outputState[z] = 1;
            outputSelectFunction(z, 1);
          }
          else if(outputHiLowArray[z] == 2){  //if toggle
            outputState[z] = !outputState[z]; //change the current state
            outputSelectFunction(z, outputState[z]);
          }
           stateRow[z] = 1;
        }
      }
      else if (durationType[z] == 2) {    //"for...seconds"
        nowTime = millis();
        netTime = nowTime - timeStampDurationRow[z];
        if(netTime >= DurationRow[z] * 1000) {
          if (outputHiLowArray[z] == 1) //if on, turn back off
          {
            outputState[z] = 0;
            outputSelectFunction(z, 0);
          }
          else if(outputHiLowArray[z] == 0){ //if off, turn back on
            outputState[z] = 1;
            outputSelectFunction(z, 1);
          }
          else if(outputHiLowArray[z] == 2){  //if toggle
            outputState[z] = !outputState[z]; //change the current state
            outputSelectFunction(z, outputState[z]);
          }
          stateRow[z] = 1;                   //Moves on to trigger-waiting state
        }
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
  for(int z = 0; z < rn; z++) {              //runs loop for each row
    trigState[z] = inputSelectFunction(z); //Call function and pass(Row number) to see if input is on or off
      if(trigState[z] == 1) {                //If input is "on"
        digitalWrite(inputLEDArray[z],HIGH);  //turn on appropriate LED array
      }
      if(trigState[z] == 0) {               //If input is off
        digitalWrite(inputLEDArray[z],LOW);  //turn off appropriate LED array
      }
  }
  
  
  
  
  
  
  
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
    EthernetBonjour.run();  //zeroconf/bonjour
}// end void loop


//----- Section open SD file for conversion to arrays -----
char* open_file(char* input_file){
  char storage[150];                    //used to store read stuff
  char ch;                              //used to store incoming byte
  byte i = 0;                           //used as counter for building string
  char* fail = "";                      //the failure return
  
  Serial.print("SD.begin = ");
  Serial.print(SD.begin(SD_CS));
  Serial.print(", has_filesystem = ");
  Serial.println(has_filesystem);
  
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
    Serial.println("no file");          //error
    return fail;                      //return w/ fail
  }
}//end open_file


//----- Section Convert cupcake string from SD/Web to arrays -----
char convert(char* readString){
  char* col[7];
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
      for (i = 1; i < 7; i++) {
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
      // duration type
      tok = strtok(col[5], ",");
      for (i = 0; i < 6; i++) {
        if (tok == NULL)
          break;
        durationType[i] = (unsigned int)atoi(tok);
        tok = strtok(NULL, ",");
      }
      // duration
      tok = strtok(col[6], ",");
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
      
      Serial.print("\nDuration Type: ");
      for (i = 0; i < 6; i++){
        Serial.print(durationType[i]);
        Serial.print("\n");
      }
      
      Serial.print("\nDuration: ");
      for (i = 0; i < 6; i++){
        Serial.print(DurationRow[i]);
        Serial.print("\n");
      }
}//end convert cupcake string to arrays function
