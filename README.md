# libutils

## What is libutils
Libutils is a collection of utility libraries supporting various features customized to the needs of my own projects.
The main goal of libutils is to avoid dependencies from other libraries.

## Features

 - Generic lists in C
 - Cross platform command line argument parser in C

## Build

*nix build using the following: `cmake -DCMAKE_BUILD_TYPE=Release libutils`
Windows build using: `cmake -DCMAKE_TOOLCHAIN_FILE=../../toolchain_w32.cmake libutils`
Testing requires **cmocka**, if this is not installed tests can be disabled with the `-DENABLE_TEST=Off` cmake argument.

## License
LGPL

## Author
qwattash
