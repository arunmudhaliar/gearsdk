#!/bin/bash

# Default IP and port
DEFAULT_IP="127.0.0.1:4000"

# Check if an argument is passed, otherwise use the default IP
IP_ADDRESS=${1:-$DEFAULT_IP}

# Loop to run the command 10 times in parallel
for i in {1..30}; do
    echo "Starting instance $i with server $IP_ADDRESS"

    # Run the command in the background using '&'
    ./qgfist-app --gserver "$IP_ADDRESS" --exit-after 60 &

    # Store the process ID
    pids[$i]=$!
done

# Wait for all background processes to finish
for pid in ${pids[*]}; do
    wait $pid
    if [ $? -eq 0 ]; then
        echo "Process $pid completed successfully"
    else
        echo "Process $pid failed"
    fi
done

echo "All instances completed"
