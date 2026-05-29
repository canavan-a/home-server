package main

import (
	"fmt"
	"net/http"
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
	mqttBroker     = "tcp://localhost:1883"
	mqttClientID   = "home-server-door"
	mqttTopicState = "open-lock-state"
	mqttTopicCmd   = "open-lock-signal"
)

type MqttDoor struct {
	mu     sync.RWMutex
	state  doorState
	client mqtt.Client
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

	d.mu.RLock()
	needsState := d.state == doorStateUnknown
	d.mu.RUnlock()

	if needsState {
		d.client.Publish(mqttTopicCmd, 1, false, "state")
	}
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
