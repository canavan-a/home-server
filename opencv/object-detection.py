import cv2
import os
import time
import numpy as np
import onnxruntime as ort
import urllib.request
import torch
from torchvision.ops import nms

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

def sigmoid(x):
    return 1 / (1 + np.exp(-x))

SCORE_THRESHOLD = 0.5
NMS_THRESHOLD = 0.4

# Feed the frame feed and process each frame
while True:
    ret, frame = cap.read()
    if not ret:
        break

    # Prepare the frame for YOLO input
    input_frame = cv2.resize(frame, (416, 416))  # Resize frame to 416x416
    input_frame = input_frame.astype(np.float32) / 255.0  # Normalize to [0, 1]
    input_frame = np.transpose(input_frame, (2, 0, 1))  # Rearrange dimensions to (3, 416, 416)
    input_frame = np.expand_dims(input_frame, axis=0)  # Add batch dimension (1, 3, 416, 416)

    # Run inference
    inputs = {ort_session.get_inputs()[0].name: input_frame}
    outputs = ort_session.run(None, inputs)

    # Extract the output from the model
    output = outputs[0]  # Shape should be (1, 25200, 85), i.e., (batch_size, grid_cells, 5 + 80)

    # Reshape the output for multiple grid sizes
    output = output.reshape(1, 3, 80, 80, 85)  # For 3 grid sizes (80x80, 40x40, 20x20)

    # Apply non-maxima suppression to filter predictions
    def non_max_suppression(output, conf_thres=0.5, iou_thres=0.4):
        predictions = []
        for batch in output:
            boxes = batch[..., :4]
            conf = batch[..., 4]
            class_scores = batch[..., 5:]
            scores, class_ids = torch.max(torch.tensor(class_scores), dim=-1)

            # Filter out predictions below confidence threshold
            mask = conf * scores > conf_thres
            boxes = boxes[mask]
            scores = scores[mask]
            class_ids = class_ids[mask]

            # Apply NMS (Non-Maximum Suppression)
            keep = nms(boxes, scores, iou_threshold=iou_thres)

            # Append the final predictions
            predictions.append(torch.stack([boxes[keep], scores[keep], class_ids[keep]], dim=1))

        return predictions

    # Run NMS on the output
    filtered_predictions = non_max_suppression(torch.tensor(output), conf_thres=SCORE_THRESHOLD, iou_thres=NMS_THRESHOLD)

    # Draw bounding boxes on the frame
    img = frame  # Use the original frame (BGR format)

    for pred in filtered_predictions[0]:
        x1 = int(pred[0][0])
        y1 = int(pred[0][1])
        x2 = int(pred[0][2])
        y2 = int(pred[0][3])
        confidence = pred[1].item()
        class_id = int(pred[2].item())

        # Draw the bounding box
        img = cv2.rectangle(img, (x1, y1), (x2, y2), (0, 255, 0), thickness=2)
        label = f"Class {class_id} Conf: {confidence:.2f}"
        cv2.putText(img, label, (x1, y1 - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 0, 0), 2)

    # Write the modified frame (with bounding boxes) to the pipe
    pipe.write(img.tobytes())

cap.release()
pipe.close()
