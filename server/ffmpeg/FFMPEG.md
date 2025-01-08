# Start video stream from device

## 1. Locate Camera via ffmpeg command

Windows: `ffmpeg -list_devices true -f dshow -i dummy`
Linux: `ffmpeg -f v4l2 -list_devices true -i dummy`

## 2. Start a webRTC stream

Windows:

`ffmpeg -f dshow -i "video=Aidan's S23 (Windows Virtual Camera)" -c:v libx264 -preset fast -rtbufsize 1500M -s 640x480 -r 25 -f mpegts pipe:1`

Linux:

`ffmpeg -f v4l2 -i /dev/video0 -c:v libx264 -preset fast -rtbufsize 1500M -s 640x480 -r 25 -f mpegts pipe:1`

```
-preset fast
This option configures the encoding speed for libx264. The fast preset offers a good trade-off between encoding speed and video quality. Other options include ultrafast, superfast, veryfast, medium, slow, etc., with ultrafast being the fastest but less efficient in terms of compression, and slow being more efficient but slower.
```

```
-s 640x480
This option sets the video resolution to 640x480 pixels (standard VGA). You can change this value to other resolutions (e.g., 1280x720 for 720p).
```

```
-r 25
This sets the frame rate (fps) of the output video to 25 frames per second. This is a typical frame rate for standard video content.
```

```
-f mpegts
This option tells FFmpeg to output the video in MPEG-TS (MPEG Transport Stream) format. MPEG-TS is a standard for streaming video and audio over the network, often used for broadcast or live streaming.
```

```
package main

import (
	"fmt"
	"log"
	"net/http"
	"os"
	"os/exec"
	"github.com/pion/webrtc/v3"
	"github.com/gorilla/websocket"
)

var upgrader = websocket.Upgrader{
	CheckOrigin: func(r *http.Request) bool {
		return true
	},
}

// WebRTCHandler will handle the WebRTC connection setup.
func WebRTCHandler(w http.ResponseWriter, r *http.Request) {
	// Set up the WebRTC peer connection
	peerConnection, err := webrtc.NewPeerConnection(webrtc.Configuration{})
	if err != nil {
		log.Fatal("Failed to create peer connection:", err)
	}
	defer peerConnection.Close()

	// Create a video track
	videoTrack, err := webrtc.NewTrack(webrtc.DefaultPayloadTypeH264, 0, "video", "video")
	if err != nil {
		log.Fatal("Failed to create video track:", err)
	}

	// Add the track to the connection
	_, err = peerConnection.AddTrack(videoTrack)
	if err != nil {
		log.Fatal("Failed to add video track:", err)
	}

	// Start FFmpeg to capture video and pipe it to the handler
	cmd := exec.Command("ffmpeg", "-f", "dshow", "-i", "video=Aidan's S23 (Windows Virtual Camera)", "-c:v", "libx264", "-preset", "fast", "-rtbufsize", "1500M", "-s", "640x480", "-r", "25", "-f", "h264", "pipe:1")
	pipe, err := cmd.StdoutPipe()
	if err != nil {
		log.Fatal("Failed to create pipe:", err)
	}

	if err := cmd.Start(); err != nil {
		log.Fatal("Failed to start FFmpeg:", err)
	}

	// Read H.264 data from FFmpeg's stdout
	buf := make([]byte, 1024)
	for {
		n, err := pipe.Read(buf)
		if err != nil {
			log.Fatal("Failed to read from FFmpeg pipe:", err)
		}
		if n > 0 {
			// Send the H.264 video frames over WebRTC
			err := videoTrack.WriteSample(webrtc.Sample{Data: buf[:n]})
			if err != nil {
				log.Fatal("Failed to send video sample:", err)
			}
		}
	}
}

// SignalingHandler handles the WebSocket signaling to establish the WebRTC connection
func SignalingHandler(w http.ResponseWriter, r *http.Request) {
	// Upgrade HTTP connection to WebSocket
	conn, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Fatal("Failed to upgrade to WebSocket:", err)
	}
	defer conn.Close()

	// This is where you would handle the WebRTC signaling (Offer, Answer, ICE Candidates)
	// For example, receiving an SDP offer and sending back an answer:

	// Receive an SDP offer from the client
	_, offerMessage, err := conn.ReadMessage()
	if err != nil {
		log.Fatal("Failed to read message:", err)
	}

	// Here you would use the offerMessage to set the WebRTC offer and generate an SDP answer
	// For now, just send a response (you'd typically need to parse and respond to the offer)

	// Send back a dummy answer (this needs to be replaced with actual SDP handling logic)
	answer := `{"type": "answer", "sdp": "sdp_data_here"}`
	err = conn.WriteMessage(websocket.TextMessage, []byte(answer))
	if err != nil {
		log.Fatal("Failed to send WebSocket message:", err)
	}
}

func main() {
	// HTTP route to start the WebRTC handler
	http.HandleFunc("/webrtc", WebRTCHandler)

	// HTTP route for signaling
	http.HandleFunc("/signaling", SignalingHandler)

	// Start the HTTP server
	log.Println("Starting server on :8080")
	err := http.ListenAndServe(":8080", nil)
	if err != nil {
		log.Fatal("Failed to start server:", err)
	}
}
```
