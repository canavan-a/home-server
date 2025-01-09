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

# run encoded video and sound to websocket

## Linux command

```
ffmpeg \
  -f v4l2 -i /dev/video0 \  # Camera input
  -f alsa -i hw:1 \  # Microphone input (adjust based on your system setup)
  -c:v vp8 \  # VP8 codec for video
  -c:a libopus \  # Opus codec for audio
  -b:a 128k \  # Audio bitrate (adjust as needed)
  -b:v 1M \  # Video bitrate (adjust as needed)
  -f webm \  # Output format WebM for WebCodecs compatibility
  -g 30 \  # Keyframe interval for video (adjust as needed)
  -r 30 \  # Frame rate (adjust as needed)
  -async 1 \  # Ensure audio and video sync
  -vsync 2 \  # Video sync mode
  -f webm - | websocat ws://yourwebsocketserver  # Pipe to WebSocket
```

## windows command

```
ffmpeg -f dshow -i video="FHD Camera" ^
  -f dshow -i audio="Microphone Array (Intel® Smart Sound Technology for Digital Microphones)" ^
  -c:v vp8 ^
  -b:v 500k ^
  -s 480x270 ^
  -framerate 24 ^
  -c:a libopus ^
  -b:a 64k ^
  -ac 1 ^
  -g 30 ^
  -rtbufsize 100M ^
  -strict experimental ^
  -map 0:v:0 -map 1:a:0 ^
  -f webm -y output.webm ^
  -async 1 ^
  -fps_mode passthrough ^
  -copyts
```

## terminal pipe

```
ffmpeg -f dshow -i video="FHD Camera" ^
  -f dshow -i audio="Microphone Array (Intel® Smart Sound Technology for Digital Microphones)" ^
  -c:v vp8 ^
  -b:v 500k ^
  -s 480x270 ^
  -framerate 24 ^
  -c:a libopus ^
  -b:a 64k ^
  -ac 1 ^
  -g 30 ^
  -rtbufsize 100M ^
  -strict experimental ^
  -map 0:v:0 -map 1:a:0 ^
  -f webm ^
  -async 1 ^
  -fps_mode passthrough ^
  -copyts ^
  http://localhost:5000/api/stream
```
