/*
*-Enter
#-Stop
D-delete
C-restart
*/
#include <avr/wdt.h>  
#include <AccelStepper.h>
#include "Adafruit_Keypad.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Define the pins for stepper motor control
#define DIR_PIN_ 8
#define DIR_PIN 9
#define STEP_PIN_ 10
#define STEP_PIN 11
#define ENABLE_PIN_ 12
#define ENABLE_PIN 13

#define LIMIT_SIGNAL 48
#define LIMIT_GND 50
#define LIMIT_VCC 52

#define START_POSITION 1000
#define STOP_POSITION 10000
#define OFFSET_POSITION 200

#define MAX_SPEED 4000 // step

#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

uint8_t state = 4;        // 0 - calibrate  1 - run  2 - stop  3-setting
uint8_t settingState = 0; // 0-rpm   1-speed
int16_t destRound = 0, thisRound = 0;
uint8_t speed = 9;
uint16_t stepSpeed = MAX_SPEED;
bool updateOled = true, resume = false;

// Create an instance of AccelStepper
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

// Define the keypad rows and columns
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};
byte rowPins[ROWS] = {22, 24, 26, 28}; // connect to the row pinouts of the keypad
byte colPins[COLS] = {30, 32, 34, 36}; // connect to the column pinouts of the keypad

unsigned long previousMillis = 0;

// initialize an instance of class NewKeypad
Adafruit_Keypad customKeypad = Adafruit_Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void setup()
{
  Serial.begin(115200);

  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.clearDisplay();

  customKeypad.begin();

  pinMode(DIR_PIN_, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(STEP_PIN_, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(ENABLE_PIN_, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);

  digitalWrite(DIR_PIN_, LOW);
  digitalWrite(DIR_PIN, LOW);
  digitalWrite(STEP_PIN_, LOW);
  digitalWrite(STEP_PIN, LOW);
  digitalWrite(ENABLE_PIN_, LOW);
  digitalWrite(ENABLE_PIN, HIGH);

  pinMode(LIMIT_GND, OUTPUT);
  pinMode(LIMIT_VCC, OUTPUT);
  pinMode(LIMIT_SIGNAL, INPUT);
  digitalWrite(LIMIT_VCC, HIGH);
  digitalWrite(LIMIT_GND, LOW);

  //  calibrate();
}

void loop()
{

  if (state == 1)
  {

    // Run the stepper motor non-blocking
    if (stepper.distanceToGo() != 0)
    {
      stepper.run();
    }
    else
    {
      // Change direction when the motor has reached its target
      if (stepper.currentPosition() == START_POSITION)
      {
        thisRound++;
        if (thisRound == destRound)
          state = 2;
        updateOled = true;
        stepper.moveTo(STOP_POSITION);
      }
      else if (stepper.currentPosition() == STOP_POSITION)
      {
        stepper.moveTo(START_POSITION);
      }
      else
      {
        stepper.moveTo(STOP_POSITION);
      }
    }

    //    unsigned long currentMillis = millis();
    //    if (currentMillis - previousMillis >= 500) {
    //      previousMillis = currentMillis;
    //      Serial.println(stepper.currentPosition());
    //    }

    readStop();
    OledDisplay();
  }
  else if (state == 2)
  {
    stepper.stop();
    // Non-blocking keypad scan
    customKeypad.tick();
    while (customKeypad.available())
    {
      keypadEvent e = customKeypad.read();
      Serial.print((char)e.bit.KEY);
      if (e.bit.EVENT == KEY_JUST_PRESSED)
      {
        Serial.println(" pressed");
        char key = e.bit.KEY;

        if (key == '*')
        {
          if (thisRound == destRound)
          {
            thisRound = 0;
            state = 0;
          }
          else
          {
            state = 1;
            beforeRun();
          }
          updateOled = true;
        }
        else if (key == 'C')
        {
          state = 4;
          thisRound = 0;
          updateOled = true;
        }
        else if (key == 'D')
        {
          ArduinoReset();
        }
      }
    }

    OledDisplay();
  }
  else if (state == 0)
  {
    if (updateOled)
    {
      display.clearDisplay();
      display.setTextSize(2); // Draw 2X-scale text
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);
      display.print("Calibrate");
      display.display();

      updateOled = false;
    }
    digitalWrite(ENABLE_PIN, LOW);
    calibrate();
  }
  else if (state == 4)
  {
    settingFunc();
  }
}

void calibrate()
{

  // Set up the stepper motor
  stepper.setMaxSpeed(2000);     // Max speed in steps per second
  stepper.setAcceleration(1000); // Acceleration in steps per second^2
  stepper.moveTo(-20000);        // Move 1000 steps forward
  stepper.run();

  //  Serial.println(digitalRead(LIMIT_SIGNAL));
  Serial.print("Calibrating..");
  while (digitalRead(LIMIT_SIGNAL))
  {
    //    Serial.print(".");
    stepper.run();
  }
  setZero();
}

void beforeRun()
{
  pinMode(28, INPUT_PULLUP);
  digitalWrite(34, LOW);
  pinMode(34, OUTPUT);
}

void readStop()
{
  if (!digitalRead(28))
  {
    Serial.println(digitalRead(28));
    stepper.stop();
    state = 2;
  }
  if (!digitalRead(LIMIT_SIGNAL))
  {
    stepper.stop();
    setZero();
  }
}

void setZero()
{
  stepper.stop();
  stepper.setCurrentPosition(0);
  Serial.println("\nCalibrated!");
  state = 1;
  beforeRun();
  stepper.setCurrentPosition(0);
  Serial.println("Stepper position set to zero.");

  updateOled = true;

  setWorkingSpeed();
}

void setWorkingSpeed()
{
  // Set up the stepper motor
  stepper.setMaxSpeed(stepSpeed);     // Max speed in steps per second
  stepper.setAcceleration(MAX_SPEED); // Acceleration in steps per second^2

  // Start moving the stepper motor
  stepper.moveTo(STOP_POSITION); // Move 10000 steps forward
}

void settingFunc()
{
  if (updateOled)
  {
    display.clearDisplay();
    display.setTextSize(2); // Draw 2X-scale text
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);

    if (settingState == 0)
    {
      display.println(F("set Round"));
      display.setCursor(0, 28);
      display.println(destRound);
    }
    else
    {
      display.println(F("set speed"));
      display.setCursor(80, 25);
      display.setTextSize(1);
      display.println(F("(0-9)"));
      display.setTextSize(2);
      display.setCursor(0, 28);
      display.println(speed);
    }
    display.setCursor(80, 50);
    display.setTextSize(1);
    display.print("* Enter");

    display.display();

    updateOled = false;
  }

  customKeypad.tick();
  while (customKeypad.available())
  {
    keypadEvent e = customKeypad.read();

    if (e.bit.EVENT == KEY_JUST_PRESSED)
    {
      Serial.print((char)e.bit.KEY);
      Serial.println(" pressed");
      char key = e.bit.KEY;

      // If the key is a digit, add it to the inputString
      if (isdigit(key))
      {
        String inputString = "";
        inputString += key;
        if (settingState == 0)
        {
          if (destRound)
          {
            destRound = destRound * 10 + inputString.toInt();
          }
          else
          {
            destRound = inputString.toInt();
          }
          Serial.print("Converted Number: ");
          Serial.println(destRound);
          updateOled = true;
        }
        else if (settingState == 1)
        {
          speed = inputString.toInt();
          stepSpeed = 1000 + ((MAX_SPEED - 1000) * (speed + 1) / 10);
          Serial.println("speed: " + String(speed) + "\tstepSpeed: " + String(stepSpeed));
          updateOled = true;
        }
      }
      else if (key == '*')
      {
        if (settingState == 0)
        {
          settingState = 1;
          updateOled = true;
        }
        else if (settingState == 1)
        {
          state = 0;
        }
        updateOled = true;
      }
      else if (key == 'D')
      {
        Serial.println("Input cleared");
        if (settingState == 0)
          destRound = 0;
        else if (settingState == 1)
          speed = 0;

        updateOled = true;
      }
    }
  }
}

void OledDisplay()
{
  if (updateOled)
  {
    display.clearDisplay();
    display.setTextSize(2); // Draw 2X-scale text
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);

    if (thisRound < 0)
    {
      display.print("0");
    }
    else
    {
      display.print(thisRound);
    }

    display.print("/");
    display.println(destRound);
    display.setCursor(80, 50);
    display.setTextSize(1);
    display.println("# Stop");
    display.display();

    updateOled = false;
  }
}


void ArduinoReset()
{
  wdt_enable(WDTO_15MS);
  while (1)
  {
  }
}