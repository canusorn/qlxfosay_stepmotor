//Red: A+
//Green: A-
//Yellow: B+
//Blue: B-


#include <AccelStepper.h>

// Define the pins for stepper motor control
#define DIR_PIN_ 8
#define DIR_PIN 9
#define STEP_PIN_ 10
#define STEP_PIN 11
#define ENABLE_PIN_ 12
#define ENABLE_PIN 13

// Create an instance of AccelStepper
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

void setup() {
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

  Serial.begin(115200);

  stepper.setMaxSpeed(10000);     // Set maximum speed (steps per second)
  stepper.setAcceleration(1000);  // Set acceleration (steps per second^2)
}

void loop() {
  // Set the target position (e.g., 1000 steps)
  stepper.moveTo(-10000);

  // Run the motor to the target position
  stepper.run();

  Serial.println(stepper.distanceToGo());
  Serial.println(stepper.currentPosition());
  Serial.println("");

  // If the motor reaches the target position, move it back to the starting point
  if (stepper.distanceToGo() == 0) {
    digitalWrite(ENABLE_PIN, HIGH);
  }
}
