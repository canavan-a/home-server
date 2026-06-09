#pragma once

#include "action.h"
#include "motorManager.h"
#include "positionMsg.h"
#include "constants.h"

struct SerialManager {

	MotorManager* mm;

	SerialManager(MotorManager* otherManager):mm(otherManager){
		Serial.begin(9600);

				
		
	}

	void run(void* param){
	    for(;;){
	        if (Serial.available()){
	            String msg = Serial.readStringUntil('\n');
	            msg.trim();
	            if (msg.length() > 0){
	                String tokens[5];
	                int count = 0;
	                int start = 0;
	                for (int i = 0; i <= msg.length(); i++){
	                    if (msg[i] == ' ' || i == msg.length()){
	                        tokens[count++] = msg.substring(start, i);
	                        start = i + 1;
	                    }
	                }
	                Action action = static_cast<Action>(tokens[0].toInt());
	                switch(action){
	                    case Action::SET_HOME:
	                    	// sets the home default position for when it goess home 
							mm->setHome{tokens[1].toInt(), tokens[2].toInt()};
							mm->setCurrentAction(Action::GO_HOME);
	                        break;
	                    case Action::GO_TO_POSITION:
	                        // tokens[1].toInt(), tokens[2].toInt()
							mm->setPosition{tokens[1].toInt(), tokens[2].toInt()};
							mm->setCurrentAction(Action::GO_TO_POSITION);
	                        break;
	                    case Action::DETECTION:
							Serial.println("detection");
	                        // tokens[1].toFloat() etc
	                        int x{static_cast<int>((tokens[1].toFloat()+tokens[3].toFloat())/2) - config::frameWidth/2};
	                        int y{static_cast<int>((tokens[2].toFloat()+tokens[4].toFloat())/2) - config::frameHeight/2};
	                        
							PositionMsg pm {x, y, Action::DETECTION};
							mm->trackingQueue.send(pm);
	                        
	                        break;
	                    default:
	                        break;
	                }
	            }
	        }
	    }
	}
	 
};
