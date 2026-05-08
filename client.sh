#!/bin/bash

SERVER_IP="localhost"

echo "Connecting to server..."

nc $SERVER_IP 5000 | while read line; do
    echo "$line"
done &
NC_PID=$!

while true; do
    read -p "Type DRAW to play: " cmd

    if [ "$cmd" == "DRAW" ]; then
        echo "DRAW" | nc $SERVER_IP 5000
    fi
done