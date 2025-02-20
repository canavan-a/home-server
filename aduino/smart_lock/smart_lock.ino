#include <AccelStepper.h>

// Define the pins for the X axis stepper
#define X_STEP_PIN 54
#define X_DIR_PIN 55
#define X_ENABLE_PIN 38

#define Y_STEP_PIN 60
#define Y_DIR_PIN 61
#define Y_ENABLE_PIN 56

#define X_MIN_PIN 3  // Endstop for minimum position (X_MIN)
#define X_MAX_PIN 2  // Endstop for maximum position (X_MAX)
#define BUTTON 14

// Create an AccelStepper object for the X axis
AccelStepper stepperX(AccelStepper::DRIVER, X_STEP_PIN, X_DIR_PIN);

AccelStepper stepperCamera(AccelStepper::DRIVER, Y_STEP_PIN, Y_DIR_PIN);

void setup() {
  // Initialize serial communication for debugging
  Serial.begin(9600);

  // Initialize the enable pin
  pinMode(X_ENABLE_PIN, OUTPUT);
  digitalWrite(X_ENABLE_PIN, LOW);

  pinMode(Y_ENABLE_PIN, OUTPUT);
  digitalWrite(Y_ENABLE_PIN, LOW);

  // Set the maximum speed and acceleration for the X stepper
  stepperX.setMaxSpeed(3000);      // Max speed (steps per second)
  stepperX.setAcceleration(3000);  // Acceleration (steps per second^2)
  stepperX.setSpeed(3000);
  digitalWrite(X_ENABLE_PIN, HIGH);  // Enable the X stepper motor

  // Set the maximum speed and acceleration for the X stepper
  stepperCamera.setMaxSpeed(3000);      // Max speed (steps per second)
  stepperCamera.setAcceleration(3000);  // Acceleration (steps per second^2)
  stepperCamera.setSpeed(3000);
  digitalWrite(Y_ENABLE_PIN, HIGH);

  pinMode(X_MIN_PIN, INPUT_PULLUP);
  pinMode(X_MAX_PIN, INPUT_PULLUP);
  pinMode(BUTTON, INPUT_PULLUP);
}

bool directionToggle = true;

void loop() {

  bool xMinState = digitalRead(X_MIN_PIN);
  bool xMaxState = digitalRead(X_MAX_PIN);

  bool buttonState = digitalRead(BUTTON);

  if (xMaxState == LOW) {
    stepperX.setSpeed(3000);
    directionToggle = true;
  } else if (xMinState == LOW) {
    stepperX.setSpeed(-3000);
    directionToggle = false;
  }

  // Check if data is available on the serial port
  if (Serial.available() > 0 || buttonState == LOW) {
    char command = Serial.read();  // Read the incoming byte (command)
    if (command == 'g' || buttonState == LOW) {
      digitalWrite(X_ENABLE_PIN, LOW);  // Enable the motor
      while (true) {
        // Move the motor at the set speed
        stepperX.runSpeed();  // Keep moving the motor until the target is reached
        // Continuously check if the X_MIN endstop is pressed
        bool xMinState = digitalRead(X_MIN_PIN);
        bool xMaxState = digitalRead(X_MAX_PIN);

        if (directionToggle && xMinState == LOW) {

          break;
        } else if (!directionToggle && xMaxState == LOW) {

          break;
        }
      }

      digitalWrite(X_ENABLE_PIN, HIGH);  // Disable the motor to reduce humming
    } else if (command == 'l' || command == 'r') {

      // enable stepper
      digitalWrite(Y_ENABLE_PIN, LOW);

      if (command == 'l'){
        stepperCamera.setSpeed(500);
      } else{
        stepperCamera.setSpeed(-500);
      }

      unsigned long startTime = millis();

      while(millis() - startTime <150){
          stepperCamera.runSpeed();
      }

      // delay(50);

      // disable stepper
      digitalWrite(Y_ENABLE_PIN, HIGH);
    } else if (command == 's'){  // status command
      if (directionToggle) {
        Serial.println("L"); // locked
      } else {
        Serial.println("O"); // neither or open
      }
      
    } else if (command == 'L' || command == 'R'){
      // expect L{number}L 
      String numberString = Serial.readStringUntil(command);

      int num = numberString.toInt();
      
      Serial.read();

      digitalWrite(Y_ENABLE_PIN, LOW);

      if (command == 'L'){
        stepperCamera.setSpeed(700);
      } else{
        stepperCamera.setSpeed(-700);
      }
      unsigned long startTime = millis();

      while(millis() - startTime < num){
        stepperCamera.runSpeed();
      }
      // delay(50);
      digitalWrite(Y_ENABLE_PIN, HIGH);
      
    }
  }
}
