#!/bin/bash


# ASSUMES EVERYTHING IS PRE BUILT

# set up tmux windows
tmux new-session -d -s client -c ~/client
tmux new-session -d -s server -c ~/server
tmux new-session -d -s ffmpeg
tmux new-session -d -s mobilenet -c ~/mobilenet
tmux new-session -d -s orch -c ~/orchestrator

# activate coral conda environment
tmux send-keys -t mobilenet "conda activate coral" Enter

# start client
tmux send-keys -t client "npm run preview -- --host" Enter

# setup nginx routing
sudo cp nginx.conf /etc/nginx/nginx.conf
sudo nginx -s reload

# make os pipes
mkfifo /tmp/video_pipe2
mkfifo /tmp/json_pipe
mkfifo /tmp/raw_frame

# start orchestrator
tmux send-keys -t orch "./orch" Enter

echo "Setup complete"
