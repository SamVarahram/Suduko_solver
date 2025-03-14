#!/bin/bash
# average_time.sh - Runs the Sudoku solver 10 times and calculates average Non-solving and Solving times.
# This script is used to calculate the average Non-solving and Solving times of the Sudoku solver.
# It was written by chatgpt 03 on GitHub and modified by me.


if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <board_file>"
    exit 1
fi

TRIALS=10
sumNon=0
sumSolve=0

for ((i=1; i<=TRIALS; i++)); do
    echo "Trial $i:"
    # Run the executable; adjust the name (./suduko_solver) if needed.
    output=$(./suduko_solver "$1")
    #echo "$output"
    
    # Extract Non-solving and Solving times.
    non=$(echo "$output" | grep "Non-solving time:" | awk '{print $3}')
    solve=$(echo "$output" | grep "Solving time:" | awk '{print $3}')
    
    # Sum up the times using 'bc' for floating point arithmetic.
    sumNon=$(echo "$sumNon + $non" | bc -l)
    sumSolve=$(echo "$sumSolve + $solve" | bc -l)
done

avgNon=$(echo "$sumNon / $TRIALS" | bc -l)
avgSolve=$(echo "$sumSolve / $TRIALS" | bc -l)

echo "--------------------------------------------------"
echo "Average Non-solving time: $avgNon seconds"
echo "Average Solving time:     $avgSolve seconds"