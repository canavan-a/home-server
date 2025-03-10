package main

import (
	"bufio"
	"bytes"
	"crypto/subtle"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"log"
	"main/clipper"
	"main/database"
	"main/rawframe"
	"main/tracker"
	"net"
	"net/http"
	"os"
	"os/exec"
	"runtime/pprof"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/gin-contrib/cors"
	"github.com/gin-gonic/gin"
	"github.com/google/uuid"
	"github.com/gorilla/websocket"
	"github.com/joho/godotenv"
	"github.com/pion/rtp"
	"github.com/pion/webrtc/v4"
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

	device := clipper.NewClipStorageDevice()
	go device.Run()

	clipMaker := clipper.NewClipper(device.StorageChannel)
	go clipMaker.Run()

	go func() {
		fmt.Println("starting video stream")
		err := StreamReader("video", "0.0.0.0", 5005)
		panic(err)
	}()

	frameReader := rawframe.NewFrameReader("/tmp/raw_frame", clipMaker.PacketChannel)
	go frameReader.Run()

	tk := tracker.NewTracker("/tmp/json_pipe", TrackerRunner, clipMaker.ReceiveEntity)
	go tk.Run()

	config := cors.DefaultConfig()
	config.AllowOrigins = []string{"*"}
	r.Use(cors.New(config))

	r.GET("/ping", func(c *gin.Context) {
		c.JSON(http.StatusOK, gin.H{
			"message": "pong",
		})
	})

	hn := database.NewHydrometerNetwork()
	hn.StartPolling()

	api := r.Group("/api")
	{
		api.GET("/health", func(c *gin.Context) { c.JSON(200, "ok") })
		api.GET("/heap", MiddlewareAuthenticate, writeHeapProfile)

		trk := api.Group("/tracker", MiddlewareAuthenticate)
		{
			trk.GET("/toggle", MakeToggleTrackerRoute(&tk))
			trk.GET("/status", MakeTrackerStatusRoute(&tk))
			speed := trk.Group("speed")
			{
				speed.GET("/get", GetTrackerSpeed)
				speed.GET("/set", SetTrackerSpeed)
			}

		}

		clp := api.Group("/clipper", MiddlewareAuthenticate)
		{
			clp.GET("/toggle", MakeClipperToggleRoute(&tk, clipMaker))
			clp.GET("/status", MakeClipperStatusRoute(&tk))
			clp.GET("/clipping", MakeClipperClippingRoute(clipMaker))
			clp.GET("/list", HandleListClips)
			clp.GET("/download", HandleDownloadClip)
		}

		hyd := api.Group("/hydrometer")
		{
			hyd.GET("/bulk", hn.CreateHandler(), MiddlewareAuthenticate)
			hyd.GET("/graph", database.HandleGraphData, MiddlewareAuthenticate)
		}

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

		onetime := api.Group("/onetime")
		{
			onetime.POST("/generate", MiddlewareAuthenticate, HandleGenerateOnetimeCode)
			onetime.POST("/clear", MiddlewareAuthenticate, HandleClearOnetimeCode)
			onetime.POST("/list", MiddlewareAuthenticate, HandleListOnetimeCodes)
			onetime.POST("/use", handleUseOnetimeCode)

		}
		camera := api.Group("/camera", MiddlewareAuthenticate)
		{
			camera.POST("/move", HandleCameraControl)
			camera.POST("/dynamic_move", HandleCameraControlDynamic)
			camera.GET("/relay", handleRelayServer)
		}
		rtc := api.Group("/rtc", MiddlewareAuthenticate)
		{
			rtc.GET("/stunturn", handleCloudflareStunTurn)
		}

		//MiddlewareAuthenticate
		api.POST("/netscan", MiddlewareAuthenticate, handleScanNetworkAddresses)

		api.GET("/conn", handleConnServer)
		api.POST("/stream", handleStreamInput)
	}

	r.Run(":5000")
}

func captureHeapProfile(filename string) {
	os.Remove(filename)
	f, err := os.Create(filename)
	if err != nil {
		log.Fatal(err)
	}
	defer f.Close()
	pprof.WriteHeapProfile(f)
}

func writeHeapProfile(c *gin.Context) {
	captureHeapProfile("heap.pprof")
	c.JSON(200, "success")
}

func HandleListClips(c *gin.Context) {

	clips, err := os.ReadDir("webm-clips")
	if err != nil {
		c.JSON(400, "could not generate file list")
		return
	}

	var output []database.Clip

	for i, clip := range clips {

		info, err := clip.Info()
		if err != nil {
			c.JSON(400, "could not get clip info")
			return
		}

		output = append(output, database.Clip{
			ID:        uint(i),
			Name:      clip.Name(),
			Timestamp: info.ModTime(),
		})
	}

	c.JSON(200, output)
}

var (
	fileLocks = make(map[string]*sync.Mutex)
	mapLock   sync.Mutex
)

func HandleDownloadClip(c *gin.Context) {
	name := c.Query("name")

	mapLock.Lock()

	if fileLocks[name] == nil {
		fileLocks[name] = &sync.Mutex{}
	}
	fileLocks[name].Lock()
	mapLock.Unlock()
	defer fileLocks[name].Unlock()

	file, err := os.Open("webm-clips/" + name)
	if err != nil {
		c.JSON(400, "file open error")
		return
	}

	stat, err := file.Stat()
	if err != nil {
		c.JSON(400, "file stat error")
		return
	}

	defer file.Close()

	c.Header("Content-Type", "video/webm")
	c.Header("Content-Length", strconv.FormatInt(stat.Size(), 10))

	http.ServeContent(c.Writer, c.Request, name, stat.ModTime(), file)
}

type IceServerResponse struct {
	IceServers IceServers `json:"iceServers"`
}

type IceServers struct {
	URLS       []string `json:"urls"`
	Username   string   `json:"username"`
	Credential string   `json:"credential"`
}

func FetchCloudflareTurnServers() (iceResponse IceServerResponse, err error) {
	cloudflareIdToken := os.Getenv("CLOUDFLARE_TURN_TOKEN_ID")
	cloudflareApiToken := os.Getenv("CLOUDFLARE_TURN_TOKEN_API")
	url := fmt.Sprintf("https://rtc.live.cloudflare.com/v1/turn/keys/%s/credentials/generate", cloudflareIdToken)

	// Define request body
	data := map[string]interface{}{
		"ttl": 86400,
	}
	jsonData, err := json.Marshal(data)
	if err != nil {
		fmt.Println("Error marshalling JSON:", err)
		return
	}

	req, err := http.NewRequest("POST", url, bytes.NewBuffer(jsonData))
	if err != nil {
		fmt.Println("Error creating request:", err)
		return
	}
	req.Header.Set("Authorization", "Bearer "+cloudflareApiToken)
	req.Header.Set("Content-Type", "application/json")

	client := &http.Client{}
	resp, err := client.Do(req)
	if err != nil {
		fmt.Println("Error making request:", err)
		return
	}
	defer resp.Body.Close()

	body, err := io.ReadAll(resp.Body)
	if err != nil {
		fmt.Println("Error reading request body:", err)
		return
	}

	err = json.Unmarshal(body, &iceResponse)
	if err != nil {
		fmt.Println("Error unmarshalling response:", err)
		return
	}

	return
}

var (
	CloudflareIceServers IceServers
	CloudflareMutex      sync.Mutex
)

func StartStunTurnRunner() {

	go func() {
		for {
			iceResponse, err := FetchCloudflareTurnServers()
			if err != nil {
				fmt.Println(err.Error())
			} else {
				CloudflareMutex.Lock()
				CloudflareIceServers = iceResponse.IceServers
				fmt.Println(CloudflareIceServers)
				CloudflareMutex.Unlock()
			}

			time.Sleep(time.Minute * 30)
		}
	}()

}

func handleCloudflareStunTurn(c *gin.Context) {
	c.JSON(200, CloudflareIceServers)
}

var (
	OnetimeMutex sync.Mutex
	OnetimeCodes []string
)

func HandleGenerateOnetimeCode(c *gin.Context) {
	code := c.Query("code")
	if code == "" {
		c.JSON(400, "no code supplied")
		return
	}
	OnetimeMutex.Lock()
	defer OnetimeMutex.Unlock()

	OnetimeCodes = append(OnetimeCodes, code)

	c.JSON(200, "added")
}

func HandleClearOnetimeCode(c *gin.Context) {
	OnetimeMutex.Lock()
	defer OnetimeMutex.Unlock()

	OnetimeCodes = []string{}

	c.JSON(200, "cleared")
}

func HandleListOnetimeCodes(c *gin.Context) {
	if OnetimeCodes != nil {
		c.JSON(200, OnetimeCodes)
		return
	} else {
		c.JSON(200, []any{})
		return
	}
}

func handleUseOnetimeCode(c *gin.Context) {
	code := c.Query("code")
	if code == "" {
		c.JSON(400, "no code supplied")
		return
	}
	OnetimeMutex.Lock()
	defer OnetimeMutex.Unlock()

	codesCopy := []string{}

	var found bool

	for _, c := range OnetimeCodes {
		if c == code {
			found = true
		} else {
			codesCopy = append(codesCopy, c)
		}
	}

	if !found {
		c.JSON(400, "invalid code")
		return
	}

	err := ToggleDoor()
	if err != nil {
		c.JSON(400, "could not toggle door")
		return
	}

	OnetimeCodes = codesCopy

	c.JSON(200, "done")

}

func MiddlewareAuthenticate(c *gin.Context) {
	if c.Request.Method == "GET" {
		fmt.Println("GET request")
		MiddlewareAuthenticateGET(c)
	} else {
		MiddlewareAuthenticatePOST(c)
	}
}

func MiddlewareAuthenticatePOST(c *gin.Context) {

	payload := struct {
		DoorCode string `json:"doorCode"`
	}{}

	err := c.BindJSON(&payload)
	if err != nil {
		c.JSON(400, gin.H{"response": "unable to parse JSON"})
		c.Abort()
		return
	}

	secret_door_code := os.Getenv("SECRET_DOOR_CODE")

	result := subtle.ConstantTimeCompare([]byte(secret_door_code), []byte(payload.DoorCode))

	if result == 0 {
		c.JSON(400, gin.H{"response": "incorrect code"})
		c.Abort()
		return
	}
	c.Next()
}
func MiddlewareAuthenticateGET(c *gin.Context) {

	token := c.Query("doorCode")

	secret_door_code := os.Getenv("SECRET_DOOR_CODE")

	result := subtle.ConstantTimeCompare([]byte(secret_door_code), []byte(token))

	fmt.Println("comapring door code")

	if result == 0 {
		c.JSON(400, gin.H{"response": "incorrect code"})
		c.Abort()
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

func SerialSendRecLine(input string) (output string, err error) {
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

	return line, nil

}

func SerialSend(input string) (err error) {
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

	return
}

func MakeToggleTrackerRoute(t *tracker.Tracker) func(c *gin.Context) {
	return func(c *gin.Context) {
		t.Toggle()
		c.JSON(200, "toggled")
	}
}

func MakeTrackerStatusRoute(t *tracker.Tracker) func(c *gin.Context) {
	return func(c *gin.Context) {
		c.JSON(200, t.Active)
	}
}

func MakeClipperToggleRoute(t *tracker.Tracker, cl *clipper.Clipper) func(c *gin.Context) {
	return func(c *gin.Context) {

		t.Mutex.Lock()
		if t.AciveClipping {
			t.AciveClipping = false
			cl.Mutex.Lock()
			if cl.Clipping {
				cl.Clipping = false
			}
			cl.Mutex.Unlock()
		} else {
			t.AciveClipping = true
		}
		t.Mutex.Unlock()

	}
}

func MakeClipperStatusRoute(t *tracker.Tracker) func(c *gin.Context) {
	return func(c *gin.Context) {
		c.JSON(200, t.AciveClipping)
	}
}

func MakeClipperClippingRoute(cl *clipper.Clipper) func(c *gin.Context) {
	return func(c *gin.Context) {
		cl.Mutex.Lock()
		c.JSON(200, cl.Clipping)
		cl.Mutex.Unlock()
	}
}

func GetTrackerSpeed(c *gin.Context) {
	speed, err := SerialSendRecLine("v")
	if err != nil {
		c.JSON(400, "could not get tracker speed")
		return
	}

	c.JSON(200, speed)
}

func SetTrackerSpeed(c *gin.Context) {
	speed := c.Query("speed")
	if speed == "" {
		c.JSON(400, gin.H{"response": "no speed found"})
		return
	}

	value, err := strconv.Atoi(speed)
	if err != nil {
		c.JSON(400, "invalid number")
		return
	}

	if !(value >= 50 && value <= 800) {
		c.JSON(400, "invalid number")
		return
	}

	command := fmt.Sprintf("V%dV", value)

	err = SerialSend(command)
	if err != nil {
		c.JSON(400, "command could not execute")
		return
	}

	c.JSON(200, "updated speed")

}

var (
	ShareCount_FRAME_EXIT   int
	ShareCount_FRAME_CENTER int
	ShareTimeLock           sync.Mutex
)

func TrackerRunner(y, x int) {
	// center coords
	// only tracking off x coord

	noDetection := (x == 0) && (y == 0)

	LEFT_RIGHT_PADDING := 93
	EXIT_FRAME_LIMIT := 11

	// handle none detection
	if noDetection {
		ShareTimeLock.Lock()
		ShareCount_FRAME_EXIT += 1
		ShareTimeLock.Unlock()

		if ShareCount_FRAME_EXIT >= EXIT_FRAME_LIMIT {
			_ = SerialSend("Q")
		}

		return
	}

	lower := LEFT_RIGHT_PADDING
	upper := 320 - LEFT_RIGHT_PADDING

	// Commands are inverted FML
	command := ""
	if x < lower {
		//move left
		command = "P"
		ShareTimeLock.Lock()
		ShareCount_FRAME_EXIT = 0
		ShareCount_FRAME_CENTER = 0
		ShareTimeLock.Unlock()
	} else if x > upper {
		// move right
		command = "S"
		ShareTimeLock.Lock()
		ShareCount_FRAME_EXIT = 0
		ShareCount_FRAME_CENTER = 0
		ShareTimeLock.Unlock()
	} else {
		// we are in the center
		command = "Q"
	}

	if command != "" {
		_ = SerialSend(command) // ignore the error
	}

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

	address := c.Query("address")
	if address == "" {
		c.JSON(400, gin.H{"response": "no address found"})
		return
	}

	AddressMutex.Lock()
	defer AddressMutex.Unlock()

	for _, addy := range Addresses {
		if addy == address {
			c.JSON(200, "added address")
			return
		}
	}

	Addresses = append(Addresses, address)

	c.JSON(200, "added address")

}

func handleRemoveAddress(c *gin.Context) {
	address := c.Query("address")
	if address == "" {
		c.JSON(400, gin.H{"response": "no address found"})
		return
	}

	AddressMutex.Lock()
	defer AddressMutex.Unlock()
	if address == "192.168.1.154" {
		c.JSON(200, "cant remove aidan's address")
		return
	}

	var newAddresses []string
	for _, a := range Addresses {
		if a != address {
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
	if status != "O" {
		ToggleDoor()
	}
}

func doUnlock() {
	status, _ := SerialSendRec("s")
	if status != "L" {
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

	pinger.Count = 10
	pinger.Interval = time.Second / 10
	pinger.Timeout = time.Second / 5

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

// nmap -sn 192.168.1.0/24 | grep "Nmap scan report for"
func handleScanNetworkAddresses(c *gin.Context) {
	addresses, err := ScanNetwork()
	if err != nil {
		c.JSON(400, "could not nmap")
		return
	}
	if len(addresses) == 0 {
		c.JSON(200, []string{})
		return
	}
	c.JSON(200, addresses)
}

func ScanNetwork() (addresses []string, err error) {
	cmd := exec.Command("nmap", "-sn", "192.168.1.0/24")
	output, err := cmd.Output()
	if err != nil {
		return
	}

	scanner := bufio.NewScanner(strings.NewReader(string(output)))

	for scanner.Scan() {
		row := scanner.Text()

		if bytes.Contains([]byte(row), []byte("Nmap scan report for ")) {
			sliced := bytes.Split(bytes.Split([]byte(row), []byte("("))[1], []byte(")"))[0]

			addresses = append(addresses, string(sliced))
		}
	}

	return
}

func HandleCameraControl(c *gin.Context) {
	direction := c.Query("direction")
	if direction == "" {
		c.JSON(400, "no dir supplied")
		return
	}

	if direction == "l" || direction == "r" {
	} else {
		c.JSON(400, "bad dir supplied")
		return
	}

	err := SerialSend(direction)
	if err != nil {
		c.JSON(400, "bad conn")
		return
	}

	c.JSON(200, "sent direction")

}

func HandleCameraControlDynamic(c *gin.Context) {
	HARD_LIMIT := 4000
	controlCommand := c.Query("controlCommand")

	if controlCommand == "" {
		c.JSON(400, "invalid command")
		return
	}

	fmt.Println(controlCommand)

	first := controlCommand[0]
	last := controlCommand[len(controlCommand)-1]

	if first != last {
		c.JSON(400, "invalid command")
		return
	}

	fmt.Println(controlCommand)

	valid := "LR"

	if !strings.Contains(valid, string(first)) {
		c.JSON(400, "invalid command")
		return
	}
	fmt.Println(controlCommand)

	middle := controlCommand[1 : len(controlCommand)-1]

	numberValue, err := strconv.Atoi(middle)
	if err != nil {
		c.JSON(400, "invalid number")
		return
	}

	if numberValue > HARD_LIMIT {
		c.JSON(400, "number too large")
		return
	}

	err = SerialSend(controlCommand)
	if err != nil {
		c.JSON(400, "could not send serial")
		return
	}

}

// webRTC stuff

// relay server
var (
	RtcMutex   = sync.Mutex{}
	RtcClients = make(map[string]*Connection)
)

type Connection struct {
	WebsocketConn *websocket.Conn
	WebrtcConn    *webrtc.PeerConnection
}

var (
	AudioMediaMap = make(map[string]RTPMediaStreamer)
	VideoMediaMap = make(map[string]RTPMediaStreamer)
	MediaMutex    = sync.Mutex{}
)

func handleRelayServer(c *gin.Context) {
	fmt.Println("upgrading")
	conn, err := Upgrader.Upgrade(c.Writer, c.Request, nil)
	if err != nil {
		fmt.Println(err)
		return
	}
	defer conn.Close()

	fmt.Println("upgraded")

	id, err := uuid.NewUUID()
	if err != nil {
		return
	}
	clientId := id.String()

	RtcMutex.Lock()
	myC := Connection{
		WebsocketConn: conn,
		WebrtcConn:    &webrtc.PeerConnection{},
	}
	RtcClients[clientId] = &myC
	RtcMutex.Unlock()

	defer func() {
		RtcMutex.Lock()
		delete(RtcClients, clientId)
		RtcMutex.Unlock()
	}()

	for {
		msgType, msg, err := conn.ReadMessage()
		if err != nil {
			break
		}

		if msgType == websocket.TextMessage {
			var offer struct {
				Type          string `json:"type"`
				Sdp           string `json:"sdp,omitempty"`
				Candidate     string `json:"candidate,omitempty"`
				SdpMid        string `json:"sdpMid,omitempty"`
				SdpMLineIndex uint16 `json:"sdpMLineIndex,omitempty"`
			}
			err := json.Unmarshal(msg, &offer)
			if err != nil {
				fmt.Println("Error unmarshaling message:", err)
				continue
			}
			if offer.Type == "offer" {
				// Create a WebRTC offer object
				webrtcOffer := webrtc.SessionDescription{
					SDP:  offer.Sdp,
					Type: webrtc.SDPTypeOffer,
				}

				// Initialize PeerConnection (if not already done)\
				rid, err := uuid.NewUUID()
				if err != nil {
					return
				}
				rtcId := rid.String()
				defer func() {
					MediaMutex.Lock()
					delete(AudioMediaMap, rtcId)
					delete(VideoMediaMap, rtcId)
					MediaMutex.Unlock()
				}()

				peerConnection, err := initPeerConnection(clientId, webrtcOffer, rtcId)
				if err != nil {
					fmt.Println("Error initializing PeerConnection:", err)
					continue
				}

				RtcMutex.Lock()
				RtcClients[clientId].WebrtcConn = peerConnection
				RtcMutex.Unlock()

				// Process the offer (e.g., set it on a PeerConnection)
				// fmt.Println("Received WebRTC offer:", webrtcOffer.SDP)
			} else if offer.Type == "candidate" {
				candidate := webrtc.ICECandidateInit{
					Candidate:     offer.Candidate,
					SDPMid:        &offer.SdpMid,
					SDPMLineIndex: &offer.SdpMLineIndex,
				}
				RtcMutex.Lock()
				err := RtcClients[clientId].WebrtcConn.AddICECandidate(candidate)
				RtcMutex.Unlock()
				if err != nil {
					fmt.Println("Error adding ice candidate:", err)
					continue
				}
			} else if offer.Type == "ping" {
				conn.WriteMessage(websocket.TextMessage, []byte("pong"))
			} else {
				fmt.Println("Received unsupported message type")
			}

		}

	}

}

func initPeerConnection(clientId string, offer webrtc.SessionDescription, rtcId string) (*webrtc.PeerConnection, error) {
	configuration := webrtc.Configuration{
		ICEServers: []webrtc.ICEServer{
			// {
			// 	URLs: []string{"stun:stun.l.google.com:19302"}, // Google's public STUN server
			// },
			{
				URLs: []string{"stun:ice.aidan.house"},
			},
			{
				URLs:           []string{"turn:ice.aidan.house"},
				Username:       "aidan",
				Credential:     "88" + os.Getenv("SECRET_DOOR_CODE"),
				CredentialType: webrtc.ICECredentialTypePassword,
			},
			{
				URLs:           []string{"turn:ice.aidan.house?transport=tcp"},
				Username:       "aidan",
				Credential:     "88" + os.Getenv("SECRET_DOOR_CODE"),
				CredentialType: webrtc.ICECredentialTypePassword,
			},
		},
		ICETransportPolicy: webrtc.ICETransportPolicyAll,
	}

	peerConnection, err := webrtc.NewPeerConnection(configuration)
	if err != nil {
		return nil, err
	}

	err = peerConnection.SetRemoteDescription(offer)
	if err != nil {
		return nil, err
	}

	peerConnection.OnICECandidate(func(c *webrtc.ICECandidate) {
		// defer candidatesGathered.Done()
		if c == nil {
			return
		}

		candidateMessage := c.ToJSON() // This will serialize the ICECandidate

		RtcMutex.Lock()
		fmt.Println("ICE CANDIDATE FOUND!!!")
		fmt.Println(candidateMessage)

		// Send the candidate as a WebSocket message to the client
		err := RtcClients[clientId].WebsocketConn.WriteJSON(candidateMessage)
		if err != nil {
			log.Fatal(err)
		}

		// Unlock the mutex after sending the candidate
		RtcMutex.Unlock()

	})

	videoTrack, err := webrtc.NewTrackLocalStaticRTP(
		webrtc.RTPCodecCapability{
			MimeType:  "video/VP8", // Use VP8 instead of H.264
			ClockRate: 90000,       // Standard for VP8
			Channels:  1,           // Single channel for video
			RTCPFeedback: []webrtc.RTCPFeedback{
				{Type: "nack"},
				{Type: "nack", Parameter: "pli"},
			}},
		"video", "rtcVideoStream")
	if err != nil {
		return nil, err
	}
	videoStreamer := CreateMediaStreamer(5005, "0.0.0.0", videoTrack)

	MediaMutex.Lock()
	VideoMediaMap[rtcId] = videoStreamer
	MediaMutex.Unlock()

	_, err = peerConnection.AddTrack(videoTrack)
	if err != nil {
		return nil, err
	}

	// Create an offer first to set remote description (you may need to adjust this depending on your setup)
	_, err = peerConnection.CreateOffer(nil)
	if err != nil {
		return nil, err
	}

	var myAnswerOption webrtc.AnswerOptions
	mySDP, err := peerConnection.CreateAnswer(&myAnswerOption)
	if err != nil {
		return nil, err
	}
	// fmt.Println("answer sdp IS: ", mySDP)

	err = peerConnection.SetLocalDescription(mySDP)
	if err != nil {
		return nil, err
	}

	RtcMutex.Lock()
	err = RtcClients[clientId].WebsocketConn.WriteJSON(mySDP)
	RtcMutex.Unlock()
	if err != nil {
		return nil, err
	}

	return peerConnection, nil

}

type RTPMediaStreamer struct {
	Port        uint16
	Hostname    string
	MimeType    string
	WebRTCTrack *webrtc.TrackLocalStaticRTP
}

func CreateMediaStreamer(Port uint16, Hostname string, WebRTCTrack *webrtc.TrackLocalStaticRTP) RTPMediaStreamer {
	return RTPMediaStreamer{
		Port: Port, Hostname: Hostname, WebRTCTrack: WebRTCTrack,
	}
}

func StreamReader(streamerType string, hostname string, port uint16, auxListeners ...chan rtp.Packet) error {
	var streamMap *map[string]RTPMediaStreamer
	switch streamerType {
	case "video":
		streamMap = &VideoMediaMap
	case "audio":
		streamMap = &AudioMediaMap
	default:
		return errors.New("invalid streamerType")
	}
	address := fmt.Sprintf("%s:%d", hostname, port)
	fmt.Println("starting UDP stream on: " + address)

	udpAddr, err := net.ResolveUDPAddr("udp", address)
	if err != nil {
		log.Fatal(err)
		return err
	}

	conn, err := net.ListenUDP("udp", udpAddr)
	if err != nil {
		log.Fatal(err)
		return err
	}
	defer conn.Close()

	buf := make([]byte, 1500)

	for {
		n, _, err := conn.ReadFromUDP(buf)
		if err != nil {
			log.Printf("Error reading from UDP: %v", err)
			continue
		}

		var packet rtp.Packet

		err = packet.Unmarshal(buf[:n])
		if err != nil {
			log.Printf("Error parsing packet")
			continue
		}

		// send packet to all auxListeners
		for _, channel := range auxListeners {
			channel <- packet
		}

		MediaMutex.Lock()

		for _, mStreamer := range *streamMap {
			err = mStreamer.WebRTCTrack.WriteRTP(&packet)
			if err != nil {
				log.Printf("Error adding pcket to track")
				continue
			}
		}
		MediaMutex.Unlock()

	}

}
