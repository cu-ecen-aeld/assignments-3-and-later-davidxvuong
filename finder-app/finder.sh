#!/bin/bash

if [ $# -lt 2 ]; then
    echo "Error - insufficient parameters passed in. Num parameters passed in: $#"
    exit 1
fi

FILESDIR=$1
SEARCHSTR=$2

if [ ! -d "$FILESDIR" ]; then
    echo "Error - file directory does not exist"
    exit 1
fi

num_files=$(find $FILESDIR/* | wc -l)
num_files_matching_str=$(find $FILESDIR -type f -exec grep -o $SEARCHSTR {} + | wc -l)

echo "The number of files are $num_files and the number of matching lines are $num_files_matching_str\n"
