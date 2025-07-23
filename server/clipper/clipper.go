package clipper

import (
	"fmt"
	"main/clipper/fixedsizequeue"
	"sync"

	"github.com/google/uuid"
)

// 3 ish packets per frame
// 15 ish FPS
// 10 seconds of prequeue
var (
	DEFAULT_PREQUEUE_SIZE = 20 * 5 // roughly 9 seconds pre clip

	MAX_CLIP_SIZE = 20 * 5 * 40 // roughly

	// need 5 consecutive frames with entity to start recording
	FRAMES_TO_START = 7

	// need 90 consecutive frames WITHOUT entity to end recording
	FRAMES_TO_KILL = 45
)

// Clip Farmer
type Clipper struct {
	Mutex              sync.Mutex
	PacketChannel      chan []byte
	Clip               [][]byte // compressed bytes
	PreQueue           *fixedsizequeue.FixedQueue[[]byte]
	Clipping           bool
	FramesToKill       int
	FramesToStart      int
	ClipStorageChannel chan string

	ClipLength int

	CurrentFile string
	AreaMinimum int
}

func NewClipper(clipStorageChannel chan string) *Clipper {
	return &Clipper{
		Mutex:              sync.Mutex{},
		PacketChannel:      make(chan []byte),
		PreQueue:           fixedsizequeue.CreateFixedQueue[[]byte](DEFAULT_PREQUEUE_SIZE),
		ClipStorageChannel: clipStorageChannel,
	}

}

func (c *Clipper) ReceiveEntity(y, x, area int) { // pass this function to the tracker

	c.Mutex.Lock()
	if c.ClipLength > MAX_CLIP_SIZE {
		c.ClipLength = 0

		c.ClipStorageChannel <- c.CurrentFile
		// stop the clipper
		c.Clipping = false

	}
	AreaMinimum := c.AreaMinimum
	c.Mutex.Unlock()

	// only detected on non zero values
	if x+y == 0 || area < AreaMinimum {
		c.Mutex.Lock()
		c.FramesToKill += 1
		c.FramesToStart = 0
		c.Mutex.Unlock()

		c.Mutex.Lock()
		if c.FramesToKill >= FRAMES_TO_KILL && c.Clipping { // break the clip
			fmt.Println("Clipper done, saving")
			c.ClipLength = 0
			// stopre the clip
			c.ClipStorageChannel <- c.CurrentFile
			// stop the clipper
			c.Clipping = false
			// clear the old clip
		}

		c.Mutex.Unlock()

	} else {

		c.Mutex.Lock()
		c.FramesToKill = 0
		c.FramesToStart += 1
		c.Mutex.Unlock()

		c.Mutex.Lock()
		if !c.Clipping && c.FramesToStart >= FRAMES_TO_START {
			c.ClipLength = 0
			fmt.Println("Clipper starting")
			c.CurrentFile = "raw-clips/" + uuid.NewString() + ".raw"
			preData := c.PreQueue.CopyOut()
			err := SaveToRawFile(preData, c.CurrentFile)
			if err != nil {
				panic(err)
			}
			c.Clipping = true
		}
		c.Mutex.Unlock()

	}

}

func (c *Clipper) Run() {
	for frame := range c.PacketChannel {
		c.Mutex.Lock()
		c.PreQueue.Add(frame)
		if c.Clipping {
			c.ClipLength++
			err := SaveToRawFile([][]byte{frame}, c.CurrentFile)
			if err != nil {
				fmt.Println(err)
				panic(err)
			}
			// c.Clip = append(c.Clip, frame)
		}
		c.Mutex.Unlock()
	}
}
