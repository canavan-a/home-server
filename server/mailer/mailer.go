package mailer

import (
	"errors"
	"fmt"
	"net/smtp"
	"os"
)

func Notify(content string) error {
	err := SendEmail("aidan.canavan3@gmail.com", []byte(content))

	return err
}

func MakeClipBody(uri string) string {
	return fmt.Sprintf(`
	<div>
		<h3>Click here to see clip</h3>
		<a href="%s">%s</a>
	</div>
	`, uri, uri)

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
