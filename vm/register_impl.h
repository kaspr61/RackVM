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
 * register architecture. Through the use of a union, operands may be 
 * accessed arbitrarily, without the need for decoding them first. */
int RegisterInterpreterLoop()
{
    void *tmp1;
    void *tmp2;
    void *tmp3;
    int32_t tmpInt;
    uint8_t tmpByte;
    Reinterpret_t reinterpret;

    /* In order to maintain consistency between LDL/STL and LDA/STA, locals must 
     * be accessed through this, since it is 4 bytes higher than 'stackFrame'.
     * This is to remove the need for adding 4 bytes to the offset each time
     * you load and store locals, which happens A LOT. */
    int32_t *stackFrameLocals = stackFrame + 1; 

    /* The last register is used for storing comparison results, and is
     * checked in conditional branches. */
    int32_t *cpr = stackBegin + 31;

/* Allows the access of registers as doubled registers, i.e. 64-bits.
 * This uses the register directly after. */
#define dreg(idx) (*(int64_t*)(reg+idx))

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

            case R_MOV: reg[DECODE_8(u8_u8, a, 0)] = reg[DECODE_8(u8_u8, b, 1)];
                instrPtr += 3;
                break;

            case R_MOV_64: dreg(DECODE_8(u8_u8, a, 0)) = dreg(DECODE_8(u8_u8, b, 1));
                instrPtr += 3;
                break;

            case R_LDI: reg[DECODE_8(u8_i32, a, 0)] = DECODE_32(u8_i32, C, 1);
                instrPtr += 6;
                break;

            case R_LDI_64: dreg(DECODE_8(u8_i64, a, 0)) = DECODE_64(u8_i64, C, 1);
                instrPtr += 10;
                break;

            case R_STM: *(int32_t*)(heap + reg[DECODE_8(u8_u8, a, 0)]) = reg[DECODE_8(u8_u8, b, 1)];
                instrPtr += 3;
                break;

            case R_STM_64: *(int64_t*)(heap + reg[DECODE_8(u8_u8, a, 0)]) = dreg(DECODE_8(u8_u8, b, 1));
                instrPtr += 3;
                break;

            case R_STMI: 
                *(int32_t*)(heap + reg[DECODE_8(u8_u8_u32, a, 0)] + DECODE_u32(u8_u8_u32, C, 2)) 
                    = reg[DECODE_8(u8_u8_u32, b, 1)];
                instrPtr += 7;
                break;

            case R_STMI_64: 
                *(int64_t*)(heap + reg[DECODE_8(u8_u8_u32, a, 0)] + DECODE_u32(u8_u8_u32, C, 2)) 
                    = dreg(DECODE_8(u8_u8_u32, b, 1));
                instrPtr += 7;
                break;

            case R_LDM: reg[DECODE_8(u8_u8, a, 0)] = *(int32_t*)(heap + reg[DECODE_8(u8_u8, b, 1)]);
                instrPtr += 3;
                break;

            case R_LDM_64: dreg(DECODE_8(u8_u8, a, 0)) = *(int64_t*)(heap + reg[DECODE_8(u8_u8, b, 1)]);
                instrPtr += 3;
                break;

            case R_LDMI: reg[DECODE_8(u8_u8_u32, a, 0)] = 
                *(int32_t*)(heap + reg[DECODE_8(u8_u8_u32, b, 1)] + DECODE_u32(u8_u8_u32, C, 2));
                instrPtr += 7;
                break;

            case R_LDMI_64: dreg(DECODE_8(u8_u8_u32, a, 0)) = 
                *(int64_t*)(heap + reg[DECODE_8(u8_u8_u32, b, 1)] + DECODE_u32(u8_u8_u32, C, 2));
                instrPtr += 7;
                break;

            case R_LDL: reg[DECODE_8(u8_u8, a, 0)] = *(int32_t*)((uint8_t*)stackFrameLocals + DECODE_8(u8_u8, b, 1));
                instrPtr += 3;
                break;

            case R_LDL_64: dreg(DECODE_8(u8_u8, a, 0)) = *(int64_t*)((uint8_t*)stackFrameLocals + DECODE_8(u8_u8, b, 1));
                instrPtr += 3;
                break;

            case R_LDA: reg[DECODE_8(u8_u8, a, 0)] = *(int32_t*)((uint8_t*)stackFrame - DECODE_8(u8_u8, b, 1));
                instrPtr += 3;
                break;

            case R_LDA_64: dreg(DECODE_8(u8_u8, a, 0)) = *(int64_t*)((uint8_t*)stackFrame - DECODE_8(u8_u8, b, 1));
                instrPtr += 3;
                break;

            case R_STL: *(int32_t*)((uint8_t*)stackFrameLocals + DECODE_8(u8_u8, a, 0)) = reg[DECODE_8(u8_u8, b, 1)];
                instrPtr += 3;
                break;

            case R_STL_64: *(int64_t*)((uint8_t*)stackFrameLocals + DECODE_8(u8_u8, a, 0)) = dreg(DECODE_8(u8_u8, b, 1));
                instrPtr += 3;
                break;

            case R_STA: *(int32_t*)((uint8_t*)stackFrame - DECODE_8(u8_u8, a, 0)) = reg[DECODE_8(u8_u8, b, 1)];
                instrPtr += 3;
                break;

            case R_STA_64: *(int64_t*)((uint8_t*)stackFrame - DECODE_8(u8_u8, a, 0)) = dreg(DECODE_8(u8_u8, b, 1));
                instrPtr += 3;
                break;

            case R_MOVS: *++sp = reg[DECODE_8(u8, C, 0)];
                instrPtr += 2;
                break;

            case R_MOVS_64: *(int64_t*)++sp = dreg(DECODE_8(u8, C, 0));
                ++sp;
                instrPtr += 2;
                break;

            case R_POP: reg[DECODE_8(u8, C, 0)] = *(int32_t*)sp--;
                instrPtr += 2;
                break;

            case R_POP_64: dreg(DECODE_8(u8, C, 0)) = *(int64_t*)--sp;
                --sp;
                instrPtr += 2;
                break;

            case R_PUSH: *++sp = reg[DECODE_32(i32, C, 0)];
                instrPtr += 5;
                break;

            case R_PUSH_64: *(int64_t*)++sp = dreg(DECODE_64(i64, C, 0));
                ++sp;
                instrPtr += 9;
                break;

            /**** Arithmetics ****/

#define REG_OP(type, op) \
    *(type)(reg+DECODE_8(u8_u8_u8, a, 0)) = \
    *(type)(reg+DECODE_8(u8_u8_u8, b, 1)) MACRO_LITERAL(op) \
    *(type)(reg+DECODE_8(u8_u8_u8, c, 2))

#define REG_OPI_32(type, layout, op) \
    *(type)(reg+DECODE_8(layout, a, 0)) = \
    *(type)(reg+DECODE_8(layout, b, 1)) MACRO_LITERAL(op) \
    DECODE_32(layout, C, 2)

#define REG_OPI_64(type, layout, op) \
    *(type)(reg+DECODE_8(layout, a, 0)) = \
    *(type)(reg+DECODE_8(layout, b, 1)) MACRO_LITERAL(op) \
    DECODE_64(layout, C, 2)

            case R_ADD: REG_OP(int32_t *, +);
                instrPtr += 4;
                break;

            case R_ADD_64: REG_OP(int64_t *, +);
                instrPtr += 4;
                break;

            case R_ADD_F: REG_OP(float *, +);
                instrPtr += 4;
                break;

            case R_ADD_F64: REG_OP(double *, +);
                instrPtr += 4;
                break;

            case R_ADDI: REG_OPI_32(int32_t *, u8_u8_i32, +);
                instrPtr += 4;
                break;

            case R_ADDI_64: REG_OPI_64(int64_t *, u8_u8_i64, +);
                instrPtr += 4;
                break;

            case R_ADDI_F: REG_OPI_32(float *, u8_u8_f32, +);
                instrPtr += 4;
                break;

            case R_ADDI_F64: REG_OPI_64(double *, u8_u8_f64, +);
                instrPtr += 4;
                break;

            case R_SUB: REG_OP(int32_t *, -);
                instrPtr += 4;
                break;

            case R_SUB_64: REG_OP(int64_t *, -);
                instrPtr += 4;
                break;

            case R_SUB_F: REG_OP(float *, -);
                instrPtr += 4;
                break;

            case R_SUB_F64: REG_OP(double *, -);
                instrPtr += 4;
                break;

            case R_SUBI: REG_OPI_32(int32_t *, u8_u8_i32, -);
                instrPtr += 4;
                break;

            case R_SUBI_64: REG_OPI_64(int64_t *, u8_u8_i64, -);
                instrPtr += 4;
                break;

            case R_SUBI_F: REG_OPI_32(float *, u8_u8_f32, -);
                instrPtr += 4;
                break;

            case R_SUBI_F64: REG_OPI_64(double *, u8_u8_f64, -);
                instrPtr += 4;
                break;

            case R_MUL: REG_OP(int32_t *, *);
                instrPtr += 4;
                break;

            case R_MUL_64: REG_OP(int64_t *, *);
                instrPtr += 4;
                break;

            case R_MUL_F: REG_OP(float *, *);
                instrPtr += 4;
                break;

            case R_MUL_F64: REG_OP(double *, *);
                instrPtr += 4;
                break;

            case R_MULI: REG_OPI_32(int32_t *, u8_u8_i32, *);
                instrPtr += 4;
                break;

            case R_MULI_64: REG_OPI_64(int64_t *, u8_u8_i64, *);
                instrPtr += 4;
                break;

            case R_MULI_F: REG_OPI_32(float *, u8_u8_f32, *);
                instrPtr += 4;
                break;

            case R_MULI_F64: REG_OPI_64(double *, u8_u8_f64, *);
                instrPtr += 4;
                break;

            case R_DIV: REG_OP(int32_t *, /);
                instrPtr += 4;
                break;

            case R_DIV_64: REG_OP(int64_t *, /);
                instrPtr += 4;
                break;

            case R_DIV_F: REG_OP(float *, /);
                instrPtr += 4;
                break;

            case R_DIV_F64: REG_OP(double *, /);
                instrPtr += 4;
                break;

            case R_DIVI: REG_OPI_32(int32_t *, u8_u8_i32, /);
                instrPtr += 4;
                break;

            case R_DIVI_64: REG_OPI_64(int64_t *, u8_u8_i64, /);
                instrPtr += 4;
                break;

            case R_DIVI_F: REG_OPI_32(float *, u8_u8_f32, /);
                instrPtr += 4;
                break;

            case R_DIVI_F64: REG_OPI_64(double *, u8_u8_f64, /);
                instrPtr += 4;
                break;

            /**** Bit Stuff ****/

            case R_INV: tmpByte = DECODE_8(u8, C, 0); 
                reg[tmpByte] = ~reg[tmpByte];
                instrPtr += 2;
                break;

            case R_INV_64: tmpByte = DECODE_8(u8, C, 0); 
                dreg(tmpByte) = ~dreg(tmpByte);
                instrPtr += 2;
                break;

            case R_NEG: tmp1 = reg + DECODE_8(u8, C, 0); 
                *(int32_t*)tmp1 = -*(int32_t*)tmp1;
                instrPtr += 2;
                break;

            case R_NEG_64: tmp1 = reg + DECODE_8(u8, C, 0); 
                *(int64_t*)tmp1 = -(*(int64_t*)tmp1);
                instrPtr += 2;
                break;

            case R_NEG_F: tmp1 = reg + DECODE_8(u8, C, 0); 
                *(float*)tmp1 = -(*(float*)tmp1);
                instrPtr += 2;
                break;

            case R_NEG_F64: tmp1 = reg + DECODE_8(u8, C, 0); 
                *(double*)tmp1 = -(*(double*)tmp1);
                instrPtr += 2;
                break;

            case R_BOR: REG_OP(int32_t *, |);
                instrPtr += 4;
                break;
            
            case R_BOR_64: REG_OP(int64_t *, |);
                instrPtr += 4;
                break;
            
            case R_BORI: REG_OPI_32(int32_t *, u8_u8_i32, |);
                instrPtr += 4;
                break;

            case R_BORI_64: REG_OPI_64(int64_t *, u8_u8_i64, |);
                instrPtr += 4;
                break;

            case R_BXOR: REG_OP(int32_t *, ^);
                instrPtr += 4;
                break;
            
            case R_BXOR_64: REG_OP(int64_t *, ^);
                instrPtr += 4;
                break;
            
            case R_BXORI: REG_OPI_32(int32_t *, u8_u8_i32, ^);
                instrPtr += 4;
                break;

            case R_BXORI_64: REG_OPI_64(int64_t *, u8_u8_i64, ^);
                instrPtr += 4;
                break;

            case R_BAND: REG_OP(int32_t *, &);
                instrPtr += 4;
                break;
            
            case R_BAND_64: REG_OP(int64_t *, &);
                instrPtr += 4;
                break;
            
            case R_BANDI: REG_OPI_32(int32_t *, u8_u8_i32, &);
                instrPtr += 4;
                break;

            case R_BANDI_64: REG_OPI_64(int64_t *, u8_u8_i64, &);
                instrPtr += 4;
                break;

            /**** Comparisons ****/

#define REG_CPR_OP(type, op) \
    *cpr = \
    *(type)(reg+DECODE_8(u8_u8, a, 0)) MACRO_LITERAL(op) \
    *(type)(reg+DECODE_8(u8_u8, b, 1))

#define REG_CPR_OPI(type, op) \
    *cpr = \
    *(type)(reg+DECODE_8(u8_i32, a, 0)) MACRO_LITERAL(op) \
    *(type)(reg+DECODE_32(u8_i32, C, 1))

            case R_OR: REG_CPR_OP(int32_t *, ||);
                instrPtr += 3;
                break;

            case R_ORI: REG_CPR_OPI(int32_t *, ||);
                instrPtr += 6;
                break;

            case R_AND: REG_CPR_OP(int32_t *, &&);
                instrPtr += 3;
                break;

            case R_ANDI: REG_CPR_OPI(int32_t *, &&);
                instrPtr += 6;
                break;

            case R_CPZ: *cpr = !(reg[DECODE_8(u8, C, 0)]);
                instrPtr += 2;
                break;

            case R_CPZ_64: *cpr = !(dreg(DECODE_8(u8, C, 0)));
                instrPtr += 2;
                break;

            case R_CPI: *cpr = reg[DECODE_8(u8_i32, a, 0)] == DECODE_32(u8_i32, C, 1);
                instrPtr += 6;
                break;
                
            case R_CPI_64: *cpr = dreg(DECODE_8(u8_i64, a, 0)) == DECODE_64(u8_i64, C, 1);
                instrPtr += 6;
                break;

            case R_CPEQ: REG_CPR_OP(int32_t *, ==);
                instrPtr += 3;
                break;

            case R_CPEQ_64: REG_CPR_OP(int64_t *, ==);
                instrPtr += 3;
                break;

            case R_CPEQ_F: REG_CPR_OP(float *, ==);
                instrPtr += 3;
                break;

            case R_CPEQ_F64: REG_CPR_OP(double *, ==);
                instrPtr += 3;
                break;

            case R_CPNQ: REG_CPR_OP(int32_t *, !=);
                instrPtr += 3;
                break;

            case R_CPNQ_64: REG_CPR_OP(int64_t *, !=);
                instrPtr += 3;
                break;

            case R_CPNQ_F: REG_CPR_OP(float *, !=);
                instrPtr += 3;
                break;

            case R_CPNQ_F64: REG_CPR_OP(double *, !=);
                instrPtr += 3;
                break;

            case R_CPGT: REG_CPR_OP(int32_t *, >);
                instrPtr += 3;
                break;

            case R_CPGT_64: REG_CPR_OP(int64_t *, >);
                instrPtr += 3;
                break;

            case R_CPGT_F: REG_CPR_OP(float *, >);
                instrPtr += 3;
                break;

            case R_CPGT_F64: REG_CPR_OP(double *, >);
                instrPtr += 3;
                break;

            case R_CPLT: REG_CPR_OP(int32_t *, <);
                instrPtr += 3;
                break;

            case R_CPLT_64: REG_CPR_OP(int64_t *, <);
                instrPtr += 3;
                break;

            case R_CPLT_F: REG_CPR_OP(float *, <);
                instrPtr += 3;
                break;

            case R_CPLT_F64: REG_CPR_OP(double *, <);
                instrPtr += 3;
                break;

            case R_CPGQ: REG_CPR_OP(int32_t *, >=);
                instrPtr += 3;
                break;

            case R_CPGQ_64: REG_CPR_OP(int64_t *, >=);
                instrPtr += 3;
                break;

            case R_CPGQ_F: REG_CPR_OP(float *, >=);
                instrPtr += 3;
                break;

            case R_CPGQ_F64: REG_CPR_OP(double *, >=);
                instrPtr += 3;
                break;

            case R_CPLQ: REG_CPR_OP(int32_t *, <=);
                instrPtr += 3;
                break;

            case R_CPLQ_64: REG_CPR_OP(int64_t *, <=);
                instrPtr += 3;
                break;

            case R_CPLQ_F: REG_CPR_OP(float *, <=);
                instrPtr += 3;
                break;

            case R_CPLQ_F64: REG_CPR_OP(double *, <=);
                instrPtr += 3;
                break;

            case R_CPSTR: *cpr = strcmp( 
                    heap + reg[DECODE_8(u8_u8, a, 0)],
                    heap + reg[DECODE_8(u8_u8, b, 1)] 
                    ) == 0;
                instrPtr += 3;
                break;

            case R_CPCHR: *cpr = *(heap + reg[DECODE_8(u8_u8, a, 0)]) ==
                                 *(heap + reg[DECODE_8(u8_u8, b, 1)]);
                instrPtr += 3;
                break;
            
            case R_BRZ: instrPtr = !*cpr ? program + DECODE_ADDR() : instrPtr + 5;
                break;

            case R_BRNZ: instrPtr = *cpr ? program + DECODE_ADDR() : instrPtr + 5;
                break;

            case R_BRIZ: instrPtr = !*cpr ? program + reg[DECODE_8(u8, C, 0)] : instrPtr + 5;
                break;

            case R_BRINZ: instrPtr = *cpr ? program + reg[DECODE_8(u8, C, 0)] : instrPtr + 5;
                break;

            case R_JMPI: instrPtr = program + reg[DECODE_8(u8, C, 0)];
                break;

            /**** Conversions ****/

#define REG_CONVERT(from, to) *(to*)(reg + DECODE_8(u8_u8, a, 0)) = (to)(*(from*)(reg + DECODE_8(u8_u8, b, 1)))

            case R_ITOL: REG_CONVERT(int32_t, int64_t);
                instrPtr += 3;
                break;

            case R_ITOF: REG_CONVERT(int32_t, float);
                instrPtr += 3;
                break;

            case R_ITOD: REG_CONVERT(int32_t, double);
                instrPtr += 3;
                break;

            case R_ITOS: snprintf(strBuf, 32, "%d", reg[DECODE_8(u8_u8, b, 1)]); 
                reg[DECODE_8(u8_u8, a, 0)] = VMHeapAllocString(strBuf);
                instrPtr += 3;
                break;

            case R_LTOI: REG_CONVERT(int64_t, int32_t);
                instrPtr += 3;
                break;

            case R_LTOF: REG_CONVERT(int64_t, float);
                instrPtr += 3;
                break;

            case R_LTOD: REG_CONVERT(int64_t, double);
                instrPtr += 3;
                break;

            case R_LTOS: snprintf(strBuf, 32, "%lld", dreg(DECODE_8(u8_u8, b, 1))); 
                dreg(DECODE_8(u8_u8, a, 0)) = VMHeapAllocString(strBuf);
                instrPtr += 3;
                break;

            case R_FTOI: REG_CONVERT(float, int32_t);
                instrPtr += 3;
                break;

            case R_FTOL: REG_CONVERT(float, int64_t);
                instrPtr += 3;
                break;

            case R_FTOD: REG_CONVERT(float, double);
                instrPtr += 3;
                break;

            case R_FTOS: tmpInt = DECODE_8(u8_u8_u8, c, 2);
                snprintf(strBuf, 32, "%.*f", (tmpInt == 0xFF ? 3 : tmpInt), *(float*)(reg + DECODE_8(u8_u8_u8, b, 1))); 
                reg[DECODE_8(u8_u8_u8, a, 0)] = VMHeapAllocString(strBuf);
                instrPtr += 4;
                break;

            case R_DTOI: REG_CONVERT(double, int32_t);
                instrPtr += 3;
                break;

            case R_DTOF: REG_CONVERT(double, float);
                instrPtr += 3;
                break;

            case R_DTOL: REG_CONVERT(double, int64_t);
                instrPtr += 3;
                break;

            case R_DTOS: tmpInt = DECODE_8(u8_u8_u8, c, 2);
                snprintf(strBuf, 32, "%.*f", (tmpInt == 0xFF ? 3 : tmpInt), *(double*)(reg + DECODE_8(u8_u8_u8, b, 1))); 
                reg[DECODE_8(u8_u8_u8, a, 0)] = VMHeapAllocString(strBuf);
                instrPtr += 4;
                break;

            case R_STOI: tmp1 = heap + reg[DECODE_8(u8_u8_i32, b, 1)];
                tmp3 = reg + DECODE_8(u8_u8_i32, a, 0);
                *(int32_t*)tmp3 = strtol(tmp1, (char**)&tmp2, 10);
                *(int32_t*)tmp3 = tmp2 == tmp1 ? DECODE_32(u8_u8_i32, C, 2) : *(int32_t*)tmp3;
                instrPtr += 7;
                break;

            case R_STOL: tmp1 = heap + reg[DECODE_8(u8_u8_i64, b, 1)];
                tmp3 = reg + DECODE_8(u8_u8_i64, a, 0);
                *(int64_t*)tmp3 = strtoll(tmp1, (char**)&tmp2, 10);
                *(int64_t*)tmp3 = tmp2 == tmp1 ? DECODE_64(u8_u8_i64, C, 2) : *(int64_t*)tmp3;
                instrPtr += 11;
                break;

            case R_STOF: reinterpret.intVal = DECODE_32(u8_u8_i32, C, 2);
                tmp1 = heap + reg[DECODE_8(u8_u8_f32, b, 1)];
                tmp3 = reg + DECODE_8(u8_u8_f32, a, 0);
                *(float*)tmp3 = strtof(tmp1, (char**)&tmp2);
                *(float*)tmp3 = tmp2 == tmp1 ? reinterpret.fltVal : *(float*)tmp3;
                instrPtr += 7;
                break;

            case R_STOD: reinterpret.longVal = DECODE_64(u8_u8_i64, C, 2);
                tmp1 = heap + reg[DECODE_8(u8_u8_f64, b, 1)];
                tmp3 = reg + DECODE_8(u8_u8_f64, a, 0);
                *(double*)tmp3 = strtod(tmp1, (char**)&tmp2);
                *(double*)tmp3 = tmp2 == tmp1 ? reinterpret.dblVal : *(double*)tmp3;
                instrPtr += 11;
                break;

            /**** Miscellaneous ****/

            case R_NEW: reg[DECODE_8(u8_u8, a, 0)] = VMHeapAlloc(reg[DECODE_8(u8_u8, b, 1)]);
                instrPtr += 3;
                break;

            case R_NEWI: reg[DECODE_8(u8_i32, a, 0)] = VMHeapAlloc(DECODE_32(u8_i32, C, 1));
                instrPtr += 6;
                break;

            case R_DEL: VMHeapFree(reg[DECODE_8(u8, C, 0)]);
                instrPtr += 2;
                break;

            case R_RESZ: reg[DECODE_8(u8_u8, a, 0)] = VMHeapRealloc(reg[DECODE_8(u8_u8, a, 0)], reg[DECODE_8(u8_u8, b, 1)]);
                instrPtr += 3;
                break;

            case R_RESZI: reg[DECODE_8(u8_i32, a, 0)] = VMHeapRealloc(reg[DECODE_8(u8_i32, a, 0)], DECODE_32(u8_i32, C, 1));
                instrPtr += 6;
                break;

            case R_SIZE: reg[DECODE_8(u8_u8, a, 0)] = VMGetHeapAllocSize(reg[DECODE_8(u8_u8, b, 1)]);
                instrPtr += 3;
                break;

            case R_STR: reg[DECODE_8(u8_u32, a, 0)] = VMHeapAllocString((const char *)(program + DECODE_u32(u8_u32, C, 1)));
                instrPtr += 6;
                break;

            case R_STRCPY: reg[DECODE_8(u8_u8_u32, a, 0)] = VMHeapAllocSubStr(
                    (const char *)(heap + reg[DECODE_8(u8_u8_u32, b, 1)]), 
                    DECODE_u32(u8_u8_u32, C, 2));
                instrPtr += 7;
                break;

            case R_STRCAT: reg[DECODE_8(u8_u8_u32, a, 0)] = VMHeapAllocCombinedString(
                    (const char *)(heap + reg[DECODE_8(u8_u8_u32, b, 1)]), 
                    (const char *)(program + DECODE_u32(u8_u8_u32, C, 2)));
                instrPtr += 7;
                break;

            case R_STRCMB: reg[DECODE_8(u8_u8_u8, a, 0)] = VMHeapAllocCombinedString(
                    (const char *)(heap + reg[DECODE_8(u8_u8_u8, b, 1)]), 
                    (const char *)(heap + reg[DECODE_8(u8_u8_u8, c, 2)]));
                instrPtr += 4;
                break;

            default: 
                return VM_EXIT_FAILURE;
        }

        if (sp >= stackEnd)
            return VM_EXIT_STACK_OVERFLOW;
    }

    return VM_EXIT_SUCCESS;
}
