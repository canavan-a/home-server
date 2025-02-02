import cv2
import os
import time
import numpy as np
import onnxruntime as ort
import urllib.request

# Download the ONNX model (Tiny YOLOv2) from the GitHub URL
model_url = "https://github.com/onnx/models/raw/refs/heads/main/validated/vision/object_detection_segmentation/tiny-yolov2/model/tinyyolov2-8.onnx"
model_path = 'tinyyolov2-8.onnx'

# If the model is not already downloaded, download it
if not os.path.exists(model_path):
    print("Downloading the ONNX model...")
    urllib.request.urlretrieve(model_url, model_path)
    print(f"Model downloaded to {model_path}")

# Load the ONNX model using ONNX Runtime
ort_session = ort.InferenceSession(model_path)

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

# Assuming ort_session is initialized properly
TARGET_CLASSES = {0: 'Person', 1: 'Car'}

# Only process these class IDs
TARGET_CLASS_IDS = [0, 1]

while True:
    ret, frame = cap.read()
    if not ret:
        break

    # Prepare the frame for YOLO input
    input_frame = cv2.resize(frame, (416, 416))
    input_frame = input_frame.astype(np.float32) / 255.0  # Normalize to [0, 1]
    input_frame = np.transpose(input_frame, (2, 0, 1))    # CHW format
    input_frame = np.expand_dims(input_frame, axis=0)     # Add batch dimension

    # Run inference
    inputs = {ort_session.get_inputs()[0].name: input_frame}
    outputs = ort_session.run(None, inputs)
    output = np.squeeze(outputs[0])  # Remove extra dimensions

    # Post-processing: Draw bounding boxes for cars and people
    for detection in output:
        class_id = int(np.argmax(detection[5:]))  # Get class ID

        if class_id in TARGET_CLASS_IDS:
            class_name = TARGET_CLASSES[class_id]

            # Bounding box coordinates (ensure scalar conversion)
            center_x, center_y, width, height = detection[:4]
            center_x = float(center_x) * frame.shape[1]
            center_y = float(center_y) * frame.shape[0]
            width = float(width) * frame.shape[1]
            height = float(height) * frame.shape[0]

            # Convert to top-left and bottom-right coordinates
            x1 = int(center_x - width / 2)
            y1 = int(center_y - height / 2)
            x2 = int(center_x + width / 2)
            y2 = int(center_y + height / 2)

            # Draw rectangle and label
            color = (0, 255, 0) if class_id == 0 else (255, 0, 0)  # Green for person, blue for car
            cv2.rectangle(frame, (x1, y1), (x2, y2), color, 2)
            cv2.putText(frame, class_name, (x1, y1 - 10), cv2.FONT_HERSHEY_SIMPLEX, 
                        0.5, color, 2)

    # Write the annotated frame
    pipe.write(frame.tobytes())


cap.release()
pipe.close()
