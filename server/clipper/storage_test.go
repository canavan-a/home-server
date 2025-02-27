package clipper_test

import (
	"fmt"
	"io"
	"log"
	"main/clipper"
	"os"
	"os/exec"
	"testing"
	"time"
)

func Test_store(t *testing.T) {

	fmt.Println("start ewewee  wqw w q Sdsdsfsfd dsd")

	packets, err := createFrameSample()
	if err != nil {
		t.Error(err.Error())
	}

	fmt.Println(len(packets))

	err = clipper.Store(packets)
	if err != nil {
		panic(err)
	}
	//create some rtp packets

}

func createFrameSample() (frames [][]byte, err error) {
	// FFmpeg command for 640x480 BGR24 raw video
	cmd := exec.Command("ffmpeg", "-f", "dshow", "-i", "video=FHD Camera", "-pix_fmt", "bgr24", "-s", "640x480", "-f", "rawvideo", "pipe:1")

	// Set up stdout pipe
	cmdOut, err := cmd.StdoutPipe()
	if err != nil {
		log.Fatalf("Error creating StdoutPipe: %v", err)
	}

	// Start FFmpeg
	if err = cmd.Start(); err != nil {
		log.Fatalf("Error starting command: %v", err)
	}
	defer func() {
		if err := cmd.Process.Kill(); err != nil {
			log.Printf("Warning: failed to kill FFmpeg: %v", err)
		}
	}()

	// Frame size: 640 * 480 * 3 (BGR24) = 921,600 bytes
	const frameSize = 640 * 480 * 3
	frameCount := 0

	// Buffer for assembling frames
	fullFrame := make([]byte, 0, frameSize) // Start empty, capacity = frameSize
	chunk := make([]byte, 65536)            // 64KB chunks

	start := time.Now()
	for time.Since(start) < 5*time.Second {
		n, err := cmdOut.Read(chunk)
		if err == io.EOF {
			break
		}
		if err != nil {
			log.Fatalf("Error reading from command output: %v", err)
		}

		// Append chunk to fullFrame
		fullFrame = append(fullFrame, chunk[:n]...)

		// Process all complete frames in the buffer
		for len(fullFrame) >= frameSize {
			frameCopy := make([]byte, frameSize)
			copy(frameCopy, fullFrame[:frameSize])

			frames = append(frames, frameCopy)
			frameCount++
			fmt.Printf("Captured frame %d, size: %d bytes\n", frameCount, frameSize)

			// Remove the processed frame, keep leftovers
			fullFrame = fullFrame[frameSize:]
		}
	}

	fmt.Printf("Total frames captured: %d\n", frameCount)
	return frames, nil
}

func TestDeleteWebm(t *testing.T) {
	err := os.Remove("temp.webm")
	if err != nil {
		panic(err)
	}

}
