<p align="center">
<img src="img/minimalloc.png">
</p>

Source code for our [ASPLOS 2023](https://www.asplos-conference.org/asplos2023/) paper, "***[MiniMalloc: A Lightweight Memory Allocator for Hardware-Accelerated Machine Learning](https://doi.org/10.1145/3623278.3624752)***."

## Setup

<pre>
$ git clone --recursive git@github.com:google/minimalloc.git && \
      cd minimalloc && cmake -DCMAKE_BUILD_TYPE=Release && make
</pre>

## Example input file

<pre>
id,lower,upper,size
b1,0,3,4
b2,3,9,4
b3,0,9,4
b4,9,21,4
b5,0,21,4
</pre>

## Example usage

<pre>
$ ./minimalloc --capacity=12 --input=benchmarks/examples/input.12.csv --output=output.12.csv
</pre>

## Example output file

<pre>
id,lower,upper,size,offset
b1,0,3,4,8
b2,3,9,4,8
b3,0,9,4,4
b4,9,21,4,4
b5,0,21,4,0
</pre>

## How to cite?

<pre>
@inproceedings{Moffitt2023,
  title = {{MiniMalloc}: A Lightweight Memory Allocator for Hardware-Accelerated Machine Learning},
  booktitle = {Proceedings of the 28th International Conference on Architectural Support for Programming Languages and Operating Systems},
  volume = {4},
  author = {Moffitt, Michael D.},
  year = {2023},
  series = {ASPLOS 2023},
  url = {https://doi.org/10.1145/3623278.3624752},
  doi = {10.1145/3623278.3624752},
}
</pre>

## Disclaimer

This is not an officially supported Google product.
