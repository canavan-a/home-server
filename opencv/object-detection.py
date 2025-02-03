import cv2
import os
import time
import numpy as np
from ultralytics import YOLO

model = YOLO('yolo11n.pt')

model.export(format="ncnn", half=False)
print("model exported")

ncnn_model = YOLO("yolo11n_ncnn_model")
print("ncnn model created")

fifo_path = '/tmp/video_pipe'

# Open physical webcam (video0)
cap = cv2.VideoCapture("/dev/video0")
if not cap.isOpened():
    print("Error: Unable to open physical webcam")

# Must create virtual camera to write to on device
# sudo modprobe v4l2loopback devices=1 video_nr=30 card_label="Virtual Camera" exclusive_caps=1

if not os.path.exists(fifo_path):
    print("making pipe")
    os.mkfifo(fifo_path)
    print("pipe made")

try:
    print("opening pipe")
    pipe = open(fifo_path, 'wb')
    print("pipe open")
except Exception as e:
    print(f"Error: Unable to open pipe ({e})")
    exit(1)

# Feed the frame feed and process each frame
while True:
    ret, frame = cap.read()
    if not ret:
        print("Failed to grab frame")
        break

    # Perform YOLO detection
    print("frame is about to process")
    results = ncnn_model(frame)

    # Draw bounding boxes for cars and people
    for result in results:
        for box in result.boxes:
            cls = int(box.cls[0])
            label = ncnn_model.names[cls]

            if label in ["car", "person"]:
                x1, y1, x2, y2 = map(int, box.xyxy[0])
                confidence = box.conf[0]

                # Draw rectangle and label
                cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
                cv2.putText(frame, f"{label} {confidence:.2f}", (x1, y1 - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)


    # Write the frame to the pipe (if needed)
    pipe.write(frame.tobytes())

# Release resources
cap.release()
pipe.close()