
# Multi-Threaded Hash Tree

This is a project for the CS/SE 3377 Sys. Prog. in Unix and Other Env. course. The goal of this project is to create a multi-threaded program to compute a hash value of a given file. The size of the file can be very large, so there is a need for the program to be multi-threaded to achieve speedup. While one thread reads a part (block) of the file, other threads can compute hash values of already read blocks.
## Getting Started

Clone the repository and navigate to the project directory:

```bash

git clone https://github.com/username/multi-threaded-hash-tree.git
cd multi-threaded-hash-tree
```
## Usage

The program can be compiled using make. The usage of the program is as follows:

```bash

./htree filename num_threads
```
filename is the name of the input file whose hash value needs to be computed, and num_threads is the number of threads to be created.

## Design

The program divides the input file into multiple blocks, where each block is a basic unit of accessing a file on an I/O device. The program creates a complete binary tree of threads, where each leaf thread computes the hash value of n/m consecutive blocks assigned to it and returns the hash value to its parent thread. An interior thread computes the hash value of the blocks assigned to it and waits for its children to return hash values. These hash values are then combined to form a string, which is then hashed again to create the final hash value of the file.

The blocks are assigned to threads as follows: for a thread with number i, n/m consecutive blocks starting from block number i * n/m are assigned.

## Acknowledgements

This project was based on the project guidelines provided by Professor Sridhar Alagar for CS/SE 3377 Sys. Prog. in Unix and Other Env. at the University of Texas at Dallas.
