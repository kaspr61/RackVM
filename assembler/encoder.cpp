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

#include "encoder.hpp"

namespace Assembly
{
    //---- General purpose instruction macros ----------------------------------------------------//

    #define DECL_INSTR0(op, mn) inline BinaryInstruction _##mn(const uint64_t, const uint64_t, const uint64_t)\
    { return BinaryInstruction(op); }

    #define DECL_INSTR1(op, mn, a) inline BinaryInstruction _##mn(const uint64_t Ra, const uint64_t, const uint64_t)\
    { return BinaryInstruction(op, (a)Ra); }

    #define DECL_INSTR2(op, mn, a, b) inline BinaryInstruction _##mn(const uint64_t Ra, const uint64_t Rb, const uint64_t)\
    { return BinaryInstruction(op, (a)Ra, (b)Rb); }

    #define DECL_INSTR3(op, mn, a, b, c) inline BinaryInstruction _##mn(const uint64_t Ra, const uint64_t Rb, const uint64_t Rc)\
    { return BinaryInstruction(op, (a)Ra, (b)Rb, (c)Rc); }

    #define LOAD_INSTR0(mn, fn, byteSize) {m_translate[mn] = _##fn; m_info[mn] = InstructionData(byteSize);}
    #define LOAD_INSTR(mn, fn, byteSize, ...) {m_translate[mn] = _##fn; m_info[mn] = InstructionData(byteSize, __VA_ARGS__);}
 
    //---- Register instruction macros -----------------------------------------------------------//

    #define DECL_REG_INSTR0(op, mn) inline BinaryInstruction R_##mn(const uint64_t, const uint64_t, const uint64_t)\
    { return BinaryInstruction(op); }

    #define DECL_REG_INSTR1(op, mn, a) inline BinaryInstruction R_##mn(const uint64_t Ra, const uint64_t, const uint64_t)\
    { return BinaryInstruction(op, (a)Ra); }

    #define DECL_REG_INSTR2(op, mn, a, b) inline BinaryInstruction R_##mn(const uint64_t Ra, const uint64_t Rb, const uint64_t)\
    { return BinaryInstruction(op, (a)Ra, (b)Rb); }

    #define DECL_REG_INSTR3(op, mn, a, b, c) inline BinaryInstruction R_##mn(const uint64_t Ra, const uint64_t Rb, const uint64_t Rc)\
    { return BinaryInstruction(op, (a)Ra, (b)Rb, (c)Rc); }

    #define LOAD_REG_INSTR0(mn, fn, byteSize) {m_translate[mn] = R_##fn; m_info[mn] = InstructionData(byteSize);}
    #define LOAD_REG_INSTR(mn, fn, byteSize, ...) {m_translate[mn] = R_##fn; m_info[mn] = InstructionData(byteSize, __VA_ARGS__);}

    //---- Stack instruction macros --------------------------------------------------------------//

    #define DECL_STACK_INSTR0(op, mn) inline BinaryInstruction S_##mn(const uint64_t, const uint64_t, const uint64_t)\
    { return BinaryInstruction(op); }

    #define DECL_STACK_INSTR1(op, mn, a) inline BinaryInstruction S_##mn(const uint64_t C, const uint64_t, const uint64_t)\
    { return BinaryInstruction(op, (a)C); }

    #define LOAD_STACK_INSTR0(mn, fn, byteSize) {m_translate[mn] = S_##fn; m_info[mn] = InstructionData(byteSize);}
    #define LOAD_STACK_INSTR(mn, fn, byteSize, ...) {m_translate[mn] = S_##fn; m_info[mn] = InstructionData(byteSize, __VA_ARGS__);}

    //--------------------------------------------------------------------------------------------//

    //---- General-purpose instructions ----//

    DECL_INSTR0(0x00, NOP)
    DECL_INSTR0(0x01, EXIT)
    DECL_INSTR1(0x02, JMP,        uint32_t)
    DECL_INSTR1(0x03, CALL,       uint32_t)
    DECL_INSTR1(0x04, RET,        uint8_t )
    DECL_INSTR1(0x05, RET_32,     uint8_t )
    DECL_INSTR1(0x06, RET_64,     uint8_t )
    DECL_INSTR1(0x07, SCALL,      uint8_t )
    DECL_INSTR1(0x08, SARG,       uint8_t )

    //---- Register-only instructions ----//

    DECL_REG_INSTR2(0x09, MOV,      Register,   Register            )
    DECL_REG_INSTR2(0x0A, MOV_64,   Register,   Register            )
    DECL_REG_INSTR2(0x0B, LDI,      Register,   uint32_t            )
    DECL_REG_INSTR2(0x0C, LDI_64,   Register,   uint64_t            )
    DECL_REG_INSTR2(0x0D, STM,      Register,   Register            )
    DECL_REG_INSTR2(0x0E, STM_64,   Register,   Register            )
    DECL_REG_INSTR3(0x0F, STMI,     Register,   Register,   uint32_t)
    DECL_REG_INSTR3(0x10, STMI_64,  Register,   Register,   uint32_t)
    DECL_REG_INSTR2(0x11, LDM,      Register,   Register            )
    DECL_REG_INSTR2(0x12, LDM_64,   Register,   Register            )
    DECL_REG_INSTR3(0x13, LDMI,     Register,   Register,   uint32_t)
    DECL_REG_INSTR3(0x14, LDMI_64,  Register,   Register,   uint32_t)
    DECL_REG_INSTR2(0x15, LDL,      Register,   uint8_t             )
    DECL_REG_INSTR2(0x16, LDL_64,   Register,   uint8_t             )
    DECL_REG_INSTR2(0x17, LDA,      Register,   uint8_t             )
    DECL_REG_INSTR2(0x18, LDA_64,   Register,   uint8_t             )
    DECL_REG_INSTR2(0x19, STL,      uint8_t,    Register            )
    DECL_REG_INSTR2(0x1A, STL_64,   uint8_t,    Register            )
    DECL_REG_INSTR2(0x1B, STA,      uint8_t,    Register            )
    DECL_REG_INSTR2(0x1C, STA_64,   uint8_t,    Register            )
    DECL_REG_INSTR1(0x1D, MOVS,     Register                        )
    DECL_REG_INSTR1(0x1E, MOVS_64,  Register                        )
    DECL_REG_INSTR1(0x1F, POP,      Register                        )
    DECL_REG_INSTR1(0x20, POP_64,   Register                        )
    DECL_REG_INSTR1(0x21, PUSH,     uint32_t                        )
    DECL_REG_INSTR1(0x22, PUSH_64,  uint64_t                        )


    DECL_REG_INSTR3(0x23, ADD,      Register,   Register,   Register)
    DECL_REG_INSTR3(0x24, ADD_64,   Register,   Register,   Register)
    DECL_REG_INSTR3(0x25, ADD_F,    Register,   Register,   Register)
    DECL_REG_INSTR3(0x26, ADD_F64,  Register,   Register,   Register)
    DECL_REG_INSTR3(0x27, ADDI,     Register,   Register,   uint32_t)
    DECL_REG_INSTR3(0x28, ADDI_64,  Register,   Register,   uint64_t)
    DECL_REG_INSTR3(0x29, ADDI_F,   Register,   Register,   uint32_t)
    DECL_REG_INSTR3(0x2A, ADDI_F64, Register,   Register,   uint64_t)
    DECL_REG_INSTR3(0x2B, SUB,      Register,   Register,   Register)
    DECL_REG_INSTR3(0x2C, SUB_64,   Register,   Register,   Register)
    DECL_REG_INSTR3(0x2D, SUB_F,    Register,   Register,   Register)
    DECL_REG_INSTR3(0x2E, SUB_F64,  Register,   Register,   Register)
    DECL_REG_INSTR3(0x2F, SUBI,     Register,   Register,   uint32_t)
    DECL_REG_INSTR3(0x30, SUBI_64,  Register,   Register,   uint64_t)
    DECL_REG_INSTR3(0x31, SUBI_F,   Register,   Register,   uint32_t)
    DECL_REG_INSTR3(0x32, SUBI_F64, Register,   Register,   uint64_t)
    DECL_REG_INSTR3(0x33, MUL,      Register,   Register,   Register)
    DECL_REG_INSTR3(0x34, MUL_64,   Register,   Register,   Register)
    DECL_REG_INSTR3(0x35, MUL_F,    Register,   Register,   Register)
    DECL_REG_INSTR3(0x36, MUL_F64,  Register,   Register,   Register)
    DECL_REG_INSTR3(0x37, MULI,     Register,   Register,   uint32_t)
    DECL_REG_INSTR3(0x38, MULI_64,  Register,   Register,   uint64_t)
    DECL_REG_INSTR3(0x39, MULI_F,   Register,   Register,   uint32_t)
    DECL_REG_INSTR3(0x3A, MULI_F64, Register,   Register,   uint64_t)
    DECL_REG_INSTR3(0x3B, DIV,      Register,   Register,   Register)
    DECL_REG_INSTR3(0x3C, DIV_64,   Register,   Register,   Register)
    DECL_REG_INSTR3(0x3D, DIV_F,    Register,   Register,   Register)
    DECL_REG_INSTR3(0x3E, DIV_F64,  Register,   Register,   Register)
    DECL_REG_INSTR3(0x3F, DIVI,     Register,   Register,   uint32_t)
    DECL_REG_INSTR3(0x40, DIVI_64,  Register,   Register,   uint64_t)
    DECL_REG_INSTR3(0x41, DIVI_F,   Register,   Register,   uint32_t)
    DECL_REG_INSTR3(0x42, DIVI_F64, Register,   Register,   uint64_t)

    DECL_REG_INSTR1(0x43, INV,      Register                        )
    DECL_REG_INSTR1(0x44, INV_64,   Register                        )
    DECL_REG_INSTR1(0x45, NEG,      Register                        )
    DECL_REG_INSTR1(0x46, NEG_64,   Register                        )
    DECL_REG_INSTR1(0x47, NEG_F,    Register                        )
    DECL_REG_INSTR1(0x48, NEG_F64,  Register                        )
    DECL_REG_INSTR3(0x49, BOR,      Register,   Register,   Register)
    DECL_REG_INSTR3(0x4A, BOR_64,   Register,   Register,   Register)
    DECL_REG_INSTR3(0x4B, BORI,     Register,   Register,   uint32_t)
    DECL_REG_INSTR3(0x4C, BORI_64,  Register,   Register,   uint64_t)
    DECL_REG_INSTR3(0x4D, BXOR,     Register,   Register,   Register)
    DECL_REG_INSTR3(0x4E, BXOR_64,  Register,   Register,   Register)
    DECL_REG_INSTR3(0x4F, BXORI,    Register,   Register,   uint32_t)
    DECL_REG_INSTR3(0x50, BXORI_64, Register,   Register,   uint64_t)
    DECL_REG_INSTR3(0x51, BAND,     Register,   Register,   Register)
    DECL_REG_INSTR3(0x52, BAND_64,  Register,   Register,   Register)
    DECL_REG_INSTR3(0x53, BANDI,    Register,   Register,   uint32_t)
    DECL_REG_INSTR3(0x54, BANDI_64, Register,   Register,   uint64_t)

    DECL_REG_INSTR2(0x55, OR,       Register,   Register            )
    DECL_REG_INSTR2(0x56, ORI,      Register,   uint32_t            )
    DECL_REG_INSTR2(0x57, AND,      Register,   Register            )
    DECL_REG_INSTR2(0x58, ANDI,     Register,   uint32_t            )
    DECL_REG_INSTR1(0x59, CPZ,      Register                        )
    DECL_REG_INSTR1(0x5A, CPZ_64,   Register                        )
    DECL_REG_INSTR2(0x5B, CPI,      Register,   uint32_t            )
    DECL_REG_INSTR2(0x5C, CPI_64,   Register,   uint64_t            )
    DECL_REG_INSTR2(0x5D, CPEQ,     Register,   Register            )
    DECL_REG_INSTR2(0x5E, CPEQ_64,  Register,   Register            )
    DECL_REG_INSTR2(0x5F, CPEQ_F,   Register,   Register            )
    DECL_REG_INSTR2(0x60, CPEQ_F64, Register,   Register            )
    DECL_REG_INSTR2(0x61, CPNQ,     Register,   Register            )
    DECL_REG_INSTR2(0x62, CPNQ_64,  Register,   Register            )
    DECL_REG_INSTR2(0x63, CPNQ_F,   Register,   Register            )
    DECL_REG_INSTR2(0x64, CPNQ_F64, Register,   Register            )
    DECL_REG_INSTR2(0x65, CPGT,     Register,   Register            )
    DECL_REG_INSTR2(0x66, CPGT_64,  Register,   Register            )
    DECL_REG_INSTR2(0x67, CPGT_F,   Register,   Register            )
    DECL_REG_INSTR2(0x68, CPGT_F64, Register,   Register            )
    DECL_REG_INSTR2(0x69, CPLT,     Register,   Register            )
    DECL_REG_INSTR2(0x6A, CPLT_64,  Register,   Register            )
    DECL_REG_INSTR2(0x6B, CPLT_F,   Register,   Register            )
    DECL_REG_INSTR2(0x6C, CPLT_F64, Register,   Register            )
    DECL_REG_INSTR2(0x6D, CPGQ,     Register,   Register            )
    DECL_REG_INSTR2(0x6E, CPGQ_64,  Register,   Register            )
    DECL_REG_INSTR2(0x6F, CPGQ_F,   Register,   Register            )
    DECL_REG_INSTR2(0x70, CPGQ_F64, Register,   Register            )
    DECL_REG_INSTR2(0x71, CPLQ,     Register,   Register            )
    DECL_REG_INSTR2(0x72, CPLQ_64,  Register,   Register            )
    DECL_REG_INSTR2(0x73, CPLQ_F,   Register,   Register            )
    DECL_REG_INSTR2(0x74, CPLQ_F64, Register,   Register            )
    DECL_REG_INSTR2(0x75, CPSTR,    Register,   Register            )
    DECL_REG_INSTR2(0x76, CPCHR,    Register,   Register            )
    DECL_REG_INSTR1(0x77, BRZ,      uint32_t                        )
    DECL_REG_INSTR1(0x78, BRNZ,     uint32_t                        )
    DECL_REG_INSTR1(0x79, BRIZ,     Register                        )
    DECL_REG_INSTR1(0x7A, BRINZ,    Register                        )
    DECL_REG_INSTR1(0x7B, JMPI,     Register                        )

    DECL_REG_INSTR2(0x7C, ITOL,     Register,   Register            )
    DECL_REG_INSTR2(0x7D, ITOF,     Register,   Register            )
    DECL_REG_INSTR2(0x7E, ITOD,     Register,   Register            )
    DECL_REG_INSTR2(0x7F, ITOS,     Register,   Register            )
    DECL_REG_INSTR2(0x80, LTOI,     Register,   Register            )
    DECL_REG_INSTR2(0x81, LTOF,     Register,   Register            )
    DECL_REG_INSTR2(0x82, LTOD,     Register,   Register            )
    DECL_REG_INSTR2(0x83, LTOS,     Register,   Register            )
    DECL_REG_INSTR2(0x84, FTOI,     Register,   Register            )
    DECL_REG_INSTR2(0x85, FTOL,     Register,   Register            )
    DECL_REG_INSTR2(0x86, FTOD,     Register,   Register            )
    DECL_REG_INSTR3(0x87, FTOS,     Register,   Register,   uint8_t )
    DECL_REG_INSTR2(0x88, DTOI,     Register,   Register            )
    DECL_REG_INSTR2(0x89, DTOL,     Register,   Register            )
    DECL_REG_INSTR2(0x8A, DTOF,     Register,   Register            )
    DECL_REG_INSTR3(0x8B, DTOS,     Register,   Register,   uint8_t )
    DECL_REG_INSTR3(0x8C, STOI,     Register,   Register,   uint32_t)
    DECL_REG_INSTR3(0x8D, STOL,     Register,   Register,   uint64_t)
    DECL_REG_INSTR3(0x8E, STOF,     Register,   Register,   uint32_t)
    DECL_REG_INSTR3(0x8F, STOD,     Register,   Register,   uint64_t)

    DECL_REG_INSTR2(0x90, NEW,      Register,   Register            )
    DECL_REG_INSTR2(0x91, NEWI,     Register,   uint32_t            )
    DECL_REG_INSTR1(0x92, DEL,      Register                        )
    DECL_REG_INSTR2(0x93, RESZ,     Register,   Register            )
    DECL_REG_INSTR2(0x94, RESZI,    Register,   uint32_t            )
    DECL_REG_INSTR2(0x95, SIZE,     Register,   Register            )
    DECL_REG_INSTR2(0x96, STR,      Register,   uint32_t            )
    DECL_REG_INSTR3(0x97, STRCPY,   Register,   Register,   uint32_t)
    DECL_REG_INSTR3(0x98, STRCAT,   Register,   Register,   uint32_t)
    DECL_REG_INSTR3(0x99, STRCMB,   Register,   Register,   Register)

    //---- Stack-only instructions ----//
    DECL_STACK_INSTR1(0x09, LDI,        uint32_t)
    DECL_STACK_INSTR1(0x0A, LDI_64,     uint64_t)
    DECL_STACK_INSTR0(0x0B, STM                 )
    DECL_STACK_INSTR0(0x0C, STM_64              )
    DECL_STACK_INSTR1(0x0D, STMI,       uint32_t)
    DECL_STACK_INSTR1(0x0E, STMI_64,    uint64_t)
    DECL_STACK_INSTR0(0x0F, LDM                 )
    DECL_STACK_INSTR0(0x10, LDM_64              )
    DECL_STACK_INSTR1(0x11, LDMI,       uint32_t)
    DECL_STACK_INSTR1(0x12, LDMI_64,    uint64_t)
    DECL_STACK_INSTR1(0x13, LDL,        uint8_t )
    DECL_STACK_INSTR1(0x14, LDL_64,     uint8_t )
    DECL_STACK_INSTR1(0x15, LDA,        uint8_t )
    DECL_STACK_INSTR1(0x16, LDA_64,     uint8_t )
    DECL_STACK_INSTR1(0x17, STL,        uint8_t )
    DECL_STACK_INSTR1(0x18, STL_64,     uint8_t )
    DECL_STACK_INSTR1(0x19, STA,        uint8_t )
    DECL_STACK_INSTR1(0x1A, STA_64,     uint8_t )

    DECL_STACK_INSTR0(0x1B, ADD                 )
    DECL_STACK_INSTR0(0x1C, ADD_64              )
    DECL_STACK_INSTR0(0x1D, ADD_F               )
    DECL_STACK_INSTR0(0x1E, ADD_F64             )
    DECL_STACK_INSTR0(0x1F, SUB                 )
    DECL_STACK_INSTR0(0x20, SUB_64              )
    DECL_STACK_INSTR0(0x21, SUB_F               )
    DECL_STACK_INSTR0(0x22, SUB_F64             )
    DECL_STACK_INSTR0(0x23, MUL                 )
    DECL_STACK_INSTR0(0x24, MUL_64              )
    DECL_STACK_INSTR0(0x25, MUL_F               )
    DECL_STACK_INSTR0(0x26, MUL_F64             )
    DECL_STACK_INSTR0(0x27, DIV                 )
    DECL_STACK_INSTR0(0x28, DIV_64              )
    DECL_STACK_INSTR0(0x29, DIV_F               )
    DECL_STACK_INSTR0(0x2A, DIV_F64             )

    DECL_STACK_INSTR0(0x2B, INV                 )
    DECL_STACK_INSTR0(0x2C, INV_64              )
    DECL_STACK_INSTR0(0x2D, NEG                 )
    DECL_STACK_INSTR0(0x2E, NEG_64              )
    DECL_STACK_INSTR0(0x2F, NEG_F               )
    DECL_STACK_INSTR0(0x30, NEG_F64             )
    DECL_STACK_INSTR0(0x31, BOR                 )
    DECL_STACK_INSTR0(0x32, BOR_64              )
    DECL_STACK_INSTR0(0x33, BXOR                )
    DECL_STACK_INSTR0(0x34, BXOR_64             )
    DECL_STACK_INSTR0(0x35, BAND                )
    DECL_STACK_INSTR0(0x36, BAND_64             )

    DECL_STACK_INSTR0(0x37, OR                  )
    DECL_STACK_INSTR0(0x38, AND                 )
    DECL_STACK_INSTR0(0x39, CPZ                 )
    DECL_STACK_INSTR0(0x3A, CPZ_64              )
    DECL_STACK_INSTR0(0x3B, CPEQ                )
    DECL_STACK_INSTR0(0x3C, CPEQ_64             )
    DECL_STACK_INSTR0(0x3D, CPEQ_F              )
    DECL_STACK_INSTR0(0x3E, CPEQ_F64            )
    DECL_STACK_INSTR0(0x3F, CPNQ                )
    DECL_STACK_INSTR0(0x40, CPNQ_64             )
    DECL_STACK_INSTR0(0x41, CPNQ_F              )
    DECL_STACK_INSTR0(0x42, CPNQ_F64            )
    DECL_STACK_INSTR0(0x43, CPGT                )
    DECL_STACK_INSTR0(0x44, CPGT_64             )
    DECL_STACK_INSTR0(0x45, CPGT_F              )
    DECL_STACK_INSTR0(0x46, CPGT_F64            )
    DECL_STACK_INSTR0(0x47, CPLT                )
    DECL_STACK_INSTR0(0x48, CPLT_64             )
    DECL_STACK_INSTR0(0x49, CPLT_F              )
    DECL_STACK_INSTR0(0x4A, CPLT_F64            )
    DECL_STACK_INSTR0(0x4B, CPGQ                )
    DECL_STACK_INSTR0(0x4C, CPGQ_64             )
    DECL_STACK_INSTR0(0x4D, CPGQ_F              )
    DECL_STACK_INSTR0(0x4E, CPGQ_F64            )
    DECL_STACK_INSTR0(0x4F, CPLQ                )
    DECL_STACK_INSTR0(0x50, CPLQ_64             )
    DECL_STACK_INSTR0(0x51, CPLQ_F              )
    DECL_STACK_INSTR0(0x52, CPLQ_F64            )
    DECL_STACK_INSTR0(0x53, CPSTR               )
    DECL_STACK_INSTR0(0x54, CPCHR               )
    DECL_STACK_INSTR1(0x55, BRZ,        uint32_t)
    DECL_STACK_INSTR1(0x56, BRNZ,       uint32_t)
    DECL_STACK_INSTR0(0x57, BRIZ                )
    DECL_STACK_INSTR0(0x58, BRINZ               )
    DECL_STACK_INSTR0(0x59, JMPI                )

    DECL_STACK_INSTR0(0x5A, ITOL                )
    DECL_STACK_INSTR0(0x5B, ITOF                )
    DECL_STACK_INSTR0(0x5C, ITOD                )
    DECL_STACK_INSTR0(0x5D, ITOS                )
    DECL_STACK_INSTR0(0x5E, LTOI                )
    DECL_STACK_INSTR0(0x5F, LTOF                )
    DECL_STACK_INSTR0(0x60, LTOD                )
    DECL_STACK_INSTR0(0x61, LTOS                )
    DECL_STACK_INSTR0(0x62, FTOI                )
    DECL_STACK_INSTR0(0x63, FTOL                )
    DECL_STACK_INSTR0(0x64, FTOD                )
    DECL_STACK_INSTR1(0x65, FTOS,       uint8_t )
    DECL_STACK_INSTR0(0x66, DTOI                )
    DECL_STACK_INSTR0(0x67, DTOL                )
    DECL_STACK_INSTR0(0x68, DTOF                )
    DECL_STACK_INSTR1(0x69, DTOS,       uint8_t )
    DECL_STACK_INSTR1(0x6A, STOI,       uint32_t)
    DECL_STACK_INSTR1(0x6B, STOL,       uint64_t)
    DECL_STACK_INSTR1(0x6C, STOF,       uint32_t)
    DECL_STACK_INSTR1(0x6D, STOD,       uint64_t)

    DECL_STACK_INSTR0(0x6E, NEW                 )
    DECL_STACK_INSTR0(0x6F, DEL                 )
    DECL_STACK_INSTR0(0x70, RESZ                )
    DECL_STACK_INSTR0(0x71, SIZE                )
    DECL_STACK_INSTR1(0x72, STR,        uint32_t)
    DECL_STACK_INSTR1(0x73, STRCPY,     uint32_t)
    DECL_STACK_INSTR1(0x74, STRCAT,     uint32_t)
    DECL_STACK_INSTR0(0x75, STRCMB              )
    //----------------------------------//

    InstructionEncoder::InstructionEncoder()
    {
    }

    void InstructionEncoder::LoadInstructionSet(const VMMode mode)
    {
        m_translate.clear();

        // Load the instructions and their byte size, 
        // followed by the maximum value for each argument.
        LOAD_INSTR0("NOP",     NOP,        1                  );
        LOAD_INSTR0("EXIT",    EXIT,       1                  );
        LOAD_INSTR ("JMP",     JMP,        5,       UINT32_MAX);
        LOAD_INSTR ("CALL",    CALL,       5,       UINT32_MAX);
        LOAD_INSTR ("RET",     RET,        2,       UINT8_MAX );
        LOAD_INSTR ("RET.32",  RET_32,     2,       UINT8_MAX );
        LOAD_INSTR ("RET.64",  RET_64,     2,       UINT8_MAX );
        LOAD_INSTR ("SCALL",   SCALL,      2,       UINT8_MAX );
        LOAD_INSTR ("SARG",    SARG,       2,       UINT8_MAX );

        if (mode == VM_MODE_REGISTER)
        {
            //              OPCODE       FUNC        BYTES    DATA RANGES ...
            LOAD_REG_INSTR ("MOV",       MOV,        3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("MOV.64",    MOV_64,     3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("LDI",       LDI,        6,       UINT8_MAX,  UINT32_MAX            );
            LOAD_REG_INSTR ("LDI.64",    LDI_64,     10,      UINT8_MAX,  UINT64_MAX            );
            LOAD_REG_INSTR ("STM",       STM,        3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("STM.64",    STM_64,     3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("STMI",      STMI,       7,       UINT8_MAX,  UINT8_MAX,  UINT32_MAX);
            LOAD_REG_INSTR ("STMI.64",   STMI_64,    7,       UINT8_MAX,  UINT8_MAX,  UINT64_MAX);
            LOAD_REG_INSTR ("LDM",       LDM,        3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("LDM.64",    LDM_64,     3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("LDMI",      LDMI,       7,       UINT8_MAX,  UINT8_MAX,  UINT32_MAX);
            LOAD_REG_INSTR ("LDMI.64",   LDMI_64,    7,       UINT8_MAX,  UINT8_MAX,  UINT64_MAX);
            LOAD_REG_INSTR ("LDL",       LDL,        3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("LDL.64",    LDL_64,     3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("LDA",       LDA,        3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("LDA.64",    LDA_64,     3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("STL",       STL,        3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("STL.64",    STL_64,     3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("STA",       STL,        3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("STA.64",    STL_64,     3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("MOVS",      MOVS,       2,       UINT8_MAX                         );
            LOAD_REG_INSTR ("MOVS.64",   MOVS_64,    2,       UINT8_MAX                         );
            LOAD_REG_INSTR ("POP",       POP,        2,       UINT8_MAX                         );
            LOAD_REG_INSTR ("POP.64",    POP_64,     2,       UINT8_MAX                         );
            LOAD_REG_INSTR ("PUSH",      PUSH,       5,       UINT32_MAX                        );
            LOAD_REG_INSTR ("PUSH.64",   PUSH_64,    9,       UINT64_MAX                        );
            
            LOAD_REG_INSTR ("ADD",       ADD,        4,       UINT8_MAX,  UINT8_MAX,  UINT8_MAX );
            LOAD_REG_INSTR ("ADD.64",    ADD_64,     4,       UINT8_MAX,  UINT8_MAX,  UINT8_MAX );
            LOAD_REG_INSTR ("ADD.F",     ADD_F,      4,       UINT8_MAX,  UINT8_MAX,  UINT8_MAX );
            LOAD_REG_INSTR ("ADD.F64",   ADD_F64,    4,       UINT8_MAX,  UINT8_MAX,  UINT8_MAX );
            LOAD_REG_INSTR ("ADDI",      ADDI,       7,       UINT8_MAX,  UINT8_MAX,  UINT32_MAX);
            LOAD_REG_INSTR ("ADDI.64",   ADDI_64,    11,      UINT8_MAX,  UINT8_MAX,  UINT64_MAX);
            LOAD_REG_INSTR ("ADDI.F",    ADDI_F,     7,       UINT8_MAX,  UINT8_MAX,  UINT32_MAX);
            LOAD_REG_INSTR ("ADDI.F64",  ADDI_F64,   11,      UINT8_MAX,  UINT8_MAX,  UINT64_MAX);
            LOAD_REG_INSTR ("SUB",       SUB,        4,       UINT8_MAX,  UINT8_MAX,  UINT8_MAX );
            LOAD_REG_INSTR ("SUB.64",    SUB_64,     4,       UINT8_MAX,  UINT8_MAX,  UINT8_MAX );
            LOAD_REG_INSTR ("SUB.F",     SUB_F,      4,       UINT8_MAX,  UINT8_MAX,  UINT8_MAX );
            LOAD_REG_INSTR ("SUB.F64",   SUB_F64,    4,       UINT8_MAX,  UINT8_MAX,  UINT8_MAX );
            LOAD_REG_INSTR ("SUBI",      SUBI,       7,       UINT8_MAX,  UINT8_MAX,  UINT32_MAX);
            LOAD_REG_INSTR ("SUBI.64",   SUBI_64,    11,      UINT8_MAX,  UINT8_MAX,  UINT64_MAX);
            LOAD_REG_INSTR ("SUBI.F",    SUBI_F,     7,       UINT8_MAX,  UINT8_MAX,  UINT32_MAX);
            LOAD_REG_INSTR ("SUBI.F64",  SUBI_F64,   11,      UINT8_MAX,  UINT8_MAX,  UINT64_MAX);
            LOAD_REG_INSTR ("MUL",       MUL,        4,       UINT8_MAX,  UINT8_MAX,  UINT8_MAX );
            LOAD_REG_INSTR ("MUL.64",    MUL_64,     4,       UINT8_MAX,  UINT8_MAX,  UINT8_MAX );
            LOAD_REG_INSTR ("MUL.F",     MUL_F,      4,       UINT8_MAX,  UINT8_MAX,  UINT8_MAX );
            LOAD_REG_INSTR ("MUL.F64",   MUL_F64,    4,       UINT8_MAX,  UINT8_MAX,  UINT8_MAX );
            LOAD_REG_INSTR ("MULI",      MULI,       7,       UINT8_MAX,  UINT8_MAX,  UINT32_MAX);
            LOAD_REG_INSTR ("MULI.64",   MULI_64,    11,      UINT8_MAX,  UINT8_MAX,  UINT64_MAX);
            LOAD_REG_INSTR ("MULI.F",    MULI_F,     7,       UINT8_MAX,  UINT8_MAX,  UINT32_MAX);
            LOAD_REG_INSTR ("MULI.F64",  MULI_F64,   11,      UINT8_MAX,  UINT8_MAX,  UINT64_MAX);
            LOAD_REG_INSTR ("DIV",       DIV,        4,       UINT8_MAX,  UINT8_MAX,  UINT8_MAX );
            LOAD_REG_INSTR ("DIV.64",    DIV_64,     4,       UINT8_MAX,  UINT8_MAX,  UINT8_MAX );
            LOAD_REG_INSTR ("DIV.F",     DIV_F,      4,       UINT8_MAX,  UINT8_MAX,  UINT8_MAX );
            LOAD_REG_INSTR ("DIV.F64",   DIV_F64,    4,       UINT8_MAX,  UINT8_MAX,  UINT8_MAX );
            LOAD_REG_INSTR ("DIVI",      DIVI,       7,       UINT8_MAX,  UINT8_MAX,  UINT32_MAX);
            LOAD_REG_INSTR ("DIVI.64",   DIVI_64,    11,      UINT8_MAX,  UINT8_MAX,  UINT64_MAX);
            LOAD_REG_INSTR ("DIVI.F",    DIVI_F,     7,       UINT8_MAX,  UINT8_MAX,  UINT32_MAX);
            LOAD_REG_INSTR ("DIVI.F64",  DIVI_F64,   11,      UINT8_MAX,  UINT8_MAX,  UINT64_MAX);

            LOAD_REG_INSTR ("INV",       INV,        2,       UINT8_MAX                         );
            LOAD_REG_INSTR ("INV.64",    INV_64,     2,       UINT8_MAX                         );
            LOAD_REG_INSTR ("NEG",       NEG,        2,       UINT8_MAX                         );
            LOAD_REG_INSTR ("NEG.64",    NEG_64,     2,       UINT8_MAX                         );
            LOAD_REG_INSTR ("NEG.F",     NEG_F,      2,       UINT8_MAX                         );
            LOAD_REG_INSTR ("NEG.F64",   NEG_F64,    2,       UINT8_MAX                         );
            LOAD_REG_INSTR ("BOR",       BOR,        4,       UINT8_MAX,  UINT8_MAX,  UINT8_MAX );
            LOAD_REG_INSTR ("BOR.64",    BOR_64,     4,       UINT8_MAX,  UINT8_MAX,  UINT8_MAX );
            LOAD_REG_INSTR ("BORI",      BORI,       7,       UINT8_MAX,  UINT8_MAX,  UINT32_MAX);
            LOAD_REG_INSTR ("BORI.64",   BORI_64,    11,      UINT8_MAX,  UINT8_MAX,  UINT64_MAX);
            LOAD_REG_INSTR ("BXOR",      BXOR,       4,       UINT8_MAX,  UINT8_MAX,  UINT8_MAX );
            LOAD_REG_INSTR ("BXOR.64",   BXOR_64,    4,       UINT8_MAX,  UINT8_MAX,  UINT8_MAX );
            LOAD_REG_INSTR ("BXORI",     BXORI,      7,       UINT8_MAX,  UINT8_MAX,  UINT32_MAX);
            LOAD_REG_INSTR ("BXORI.64",  BXORI_64,   11,      UINT8_MAX,  UINT8_MAX,  UINT64_MAX);
            LOAD_REG_INSTR ("BAND",      BAND,       4,       UINT8_MAX,  UINT8_MAX,  UINT8_MAX );
            LOAD_REG_INSTR ("BAND.64",   BAND_64,    4,       UINT8_MAX,  UINT8_MAX,  UINT8_MAX );
            LOAD_REG_INSTR ("BANDI",     BANDI,      7,       UINT8_MAX,  UINT8_MAX,  UINT32_MAX);
            LOAD_REG_INSTR ("BANDI.64",  BANDI_64,   11,      UINT8_MAX,  UINT8_MAX,  UINT64_MAX);

            LOAD_REG_INSTR ("OR",        OR,         3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("ORI",       ORI,        6,       UINT8_MAX,  UINT32_MAX            );
            LOAD_REG_INSTR ("AND",       AND,        3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("ANDI",      ANDI,       6,       UINT8_MAX,  UINT32_MAX            );
            LOAD_REG_INSTR ("CPZ",       CPZ,        2,       UINT8_MAX                         );
            LOAD_REG_INSTR ("CPZ.64",    CPZ_64,     2,       UINT8_MAX                         );
            LOAD_REG_INSTR ("CPI",       CPI,        6,       UINT8_MAX,  UINT32_MAX            );
            LOAD_REG_INSTR ("CPI.64",    CPI_64,     10,      UINT8_MAX,  UINT64_MAX            );
            LOAD_REG_INSTR ("CPEQ",      CPEQ,       3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("CPEQ.64",   CPEQ_64,    3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("CPEQ.F",    CPEQ_F,     3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("CPEQ.F64",  CPEQ_F64,   3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("CPNQ",      CPNQ,       3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("CPNQ.64",   CPNQ_64,    3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("CPNQ.F",    CPNQ_F,     3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("CPNQ.F64",  CPNQ_F64,   3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("CPGT",      CPGT,       3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("CPGT.64",   CPGT_64,    3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("CPGT.F",    CPGT_F,     3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("CPGT.F64",  CPGT_F64,   3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("CPLT",      CPLT,       3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("CPLT.64",   CPLT_64,    3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("CPLT.F",    CPLT_F,     3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("CPLT.F64",  CPLT_F64,   3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("CPGQ",      CPGQ,       3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("CPGQ.64",   CPGQ_64,    3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("CPGQ.F",    CPGQ_F,     3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("CPGQ.F64",  CPGQ_F64,   3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("CPLQ",      CPLQ,       3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("CPLQ.64",   CPLQ_64,    3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("CPLQ.F",    CPLQ_F,     3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("CPLQ.F64",  CPLQ_F64,   3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("CPSTR",     CPSTR,      3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("CPCHR",     CPCHR,      3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("BRZ",       BRZ,        5,       UINT32_MAX                        );
            LOAD_REG_INSTR ("BRNZ",      BRNZ,       5,       UINT32_MAX                        );
            LOAD_REG_INSTR ("BRIZ",      BRIZ,       2,       UINT8_MAX                         );
            LOAD_REG_INSTR ("BRINZ",     BRINZ,      2,       UINT8_MAX                         );
            LOAD_REG_INSTR ("JMPI",      JMPI,       2,       UINT8_MAX                         );

            LOAD_REG_INSTR ("ITOL",      ITOL,       3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("ITOF",      ITOF,       3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("ITOD",      ITOD,       3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("ITOS",      ITOS,       3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("LTOI",      LTOI,       3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("LTOF",      LTOF,       3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("LTOD",      LTOD,       3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("LTOS",      LTOS,       3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("FTOI",      FTOI,       3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("FTOL",      FTOL,       3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("FTOD",      FTOD,       3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("FTOS",      FTOS,       4,       UINT8_MAX,  UINT8_MAX,  UINT8_MAX );
            LOAD_REG_INSTR ("DTOI",      DTOI,       3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("DTOL",      DTOL,       3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("DTOF",      DTOF,       3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("DTOS",      DTOS,       4,       UINT8_MAX,  UINT8_MAX,  UINT8_MAX );
            LOAD_REG_INSTR ("STOI",      STOI,       7,       UINT8_MAX,  UINT8_MAX,  UINT32_MAX);
            LOAD_REG_INSTR ("STOL",      STOL,       11,      UINT8_MAX,  UINT8_MAX,  UINT64_MAX);
            LOAD_REG_INSTR ("STOF",      STOF,       7,       UINT8_MAX,  UINT8_MAX,  UINT32_MAX);
            LOAD_REG_INSTR ("STOD",      STOD,       11,      UINT8_MAX,  UINT8_MAX,  UINT64_MAX);

            LOAD_REG_INSTR ("NEW",       NEW,        3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("NEWI",      NEWI,       6,       UINT8_MAX,  UINT32_MAX            );
            LOAD_REG_INSTR ("DEL",       DEL,        2,       UINT8_MAX                         );
            LOAD_REG_INSTR ("RESZ",      RESZ,       3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("RESZI",     RESZI,      6,       UINT8_MAX,  UINT32_MAX            );
            LOAD_REG_INSTR ("SIZE",      SIZE,       3,       UINT8_MAX,  UINT8_MAX             );
            LOAD_REG_INSTR ("STR",       STR,        6,       UINT8_MAX,  UINT32_MAX            );
            LOAD_REG_INSTR ("STRCPY",    STRCPY,     7,       UINT8_MAX,  UINT8_MAX,  UINT32_MAX);
            LOAD_REG_INSTR ("STRCAT",    STRCAT,     7,       UINT8_MAX,  UINT8_MAX,  UINT32_MAX);
            LOAD_REG_INSTR ("STRCMB",    STRCMB,     4,       UINT8_MAX,  UINT8_MAX,  UINT8_MAX );
        }
        else if (mode == VM_MODE_STACK)
        {
            //                OPCODE     FUNC        BYTES    DATA RANGES ...
            // Load & Store
            LOAD_STACK_INSTR ("LDI",     LDI,        5,       UINT32_MAX);
            LOAD_STACK_INSTR ("LDI.64",  LDI_64,     9,       UINT64_MAX);
            LOAD_STACK_INSTR0("STM",     STM,        1                  );
            LOAD_STACK_INSTR0("STM.64",  STM_64,     1                  );
            LOAD_STACK_INSTR ("STMI",    STMI,       5,       UINT32_MAX);
            LOAD_STACK_INSTR ("STMI.64", STMI_64,    5,       UINT32_MAX);
            LOAD_STACK_INSTR0("LDM",     LDM,        1                  );
            LOAD_STACK_INSTR0("LDM.64",  LDM_64,     1                  );
            LOAD_STACK_INSTR ("LDMI",    LDMI,       5,       UINT32_MAX);
            LOAD_STACK_INSTR ("LDMI.64", LDMI_64,    5,       UINT32_MAX);
            LOAD_STACK_INSTR ("LDL",     LDL,        2,       UINT8_MAX );
            LOAD_STACK_INSTR ("LDL.64",  LDL_64,     2,       UINT8_MAX );
            LOAD_STACK_INSTR ("LDA",     LDA,        2,       UINT8_MAX );
            LOAD_STACK_INSTR ("LDA.64",  LDA_64,     2,       UINT8_MAX );
            LOAD_STACK_INSTR ("STL",     STL,        2,       UINT8_MAX );
            LOAD_STACK_INSTR ("STL.64",  STL_64,     2,       UINT8_MAX );
            LOAD_STACK_INSTR ("STA",     STA,        2,       UINT8_MAX );
            LOAD_STACK_INSTR ("STA.64",  STA_64,     2,       UINT8_MAX );
            // Arithmetics
            LOAD_STACK_INSTR0("ADD",     ADD,        1                  );
            LOAD_STACK_INSTR0("ADD.64",  ADD_64,     1                  );
            LOAD_STACK_INSTR0("ADD.F",   ADD_F,      1                  );
            LOAD_STACK_INSTR0("ADD.F64", ADD_F64,    1                  );
            LOAD_STACK_INSTR0("SUB",     SUB,        1                  );
            LOAD_STACK_INSTR0("SUB.64",  SUB_64,     1                  );
            LOAD_STACK_INSTR0("SUB.F",   SUB_F,      1                  );
            LOAD_STACK_INSTR0("SUB.F64", SUB_F64,    1                  );
            LOAD_STACK_INSTR0("MUL",     MUL,        1                  );
            LOAD_STACK_INSTR0("MUL.64",  MUL_64,     1                  );
            LOAD_STACK_INSTR0("MUL.F",   MUL_F,      1                  );
            LOAD_STACK_INSTR0("MUL.F64", MUL_F64,    1                  );
            LOAD_STACK_INSTR0("DIV",     DIV,        1                  );
            LOAD_STACK_INSTR0("DIV.64",  DIV_64,     1                  );
            LOAD_STACK_INSTR0("DIV.F",   DIV_F,      1                  );
            LOAD_STACK_INSTR0("DIV.F64", DIV_F64,    1                  );
            // Bit stuff
            LOAD_STACK_INSTR0("INV",     INV,        1                  );
            LOAD_STACK_INSTR0("INV.64",  INV_64,     1                  );
            LOAD_STACK_INSTR0("NEG",     NEG,        1                  );
            LOAD_STACK_INSTR0("NEG.64",  NEG_64,     1                  );
            LOAD_STACK_INSTR0("NEG.F",   NEG_F,      1                  );
            LOAD_STACK_INSTR0("NEG.F64", NEG_F64,    1                  );
            LOAD_STACK_INSTR0("BOR",     BOR,        1                  );
            LOAD_STACK_INSTR0("BOR.64",  BOR_64,     1                  );
            LOAD_STACK_INSTR0("BXOR",    BXOR,       1                  );
            LOAD_STACK_INSTR0("BXOR.64", BXOR_64,    1                  );
            LOAD_STACK_INSTR0("BAND",    BAND,       1                  );
            LOAD_STACK_INSTR0("BAND.64", BAND_64,    1                  );
            LOAD_STACK_INSTR0("OR",      OR,         1                  );
            LOAD_STACK_INSTR0("AND",     AND,        1                  );
            // Conditions & branches
            LOAD_STACK_INSTR0("CPZ",     CPZ,        1                  );
            LOAD_STACK_INSTR0("CPZ.64",  CPZ_64,     1                  );
            LOAD_STACK_INSTR0("CPEQ",    CPEQ,       1                  );
            LOAD_STACK_INSTR0("CPEQ.64", CPEQ_64,    1                  );
            LOAD_STACK_INSTR0("CPEQ.F",  CPEQ_F,     1                  );
            LOAD_STACK_INSTR0("CPEQ.F64",CPEQ_F64,   1                  );
            LOAD_STACK_INSTR0("CPNQ",    CPNQ,       1                  );
            LOAD_STACK_INSTR0("CPNQ.64", CPNQ_64,    1                  );
            LOAD_STACK_INSTR0("CPNQ.F",  CPNQ_F,     1                  );
            LOAD_STACK_INSTR0("CPNQ.F64",CPNQ_F64,   1                  );
            LOAD_STACK_INSTR0("CPGT",    CPGT,       1                  );
            LOAD_STACK_INSTR0("CPGT.64", CPGT_64,    1                  );
            LOAD_STACK_INSTR0("CPGT.F",  CPGT_F,     1                  );
            LOAD_STACK_INSTR0("CPGT.F64",CPGT_F64,   1                  );
            LOAD_STACK_INSTR0("CPLT",    CPLT,       1                  );
            LOAD_STACK_INSTR0("CPLT.64", CPLT_64,    1                  );
            LOAD_STACK_INSTR0("CPLT.F",  CPLT_F,     1                  );
            LOAD_STACK_INSTR0("CPLT.F64",CPLT_F64,   1                  );
            LOAD_STACK_INSTR0("CPGQ",    CPGQ,       1                  );
            LOAD_STACK_INSTR0("CPGQ.64", CPGQ_64,    1                  );
            LOAD_STACK_INSTR0("CPGQ.F",  CPGQ_F,     1                  );
            LOAD_STACK_INSTR0("CPGQ.F64",CPGQ_F64,   1                  );
            LOAD_STACK_INSTR0("CPLQ",    CPLQ,       1                  );
            LOAD_STACK_INSTR0("CPLQ.64", CPLQ_64,    1                  );
            LOAD_STACK_INSTR0("CPLQ.F",  CPLQ_F,     1                  );
            LOAD_STACK_INSTR0("CPLQ.F64",CPLQ_F64,   1                  );
            LOAD_STACK_INSTR0("CPSTR",   CPSTR,      1                  );
            LOAD_STACK_INSTR0("CPCHR",   CPCHR,      1                  );
            LOAD_STACK_INSTR ("BRZ",     BRZ,        5,       UINT32_MAX);
            LOAD_STACK_INSTR ("BRNZ",    BRNZ,       5,       UINT32_MAX);
            LOAD_STACK_INSTR0("BRIZ",    BRIZ,       1                  );
            LOAD_STACK_INSTR0("BRINZ",   BRINZ,      1                  );
            LOAD_STACK_INSTR0("JMPI",    JMPI,       1                  );
            // Conversions
            LOAD_STACK_INSTR0("ITOL",    ITOL,       1                  );
            LOAD_STACK_INSTR0("ITOF",    ITOF,       1                  );
            LOAD_STACK_INSTR0("ITOD",    ITOD,       1                  );
            LOAD_STACK_INSTR0("ITOS",    ITOS,       1                  );
            LOAD_STACK_INSTR0("LTOI",    LTOI,       1                  );
            LOAD_STACK_INSTR0("LTOF",    LTOF,       1                  );
            LOAD_STACK_INSTR0("LTOD",    LTOD,       1                  );
            LOAD_STACK_INSTR0("LTOS",    LTOS,       1                  );
            LOAD_STACK_INSTR0("FTOI",    FTOI,       1                  );
            LOAD_STACK_INSTR0("FTOL",    FTOL,       1                  );
            LOAD_STACK_INSTR0("FTOD",    FTOD,       1                  );
            LOAD_STACK_INSTR ("FTOS",    FTOS,       2,       UINT8_MAX );
            LOAD_STACK_INSTR0("DTOI",    DTOI,       1                  );
            LOAD_STACK_INSTR0("DTOL",    DTOL,       1                  );
            LOAD_STACK_INSTR0("DTOF",    DTOF,       1                  );
            LOAD_STACK_INSTR ("DTOS",    DTOS,       2,       UINT8_MAX );
            LOAD_STACK_INSTR ("STOI",    STOI,       5,       UINT32_MAX);
            LOAD_STACK_INSTR ("STOL",    STOL,       9,       UINT64_MAX);
            LOAD_STACK_INSTR ("STOF",    STOF,       5,       UINT32_MAX);
            LOAD_STACK_INSTR ("STOD",    STOD,       9,       UINT64_MAX);
            // Other stuff
            LOAD_STACK_INSTR0("NEW",     NEW,        1                  );
            LOAD_STACK_INSTR0("DEL",     DEL,        1                  );
            LOAD_STACK_INSTR0("RESZ",    RESZ,       1                  );
            LOAD_STACK_INSTR0("SIZE",    SIZE,       1                  );
            LOAD_STACK_INSTR ("STR",     STR,        5,       UINT32_MAX);
            LOAD_STACK_INSTR ("STRCPY",  STRCPY,     5,       UINT32_MAX);
            LOAD_STACK_INSTR ("STRCAT",  STRCAT,     5,       UINT32_MAX);
            LOAD_STACK_INSTR0("STRCMB",  STRCMB,     1                  );
        }
    }

    size_t InstructionEncoder::GetInstructionByteSize(const std::string& opcode) const
    {
        auto it = m_info.find(opcode);
        if (it == m_info.end())
            return 0;

        return it->second.byteSize;
    }

    uint64_t InstructionEncoder::GetInstructionMaxArgSize(const std::string& opcode, int argIdx) const
    {
        if (argIdx >= 3)
            return 0;

        auto it = m_info.find(opcode);
        if (it == m_info.end())
            return 0;

        return it->second.argMax[argIdx];
    }

    size_t InstructionEncoder::GetInstructionArgCount(const std::string& opcode) const
    {
        auto it = m_info.find(opcode);
        if (it == m_info.end())
            return 0;

        size_t cnt;
        for (cnt = 0; cnt < 3 && it->second.argMax[cnt] > 0; cnt++);

        return cnt;
    }

}
