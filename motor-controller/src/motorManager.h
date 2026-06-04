#pragma once

#include <Arduino_FreeRTOS.h>

#include "constants.h"
#include "stepper.h"
#include "queue.h"

const int TRACKING_HOME_TIMEOUT_DELAY{3000};

enum Task {
	NONE,
	TRACK,
	HOME,
	SET_HOME,	
};

// location values
struct TrackingMsg {
	// serial data	
	int x;
	int y;
};


struct MotorManager {

	Stepper yaw;
	Stepper pitch;

	bool trackingEnabled{true};
	
	Task currentTask{NONE};

	Queue<TrackingMsg> trackingQueue;
	Queue<Task> taskQueue;

	MotorManager(){
		tackingQueue = Queue(1); // mailbox
		taskQueue = Queue(10); // regular queue
		yaw = Stepper{stepper::Yaw::step, stepper::Yaw::dir, stepper::Yaw::enable };
		pitch = Stepper{stepper::Pitch::stepper, stepper::Pitch::dir, stepper::Pitch::enable };
		yaw.enable();
		pitch.enable();
	}
	
	void setHome(int yawHome, int pitchHome){
		yaw.setHome(yawHome);
		pitch.setHome(pitchHome);
	}

	void taskRunner(){
		for (;;){
			// switch on task and tick based on which task in operation
		}
	}

	void setCurrentTask(Task task){
		// add special home override task condition??? can any task override?
		currentTask = task;
	}

	void trackRunner(){
		
		
		for(;;){
			// delay task to stop wasting cpu time
			if (trackingEnabled) {
				// do main track runner
				TrackingMsg tm;
				valid = trackingQueue.receive(&tm, pdMS_TO_TICKS(TRACKING_HOME_TIMEOUT_DELAY));		
				if (!valid){
					setCurrentTask(Task::HOME);
					continue;
				}
				setCurrentTask(Task::TRACK);
				
				
			} else{
				vTaskDelay(pdMS_TO_TICKS(500));
			}
		}
	}
	
};
