// Simple program to torture test all the outputs on the Hauntbox. Not terriby useful past that. Logs tests to SD card.

#include <SD.h>

const int SD_CS = 4;      // pin 4 is the SPI select pin for the SDcard
const int ETHER_CS = 10;  // pin 10 is the SPI select pin for the Ethernet



byte inputLEDArray [] =  {39,32,33,34,35,36};   //Array of arduino pins that correspond to the LEDs that indicate an input is triggered
byte outputLEDArray [] = {47,46,45,44,43,42};
byte outputArray[] = {49,48,38,41,40,37};
int onTime = 100;
int offTime = 0;
int relayOnTime = 1000;
int relayOffTime = 500;
unsigned long tortureCounter = 0;

void setup(){
	Serial.begin(115200);
	Serial.print("Starting up Hauntbox torture test... ");

	// Ensure we are in a consistent state after power-up or a reset button These pins are standard for the Arduino w5100 Rev 3 ethernet board They may need to be re-jigged for different boards
	pinMode(ETHER_CS, OUTPUT); 	// Set the CS pin as an output
	digitalWrite(ETHER_CS, HIGH); // Turn off the W5100 chip! (wait for configuration)
	pinMode(SD_CS, OUTPUT);       // Set the SDcard CS pin as an output
	digitalWrite(SD_CS, HIGH); 	// Turn off the SD card! (wait for configuration)


	Serial.print("Initializing SD card...");
	// see if the card is present and can be initialized:
	if (!SD.begin(SD_CS)) {
		Serial.println("Card failed, or not present");
		// don't do anything more:
		return;
	}
	Serial.println("SD card initialized.");

	for (int i = 0;i<6;i++){						//set all as outputs
		pinMode(outputArray[i], OUTPUT);
		pinMode(outputLEDArray[i], OUTPUT);
		pinMode(inputLEDArray[i], OUTPUT);
	}
	
	for (int i=0;i<6;i++){
		flash(inputLEDArray[i], onTime, offTime);
	}
	for (int i=5;i>=0;i--){
		flash(outputLEDArray[i], onTime, offTime);
	}
	Serial.println("Ready");
	Serial.println("Seconds, Cycle Count");
}

void flash(int pin, int onTime, int offTime){
	digitalWrite(pin, HIGH);
	delay(onTime);
	digitalWrite(pin, LOW);
	delay(offTime);
}

void output(int pin, int z, int onTime, int offTime){
	digitalWrite(pin, HIGH);
	digitalWrite(outputLEDArray[z], HIGH);
	delay(onTime);
	digitalWrite(pin, LOW);
	digitalWrite(outputLEDArray[z], LOW);
	delay(offTime);
}

void output_inverted(int pin, int z, int offTime, int onTime){
	digitalWrite(pin, LOW);
	digitalWrite(outputLEDArray[z], LOW);
	delay(offTime);
	digitalWrite(pin, HIGH);
	digitalWrite(outputLEDArray[z], HIGH);
	delay(onTime);
}

void printStats(){
	//Serial.print("Seconds: ")
	Serial.print(millis()/1000);
	Serial.print(", ");//Cycle Count: ")
	Serial.println(tortureCounter);
}

void loop(){
	//printStats();
	for (int z=0;z<6;z++){
		output_inverted(outputArray[z],z,relayOnTime,relayOffTime);
	}
	tortureCounter += 1;

	// make a string for assembling the data to log:
	String dataString = "";

	dataString += millis()/1000;
	dataString += ", ";
	dataString += tortureCounter;

	// open the file. note that only one file can be open at a time,
	// so you have to close this one before opening another.
	File dataFile = SD.open("log.txt", FILE_WRITE);

	if (dataFile) {								// if the file is available, write to it:
		dataFile.println(dataString);
		dataFile.close();
		// print to the serial port too:
		//Serial.println(dataString);
	}else {									// if the file isn't open, pop up an error:
		Serial.println("error opening log.txt");
	} 
	Serial.println(dataString);
}