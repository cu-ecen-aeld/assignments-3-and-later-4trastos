#!/bin/sh

# finder.sh created by Andrea Fuggetta for assignment 1

# We are expecting two arguments:
# - filesdir
# - searchstr
if [ "$#" -ne 2 ]; then
    echo "Error: missing parameter(s). You need to provide two arguments."
    exit 1
fi

# Store arguments into variables
WRITEDIR=$1
WRITESTR=$2

# Directory in $1 has to exist
if [ -d "$WRITEDIR" ]; then
    echo "$WRITEDIR created"
else
    echo "Error: directory passed does not exist!"
    exit 1
fi

FILENUMBER=0
MATCHNUMBER=0
for FILE in "$WRITEDIR"/*; do
    if [ -f "$FILE" ]; then
        FILENUMBER=$((FILENUMBER + 1))
        TEMPMATCHNUMBER=$(grep -o "$WRITESTR" "$FILE" | wc -l)
        MATCHNUMBER=$((MATCHNUMBER + $TEMPMATCHNUMBER))
    fi
done

echo "The number of files are $FILENUMBER and the number of matching lines are $MATCHNUMBER"