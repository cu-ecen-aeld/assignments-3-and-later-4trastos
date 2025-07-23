#!/bin/sh

# writer.sh
# Arguments:
#   $1 - full path to the file to write (writefile)
#   $2 - string to write into the file (writestr)

# Check if both arguments are provided
if [ $# -ne 2 ]; then
    echo "Error: Two arguments required."
    echo "Usage: $0 <writefile> <writestr>"
    exit 1
fi

writefile=$1
writestr=$2

# Extract directory path from the file path
dirpath=$(dirname "$writefile")

# Check if directory exists, if not create it
if [ ! -d "$dirpath" ]; then
    mkdir -p "$dirpath" || {
        echo "Error: Could not create directory $dirpath"
        exit 1
    }
fi

# Write the string into the file (overwrite)
echo "$writestr" > "$writefile" 2>/dev/null

# Check if writing succeeded
if [ $? -ne 0 ]; then
    echo "Error: Failed to write to file $writefile"
    exit 1
fi

exit 0