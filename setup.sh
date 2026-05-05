#!/bin/bash


# ASSUMES EVERYTHING IS PRE BUILT

# set up tmux windows
tmux new-session -d -s client -c "$(pwd)/client"
tmux new-session -d -s server -c "$(pwd)/server"
# tmux new-session -d -s ffmpeg 
# tmux new-session -d -s mobilenet -c "$(pwd)/mobilenet"
# tmux new-session -d -s orch -c "$(pwd)/orchestrator"
tmux new-session -d -s stream -c "$(pwd)/better-video-pipeline"

echo "Tmux sessions created"


# activate coral conda environment
# tmux send-keys -t mobilenet "conda activate coral" Enter

# echo "python env initialized"

# start client
tmux send-keys -t client "npm run preview -- --host" Enter

echo "client starting"

# setup nginx routing
sudo cp nginx.conf /etc/nginx/nginx.conf
sudo nginx -s reload

echo "nginx config applied"

# setup orch
tmux send-keys -t server "./myprogram" Enter
# echo "Orchestrator started"

tmux send-keys -t stream "./build/main" Enter

echo "Setup complete"
