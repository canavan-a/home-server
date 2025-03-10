package clipper

import (
	"fmt"
	"main/database"
	"main/mailer"
	"os"
	"os/exec"
	"time"

	"github.com/google/uuid"
	"gorm.io/gorm"
)

type ClipStorageDevice struct {
	StorageChannel chan string
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
		StorageChannel: make(chan string),
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

	webmData = clip.Clip

	txn.Commit()

	return
}

func Store(filePath string) error {

	now := time.Now()

	randomValue := uuid.NewString()

	outputFilenameTemp := fmt.Sprintf("temp-webm-clips/%s.webm", randomValue)

	outputFilename := fmt.Sprintf("webm-clips/%s.webm", randomValue)

	err := ConvertFileToWebm(filePath, outputFilenameTemp)
	if err != nil {
		return err
	}

	err = os.Rename(outputFilenameTemp, outputFilename)
	if err != nil {
		return err
	}

	uri := fmt.Sprintf("https://aidan.house/api/clipper/download?name=%s&doorCode=%s", randomValue+".webm", os.Getenv("SECRET_DOOR_CODE"))
	mailer.Notify(mailer.MakeClipBody(uri, now.Format("January 2, 2006 15:04:05")))

	// store this in database
	return nil

}

func ConvertFileToWebm(rawFilename, outputFilename string) error {

	cmd := exec.Command(
		"ffmpeg", "-f", "rawvideo", "-pix_fmt",
		"bgr24", "-s", "1280x720",
		"-framerate", "16", "-i",
		rawFilename,
		"-c:v", "libvpx", "-crf", "10", "-b:v", "3M",
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
	// Open the file in append mode, create it if it doesn't exist
	file, err := os.OpenFile(filename, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0666)
	if err != nil {
		return fmt.Errorf("could not open or create file: %v", err)
	}
	defer file.Close()

	// Iterate through the frames and append to the file
	for _, frame := range frames {
		_, err := file.Write(frame)
		if err != nil {
			return fmt.Errorf("could not write frame to file: %v", err)
		}
	}

	return nil
}

func (csd *ClipStorageDevice) Run() {
	for frames := range csd.StorageChannel {
		go func() {
			err := Store(frames)
			if err != nil {
				fmt.Println(err)
				fmt.Println("Error storing frames")
			}
		}()
	}
}
