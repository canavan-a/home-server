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
input_name = ort_session.get_inputs()[0].name

# Constants for YOLOv2
numClasses = 20
anchors = [1.08, 1.19, 3.42, 4.41, 6.63, 11.38, 9.42, 5.11, 16.62, 10.52]
clut = [(0, 0, 0), (255, 0, 0), (255, 0, 255), (0, 0, 255), (0, 255, 0), (0, 255, 128),
        (128, 255, 0), (128, 128, 0), (0, 128, 255), (128, 0, 128),
        (255, 0, 128), (128, 0, 255), (255, 128, 128), (128, 255, 128), (255, 255, 0),
        (255, 128, 128), (128, 128, 255), (255, 128, 128), (128, 255, 128)]
label = ["aeroplane", "bicycle", "bird", "boat", "bottle",
         "bus", "car", "cat", "chair", "cow", "diningtable",
         "dog", "horse", "motorbike", "person", "pottedplant",
         "sheep", "sofa", "train", "tvmonitor"]

# Sigmoid and softmax functions
def sigmoid(x, derivative=False):
    return x * (1 - x) if derivative else 1 / (1 + np.exp(-x))

def softmax(x):
    scoreMatExp = np.exp(np.asarray(x))
    return scoreMatExp / scoreMatExp.sum(0)

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

# Feed the frame feed and process each frame
while True:
    ret, frame = cap.read()
    if not ret:
        break

    # Get original frame dimensions
    original_height, original_width = frame.shape[:2]

    # Resize the frame to 416x416 while maintaining aspect ratio
    scale = min(416 / original_width, 416 / original_height)
    new_width = int(original_width * scale)
    new_height = int(original_height * scale)
    resized_frame = cv2.resize(frame, (new_width, new_height))

    # Pad the resized frame to make it 416x416
    pad_x = (416 - new_width) // 2
    pad_y = (416 - new_height) // 2
    padded_frame = cv2.copyMakeBorder(resized_frame, pad_y, pad_y, pad_x, pad_x, cv2.BORDER_CONSTANT, value=(0, 0, 0))

    # Prepare the frame for YOLO input
    input_frame = cv2.cvtColor(padded_frame, cv2.COLOR_BGR2RGB)  # Convert BGR to RGB
    input_frame = input_frame.transpose(2, 0, 1)  # Rearrange dimensions to (3, 416, 416)
    input_frame = np.expand_dims(input_frame, axis=0)  # Add batch dimension (1, 3, 416, 416)
    input_frame = input_frame.astype(np.float32) / 255.0  # Normalize to [0, 1]

    # Run inference
    outputs = ort_session.run(None, {input_name: input_frame})
    out = outputs[0][0]  # Extract the output

    # Debug: Print model output shape
    print("Model output shape:", outputs[0].shape)

    # Post-process the output to draw bounding boxes
    for cy in range(0, 13):
        for cx in range(0, 13):
            for b in range(0, 5):
                channel = b * (numClasses + 5)
                tx = out[channel][cy][cx]
                ty = out[channel + 1][cy][cx]
                tw = out[channel + 2][cy][cx]
                th = out[channel + 3][cy][cx]
                tc = out[channel + 4][cy][cx]

                x = (float(cx) + sigmoid(tx)) * 32
                y = (float(cy) + sigmoid(ty)) * 32
                w = np.exp(tw) * 32 * anchors[2 * b]
                h = np.exp(th) * 32 * anchors[2 * b + 1]

                confidence = sigmoid(tc)
                classes = np.zeros(numClasses)
                for c in range(0, numClasses):
                    classes[c] = out[channel + 5 + c][cy][cx]
                classes = softmax(classes)
                detectedClass = classes.argmax()

                if 0.3 < classes[detectedClass] * confidence:  # Lower confidence threshold for debugging
                    # Scale bounding box coordinates back to original resolution
                    x = int((x - pad_x) / scale)
                    y = int((y - pad_y) / scale)
                    w = int(w / scale)
                    h = int(h / scale)

                    # Debug: Print detected class and confidence
                    print(f"Detected class: {label[detectedClass]}, Confidence: {classes[detectedClass] * confidence}")

                    # Draw bounding box and label on the original frame
                    color = clut[detectedClass]
                    cv2.rectangle(frame, (x, y), (x + w, y + h), color, 2)
                    cv2.putText(frame, label[detectedClass], (x, y - 5), cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 2)

    # Write the frame to the pipe (if needed)
    pipe.write(frame.tobytes())

    # Break the loop if 'q' is pressed
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# Release resources
cap.release()
pipe.close()