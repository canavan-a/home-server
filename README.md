# home-server

aidan.house

## Home automation system

Running on lenovo mini PC with Arduino and ESP32 for system hardware controls

### Real time camera inference

OpenCV -> OpenVINO(yolo26n) -> Gstreamer(VP8 rtp) -> Pion WebRTC(Go) -> Go relay server + other web controls
                            -> Arduino Motor Controls
 