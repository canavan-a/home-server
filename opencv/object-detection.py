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

# YOLOv2 Tiny specific parameters
grid_size = 13
num_bboxes = 5
num_classes = 20
anchors = [1.08, 1.19, 3.42, 4.41, 6.63, 11.38, 9.42, 5.11, 16.62, 10.52]

def sigmoid(x):
    return 1 / (1 + np.exp(-x))

def softmax(x):
    return np.exp(x) / np.sum(np.exp(x), axis=-1, keepdims=True)

def parse_output(output):
    output = output.reshape((grid_size, grid_size, num_bboxes, num_classes + 5))
    boxes = []
    for i in range(grid_size):
        for j in range(grid_size):
            for b in range(num_bboxes):
                tx, ty, tw, th, to = output[i, j, b, :5]
                x = (j + sigmoid(tx)) / grid_size
                y = (i + sigmoid(ty)) / grid_size
                w = np.exp(tw) * anchors[2 * b] / grid_size
                h = np.exp(th) * anchors[2 * b + 1] / grid_size
                confidence = sigmoid(to)
                class_probs = softmax(output[i, j, b, 5:])
                class_id = np.argmax(class_probs)
                class_prob = class_probs[class_id]
                if confidence * class_prob > 0.5:  # Confidence threshold
                    boxes.append([x, y, w, h, confidence, class_id])
    return boxes

while True:
    ret, frame = cap.read()
    if not ret:
        break

    # Prepare the frame for YOLO input
    input_frame = cv2.resize(frame, (416, 416))
    input_frame = input_frame.astype(np.float32) / 255.0
    input_frame = np.transpose(input_frame, (2, 0, 1))
    input_frame = np.expand_dims(input_frame, axis=0)

    # Run inference
    inputs = {ort_session.get_inputs()[0].name: input_frame}
    outputs = ort_session.run(None, inputs)

    # Parse the output
    boxes = parse_output(outputs[0])

    # Draw bounding boxes on the frame
    for box in boxes:
        x, y, w, h, confidence, class_id = box
        x = int(x * frame_width)
        y = int(y * frame_height)
        w = int(w * frame_width)
        h = int(h * frame_height)
        cv2.rectangle(frame, (x - w // 2, y - h // 2), (x + w // 2, y + h // 2), (0, 255, 0), 2)
        cv2.putText(frame, f'Class {class_id}', (x - w // 2, y - h // 2 - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.9, (0, 255, 0), 2)

    # Write the frame with bounding boxes
    pipe.write(frame.tobytes())

cap.release()
pipe.close()
