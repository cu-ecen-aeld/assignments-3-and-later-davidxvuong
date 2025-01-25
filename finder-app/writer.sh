#!/bin/bash

if [ $# -lt 2 ]; then
    echo "Error - insufficient parameters passed in. Num parameters passed in: $#"
    exit 1
fi

WRITEFILE=$1
WRITESTR=$2

mkdir -p "$(dirname "$WRITEFILE")" && touch $WRITEFILE

if [ $? -ne 0 ]; then
    echo "Could not create file $WRITEFILE"
    exit 1
fi


echo "$WRITESTR" > $WRITEFILE

