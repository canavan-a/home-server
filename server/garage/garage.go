package garage

import (
	"errors"
	"fmt"
	"io"
	"net/http"
	"strconv"

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

func HandleGarageStatus(c *gin.Context) {
	value, err := viewGarageStatus(false)
	if err != nil {
		c.JSON(400, "issue making request")
		return
	}

	c.JSON(200, value)
}
func HandleGarageStatusOpen(c *gin.Context) {
	value, err := viewGarageStatus(true)
	if err != nil {
		c.JSON(400, "issue making request")
		return
	}

	c.JSON(200, value)
}

const GARAGE_ESP32_IP = "192.168.1.165"

func triggerGarage() error {

	resp, err := http.Get("http://" + GARAGE_ESP32_IP + "/change")
	if err != nil {
		fmt.Println("bad api call")
		return err
	}

	if resp.StatusCode >= 400 {
		return errors.New("bad code")
	}

	return nil

}

func viewGarageStatus(openStatus bool) (int, error) {
	// status based off magnet trigger

	uri := "http://" + GARAGE_ESP32_IP + "/garage"

	if openStatus {
		uri += "_open"
	}

	resp, err := http.Get(uri)
	if err != nil {
		fmt.Println("bad api call")
		return 0, err
	}

	if resp.StatusCode >= 400 {
		return 0, errors.New("bad code")
	}

	data, err := io.ReadAll(resp.Body)
	if err != nil {
		return 0, err
	}
	defer resp.Body.Close()

	strValue := string(data)

	intValue, err := strconv.Atoi(strValue)
	if err != nil {
		return 0, err
	}

	return intValue, nil

}
