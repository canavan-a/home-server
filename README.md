# home-server

aidan.home

## upload the ino file from raspi to the arduino

### install libraries

`arduino-cli lib install "AccelStepper"`

### compile the sketch

`arduino-cli compile --fqbn arduino:avr:mega your_sketch.ino`

### send sketch to the arduino

`arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:avr:mega your_sketch.ino`

## FFMPEG

### command to start video rtp server

`ffmpeg -f v4l2 -i /dev/video0 -vf "drawtext=text='%{localtime}':x=10:y=10:fontcolor=white:fontsize=16:box=1:boxcolor=black@0.5" -r 15 -c:v libx264 -preset fast -tune zerolatency -maxrate 800k -bufsize 1600k -g 30 -keyint_min 30 -f rtp -flush_packets 0 rtp://localhost:5005`

### VP8

`ffmpeg -f v4l2 -framerate 30 -fflags nobuffer -flags low_delay -i /dev/video0 -c:v vp8 -b:v 200k -g 15 -an -s 480x360 -filter:v "drawtext=text='%{localtime}':x=10:y=10:fontcolor=white:fontsize=14:box=1:boxcolor=black@0.5" -preset ultrafast -f rtp rtp://127.0.0.1:5005`
