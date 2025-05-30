package garage

import (
	"errors"
	"fmt"
	"net/http"

	"github.com/gin-gonic/gin"
)

func HandleTriggerGarage(c *gin.Context) {
	err := triggerGarage()
	if err != nil {
		c.JSON(400, err.Error())
		return
	}
	c.JSON(200, "triggered")
}

const GARAGE_ESP32_IP = "192.168.1.165"

func triggerGarage() error {

	resp, err := http.Get("http://" + GARAGE_ESP32_IP + "/trigger")
	if err != nil {
		fmt.Println("bad api call")
		return err
	}

	if resp.StatusCode >= 400 {
		return errors.New("bad code")
	}

	return nil

}

func viewGarageStatus() {
	// status based off magnet trigger
}
