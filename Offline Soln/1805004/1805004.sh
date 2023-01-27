#!/bin/bash

max_score=100
max_std_id=5

if [[ $# != 0 ]]; then
    max_score=$1
    if [[ $# == 2 ]]; then
        max_std_id=$2
    fi
fi

diff_calculation() {
    difference=$(diff -w $1 "AcceptedOutput.txt")
    count=$(echo "$difference" | grep "<" | wc -l)
    count=$(($count + $(echo "$difference" | grep ">" | wc -l)))
    mark=$(($max_score - $count * 5))
    echo $mark
}

negative_mark_check() {
    if [[ $1 -lt 0 ]]; then
        echo 0
    else
        echo $1
    fi
}

start=1805121
end=$(($start + $max_std_id - 1))

for ((i = $start; i <= $end; i++)); do
    dir=$(find Submissions/ -name "$i" -type d)
    if [[ $dir != '' ]]; then
        file=$(find "$dir" -name "$i.sh" -type f)
        if [[ $file != '' ]]; then
            temp="${file%.sh}.txt"
            bash "./$file" >$temp
            mark=$(diff_calculation $temp)
            mark=$(negative_mark_check $mark)
            echo "$mark" >>"mark.txt"
            rm $temp
        else
            mark=0
            echo "$mark" >>"mark.txt"
        fi
    else
        mark=0
        echo "$mark" >>"mark.txt"
    fi
done

echo "student_id,score" >"output.csv"

for ((i = $start; i <= $end; i++)); do
    index=$(($i - 1805120))
    mark=$(cat mark.txt | head "-n$index" | tail -n1)
    t=0

    dir=$(find Submissions/ -name "$i" -type d)
    if [[ $dir != '' ]]; then
        file1=$(find "$dir" -name "$i.sh" -type f)

        for ((j = $start; j <= $end; j++)); do
            if [[ $j == $i ]]; then
                continue
            fi

            dir2=$(find Submissions/ -name "$j" -type d)
            if [[ $dir2 != '' ]]; then
                file2=$(find "$dir2" -name "$j.sh" -type f)

                if [[ $file1 != '' && $file2 != '' ]]; then
                    diff -Z -B $file1 $file2 >"temp.txt"
                    count=$(cat "temp.txt" | wc -l)
                    if [[ $count == 0 && $mark > 0 ]]; then
                        echo "$i,-$mark" >>"output.csv"
                        t=1
                        break
                    fi
                fi
            fi
        done

        if [[ $t == 0 ]]; then
            echo "$i,$mark" >>"output.csv"
        fi
    else
        echo "$i,$mark" >>"output.csv"
    fi
done

rm "mark.txt"
rm "temp.txt"
