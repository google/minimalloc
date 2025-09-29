#!/bin/bash

for input in benchmarks/challenging/*; do
  cmd="python3 python/minimalloc/cli.py --capacity=1048576 --input=$input --output=out.txt"
  runtime=$( (eval "$cmd") 2>&1 )
  printf "%s %12s\n" "$input" "$runtime"
done