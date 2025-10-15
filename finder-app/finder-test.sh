#!/bin/sh
# Tester script for assignment 1 and assignment 2
# Author: Siddhant Jajoo
# Corrected version by ChatGPT

set -e
set -u

# Default values
NUMFILES=10
WRITESTR=AELD_IS_FUN
WRITEDIR=/tmp/aeld-data

# Use absolute paths
USERNAME=$(cat /etc/finder-app/conf/username.txt)
ASSIGNMENT=$(cat /etc/finder-app/conf/assignment.txt)

# Handle optional script arguments
if [ $# -lt 3 ]; then
    echo "Using default value ${WRITESTR} for string to write"
    if [ $# -lt 1 ]; then
        echo "Using default value ${NUMFILES} for number of files to write"
    else
        NUMFILES=$1
    fi
else
    NUMFILES=$1
    WRITESTR=$2
    WRITEDIR=/tmp/aeld-data/$3
fi

MATCHSTR="The number of files are ${NUMFILES} and the number of matching lines are ${NUMFILES}"

echo "Writing ${NUMFILES} files containing string ${WRITESTR} to ${WRITEDIR}"

# Remove old directory if exists
rm -rf "$WRITEDIR"

# Always create the directory to avoid writer failures
mkdir -p "$WRITEDIR"

if [ -d "$WRITEDIR" ]; then
    echo "$WRITEDIR created"
else
    echo "Error: failed to create $WRITEDIR"
    exit 1
fi

# Loop to write files using writer
for i in $(seq 1 $NUMFILES); do
    /bin/writer "$WRITEDIR/${USERNAME}_$i.txt" "$WRITESTR"
done

# Run finder.sh on the created files
OUTPUTSTRING=$(finder.sh "$WRITEDIR" "$WRITESTR")

# Save result for assignment 4
echo "$OUTPUTSTRING" > /tmp/assignment4-result.txt

# Clean temporary files
rm -rf "$WRITEDIR"

# Validate output
set +e
echo "$OUTPUTSTRING" | grep "$MATCHSTR"
if [ $? -eq 0 ]; then
    echo "success"
    exit 0
else
    echo "failed: expected '${MATCHSTR}' in '${OUTPUTSTRING}' but instead found"
    exit 1
fi
