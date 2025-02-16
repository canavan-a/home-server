import cv2
import numpy as np

# Initialize webcam capture
cap = cv2.VideoCapture(0)
cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*"MJPG"))  # Force MJPG mode
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)  # Try 1280x720 for balance
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)
cap.set(cv2.CAP_PROP_FPS, 30)

while True:
    ret, frame = cap.read()
    if not ret:
        print("Failed to grab frame")
        break

    # Convert to grayscale
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

    # Adjust brightness and contrast (can be fine-tuned)
    alpha = 2.0  # Contrast control (1.0-3.0)
    beta = 50     # Brightness control (0-100)
    adjusted = cv2.convertScaleAbs(gray, alpha=alpha, beta=beta)

    # Apply histogram equalization
    equalized = cv2.equalizeHist(adjusted)

    # Apply Gaussian blur to reduce noise
    blurred = cv2.GaussianBlur(equalized, (5, 5), 0)

    # Apply a colormap for a night vision-like effect
    night_vision = cv2.applyColorMap(blurred, cv2.COLORMAP_OCEAN)

    # Show the processed "pseudo night vision" effect
    cv2.imshow("Pseudo Night Vision", night_vision)

    # Display the original webcam feed (optional)
    cv2.imshow("Webcam", frame)

    # Break the loop if 'q' key is pressed
    if cv2.waitKey(1) & 0xFF == ord("q"):
        break

# Release the webcam and close all OpenCV windows
cap.release()
cv2.destroyAllWindows()
