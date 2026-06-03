#pragma once


#include "constants.h"



struct MotorManager {

	Stepper yaw;
	Stepper pitch;

	MotorManager(){
		yaw = Stepper{stepper::Yaw::step, stepper::Yaw::dir, stepper::Yaw::enable };
		pitch = Stepper{stepper::Pitch::stepper, stepper::Pitch::dir, stepper::Pitch::enable };
		yaw.enable();
		pitch.enable();
	}

	

	void run(){
		
	}
	
};
