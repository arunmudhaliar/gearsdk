#!/bin/bash

cd $(pwd)/docs
# Define variables
DOXYFILE=./Doxyfile

# Check if Doxyfile exists
if [ ! -f "${DOXYFILE}" ]; then
    echo "Doxyfile not found in the current directory. Please create a Doxyfile and try again."
    cd ..
    exit 1
fi

# Generate the documentation
doxygen "${DOXYFILE}"
cd ./sphinx
make html

# Check if the documentation was generated successfully
if [ $? -eq 0 ]; then
    echo "Doxygen documentation generated successfully."
else
    echo "Failed to generate Doxygen documentation. Please check the Doxyfile and try again."
    exit 1
fi
