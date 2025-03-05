package rawframe_test

import (
	"fmt"
	"main/clipper"
	"main/rawframe"
	"testing"
	"time"
)

func TestRawFrameReading(t *testing.T) {

	dataChannel := make(chan []byte)

	frameReader := rawframe.NewFrameReader("/tmp/raw_frame", dataChannel)

	go frameReader.Run()

	frames := [][]byte{}

	start := time.Now()

	for frame := range dataChannel {
		fmt.Println("frame aquired")
		frames = append(frames, frame)
		if time.Since(start) > 5*time.Second {
			break
		}
	}

	clipper.SaveToRawFile(frames, "test.raw")

}

func TestRawFrameReadingUdp(t *testing.T) {

	address := "localhost:999"

	dataChannel := make(chan []byte)

	frameReader := rawframe.NewFrameReader("testtesttest", dataChannel)

	go frameReader.RunUdp(address)

	frames := [][]byte{}

	start := time.Now()

	for frame := range dataChannel {
		fmt.Println("frame aquired")
		frames = append(frames, frame)
		if time.Since(start) > 5*time.Second {
			break
		}
	}

	// ffmpeg -f rawvideo -pix_fmt bgr24 -s 640x480 -i test.raw -c:v libvpx -crf 10 -b:v 1M output.webm

	clipper.SaveToRawFile(frames, "test.raw")

}
