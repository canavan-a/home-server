# Use the Debian bookworm base image
FROM debian:bookworm-slim

# Set environment variables to avoid prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Update package list and install Python 3.8.20 and required packages
RUN apt-get update && \
    apt-get install -y \
    python3=3.8.20-1~deb11u1 \
    python3-pip \
    python3-dev \
    build-essential \
    && apt-get clean

# Ensure python3 points to the correct version
RUN ln -sf /usr/bin/python3.8 /usr/bin/python3

# Upgrade pip to the latest version
RUN python3 -m pip install --upgrade pip

RUN apt-get update && apt-get install -y ffmpeg

RUN apt-get update && apt-get install -y golang-1.24

