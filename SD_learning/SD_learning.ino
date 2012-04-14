#include <SD.h>

void setup(){
  Serial.begin(115200);
  Serial.print("Prepping SD...");
  pinMode(10,OUTPUT);
  
  
  char* newvar = open_file("program.txt");
  Serial.println("new:");
  Serial.println(newvar);
}



char* open_file(char* input_file){
  char storage[100];                    //used to store read stuff
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

void loop(){
}
