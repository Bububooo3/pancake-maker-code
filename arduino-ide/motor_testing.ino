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
  // pinMode(enPin, OUTPUT);
  // digitalWrite(enPin, LOW);
  pinMode(pushPin, INPUT_PULLUP);
  Serial.begin(9600);
  // pinMode(stepPin, OUTPUT);
  // pinMode(dirPin, OUTPUT);
}

void loop() {
  // stepper.setCurrentPosition(0);
  // stepper.setSpeed(800);

  for (int x = 0; x < stepsPerRevolution; x++) {
    if (digitalRead(pushPin) == LOW) {
      stepper.SetSpeed(400);
      // digitalWrite(stepPin, HIGH);
      // delayMicroseconds(500);
      // digitalWrite(stepPin, LOW);
      // delayMicroseconds(500);

      Serial.println("Pressed");
    } else {
      stepper.SetSpeed(0);

      Serial.println("Not Pressed");
    }
    
    stepper.runSpeed();
  }
}
