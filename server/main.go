package main

import (
	"bufio"
	"crypto/subtle"
	"errors"
	"fmt"
	"io"
	"net/http"
	"os"
	"strings"
	"sync"
	"time"

	"github.com/gin-contrib/cors"
	"github.com/gin-gonic/gin"
	"github.com/gorilla/websocket"
	"github.com/joho/godotenv"
	probing "github.com/prometheus-community/pro-bing"
	"github.com/tarm/serial"
)

var (
	Addresses           = []string{"192.168.1.154"}
	AddressMutex        sync.Mutex
	NewtworkActorActive = true
)

// run on linux
//sudo setcap cap_net_raw+ep ./{binary name}

func main() {
	r := gin.Default()
	err := godotenv.Load()
	if err != nil {
		panic("Error loading .env file")
	}

	config := cors.DefaultConfig()
	config.AllowOrigins = []string{"*"}
	r.Use(cors.New(config))

	r.GET("/ping", func(c *gin.Context) {
		c.JSON(http.StatusOK, gin.H{
			"message": "pong",
		})
	})

	api := r.Group("/api")
	{
		api.POST("/toggle", MiddlewareAuthenticate, handleToggleDoor)
		api.POST("/status", MiddlewareAuthenticate, handleGetStatus)
		api.POST("/test_auth_key", MiddlewareAuthenticate, HandleTestAuthKey)
		address := api.Group("/address", MiddlewareAuthenticate)
		{
			address.POST("/add", handleAddAddress)
			address.POST("/remove", handleRemoveAddress)
			address.POST("/list", handleListAddresses)
		}

		networkActor := api.Group("/network_actor", MiddlewareAuthenticate)
		{
			networkActor.POST("/start", HandleStartNetworkActor)
			networkActor.POST("/stop", HandleStopNetworkActor)
			networkActor.POST("/status", HandleStatusNetworkActor)
		}
		api.GET("/conn", handleConnServer)
		api.POST("/stream", handleStreamInput)
	}

	// r.GET("/na", testNetworkActor)
	go runNetworkActor()

	r.Run(":5000")
}

func MiddlewareAuthenticate(c *gin.Context) {
	payload := struct {
		DoorCode string `json:"doorCode"`
	}{}

	err := c.BindJSON(&payload)
	if err != nil {
		c.JSON(400, gin.H{"response": "unable to parse JSON"})
		return
	}

	secret_door_code := os.Getenv("SECRET_DOOR_CODE")

	result := subtle.ConstantTimeCompare([]byte(secret_door_code), []byte(payload.DoorCode))

	if result == 0 {
		c.JSON(400, gin.H{"response": "incorrect code"})
		return
	}
	c.Next()
}

func HandleTestAuthKey(c *gin.Context) {
	c.JSON(200, gin.H{"response": "valid"})
}

func handleToggleDoor(c *gin.Context) {

	err := ToggleDoor()
	if err != nil {
		c.JSON(400, gin.H{"response": "could not unlock door"})
		return

	}

	c.JSON(200, gin.H{"response": "toggled"})
}

var (
	c = &serial.Config{
		Name:        "/dev/ttyACM0",
		Baud:        9600,
		ReadTimeout: time.Second,
	}
	serialMutex sync.Mutex
)

func ToggleDoor() (err error) {
	serialMutex.Lock()
	defer serialMutex.Unlock()
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

func SerialSendRec(input string) (output string, err error) {
	serialMutex.Lock()
	defer serialMutex.Unlock()

	s, err := serial.OpenPort(c)
	if err != nil {
		return
	}
	defer s.Close()

	command := []byte(input)
	_, err = s.Write(command)
	if err != nil {
		return
	}

	reader := bufio.NewReader(s)

	line, err := reader.ReadString('\n')
	if err != nil {
		return
	}

	return strings.Split(line, "")[0], nil

}

func handleGetStatus(c *gin.Context) {

	status, err := SerialSendRec("s")
	if err != nil {
		c.JSON(400, gin.H{"repsonse": "cannot get status"})
		return
	}

	c.JSON(200, gin.H{"status": status})
}

func handleAddAddress(c *gin.Context) {

	payload := struct {
		Address string `json:"address"`
	}{}

	err := c.BindJSON(&payload)
	if err != nil {
		c.JSON(400, gin.H{"response": "issue parsing json"})
		return
	}

	AddressMutex.Lock()
	defer AddressMutex.Unlock()

	Addresses = append(Addresses, payload.Address)

	c.JSON(200, "added address")

}

func handleRemoveAddress(c *gin.Context) {
	payload := struct {
		Address string `json:"address"`
	}{}

	err := c.BindJSON(&payload)
	if err != nil {
		c.JSON(400, gin.H{"response": "issue parsing json"})
		return
	}

	AddressMutex.Lock()
	defer AddressMutex.Unlock()

	var newAddresses []string
	for _, a := range Addresses {
		if a != payload.Address {
			newAddresses = append(newAddresses, a)
		}
	}

	Addresses = newAddresses

	c.JSON(200, "removed address")
}

func handleListAddresses(c *gin.Context) {
	AddressMutex.Lock()
	defer AddressMutex.Unlock()

	c.JSON(200, gin.H{"addresses": Addresses})
}

func doLock() {
	status, _ := SerialSendRec("s")
	if status != "L" {
		ToggleDoor()
	}
}

func doUnlock() {
	status, _ := SerialSendRec("s")
	if status != "O" {
		ToggleDoor()
	}
}

func runNetworkActor() {

	addressMap := make(map[string]string)

	for NewtworkActorActive {
		// copy addesses
		snapshot := append([]string(nil), Addresses...)

		for _, currentIp := range snapshot {
			if addressMap[currentIp] == "" {
				fmt.Println("initializing ip in map")
				err := PingIp(currentIp)
				if err != nil {
					addressMap[currentIp] = "offline"
				} else {
					addressMap[currentIp] = "online"
				}
			} else {
				//ping IP here
				err := PingIp(currentIp)
				if err != nil {
					// we are offline
					// detect change
					if addressMap[currentIp] == "online" {
						fmt.Println("change detected now offline")
						doLock()
						addressMap[currentIp] = "offline"
					} else {
						fmt.Println(err)
						fmt.Println("no change but offline")
					}

				} else {
					// we are online
					if addressMap[currentIp] == "offline" {
						fmt.Println("change detected now online")
						doUnlock()
						addressMap[currentIp] = "online"
					} else {
						fmt.Println("no change but online")
					}

				}

			}
		}

	}

}

func PingIp(ip string) (err error) {
	pinger, err := probing.NewPinger(ip)
	if err != nil {
		return
	}

	pinger.SetPrivileged(true) // possibly windows only

	pinger.Count = 3
	pinger.Interval = time.Second / 2
	pinger.Timeout = time.Second

	// Run the pinger
	err = pinger.Run()
	if err != nil {
		return
	}

	if pinger.Statistics().PacketsRecv == 0 {
		err = errors.New("no packets received, could not ping")
	}

	return
}

func HandleStartNetworkActor(c *gin.Context) {
	AddressMutex.Lock()
	NewtworkActorActive = true
	AddressMutex.Unlock()
	go runNetworkActor()
	c.JSON(200, "network actor started")
}

func HandleStopNetworkActor(c *gin.Context) {
	AddressMutex.Lock()
	NewtworkActorActive = false
	AddressMutex.Unlock()
	c.JSON(200, "network actor stopped")
}

func HandleStatusNetworkActor(c *gin.Context) {
	AddressMutex.Lock()
	defer AddressMutex.Unlock()
	c.JSON(200, gin.H{"running": NewtworkActorActive})
}

var Upgrader = websocket.Upgrader{
	ReadBufferSize:  1024,
	WriteBufferSize: 1024,
	CheckOrigin: func(r *http.Request) bool {
		return true
	},
}

var (
	Mutex   = sync.Mutex{}
	Clients = make(map[*websocket.Conn]bool)
)

func handleStreamInput(c *gin.Context) {
	fmt.Println("Receiving FFmpeg WebM stream")

	// Read the stream from the request body
	defer c.Request.Body.Close()

	buffer := make([]byte, 1024*64) // Use a buffer large enough for WebM data chunks
	for {
		n, err := c.Request.Body.Read(buffer)
		if n > 0 {
			Mutex.Lock()
			// Send the binary data to all WebSocket clients
			for client := range Clients {
				if Clients[client] {
					err := client.WriteMessage(websocket.BinaryMessage, buffer[:n])
					if err != nil {
						fmt.Println("Error writing to WebSocket client:", err)
						client.Close()
						delete(Clients, client)
					}
				}
			}
			Mutex.Unlock()
		}

		if err != nil {
			if err != io.EOF {
				fmt.Println("Error reading FFmpeg stream:", err)
			}
			break
		}
	}
}

func handleConnServer(c *gin.Context) {

	fmt.Println("connected to ws")

	conn, err := Upgrader.Upgrade(c.Writer, c.Request, nil)
	if err != nil {
		return
	}
	defer conn.Close()

	Mutex.Lock()
	Clients[conn] = true
	Mutex.Unlock()

	defer func() {
		Mutex.Lock()
		Clients[conn] = false
		Mutex.Unlock()
	}()

	for {
		msgType, msg, err := conn.ReadMessage()
		if err != nil {
			break
		}
		fmt.Println(msg)
		Mutex.Lock()
		for client := range Clients {
			if Clients[client] && client != conn {
				client.WriteMessage(msgType, msg)
			}
		}
		Mutex.Unlock()
	}

}
