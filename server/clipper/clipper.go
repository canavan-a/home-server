package clipper

import (
	"main/clipper/fixedsizequeue"
	"sync"

	"github.com/pion/rtp"
)

// 3 ish packets per frame
// 15 ish FPS
// 10 seconds of prequeue
var (
	DEFAULT_PREQUEUE_SIZE = 3 * 15 * 60 * 10

	// need 5 consecutive frames with entity to start recording
	FRAMES_TO_START = 5

	// need 90 consecutive frames WITHOUT entity to end recording
	FRAMES_TO_KILL = 90
)

// Clip Farmer
type Clipper struct {
	Mutex              sync.Mutex
	PacketChannel      chan rtp.Packet
	Clip               []rtp.Packet
	PreQueue           *fixedsizequeue.FixedQueue[rtp.Packet]
	Clipping           bool
	FramesToKill       int
	FramesToStart      int
	ClipStorageChannel chan []rtp.Packet
}

func NewClipper(clipStorageChannel chan []rtp.Packet) *Clipper {
	return &Clipper{
		Mutex:              sync.Mutex{},
		PacketChannel:      make(chan rtp.Packet),
		PreQueue:           fixedsizequeue.CreateFixedQueue[rtp.Packet](DEFAULT_PREQUEUE_SIZE),
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
			// stopre the clip
			c.ClipStorageChannel <- c.Clip
			// stop the clipper
			c.Clipping = false
			// clear the old clip
			c.Clip = []rtp.Packet{}
		}
		c.Mutex.Unlock()

	} else {
		c.Mutex.Lock()
		c.FramesToKill = 0
		c.FramesToStart += 1
		c.Mutex.Unlock()

		c.Mutex.Lock()
		if !c.Clipping && c.FramesToStart >= FRAMES_TO_START {
			c.Clip = append([]rtp.Packet{}, c.PreQueue.CopyOut()...)
			c.Clipping = true
		}
		c.Mutex.Unlock()

	}

}

func (c *Clipper) Run() {
	for packet := range c.PacketChannel {
		c.Mutex.Lock()
		c.PreQueue.Add(packet)
		if c.Clipping {
			c.Clip = append(c.Clip, packet)
		}
		c.Mutex.Unlock()
	}
}
