#include <Arduino.h>

#include "constants.h"
#include "stepper.h"


Stepper yaw;
Stepper pitch;

void setup()
{
    // Initialize serial communication for debugging
    Serial.begin(9600);

	yaw = Stepper{stepper::Yaw::step, stepper::Yaw::dir, stepper::Yaw::enable };
	
	
	pitch = Stepper{stepper::Pitch::step, stepper::Pitch::dir, stepper::Pitch::enable };


	yaw.enable();
	pitch.enable();
}

unsigned long start = millis();

void loop()
{
	if (millis() - start > 1000){
		Serial.println("swapping directions");
		start = millis();
		yaw.setSpeed(yaw.speed * -1);
		pitch.setSpeed(pitch.speed * -1);
	}

	pitch.run();
	yaw.run();		
}
