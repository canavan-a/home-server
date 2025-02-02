import cv2
from ultralytics import YOLO
import os
import time


# Load YOLO model
model = YOLO("yolov11n.pt") 

fifo_path = '/tmp/video_pipe'


# Open physical webcam (video0)
cap = cv2.VideoCapture("/dev/video0")
if not cap.isOpened():
    print("Error: Unable to open physical webcam")

# Open virtual webcam (video10)
frame_width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
frame_height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
fps = int(cap.get(cv2.CAP_PROP_FPS)) or 30  # Default to 30 FPS if 0

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

# Class IDs to keep: 0 for 'person', 2 for 'car'
TARGET_CLASSES = [0, 2]

while True:
    ret, frame = cap.read()
    if not ret:
        break

    # Run YOLO inference on the frame
    results = model(frame)

    # Get detections from the first result
    detections = results[0].boxes
    start_time = time.time()
    # Draw filtered bounding boxes
    for box in detections:
        cls_id = int(box.cls[0])  # Class ID
        conf = box.conf[0]        # Confidence
        x1, y1, x2, y2 = map(int, box.xyxy[0])  # Bounding box coordinates

        if cls_id in TARGET_CLASSES:
            label = "human" if model.names[cls_id] == "person" else "car"
            color = (0, 255, 0) if cls_id == 0 else (255, 0, 0)  # Green for person, Blue for car

            # Draw rectangle and label
            cv2.rectangle(frame, (x1, y1), (x2, y2), color, 1)
    print("Draw rate: ", 1 / (time.time() - start_time))    # Write processed frame to virtual camera
    pipe.write(frame.tobytes())

cap.release()
pipe.close()