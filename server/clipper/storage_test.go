package clipper_test

import (
	"fmt"
	"io"
	"log"
	"main/clipper"
	"main/rawframe"
	"os"
	"os/exec"
	"testing"
	"time"
)

func Test_store(t *testing.T) {

	fmt.Println("starting")

	packets, err := createFrameSample()
	if err != nil {
		t.Error(err.Error())
	}

	clipper.Store(packets)
	//create some rtp packets

}

func createFrameSample() (frames [][]byte, err error) {
	// Adjusted FFmpeg command for 640x480
	cmd := exec.Command("ffmpeg", "-f", "dshow", "-i", "video=FHD Camera", "-pix_fmt", "bgr24", "-s", "640x480", "-f", "rawvideo", "pipe:1")

	// Create a buffer to hold the output
	cmdOut, err := cmd.StdoutPipe()
	if err != nil {
		log.Fatalf("Error creating StdoutPipe: %v", err)
	}

	// Start the command
	err = cmd.Start()
	if err != nil {
		log.Fatalf("Error starting command: %v", err)
	}

	// Define buffer to hold frame data
	frameSize := 640 * 480 * 3 // 921,600 bytes for BGR24 640x480
	frameCount := 0

	// Buffer for one full frame
	fullFrame := make([]byte, frameSize)
	bytesRead := 0

	start := time.Now()
	for time.Since(start) < 5*time.Second {
		// Temporary buffer for partial reads
		chunk := make([]byte, 65536) // 64KB chunks
		n, err := cmdOut.Read(chunk)
		if err == io.EOF {
			break
		}
		if err != nil {
			log.Fatalf("Error reading from command output: %v", err)
		}

		// Assemble chunks into full frame
		copy(fullFrame[bytesRead:], chunk[:n])
		bytesRead += n

		if bytesRead >= frameSize {
			// Store completed frame
			frameCopy := make([]byte, frameSize)
			copy(frameCopy, fullFrame[:frameSize])

			compressed, err := rawframe.Compress(frameCopy)
			if err != nil {
				panic(err)
			}

			fmt.Println("appendingasasas as")
			frames = append(frames, compressed)
			frameCount++
			fmt.Printf("Captured frame %d, size: %d bytes\n", frameCount, frameSize)

			// Handle leftovers for next frame
			leftover := bytesRead - frameSize
			if leftover > 0 {
				temp := make([]byte, frameSize)
				copy(temp, fullFrame[frameSize:bytesRead])
				fullFrame = temp
			} else {
				fullFrame = make([]byte, frameSize)
			}
			bytesRead = leftover
		}
	}

	// Stop the command
	err = cmd.Process.Kill()
	if err != nil {
		log.Fatalf("Error stopping command: %v", err)
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
