#pragma once

#include <Arduino.h>
#include "motorManager.h"
#include "action.h"
#include "positionMsg.h"

struct Simulate {

	Simulate() = default;

	MotorManager* mm;
	unsigned long lastSend{0};
	unsigned long startTime{0};
	bool started{false};
	int polarity{1};

	Simulate(MotorManager* manager) : mm{manager} {}

	void begin(){
		startTime = millis();
	}

	void tick(){
		if(!started){
			if(millis() - startTime < 5000) return;
			started = true;
		}
		if(millis() - lastSend >= 500){
			lastSend = millis();
			PositionMsg pm{polarity * 300, polarity * 300, Action::DETECTION};
			mm->trackingQueue.send(pm);
			polarity = -1 * polarity;
		}
	}
};
