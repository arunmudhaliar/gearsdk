#!/bin/bash

# Check if the number of iterations is provided
if [ -z "$1" ]; then
  echo "Usage: $0 <number_of_iterations>"
  exit 1
fi

# Get the number of iterations from the first argument
n=$1

# Run the command in a loop
for ((i=1; i<=n; i++))
do
  echo "Running iteration $i..."
  
  # Run the command and capture the exit status
  ./qfist-app --server 127.0.0.1:4004 --exit-after 30
  exit_status=$?

  # Log success or failure for each iteration
  if [ $exit_status -eq 0 ]; then
    echo "Iteration $i: SUCCESS"
  else
    echo "Iteration $i: FAILED with exit status $exit_status"
  fi

done

echo "Completed $n iterations."
