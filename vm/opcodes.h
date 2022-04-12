#ifndef INC_OPCODES_H
#define INC_OPCODES_H

typedef enum {
    NOP         = 0x00,
    EXIT        = 0x01,
    JMP,
    CALL,
    RET,
    RET_32,
    RET_64,
    SCALL,
    SARG,

    /******** REGISTER-BASED INSTRUCTIONS ********/
    /* Load & Store */
    R_MOV       = 0x09,
    R_MOV_64,
    R_LDI,
    R_LDI_64,
    R_STM,
    R_STM_64,
    R_STMI,
    R_STMI_64,
    R_LDM,
    R_LDM_64,
    R_LDMI,
    R_LDMI_64,
    R_LDL,
    R_LDL_64,
    R_LDA,
    R_LDA_64,
    R_STL,
    R_STL_64,
    R_STA,
    R_STA_64,
    R_MOVS,
    R_MOVS_64,
    R_POP,
    R_POP_64,
    R_PUSH,
    R_PUSH_64,

    /* Arithmetics */
    R_ADD       = 0x23,
    R_ADD_64,
    R_ADD_F,
    R_ADD_F64,
    R_ADDI,
    R_ADDI_64,
    R_ADDI_F,
    R_ADDI_F64,
    R_SUB,
    R_SUB_64,
    R_SUB_F,
    R_SUB_F64,
    R_SUBI,
    R_SUBI_64,
    R_SUBI_F,
    R_SUBI_F64,
    R_MUL,
    R_MUL_64,
    R_MUL_F,
    R_MUL_F64,
    R_MULI,
    R_MULI_64,
    R_MULI_F,
    R_MULI_F64,
    R_DIV,
    R_DIV_64,
    R_DIV_F,
    R_DIV_F64,
    R_DIVI,
    R_DIVI_64,
    R_DIVI_F,
    R_DIVI_F64,

    /* Bit Stuff */
    R_INV       = 0x43,
    R_INV_64,
    R_NEG,
    R_NEG_64,
    R_NEG_F,
    R_NEG_F64,
    R_BOR,
    R_BOR_64,
    R_BORI,
    R_BORI_64,
    R_BXOR,
    R_BXOR_64,
    R_BXORI,
    R_BXORI_64,
    R_BAND,
    R_BAND_64,
    R_BANDI,
    R_BANDI_64,

    /* Conditions & Branches */
    R_OR        = 0x55,
    R_ORI,
    R_AND,
    R_ANDI,
    R_CPZ,
    R_CPZ_64,
    R_CPI,
    R_CPI_64,
    R_CPEQ,
    R_CPEQ_64,
    R_CPEQ_F,
    R_CPEQ_F64,
    R_CPNQ,
    R_CPNQ_64,
    R_CPNQ_F,
    R_CPNQ_F64,
    R_CPGT,
    R_CPGT_64,
    R_CPGT_F,
    R_CPGT_F64,
    R_CPLT,
    R_CPLT_64,
    R_CPLT_F,
    R_CPLT_F64,
    R_CPGQ,
    R_CPGQ_64,
    R_CPGQ_F,
    R_CPGQ_F64,
    R_CPLQ,
    R_CPLQ_64,
    R_CPLQ_F,
    R_CPLQ_F64,
    R_CPSTR,
    R_CPCHR,
    R_BRZ,
    R_BRNZ,
    R_BRIZ,
    R_BRINZ,
    R_JMPI,

    /* Conversions */
    R_ITOL      = 0x7C,
    R_ITOF,
    R_ITOD,
    R_ITOS,
    R_LTOI,
    R_LTOF,
    R_LTOD,
    R_LTOS,
    R_FTOI,
    R_FTOL,
    R_FTOD,
    R_FTOS,
    R_DTOI,
    R_DTOL,
    R_DTOF,
    R_DTOS,
    R_STOI,
    R_STOL,
    R_STOF,
    R_STOD,

    /* Miscellaneous */
    R_NEW       = 0x90,
    R_NEWI,
    R_DEL,
    R_RESZ,
    R_RESZI,
    R_SIZE,
    R_STR,
    R_STRCPY,
    R_STRCAT,
    R_STRCMB,

    R_OPCODE_COUNT,

    /******** STACK-BASED INSTRUCTIONS ********/
    /* Load & Store */
    S_LDI       = 0x09,
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
    S_ADD       = 0x1B,
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
    S_INV       = 0x2B,
    S_INV_64,
    S_NEG,
    S_NEG_64,
    S_NEG_F,
    S_NEG_F64,
    S_BOR,
    S_BOR_64,
    S_BXOR,
    S_BXOR_64,
    S_BAND,
    S_BAND_64,

    /* Conditions & Branches */
    S_OR        = 0x37,
    S_AND,
    S_CPZ,
    S_CPZ_64,
    S_CPEQ,
    S_CPEQ_64,
    S_CPEQ_F,
    S_CPEQ_F64,
    S_CPNQ,
    S_CPNQ_64,
    S_CPNQ_F,
    S_CPNQ_F64,
    S_CPGT,
    S_CPGT_64,
    S_CPGT_F,
    S_CPGT_F64,
    S_CPLT,
    S_CPLT_64,
    S_CPLT_F,
    S_CPLT_F64,
    S_CPGQ,
    S_CPGQ_64,
    S_CPGQ_F,
    S_CPGQ_F64,
    S_CPLQ,
    S_CPLQ_64,
    S_CPLQ_F,
    S_CPLQ_F64,
    S_CPSTR,
    S_CPCHR,
    S_BRZ,
    S_BRNZ,
    S_BRIZ,
    S_BRINZ,
    S_JMPI,

    /* Conversions */
    S_ITOL      = 0x5A,
    S_ITOF,
    S_ITOD,
    S_ITOS,
    S_LTOI,
    S_LTOF,
    S_LTOD,
    S_LTOS,
    S_FTOI,
    S_FTOL,
    S_FTOD,
    S_FTOS,
    S_DTOI,
    S_DTOL,
    S_DTOF,
    S_DTOS,
    S_STOI,
    S_STOL,
    S_STOF,
    S_STOD,

    /* Miscellaneous */
    S_NEW       = 0x6E,
    S_DEL,
    S_RESZ,
    S_SIZE,
    S_STR,
    S_STRCPY,
    S_STRCAT,
    S_STRCMB,

    S_OPCODE_COUNT
} Opcode_t;

#endif /* INC_OPCODES_H */