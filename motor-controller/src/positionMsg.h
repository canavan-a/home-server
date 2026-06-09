#pragma once

#include "action.h"

// location values
struct PositionMsg {
	Action action;
	// serial data	
	int pitch;
	int yaw;

	PostitionMsg(int x, int y, Action a): action{a}, yaw{x}, pitch{y}{
	}
};
