# 1. Introduction
***DISCLAIMER:*** The compiler is only partially functional, not updated to the latest version of the instruction set, and produces only stack-based assembly.

This repository contains software tools for assembling and running programs in RackVM, which is a very rudimentary, experimental virtual machine, capable of executing either stack-based, or register-based instructions. This may be configured at the assembly level for each program. The VM uses *switch dispatch*.

The purpose behind this project is to produce research artifacts, and other tools to aid in my research on using *union*s in C, in an attempt at reducing overhead from instruction decoding in the VM interpreter loop. I suspected that this would benefit register architectures more, since decoding overhead is often referred to as a weakness of virtual register machines. Hence, the support for both stack and register VM architectures.

The repository contains:

<ul>
    <li> RackVM - written in C.
    <li> Assembler - written in C++, translates custom assembly to a binary, executed by RackVM.
    <li> Experiment - some data that has been produced from the VM in benchmark mode, as well as a Python script to analyze and generate figures from said data.
    <li> Compiler - developed using C++ with Bison and Flex. Note: is unfinished, produces only partial stack-based assembly, and is non-optimizing due to time constraints.
</ul>


# 2. Build
This project uses CMake. If you want to compile the compiler, you should have at least Bison 3.8.2, and Flex 2.6.4 installed. It might very well work on earlier versions, but it hasn't been tested. Keep in mind that the compiler cannot produce fully functional code, but serves as a good base for future endeavors. 

Everything should be working correctly on GCC, but the VM is not currently stable on MSVC. You may choose to generate it however you want, but I use the "MSYS Makefiles" generator (on Windows 10), like this:
```bash
mkdir build
cd build
cmake -S .. -G "MSYS Makefiles"
make
```
Of course, you could set the build type to release or debug, as well as some other flags pertaining specifically to the VM. You may also choose to only build specific artifacts. Listed below are the valid CMake targets:
<ul>
    <li> vm - RackVM
    <li> asm - assembler
    <li> compiler
    <li> decoding-exp - initial union/bitmask experiment, which is not very reliable
</ul>

# 3. How to Use
## 3.1 Assembler
***Work in progress...***

## 3.2 RackVM
Actually running the programs in RackVM is trivial. Simply run the VM along with the path to the chosen binary as an argument, and it will execute it. If you choose to compile the VM in Debug mode, each run of the VM will print the current state of the stack to allow inspection.

The following example is the terminal output after running the program [add.asm](examples/asm/add.asm) in Debug mode. You can see the top-of-stack is decorated with `SP ==32=>`, meaning "stack pointer, 32-bit value". The previous location, the line beneath it, is where the top-of-stack value should be read if it is a 64-bit value. In this case the top-of-stack is the result of the addition operation of `5 + 81`.
```
$ ./vm/vm ../examples/asm/add.bin
[RackVM] Decoding instructions using the union technique.
Enter a number: 5
Enter a number: 81
I say 5 + 81 = 86
======== STACK DUMP ===================================================================
          []  i32        i64                  f32          f64             hex
---------------------------------------------------------------------------------------
SP ==32=> 5   86         721554505814         0.000000     0.000000        0x56
   --64-> 4   81         369367187537         0.000000     0.000000        0x51
          3   5          347892350981         0.000000     0.000000        0x5 
          2   24         21474836504          0.000000     0.000000        0x18
          1   64206      103079279310         0.000000     0.000000        0xFACE
          0   44061      275762670251037      0.000000     0.000000        0xAC1D
---------------------------------------------------------------------------------------
```

*A quick and dirty trick to debug simple programs is to insert an **EXIT** instruction where you want to break, which will then exit and print out the stack at that state when it is run.*



## 3.3 Running Benchmarks in RackVM
Benchmark mode is currently only available on Windows. Since that is what I'm using, and this is part of my thesis, I ended up only writing this feature for Windows. But you could easily just swap out the timing functionality on another platform.

Benchmark mode makes it so that instead of just running your program, the VM will prompt you for the number of runs you want it to do. It will then do that amount of runs, save the elapsed times and calculate the mean elapsed time for all runs, standard deviation, and individual deviations from the mean. The output will be saved as a timestamped .txt file, along with a .csv file of the run times.

In order to run RackVM in benchmark mode, it must be compiled in **Release** mode, and with the **BENCHMARK** flag defined.

Example benchmark output from running [circles_r](vm/benchmarks/circles_r.asm) 8 times:
```
=============================================
 Benchmark Results: Tue Apr 26 16:48:06 2022
 Program: ../vm/benchmarks/circles_r.bin
 VM Mode: Register
 Decoding: Union

   Run    Elapsed (ms)  Dev. from mean
---------------------------------------------
     1      805.265200       -0.575862
     2      800.056600       -5.784462
     3      805.044500       -0.796562
     4      802.029500       -3.811562
     5      805.498200       -0.342862
     6      826.885100       21.044038
     7      803.156800       -2.684262
     8      798.792600       -7.048462
---------------------------------------------

  Mean run time:      805.841062 ms.
  Standard deviation: 8.854452.
=============================================
```
*This is just an example, and is not representative of the quality of the actual experiment. The deviations are lower in a proper setting. The real benchmarks were running the programs 200 times in each configuration, for different optimization levels.*

## 3.4 Compiler
***Work in progress...***
