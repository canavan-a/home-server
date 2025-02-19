package tracker

import (
	"bufio"
	"encoding/json"
	"fmt"
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
	FifoPath string
	Action   func(TrackerPacket)
	Active   bool
	Mutex    sync.Mutex
}

func NewTracker(fifoPath string, action func(TrackerPacket)) (t Tracker) {
	t.FifoPath = fifoPath
	t.Action = action
	t.Active = true
	return
}

func (t *Tracker) Toggle() {
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
		if t.Active {
			var packets []TrackerPacket
			_ = json.Unmarshal([]byte(line), &packets)

			fmt.Println(packets)
		}
	}

}
