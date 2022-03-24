# Introduction
***DISCLAIMER:*** this is a work in progress. Nothing is fully functional yet.

This repository contains software tools for compiling the Rack language into executable programs. Compiled programs are designed to run on *RackVM*, which is a very rudimentary, experimental virtual machine, capable of executing either stack-based, or register-based instructions/bytecodes. This is defined per executable.

The purpose of development was to produce research artifacts, and other tools to aid in my research on using *union*s in C, in an attempt at reducing overhead from VM instruction decoding in the execution loop. I suspected that this would benefit register architectures more, since decoding overhead is often referred to as a weakness of virtual register machines. Hence, the support for both stack and register VM architectures.

The repository contains:

<ul>
    <li> RackVM - written in C.
    <li> Compiler - developed using C++ with Bison and Flex. Non-optimizing due to time constraints.
    <li> Assembler - written in C++, translates custom assembly to a binary, executed by RackVM.
    <li> Rack source code - examples, and the very minimal (and unprioritized) standard library.
</ul>


# Build
This project uses CMake. If you want to compile the compiler, you should have at least Bison 3.8.2, and Flex installed. It might work on earlier versions, but it hasn't been tested.

You may choose to generate it however you want, but I use the "MSYS Makefiles" generator (on Windows 10), like this:
```bash
mkdir build
cd build
cmake -S .. -G "MSYS Makefiles"
make
```

# How to Use
***Work in progress...***