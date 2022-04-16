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

; File: circles_s.asm
; Description: Performs some simple calculations on 10,000 spheres, using
;              some (deliberately) slow approximations of pi and sqrt.

.MODE       Stack
.HEAP       128
.HEAP_MAX   128

  JMP       main

; double sqrt(double n)
; Returns a sqrt approximation based on the following code:
;
; double start = 0, end = n, mid = 0;
; while((end - start) >= 0.000001) {
; 	mid = (start + end)/2;
; 	if (mid * mid < n)
; 		start = mid;
; 	if (mid * mid >= n)
; 		end = mid;
; }
; return mid;
sqrt:
  LDI.64    0.0         ; +4  start
  LDI.64    0.0         ; +12 end
  LDI.64    0.0         ; +20 mid
  LDA.64    8
  STL.64    12
sqrt_test:
  LDL.64    12
  LDL.64    4
  SUB.F64  
  LDI.64    0.000001
  CPGQ.F64
  BRZ       sqrt_ret

  LDL.64    4
  LDL.64    12
  ADD.F64
  LDI.64    2.0
  DIV.F64
  STL.64    20

  LDL.64    20
  LDL.64    20
  MUL.F64
  LDA.64    8
  CPLT.F64
  BRZ       sqrt_next_if
  LDL.64    20
  STL.64    4
sqrt_next_if:
  LDL.64    20
  LDL.64    20
  MUL.F64
  LDA.64    8
  CPGQ.F64
  BRZ       sqrt_test
  LDL.64    20
  STL.64    12
  JMP       sqrt_test
sqrt_ret:
  LDL.64    20
  RET.64    8

; double pi(int precision)
; Returns a rough pi approximation based on the following code:
;
; double result = 0;
; int sign = 1;
; int i;
; for (i = 1; i < precision; i += 2)
; {
;     if (sign)
;     {
;         result += 1.0 / i;
;         sign = 0;
;     }
;     else
;     {
;         result -= 1.0 / i;
;         sign = 1;
;     }
; }
; return result * 4;
pi:
  LDI.64    0           ; +4 double result
  LDI       1           ; +12 int sign
  LDI       1           ; +16 int i
pi_test:
  LDL       16
  LDA       4
  CPLT
  BRZ       pi_ret

  LDL       12
  BRZ       pi_else
  LDL.64    4
  LDI.64    1.0
  LDL       16
  ITOD
  DIV.F64
  ADD.F64
  STL.64    4
  LDI       0
  STL       12
  JMP       pi_loop_inc
pi_else:
  LDL.64    4
  LDI.64    1.0
  LDL       16
  ITOD
  DIV.F64
  SUB.F64
  STL.64    4
  LDI       1
  STL       12
pi_loop_inc:
  LDL       16
  LDI       2
  ADD
  STL       16
  JMP       pi_test
pi_ret:
  LDL.64    4
  LDI.64    4.0
  MUL.F64
  RET.64    4

; void dmemset(int dstAddress, int count, double val)
; Fills out a block of count number of doubles with val.
;
; for (int i = 0, int i < count; ++i) {
;     mem[dstAddress + i * 8] = val;
; }
dmemset:
  LDI       0           ; +4 i

dmemset_test:
  LDL       4
  LDA       8
  CPLT
  BRZ       dmemset_ret

  LDA.64    16
  LDA       4
  LDL       4
  LDI       8
  MUL
  ADD
  STM.64

  LDL       4
  LDI       1
  ADD
  STL       4
  JMP       dmemset_test
dmemset_ret:
  RET       16

; double calc_volume(double radius)
; Does a bunch of unnecessary conversions to finally end up with
; volume of a sphere with the given radius.
calc_volume:
  ; Radius to surface area: A = 4πr^2
  LDI.64    4.0
  LDI       2000 ; pi precision
  CALL      pi
  MUL.F64
  LDA.64    8
  MUL.F64
  LDA.64    8
  MUL.F64
  STA.64    8 ; arg is now surface area

  ; Surface area to radius: r = sqrt(A/(4π))
  LDA.64    8
  LDI.64    4.0
  LDI       2000 ; pi precision
  CALL      pi
  MUL.F64
  DIV.F64
  CALL      sqrt
  STA.64    8

  ; Radius to volume: V = (4/3)*πr^3
  LDI.64    4.0
  LDI.64    3.0
  DIV.F64
  LDI       2000 ; pi precision
  CALL      pi
  MUL.F64
  LDA.64    8
  LDA.64    8
  MUL.F64
  LDA.64    8
  MUL.F64
  MUL.F64
  RET.64    8

; double calc_avg_volume(int spheresBegin, int sphereCount)
calc_avg_volume:
  LDI       0         ; +4 i
  LDI.64    0         ; +8 sum

calc_avg_volume_test:
  LDL       4
  LDA       8
  CPLT
  BRZ       calc_avg_volume_ret

  LDA       4
  LDL       4
  LDI       8
  MUL
  ADD
  LDM.64
  CALL      calc_volume
  LDL.64    8
  ADD.F64
  STL.64    8

  LDL       4
  LDI       1
  ADD
  STL       4
  JMP       calc_avg_volume_test
calc_avg_volume_ret:
  LDL.64    8
  LDA       8
  ITOD
  DIV.F64
  RET.64    8

; Entry point
main:
  LDI       0         ; +4  spheresBegin
  LDI       0         ; +8  spheresCount

  ; Allocate 10,000 spheres, i.e. 10,000 radii (double).
  LDI       10 000
  STL       8
  LDL       8
  LDI       8
  MUL
  NEW
  STL       4

  ; Fill up the memory with 10.0.
  LDI.64    5.0
  LDL       8
  LDL       4
  CALL      dmemset

  ; Now calculate the average volume of the spheres.
  LDL       8
  LDL       4
  CALL      calc_avg_volume

  EXIT

.DATA
