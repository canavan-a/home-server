#pragma once

#include "action.h"
#include "motorManager.h"
#include "positionMsg.h"
#include "constants.h"

struct SerialManager {

	MotorManager* mm;

	SerialManager(MotorManager* otherManager):mm(otherManager){}

	void tick(){
		if (!Serial.available()) return;
		String msg = Serial.readStringUntil('\n');
		msg.trim();
		if (msg.length() == 0) return;

		String tokens[5];
		int count = 0;
		int start = 0;
		for (int i = 0; i <= (int)msg.length(); i++){
			if (msg[i] == ' ' || i == (int)msg.length()){
				if (count < 5) tokens[count++] = msg.substring(start, i);
				start = i + 1;
			}
		}

		Action action = static_cast<Action>(tokens[0].toInt());
		Serial.print("[SERIAL] action="); Serial.print((int)action);
		Serial.print(" tokens="); Serial.println(count);

		switch(action){
			case Action::SET_HOME:
				mm->setHome(tokens[1].toInt(), tokens[2].toInt());
				mm->setCurrentAction(Action::GO_HOME);
				Serial.print("[HOME] yaw="); Serial.print(tokens[1].toInt());
				Serial.print(" pitch="); Serial.println(tokens[2].toInt());
				break;
			case Action::GO_TO_POSITION:
				mm->setPosition(tokens[1].toInt(), tokens[2].toInt());
				mm->setCurrentAction(Action::GO_TO_POSITION);
				Serial.print("[GOTO] yaw="); Serial.print(tokens[1].toInt());
				Serial.print(" pitch="); Serial.println(tokens[2].toInt());
				break;
			case Action::DETECTION:
				{
				if(count < 5){ Serial.println("[DETECT] drop: bad token count"); break; }
				float x1 = tokens[1].toFloat();
				float y1 = tokens[2].toFloat();
				float x2 = tokens[3].toFloat();
				float y2 = tokens[4].toFloat();
				if(x2 <= x1 || x1 < 0 || x2 > config::frameWidth || y1 < 0 || y2 > config::frameHeight){
					Serial.println("[DETECT] drop: invalid bbox");
					break;
				}
				int yaw  = static_cast<int>((x1 + x2) / 2.0f) - config::frameWidth  / 2;
				int pitch = static_cast<int>((y1 + y2) / 2.0f) - config::frameHeight / 2;
				Serial.print("[DETECT] x1="); Serial.print(x1);
				Serial.print(" y1="); Serial.print(y1);
				Serial.print(" x2="); Serial.print(x2);
				Serial.print(" y2="); Serial.print(y2);
				Serial.print(" -> yaw="); Serial.print(yaw);
				Serial.print(" pitch="); Serial.println(pitch);
				PositionMsg pm{yaw, pitch, Action::DETECTION};
				mm->trackingQueue.send(pm);
				}
				break;
			default:
				Serial.print("[SERIAL] unknown action: "); Serial.println((int)action);
				break;
		}
	}
};
