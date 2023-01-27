#!/bin/bash

SAVEIFS=$IFS
IFS=$(echo -en "\n\b")
for file in $(find -name "*.txt" -type f); do
    dir=$(cat "$file" | tail -n2 | head -1)
    dir2="${file%.*}"
    echo $dir2
    exist=$(find -name "$dir2" -type d)
    echo $exist
    if [[ exist != '' ]]; then
        mv "$file" "$exist"
    else
        mkdir "$dir2"
        mv "$file" "$dir2"
    fi
done
