//Second attempt at basic building block of HauntBox control
//Trying to write code for a row in the GUI, not for a specific input.
//Code written for 6 row control from HB
//12-01-20 (YY-MM-DD)

int d = 0;  //Delay used in testing of code.  Set to 0 if not testing code.

//Define/get variables from GUI
//THESE VALUES ARE SET ARBITRARILY TO TEST CODE.  WILL NEED TO CHANGE IN FINAL VERSION!
int inputArray[] =       {1, 6, 6, 6, 6, 6};    //which input (0-6) is read by which row ({row0, row1, ...})
                                                //0 = no input, 1 = input #1, 2 = input #2, etc

int inputHiLowArray[] =  {1, 1, 1, 1, 1, 0};    //What signal level is considered "on" for input # ({input1, input2, ...})
                                                //1 = High, 0 = Low 
                                                
int outputArray[] =      {1, 2, 3, 4, 5, 6};    //which outputs (0-6) are controlled by which row ({row0, row1, ...})
                                                //0 = no output, 1 = output #1, 2 = output #2, etc
                                                
int outputHiLowArray[] = {1, 1, 1, 1, 1, 1};    //Output considered on when High (1) or Low (0)
int DelayRow[] = {1000, 2000, 3000, 4000, 5000, 6000};     //Time in millis  
int DurationRow[] = {6000, 6000, 6000, 6000, 6000, 6000};  //Time in millis  //TURN INTO AN ARRAY FOR FINAL CODE


//Define variables in code
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
int pinOut1 = 7; //Digital pin
int pinOut2 = 6; //Digital pin
int pinOut3 = 5; //Digital pin
int pinOut4 = 4; //Digital pin
int pinOut5 = 3; //Digital pin
int pinOut6 = 2; //Digital pin

void setup() {
   Serial.begin(9600);                     
   pinMode(pinOut1, OUTPUT);
   pinMode(pinOut2, OUTPUT);
   pinMode(pinOut3, OUTPUT);
   pinMode(pinOut4, OUTPUT);
   pinMode(pinOut5, OUTPUT);
   pinMode(pinOut6, OUTPUT);
}

// Function that initializes the outputs states to "off"
void initializeFunction() {
   Serial.println(" ");
   delay(d*3);
   Serial.println(" ");
   Serial.println("Initializing Variables");
   for(int a = 0; a < 7; a++) {
     outputSelectFunction(a, 0);  //start with all pins off
   }
//   stateRow[rn];
//   trigState[rn];
//   timeStampDelayRow[rn];    //gets initialized immediately before it is used
//   timeStampDurationRow[rn];
   ini = 1;
   delay(d*3);
   Serial.println("Initialization Complete");
   Serial.println(" ");
}

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

void loop(){
  delay(d/2);  //For debug use only
  
  if(ini == 0) {  //if the states have not been initialized, do so.
    initializeFunction();
  }
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
      if(netTime >= DurationRow[z]) {      //SHOULD THIS BE MOVED TO STATE 1?
        outputSelectFunction(z, 0);        //Turn output off after duration is over
        stateRow[z] = 1;                   //Moves on to trigger-waiting state
      }
    }
    else {                                 //if state is not 1-5, set to 1 (waiting)
      stateRow[z] = 1;                     // this is to increase robustness
      Serial.print("Initializing Row State ");
      Serial.println(z);
      delay (d);
    }
    
//    Serial.print("Row State ");
//    Serial.print(z);
//    Serial.print(": ");
//    Serial.println(stateRow[z]);
  }
  //Serial.println("");
  //for(int n = 0; n < 6; n++) {
  //Serial.print(trigState[n]);
  //Serial.print(",");
  //}
 // Serial.println(" ");
 tempStampB = millis();
 //int tem = tempStampB - tempStampA;
 Serial.println(tempStampB);
 //Serial.print(" - ");
 //Serial.print(tempStampA);
 //Serial.print(" ");
 //Serial.println(tem);
}
