// Simple program to test all the LEDs on the Hauntbox. Not terriby useful past that.

int timer = 100;           // The higher the number, the slower the timing.
int ledPins[] = { 32,33,34,35,36,42,43,44,45,46,47,39 };       // an array of pin numbers to which LEDs are attached
int ledPinsShifted[] = { 39,32,33,34,35,36,42,43,44,45,46,47 };       // shifed for animation

int pinCount = 12;           // the number of pins (i.e. the length of the array)

void setup() {

pinMode(30, OUTPUT);
pinMode(31, OUTPUT);
pinMode(32, OUTPUT);
pinMode(33, OUTPUT);
pinMode(34, OUTPUT);
pinMode(35, OUTPUT);
pinMode(36, OUTPUT);
pinMode(37, OUTPUT);
pinMode(38, OUTPUT);
pinMode(39, OUTPUT);
pinMode(40, OUTPUT);
pinMode(41, OUTPUT);
pinMode(42, OUTPUT);
pinMode(43, OUTPUT);
pinMode(44, OUTPUT);
pinMode(45, OUTPUT);
pinMode(46, OUTPUT);
pinMode(47, OUTPUT);
pinMode(48, OUTPUT);
pinMode(49, OUTPUT);
pinMode(50, OUTPUT);
pinMode(51, OUTPUT);
pinMode(52, OUTPUT);
pinMode(53, OUTPUT);

}

void loop() {
   for (int thisPin = 0; thisPin < pinCount; thisPin++) { 
    // turn the pin on:
    digitalWrite(ledPins[thisPin], HIGH);
    delay(timer);                  
    // turn the pin off:
    digitalWrite(ledPins[thisPin], LOW);    

  }
}
