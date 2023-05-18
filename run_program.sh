#!/bin/bash

# Checking if the directory is provided
if [ "$#" -ne 1 ]; then
    echo "Usage: ./run_program.sh <directory>"
    exit 1
fi

# Checking if the directory exists
if [ ! -d "$1" ]; then
    echo "Error: Directory '$1' does not exist."
    exit 1
fi

# Collecting all .ppm files in the provided directory into an array
files=("$1"/*.ppm)

# Running edge_detector with all files as inputs
./edge_detector "${files[@]}"
