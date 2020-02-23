
INTRODUCTION
------------

The goal of this homework is to become familiar with using shared memory and creating multiple processes. In this
project we will be using multiple processes to determine if a set of numbers are prime or not.


COMPILATION
-----------

To compile, open a new CLI window, change the 
directory nesting your module. Type:

 * make


EXECUTION
---------
	
Find the executable named oss, located inside that
directory in which you compiled the module.
To run it simply type:
  ./oss -b 101 -i 4

To run command line arguments:
  ./oss [-h] [-n  -s  -b  -i -o ] 
 
  * h Print a help message and exit.											
  * n This program will detect if numbers within a set are prime using child processes(Default 4)
  * s x Indicate the number of children allowed to exist in the system at the same time. (Default 2)
  * b B Start of the sequence of numbers to be tested for primality
  * i I Increment between numbers that we test
  * o filename Output file

If no arguments provided, it will it will compute 0 primality.

Version Control
----------------------
For Version Control i have used github.

README.txt
Displaying README.txt.
