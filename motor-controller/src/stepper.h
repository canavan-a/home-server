#pragma once

#include <AccelStepper.h>


enum Direction{
	Yaw,
	Pitch,	
};

const int DEFAULT_SPEED{200};

struct Stepper{

	AccelStepper stepper;

	int enablePin{};

	int speed{DEFAULT_SPEED};

	int home{0};

	Stepper() = default;
	
	Stepper(int step, int dir, int myEnablePin):stepper{AccelStepper(AccelStepper::DRIVER, step, dir)}, enablePin{myEnablePin} {
		Serial.println("stepper is initialized");
		pinMode(myEnablePin, OUTPUT);
		stepper.setMaxSpeed(3000);
		stepper.setAcceleration(1500);
		
		stepper.setSpeed(DEFAULT_SPEED);
	}

	void setHome(int newHome){
		home = newHome;
		goHome();
	}

	void moveToPosition(int pos){
		stepper.moveTo(pos);
	}

	void goHome(){
		this->moveToPosition(home);
	}

	bool runToPos(){
		return stepper.run();
	}

	int position(){
		return stepper.currentPosition();
	}

	void enable(){
		digitalWrite(enablePin, LOW);
	}

	void disable(){
		digitalWrite(enablePin, HIGH);
	}

	void setSpeed(int newSpeed){
		speed = newSpeed;
		stepper.setSpeed(newSpeed);
	}

	void run(){
		stepper.runSpeed();
	}

};
