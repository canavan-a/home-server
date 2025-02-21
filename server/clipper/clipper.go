package clipper

import (
	"sync"

	"github.com/pion/rtp"
)

type Clipper struct {
	Mutex         sync.Mutex
	PacketChannel chan rtp.Packet
}

func (c *Clipper) RunFrame(y, x int) { // pass this function to the tracker

}
