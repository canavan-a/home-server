package rawframe

import (
	"encoding/binary"
	"fmt"
	"net"
	"os"
	"sync"
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
			continue
		}

		totalBytes += bytesRead

		// Keep reading until we have a complete frame
		if totalBytes >= frameSize {
			// Copy complete frame

			fullFrame := buffer[:frameSize]

			fr.OutputChannel <- fullFrame

			// Shift remaining bytes (if any) to start of buffer
			remaining := totalBytes - frameSize
			if remaining > 0 {
				copy(buffer, buffer[frameSize:totalBytes])
			}
			totalBytes = remaining

		}
	}

}

func (fr *FrameReader) RunUdp(udpUri string) {
	udpAddr, err := net.ResolveUDPAddr("udp", udpUri)
	if err != nil {
		panic(err)
	}

	conn, err := net.ListenUDP("udp", udpAddr)
	if err != nil {
		panic(err)
	}
	defer conn.Close()

	const maxUDPSize = 65507
	const frameSize = 640 * 480 * 3

	type FrameBuffer struct {
		mu       sync.Mutex
		chunks   map[int][]byte
		expected int
		received int
	}

	buffers := make(map[int]*FrameBuffer)

	for {
		// Receive buffer
		buffer := make([]byte, maxUDPSize+12)
		n, _, err := conn.ReadFromUDP(buffer)
		if err != nil || n < 12 { // Ensure the packet has at least the 12-byte header
			if n < 12 {
				fmt.Println("Invalid packet received: Packet too small")
			}
			continue
		}

		// Extract frame metadata
		frameID := int(binary.BigEndian.Uint32(buffer[0:4]))
		chunkID := int(binary.BigEndian.Uint32(buffer[4:8]))
		totalChunks := int(binary.BigEndian.Uint32(buffer[8:12]))
		data := buffer[12:n]

		// Initialize buffer for new frame
		if _, exists := buffers[frameID]; !exists {
			buffers[frameID] = &FrameBuffer{
				chunks:   make(map[int][]byte),
				expected: totalChunks,
			}
		}

		// Store chunk
		fb := buffers[frameID]
		fb.mu.Lock()
		fb.chunks[chunkID] = data
		fb.received++
		fb.mu.Unlock()

		// If frame is complete, reassemble and send to OutputChannel
		if fb.received == fb.expected {
			assembled := make([]byte, 0, frameSize)
			for i := 0; i < totalChunks; i++ {
				assembled = append(assembled, fb.chunks[i]...)
			}

			// Send complete frame
			fr.OutputChannel <- assembled
			fmt.Println("Frame received:", len(assembled), "bytes")

			// Remove old frame from buffer
			delete(buffers, frameID)
		}
	}
}
