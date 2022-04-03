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

    //---- Register-only instructions ----//

    DECL_REG_INSTR2(0x00, MOV,      Register,   Register)
    DECL_REG_INSTR2(0x00, LDI,      Register,   uint32_t)
    DECL_REG_INSTR2(0x00, LDI_64,   Register,   uint64_t)
    DECL_REG_INSTR3(0x00, ADD,      Register,   Register,   Register)
    DECL_REG_INSTR3(0x00, ADD_64,   Register,   Register,   Register)
    DECL_REG_INSTR3(0x00, ADDI,     Register,   Register,   uint32_t)
    DECL_REG_INSTR3(0x00, ADDI_64,  Register,   Register,   uint64_t)

    //---- Stack-only instructions ----//
    DECL_STACK_INSTR1(0x01, LDI,        uint32_t)
    DECL_STACK_INSTR1(0x02, LDI_64,     uint64_t)
    DECL_STACK_INSTR0(0x03, STM                 )
    DECL_STACK_INSTR0(0x04, STM_64              )
    DECL_STACK_INSTR1(0x05, STMI,       uint32_t)
    DECL_STACK_INSTR1(0x06, STMI_64,    uint64_t)
    DECL_STACK_INSTR0(0x07, LDM                 )
    DECL_STACK_INSTR0(0x08, LDM_64              )
    DECL_STACK_INSTR1(0x09, LDMI,       uint32_t)
    DECL_STACK_INSTR1(0x0A, LDMI_64,    uint64_t)
    DECL_STACK_INSTR1(0x0B, LDL,        uint8_t )
    DECL_STACK_INSTR1(0x0C, LDL_64,     uint8_t )
    DECL_STACK_INSTR1(0x0D, LDA,        uint8_t )
    DECL_STACK_INSTR1(0x0E, LDA_64,     uint8_t )
    DECL_STACK_INSTR1(0x0F, STL,        uint8_t )
    DECL_STACK_INSTR1(0x10, STL_64,     uint8_t )
    DECL_STACK_INSTR1(0x11, STA,        uint8_t )
    DECL_STACK_INSTR1(0x12, STA_64,     uint8_t )

    DECL_STACK_INSTR0(0x00, ADD                 )
    DECL_STACK_INSTR0(0x00, ADD_64              )
    DECL_STACK_INSTR0(0x00, ADD_F               )
    DECL_STACK_INSTR0(0x00, ADD_F64             )
    DECL_STACK_INSTR0(0x00, SUB                 )
    DECL_STACK_INSTR0(0x00, SUB_64              )
    DECL_STACK_INSTR0(0x00, SUB_F               )
    DECL_STACK_INSTR0(0x00, SUB_F64             )
    DECL_STACK_INSTR0(0x00, MUL                 )
    DECL_STACK_INSTR0(0x00, MUL_64              )
    DECL_STACK_INSTR0(0x00, MUL_F               )
    DECL_STACK_INSTR0(0x00, MUL_F64             )
    DECL_STACK_INSTR0(0x00, DIV                 )
    DECL_STACK_INSTR0(0x00, DIV_64              )
    DECL_STACK_INSTR0(0x00, DIV_F               )
    DECL_STACK_INSTR0(0x00, DIV_F64             )

    DECL_STACK_INSTR0(0x00, INV                 )
    DECL_STACK_INSTR0(0x00, INV_64              )
    DECL_STACK_INSTR0(0x00, NEG                 )
    DECL_STACK_INSTR0(0x00, NEG_64              )
    DECL_STACK_INSTR0(0x00, NEG_F               )
    DECL_STACK_INSTR0(0x00, NEG_F64             )
    DECL_STACK_INSTR0(0x00, BOR                 )
    DECL_STACK_INSTR0(0x00, BOR_64              )
    DECL_STACK_INSTR0(0x00, BXOR                )
    DECL_STACK_INSTR0(0x00, BXOR_64             )
    DECL_STACK_INSTR0(0x00, BAND                )
    DECL_STACK_INSTR0(0x00, BAND_64             )
    DECL_STACK_INSTR0(0x00, OR                  )
    DECL_STACK_INSTR0(0x00, AND                 )

    DECL_STACK_INSTR0(0x00, CPZ                 )
    DECL_STACK_INSTR0(0x00, CPZ_64              )
    DECL_STACK_INSTR0(0x00, CPEQ                )
    DECL_STACK_INSTR0(0x00, CPEQ_64             )
    DECL_STACK_INSTR0(0x00, CPEQ_F              )
    DECL_STACK_INSTR0(0x00, CPEQ_F64            )
    DECL_STACK_INSTR0(0x00, CPNQ                )
    DECL_STACK_INSTR0(0x00, CPNQ_64             )
    DECL_STACK_INSTR0(0x00, CPNQ_F              )
    DECL_STACK_INSTR0(0x00, CPNQ_F64            )
    DECL_STACK_INSTR0(0x00, CPGT                )
    DECL_STACK_INSTR0(0x00, CPGT_64             )
    DECL_STACK_INSTR0(0x00, CPGT_F              )
    DECL_STACK_INSTR0(0x00, CPGT_F64            )
    DECL_STACK_INSTR0(0x00, CPLT                )
    DECL_STACK_INSTR0(0x00, CPLT_64             )
    DECL_STACK_INSTR0(0x00, CPLT_F              )
    DECL_STACK_INSTR0(0x00, CPLT_F64            )
    DECL_STACK_INSTR0(0x00, CPGQ                )
    DECL_STACK_INSTR0(0x00, CPGQ_64             )
    DECL_STACK_INSTR0(0x00, CPGQ_F              )
    DECL_STACK_INSTR0(0x00, CPGQ_F64            )
    DECL_STACK_INSTR0(0x00, CPLQ                )
    DECL_STACK_INSTR0(0x00, CPLQ_64             )
    DECL_STACK_INSTR0(0x00, CPLQ_F              )
    DECL_STACK_INSTR0(0x00, CPLQ_F64            )
    DECL_STACK_INSTR1(0x00, BRZ,        uint32_t)
    DECL_STACK_INSTR1(0x00, BRNZ,       uint32_t)
    DECL_STACK_INSTR1(0x00, JMP,        uint32_t)
    DECL_STACK_INSTR0(0x00, BRIZ                )
    DECL_STACK_INSTR0(0x00, BRINZ               )
    DECL_STACK_INSTR0(0x00, JMPI                )

    DECL_STACK_INSTR0(0x00, NEW                 )
    DECL_STACK_INSTR0(0x00, DEL                 )
    DECL_STACK_INSTR0(0x00, RESZ                )
    DECL_STACK_INSTR0(0x00, SIZE                )
    DECL_STACK_INSTR1(0x00, CALL,       uint32_t)
    DECL_STACK_INSTR1(0x00, SCALL,      uint8_t )
    DECL_STACK_INSTR0(0x00, RET                 )
    //----------------------------------//

    InstructionEncoder::InstructionEncoder()
    {
    }

    void InstructionEncoder::LoadInstructionSet(const VMMode mode)
    {
        m_translate.clear();

        // Load the instructions and their byte size, 
        // followed by the maximum value for each argument.
        LOAD_INSTR0("NOP", NOP, 1);
        LOAD_INSTR0("EXIT", EXIT, 1);

        if (mode == VM_MODE_REGISTER)
        {
            //              OPCODE       FUNC        BYTES    DATA RANGES ...
            LOAD_REG_INSTR ("LDI",       LDI,        0,       UINT8_MAX,  UINT32_MAX            );
            LOAD_REG_INSTR ("LDI.64",    LDI_64,     0,       UINT8_MAX,  UINT64_MAX            );
            LOAD_REG_INSTR ("ADD",       ADD,        0,       UINT8_MAX,  UINT8_MAX,  UINT8_MAX );
            LOAD_REG_INSTR ("ADD.64",    ADD_64,     0,       UINT8_MAX,  UINT8_MAX,  UINT8_MAX );
            LOAD_REG_INSTR ("ADDI",      ADDI,       0,       UINT8_MAX,  UINT8_MAX,  UINT32_MAX);
            LOAD_REG_INSTR ("ADDI.64",   ADDI_64,    0,       UINT8_MAX,  UINT8_MAX,  UINT64_MAX);
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
            LOAD_STACK_INSTR ("BRZ",     BRZ,        5,       UINT32_MAX);
            LOAD_STACK_INSTR ("BRNZ",    BRNZ,       5,       UINT32_MAX);
            LOAD_STACK_INSTR ("JMP",     JMP,        5,       UINT32_MAX);
            LOAD_STACK_INSTR0("BRIZ",    BRIZ,       1                  );
            LOAD_STACK_INSTR0("BRINZ",   BRINZ,      1                  );
            LOAD_STACK_INSTR0("JMPI",    JMPI,       1                  );
            // Other stuff
            LOAD_STACK_INSTR0("NEW",     NEW,        1                  );
            LOAD_STACK_INSTR0("DEL",     DEL,        1                  );
            LOAD_STACK_INSTR0("RESZ",    RESZ,       1                  );
            LOAD_STACK_INSTR0("SIZE",    SIZE,       1                  );
            LOAD_STACK_INSTR ("CALL",    CALL,       5,       UINT32_MAX);
            LOAD_STACK_INSTR ("SCALL",   SCALL,      2,       UINT8_MAX );
            LOAD_STACK_INSTR0("RET",     RET,        1                  );
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
