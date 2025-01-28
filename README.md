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

`ffmpeg -f v4l2 -i /dev/video0 -vf "drawtext=text='%{localtime}':x=10:y=10:fontcolor=white:fontsize=16:box=1:boxcolor=black@0.5" -r 15 -c:v vp8 -quality good -cpu-used 0 -b:v 800k -f rtp -flush_packets 0 rtp://localhost:5005`
