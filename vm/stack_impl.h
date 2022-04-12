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

#include "shared_impl.h"

/* Implements an interpreter loop with switch dispatch for the 
 * stack architecture. Through the use of a union, operands may be 
 * accessed arbitrarily, without the need for decoding them first. */
int StackInterpreterLoop()
{
    char *tmp1;
    char *tmp2;
    int32_t tmpInt;
    Reinterpret_t reinterpret;

    /* In order to maintain consistency between LDL/STL and LDA/STA, locals must 
     * be accessed through this, since it is 4 bytes higher than 'stackFrame'.
     * This is to remove the need for adding 4 bytes to the offset each time
     * you load and store locals, which happens A LOT. */
    int32_t *stackFrameLocals = stackFrame + 1; 

    while (instrPtr < instrEnd)
    {
        instr = *(Instr_t *)instrPtr;

        switch (DECODE_OPCODE())
        {
            case NOP: SHARED_NOP(); break;

            case EXIT: SHARED_EXIT();

            case JMP: SHARED_JMP(); break;

            case CALL: SHARED_CALL(); break;

            case RET: SHARED_RET(); break;

            case RET_32: SHARED_RET_32(); break;

            case RET_64: SHARED_RET_64(); break;

            case SCALL: SHARED_SCALL(); break;

            case SARG: SHARED_SARG(); break;

            /**** Load & Store ****/

            case S_LDI: *(int32_t*)++sp = DECODE_32(i32, C, 0);
                instrPtr += 5;
                break;

            case S_LDI_64: ++sp; *(int64_t*)sp++ = DECODE_64(i64, C, 0);
                instrPtr += 9;
                break;

            case S_STM: *(int32_t*)(heap + *sp) = *(int32_t*)(sp-1);
                sp -= 2;
                instrPtr += 1;
                break;

            case S_STM_64: *(int64_t*)(heap + *sp) = *(int64_t*)(sp-2);
                sp -= 3;
                instrPtr += 1;
                break;

            case S_STMI: *(int32_t*)(heap + *sp + DECODE_ADDR()) = *(int32_t*)(sp-1);
                sp -= 2;
                instrPtr += 5;
                break;

            case S_STMI_64: *(int64_t*)(heap + *sp + DECODE_ADDR()) = *(int64_t*)(sp-2);
                sp -= 3;
                instrPtr += 5;
                break;

            case S_LDM: *(int32_t*)sp = *(int32_t*)(heap + *sp);
                instrPtr += 1;
                break;

            case S_LDM_64: *(int64_t*)sp = *(int64_t*)(heap + *sp);
                ++sp;
                instrPtr += 1;
                break;

            case S_LDMI: *(int32_t*)sp = *(int32_t*)(heap + *sp + DECODE_ADDR());
                instrPtr += 5;
                break;

            case S_LDMI_64: *(int64_t*)sp = *(int64_t*)(heap + *sp + DECODE_ADDR());
                ++sp;
                instrPtr += 5;
                break;

            case S_LDL: *(int32_t*)++sp = *(int32_t*)((uint8_t*)stackFrameLocals + DECODE_8(u8, C, 0));
                instrPtr += 2;
                break;

            case S_LDL_64: *(int64_t*)++sp = *(int64_t*)((uint8_t*)stackFrameLocals + DECODE_8(u8, C, 0));
                ++sp;
                instrPtr += 2;
                break;

            case S_LDA: *(int32_t*)++sp = *(int32_t*)((uint8_t*)stackFrame - DECODE_8(u8, C, 0));
                instrPtr += 2;
                break;

            case S_LDA_64: *(int64_t*)++sp = *(int64_t*)((uint8_t*)stackFrame - DECODE_8(u8, C, 0));
                ++sp;
                instrPtr += 2;
                break;

            case S_STL: *(int32_t*)((uint8_t*)stackFrameLocals + DECODE_8(u8, C, 0)) = *(int32_t*)sp--;
                instrPtr += 2;
                break;

            case S_STL_64: *(int64_t*)((uint8_t*)stackFrameLocals + DECODE_8(u8, C, 0)) = *(int64_t*)--sp;
                --sp;
                instrPtr += 2;
                break;

            case S_STA: *(int32_t*)((uint8_t*)stackFrame - DECODE_8(u8, C, 0)) = *(int32_t*)sp--;
                instrPtr += 2;
                break;

            case S_STA_64: *(int64_t*)((uint8_t*)stackFrame - DECODE_8(u8, C, 0)) = *(int64_t*)--sp;
                --sp;
                instrPtr += 2;
                break;

            /**** Arithmetics ****/

/* Consumes 2 32-bit values and pushes a 32-bit value. */
#define STACK_OP_32(type, op) *(type)--sp = *(type)(sp-1) MACRO_LITERAL(op) *(type)(sp)

/* Consumes 2 64-bit values and pushes a 64-bit value. */
#define STACK_OP_64(type, op) sp -= 3; *(type)sp++ = *(type)sp MACRO_LITERAL(op) *(type)(sp+2)

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

            /**** Bit Stuff ****/

            case S_INV: *(int32_t*)sp = ~(int32_t)*sp;
                instrPtr += 1;
                break;

            case S_INV_64: *(int64_t*)(sp-1) = ~(int64_t)*(sp-1);
                instrPtr += 1;
                break;

            case S_NEG: *(int32_t*)sp = -(*(int32_t*)sp);
                instrPtr += 1;
                break;

            case S_NEG_64: *(int64_t*)(sp-1) = -(*(int64_t*)(sp-1));
                instrPtr += 1;
                break;

            case S_NEG_F: *(float*)sp = -(*(float*)sp);
                instrPtr += 1;
                break;

            case S_NEG_F64: *(double*)(sp-1) = -(*(double*)(sp-1));
                instrPtr += 1;
                break;

            case S_BOR: STACK_OP_32(int32_t *, |);
                instrPtr += 1;
                break;

            case S_BOR_64: STACK_OP_64(int64_t *, |);
                instrPtr += 1;
                break;

            case S_BXOR: STACK_OP_32(int32_t *, ^);
                instrPtr += 1;
                break;

            case S_BXOR_64: STACK_OP_64(int64_t *, ^);
                instrPtr += 1;
                break;

            case S_BAND: STACK_OP_32(int32_t *, &);
                instrPtr += 1;
                break;

            case S_BAND_64: STACK_OP_64(int64_t *, &);
                instrPtr += 1;
                break;

            /**** Comparisons ****/

/* Consumes 2 32-bit values and pushes a bool value (int32_t). */
#define STACK_OP_32_BOOL(type, op) *(int32_t*)--sp = *(type)(sp-1) MACRO_LITERAL(op) *(type)(sp)

/* Consumes 2 64-bit values and pushes a bool value (int32_t). */
#define STACK_OP_64_BOOL(type, op) sp -= 3; *(int32_t*)sp = *(type)sp MACRO_LITERAL(op) *(type)(sp+2)

            case S_OR: STACK_OP_32(int32_t *, ||);
                instrPtr += 1;
                break;

            case S_AND: STACK_OP_32(int32_t *, &&);
                instrPtr += 1;
                break;

            case S_CPZ: *(int32_t*)sp = !(*(int32_t*)sp);
                instrPtr += 1;
                break;

            case S_CPZ_64: *(int64_t*)(sp-1) = !(*(int64_t*)(sp-1));
                instrPtr += 1;
                break;

            case S_CPEQ: STACK_OP_32_BOOL(int32_t*, ==);
                instrPtr += 1;
                break;

            case S_CPEQ_64: STACK_OP_64_BOOL(int64_t*, ==);
                instrPtr += 1;
                break;

            case S_CPEQ_F: STACK_OP_32_BOOL(float*, ==);
                instrPtr += 1;
                break;

            case S_CPEQ_F64: STACK_OP_64_BOOL(double*, ==);
                instrPtr += 1;
                break;

            case S_CPNQ: STACK_OP_32_BOOL(int32_t*, !=);
                instrPtr += 1;
                break;

            case S_CPNQ_64: STACK_OP_64_BOOL(int64_t*, !=);
                instrPtr += 1;
                break;

            case S_CPNQ_F: STACK_OP_32_BOOL(float*, !=);
                instrPtr += 1;
                break;

            case S_CPNQ_F64: STACK_OP_64_BOOL(double*, !=);
                instrPtr += 1;
                break;

            case S_CPGT: STACK_OP_32_BOOL(int32_t*, >);
                instrPtr += 1;
                break;

            case S_CPGT_64: STACK_OP_64_BOOL(int64_t*, >);
                instrPtr += 1;
                break;

            case S_CPGT_F: STACK_OP_32_BOOL(float*, >);
                instrPtr += 1;
                break;

            case S_CPGT_F64: STACK_OP_64_BOOL(double*, >);
                instrPtr += 1;
                break;

            case S_CPLT: STACK_OP_32_BOOL(int32_t*, <);
                instrPtr += 1;
                break;

            case S_CPLT_64: STACK_OP_64_BOOL(int64_t*, <);
                instrPtr += 1;
                break;

            case S_CPLT_F: STACK_OP_32_BOOL(float*, <);
                instrPtr += 1;
                break;

            case S_CPLT_F64: STACK_OP_64_BOOL(double*, <);
                instrPtr += 1;
                break;

            case S_CPGQ: STACK_OP_32_BOOL(int32_t*, >=);
                instrPtr += 1;
                break;

            case S_CPGQ_64: STACK_OP_64_BOOL(int64_t*, >=);
                instrPtr += 1;
                break;

            case S_CPGQ_F: STACK_OP_32_BOOL(float*, >=);
                instrPtr += 1;
                break;

            case S_CPGQ_F64: STACK_OP_64_BOOL(double*, >=);
                instrPtr += 1;
                break;

            case S_CPLQ: STACK_OP_32_BOOL(int32_t*, <=);
                instrPtr += 1;
                break;

            case S_CPLQ_64: STACK_OP_64_BOOL(int64_t*, <=);
                instrPtr += 1;
                break;

            case S_CPLQ_F: STACK_OP_32_BOOL(float*, <=);
                instrPtr += 1;
                break;

            case S_CPLQ_F64: STACK_OP_64_BOOL(double*, <=);
                instrPtr += 1;
                break;

            case S_CPSTR: *--sp = strcmp(heap + *(sp-1), heap + *sp) == 0;
                instrPtr += 1;
                break;

            case S_CPCHR: *--sp = *(heap + *(sp-1)) == *(heap + *sp);
                instrPtr += 1;
                break;

            case S_BRZ: instrPtr = !*sp-- ? program + DECODE_ADDR() : instrPtr + 5;
                break;

            case S_BRNZ: instrPtr = *sp-- ? program + DECODE_ADDR() : instrPtr + 5;
                break;


            case S_BRIZ: sp -= 2; instrPtr = !*(sp+1) ? program + *(uint32_t*)(sp+2) : instrPtr + 1;
                break;

            case S_BRINZ: sp -= 2; instrPtr = *(sp+1) ? program + *(uint32_t*)(sp+2) : instrPtr + 1;
                break;

            case S_JMPI: instrPtr = program + *(uint32_t*)sp--;
                break;

            /**** Conversions ****/

            case S_ITOL: *(int64_t*)sp++ = (int64_t)*sp;
                instrPtr += 1;
                break;

            case S_ITOF: *(float*)sp = (float)*sp;
                instrPtr += 1;
                break;

            case S_ITOD: *(double*)sp++ = (double)*sp;
                instrPtr += 1;
                break;

            case S_ITOS: snprintf(strBuf, 32, "%d", *(int32_t*)sp); 
                *sp = HeapAllocString(strBuf);
                instrPtr += 1;
                break;

            case S_LTOI: *(int32_t*)--sp = (int32_t)(*(int64_t*)(i32sp-1));
                instrPtr += 1;
                break;

            case S_LTOF: *(float*)--sp = (float)(*(int64_t*)(i32sp-1));
                instrPtr += 1;
                break;

            case S_LTOD: *(double*)(sp-1) = (double)(*(int64_t*)(i32sp-1));
                instrPtr += 1;
                break;

            case S_LTOS: snprintf(strBuf, 32, "%lld", *(int64_t*)(--sp)); 
                *sp = HeapAllocString(strBuf);
                instrPtr += 1;
                break;

            case S_FTOI: *(int32_t*)sp = (int32_t)(*f32sp);
                instrPtr += 1;
                break;

            case S_FTOL: *(int64_t*)sp++ = (int64_t)(*f32sp);
                instrPtr += 1;
                break;

            case S_FTOD: *(double*)sp++ = (double)(*f32sp);
                instrPtr += 1;
                break;

            case S_FTOS: tmpInt = DECODE_8(u8, C, 0);
                snprintf(strBuf, 32, "%.*f", (tmpInt == 0xFF ? 3 : tmpInt), *f32sp); 
                *sp = HeapAllocString(strBuf);
                instrPtr += 2;
                break;

            case S_DTOI: *(int32_t*)--sp = (int32_t)(*(double*)(i32sp-1));
                instrPtr += 1;
                break;

            case S_DTOF: *(float*)--sp = (float)(*(double*)(i32sp-1));
                instrPtr += 1;
                break;

            case S_DTOL: *(int64_t*)(sp-1) = (int64_t)(*(double*)(i32sp-1));
                instrPtr += 1;
                break;

            case S_DTOS: tmpInt = DECODE_8(u8, C, 0);
                snprintf(strBuf, 32, "%.*f", (tmpInt == 0xFF ? 3 : tmpInt), *(double*)((int32_t*)--sp)); 
                *sp = HeapAllocString(strBuf);
                instrPtr += 2;
                break;

            case S_STOI: tmp1 = heap + *sp; *(int32_t*)sp = strtol(tmp1, &tmp2, 10);
                *i32sp = tmp2 == tmp1 ? DECODE_32(i32, C, 0) : *sp;
                instrPtr += 5;
                break;

            case S_STOL: tmp1 = heap + *sp; *(int64_t*)sp = strtoll(tmp1, &tmp2, 10);
                *(int64_t*)sp++ = tmp2 == tmp1 ? DECODE_64(i64, C, 0) : *(int64_t*)sp;
                instrPtr += 9;
                break;

            case S_STOF: reinterpret.intVal = DECODE_32(i32, C, 0);
                tmp1 = heap + *sp; *(float*)sp = strtof(tmp1, &tmp2);
                *(float*)sp = tmp2 == tmp1 ? reinterpret.fltVal : *(float*)sp;
                instrPtr += 5;
                break;

            case S_STOD: reinterpret.longVal = DECODE_64(i64, C, 0);
                tmp1 = heap + *sp; *(double*)sp = strtod(tmp1, &tmp2);
                *(double*)sp++ = tmp2 == tmp1 ? reinterpret.dblVal : *(double*)sp;
                instrPtr += 9;
                break;

            /**** Miscellaneous ****/

            case S_NEW: *sp = HeapAlloc(*sp);
                instrPtr += 1;
                break;

            case S_DEL: HeapFree(*sp--);
                instrPtr += 1;
                break;

            case S_RESZ: *--sp = HeapRealloc(*sp, *(sp-1));
                instrPtr += 1;
                break;

            case S_SIZE: *sp = GetHeapAllocSize(*sp);
                instrPtr += 1;
                break;

            case S_STR: *++sp = HeapAllocString((const char *)(program + DECODE_ADDR()));
                instrPtr += 5;
                break;

            case S_STRCPY: *sp = HeapAllocSubStr((const char *)(heap + *sp), DECODE_32(u32, C, 0));
                instrPtr += 5;
                break;

            case S_STRCAT: *sp = HeapAllocCombinedString(heap + *sp, program + DECODE_ADDR());
                instrPtr += 5;
                break;

            case S_STRCMB: *--sp = HeapAllocCombinedString(heap + *(sp-1), heap + *sp);
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
