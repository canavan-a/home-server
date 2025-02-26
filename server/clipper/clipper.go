package clipper

import (
	"fmt"
	"main/clipper/fixedsizequeue"
	"slices"
	"sync"
)

// 3 ish packets per frame
// 15 ish FPS
// 10 seconds of prequeue
var (
	DEFAULT_PREQUEUE_SIZE = 15 * 10 // roughly 10 seconds pre clip

	MAX_CLIP_SIZE = 15 * 3 * 60 // roughly 3 minutes of video

	// need 5 consecutive frames with entity to start recording
	FRAMES_TO_START = 4

	// need 90 consecutive frames WITHOUT entity to end recording
	FRAMES_TO_KILL = 20
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
	ClipStorageChannel chan [][]byte
}

func NewClipper(clipStorageChannel chan [][]byte) *Clipper {
	return &Clipper{
		Mutex:              sync.Mutex{},
		PacketChannel:      make(chan []byte),
		PreQueue:           fixedsizequeue.CreateFixedQueue[[]byte](DEFAULT_PREQUEUE_SIZE),
		ClipStorageChannel: clipStorageChannel,
	}

}

func (c *Clipper) ReceiveEntity(y, x int) { // pass this function to the tracker

	// only detected on non zero values
	if x+y == 0 {
		c.Mutex.Lock()
		c.FramesToKill += 1
		c.FramesToStart = 0
		c.Mutex.Unlock()

		c.Mutex.Lock()
		if c.FramesToKill >= FRAMES_TO_KILL && c.Clipping { // break the clip
			fmt.Println("Clipper done, saving")

			// stopre the clip
			c.ClipStorageChannel <- c.Clip
			// stop the clipper
			c.Clipping = false
			// clear the old clip
			c.Clip = [][]byte{}
		}
		c.Mutex.Unlock()

	} else {
		c.Mutex.Lock()
		c.FramesToKill = 0
		c.FramesToStart += 1
		c.Mutex.Unlock()

		c.Mutex.Lock()
		if !c.Clipping && c.FramesToStart >= FRAMES_TO_START {
			fmt.Println("Clipper starting")
			c.Clip = slices.Clone(c.PreQueue.CopyOut())
			c.Clipping = true
		}
		c.Mutex.Unlock()

		c.Mutex.Lock()
		// if clips gets too large save and break
		if len(c.Clip) > MAX_CLIP_SIZE {
			// clip is too big, save
			fmt.Println("Clip has reached maximum size")

			c.ClipStorageChannel <- c.Clip
			c.Clipping = false
			c.Clip = [][]byte{}

			// do not adjust frame counts so we can just continue and get the next clip right after

		}
		c.Mutex.Unlock()

	}

}

func (c *Clipper) Run() {
	for frame := range c.PacketChannel {
		c.Mutex.Lock()
		c.PreQueue.Add(frame)
		if c.Clipping {
			c.Clip = append(c.Clip, frame)
		}
		c.Mutex.Unlock()
	}
}
