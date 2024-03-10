#!/bin/bash

# Check if the user provided the directory path and the variable
if [ $# -lt 2 ]; then
    echo "Usage: $0 <directory_path> <variable>"
    exit 1
fi

# Extract the arguments
directory_path="$1"
variable="$2"

# Check if the directory exists
if [ ! -d "$directory_path" ]; then
    echo "Directory $directory_path does not exist."
    exit 1
fi

# Find all files and directories containing "RT-AX5400" in their names
# and create copies with the specified variable appended to their names
find "$directory_path" -name '*RT-AX5400*' | while read -r item; do
    # Extract the file or directory name without the path
    base=$(basename "$item")

    # Determine the new name with the variable
    new_name="${base/RT-AX5400/$variable}"

    # Create a copy with the new name
    cp -r "$item" "$(dirname "$item")/$new_name"

    # Check if the copied item is a directory
    if [ -d "$(dirname "$item")/$new_name" ]; then
        # Check if the copied item is a symlink
        if [ -L "$(dirname "$item")/$new_name" ]; then
            echo "Symlink $(dirname "$item")/$new_name detected. No action taken."
        else
            # Remove the contents inside the new directory
            rm -rf "$(dirname "$item")/$new_name"/*
            echo "Contents inside $(dirname "$item")/$new_name removed."
        fi
    fi
done

echo "Copies created successfully."

