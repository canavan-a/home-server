package main

import (
	"errors"
	"fmt"
	"net/http"
	"net/smtp"
	"os"
	"os/exec"
	"time"

	"github.com/joho/godotenv"
)

func main() {
	err := godotenv.Load()
	if err != nil {
		panic("Error loading .env file")
	}
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
			signalRestart()
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
	ffmpegCmd := exec.Command("tmux", "send-keys", "-t", "ffmpeg", `ffmpeg -f rawvideo -pix_fmt bgr24 -s 1280x720 -r 30 -i /tmp/video_pipe2 -c:v vp8 -b:v 1M -g 15 -an -filter:v "drawtext=text='%{localtime}':x=10:y=10:fontcolor=white:fontsize=14:box=1:boxcolor=black@0.5" -preset ultrafast -f rtp rtp://127.0.0.1:5005`, "C-m")
	ffmpegCmd.Start()
	mobilenetCmd := exec.Command("tmux", "send-keys", "-t", "mobilenet", "python3 opencv_object_detection.py", "C-m")
	mobilenetCmd.Start()
	serverCmd := exec.Command("tmux", "send-keys", "-t", "server", "./myprogram", "C-m")
	serverCmd.Start()
}

func signalRestart() {

	content := MakeClipBody()
	err := Notify(content)
	if err != nil {
		fmt.Println("could not send email")
	}

}

func Notify(content string) error {

	email := GenerateEmailContent("Memory Overflow", content)

	err := SendEmail("aidan.canavan3@gmail.com", email)

	return err
}

func MakeClipBody() string {
	return `
	<html>
	  <body>
		<h1>Memory Overflow</h1>
		<h3></h3>
		<p>Process restarting...</p>
	  </body>
	</html>
	`

}

func SendEmail(recipient string, content []byte) error {

	auth := LoginAuth(os.Getenv("MAIL_ADDR"), os.Getenv("MAIL_PW"))

	to := []string{recipient}
	msg := content

	err := smtp.SendMail(os.Getenv("MAIL_HOST")+":"+os.Getenv("MAIL_PORT"),
		auth, os.Getenv("MAIL_ADDR"), to, msg)

	return err

}

func GenerateEmailContent(subject string, body string) []byte {
	subjectOutput := "Subject: " + subject + "\n"
	mime := "MIME-version: 1.0;\nContent-Type: text/html; charset=\"UTF-8\";\n\n"

	return []byte(subjectOutput + mime + body)
}

type loginAuth struct {
	username, password string
}

// LoginAuth is used for smtp login auth
// https://stackoverflow.com/questions/42305763/connecting-to-exchange-with-golang
func LoginAuth(username, password string) smtp.Auth {
	return &loginAuth{username, password}
}

func (a *loginAuth) Start(server *smtp.ServerInfo) (string, []byte, error) {
	return "LOGIN", []byte(a.username), nil
}

func (a *loginAuth) Next(fromServer []byte, more bool) ([]byte, error) {
	if more {
		switch string(fromServer) {
		case "Username:":
			return []byte(a.username), nil
		case "Password:":
			return []byte(a.password), nil
		default:
			return nil, errors.New("unknown from server")
		}
	}
	return nil, nil
}
