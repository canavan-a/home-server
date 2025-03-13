import cv2
import numpy as np
import os
import json
from pycoral.adapters import common, detect # type: ignore
from pycoral.utils.dataset import read_label_file # type: ignore
from pycoral.utils.edgetpu import make_interpreter # type: ignore

# Model and label file paths
MODEL_PATH = "ssdlite_mobiledet_coco_qat_postprocess_edgetpu.tflite"
LABEL_PATH = "coco_labels.txt"
SCORE_THRESHOLD = 0.60

# Load model and labels
interpreter = make_interpreter(MODEL_PATH)
interpreter.allocate_tensors()
labels = read_label_file(LABEL_PATH)

fifo_path = '/tmp/video_pipe2'
json_pipe_path = '/tmp/json_pipe'
raw_frame_pipe = '/tmp/raw_frame'

# Open video capture
cap = cv2.VideoCapture(0)  # /dev/video0
cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*"MJPG"))  # Force MJPG mode
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)  # Try 1280x720 for balance
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)
cap.set(cv2.CAP_PROP_FPS, 30)

if not cap.isOpened():
    print("Error: Could not open video stream.")
else:
    print("Video stream is open.")

if not os.path.exists(fifo_path):
    print("making pipe")
    os.mkfifo(fifo_path)
    print("pipe made")

if not os.path.exists(json_pipe_path):
    print("making json pipe")
    os.mkfifo(json_pipe_path)
    print("json pipe made")

if not os.path.exists(raw_frame_pipe):
    print("making json pipe")
    os.mkfifo(raw_frame_pipe)
    print("json pipe made")

try:
    print("opening pipe")
    pipe = open(fifo_path, 'wb')
    print("pipe open")
except Exception as e:
    print(f"Error: Unable to open pipe ({e})")
    exit(1)

try:
    print("opening pipe")
    rf_pipe = open(raw_frame_pipe, 'wb')
    print("pipe open")
except Exception as e:
    print(f"Error: Unable to open pipe ({e})")
    exit(1)

try:
    print("opening pipe")
    json_pipe = open(json_pipe_path, 'wb')
    print("pipe open")
except Exception as e:
    print(f"Error: Unable to open pipe ({e})")
    exit(1)

if not cap.isOpened():
    print("Error: Could not open camera.")
    exit()

allowed_objects = ["car", "truck", "person"]

print("starting")

while True:
    ret, frame = cap.read()
    if not ret:
        print("Failed to grab frame")
        break

    # Get model's required input size
    input_size = common.input_size(interpreter)
    resized_frame = cv2.resize(frame, input_size)

    # Set input tensor and perform inference
    common.set_input(interpreter, resized_frame)
    interpreter.invoke()
    objects = detect.get_objects(interpreter, SCORE_THRESHOLD)

    # Get scaling factors
    h_orig, w_orig = frame.shape[:2]
    h_resized, w_resized = input_size
    scale_x = w_orig / w_resized
    scale_y = h_orig / h_resized

    # Draw detections

    people = []
    person_index = 0

    for obj in objects:
        label = labels.get(obj.id, "Unknown")
        if label in allowed_objects:
            
            ymin, xmin, ymax, xmax = obj.bbox.ymin, obj.bbox.xmin, obj.bbox.ymax, obj.bbox.xmax

            if label == "person":
                people.append({ "person_index": person_index, "ymin":obj.bbox.ymin, "xmin":obj.bbox.xmin, "ymax": obj.bbox.ymax, "xmax":obj.bbox.xmax })
                person_index += 1

            # Rescale bounding box to original frame size
            xmin = int(xmin * scale_x)
            xmax = int(xmax * scale_x)
            ymin = int(ymin * scale_y)
            ymax = int(ymax * scale_y)


            score = obj.score

            # Draw bounding box and label
            if label == "person":
                color = (255, 0, 0)
            else:
                color = (0, 255, 0)

            cv2.rectangle(frame, (xmin, ymin), (xmax, ymax), color, 3)

    
    # if len(people) != 0:
    json_pipe.write((json.dumps(people)+"\n").encode())
    json_pipe.flush()

    # Display frame
    resized_frame = frame

    pipe.write(resized_frame.tobytes())
    rf_pipe.write(resized_frame.tobytes())

    # Break on 'q' key
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
pipe.close()
# cv2.destroyAllWindows()
