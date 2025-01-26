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

`ffmpeg -f v4l2 -framerate 30 -i /dev/video0 -vcodec h264_v4l2m2m -pix_fmt yuv420p -s 640x360 -rtbufsize 64M -use_wallclock_as_timestamps 1 -f rtp -sdp_file video.sdp rtp://127.0.0.1:5005`
