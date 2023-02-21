#!/bin/bash

# # solution 1

# if [ $# -eq 0 ]; then
#     echo "Usage Solutions.sh file1 file2 ..."
#     exit 1
# fi

# for file in $*; do
#     if [ -f $file ]; then
#         if [ -x $file ]; then
#             echo "$file is executable; so changing its permissions"
#             chmod -x $file
#         else
#             echo "$file is not executable"
#         fi
#     fi
# done

# # solution 2

# for file in $(ls); do
#     if [ -f $file ]; then
#         if cat $file | head -n $1 | tail -1 | grep -i $2; then
#             rm $file
#         fi
#     fi
# done

# # solution 3

# find -maxdepth 1 -name "*[0-9]*" -type f -delete

# # solution 4

# for file in $(find -name "*.cpp" -type f); do
#     mv $file "${file%.cpp}.c"
# done

# # solution 5

# count=0
# until [ "$?" -ne 0 ]; do
#     count=$((count + 1))
#     ./rand.sh >out.txt 2>err.txt
# done

# echo "It took $count tries to get the error"
# echo "The output is: $(cat out.txt)"
# echo "The error is: $(cat err.txt)"
