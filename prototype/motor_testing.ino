#include <Arduino.h>
#include "AccelStepper.h"

// Define stepper motor connections and motor interface type.
// Motor interface type must be set to 1 when using a driver
#define dirPin 2
#define stepPin 3
#define stepsPerRevolution 200
#define motorInterfaceType 1
// #define enPin 4
#define pushPin 12

// Create a new instance of the AccelStepper class:
AccelStepper stepper = AccelStepper(motorInterfaceType, stepPin, dirPin);

void setup() {
  // Set the maximum speed in steps per second:
  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(200);
  pinMode(pushPin, INPUT_PULLUP);
  Serial.begin(9600);
}

void loop() {
  for (int x = 0; x < stepsPerRevolution; x++) {
    if (digitalRead(pushPin) == LOW) {
      stepper.setSpeed(800);

      Serial.println("Pressed");
    } else {
      stepper.setSpeed(0);

      Serial.println("Not Pressed");
    }
    
    stepper.runSpeed();
  }
}
