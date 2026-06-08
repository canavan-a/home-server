#pragma once

#include <Arduino.h>

#include "motorManager.h"



struct Scheduler{

	MotorManager *mm;

	Scheduler(){
		mm = new MotorManager();
	}

	static void trackingTask(void* param){
		static_cast<Scheduler*>(param)->mm->trackRunner(param);
	}

	static void actionTask(void* param){
		static_cast<Scheduler*>(param)->mm->actionRunner(param);
	}

	void start(){
		Serial.println("starting tasks");
		xTaskCreate(trackingTask, "tracker", 4096, this, 1, NULL);
		xTaskCreate(actionTask, "action", 4096, this, 1, NULL);
		
	}
	
};
