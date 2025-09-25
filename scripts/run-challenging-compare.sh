#!/bin/bash

set -e

for input in benchmarks/challenging/*; do

    # Run Python version
    python_cmd="python3 python/minimalloc/cli.py --capacity=1048576 --input=$input --output=python_out.txt"
    python_runtime=$( (eval "$python_cmd") 2>&1)

    # Run C   version
    cpp_cmd="./build/minimalloc --capacity=1048576 --input=$input --output=cpp_out.txt"
    cpp_runtime=$( (eval "$cpp_cmd") 2>&1)

    # Compare outputs
    if ! cmp -s python_out.txt cpp_out.txt; then
        echo "ERROR: Output mismatch for $input"
        echo "Python runtime: $python_runtime sec."
        echo "C++ runtime: $cpp_runtime sec."
        echo "Outputs differ between Python and C++ API."
        exit 1
    fi

    printf "%s \t[Python] %12s \t[C++] %12s \tPASS\n" "$input" "$python_runtime" "$cpp_runtime"

    # Clean up temporary files
    rm -f python_out.txt cpp_out.txt
done

echo "All tests passed - Python and C++ API produced identical results"
