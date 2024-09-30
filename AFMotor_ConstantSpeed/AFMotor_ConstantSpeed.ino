/*
  -Enter
  #-Stop
  D-delete
  C-restart

  1000 step  = 13mm
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

#define START_POSITION 0
#define STOP_POSITION 1000
#define OFFSET_POSITION 500

#define MAX_SPEED 4000 // step

#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

uint8_t state = 3;        // 0-calibrate  1-run  2-stop  3-setting
uint8_t settingState = 0; // 0-rpm   1-speed   2-span
int16_t destRound = 0, thisRound = 0;

uint8_t span = 0;
uint8_t speed = 9;
uint16_t stepSpeed = MAX_SPEED;
uint32_t spanStep;
int16_t togo = -1;
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

  if (state == 1) // run
  {
    if (resume)
    {
      stepper.moveTo(togo);
      resume = false;
      Serial.println("resume");
    }
    // Change direction when the motor has reached its target
    else if (!digitalRead(LIMIT_SIGNAL))
    {
      stepper.stop();
      stepper.setCurrentPosition(0);
      thisRound++;
      if (thisRound == destRound)
        state = 2;

      updateOled = true;
      stepper.moveTo(spanStep);
      togo = spanStep;
    }
    // Run the stepper motor non-blocking
    else if (stepper.distanceToGo() != 0)
    {
      stepper.run();
    }

    else if (stepper.currentPosition() == START_POSITION)
    {
      stepper.move(-OFFSET_POSITION);
    }
    else if (stepper.currentPosition() == spanStep)
    {
      stepper.moveTo(START_POSITION - OFFSET_POSITION);
      togo = START_POSITION;
    }
    else if (spanStep >= 0)
    {
      stepper.moveTo(togo);
    }
    else
    {
      stepper.moveTo(spanStep);
      togo = spanStep;
    }

    //    unsigned long currentMillis = millis();
    //    if (currentMillis - previousMillis >= 500) {
    //      previousMillis = currentMillis;
    //      Serial.println(stepper.currentPosition());
    //    }

    readStop();
  }
  else
  {
    customKeypad.tick();
    while (customKeypad.available())
    {
      // Non-blocking keypad scan
      keypadEvent e = customKeypad.read();
      Serial.print((char)e.bit.KEY);
      char key;
      if (e.bit.EVENT == KEY_JUST_PRESSED)
      {
        Serial.println(" pressed");
        key = e.bit.KEY;

        if (key == 'A')
        {
          display.clearDisplay();
          display.setTextSize(2); // Draw 2X-scale text
          display.setTextColor(SSD1306_WHITE);
          display.setCursor(0, 0);
          display.print("Restart");
          display.display();
          ArduinoReset();
        }

        if (state == 2) // stop
        {
          if (key == '*')
          {
            if (thisRound == destRound)
            {
              thisRound = 0;
              state = 0;
            }
            else
            {
              resume = true;
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
        }
      }

      else if (state == 3) // setting
      {
        if (isdigit(key))
        {
          String inputString = "";
          inputString += key;
          if (settingState == 0) // round
          {
            if (destRound)
            {
              destRound = destRound * 10 + inputString.toInt();
            }
            else
            {
              destRound = inputString.toInt();
            }
            Serial.println("Round: " + String(destRound));
            updateOled = true;
          }
          else if (settingState == 1) // speed
          {
            speed = inputString.toInt();
            stepSpeed = 1000 + ((MAX_SPEED - 1000) * (speed + 1) / 10);
            Serial.println("Speed: " + String(speed) + "\tStepSpeed: " + String(stepSpeed));
            updateOled = true;
          }
          else if (settingState == 2) // span
          {
            if (span)
            {
              span = span * 10 + inputString.toInt();
            }
            else
            {
              span = inputString.toInt();
            }
            spanStep = span * 77;
            // spanStep /= 13;
            Serial.println("Span: " + String(span) + "\tSpanStep: " + String(spanStep));
            // Serial.println(spanStep);
            updateOled = true;
          }
        }
        else if (key == '*') // next
        {
          if (settingState == 0)
          {
            settingState = 1;
          }
          else if (settingState == 1)
          {
            settingState = 2;
          }
          else if (settingState == 2)
          {
            state = 0;
          }
          updateOled = true;
        }
        else if (key == '#') // back
        {
          if (settingState == 1)
          {
            settingState = 0;
          }
          else if (settingState == 2)
          {
            settingState = 1;
          }
          updateOled = true;
        }
        else if (key == 'D') // delete
        {
          Serial.println("Input cleared");
          if (settingState == 0)
            destRound = 0;
          else if (settingState == 1)
            speed = 0;
          else if (settingState == 2)
          {
            span = 0;
            spanStep = 0;
          }

          updateOled = true;
        }
      }
    }

    if (state == 2) // stop
    {
      stepper.stop();
    }
    else if (state == 0)
    {
      digitalWrite(ENABLE_PIN, LOW);
      calibrate();
    }
  }

  displayOled();
}

void calibrate()
{

  // Set up the stepper motor
  stepper.setMaxSpeed(2000);     // Max speed in steps per second
  stepper.setAcceleration(1000); // Acceleration in steps per second^2
  stepper.moveTo(-20000);        // Move 1000 steps forward
  stepper.run();

  display.clearDisplay();
  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Calibrating");
  display.display();

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
    updateOled = true;
  }
  // if (!digitalRead(LIMIT_SIGNAL))
  // {
  //   stepper.stop();
  //   // setZero();
  //   stepper.setCurrentPosition(0);
  //   stepper.moveTo(spanStep); // Move 10000 steps forward
  //   togo = spanStep;
  //   updateOled = true;
  // }
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
  stepper.setMaxSpeed(stepSpeed);         // Max speed in steps per second
  stepper.setAcceleration(MAX_SPEED * 2); // Acceleration in steps per second^2

  // Start moving the stepper motor
  stepper.moveTo(spanStep); // Move 10000 steps forward
  togo = spanStep;
}

void ArduinoReset()
{
  wdt_enable(WDTO_15MS);
  while (1)
  {
  }
}

void displayOled()
{
  if (!updateOled)
    return;

  display.clearDisplay();
  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);

  if (state == 1) // run
  {
    display.setCursor(0, 0);
    display.println("RUN");

    display.setCursor(0, 20);
    if (thisRound < 0)
    {
      display.print("0");
    }
    else
    {
      display.print(thisRound);
    }

    display.print("/");
    display.print(destRound);
    display.setTextSize(1);
    display.println(" [" + String(spanStep) + "]");
    display.setCursor(80, 50);
    // display.setTextSize(1);
    display.println("# Stop");
  }
  else if (state == 2) //  stop
  {
    display.setCursor(0, 0);
    display.println("STOP");

    display.setCursor(0, 20);
    if (thisRound < 0)
    {
      display.print("0");
    }
    else
    {
      display.print(thisRound);
    }

    display.print("/");
    display.print(destRound);
    display.setTextSize(1);
    display.println(" [" + String(spanStep) + "]");
    display.setCursor(0, 50);
    // display.setTextSize(1);
    display.println("A Reset    * Resume");
  }
  else if (state == 3)
  {
    display.setCursor(0, 0);

    if (settingState == 0)
    {
      display.println(F("Round"));
      display.setCursor(0, 28);
      display.println(destRound);
    }
    else if (settingState == 1)
    {
      display.println(F("Speed(0-9)"));
      // display.setCursor(80, 25);
      // display.setTextSize(1);
      // display.println(F(""));
      // display.setTextSize(2);
      display.setCursor(0, 28);
      display.println(speed);
    }
    else
    {
      display.println(F("Span[cm]"));
      display.setTextSize(2);
      display.setCursor(0, 28);
      display.println(span);
    }
    display.setCursor(80, 50);
    display.setTextSize(1);
    display.print("* Enter");
  }

  display.display();
  updateOled = false;
}
