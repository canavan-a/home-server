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

	frameReader.Run()

	frames := [][]byte{}

	start := time.Now()

	for frame := range dataChannel {
		fmt.Println("frame aquired")
		frames = append(frames, frame)
		if time.Since(start) < 5*time.Second {
			break
		}
	}

	clipper.SaveToRawFile(frames, "test.raw")

}
