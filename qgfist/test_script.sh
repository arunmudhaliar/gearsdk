#!/bin/bash

# Default IP and port
DEFAULT_IP="127.0.0.1:4000"
DEFAULT_SLEEP_DELAY_MULTIPLIER=4  # Fixed the typo here from DEFUALT to DEFAULT

# Check if an argument is passed, otherwise use the default IP
IP_ADDRESS=${1:-$DEFAULT_IP}
SLEEP_DELAY_MULTIPLIER=${2:-$DEFAULT_SLEEP_DELAY_MULTIPLIER}

# Initialize sleep_time
sleep_time=0
# Loop to run the command 10 times in parallel
for i in {1..10}; do
    echo "Starting instance $i with server $IP_ADDRESS, sleep delay multiplier $sleep_time"

    # Run the command in the background using '&'
    ./qgfist-app --gserver "$IP_ADDRESS" --exit-after 60 &

    # Store the process ID
    pids[$i]=$!

    # Update sleep_time for the next iteration
    sleep_time=$(( (i + 1) * SLEEP_DELAY_MULTIPLIER ))  # Incremental delay of 4 seconds for each instance
    # Introduce an incremental delay (e.g., 2 seconds for each instance)
    sleep $sleep_time  # Move this before starting the instance
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
