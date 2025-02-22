package clipper

import (
	"fmt"

	"github.com/pion/rtp"
)

type ClipStorageDevice struct {
	StorageChannel chan []rtp.Packet
}

func NewClipStorageDevice() *ClipStorageDevice {
	return &ClipStorageDevice{
		StorageChannel: make(chan []rtp.Packet),
	}
}

func store(packets []rtp.Packet) {
	fmt.Println("STORING PACKET with LENGTH:", len(packets))

}

func (csd *ClipStorageDevice) Run() {
	for packets := range csd.StorageChannel {
		store(packets)
	}
}
