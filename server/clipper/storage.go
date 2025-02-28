package clipper

import (
	"bytes"
	"fmt"
	"io"
	"main/database"
	"main/rawframe"
	"os"
	"os/exec"
	"time"

	"gorm.io/gorm"
)

type ClipStorageDevice struct {
	StorageChannel chan [][]byte
}

var (
	DB *gorm.DB
)

func NewClipStorageDevice() *ClipStorageDevice {

	db, err := database.Connect()
	if err != nil {
		panic(err)
	}

	DB = db

	return &ClipStorageDevice{
		StorageChannel: make(chan [][]byte),
	}
}

func ListClips() (clips []database.Clip, err error) {
	txn := DB.Begin()
	clips, err = database.ListClips(txn)
	txn.Commit()

	return
}

func DownloadClip(id int) (webmData []byte, err error) {

	txn := DB.Begin()
	clip, err := database.GetClip(txn, id)
	if err != nil {
		return nil, err
	}

	webmData, err = rawframe.Decompress(clip.Clip)
	if err != nil {
		return nil, err
	}

	txn.Commit()

	return
}

func Store(frames [][]byte) error {

	rawFilename := "temp.raw"
	err := SaveToRawFile(frames, rawFilename)
	if err != nil {
		return err
	}
	defer os.Remove("temp.raw")

	outputFilename := "temp.webm"

	err = ConvertFileToWebm(rawFilename, outputFilename)
	if err != nil {
		return err
	}
	defer os.Remove("temp.webm")

	webmFile, err := os.Open(outputFilename)
	if err != nil {
		return err
	}

	rawData, err := io.ReadAll(webmFile)
	if err != nil {
		return err
	}

	err = webmFile.Close()
	if err != nil {
		return err
	}

	compressed, err := rawframe.Compress(rawData)
	if err != nil {
		return err
	}

	txn := DB.Begin()
	err = database.InsertClip(txn, database.Clip{
		Timestamp: time.Now(),
		Clip:      compressed,
	})
	if err != nil {
		return err
	}
	txn.Commit()

	// store this in database
	return nil

}

func ConvertFileToWebm(rawFilename, outputFilename string) error {

	cmd := exec.Command(
		"ffmpeg", "-f", "rawvideo", "-pix_fmt",
		"bgr24", "-s", "640x480",
		"-framerate", "10", "-i",
		rawFilename,
		"-c:v", "libvpx", "-crf", "10", "-b:v", "1M",
		"-g", "3",
		"-auto-alt-ref", "0",
		"-movflags", "faststart",
		"-metadata:s:v:0", "alpha_mode=1",
		outputFilename,
	)
	err := cmd.Start()
	if err != nil {
		return err
	}

	err = cmd.Wait()
	if err != nil {
		return err
	}

	return nil
}

func SaveToRawFile(frames [][]byte, filename string) error {
	// ffmpeg -f rawvideo -pix_fmt bgr24 -s 640x480 -i test.raw -c:v libvpx -crf 10 -b:v 1M output.webm

	var buffer bytes.Buffer
	expectedSize := 640 * 480 * 3 // 921,600 bytes for BGR24 640x480

	for i, frame := range frames {

		fmt.Println("Frame", i, "length:", len(frame))
		if len(frame) != expectedSize {
			return fmt.Errorf("frame %d has invalid size: got %d, expected %d", i, len(frame), expectedSize)
		}

		buffer.Write(frame)
	}

	err := os.WriteFile(filename, buffer.Bytes(), 0644)
	if err != nil {
		return err
	}

	return nil
}

func (csd *ClipStorageDevice) Run() {
	for frames := range csd.StorageChannel {
		err := Store(frames)
		if err != nil {
			fmt.Println("Error storing frames")
		}
	}
}
