#include <AccelStepper.h>
#include "Adafruit_Keypad.h"

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
#define STOP_POSITION 12000
#define OFFSET_POSITION 200


#define MAX_SPEED 4000

uint8_t state = 0;     // 0 - calibrate 1 - run 2 - stop

// Create an instance of AccelStepper
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

// Define the keypad rows and columns
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {22, 24, 26, 28}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {30, 32, 34, 36}; //connect to the column pinouts of the keypad

unsigned long previousMillis = 0;

//initialize an instance of class NewKeypad
Adafruit_Keypad customKeypad = Adafruit_Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void setup() {
  Serial.begin(115200);

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
  digitalWrite(ENABLE_PIN, LOW);

  pinMode(LIMIT_GND, OUTPUT);
  pinMode(LIMIT_VCC, OUTPUT);
  pinMode(LIMIT_SIGNAL, INPUT);
  digitalWrite(LIMIT_VCC, HIGH);
  digitalWrite(LIMIT_GND, LOW);

  //  calibrate();
}

void loop() {

  if (state == 1) {
    // Run the stepper motor non-blocking
    if (stepper.distanceToGo() != 0) {
      stepper.run();
    } else {
      // Change direction when the motor has reached its target
      if (stepper.currentPosition() == START_POSITION) {
        stepper.moveTo(STOP_POSITION);
      } else if (stepper.currentPosition() == STOP_POSITION ) {
        stepper.moveTo(START_POSITION);
      } else {
        stepper.moveTo(STOP_POSITION);
      }
    }

    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= 500) {
      previousMillis = currentMillis;

      Serial.println(stepper.currentPosition());

    }

    readStop();
  }
  else if (state == 2) {
    stepper.stop();
    // Non-blocking keypad scan
    customKeypad.tick();
    while (customKeypad.available()) {
      keypadEvent e = customKeypad.read();
      Serial.print((char)e.bit.KEY);
      if (e.bit.EVENT == KEY_JUST_PRESSED) Serial.println(" pressed");
      else if (e.bit.EVENT == KEY_JUST_RELEASED) Serial.println(" released");
    }
  }
  else if (state == 0) {
    calibrate();
  }



}

void calibrate() {

  // Set up the stepper motor
  stepper.setMaxSpeed(2000);     // Max speed in steps per second
  stepper.setAcceleration(1000);  // Acceleration in steps per second^2
  stepper.moveTo(-20000); // Move 1000 steps forward
  stepper.run();

  //  Serial.println(digitalRead(LIMIT_SIGNAL));
  Serial.print("Calibrating..");
  while (digitalRead(LIMIT_SIGNAL)) {
    //    Serial.print(".");
    stepper.run();
  }
  setZero();

}

void readStop() {
  pinMode(28, INPUT_PULLUP);
  digitalWrite(34, LOW);
  pinMode(34, OUTPUT);
  if (!digitalRead(28)) {
    Serial.println(digitalRead(28));
    stepper.stop();
    state = 2;
  }
  if (!digitalRead(LIMIT_SIGNAL)) {
    stepper.stop();
    setZero();
  }
}

void setZero() {
  stepper.stop();
  stepper.setCurrentPosition(0);
  Serial.println("\nCalibrated!");
  state = 1;
  stepper.setCurrentPosition(0);
  Serial.println("Stepper position set to zero.");

  setWorkingSpeed();
}

void setWorkingSpeed() {
  // Set up the stepper motor
  stepper.setMaxSpeed(MAX_SPEED);     // Max speed in steps per second
  stepper.setAcceleration(MAX_SPEED);  // Acceleration in steps per second^2

  // Start moving the stepper motor
  stepper.moveTo(STOP_POSITION); // Move 10000 steps forward
}
