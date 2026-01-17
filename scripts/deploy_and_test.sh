#!/bin/bash
set -e

# Configuration
PI_USER="gad"
PI_HOST="192.168.1.21"
LOCAL_DIR="./pi/"
REMOTE_DIR="~/smart-irrigation-system/"

echo "Syncing code to Raspberry Pi..."
rsync -avz --exclude 'build' --exclude '.vscode' --exclude '.git' "$LOCAL_DIR" "$PI_USER@$PI_HOST:$REMOTE_DIR"

echo "Building and Testing on Raspberry Pi..."
ssh "$PI_USER@$PI_USER" "cd smart-irrigation-system && mkdir -p build && cd build && cmake .. && cmake --build . && echo 'Running Tests...' && ./irrigation_tests"

echo "Done!"
