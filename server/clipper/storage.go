package clipper

import (
	"fmt"

	"github.com/pion/rtp"
	"github.com/pion/webrtc/v4/pkg/media/ivfwriter"
)

type ClipStorageDevice struct {
	StorageChannel chan []rtp.Packet
}

func NewClipStorageDevice() *ClipStorageDevice {
	return &ClipStorageDevice{
		StorageChannel: make(chan []rtp.Packet),
	}
}

func Store(packets []rtp.Packet) {

	// Create a new OGG writer
	writer, err := ivfwriter.New("output.ivf", ivfwriter.WithCodec("VP8"))
	if err != nil {
		fmt.Println("Error creating IVF writer:", err)
		return
	}

	defer writer.Close()

	for _, packet := range packets {
		writer.WriteRTP(&packet)

	}
	fmt.Println("IVF file created: output.ivf")
}

func (csd *ClipStorageDevice) Run() {
	for packets := range csd.StorageChannel {
		Store(packets)
	}
}
