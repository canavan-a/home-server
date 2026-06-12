package main

import (
	"fmt"
	"net/http"
	"strconv"
	"sync"

	mqtt "github.com/eclipse/paho.mqtt.golang"
	"github.com/gin-gonic/gin"
)

type doorState int

const (
	doorStateUnknown doorState = iota
	doorStateOpen
	doorStateClosed
)

func (s doorState) String() string {
	switch s {
	case doorStateOpen:
		return "open"
	case doorStateClosed:
		return "closed"
	default:
		return "unknown"
	}
}

const (
	mqttBroker            = "tcp://localhost:1883"
	mqttClientID          = "home-server-door"
	mqttTopicState        = "open-lock-state"
	mqttTopicBatteryState = "open-lock-battery"
	mqttTopicCmd          = "open-lock-signal"
)

type MqttDoor struct {
	mu             sync.RWMutex
	state          doorState
	batteryPercent int
	client         mqtt.Client
}

func NewMqttDoor() (*MqttDoor, error) {
	d := &MqttDoor{state: doorStateUnknown}

	opts := mqtt.NewClientOptions().
		AddBroker(mqttBroker).
		SetClientID(mqttClientID).
		SetAutoReconnect(true).
		SetOnConnectHandler(d.onConnect)

	d.client = mqtt.NewClient(opts)
	if tok := d.client.Connect(); tok.Wait() && tok.Error() != nil {
		return nil, tok.Error()
	}

	return d, nil
}

func (d *MqttDoor) onConnect(mc mqtt.Client) {
	mc.Subscribe(mqttTopicState, 1, func(_ mqtt.Client, msg mqtt.Message) {
		d.mu.Lock()
		defer d.mu.Unlock()
		switch string(msg.Payload()) {
		case "open":
			d.state = doorStateOpen
		case "closed":
			d.state = doorStateClosed
		}
		fmt.Println("door state updated:", d.state)
	})

	mc.Subscribe(mqttTopicBatteryState, 1, func(_ mqtt.Client, msg mqtt.Message) {
		d.mu.Lock()
		defer d.mu.Unlock()

		batteryPercent, err := strconv.Atoi(string(msg.Payload()))
		if err != nil {
			// 999 signals parse error to the UI
			batteryPercent = 999
		}

		d.batteryPercent = batteryPercent
	})

	d.mu.RLock()
	needsState := d.state == doorStateUnknown
	d.mu.RUnlock()

	if needsState {
		d.client.Publish(mqttTopicCmd, 1, false, "state")
	}
	d.client.Publish(mqttTopicCmd, 1, false, "battery")
}

func (d *MqttDoor) HandleGetState(c *gin.Context) {
	d.mu.RLock()
	s := d.state
	d.mu.RUnlock()
	c.JSON(http.StatusOK, gin.H{"state": s.String()})
}

func (d *MqttDoor) HandleOpen(c *gin.Context) {
	d.client.Publish(mqttTopicCmd, 1, false, "open")
	c.JSON(http.StatusOK, gin.H{"sent": "open"})
}

func (d *MqttDoor) HandleClose(c *gin.Context) {
	d.client.Publish(mqttTopicCmd, 1, false, "closed")
	c.JSON(http.StatusOK, gin.H{"sent": "closed"})
}

func (d *MqttDoor) HandleGetBatteryState(c *gin.Context) {
	d.mu.RLock()
	b := d.batteryPercent
	d.mu.RUnlock()
	c.JSON(http.StatusOK, gin.H{"battery": b})
}
