package clipper_test

import (
	"fmt"
	"log"
	"main/clipper"
	"net"
	"os/exec"
	"testing"
	"time"

	"github.com/pion/rtp"
)

func Test_store(t *testing.T) {

	fmt.Println("collect rtp pckets")
	packets, err := createRtpsample()
	if err != nil {
		t.Error(err.Error())
	}
	fmt.Println("done collecting packet count:", len(packets))

	clipper.Store(packets)
	//create some rtp packets

}

func createRtpsample() (packets []rtp.Packet, err error) {

	cmd := exec.Command(
		"ffmpeg",
		"-f", "dshow",
		"-i", "video=FHD Camera",
		"-c:v", "vp8",
		"-b:v", "600k",
		"-f", "rtp",
		"rtp://127.0.0.1:5010",
	)
	fmt.Println("started rtp server")
	err = cmd.Start()
	if err != nil {
		fmt.Println("Error starting FFmpeg:", err)
		return
	}

	defer cmd.Process.Kill()

	address := "127.0.0.1:5010"

	udpAddr, err := net.ResolveUDPAddr("udp", address)
	if err != nil {
		log.Fatal(err)
		return
	}

	conn, err := net.ListenUDP("udp", udpAddr)
	if err != nil {
		log.Fatal(err)
		return
	}
	defer conn.Close()

	buf := make([]byte, 1500)

	start := time.Now()
	fmt.Println("listening to rtp server")

	for time.Since(start) < 5*time.Second {
		n, _, err := conn.ReadFromUDP(buf)
		if err != nil {
			fmt.Println("Error reading from UDP:", err)
			continue
		}

		fmt.Println("packet found")

		var packet rtp.Packet

		err = packet.Unmarshal(buf[:n])
		if err != nil {
			log.Printf("Error parsing packet")
			continue
		}

		packets = append(packets, packet)

	}

	fmt.Println("captured packets")

	return
}
