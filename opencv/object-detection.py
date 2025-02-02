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

# Class IDs to keep: 0 for 'person', 1 for 'car'
TARGET_CLASSES = [0, 1]

while True:
    ret, frame = cap.read()
    if not ret:
        break

    # Prepare the frame for YOLO input (resize to 640x640 and normalize)
    input_frame = cv2.resize(frame, (416, 416))
    input_frame = input_frame.astype(np.float32)
    input_frame /= 255.0  # Normalize to [0, 1]

    # Convert to CHW format (needed for ONNX models)
    input_frame = np.transpose(input_frame, (2, 0, 1))
    input_frame = np.expand_dims(input_frame, axis=0)

    # Run inference
    inputs = {ort_session.get_inputs()[0].name: input_frame}
    outputs = ort_session.run(None, inputs)

    output = outputs[0]  # Extract the array from the list

    # Remove any extra dimensions (if present)
    output = np.squeeze(output)

    # Check the output shape to understand its structure
    print(f"Output shape: {output.shape}")

    # Assuming output shape: (num_detections, 6) -> [x1, y1, x2, y2, confidence, class_id]
    # Adjust this if the output shape differs
    boxes = output[:, :4]          # Bounding box coordinates (x1, y1, x2, y2)
    confidences = output[:, 4]     # Confidence scores
    class_ids = output[:, 5].astype(int)  # Class IDs (converted to int)

    start_time = time.time()

    # Draw filtered bounding boxes
    for i in range(len(boxes)):
        if confidences[i] > 0.5:  # Confidence threshold
            x1, y1, x2, y2 = boxes[i]
            cls_id = class_ids[i]

            if cls_id in TARGET_CLASSES:
                label = "human" if cls_id == 0 else "car"
                color = (0, 255, 0) if cls_id == 0 else (255, 0, 0)  # Green for person, Blue for car

                # Draw rectangle and label
                cv2.rectangle(frame, (int(x1), int(y1)), (int(x2), int(y2)), color, 1)
                cv2.putText(frame, label, (int(x1), int(y1)-5), cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 1)
        print("Draw rate: ", 1 / (time.time() - start_time))  # Write processed frame to virtual camera
    pipe.write(frame.tobytes())

cap.release()
pipe.close()
