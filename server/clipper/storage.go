package clipper

import (
	"bytes"
	"fmt"
	"main/rawframe"
	"os"

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

func Store(frames [][]byte) {
	err := SaveToRawFile(frames, "test.raw")
	if err != nil {
		panic(err)
	}

}

func SaveToRawFile(frames [][]byte, filename string) error {
	// need to store raw data and then convert in ffmpeg to compressed data

	// ffmpeg -f rawvideo -pix_fmt bgr24 -s 640x480 -i test.raw -c:v libvpx -crf 10 -b:v 1M output.webm

	var buffer bytes.Buffer
	expectedSize := 640 * 480 * 3 // 921,600 bytes for BGR24 640x480

	for i, frame := range frames {
		fmt.Println("Frame", i, "length:", len(frame))
		if len(frame) != expectedSize {
			return fmt.Errorf("frame %d has invalid size: got %d, expected %d", i, len(frame), expectedSize)
		}

		compressed, err := rawframe.Compress(frame)
		if err != nil {
			return err
		}

		decompressed, err := rawframe.Decompress(compressed)
		if err != nil {
			return err
		}

		buffer.Write(decompressed)
	}
	return os.WriteFile(filename, buffer.Bytes(), 0644)
}

func (csd *ClipStorageDevice) Run() {
	// for packets := range csd.StorageChannel {
	// 	Store(packets)
	// }
}
