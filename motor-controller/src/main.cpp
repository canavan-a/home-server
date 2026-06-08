#include <Arduino.h>

#include "constants.h"
#include "stepper.h"
#include "scheduler.h"

Stepper yaw;
Stepper pitch;


Scheduler* s;

void setup()
{
    // Initialize serial communication for debugging
    Serial.begin(9600);

// 	yaw = Stepper{stepper::Yaw::step, stepper::Yaw::dir, stepper::Yaw::enable };
// 	
// 	
// 	pitch = Stepper{stepper::Pitch::step, stepper::Pitch::dir, stepper::Pitch::enable };
// 
// 
// 	yaw.enable();
// 	pitch.enable();
	s = new Scheduler();
	s->start();
}

unsigned long start = millis();

int homeValue{100};
void loop()
{
	// if (millis() - start > 1000){
	// 	Serial.println("swapping directions");
	// 	start = millis();
	// 	yaw.setSpeed(yaw.speed * -1);
	// 	pitch.setSpeed(pitch.speed * -1);
	// }
	// Serial.println(yaw.stepper.currentPosition());
	// 
	// Serial.println(pitch.stepper.currentPosition());
	// pitch.run();
	// yaw.run();
	// if (homeValue==100){
	// 	homeValue=-100;
	// } else{
	// 	homeValue = 100;
	// }

	// yaw.setHome(homeValue);
	// yaw.goHome();
	// while(yaw.runToPos());
	vTaskDelay(portMAX_DELAY);
	
}

// vTaskDelay(portMAX_DELAY);
