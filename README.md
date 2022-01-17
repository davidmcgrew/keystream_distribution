# Tools for computing the keystream distribution for rc4 and other keystream generators

This repo contains the source code to build the utility program `keystream_distribution`, which computes the distribution of keystream bytes for rc4.  This information can be useful for studying the security of real-world applications, and the implementation itself is a good example of the use of C++ `for_each`/`accumulate` with a parallel execution policy.

To build, install TBB (libtbb-dev) then run `make`.  To run, see the usage below.

```
Usage: ./keystream_distribution [COMMAND], where COMMAND is one of:

Compute command:

   [trials=<num>] [input=<file>] [output=<file>] [concurrency=<num>] [verbose]

   performs trial computations and creates/updates distribution, where
      trials=<num> performs <num> trials
      input=<file> uses distribution in <file> as initial distribution
      output=<file> writes final distribution to <file>
      concurrency=<num> uses <num> threads of execution

Merge command:

   merge <file1> <file2> [<file3> ... ] [output=<outfile>] [verbose]

   reads distributions from two or more files, writes merged distribution

Help command:

   help

   prints out this usage guidance

NOTES

   <num> can be an integer (e.g. 1024) or power of 2 (e.g. 2^10)

   if output=<file> is not specified, the standard output is used

   if conncurency=<num> is not specified, the number of cores is used

   verbose keyword sends verbose output to standard error

FILE FORMAT

   cnt[i][j] counts the number of times the i^th byte of keystream equals j


```
