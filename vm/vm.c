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
#include <stdbool.h>
#include <string.h>

#include "vm_memory.h"

/* Disables the automatic dump of the stack to stdout on program exit. 
 * This only takes effect when compiling in debug mode. */
/* #define NO_STACK_DUMP */

/* The number of 32-bit elements on the stack. 512 = 2 KiB stack. */
#define STACK_SIZE 512

/* VM runtime exit codes. */
#define VM_EXIT_SUCCESS        0
#define VM_EXIT_FAILURE        100
#define VM_EXIT_STACK_OVERFLOW 101

#define MACRO_LITERAL(x) x

typedef enum {
    VM_MODE_REGISTER = 0,
    VM_MODE_STACK = 1
} VMMode_t;

typedef enum {
    SYSFUNC_PRINT = 0,
    SYSFUNC_INPUT = 1,
    SYSFUNC_WRITE = 2,
    SYSFUNC_READ  = 3,
    SYSFUNC_OPEN  = 4,
    SYSFUNC_CLOSE = 5,
    SYSFUNC_STR   = 6
} SysFunc_t;

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
#ifdef UNION_DECODING
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
        uint8_t opcode;
        float C;
    } f32;

    struct GCC_PACK {
        uint8_t opcode;
        double C;
    } f64;
#endif
    struct GCC_PACK {
        uint8_t  opcode;
        uint32_t operand[3];
    };
} Instr_t;

#ifndef __GNUC__
    #pragma pack()
#endif

#ifdef UNION_DECODING
    #define DECODE(layout, type, field, offset, mask) instr.layout.field
    #define DECODE_ADDR() instr.u32.C
    #define DECODE_8( layout, field, offset) instr.layout.field
    #define DECODE_32(layout, field, offset) instr.layout.field
    #define DECODE_64(layout, field, offset) instr.layout.field
    #define DECODE_F32(layout, field, offset) instr.layout.field
    #define DECODE_F64(layout, field, offset) instr.layout.field
    #define DECODE_OPCODE() instr.opcode
#else
    #define DECODE(layout, type, field, offset, mask) ((*(type *)(instr.raw + 1 + offset) & mask) >> (offset * 8))
    #define DECODE_ADDR() ((uint32_t)(DECODE(layout, uint32_t, field, offset, 0xFFFFFFFF)))
    #define DECODE_8( layout, field, offset) ((uint8_t)(DECODE(layout, uint8_t, field, offset, 0xFF)))
    #define DECODE_32(layout, field, offset) ((int32_t)(DECODE(layout, int32_t, field, offset, 0xFFFFFFFF)))
    #define DECODE_64(layout, field, offset) ((int32_t)(DECODE(layout, int32_t, field, offset, 0xFFFFFFFFFFFFFFFF)))
    #define DECODE_F32(layout, field, offset) ((float)((*(uint32_t *)(instr.raw + 1 + offset) & 0xFFFFFFFF) >> (offset * 8)))
    #define DECODE_F64(layout, field, offset) ((double)((*(uint64_t *)(instr.raw + 1 + offset) & 0xFFFFFFFFFFFFFFFF) >> (offset * 8)))
    #define DECODE_OPCODE() (*(uint8_t *)(&instr) & 0xFF)
#endif

/**** GLOBALS ****/

static int32_t  *sp;         /* Stack pointer (top-of-stack). */
static int32_t  *stackBegin; /* Pointer to the beginning of the stack. */
static int32_t  *stackEnd;   /* Pointer to the beginning of the stack. */
static int32_t  *stackFrame; /* Pointer to the current function's stack frame. */
static Instr_t  instr;       /* Instruction "register". */
static uint8_t  *instrPtr;   /* Pointer to the next instruction. */
static uint8_t  *program;    /* Pointer to start of program memory. */
static uint8_t  *programEnd; /* Pointer to end of program memory (incl. data). */
static uint8_t  *instrEnd;   /* Pointer to end of instructions in program memory. */
static VMMode_t vmMode;
static uint8_t  sysArgs[8];  /* Holds temporary size information about system function arguments. */
static uint8_t  *sysArgPtr;  /* This and sysArgs is used only for variadic system function calls. */
static char     strBuf[128];

/* Shorthand macros for casting the stack pointer. */
#define i32sp ((int32_t*)sp)
#define u32sp ((unt32_t*)sp)
#define i64sp ((int64_t*)sp)
#define f32sp ((float*)sp)
#define f64sp ((double*)sp)

#include "opcodes.h"

/* The implementations of the stack and register interpreter loops are 
 * separated into their own files for readability. They are included here,
 * and HERE ONLY, as this only meant to be a copy-paste situation. */
#include "stack_impl.h"
#include "register_impl.h"

/* RackVM Stack Frame: (offsets are in bytes. 4 bytes = 1 word in RackVM.)
      
           +* nth local variable
           +8 first local variable
           +4 return address          (refers to program memory)
stackFrame -> offset to previous ptr. (refers to stack)
           -4 1st function argument
           -* nth function argument
*/

static bool ReadProgram(const char *fileName)
{
    FILE *file = fopen(fileName, "rb");
    if (!file)
        return false;

    uint32_t header[4] = {0};
    size_t headerRead = fread(header, sizeof(uint32_t), 4, file);
    if (headerRead != 4)
    {
        printf("Malformed program header.\n");
        fclose(file);
        return false;
    }

    /* 0 = vm mode, 1 = initial heap size, 2 = max heap size, 3 = data section start address. */
    vmMode = header[0];

    /* Convert heap sizes from KiB to bytes. */
    header[1] *= 1024;
    header[2] *= 1024;

    if (!AllocateHeap(header[1], header[2]))
    {
        printf("Failed to allocate %llu heap memory (max %llu)!\n", 
            header[1], header[2]);
        return false;
    }

    size_t headerEnd = ftell(file);
    fseek(file, 0, SEEK_END);

    /* Get the program size excluding the header and data section, and 
     * return to the end of the header. */
    size_t programSize = ftell(file) - headerEnd;
    fseek(file, headerEnd, SEEK_SET);

    /* Dynamically allocate the program memory. */
    program = malloc(programSize);
    instrEnd = program + header[3];
    programEnd = program + programSize;

    /* Copy the program file's contents to the designated block of memory. */
    fread(program, 1, programSize, file);
    
    fclose(file);

    /* Setup some other pointers. */
    instrPtr = program;
    sysArgPtr = sysArgs;

    return true;
}

static void AllocateStack()
{
    stackBegin = malloc(STACK_SIZE * sizeof(int32_t));
    stackEnd = stackBegin + STACK_SIZE;
    sp = stackBegin;
    stackFrame = stackBegin; /* For function call stack. */

    *sp++ = 0xAC1D;
    *sp = 0xFACE;
}

static void DumpStack()
{
    /* Print the stack up until sp */
    puts(  "======== STACK DUMP ===================================================================");
    printf("          %-3s %-10s %-20s %-12s %-12s    %-s \n", "[]", "i32", "i64", "f32", "f64", "hex");
    puts(  "---------------------------------------------------------------------------------------");

    int32_t *currSp = sp;

    snprintf(strBuf, 12, "%-11f", *(float*)currSp);
    snprintf(strBuf+16, 16, "%-16lf", *(double*)currSp);
    printf("SP ==32=> %-3lu %-10ld %-20lld %-12s %-12s 0x%-0X \n", 
        currSp - stackBegin, *currSp, *(int64_t*)currSp, strBuf, strBuf+16, *currSp);

    currSp = sp-1;
    snprintf(strBuf, 12, "%-11f", *(float*)currSp);
    snprintf(strBuf+16, 16, "%-16lf", *(double*)currSp);
    printf("   --64-> %-3lu %-10ld %-20lld %-12s %-12s 0x%-0X \n", 
        currSp - stackBegin, *currSp, *(int64_t*)currSp, strBuf, strBuf+16, *currSp);

    int32_t *i;
    for (i = sp - 2; i >= stackBegin; --i)
    {
        currSp = i;
        snprintf(strBuf, 12, "%-11f", *(float*)currSp);
        snprintf(strBuf+16, 16, "%-16lf", *(double*)currSp);
        printf("          %-3lu %-10ld %-20lld %-12s %-12s 0x%-0X \n",
            currSp - stackBegin, *currSp, *(int64_t*)currSp, strBuf, strBuf+16, *currSp);
    }

#undef PRINT_SP

    puts("---------------------------------------------------------------------------------------");
}

static void Cleanup()
{
    DeallocateHeap();
    free(stackBegin);
    free(program);
}

int main(int argc, const char **argv)
{
#ifndef NDEBUG
    #if UNION_DECODING
        puts("[RackVM] Decoding instructions using the union technique.");
    #else
        puts("[RackVM] Decoding instructions using the bitmasking technique.");
    #endif
#endif
    /* First of all, do a runtime check for the size of Instr_t. 
     * If this does not match, instructions will be misinterpreted,
     * and the union "decoding" technique would be meaningless.*/
    if (sizeof(Instr_t) != 13)
    {
        printf("[RackVM] Invalid size of instruction struct (%llu).\nAborting.\n", sizeof(Instr_t));
        return 0;
    }

    if (argc != 2)
    {
        printf("[RackVM] Invalid arguments.\n");
        return 0;
    }

    if (!ReadProgram(argv[1]))
    {
        printf("[RackVM] Couldn't read file \"%s\".\n", argv[1]);
        return 0;
    }

    AllocateStack();

    int exitCode;
    if (vmMode == VM_MODE_STACK)
        exitCode = StackInterpreterLoop();
    else
        exitCode = 0;

    if (exitCode != VM_EXIT_SUCCESS)
        printf("[RackVM] Exited with exit code %d", exitCode);

    /* Check and report on potential stack corruption. */
    if (stackBegin[0] != 0xAC1D || stackBegin[1] != 0xFACE)
    {
        puts("[RackVM] Warning: stack was corrupted during execution (underflow).\n");
    }

#if !defined(NDEBUG) && !defined(NO_STACK_DUMP)
    DumpStack();
#endif

    Cleanup();
    return 0;
}
