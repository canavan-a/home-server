package main

import (
	"crypto/subtle"
	"net/http"
	"os"
	"time"

	"github.com/gin-gonic/gin"
	"github.com/tarm/serial"
)

func main() {
	r := gin.Default()
	r.GET("/ping", func(c *gin.Context) {
		c.JSON(http.StatusOK, gin.H{
			"message": "pong",
		})
	})

	api := r.Group("/api")
	{
		api.GET("/toggle/:doorCode", handleUnlockDoor)
	}

	r.NoRoute(func(c *gin.Context) {
		c.JSON(200, gin.H{"response": "ok"})
	})
	r.Run(":5000")
}

func handleUnlockDoor(c *gin.Context) {

	doorCode := c.Param("doorCode")

	secret_door_code := os.Getenv("SECRET_DOOR_CODE")

	result := subtle.ConstantTimeCompare([]byte(secret_door_code), []byte(doorCode))

	if result != 0 {
		c.JSON(400, gin.H{"response": "incorrect code"})
		return
	}

	err := UnlockDoor()
	if err != nil {
		c.JSON(400, gin.H{"response": "could not unlock door"})
		return

	}

	c.JSON(200, gin.H{"response": "toggled"})
}

func UnlockDoor() (err error) {
	c := &serial.Config{
		Name:        "/dev/ttyACM0",
		Baud:        9600,
		ReadTimeout: time.Second,
	}

	s, err := serial.OpenPort(c)
	if err != nil {
		return err
	}
	defer s.Close()

	command := []byte("g")
	_, err = s.Write(command)
	if err != nil {
		return err
	}

	return nil

}
