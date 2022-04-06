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

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

/* The number of 32-bit elements on the stack. 512 = 2 KiB stack. */
#define STACK_SIZE 512

/* VM runtime exit codes. */
#define VM_EXIT_SUCCESS        0
#define VM_EXIT_FAILURE        100
#define VM_EXIT_STACK_OVERFLOW 101

typedef enum {
    VM_MODE_REGISTER = 0,
    VM_MODE_STACK = 1
} VMMode_t;

typedef enum {
    NOP         = 0x00,
    EXIT        = 0x01,

    /******** STACK-BASED INSTRUCTIONS ********/
    /* Load & Store */
    S_LDI       = 0x02,
    S_LDI_64,
    S_STM,
    S_STM_64,
    S_STMI,
    S_STMI_64,
    S_LDM,
    S_LDM_64,
    S_LDMI,
    S_LDMI_64,
    S_LDL,
    S_LDL_64,
    S_LDA,
    S_LDA_64,
    S_STL,
    S_STL_64,
    S_STA,
    S_STA_64,
    /* Arithmetics */
    S_ADD       = 0x14,
    S_ADD_64,
    S_ADD_F,
    S_ADD_F64,
    S_SUB,
    S_SUB_64,
    S_SUB_F,
    S_SUB_F64,
    S_MUL,
    S_MUL_64,
    S_MUL_F,
    S_MUL_F64,
    S_DIV,
    S_DIV_64,
    S_DIV_F,
    S_DIV_F64,


    /* Bit Stuff */


    /* Conditions & Branches */


    /* Conversions */


    /* Miscellaneous */


    OPCODE_COUNT
} Opcode_t;

#ifdef __GNUC__
    #define GCC_PACK __attribute__ ((__packed__))
#else
    #define GCC_PACK
#endif

#ifndef __GNUC__
    #pragma pack(1)
#endif

typedef union {
    uint8_t raw[13];
    
    struct GCC_PACK {
        uint8_t opcode;
        uint8_t C;
    } u8;

    struct GCC_PACK {
        uint8_t opcode;
        int32_t C;
    } i32;

    struct GCC_PACK {
        uint8_t  opcode;
        uint32_t C;
    } u32;

    struct GCC_PACK {
        uint8_t opcode;
        int64_t C;
    } i64;

    struct GCC_PACK {
        uint8_t  opcode;
        uint32_t operand[3];
    };
} Instr_t;

#ifndef __GNUC__
    #pragma pack()
#endif

typedef union {
    uint8_t u8;
    int32_t i32;
    int64_t i64;
    int32_t f32;
    int64_t f64;
} StackValue_t;

/**** GLOBALS ****/

static int32_t *stackBegin; /* Pointer to the beginning of the stack. */
static int32_t *stackEnd;   /* Pointer to the beginning of the stack. */
static int32_t *sp;         /* Stack pointer (top-of-stack). */
static Instr_t instr;       /* Instruction "register". */
static uint8_t *instrPtr;   /* Pointer to the next instruction. */
static uint8_t *program;    /* Pointer to start of program memory. */
static uint8_t *programEnd; /* Pointer to end of program memory. */

/* 
 * Implements an interpreter loop with switch dispatch for the 
 * stack architecture. Through the use of a union, operands may be 
 * accessed arbitrarily, without the need for decoding them first.
*/
int StackInterpreterLoop()
{
    while (instrPtr < programEnd)
    {
        instr = *(Instr_t *)instrPtr;

        switch (instr.opcode)
        {
            case NOP: instrPtr += 1;
                break;

            case EXIT:
                return VM_EXIT_SUCCESS;

            /**** Load & Store ****/

            case S_LDI: *(int32_t*)++sp = instr.i32.C;
                instrPtr += 5;
                break;

            case S_LDI_64: ++sp; *(int64_t*)sp++ = instr.i64.C;
                instrPtr += 9;
                break;

            /**** Arithmetics ****/
#define STACK_OP(op) op
#define STACK_OP_32(type, op) *(type)--sp = *(type)(sp-1) STACK_OP(op) *(type)(sp)
#define STACK_OP_64(type, op) sp -= 3; *(type)sp++ = *(type)sp STACK_OP(op) *(type)(sp+2)

            case S_ADD: STACK_OP_32(int32_t *, +);
                instrPtr += 1;
                break;

            case S_ADD_64: STACK_OP_64(int64_t *, +);
                instrPtr += 1;
                break;

            case S_ADD_F: STACK_OP_32(float *, +);
                instrPtr += 1;
                break;

            case S_ADD_F64: STACK_OP_64(double *, +);
                instrPtr += 1;
                break;

            case S_SUB: STACK_OP_32(int32_t *, -);
                instrPtr += 1;
                break;

            case S_SUB_64: STACK_OP_64(int64_t *, -);
                instrPtr += 1;
                break;

            case S_SUB_F: STACK_OP_32(float *, -);
                instrPtr += 1;
                break;

            case S_SUB_F64: STACK_OP_64(double *, -);
                instrPtr += 1;
                break;

            case S_MUL: STACK_OP_32(int32_t *, *);
                instrPtr += 1;
                break;

            case S_MUL_64: STACK_OP_64(int64_t *, *);
                instrPtr += 1;
                break;

            case S_MUL_F: STACK_OP_32(float *, *);
                instrPtr += 1;
                break;

            case S_MUL_F64: STACK_OP_64(double *, *);
                instrPtr += 1;
                break;

            case S_DIV: STACK_OP_32(int32_t *, /);
                instrPtr += 1;
                break;

            case S_DIV_64: STACK_OP_64(int64_t *, /);
                instrPtr += 1;
                break;

            case S_DIV_F: STACK_OP_32(float *, /);
                instrPtr += 1;
                break;

            case S_DIV_F64: STACK_OP_64(double *, /);
                instrPtr += 1;
                break;

            default: 
                return VM_EXIT_FAILURE;
        }

        if (sp >= stackEnd)
            return VM_EXIT_STACK_OVERFLOW;
    }

    return VM_EXIT_SUCCESS;
}

int ReadProgram(const char *fileName)
{
    FILE *file = fopen(fileName, "rb");
    if (!file)
        return 0;

    uint32_t header[3] = {0};
    size_t headerRead = fread(header, sizeof(uint32_t), 3, file);
    if (headerRead != 3)
    {
        printf("Malformed program header.\n");
        fclose(file);
        return 0;
    }

    /* 0 = vm mode, 1 = initial heap size, 2 = max heap size. */
    VMMode_t mode = header[0];

    /*
        ... Here it should allocate initial heap memory.
        This will be implemented using a memory manager...
    */

    size_t headerEnd = ftell(file);
    fseek(file, 0, SEEK_END);

    /* Get the program size (excluding the header), and 
     * return to the end of the header. */
    size_t programSize = ftell(file) - headerEnd;
    fseek(file, headerEnd, SEEK_SET);

    /* Dynamically allocate the program memory. */
    program = malloc(programSize);
    programEnd = program + programSize;

    /* Copy the program file's contents to the designated block of memory. */
    fread(program, 1, programSize, file);
    
    fclose(file);

    instrPtr = program;

    return 1;
}

void AllocateStack()
{
    stackBegin = malloc(STACK_SIZE * sizeof(int32_t));
    stackEnd = stackBegin + STACK_SIZE;
    sp = stackBegin;

    *sp++ = 0xAC1D;
    *sp = 0xFACE;
}

void DumpStack()
{
    /* Print the stack up until sp */
    printf("======== STACK DUMP ==============================================\n");
    printf("    %-3s %-10s %-10s %-10s %-10s %-s \n", "[]", "i32", "i64", "f32", "f64", "hex");
    puts("------------------------------------------------------------------");
    printf(" -> %-3lu %-10d %-10ld %-10f %-10lf 0x%0X \n", 
        sp - stackBegin, *sp, *(sp-1), *(float*)sp, *(double*)(sp-1), *sp);

    int32_t *i;
    for (i = sp - 1; i >= stackBegin; --i)
        printf("    %-3lu %-10d %-10ld %-10f %-10lf 0x%0X \n", 
            i - stackBegin, *i, *(i-1), *(float*)i, *(double*)(i-1), *i);

    puts("------------------------------------------------------------------");
}

void Cleanup()
{
    free(stackBegin);
    free(program);
}

int main(int argc, const char **argv)
{
    /* First of all, do a runtime check for the size of Instr_t. 
     * If this does not match, instructions will be misinterpreted,
     * and the union "decoding" technique would be meaningless.*/
    if (sizeof(Instr_t) != 13)
    {
        printf("Invalid size of instructions (%llu).\nAborting.\n", sizeof(Instr_t));
        return 0;
    }

    if (argc != 2)
    {
        printf("Invalid arguments.\n");
        return 0;
    }

    if (!ReadProgram(argv[1]))
    {
        printf("Couldn't read file \"%s\".\n", argv[1]);
        return 0;
    }

    AllocateStack();

    int exitCode = StackInterpreterLoop();
    if (exitCode != VM_EXIT_SUCCESS)
        printf("Exited with exit code %d", exitCode);

    /* Check and report on potential stack corruption. */
    if (stackBegin[0] != 0xAC1D || stackBegin[1] != 0xFACE)
    {
        puts("Warning: stack was corrupted during execution (underflow).\n");
    }

    DumpStack();

    Cleanup();
    return 0;
}
