#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <AD9850SPI.h>

#define SCREEN_WIDTH 128 // OLED display width,  in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define CLK 2
#define DT 3
#define SW 4
#define LED 13

#define W_CLK_PIN 10
#define FQ_UD_PIN 9
#define RESET_PIN 8

//Adafruit_SSD1306 display(-1);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

int shift_min = 10;
int shift_max = 900;
int shift = shift_min;
int phase = 0;
long p_dur;
float freq1_min = 20.00;
float freq1_max = 25.00;
float freq1 = freq1_min;
float freq2;
float freq3;
float freq4;
int bandwidth_min = 10;
int bandwidth_max = 200;
int bandwidth = bandwidth_min;

int counter = 0;
int timeDelay_min = 100;
int timeDelay_max = 1000;
int timeDelay = timeDelay_min;
int timeStep_min = 1;
int timeStep_max = 5;
int timeStep = timeStep_min;

int time_step = 1;
int currentStateCLK;
int lastStateCLK;
String currentDir = "";

int ButtonState;
byte press;
bool buttonReleased = true;
bool lastButtonState;
bool longButtonPress = false;
unsigned long lastButtonPress = 0;

int x2 = 4;
int x1 = 17;
int y1 = 26;
int z1 = 35;
int t1 = 44;
int k1 = 53;
int frame = 0;

const int LONG_PRESS_TIME  = 700;
int lastState = LOW;  // the previous state from the input pin
int currentState;     // the current reading from the input pin
unsigned long pressedTime  = 0;
unsigned long releasedTime = 0;

String before1[4];
byte after[4];
byte before[] = {B1000, B0100, B0010, B0001};
uint8_t array_size = sizeof(before) / sizeof(before[0]);

void printByteDisplay(byte aByte, byte nbBits = 8, int a=0, int b=0) { // nbBits is how many bits you want to print starting from LSb
  if (nbBits == 0) return; // nothing to print
  for (int i = 0 ; i <= nbBits - 1; i++) {
    display.setCursor(a, b+i*9);
    display.write(bitRead(aByte, nbBits - 1-i) == 0 ? '0' : '1');
  }
}

void setup() {
  // Setup Serial Monitor
  Serial.begin(9600);
  while (!Serial);
  //DDS.begin(W_CLK_PIN, FQ_UD_PIN, RESET_PIN);
  //DDS.calibrate(124999500);
  // initialize with the I2C addr 0x3C
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }

  // Clear the buffer.
  display.clearDisplay();

  // Set encoder pins as inputs
  pinMode(CLK, INPUT_PULLUP);
  pinMode(DT, INPUT_PULLUP);
  pinMode(SW, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  //digitalWrite(12, HIGH);

  randomSeed(analogRead(0));
  for (int i = 0; i < 4; i++) {

    // ---Pick a random array element.---
    uint8_t pick = random(0, array_size);

    // ---Assign the values to after[] array---
    after[i] = before[pick];

    // ---Overwrite with the last element of the array.---
    before[pick] = before[array_size - 1];

    // ---Reduce the array size to be selected.---
    array_size--;
    digitalWrite(LED, HIGH);
  }
  // Read the initial state of CLK
  lastStateCLK = digitalRead(CLK);

  // Call updateEncoder() when any high/low changed seen
  // on interrupt 0 (pin 2), or interrupt 1 (pin 3)
  attachInterrupt(0, updateEncoder, CHANGE);
  attachInterrupt(1, updateEncoder, CHANGE);
  PCICR |=B00000100;
  PCMSK2 |=B00010000;
}

void loop() {

  //sweep(100);
  //DDS.down();
  //delayMicroseconds(timeDelay);
  //DDS.up();
  
  freq2 = freq1 + 0.001*(shift+bandwidth);
  freq3 = freq1 + 2 * 0.001*(shift+bandwidth);
  freq4 = freq1 + 3 * 0.001*(shift+bandwidth);

  //button();

  for (int i = 0; i < 4; i++) {
    //Serial.println(i);
      if(after[i]==B0001){
      sweep(freq1*pow(10,6));
      DDS.down();
      delayMicroseconds(timeDelay);
      DDS.up();
      }
      else if(after[i]==B0010){
      sweep(freq2*pow(10,6));
      DDS.down();
      delayMicroseconds(timeDelay);
      DDS.up();
      }
      else if(after[i]==B0100){
      sweep(freq3*pow(10,6));
      DDS.down();
      delayMicroseconds(timeDelay);
      DDS.up();
      }
      else {//if(after[i]==B1000)
      sweep(freq4*pow(10,6));
      DDS.down();
      delayMicroseconds(timeDelay);
      DDS.up();
      }    
  }
  //Serial.println(btn);
  frame = counter;
  if (frame < 0) {
    frame = abs(frame % 5);
    if (frame == 1) frame = 5;
    else if (frame == 2) frame = 4;
    else if (frame == 3) frame = 3;
    else if (frame == 4) frame = 2;
    //else if (frame == 5) frame = 2;
    else if (frame == 0) frame = 1;
  }
  else if (frame < 0 && frame % 5 == 0) frame = 1;
  else if (frame > 5 && frame % 5 != 0) frame = frame % 5;
  else if (frame > 5 && frame % 5 == 0) frame = 5;

  if (buttonReleased) {
    switch (frame) {
      case 0:
        frame0();
        break;
      case 1:
        frame1();
        break;
      case 2:
        frame2();
        break;
      case 3:
        frame3();
        break;
      case 4:
        frame4();
        break;
      case 5:
        frame5();
        break;
    }
  }
  else if (!buttonReleased) {
    switch (frame) {
      case 0:
        buttonReleased = true;
        break;
      case 1:
        subFrame1();
        break;
      case 2:
        subFrame2();
        //value = timeDelay;
        break;
      case 3:
        subFrame3();
        break;
      case 4:
        subFrame4();
        break;
      case 5:
        subFrame5();
        break;
    }
  }
}

void frame0() {
  header();
  display.setCursor(0, x1);  display.print (F(" Generated matrix"));
  display.setCursor(0, y1);  display.print (F(" Time delay/step"));
  //display.setCursor(0, z1);  display.print (F(" Time step"));
  display.setCursor(0, z1);  display.print (F(" Frequency"));
  display.setCursor(0, t1);  display.print (F(" Bandwidth/Shift"));
  display.setCursor(0, k1);  display.print (F(" About project"));
  refresh();
}

void frame1()
{
  header();
  display.setCursor(0, x1);  display.print (F(">Generated matrix"));
  display.setCursor(0, y1);  display.print (F(" Time delay/step"));
  //display.setCursor(0, z1);  display.print (F(" Time step"));
  display.setCursor(0, z1);  display.print (F(" Frequency"));
  display.setCursor(0, t1);  display.print (F(" Bandwidth/Shift"));
  display.setCursor(0, k1);  display.print (F(" About project"));
  refresh();
}

void frame2()
{
  header();
  display.setCursor(0, x1);  display.print (F(" Generated matrix"));
  display.setCursor(0, y1);  display.print (F(">Time delay/step"));
  //display.setCursor(0, z1);  display.print (F(" Time step"));
  display.setCursor(0, z1);  display.print (F(" Frequency"));
  display.setCursor(0, t1);  display.print (F(" Bandwidth/Shift"));
  display.setCursor(0, k1);  display.print (F(" About project"));
  refresh();
}

void frame3()
{
  header();
  display.setCursor(0, x1);  display.print (F(" Generated matrix"));
  display.setCursor(0, y1);  display.print (F(" Time delay/step"));
  //display.setCursor(0, z1);  display.print (F(">Time step"));
  display.setCursor(0, z1);  display.print (F(">Frequency"));
  display.setCursor(0, t1);  display.print (F(" Bandwidth/Shift"));
  display.setCursor(0, k1);  display.print (F(" About project"));
  refresh();
}

void frame4()
{
  header();
  display.setCursor(0, x1);  display.print (F(" Generated matrix"));
  display.setCursor(0, y1);  display.print (F(" Time delay/step"));
  //display.setCursor(0, z1);  display.print (F(" Time step"));
  display.setCursor(0, z1);  display.print (F(" Frequency"));
  display.setCursor(0, t1);  display.print (F(">Bandwidth/Shift"));
  display.setCursor(0, k1);  display.print (F(" About project"));
  refresh();
}

void frame5()
{
  header();
  display.setCursor(0, x1);  display.print (F(" Generated matrix"));
  display.setCursor(0, y1);  display.print (F(" Time delay/step"));
  //display.setCursor(0, z1);  display.print (F(" Time step"));
  display.setCursor(0, z1);  display.print (F(" Frequency"));
  display.setCursor(0, t1);  display.print (F(" Bandwidth/Shift"));
  display.setCursor(0, k1);  display.print (F(">About project"));
  refresh();
}
/*
void subFrame1()
{
  display.setTextSize(1);
  display.setCursor(0, 2);  display.print (F("  Generated matrix"));
  display.drawRoundRect(5, 0, 108, 12, 3, WHITE);
  display.setTextSize(1.5);
  for (int element : after)
    display.setCursor(0, x1 + 5); printByteDisplay(after[0], 4);
  display.setCursor(0, y1 + 5); printByteDisplay(after[1], 4);
  display.setCursor(0, z1 + 5); printByteDisplay(after[2], 4);
  display.setCursor(0, t1 + 5); printByteDisplay(after[3], 4);
  refresh();
}
*/
void subFrame1()
{
  display.setTextSize(1);
  display.setCursor(0, 2);  display.print (F("  Generated matrix"));
  display.drawRoundRect(5, 0, 108, 12, 3, WHITE);
  display.setTextSize(1.5);
  for (int element : after)
  printByteDisplay(after[0], 4,0,x1+5);
  printByteDisplay(after[1], 4,5,x1+5);
  printByteDisplay(after[2], 4,10,x1+5);
  printByteDisplay(after[3], 4,15,x1+5);
  refresh();
}

void subFrame2()
{
  display.setTextSize(1);
  display.setCursor(0, 2);  display.print (F("  Time delay/step"));
  display.drawRoundRect(5, 0, 108, 12, 3, WHITE);
  display.setTextSize(1);
  display.setCursor(0, x1 + 1);  display.print (F("       Choose:"));
  display.setCursor(0, y1+1);  display.print (F("Delay"));
  display.setCursor(75, y1+1);  display.print (F("Step"));
  display.drawRoundRect(5, 0, 108, 12, 3, WHITE);
  display.setTextSize(2);
  display.setCursor(0, z1 + 4);  display.print (timeDelay);
  display.setCursor(75, z1 + 4);  display.print (time_step);
  display.setTextSize(1);
  display.setCursor(0, k1 + 3);  display.print (F("ms"));
  display.setCursor(75, k1 + 3);  display.print (F("ms"));
  refresh();
}

void subFrame3()
{
  display.setTextSize(1);
  display.setCursor(0, 2);  display.print (F("     Frequency"));
  display.drawRoundRect(5, 0, 108, 12, 3, WHITE);
  display.setTextSize(1);
  display.setCursor(0, x1);  display.print (F("    Choose f1:"));
  display.setTextSize(2);
  display.setCursor(0, y1 + 4);  display.print (freq1);
  display.setTextSize(1);
  display.setCursor(60, y1 + 4);  display.print (F("f2="));
  display.setCursor(80, y1 + 4);  display.print (freq2);
  display.setCursor(60, z1 + 4);  display.print (F("f3="));
  display.setCursor(80, z1 + 4);  display.print (freq3);
  display.setCursor(60, t1 + 4);  display.print (F("f4="));
  display.setCursor(0, t1 + 4);  display.print (F("MHz"));
  display.setCursor(80, t1 + 4);  display.print (freq4);
  refresh();
}

void subFrame4()
{
  display.setTextSize(1);
  display.setCursor(0, 2);  display.print (F("   Bandwidth/Shift"));
  display.setCursor(0, x1 + 1);  display.print (F("       Choose:"));
  display.setCursor(0, y1+1);  display.print (F("Bandwidth"));
  display.setCursor(75, y1+1);  display.print (F("Shift"));
  display.drawRoundRect(5, 0, 108, 12, 3, WHITE);
  display.setTextSize(2);
  display.setCursor(0, z1 + 4);  display.print (bandwidth);
  display.setCursor(75, z1 + 4);  display.print (shift);
  display.setTextSize(1);
  display.setCursor(0, k1 + 3);  display.print (F("kHz"));
  display.setCursor(75, k1 + 3);  display.print (F("kHz"));
  refresh();
}
void subFrame5()
{
  display.setTextSize(1);
  display.setCursor(0, 2);  display.print (F("    About project"));
  display.drawRoundRect(5, 0, 108, 12, 3, WHITE);
  //display.setCursor(0, x1 + 1);  display.print (F("       Choose:"));
  display.setCursor(0, x1+1);  display.print (F("This device generatesrandom SF-LFM pulses for radar"));
  refresh();
}
void header() {
  display.setTextSize(1);          // text size
  display.setTextColor(WHITE);     // text color
  display.setCursor(0, 0);        // position to display
  display.println("Developed by Mirana"); // text to display
}
//ISR (PCINT2_vect){button();}

//void button(){
  ISR (PCINT2_vect){
  currentState = digitalRead(SW);

  if(lastState == HIGH && currentState == LOW)        // button is pressed
    pressedTime = millis();
  else if(lastState == LOW && currentState == HIGH) { // button is released
    releasedTime = millis();

    long pressDuration = releasedTime - pressedTime;

    if( pressDuration < LONG_PRESS_TIME || press==0) {
      Serial.println("A short press is detected");
buttonReleased = ! buttonReleased;
longButtonPress=0;
}
else if( pressDuration > LONG_PRESS_TIME && press!=0){
      Serial.println("A long press is detected");
      //Serial.println(longButtonPress);
longButtonPress = ! longButtonPress;
//if(press>1) press=1;
}
press++;
if(press>1) press=1;
Serial.println(press);
  }

  // save the the last state
  lastState = currentState;
    
}

void updateEncoder() {

  // Read the current state of CLK
  currentStateCLK = digitalRead(CLK);

  // If last and current state of CLK are different, then pulse occurred
  // React to only 1 state change to avoid double count
  if (currentStateCLK != lastStateCLK && lastStateCLK==1) {
/*&& currentStateCLK == 1*/
    // If the DT state is different than the CLK state then
    // the encoder is rotating CCW so decrement
    if (digitalRead(DT) != currentStateCLK && buttonReleased) {
      //if(lastStateCLK!=0)
      counter --;
      currentDir = "CCW";
      //else if(lastStateCLK==0) counter-=2; //dec=true;
    } else if (digitalRead(DT) == currentStateCLK && buttonReleased ) {
      // Encoder is rotating CW so increment
      //if(lastStateCLK!=0)
      counter ++;
      currentDir = "CW";
      //else if(lastStateCLK==0) counter +=2; //inc=true;
    }
    else if (digitalRead(DT) != currentStateCLK && frame == 2&&!buttonReleased&&!longButtonPress) {
      timeDelay = timeDelay + time_step;
      if (timeDelay > timeDelay_max) timeDelay = timeDelay_max;
    }
    else if (digitalRead(DT) == currentStateCLK && frame == 2&&!buttonReleased&&!longButtonPress) {
      timeDelay = timeDelay - time_step;
      if (timeDelay < timeDelay_min) timeDelay = timeDelay_min;
    }
    else if (digitalRead(DT) != currentStateCLK && frame == 2&& longButtonPress) {
      timeStep++;
      if (timeStep > timeStep_max) timeStep = timeStep_max;
      time_stp();
    }
    else if (digitalRead(DT) == currentStateCLK && frame == 2&& longButtonPress) {
      timeStep--;
      if (timeStep < timeStep_min) timeStep = timeStep_min;
      time_stp();
    }

    else if (digitalRead(DT) != currentStateCLK && frame == 3) {
      //freq1=freq1+0.001*shift;
      freq1++;
      if (freq1 > freq1_max) freq1 = freq1_max;
    }
    else if (digitalRead(DT) == currentStateCLK && frame == 3) {
      //freq1=freq1+0.001*shift;
      freq1--;
      if (freq1 < freq1_min) freq1 = freq1_min;
    }
    else if (digitalRead(DT) != currentStateCLK && frame == 4&&!buttonReleased&&!longButtonPress) {
      bandwidth += 10;
      if (bandwidth > bandwidth_max) bandwidth = bandwidth_max;
    }
    else if (digitalRead(DT) == currentStateCLK && frame == 4&&!buttonReleased&&!longButtonPress) {
      bandwidth -= 10;
      if (bandwidth < bandwidth_min) bandwidth = bandwidth_min;

    }
    else if (digitalRead(DT) != currentStateCLK && frame == 4 && longButtonPress) {
      shift+=10;
      if (shift > shift_max) shift = shift_max;
    }
    else if (digitalRead(DT) == currentStateCLK && frame == 4 && longButtonPress) {
      shift-=10;
      if (shift < shift_min) shift = shift_min;
    }

    //Serial.print("Direction: "));
    //Serial.print(currentDir);
    //Serial.print(" | Counter: "));
    //Serial.println(counter);
  }

  // Remember last CLK state
  lastStateCLK = currentStateCLK;
}

void refresh()
{
  display.display();
  delay(00);
  display.clearDisplay();
}

void time_stp() {
  switch (timeStep) {
    case 1:
      time_step = 1;
      break;
    case 2:
      time_step = 5;
      break;
    case 3:
      time_step = 10;
      break;
    case 4:
      time_step = 50;
      break;
    case 5:
      time_step = 100;
      break;
  }
}

void sweep(unsigned long freq) {
  //Serial.println(freq);
  //Serial.println(freq+bandwidth*pow(10,3));
  for (long i =freq; i<= freq+bandwidth*pow(10,3); i+=100) {
    DDS.setfreq(freq, phase);
  }
}

