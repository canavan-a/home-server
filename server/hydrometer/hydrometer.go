package hydrometer

import (
	"fmt"
	"io"
	"net/http"
	"strconv"
	"sync"
	"time"

	"github.com/gin-gonic/gin"
	"gorm.io/gorm"
)

type PushStack []int

func NewPushStack(size int) PushStack {
	return make([]int, size)
}

func (ps *PushStack) PushItem(item int) {
	copy := append(*ps, item)
	*ps = copy[1:]
}

func (ps *PushStack) Average() (average int) {
	sum := 0
	valid := 0
	for _, item := range *ps {
		if item != 0 {
			sum += item
			valid++
		}
	}
	if valid != 0 {
		return sum / valid
	} else {
		return 0
	}
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

var (
	DB *gorm.DB
)

func NewHydrometerNetwork() (hn HydrometerNetwork) {

	// init db connection
	db, err := connect()
	if err != nil {
		panic(err)
	}
	// init db
	DB = db

	PUSH_STACK_SIZE := 100

	h0 := Hydrometer{
		ID:             0,
		IP:             "192.168.1.160",
		Name:           "Dieffenbachia",
		WaterThreshold: 2100,
		Connected:      false,
		PushStack:      NewPushStack(PUSH_STACK_SIZE),
	}
	h1 := Hydrometer{
		ID:             1,
		IP:             "192.168.1.161",
		Name:           "Money Tree",
		WaterThreshold: 2100,
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
		Value:     h.PushStack.Average(),
	}

	go StoreReading(reading)

}

func StoreReading(reading Reading) {

	txn := DB.Begin()

	err := InsertReading(txn, reading)
	if err != nil {
		txn.Rollback()
		fmt.Println("could not insert")
		return
	}

	txn.Commit()
}

func (hn *HydrometerNetwork) CreateHandler() func(c *gin.Context) {

	return func(c *gin.Context) {

		c.JSON(200, hn.Nodes)
	}

}

func HandleGraphData(c *gin.Context) {

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

	txn := DB.Begin()

	readings, err := GetSpacedReadings(txn, plantId, count, startDate)
	if err != nil {
		c.JSON(400, "error fetching")
		return
	}

	txn.Commit()

	c.JSON(200, readings)

}
