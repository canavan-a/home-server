import cv2
from ultralytics import YOLO

# Load YOLO model
model = YOLO("yolov8n.pt")

# Define class IDs for cars, trucks, and people (from COCO dataset)
ALLOWED_CLASSES = {0, 2, 7}  # 0: person, 2: car, 7: truck

# Open webcam
cap = cv2.VideoCapture(0)
cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*"MJPG"))  # Force MJPG mode
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)  # Try 1280x720 for balance
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
cap.set(cv2.CAP_PROP_FPS, 30)
if not cap.isOpened():
    print("Error: Unable to open physical webcam")
    exit()

while True:
    ret, frame = cap.read()
    if not ret:
        print("Failed to grab frame")
        break

    # Perform inference
    results = model(frame)

    # Draw bounding boxes for only allowed classes
    for result in results:
        for box in result.boxes:
            cls = int(box.cls[0].item())  # Class ID
            if cls in ALLOWED_CLASSES:
                x1, y1, x2, y2 = map(int, box.xyxy[0])  # Bounding box coordinates
                conf = box.conf[0].item()  # Confidence score
                label = f"{model.names[cls]} {conf:.2f}"

                # Draw rectangle and label
                cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
                cv2.putText(frame, label, (x1, y1 - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)

    # Display frame
    cv2.imshow("YOLOv8 Object Detection (Cars, Trucks, People)", frame)

    # Exit on 'q' key
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()
