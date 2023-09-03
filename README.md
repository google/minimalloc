<p align="center">
<img src="img/minimalloc.png">
</p>

Source code for our [ASPLOS 2024](https://www.asplos-conference.org/asplos2024/) paper, "***MiniMalloc: A Lightweight Memory Allocator for Hardware-Accelerated Machine Learning***."

## Setup

<pre>
$ git clone --recursive git@github.com:google/minimalloc.git && \
      cd minimalloc && cmake -DCMAKE_BUILD_TYPE=Release && make
</pre>

## Example input file

<pre>
buffer,start,end,size
b1,0,3,4
b2,3,9,4
b3,0,9,4
b4,9,21,4
b5,0,21,4
</pre>

## Example usage

<pre>
$ ./minimalloc --capacity=12 --input=benchmarks/examples/input.12.csv --output=output.csv
</pre>

## Example output file

<pre>
buffer,start,end,size,offset
b1,0,2,4,8
b2,3,8,4,8
b3,0,8,4,4
b4,9,20,4,4
b5,0,20,4,0
</pre>

## How to cite?

<pre>
@inproceedings{Moffitt2024,
  title = {Mini{M}alloc: A Lightweight Memory Allocator for Hardware-Accelerated Machine Learning},
  booktitle = {Proceedings of the 29th International Conference on Architectural Support for Programming Languages and Operating Systems},
  volume = {1},
  author = {Moffitt, Michael D.},
  year = {2024},
  series = {ASPLOS 2024}
}
</pre>

## Disclaimer

This is not an officially supported Google product.
