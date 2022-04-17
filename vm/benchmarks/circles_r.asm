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

; File: circles_r.asm
; Description: Performs some simple calculations on 10,000 spheres, using
;              some (deliberately) slow approximations of pi and sqrt.

.MODE       Register
.HEAP       128
.HEAP_MAX   128

  JMP       main

; double sqrt(double n)
; Returns a sqrt approximation based on the following code:
; Uses: R21-R30
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
  LDI.64    R25,  0.0         ; start
  LDA.64    R27,  8           ; end
  LDI.64    R29,  0.0         ; mid
sqrt_test:
  SUB.F64   R21,  R27,  R25
  LDI.64    R23,  0.000001
  CPGQ.F64  R21,  R23
  BRZ       sqrt_ret

  ADD.F64   R29,  R25,  R27
  DIVI.F64  R29,  R29,  2.0

  MUL.F64   R21,  R29,  R29
  LDA.64    R23,  8
  CPLT.F64  R21,  R23
  BRZ       sqrt_next_if
  MOV.64    R25,  R29
sqrt_next_if:
  MUL.F64   R21,  R29,  R29
  LDA.64    R23,  8
  CPGQ.F64  R21,  R23
  BRZ       sqrt_test
  MOV.64    R27,  R29
  JMP       sqrt_test
sqrt_ret:
  MOVS.64   R29
  RET.64    8

; double pi(int precision)
; Returns a rough pi approximation based on the following code:
; In: precision R24
; Out: R25-R26
; Uses: R22-R30
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
  LDI       R22,  1     ; int i
  LDI       R23,  1     ; int sign
  LDI.64    R25,  0     ; double result
pi_test:
  CPLT      R22,  R24
  BRZ       pi_ret

  CPZ       R23
  BRNZ      pi_else
  LDI.64    R27,  1.0
  ITOD      R29,  R22
  DIV.F64   R27,  R27,  R29
  ADD.F64   R25,  R25,  R27
  LDI       R23,  0
  JMP       pi_loop_inc
pi_else:
  LDI.64    R27,  1.0
  ITOD      R29,  R22
  DIV.F64   R27,  R27,  R29
  SUB.F64   R25,  R25,  R27
  LDI       R23,  1
pi_loop_inc:
  ADDI      R22,  R22,  2
  JMP       pi_test
pi_ret:
  MULI.F64  R25,  R25,  4.0
  RET       0

; void dmemset(int dstAddress, int count, double val)
; Fills out a block of count number of doubles with val.
; Uses: R16-R21
;
; for (int i = 0, int i < count; ++i) {
;     mem[dstAddress + i * 8] = val;
; }
dmemset:
  LDA       R16,  4   ; dstAddress
  LDA       R17,  8   ; count
  LDA.64    R18,  16  ; val
  LDI       R20,  0   ; i

dmemset_test:
  CPLT      R20,  R17
  BRZ       dmemset_ret

  MULI      R21,  R20,  8
  ADD       R21,  R21,  R16
  STM.64    R21,  R18

  ADDI      R20,  R20,  1
  JMP       dmemset_test
dmemset_ret:
  RET       16

; double calc_volume(double radius)
; Does a bunch of unnecessary conversions to finally end up with
; volume of a sphere with the given radius.
; Uses: R20-R26
calc_volume:
  LDA.64    R20,  8
  ; Radius to surface area: A = 4πr^2
  LDI       R24,  2000  ; pi precision
  CALL      pi
  MULI.F64  R25,  R25,  4.0
  MUL.F64   R25,  R25,  R20
  MUL.F64   R20,  R25,  R20
  STL.64    8,    R20

  ; Surface area to radius: r = sqrt(A/(4π))
  LDI       R24,  2000  ; pi precision
  CALL      pi
  MULI.F64  R25,  R25,  4.0
  DIV.F64   R20,  R20,  R25
  MOVS.64   R20
  CALL      sqrt
  POP.64    R20

  ; Radius to volume: V = (4/3)*πr^3
  LDI       R24,  2000  ; pi precision
  CALL      pi
  MUL.F64   R23,  R25,  R20
  MUL.F64   R23,  R23,  R20
  MUL.F64   R23,  R23,  R20
  LDI.64    R20,  4.0
  DIVI.F64  R20,  R20,  3.0
  MUL.F64   R20,  R20,  R23
  MOVS.64   R20
  RET.64    8

; double calc_avg_volume(int spheresBegin, int sphereCount)
; Uses: R16-R22
calc_avg_volume:
  LDI       R16,  0   ; i
  LDI.64    R17,  0   ; sum
  LDA       R19,  4   ; spheresBegin

calc_avg_volume_test:
  LDA       R20,  8   ; sphereCount
  CPLT      R16,  R20
  BRZ       calc_avg_volume_ret

  MULI      R21,  R16,  8
  ADD       R21,  R21,  R19
  LDM.64    R21,  R21
  MOVS.64   R21
  CALL      calc_volume
  POP.64    R21
  ADD.F64   R17,  R17,  R21

  ADDI      R16,  R16,  1
  JMP       calc_avg_volume_test
calc_avg_volume_ret:
  LDA       R20,  8
  ITOD      R20,  R20
  DIV.F64   R17,  R17,  R20
  MOVS.64   R17
  RET.64    8

; Entry point
main:
  LDI       R0,   0     ; spheresBegin
  LDI       R1,   0     ; spheresCount

  LDI.64    R21,  25.0
  LDI.64    R23,  0.000001
  CPGQ.F64  R21,  R23

  ; Allocate 10,000 spheres, i.e. 10,000 radii (double).
  LDI       R1,   10 000
  MULI      R2,   R1,   8
  NEW       R0,   R2

  ; Fill up the memory with 5.0.
  PUSH.64   5.0
  MOVS      R1
  MOVS      R0
  CALL      dmemset

  ; Now calculate the average volume of the spheres.
  MOVS      R1
  MOVS      R0
  CALL      calc_avg_volume

  EXIT

.DATA
