#! /bin/bash

ecount () {
    total=0
    for arg in "${commandline_args[@]}"; do
        ecount=`tr -dc 'e' < "$arg" | wc -c`
        printf '%10d %s\n' $ecount "$arg"
        total=$(( $total + $ecount ))
    done
    printf '%10d total\n' $total
}

commandline_args=("$@")

ecount
