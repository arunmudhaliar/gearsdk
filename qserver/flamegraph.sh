#!/bin/bash
FILENAME=$(date +"%Y-%m-%d_%H.%M.%S")
EXE='qserver-app'

# Check if the executable exists
if ! command -v ./${EXE} &> /dev/null; then
    echo "Error: ${EXE} not found!"
    exit 1
fi

# If the executable is found, continue with the dtrace and flamegraph commands
sudo dtrace -c "./${EXE}" -o ./${FILENAME}-${EXE}-sample.stacks -n "profile-997 /execname == \"${EXE}\"/ { @[ustack(100)] = count(); }"

# generate the flamegraph
stackcollapse.pl ${FILENAME}-${EXE}-sample.stacks | flamegraph.pl > ./${FILENAME}-${EXE}-pretty-graph.svg
