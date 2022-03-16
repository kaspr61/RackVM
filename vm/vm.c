/*
BSD 2-Clause License

Copyright (c) 2022, Kasper Skott

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <memory.h>
#include <stdint.h>
#include <stdio.h>

#define REGISTER_COUNT 32
#define USE_REG(r)  R_usedCount += R_used[r] ? 0 : 1; R_used[r] = 1
#define FREE_REG(r) R_usedCount -= R_used[r] ? 1 : 0; R_used[r] = 0
#define IsRegFree(r) (!R_used[r])
#define IsOutOfRegs(r) (R_usedCount >= REGISTER_COUNT)

typedef int32_t  Word;
typedef uint32_t Uword;
typedef uint32_t Ptr;

typedef union Instr {
    struct {
        uint8_t  opcode;
        uint8_t  R0;
        uint8_t  R1;
        uint8_t  R2;
    };
    struct {
        uint8_t  padding;
        uint8_t  R;
        uint16_t K;
    };
    uint32_t raw;
} Instr_t;

/* Declare registers. */
static Word R[REGISTER_COUNT];

static Instr_t IR; /* Instruction register. */

/*** INSTRUCTIONS ***/
enum OpCode {
    OPCODE_MOV = 0x00,
    OPCODE_LDI = 0x01,
    OPCODE_ADD = 0x02,

    OPCODE_COUNT
};

static void instr_MOV(void)
{
    *(R+IR.R0) = *(R+IR.R1);
}

static void instr_LDI(void)
{
    *(R+IR.R) = IR.K;
}

static void instr_ADD(void)
{
    *(R+IR.R0) = *(R+IR.R1) + *(R+IR.R2);
}

/* Declare an array of function pointers for instructions. */
static void(*DoInstruction[OPCODE_COUNT])(void) = {
    instr_MOV,
    instr_LDI,
    instr_ADD
    };

/********************/

int main(int argc, const char **argv)
{
    IR.raw = OPCODE_LDI;
    IR.R0 = 0x0;
    IR.K = 1024;

    //printf("IR = 0x%08X\n", IR.raw);

    printf("opcode = %d\nR0 = %d\nK = %d\n", IR.opcode, IR.R0, IR.K);

    DoInstruction[IR.opcode]();
    printf("\nR[0] = %d\n\n", R[0]);

    IR.raw = OPCODE_LDI;
    IR.R0 = 0x1;
    IR.K = 6;
    printf("opcode = %d\nR0 = %d\nK = %d\n", IR.opcode, IR.R0, IR.K);

    DoInstruction[IR.opcode]();
    printf("\nR[1] = %d\n\n", R[1]);
    
    IR.raw = OPCODE_ADD;
    IR.R0 = 0x0;
    IR.R1 = 0x0;
    IR.R2 = 0x1;
    printf("opcode = %d\nR0 = %d\nR1 = %d\nR2 = %d\n", IR.opcode, IR.R0, IR.R1, IR.R2);

    DoInstruction[IR.opcode]();
    printf("\nR[0] = %d\n\n", R[0]);  
    
    return 0;
}
