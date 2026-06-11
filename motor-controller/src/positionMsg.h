#pragma once

#include "action.h"

// location values
struct PositionMsg {
	Action action;
	// serial data	
	int pitch;
	int yaw;
	PositionMsg() = default;
	PositionMsg(int x, int y, Action a): action{a}, yaw{x}, pitch{y}{
	}
};
