#include <Arduino.h>
#include "scheduler.h"

Scheduler s;

void setup(){
	Serial.begin(9600);
	s.begin();
}

void loop(){
	s.tick();
}
