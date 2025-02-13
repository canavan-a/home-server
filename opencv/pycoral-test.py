import cv2
import os
import time
import numpy as np

from pycoral.utils.edgetpu import make_interpreter
from pycoral.adapters.common import input_size
from pycoral.adapters.classify import get_classes

# model = YOLO('yolo11n.pt')
# model = YOLO("yolov8n.pt")
edgetpu_delegate_path = "/usr/lib/aarch64-linux-gnu/libedgetpu.so.1"

MODEL_PATH = "yolov8n_full_integer_quant_edgetpu.tflite"
LABELS = {0: "person", 1: "car"}

interpreter = make_interpreter(MODEL_PATH, edgetpu_delegate_path)
interpreter.allocate_tensors()

width, height = input_size(interpreter)



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


# Feed the frame feed and process each frame
while True:
    ret, frame = cap.read()
    if not ret:
        print("Failed to grab frame")
        break

    resized_frame = cv2.resize(frame, (width, height))

    input_tensor = np.expand_dims(resized_frame, axis=0)
    interpreter.set_tensor(interpreter.get_input_details()[0]['index'], input_tensor)

    interpreter.invoke()

    
    results = get_classes(interpreter, top_k=1, score_threshold=0.5)

    filtered_results = [r for r in results if r.id in LABELS]

    # Draw bounding boxes for cars and people
    for result in filtered_results:
        label = f"{LABELS[result.id]}: {result.score:.2f}"
        cv2.putText(resized_frame, label, (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)

    # Write the frame to the pipe (if needed)
    pipe.write(resized_frame.tobytes())

# Release resources
cap.release()
pipe.close()


