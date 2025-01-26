# home-server

aidan.home

## upload the ino file from raspi to the arduino

### install libraries

`arduino-cli lib install "AccelStepper"`

### compile the sketch

`arduino-cli compile --fqbn arduino:avr:mega your_sketch.ino`

### send sketch to the arduino

`arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:avr:mega your_sketch.ino`
