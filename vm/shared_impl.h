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

#ifndef INC_SHARED_IMPL_H
#define INC_SHARED_IMPL_H

/* Normally you might break the following macros out into functions,
   but since they meant to be part of the execution loop, performance
   really matters. I also made them macros because they are used in both
   the stack instruction set and the register instruction set. */

#define SHARED_NOP() instrPtr += 1

#define SHARED_EXIT() return VM_EXIT_SUCCESS

#define SHARED_JMP() instrPtr = program + DECODE_ADDR()

#define SHARED_CALL() \
    tmp1 = (char *)stackFrame;\
    stackFrame = ++sp;                                        /* Set new stack frame */\
    stackFrameLocals = stackFrame + 1;\
    *(int32_t*)(sp) = (int32_t)((int32_t*)tmp1 - stackBegin); /* Put offset to previous stack frame. */\
    *(Addr_t*)++sp = (Addr_t)((instrPtr + 5) - program);      /* Put return address. */\
    instrPtr = program + DECODE_ADDR()

#define SHARED_RET() \
    instrPtr = program + *(stackFrame + 1); /* Jump to return address. */\
    /* Set SP to current stack frame - size of args. */\
    sp = (int32_t*)((uint8_t*)stackFrame - DECODE_8(u8, C, 0)) - 1; \
    stackFrame = stackBegin + *stackFrame; /* Reset to previous stack frame. */\
    stackFrameLocals = stackFrame + 1

#define SHARED_RET_32() \
    tmp1 = (char *)sp; /* Save ptr to last value on stack. */\
    instrPtr = program + *(stackFrame + 1);\
    sp = (int32_t*)((uint8_t*)stackFrame - DECODE_8(u8, C, 0)) - 1; \
    stackFrame = stackBegin + *stackFrame;\
    stackFrameLocals = stackFrame + 1;\
    *(int32_t*)(++sp) = *(int32_t*)tmp1

#define SHARED_RET_64() \
    tmp1 = (char *)(sp-1); /* Save ptr to last value on stack. */\
    instrPtr = program + *(stackFrame + 1);\
    sp = (int32_t*)((uint8_t*)stackFrame - DECODE_8(u8, C, 0)) - 1; \
    stackFrame = stackBegin + *stackFrame;\
    stackFrameLocals = stackFrame + 1;\
    ++sp;\
    *(int64_t*)sp++ = *(int64_t*)tmp1

#define SHARED_SCALL() \
    /* Number of arguments system function call. */\
    tmpInt = sysArgPtr - sysArgs; \
    switch ((SysFunc_t)DECODE_8(u8, C, 0))\
    {\
        case SYSFUNC_PRINT: SysPrint(tmpInt);\
            break;\
        case SYSFUNC_INPUT: *++sp = HeapAllocString(fgets(strBuf, 128, stdin));\
            break;\
        case SYSFUNC_STR: SysStr(tmpInt);\
            break;\
    }\
    sysArgPtr = sysArgs;     /* Reset the pointer. */\
    *(uint64_t*)sysArgs = 0; /* Reset all 8 bytes to 0 at once. */\
    instrPtr += 2

/* To indicate a pointer type (e.g. string or array), set bit 7 (MSB) to 1 (0x80). */
/* To indicate a double type, set bit 6 to 1 (0x40). */
/* To indicate a float type, set bit 5 to 1 (0x20). */
/* To indicate a long type, set bit 4 to 1 (0x10). */
/* The default value is an int (32-bits). */
#define SHARED_SARG() \
    *sysArgPtr++ = DECODE_8(u8, C, 0);\
    instrPtr += 2


/*
 * Below is some ugly code. Hardcoding function calls that should really be
 * variadic really does get ugly, especially with support for differing types. 
*/

#define PRINT_VAL_SIZE(argIdx) ((sysArgs[argIdx] & 0x0F) / 4)
#define PRINT_ARG(argIdx) (argVal[argIdx].addr)

void SysPrint(int32_t argCnt)
{
    union {
        char    *addr;
        int32_t i32;
        int64_t i64;
        double  f64; /* No need for floats since only doubles are really used in printf, etc. */
    } argVal[8] = {0};

    size_t valueSizeSum = 0;
    uint8_t sysArgFlags;
    for (int i = argCnt - 1; i >= 0; --i)
    {
        sysArgFlags = sysArgs[i];
        if (sysArgFlags & 0x80)
        {
            argVal[i].addr = (char *)(heap + *(sp - valueSizeSum));
            ++valueSizeSum;
        }
        else if (sysArgFlags & 0x40)
        {
            argVal[i].f64 = *(double*)(sp - valueSizeSum - 1);
            valueSizeSum += 2;
        }
        else if (sysArgFlags & 0x20)
        {
            argVal[i].f64 = *(float*)(sp - valueSizeSum);
            ++valueSizeSum;
        }
        else if (sysArgFlags & 0x10)
        {
            argVal[i].i64 = *(int64_t*)(sp - valueSizeSum - 1);
            valueSizeSum += 2;
        }
        else
        {
            argVal[i].i32 = *(int32_t*)(sp - valueSizeSum);
            ++valueSizeSum;
        }
    }

    switch (argCnt)
    {
        case 0:
        case 1: printf(heap + *sp--); 
            break;
        case 2: printf(PRINT_ARG(0), PRINT_ARG(1)); 
            sp -= PRINT_VAL_SIZE(0) + PRINT_VAL_SIZE(1);
            break;
        case 3: printf(PRINT_ARG(0), PRINT_ARG(1), PRINT_ARG(2)); 
            sp -= PRINT_VAL_SIZE(0) + PRINT_VAL_SIZE(1) + PRINT_VAL_SIZE(2);
            break;
        case 4: printf(PRINT_ARG(0), PRINT_ARG(1), PRINT_ARG(2), PRINT_ARG(3)); 
            sp -= PRINT_VAL_SIZE(0) + PRINT_VAL_SIZE(1) + PRINT_VAL_SIZE(2) + 
                  PRINT_VAL_SIZE(3);
            break;
        case 5: printf(PRINT_ARG(0), PRINT_ARG(1), PRINT_ARG(2), PRINT_ARG(3),
                       PRINT_ARG(4)); 
            sp -= PRINT_VAL_SIZE(0) + PRINT_VAL_SIZE(1) + PRINT_VAL_SIZE(2) + 
                  PRINT_VAL_SIZE(3) + PRINT_VAL_SIZE(4);
            break;
        case 6: printf(PRINT_ARG(0), PRINT_ARG(1), PRINT_ARG(2), PRINT_ARG(3),
                       PRINT_ARG(4), PRINT_ARG(5)); 
            sp -= PRINT_VAL_SIZE(0) + PRINT_VAL_SIZE(1) + PRINT_VAL_SIZE(2) + 
                  PRINT_VAL_SIZE(3) + PRINT_VAL_SIZE(4) + PRINT_VAL_SIZE(5);
            break;
        case 7: printf(PRINT_ARG(0), PRINT_ARG(1), PRINT_ARG(2), PRINT_ARG(3),
                       PRINT_ARG(4), PRINT_ARG(5), PRINT_ARG(6)); 
            sp -= PRINT_VAL_SIZE(0) + PRINT_VAL_SIZE(1) + PRINT_VAL_SIZE(2) + 
                  PRINT_VAL_SIZE(3) + PRINT_VAL_SIZE(4) + PRINT_VAL_SIZE(5) +
                  PRINT_VAL_SIZE(6);
            break;
        case 8: printf(PRINT_ARG(0), PRINT_ARG(1), PRINT_ARG(2), PRINT_ARG(3),
                       PRINT_ARG(4), PRINT_ARG(5), PRINT_ARG(6), PRINT_ARG(7)); 
            sp -= PRINT_VAL_SIZE(0) + PRINT_VAL_SIZE(1) + PRINT_VAL_SIZE(2) + 
                  PRINT_VAL_SIZE(3) + PRINT_VAL_SIZE(4) + PRINT_VAL_SIZE(5) +
                  PRINT_VAL_SIZE(6) + PRINT_VAL_SIZE(7);
            break;
    }
}

void SysStr(int32_t argCnt)
{
    union {
        char    *addr;
        int32_t i32;
        int64_t i64;
        float   f32;
        double  f64;
    } argVal[8] = {0};

    size_t valueSizeSum = 0;
    uint8_t sysArgFlags;
    for (int i = argCnt - 1; i >= 0; --i)
    {
        sysArgFlags = sysArgs[i];
        if (sysArgFlags & 0x80)
        {
            argVal[i].addr = (char *)(heap + *(sp - valueSizeSum++));
            continue;
        }

        if (sysArgFlags & 0x40)
        {
            argVal[i].f64 = *(double*)(sp - valueSizeSum);
            valueSizeSum += 2;
            continue;
        }

        if (sysArgFlags & 0x20)
        {
            argVal[i].f32 = *(float*)(sp - valueSizeSum++);
            continue;
        }

        if (sysArgFlags & 0x10)
        {
            argVal[i].i64 = *(int64_t*)(sp - valueSizeSum);
            valueSizeSum += 2;
            continue;
        }

        argVal[i].i32 = *(int32_t*)(sp - valueSizeSum++);
    }

    switch (argCnt)
    {
        case 0:
        case 1: snprintf(strBuf, 128, heap + *sp--); 
            break;
        case 2: snprintf(strBuf, 128, PRINT_ARG(0), PRINT_ARG(1)); 
            sp -= PRINT_VAL_SIZE(0) + PRINT_VAL_SIZE(1);
            break;
        case 3: snprintf(strBuf, 128, PRINT_ARG(0), PRINT_ARG(1), PRINT_ARG(2)); 
            sp -= PRINT_VAL_SIZE(0) + PRINT_VAL_SIZE(1) + PRINT_VAL_SIZE(2);
            break;
        case 4: snprintf(strBuf, 128, PRINT_ARG(0), PRINT_ARG(1), PRINT_ARG(2), PRINT_ARG(3)); 
            sp -= PRINT_VAL_SIZE(0) + PRINT_VAL_SIZE(1) + PRINT_VAL_SIZE(2) + 
                  PRINT_VAL_SIZE(3);
            break;
        case 5: snprintf(strBuf, 128, PRINT_ARG(0), PRINT_ARG(1), PRINT_ARG(2), PRINT_ARG(3),
                       PRINT_ARG(4)); 
            sp -= PRINT_VAL_SIZE(0) + PRINT_VAL_SIZE(1) + PRINT_VAL_SIZE(2) + 
                  PRINT_VAL_SIZE(3) + PRINT_VAL_SIZE(4);
            break;
        case 6: snprintf(strBuf, 128, PRINT_ARG(0), PRINT_ARG(1), PRINT_ARG(2), PRINT_ARG(3),
                       PRINT_ARG(4), PRINT_ARG(5)); 
            sp -= PRINT_VAL_SIZE(0) + PRINT_VAL_SIZE(1) + PRINT_VAL_SIZE(2) + 
                  PRINT_VAL_SIZE(3) + PRINT_VAL_SIZE(4) + PRINT_VAL_SIZE(5);
            break;
        case 7: snprintf(strBuf, 128, PRINT_ARG(0), PRINT_ARG(1), PRINT_ARG(2), PRINT_ARG(3),
                       PRINT_ARG(4), PRINT_ARG(5), PRINT_ARG(6)); 
            sp -= PRINT_VAL_SIZE(0) + PRINT_VAL_SIZE(1) + PRINT_VAL_SIZE(2) + 
                  PRINT_VAL_SIZE(3) + PRINT_VAL_SIZE(4) + PRINT_VAL_SIZE(5) +
                  PRINT_VAL_SIZE(6);
            break;
        case 8: snprintf(strBuf, 128, PRINT_ARG(0), PRINT_ARG(1), PRINT_ARG(2), PRINT_ARG(3),
                       PRINT_ARG(4), PRINT_ARG(5), PRINT_ARG(6), PRINT_ARG(7)); 
            sp -= PRINT_VAL_SIZE(0) + PRINT_VAL_SIZE(1) + PRINT_VAL_SIZE(2) + 
                  PRINT_VAL_SIZE(3) + PRINT_VAL_SIZE(4) + PRINT_VAL_SIZE(5) +
                  PRINT_VAL_SIZE(6) + PRINT_VAL_SIZE(7);
            break;
    }

    *++sp = HeapAllocString(strBuf);
}

#undef PRINT_VAL_SIZE

#endif /* INC_SHARED_IMPL_H */