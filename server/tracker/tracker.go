package tracker

import (
	"bufio"
	"fmt"
	"os"
	"sync"
)

type TrackerPacket struct {
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
	t.Active = false
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
		fmt.Println(line)
		if t.Active {
			// do something
		}
	}

}
