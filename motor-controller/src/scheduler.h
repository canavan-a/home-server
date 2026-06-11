#pragma once

#include <Arduino.h>
#include "motorManager.h"
#include "serialManager.h"
#include "simulate.h"

const int TRACKING_HOME_TIMEOUT_DELAY{3000};

struct Scheduler {

	MotorManager mm;
	SerialManager serial{&mm};
	// Simulate s;
	unsigned long lastMsgTime{0};

	Scheduler() {}

	void begin(){
		Serial.begin(115200);
		// s.begin();
		lastMsgTime = millis();
	}

	void tick(){
		serial.tick();
		// s.tick();

		PositionMsg pm;
		if(mm.trackingQueue.receiveNonBlocking(pm)){
			lastMsgTime = millis();
			mm.setCurrentAction(Action::DETECTION);
			Serial.print("[SCHED] dequeued yaw="); Serial.print(pm.yaw);
			Serial.print(" pitch="); Serial.println(pm.pitch);
			mm.yaw.handlePID(pm.yaw);
			mm.pitch.handlePID(pm.pitch, true);
		}

		if(millis() - lastMsgTime > TRACKING_HOME_TIMEOUT_DELAY){
			mm.setCurrentAction(Action::GO_HOME);
		}

		mm.actionStep();
	}
};
