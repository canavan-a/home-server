#!/bin/bash


# ASSUMES EVERYTHING IS PRE BUILT

# set up tmux windows
tmux new-session -d -s client
tmux new-session -d -s server
tmux new-session -d -s ffmpeg
tmux new-session -d -s mobilenet
tmux new-session -d -s orch

echo "Tmux sessions created"


# activate coral conda environment
tmux send-keys -t mobilenet "cd ~/mobilenet && conda activate coral" Enter

echo "python env initialized"

# start client
tmux send-keys -t client "cd ~/client && npm run preview -- --host" Enter

echo "client starting"


# setup server
tmux send-keys -t server "cd ~/server" Enter

# setup nginx routing
sudo cp nginx.conf /etc/nginx/nginx.conf
sudo nginx -s reload

echo "nginx config applied"

# make os pipes
mkfifo /tmp/video_pipe2
mkfifo /tmp/json_pipe
mkfifo /tmp/raw_frame

echo "Pipes created"

# setup orch
tmux send-keys -t orch "cd ~/orchestrator && ./orch" Enter
echo "Orchestrator started"

echo "Setup complete"
