package rawframe

import (
	"fmt"
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
		panic(err)
	}
	defer pipe.Close()

	const frameSize = 640 * 480 * 3 // 921,600 bytes for BGR24 640x480
	fullFrame := make([]byte, frameSize)
	buffer := make([]byte, frameSize)
	totalBytes := 0
	// Slice to store all frames

	for {
		bytesRead, err := pipe.Read(buffer[totalBytes:])
		if err != nil {
			if err.Error() == "EOF" {
				if totalBytes > 0 {
					fmt.Println("Incomplete frame at EOF:", totalBytes, "bytes")
				}
				break
			}
			panic(err)
		}

		totalBytes += bytesRead

		// Keep reading until we have a complete frame
		if totalBytes >= frameSize {
			// Copy complete frame
			copy(fullFrame, buffer[:frameSize])
			frameCopy := make([]byte, frameSize)
			copy(frameCopy, fullFrame)

			compressedCopy, err := Compress(frameCopy)
			if err != nil {
				panic(err)
			}

			fr.OutputChannel <- compressedCopy //send compressed data

			// Shift remaining bytes (if any) to start of buffer
			remaining := totalBytes - frameSize
			if remaining > 0 {
				copy(buffer, buffer[frameSize:totalBytes])
			}
			totalBytes = remaining

			fmt.Println("Sent complete frame")
		}
	}

}
