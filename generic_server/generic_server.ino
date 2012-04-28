#include <pins_arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <Flash.h>
#include <SD.h>
#include <TinyWebServer.h>

/****************VALUES YOU CHANGE*************/
// pin 4 is the SPI select pin for the SDcard
const int SD_CS = 4;

// pin 10 is the SPI select pin for the Ethernet
const int ETHER_CS = 10;

// Don't forget to modify the IP to an available one on your home network
byte ip[] = { 192, 168, 1, 9 };
//byte ip[] = {10,0,0,10 };
int inputArray[6];

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
  {"/", TinyWebServer::GET, &index_handler },
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
      web_server.send_error_code(404);        //TESTING 404 CODES FOR JQUERY SAVE ERRORS
      web_server << F("Could not find file: ") << filename << "\n";
    }
  }
}



boolean file_handler(TinyWebServer& web_server) {
  char* filename = TinyWebServer::get_file_from_path(web_server.get_path());
  send_file_name(web_server, filename);
  free(filename);
  return true;
}



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


boolean program_handler(TinyWebServer& web_server){
  send_file_name(web_server, "program.txt");
  
  Client& client = web_server.get_client();
  //Serial.print("Free RAM: ");
  //Serial.println(FreeRam());
  //client.println(browser_header);

  //client.print("0,2,3,4,5,6;0,1,0,1,0,1;0,1,10,100,500,900;6,5,4,3,2,1;0,1,0,1,0,1;0,1,10,100,500,900;0;");
  client.print(";0;");
  client.print(FreeRam());
  client.print(";");
  client.print("\n");
  client.stop();
  return true; //exit the handler 
}

boolean settings_handler(TinyWebServer& web_server){
  send_file_name(web_server, "settings.txt");

  Client& client = web_server.get_client();
  //client.println(browser_header);
  
  //client.print("0,1,0,0,1,0,0,0,1,0,0,0;input1,input2,input3,input4,input5,input6;output1,output2,output3,output4,output5,output6;103,246,492,103,246,492,103,246,492,103,246,492;1,2,3,4,5,6;0;");
  client.print(";0;");    //send status code = 0
  client.print(FreeRam());
  client.print(";");
  client.print("\n");
  client.stop();
  return true; //exit the handler 
}

boolean export_handler(TinyWebServer& web_server) {
  Client& client = web_server.get_client();
  client.println(browser_header);
  
  client.print("Date, Description\nSettings:0,1,0,0,1,0,0,0,1,0,0,0;input1,input2,input3,input4,input5,input6;output1,output2,output3,output4,output5,output6;103,246,492,103,246,492,103,246,492,103,246,492;1,2,3,4,5,6;\nProgram: 0,2,3,4,5,6;0,1,0,1,0,1;0,1,10,100,500,900;6,5,4,3,2,1;0,1,0,1,0,1;0,1,10,100,500,900;\n");
  client.stop();
  return true;
}


boolean index_handler(TinyWebServer& web_server) {
  send_file_name(web_server, "gui.htm");
  return true;
}

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
    
    //IF PROGRAM.TXT WAS UPDATED, TRIGGER UPDATE TO PROGRAM ARRAYS
  }
}



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
}

void loop() {
  if (has_filesystem) {
    web.process();
  }
}
