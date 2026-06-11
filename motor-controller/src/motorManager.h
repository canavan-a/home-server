#pragma once

#include "constants.h"
#include "stepper.h"
#include "queueWrapper.h"
#include "action.h"
#include "positionMsg.h"

struct MotorManager {

	Stepper yaw;
	Stepper pitch;

	bool trackingEnabled{true};

	Action currentAction{EMPTY};

	Queue<PositionMsg, 1> trackingQueue{};
	Queue<Action, 10> actionQueue{};

	MotorManager(){
		yaw = Stepper{stepper::Yaw::step, stepper::Yaw::dir, stepper::Yaw::enable };
		pitch = Stepper{stepper::Pitch::step, stepper::Pitch::dir, stepper::Pitch::enable };
		yaw.enable();
		pitch.enable();
	}

	void setHome(int yawHome, int pitchHome){
		yaw.setHome(yawHome);
		pitch.setHome(pitchHome);
	}

	void setPosition(int yawPos, int pitchPos){
		yaw.moveToPosition(yawPos);
		pitch.moveToPosition(pitchPos);
	}

	void toggleTracking(){
		trackingEnabled = !trackingEnabled;
	}

	void setCurrentAction(Action action){
		if(action == Action::EMPTY){
			yaw.disable();
			pitch.disable();
		} else {
			yaw.enable();
			pitch.enable();
		}
		currentAction = action;
	}

	// Called every loop iteration — runs one step of the action state machine
	void actionStep(){
		if(currentAction == Action::EMPTY){
			// idle
		} else if(currentAction == Action::GO_HOME){
			setPosition(yaw.home, pitch.home);
			bool moving = yaw.runToPos() || pitch.runToPos();
			if(!moving) setCurrentAction(Action::EMPTY);
		} else if(currentAction == Action::DETECTION){
			yaw.run();
			pitch.run();
		} else if(currentAction == Action::GO_TO_POSITION){
			bool moving = yaw.runToPos() || pitch.runToPos();
			if(!moving) setCurrentAction(Action::EMPTY);
		}
	}
};
