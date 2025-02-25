package clipper

import (
	"fmt"
	"os"
	"time"

	"github.com/at-wat/ebml-go/webm"
	"github.com/pion/interceptor/pkg/jitterbuffer"
	"github.com/pion/rtp"
	"github.com/pion/rtp/codecs"
	"github.com/pion/webrtc/v4/pkg/media/samplebuilder"
)

type ClipStorageDevice struct {
	StorageChannel chan []rtp.Packet
}

func NewClipStorageDevice() *ClipStorageDevice {
	return &ClipStorageDevice{
		StorageChannel: make(chan []rtp.Packet),
	}
}

func Store(packets []rtp.Packet) {

	vp8Builder := samplebuilder.New(10, &codecs.VP8Packet{}, 90000)
	saver := newWebmSaver()

	for _, packet := range packets {
		vp8Builder.Push(&packet)
	}

	vp8Builder.Flush()

	for {
		sample := vp8Builder.Pop()
		if sample == nil {
			break
		}
		// Read VP8 header.
		videoKeyframe := (sample.Data[0]&0x1 == 0)
		if videoKeyframe {
			// Keyframe has frame information.
			raw := uint(sample.Data[6]) | uint(sample.Data[7])<<8 | uint(sample.Data[8])<<16 | uint(sample.Data[9])<<24
			width := int(raw & 0x3FFF)
			height := int((raw >> 16) & 0x3FFF)

			if saver.videoWriter == nil {
				fmt.Println("init writer")
				saver.InitWriter(false, width, height)
			}
		} else {
			fmt.Println("no keyframes")
		}
		if saver.videoWriter != nil {
			fmt.Println("saving data")
			saver.videoTimestamp += sample.Duration
			if _, err := saver.videoWriter.Write(videoKeyframe, int64(saver.videoTimestamp/time.Millisecond), sample.Data); err != nil {
				panic(err)
			}
		}
	}
	saver.Close()

}

type webmSaver struct {
	audioWriter, videoWriter       webm.BlockWriteCloser
	audioBuilder, vp8Builder       *samplebuilder.SampleBuilder
	audioTimestamp, videoTimestamp time.Duration

	h264JitterBuffer   *jitterbuffer.JitterBuffer
	lastVideoTimestamp uint32
}

func newWebmSaver() *webmSaver {
	return &webmSaver{
		audioBuilder:     samplebuilder.New(10, &codecs.OpusPacket{}, 48000),
		vp8Builder:       samplebuilder.New(10, &codecs.VP8Packet{}, 90000),
		h264JitterBuffer: jitterbuffer.New(),
	}
}

func (s *webmSaver) Close() {
	fmt.Printf("Finalizing webm...\n")
	if s.audioWriter != nil {
		if err := s.audioWriter.Close(); err != nil {
			panic(err)
		}
	}
	if s.videoWriter != nil {
		if err := s.videoWriter.Close(); err != nil {
			panic(err)
		}
	}
}

func (s *webmSaver) PushVP8(rtpPacket *rtp.Packet) {
	s.vp8Builder.Push(rtpPacket)

	for {
		sample := s.vp8Builder.Pop()
		if sample == nil {
			return
		}
		// Read VP8 header.
		videoKeyframe := (sample.Data[0]&0x1 == 0)
		if videoKeyframe {
			// Keyframe has frame information.
			raw := uint(sample.Data[6]) | uint(sample.Data[7])<<8 | uint(sample.Data[8])<<16 | uint(sample.Data[9])<<24
			width := int(raw & 0x3FFF)
			height := int((raw >> 16) & 0x3FFF)

			if s.videoWriter == nil {
				s.InitWriter(false, width, height)
			}
		}
		if s.videoWriter != nil {
			s.videoTimestamp += sample.Duration
			if _, err := s.videoWriter.Write(videoKeyframe, int64(s.videoTimestamp/time.Millisecond), sample.Data); err != nil {
				panic(err)
			}
		}
	}
}

func (s *webmSaver) InitWriter(isH264 bool, width, height int) {
	w, err := os.OpenFile("test.webm", os.O_WRONLY|os.O_CREATE|os.O_TRUNC, 0o600)
	if err != nil {
		panic(err)
	}

	videoMimeType := "V_VP8"
	if isH264 {
		videoMimeType = "V_MPEG4/ISO/AVC"
	}

	ws, err := webm.NewSimpleBlockWriter(w,
		[]webm.TrackEntry{
			{
				Name:            "Audio",
				TrackNumber:     1,
				TrackUID:        12345,
				CodecID:         "A_OPUS",
				TrackType:       2,
				DefaultDuration: 20000000,
				Audio: &webm.Audio{
					SamplingFrequency: 48000.0,
					Channels:          2,
				},
			}, {
				Name:            "Video",
				TrackNumber:     2,
				TrackUID:        67890,
				CodecID:         videoMimeType,
				TrackType:       1,
				DefaultDuration: 33333333,
				Video: &webm.Video{
					PixelWidth:  uint64(width),
					PixelHeight: uint64(height),
				},
			},
		})
	if err != nil {
		panic(err)
	}
	fmt.Printf("WebM saver has started with video width=%d, height=%d\n", width, height)
	s.audioWriter = ws[0]
	s.videoWriter = ws[1]
}

func (csd *ClipStorageDevice) Run() {
	for packets := range csd.StorageChannel {
		Store(packets)
	}
}
