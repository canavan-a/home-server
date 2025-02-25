package rawframe

import (
	"bytes"
	"compress/zlib"
	"io"
)

type RawFrameTool struct {
}

func Compress(frame []byte) ([]byte, error) {

	var compressedBuffer bytes.Buffer
	writer := zlib.NewWriter(&compressedBuffer)

	// Write the raw frame value into the zlib writer to compress it
	_, err := writer.Write(frame)
	if err != nil {
		return nil, err
	}

	// Close the writer to finalize compression
	writer.Close()

	// Return the compressed data
	return compressedBuffer.Bytes(), nil
}

func Decompress(compressedFrame []byte) ([]byte, error) {
	// Create a buffer for the compressed data
	compressedBuffer := bytes.NewBuffer(compressedFrame)

	// Create a zlib reader
	reader, err := zlib.NewReader(compressedBuffer)
	if err != nil {
		return nil, err
	}
	defer reader.Close()

	// Create a buffer to hold the decompressed data
	var decompressedBuffer bytes.Buffer

	// Copy the decompressed data into the buffer
	_, err = io.Copy(&decompressedBuffer, reader)
	if err != nil {
		return nil, err
	}

	// Return the decompressed data
	return decompressedBuffer.Bytes(), nil
}
