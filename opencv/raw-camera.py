import cv2

# Open webcam
cap = cv2.VideoCapture("/dev/video0")

if not cap.isOpened():
    print("Error: Unable to open physical webcam")
    exit()

while True:
    ret, frame = cap.read()
    if not ret:
        print("Failed to grab frame")
        break

    # Show raw video feed
    cv2.imshow("Raw Camera Feed", frame)

    # Exit on 'q' key
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()
