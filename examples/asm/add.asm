; BSD 2-Clause License
; 
; Copyright (c) 2022, Kasper Skott
; 
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions are met:
; 
; 1. Redistributions of source code must retain the above copyright notice, this
;    list of conditions and the following disclaimer.
; 
; 2. Redistributions in binary form must reproduce the above copyright notice,
;    this list of conditions and the following disclaimer in the documentation
;    and/or other materials provided with the distribution.
; 
; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
; AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
; IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
; DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
; FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
; DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
; SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
; CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
; OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
; OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

; File: add.asm
; Description: Asks the user to input two integers, then prints the sum of them.

.MODE     Stack         ; Use the stack instruction set.
.HEAP     1             ; KiB
.HEAP_MAX 262144        ; KiB

  JMP     main


; void add(int a, int b) locals: int
add:
  ; Initialize locals (1). Note: don't STL when first 
  ; initializing them, just leave them on the stack.
  LDI     0
  ; Body
  LDA     4     ; Load 1st function argument.
  LDA     8     ; Load 2nd function argument.
  ADD
  STL     4     ; You could actually just RET.32 after ADD, but I want to 
  LDL     4     ; demonstrate the usage of local variables.
  RET.32  8

main:
  ; Initialize locals (3 ints). Don't STL them.
  LDI     0     ; +4   promptMsg
  LDI     0     ; +8   leftNum
  LDI     0     ; +12  rightNum
  LDI     0     ; +16  result

  STR     _S0       ; Create a new string from program data at _S0.
  STL     4         ; Store string ptr in promptMsg.
  
  LDL     4
  SCALL   __print   ; Prompt the user for a number.
  SCALL   __input   ; Read and allocate new string from stdin.
  STOI    0         ; Convert the string to an int.
  STL     8         ; Store it in leftNum.
  
  LDL     4
  SCALL   __print
  SCALL   __input 
  STOI    0
  STL     12        ; Store it in rightNum.

  LDL     12        ; Function arguments are pushed in reversed order.
  LDL     8
  CALL    add       ; Add them through a function call to "int add(int,int)"
  STL     16        ; Store the function return value in result.

  ; When calling variadic system functions, like print or str,
  ; SARG must follow each argument as they are placed on the stack.
  ; The argument of SARG consists of 1 byte. The lower nibble indicates
  ; the size of the argument type in bytes, and the higher nibble
  ; indicates the argument type itself.
  ; 0x8* - string
  ; 0x4* - double
  ; 0x2* - float
  ; 0x1* - long
  ; 0x0* - int
  STR     _S1       ; Create the format string.
  SARG    132       ; (0x84) String arguments must have the last bit (7) set.
  LDL     8         ; Push leftNumber.
  SARG    4         ; (0x04)
  LDL     12        ; Push rightNumber.
  SARG    4         ; (0x04)
  LDL     16        ; Push result.
  SARG    4         ; (0x04)
  SCALL   __print

  EXIT

.DATA
_S0:
  .BYTE   17,   "Enter a number: "
_S1:
  .BYTE   20,    "I say %d + %d = %d\n"
