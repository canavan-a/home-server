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

def sigmoid(x):
    return 1 / (1 + np.exp(-x))

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

    # Process output
    output = outputs[0]  # Output shape: (1, 125, 13, 13)
    
    # Postprocessing: Extract bounding boxes, class scores, and confidence
    conf_threshold = 0.5  # Confidence threshold for filtering boxes
    nms_threshold = 0.4  # Non-maxima suppression threshold

    grid_size = 13  # YOLO grid size (13x13)
    num_classes = 80  # Number of classes in the Pascal VOC dataset
    anchors = [(10, 13), (16, 30), (33, 23), (30, 61), (62, 45), (59, 119), (116, 90), (156, 198), (373, 326)]  # Anchors

    boxes = []
    confidences = []
    class_ids = []

    # Decode the outputs
    for i in range(grid_size):
        for j in range(grid_size):
            for b in range(3):  # 3 anchors
                box = output[0, b, i, j]
                center_x = (sigmoid(box[0]) + j) / grid_size  # x center
                center_y = (sigmoid(box[1]) + i) / grid_size  # y center
                width = np.exp(box[2]) * anchors[b][0] / grid_size  # width
                height = np.exp(box[3]) * anchors[b][1] / grid_size  # height
                objectness = sigmoid(box[4])  # objectness score
                class_probs = sigmoid(box[5:])  # class probabilities

                # Get the class with the highest probability
                class_id = np.argmax(class_probs)
                class_prob = class_probs[class_id]

                # Calculate final confidence score
                confidence = objectness * class_prob

                if confidence > conf_threshold:
                    # Rescale box coordinates to original frame size
                    x_min = int((center_x - width / 2) * frame.shape[1])
                    y_min = int((center_y - height / 2) * frame.shape[0])
                    x_max = int((center_x + width / 2) * frame.shape[1])
                    y_max = int((center_y + height / 2) * frame.shape[0])

                    boxes.append([x_min, y_min, x_max, y_max])
                    confidences.append(confidence)
                    class_ids.append(class_id)

    # Apply non-maxima suppression
    indices = cv2.dnn.NMSBoxes(boxes, confidences, conf_threshold, nms_threshold)

    # Draw bounding boxes on the frame
    if len(indices) > 0:
        for i in indices.flatten():
            x_min, y_min, x_max, y_max = boxes[i]
            cv2.rectangle(frame, (x_min, y_min), (x_max, y_max), (0, 255, 0), 2)
            label = f"Class: {class_ids[i]} Confidence: {confidences[i]:.2f}"
            cv2.putText(frame, label, (x_min, y_min - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 0, 0), 2)

    # Show the frame with bounding boxes
    cv2.imshow("Frame", frame)

    # Write the frame (optional)
    pipe.write(frame.tobytes())

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break
cap.release()
pipe.close()
