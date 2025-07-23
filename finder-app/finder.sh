#!/bin/sh

# Check if exactly two arguments are provided
if [ "$#" -ne 2 ]; then
    echo "Error: Two arguments required."
    echo "Usage: $0 <directory> <search_string>"
    exit 1
fi

filesdir="$1"
searchstr="$2"

# Check if the directory exists
if [ ! -d "$filesdir" ]; then
    echo "Error: '$filesdir' is not a valid directory."
    exit 1
fi

# Count number of files (recursively)
num_files=$(find "$filesdir" -type f | wc -l)

# Count matching lines
num_matches=$(grep -r "$searchstr" "$filesdir" 2>/dev/null | wc -l)

# Output as per requirement
echo "The number of files are $num_files and the number of matching lines are $num_matches"

# Exit successfully
exit 0