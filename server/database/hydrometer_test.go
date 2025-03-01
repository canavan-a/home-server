package database_test

import (
	"fmt"
	"main/database"
	"testing"
)

func TestHydrometer(t *testing.T) {
	hyd := database.Hydrometer{
		ID:        1,
		IP:        "192.168.1.164",
		PushStack: database.NewPushStack(10),
	}

	temp := hyd.GetTemperature()

	if temp == 0.0 {
		t.Error("invalid temp")
	}

	fmt.Println(temp)
}
