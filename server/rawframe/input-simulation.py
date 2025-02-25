import cv2
import numpy as np
import os
import json


raw_frame_pipe = '/tmp/raw_frame'

# Open video capture
cap = cv2.VideoCapture(0)  # /dev/video0
cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*"MJPG"))  # Force MJPG mode
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)  # Try 1280x720 for balance
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)
cap.set(cv2.CAP_PROP_FPS, 30)



if not os.path.exists(raw_frame_pipe):
    print("making json pipe")
    os.mkfifo(raw_frame_pipe)
    print("json pipe made")

try:
    print("opening pipe")
    rf_pipe = open(raw_frame_pipe, 'wb')
    print("pipe open")
except Exception as e:
    print(f"Error: Unable to open pipe ({e})")
    exit(1)


if not cap.isOpened():
    print("Error: Could not open camera.")
    exit()

allowed_objects = ["car", "truck", "person"]

print("starting")

while True:
    ret, frame = cap.read()
    if not ret:
        print("Failed to grab frame")
        break

    resized_frame = cv2.resize(frame, (640, 480))

    rf_pipe.write(resized_frame.tobytes())

    # Break on 'q' key
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
