package rawframe

import (
	"fmt"
	"io"
	"log"
	"os"
)

type FrameReader struct {
	FifoPath      string
	OutputChannel chan []byte
}

func NewFrameReader(path string, output chan []byte) *FrameReader {
	return &FrameReader{
		FifoPath:      path,
		OutputChannel: output,
	}
}

func (fr *FrameReader) Run() {
	pipe, err := os.Open(fr.FifoPath)
	if err != nil {
		// could not open pipe
		panic(err)
	}

	defer pipe.Close()

	frameSize := 640 * 480 * 3 // 921,600 bytes for BGR24 640x480
	frameCount := 0

	// Buffer for one full frame
	fullFrame := make([]byte, frameSize)
	bytesRead := 0

	for {
		// Temporary buffer for partial reads
		chunk := make([]byte, 65536) // 64KB chunks
		n, err := pipe.Read(chunk)
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
			fr.OutputChannel <- frameCopy
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

}
