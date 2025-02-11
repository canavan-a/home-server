import cv2
import os
import time
import numpy as np
from ultralytics import YOLO

# model = YOLO('yolo11n.pt')
# model = YOLO("yolov8n.pt")
model = YOLO("yolo11n_full_integer_quant_edgetpu.tflite", task="detect")


# model.export(format='ncnn', half=True)
# print("model exported")

# ncnn_model = YOLO("yolo11n_ncnn_model")
# print("ncnn model created")

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

def resize_with_aspect_ratio(image, target_size=(320, 320)):
    h, w = image.shape[:2]
    scale = min(target_size[0] / w, target_size[1] / h)
    new_w, new_h = int(w * scale), int(h * scale)
    
    resized = cv2.resize(image, (new_w, new_h))
    
    # Create a black canvas and center the resized image
    padded = cv2.copyMakeBorder(resized, 
                                (target_size[1] - new_h) // 2, (target_size[1] - new_h + 1) // 2,
                                (target_size[0] - new_w) // 2, (target_size[0] - new_w + 1) // 2,
                                cv2.BORDER_CONSTANT, value=(0, 0, 0))
    return padded


# Feed the frame feed and process each frame
while True:
    ret, frame = cap.read()
    if not ret:
        print("Failed to grab frame")
        break

    
    # resized_frame = resize_with_aspect_ratio(frame, (320, 320))

    # resized_frame = cv2.resize(frame, (320, 320))
    # Perform YOLO detection
    print("frame is about to process")
    results = model(frame)

    # Draw bounding boxes for cars and people
    for result in results:
        for box in result.boxes:
            cls = int(box.cls[0])
            print("get model names")
            label = model.names[cls]

            if label in ["car", "person"]:
                x1, y1, x2, y2 = map(int, box.xyxy[0])
                confidence = box.conf[0]

                color = (0, 255, 0)
                if label == "person":
                    color = (0, 0, 255)

                # Draw rectangle and label
                cv2.rectangle(frame, (x1, y1), (x2, y2), color, 2)
                # cv2.putText(frame, f"{label} {confidence:.2f}", (x1, y1 - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)


    # Write the frame to the pipe (if needed)
    pipe.write(frame.tobytes())

# Release resources
cap.release()
pipe.close()


