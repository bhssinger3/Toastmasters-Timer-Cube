#include <SoftwareSerial.h>
#include <ClickEncoder.h>
#include <TimerOne.h>

// LED Pins
#define RED 8
#define YELLOW 9
#define GREEN 10
#define NULL 11 // Nothing on the pin, but only to be position zero in the arrray

// Define automatic time limits (in seconds)
const int tableTopics[3] = {60, 90, 120};
const int evaluation[3] = {120, 150, 180};
const int stdSpeech[3] = {300, 360, 420};
const int LEDArray[4] = {NULL, GREEN, YELLOW, RED};
const char* myStrings[]={"0000", "1--2", "2--3", "5--7"};

// Initiate Encoder
ClickEncoder *encoder;
int16_t last, value;

// These are the Arduino pins required to create a software serial
//  instance. We'll actually only use the TX pin.
const int softwareTx = 12;
const int softwareRx = 1; // Not in use, though
SoftwareSerial s7s(softwareRx, softwareTx);

unsigned int counter = 0;  // This variable will count up to 65k
char tempString[10];  // Will be used with sprintf to create strings

// Other variables
boolean Running = 0;
long startTime;

void timerIsr() {
  encoder->service();
}

void setup()
{ 
  // Create the lights
  pinMode(GREEN, OUTPUT);
  pinMode(YELLOW, OUTPUT);
  pinMode(RED, OUTPUT);
  digitalWrite(GREEN, LOW);
  digitalWrite(YELLOW, LOW);
  digitalWrite(RED, LOW);
 
  // Setup the rotary encoder
  Serial.begin(9600);
  encoder = new ClickEncoder(A2, A1, A0, 4);
  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr);   
  last = -1;
  
  // Setup the display over serial
  s7s.begin(9600);
  clearDisplay();  // Clears display
  setBrightness(255);  // Full brightness
  s7s.print("8888");  // Show all digit segments and...
  setDecimals(0b111111);  // Turn on all options

  // Set initial Running state
  Running = 0;

  // Turn off the lights
  showLED(0);

}

void loop()
{
  
  // Check state (running or not)
  if(Running)
  {
    setDecimals(0b00010000);
    
    long currentTime = millis();
    // Increment time
    // If manual mode, knob changes LED
    // If preset mode, use time presets
    
    // Button stops and changes to control mode
    ClickEncoder::Button b = encoder->getButton();
    if (b == ClickEncoder::Clicked) { Running = 0; }
    
    long accSecs = (currentTime - startTime) / 1000L;
    long minutes = accSecs / 60;
    long seconds = accSecs % 60;

    sprintf(tempString, "%02d%02d", (int)minutes, (int)seconds);
    //sprintf(tempString, "%04d", accSecs);
    s7s.print(tempString);
    
    // Just putting in raw code to manually light the lights
    value += encoder->getValue();
    if (value != last) 
    {
      int whichOne = abs(value)%4;
      last = value;
      showLED(whichOne);
      
      // Okay. This next bit is going to manually set the decimals
      // on the display to indicate which LED is on.
      // For now it's going to map the full bit stream for each.
      // The next version should just turn on/off the bits that need
      // to be changed each time.
      // Note: This block isn't working. I'm leaving it in for now though.
      switch(whichOne) {
        case 1: // Green
          setDecimals(0b00010100);
          break;
        case 2: // Yellow
          setDecimals(0b00010010);
          break;
        case 3: // Red
          setDecimals(0b00010001);
          break;
        default:
          setDecimals(0b00011000);
          break;
      } // switch(whichOne)
      
    } // if (value != last) 

  } else { // not running

    // Knob controls mode (manual, TT, EV, SS)
    // Load timer presets

    // Button starts timer
    ClickEncoder::Button b = encoder->getButton();
    if (b == ClickEncoder::Clicked) 
    { 
      Running = 1;
      startTime = millis();
    }
    // Resets on hold
    if (b == ClickEncoder::Held) 
    { 
      startTime = millis(); 
      s7s.print("0000");
    }  
  } // End if/else for not running

}

////////////////
// Functions //
///////////////

void showLED(int whichOne)
{
  for(int i=0; i<4; i++) {
    if(i == whichOne)
    {
      digitalWrite(LEDArray[i], HIGH);
    } 
    else
    {
      digitalWrite(LEDArray[i], LOW);
    }
  }
}

// Send the clear display command (0x76)
//  This will clear the display and reset the cursor
void clearDisplay()
{
  s7s.write(0x76);  // Clear display command
}

// Set the displays brightness. Should receive byte with the value
//  to set the brightness to
//  dimmest------------->brightest
//     0--------127--------255
void setBrightness(byte value)
{
  s7s.write(0x7A);  // Set brightness command byte
  s7s.write(value);  // brightness data byte
}

// Turn on any, none, or all of the decimals.
//  The six lowest bits in the decimals parameter sets a decimal 
//  (or colon, or apostrophe) on or off. A 1 indicates on, 0 off.
//  [MSB] (X)(X)(Apos)(Colon)(Digit 4)(Digit 3)(Digit2)(Digit1)
void setDecimals(byte decimals)
{
  s7s.write(0x77);
  s7s.write(decimals);
}

void origDispLoop()
{
  // Magical sprintf creates a string for us to send to the s7s.
  //  The %4d option creates a 4-digit integer.
  sprintf(tempString, "%4d", counter);

  // This will output the tempString to the S7S
  s7s.print(tempString);
  setDecimals(0b00000100);  // Sets digit 3 decimal on

  counter++;  // Increment the counter
  delay(100);  // This will make the display update at 10Hz.
}

void readButton()
{
  ClickEncoder::Button b = encoder->getButton();
  if (b != ClickEncoder::Open) {
    Serial.print("Button: ");
    #define VERBOSECASE(label) case label: Serial.println(#label); break;
    switch (b) {
      VERBOSECASE(ClickEncoder::Pressed);
      VERBOSECASE(ClickEncoder::Held)
      VERBOSECASE(ClickEncoder::Released)
      VERBOSECASE(ClickEncoder::Clicked)
      case ClickEncoder::DoubleClicked:
          Serial.println("ClickEncoder::DoubleClicked");
          encoder->setAccelerationEnabled(!encoder->getAccelerationEnabled());
          Serial.print("  Acceleration is ");
          Serial.println((encoder->getAccelerationEnabled()) ? "enabled" : "disabled");
        break;
    } // switch (b) {
  } // if (b != ClickEncoder::Open) {
}
