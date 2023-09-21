#!/bin/bash

for input in benchmarks/challenging/*; do
  cmd="./minimalloc --capacity=1048576 --input=$input --output=out.txt"
  runtime=$( (eval "$cmd") 2>&1 )
  printf "%s %8s sec.\n" "$input" "$runtime"
done
