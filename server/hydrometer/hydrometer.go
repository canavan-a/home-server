package hydrometer

import (
	"fmt"
	"io"
	"net/http"
	"strconv"
	"sync"
	"time"

	"github.com/gin-gonic/gin"
)

type PushStack []int

func NewPushStack(size int) PushStack {
	return make([]int, size)
}

func (ps *PushStack) PushItem(item int) {
	copy := append(*ps, item)
	*ps = copy[1:]
}

type Hydrometer struct {
	ID             int
	IP             string
	Name           string
	WaterThreshold int
	CurrentValue   int
	Connected      bool
	PushStack      PushStack
}

type HydrometerNetwork struct {
	Nodes []Hydrometer
	Mutex sync.Mutex
}

func NewHydrometerNetwork() (hn HydrometerNetwork) {
	PUSH_STACK_SIZE := 100

	h0 := Hydrometer{
		ID:             0,
		IP:             "192.168.1.160",
		Name:           "Dieffenbachia",
		WaterThreshold: 2500,
		Connected:      false,
		PushStack:      NewPushStack(PUSH_STACK_SIZE),
	}
	h1 := Hydrometer{
		ID:             1,
		IP:             "192.168.1.161",
		Name:           "Money Tree",
		WaterThreshold: 2500,
		Connected:      false,
		PushStack:      NewPushStack(PUSH_STACK_SIZE),
	}

	return HydrometerNetwork{
		Nodes: []Hydrometer{h0, h1},
		Mutex: sync.Mutex{},
	}

}

func (hn *HydrometerNetwork) StartPolling() {

	go func() {

		for {
			for i := range hn.Nodes {
				iVal := i
				hn.Nodes[iVal].Poll(&hn.Mutex)

			}
			time.Sleep(time.Millisecond * 100)

		}
	}()

}

func (h *Hydrometer) Poll(mut *sync.Mutex) {
	retryCount := 5

	var (
		resp *http.Response
		err  error
	)

	for {
		resp, err = http.Get("http://" + h.IP + "/hydrometer")
		if err != nil {
			retryCount -= 1
		} else {
			mut.Lock()
			h.Connected = true
			mut.Unlock()
			break
		}
		if retryCount == 0 {
			mut.Lock()

			h.Connected = false
			mut.Unlock()
			return
		}
	}

	output, err := io.ReadAll(resp.Body)
	if err != nil {
		mut.Lock()
		h.Connected = false
		mut.Unlock()
	}

	val, err := strconv.Atoi(string(output))
	if err != nil {
		mut.Lock()
		h.Connected = false
		mut.Unlock()
	}
	mut.Lock()
	h.CurrentValue = val
	h.PushStack.PushItem(val)
	mut.Unlock()

	// store item
	reading := Reading{
		PlantID:   h.ID,
		Timestamp: time.Now(),
		Value:     val,
	}

	go StoreReading(reading)

}

func StoreReading(reading Reading) {
	db, err := connect()
	if err != nil {
		fmt.Println("could not connect")
		return
	}

	err = InsertReading(db, reading)
	if err != nil {
		fmt.Println("could not insert")
		return
	}
}

func (hn *HydrometerNetwork) CreateHandler() func(c *gin.Context) {

	return func(c *gin.Context) {

		c.JSON(200, hn.Nodes)
	}

}

func HandleGraphData(c *gin.Context) {
	db, err := connect()
	if err != nil {
		c.JSON(400, "error connecting to db")
		return
	}

	plantId, err := strconv.Atoi(c.Query("plant"))
	if err != nil {
		c.JSON(400, "error converting")
		return
	}

	count, err := strconv.Atoi(c.Query("count"))
	if err != nil {
		c.JSON(400, "error converting")
		return
	}

	hours, err := strconv.Atoi(c.Query("hours"))
	if err != nil {
		c.JSON(400, "error converting")
		return
	}

	startDate := time.Now().Add(time.Duration(-hours) * time.Hour)

	readings, err := GetSpacedReadings(db, plantId, count, startDate)
	if err != nil {
		c.JSON(400, "error fetching")
		return
	}

	c.JSON(200, readings)

}
