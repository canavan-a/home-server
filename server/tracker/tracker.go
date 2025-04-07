package tracker

import (
	"bufio"
	"encoding/json"
	"os"
	"sync"
)

type TrackerPacket struct {
	// [{"person_index": 0, "ymin": 29, "xmin": 171, "ymax": 179, "xmax": 202}]
	PersonIndex int `json:"person_index"`
	YMin        int `json:"ymin"`
	XMin        int `json:"xmin"`
	YMax        int `json:"ymax"`
	XMax        int `json:"xmax"`
}

type Tracker struct {
	FifoPath      string
	Actions       []func(y, x int)
	Active        bool
	AciveClipping bool
	Mutex         sync.Mutex
}

func NewTracker(fifoPath string, actions ...func(int, int)) (t Tracker) {
	t.FifoPath = fifoPath
	t.Actions = actions
	t.Active = false
	t.AciveClipping = true
	return
}

func (t *Tracker) Toggle() {
	t.Mutex.Lock()
	t.Active = !t.Active
	t.Mutex.Unlock()
}

func (t *Tracker) ToggleClipping() {
	t.Mutex.Lock()
	t.Active = !t.Active
	t.Mutex.Unlock()
}

func (t *Tracker) Run() {
	pipe, err := os.Open(t.FifoPath)
	if err != nil {
		// could not open pipe
		panic(err)
	}

	defer pipe.Close()

	scanner := bufio.NewScanner(pipe)

	for scanner.Scan() {
		line := scanner.Text()

		var packets []TrackerPacket
		_ = json.Unmarshal([]byte(line), &packets)
		y, x := calclulateCenter(packets)
		for i, action := range t.Actions {
			if i == 0 && t.Active { // movement tracking
				action(y, x)

			} else if i == 1 && t.AciveClipping { // clip farming
				action(y, x)
			}
		}

	}

}

func calclulateCenter(packets []TrackerPacket) (y, x int) {

	if len(packets) == 0 {
		return
	}

	for _, packet := range packets {
		vY, vX := packet.Center()
		y += vY
		x += vX
	}

	y = y / len(packets)
	x = x / len(packets)

	return
}

func (tp TrackerPacket) Center() (y, x int) {
	x = (tp.XMax + tp.XMin) / 2
	y = (tp.YMax + tp.YMin) / 2
	return
}
