package sockstreamer

import (
	"fmt"
	"net/http"
	"sync"

	"github.com/gin-gonic/gin"
	"github.com/gorilla/websocket"
)

var Upgrader = websocket.Upgrader{
	ReadBufferSize:  1024,
	WriteBufferSize: 1024,
	CheckOrigin: func(r *http.Request) bool {
		return true
	},
}

type SocketStreamer struct {
	mutex sync.Mutex
	conns map[*websocket.Conn]bool
}

func NewSocketStreamer() *SocketStreamer {

	ss := &SocketStreamer{
		mutex: sync.Mutex{},
		conns: make(map[*websocket.Conn]bool),
	}

	return ss
}

func (ss *SocketStreamer) Send(data []byte) {
	ss.mutex.Lock()
	defer ss.mutex.Unlock()

	for conn, valid := range ss.conns {
		if valid {
			conn.WriteMessage(websocket.TextMessage, data)

		}
	}
}

func (ss *SocketStreamer) ListenRoute(c *gin.Context) {
	conn, err := Upgrader.Upgrade(c.Writer, c.Request, nil)
	if err != nil {
		fmt.Println(err)
		return
	}

	ss.mutex.Lock()
	ss.conns[conn] = true
	ss.mutex.Unlock()

	for {
		_, _, err := conn.ReadMessage()
		if err != nil {
			break
		}
	}

	ss.mutex.Lock()
	delete(ss.conns, conn)
	ss.mutex.Unlock()

}
