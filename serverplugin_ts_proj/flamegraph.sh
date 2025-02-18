#!/bin/bash
FILENAME=$(date +"%Y-%m-%d_%H.%M.%S")
EXE='node'  # Node.js is the target executable

# Start npm process in the background and capture its PID
npm run start:release &
NPM_PID=$!

# Wait for the actual Node.js process to start
sleep 5  # Adjust this delay if needed

# Find the child Node.js process of npm (the actual application)
PID=$(pgrep -P "$NPM_PID" | head -n 1)

if [ -z "$PID" ]; then
    echo "Error: Could not find a running Node.js process for 'npm run start:release'!"
    exit 1
fi

echo "Tracing Node.js process: $PID"

# Run dtrace on the correct Node.js process
sudo dtrace -p "$PID" -o ./${FILENAME}-${EXE}-sample.stacks -n "profile-997 /execname == \"${EXE}\"/ { @[ustack(100)] = count(); }"

# Generate the flamegraph
stackcollapse.pl ${FILENAME}-${EXE}-sample.stacks | flamegraph.pl > ./${FILENAME}-${EXE}-pretty-graph.svg

echo "Flamegraph saved as ./${FILENAME}-${EXE}-pretty-graph.svg"
