import cv2
import numpy as np
import os
import json
import socket
import struct

UDP_IP = "127.0.0.1"  # Replace with the target IP
UDP_PORT = 999 

# Open video capture
cap = cv2.VideoCapture(0)  # /dev/video0
cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*"MJPG"))  # Force MJPG mode
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)  # Try 1280x720 for balance
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)
cap.set(cv2.CAP_PROP_FPS, 30)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

MAX_UDP_SIZE = 65000    # Max UDP payload size

def send_large_frame(sock, frame, ip, port):
    print("sending")
    frame_data = frame.tobytes()
    total_len = len(frame_data)
    num_chunks = (total_len // MAX_UDP_SIZE) + 1

    # Send frame metadata first (frame_id, num_chunks)
    frame_id = int.from_bytes(os.urandom(4), "big")  # Unique frame ID
    sock.sendto(struct.pack("!II", frame_id, num_chunks), (ip, port))

    # Send each chunk with sequence number
    for i in range(num_chunks):
        start = i * MAX_UDP_SIZE
        end = min(start + MAX_UDP_SIZE, total_len)
        chunk = frame_data[start:end]
        header = struct.pack("!III", frame_id, i, num_chunks)  # (frame_id, chunk_id, total_chunks)
        sock.sendto(header + chunk, (ip, port))

while True:
    ret, frame = cap.read()
    if not ret:
        print("Failed to grab frame")
        break

    resized_frame = cv2.resize(frame, (640, 480))

    
    print(len(resized_frame))
    # cv2.imshow("hello", resized_frame)

    send_large_frame(sock, resized_frame, UDP_IP, UDP_PORT)
    
    
    # Break on 'q' key
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cv2.destroyAllWindows()
cap.release()
