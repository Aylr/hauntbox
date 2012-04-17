#include <SD.h>

void setup(){
  Serial.begin(115200);
  Serial.print("Prepping SD...");
  pinMode(10,OUTPUT);
  
  
  char* newvar = open_file("upload.txt");
  //Serial.println("new:");
  Serial.println(newvar);
  convert(newvar);
}



char* open_file(char* input_file){
  char storage[150];                    //used to store read stuff
  char ch;                              //used to store incoming byte
  byte i = 0;                           //used as counter for building string
  char* fail = "";                      //the failure return
  
  if (!SD.begin(4)) {
    Serial.println("failed");
    return fail;          
  }
  Serial.println("ready");
  
  File file = SD.open(input_file);
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
}



char convert(char* readString){
  char* col[6];
  char* tok;
  byte input_arr[6];
  byte in_onoff[6];
  unsigned int ondelay[6];
  byte out_arr[6];
  byte out_onoff[6];
  unsigned int duration[6];
  
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
      for (i = 0; i < 6; i++){
        Serial.print(input_arr[i]);
        Serial.print(" ");
      }
      
      Serial.print("\nInput on/off: ");
      for (i = 0; i < 6; i++){
        Serial.print(in_onoff[i]);
        Serial.print(" ");
      }
      
      Serial.print("\nOn delay: ");
      for (i = 0; i < 6; i++){
        Serial.print(ondelay[i]);
        Serial.print(" ");
      }
      
      Serial.print("\nOutput array: ");
      for (i = 0; i < 6; i++){
        Serial.print(out_arr[i]);
        Serial.print(" ");
      }
      
      Serial.print("\nOutput on/off: ");
      for (i = 0; i < 6; i++){
        Serial.print(out_onoff[i]);
        Serial.print(" ");
      }
      
      Serial.print("\nDuration: ");
      for (i = 0; i < 6; i++){
        Serial.print(duration[i]);
        Serial.print(" ");
      }
}
        

void loop(){
}
