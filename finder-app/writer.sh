#!/bin/sh

# writer.sh created by Andrea Fuggetta for assignment 1

# We are expecting two arguments:
# - filepath
# - filecontent
if [ "$#" -ne 2 ]; then
    echo "Error: missing parameter(s). You need to provide two arguments."
    exit 1
fi

# Store arguments into variables
FILEPATH=$1
WRITESTR=$2

DIRPATH=$(dirname "$FILEPATH")

# Directory in $1 has to exist
if [ ! -d "$DIRPATH" ]; then
    mkdir -p "$DIRPATH"
fi

echo "$WRITESTR" >> "$FILEPATH"

# Directory in $1 has to exist
if [ -f "$FILEPATH" ]; then
    echo "$FILEPATH created"
else
    echo "Error: Failed to create file!"
    exit 1
fi