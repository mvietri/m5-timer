#include <Arduino.h>
#include <M5Stack.h>
#include <FastLED.h>

// Free Fonts for nice looking fonts on the screen
#include "Free_Fonts.h"

#define M5STACK_FIRE_NEO_NUM_LEDS 10
#define M5STACK_FIRE_NEO_DATA_PIN 15

// Define the array of leds
CRGB leds[M5STACK_FIRE_NEO_NUM_LEDS]; 

// Global variables 
unsigned int bright = 100;  // default
unsigned int bright_leds = 5;  // default
unsigned int led_status = 0;

int idle = 0; // seconds in idle
int shutdown_timeout = 20; // seconds to begin shutdown sequence
int shutdown_time = 10; // seconds left to show warning (shutdown total time is combined (timeout + time))

unsigned long previousMillis = 0;
const long interval = 1000;   
int state = 0;
bool displayTimer = false;

int minutes = 0;
int seconds = 10;

bool isAlarmOff = false;

void setLedState(int newState);

void setup() {  
  // load these before M5.begin() so they can eventually be turned off
  FastLED.addLeds<WS2812B, M5STACK_FIRE_NEO_DATA_PIN, GRB>(leds, M5STACK_FIRE_NEO_NUM_LEDS);
  FastLED.clear();
  FastLED.show(); 

  // Initialize the M5Stack object
  M5.begin();
  Wire.begin(); 

  /*
    Power chip connected to gpio21, gpio22, I2C device
    Set battery charging voltage and current
    If used battery, please call this function in your project
  */
  M5.Power.begin();

  // Set up and turn off the speaker output to avoid most of the anoying sounds
  pinMode(25, OUTPUT);
  M5.Speaker.mute(); 

  M5.Lcd.fillScreen(BLACK);

  Serial.begin(9600);
  Serial.println("Starting up");
}

void loop() {      
  // put your main code here, to run repeatedly:
  unsigned long currentMillis = millis(); 
  M5.update();

  bool adjusting = false;

  if (M5.BtnC.wasPressed()) {
    Serial.println("C pressed");
 
    // start/stop the timer
    switch (state)
    {
      case 0:
        state = 1;
        break;
      case 1:
      case 2:
        isAlarmOff = false;
        state = 0;
        break;
    } 
  }

  if (state == 0) {
    if (M5.BtnA.pressedFor(500)){
      minutes = 0;
      adjusting = true;
      idle = 0;
    }

    if (M5.BtnA.wasPressed()) {
      Serial.println("A pressed");

      // add minutes
      minutes++;
      adjusting = true;
      idle = 0;
    }

    if (M5.BtnB.pressedFor(500)){
      seconds = 0;
      adjusting = true;
      idle = 0;
    }

    if (M5.BtnB.wasPressed()) {
      Serial.println("B pressed");

      // add seconds
      seconds++;
      adjusting = true;
      idle = 0;

      if (seconds > 59) {
        seconds = 0;
        minutes++;
      }
    }
  }

  if ((currentMillis - previousMillis >= interval) || adjusting) {
    M5.Lcd.clear();

    // a second has passed
    previousMillis = currentMillis; 

    if (state == 0 || state == 2) {
        //blink mode
        displayTimer = (!displayTimer || adjusting);
        idle++;

        M5.Lcd.setTextColor(WHITE);
        M5.Lcd.setTextSize(2);
        
        if (state == 0) {
          M5.Lcd.setCursor(45, 225);
          M5.Lcd.print("MIN+");
          
          M5.Lcd.setCursor(142, 225);
          M5.Lcd.print("SEC+");
          
          M5.Lcd.setTextSize(1);
          M5.Lcd.setCursor(240, 220);
          M5.Lcd.print("START");
          M5.Lcd.setCursor(240, 232);
          M5.Lcd.print("PAUSE");
        }

        if (state == 2) {          
          M5.Lcd.setCursor(225, 225);
          M5.Lcd.print("RESET");
        }
    } else {
        displayTimer = true;
        idle = 0;

        // timer is running 
        Serial.println("Tick");
        seconds--;

        if (seconds <= 0) {
          if (minutes <= 0) {
            // finish
            state = 2;
            seconds = 0;
            minutes = 0;
          } else {
            seconds = 59;
            minutes--;
          }
        }
    }

    if (idle >= shutdown_timeout) {
      if (idle >= (shutdown_timeout + shutdown_time)) {
          setLedState(0);
          M5.Power.powerOFF();
      } else {
          M5.Lcd.setTextSize(1); 
          M5.Lcd.setCursor(1, 1);
          M5.Lcd.setTextColor(ORANGE); 
          M5.Lcd.printf("Powering off in %02d...", ((shutdown_timeout + shutdown_time) - idle));
      }
    }
    
    if (displayTimer) {
      if (state == 2) {
        M5.Lcd.setTextColor(GREEN);

        if (!isAlarmOff) {
          M5.Speaker.tone(900, 2000);
          isAlarmOff = true;
        }

        setLedState(2);
      } else {
        if (state == 1 && (minutes == 0 && seconds <= 10)) {
          M5.Lcd.setTextColor(RED);
          M5.Speaker.beep();
          if (seconds <= 3) {
            delay(250);
            M5.Speaker.beep(); // another for the last 3 secs
          } 
          setLedState(1);
        } else { 
          if (adjusting)
            M5.Lcd.setTextColor(YELLOW);
          else
            M5.Lcd.setTextColor(WHITE);
          setLedState(0);
        }
      }

      M5.Lcd.setTextSize(15); 
      M5.Lcd.setCursor(55, 90);
      M5.Lcd.printf("%02d:%02d", minutes, seconds);
    }
  } 
}

void setLedState(int newState) {
  if (led_status != newState) {
    
    led_status = newState;

    int R = 0, G = 0, B = 0;

    switch (led_status)
    {
      case 1:
        R = bright_leds;
        break; 
      case 2:
        G = bright_leds;
        break;
    }
    
    for (int pixelNumber = 0; pixelNumber < 10; pixelNumber++)
      leds[pixelNumber].setRGB(R, G, B);  

    FastLED.show();
  }
}