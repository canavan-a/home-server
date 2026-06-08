#pragma once

#include <Arduino_FreeRTOS.h>

#include "constants.h"
#include "stepper.h"
#include "queueWrapper.h"
#include "action.h"

const int TRACKING_HOME_TIMEOUT_DELAY{3000};

// location values
struct PositionMsg {
	Action action;
	// serial data	
	int pitch;
	int yaw;
};


struct MotorManager {

	Stepper yaw;
	Stepper pitch;

	bool trackingEnabled{true};
	
	Action currentAction{EMPTY};
	
	Queue<PositionMsg, 1> trackingQueue{}; // incoming 
	Queue<Action, 10> actionQueue{}; //

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

	void actionRunner(void* param){
		for (;;){ 
			// switch on action and tick based on which action in operation
			// NONE action has a slight delay to restrict spinning
			// NONE action is only set when home is complete

			if (currentAction == Action::EMPTY){
				vTaskDelay(500);
			} else if(currentAction == Action::GO_HOME) {
				setPosition(yaw.home, pitch.home);
				if (!yaw.runToPos() &&  !pitch.runToPos()){
					// back to idle when done
					setCurrentAction(Action::EMPTY);
				}
			} else if(currentAction == Action::DETECTION){
				// run speed based on control trackerRunner control loop
				yaw.run();
				pitch.run();
			} else if(currentAction == Action::GO_TO_POSITION){
				// assume position is already set externally
				if (!yaw.runToPos() &&  !pitch.runToPos()){
					// back to idle when done
					setCurrentAction(Action::EMPTY);
				}
			}
			
		}
	}

	void setCurrentAction(Action action){
		// add special home override action condition??? can any action override?
		currentAction = action;
	}

	void updateMotorSpeeds(){
		
	}	

	void trackRunner(void* param){
		
		
		for(;;){
			// delay action to stop wasting cpu time
			if (trackingEnabled) {
				// do main track runner
				PositionMsg pm;
				bool valid = trackingQueue.receiveUntil(pm, pdMS_TO_TICKS(TRACKING_HOME_TIMEOUT_DELAY));		
				if (!valid){
					pitch.PDEnd();
					yaw.PDEnd();
					setCurrentAction(Action::GO_HOME);
					continue;
				}
				setCurrentAction(Action::DETECTION);
				pitch.PDStart();
				yaw.PDStart();

				pitch.PDEvent();
				yaw.PDEvent();
				
			} else{
				vTaskDelay(pdMS_TO_TICKS(500));
			}
		}
	}
	
};


