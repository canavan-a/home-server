package main

import (
	"errors"
	"fmt"
	"net/http"
	"os/exec"
	"time"
)

func main() {

	fmt.Println("starting orchestrator")

	var failCount int

	for {
		fmt.Println("polling server")
		err := healthCheckServer()
		if err != nil {
			fmt.Println("failed")
			failCount += 1
		} else {
			failCount = 0
		}

		if failCount == 5 {
			fmt.Println("restarting services")
			restartProcesses()
			failCount = 0
		}

		time.Sleep(time.Second * 3)

	}

}

func healthCheckServer() error {

	resp, err := http.Get("http://localhost:5000/api/health")
	if err != nil {
		return err
	}

	if resp.StatusCode != 200 {
		return errors.New("failed request")
	}

	return nil

}

func restartProcesses() {
	ffmpegCmd := exec.Command("tmux", "send-keys", "-t", "ffmpeg", `ffmpeg -f rawvideo -pix_fmt bgr24 -s 640x480 -r 30 -i /tmp/video_pipe2 -c:v vp8 -b:v 200k -g 15 -an -s 480x360 -filter:v "drawtext=text='%{localtime}':x=10:y=10:fontcolor=white:fontsize=14:box=1:boxcolor=black@0.5" -preset ultrafast -f rtp rtp://127.0.0.1:5005`, "C-m")
	ffmpegCmd.Start()
	mobilenetCmd := exec.Command("tmux", "send-keys", "-t", "mobilenet", "python3 opencv_object_detection.py", "C-m")
	mobilenetCmd.Start()
	serverCmd := exec.Command("tmux", "send-keys", "-t", "server", "./myprogram", "C-m")
	serverCmd.Start()
}
