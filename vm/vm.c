#include <memory.h>
#include <stdint.h>
#include <stdio.h>

#define REGISTER_COUNT 32
#define USE_REG(r)  R_usedCount += R_used[r] ? 0 : 1; R_used[r] = 1
#define FREE_REG(r) R_usedCount -= R_used[r] ? 1 : 0; R_used[r] = 0
#define IsRegFree(r) (!R_used[r])
#define IsOutOfRegs(r) (R_usedCount >= REGISTER_COUNT)

/* Extracts K from instruction (last 18 bits). */
#define GetIR_K() ((IR.raw & (0x3FFFF << 14)) >> 14) 

/* Extracts R0 from instruction (bit 6-13). */
#define GetIR_R0() ((IR.raw & (0xFF << 6)) >> 6) 

/* Extracts R1 from instruction (bit 14-21). */
#define GetIR_R1() ((IR.raw & (0xFF << 14)) >> 14)

/* Extracts R2 from instruction (bit 22-29). */
#define GetIR_R2() ((IR.raw & (0xFF << 22)) >> 22) 

typedef int32_t  Word;
typedef uint32_t Uword;
typedef uint32_t Ptr;

typedef union Instr {
    uint8_t  opcode : 6;
    uint32_t raw;
} Instr_t;

/* Declare registers. */
static Word R[REGISTER_COUNT];

static Instr_t IR; /* Instruction register. */

/*** INSTRUCTIONS ***/
enum OpCode {
    OPCODE_MOV = 0x00,
    OPCODE_LDK = 0x01,
    OPCODE_ADD = 0x02,

    OPCODE_COUNT
};

static void instr_MOV(void)
{
    *(R+GetIR_R0()) = *(R+GetIR_R1());
}

static void instr_LDK(void)
{
    *(R+GetIR_R0()) = GetIR_K();
}

static void instr_ADD(void)
{
    *(R+GetIR_R0()) = *(R+GetIR_R1()) + *(R+GetIR_R2());
}

/* Declare an array of function pointers for instructions. */
static void(*DoInstruction[OPCODE_COUNT])(void) = {
    instr_MOV,
    instr_LDK,
    instr_ADD
    };

/********************/

#define SetR0(val) IR.raw |= (val << 6)
#define SetR1(val) IR.raw |= (val << 14)
#define SetR2(val) IR.raw |= (val << 22)
#define SetK(val)  IR.raw |= (val << 14)

int main(int argc, const char **argv)
{
    IR.raw = OPCODE_LDK;
    SetR0(0x0);
    SetK(1024);

    //printf("IR = 0x%08X\n", IR.raw);

    printf("opcode = %d\nR0 = %d\nK = %d\n", IR.opcode, GetIR_R0(), GetIR_K());

    DoInstruction[IR.opcode]();
    printf("\nR[0] = %d\n\n", R[0]);

    IR.raw = OPCODE_LDK;
    SetR0(0x1);
    SetK(6);
    printf("opcode = %d\nR0 = %d\nK = %d\n", IR.opcode, GetIR_R0(), GetIR_K());

    DoInstruction[IR.opcode]();
    printf("\nR[1] = %d\n\n", R[1]);
    
    IR.raw = OPCODE_ADD;
    SetR0(0x0);
    SetR1(0x0);
    SetR2(0x1);
    printf("opcode = %d\nR0 = %d\nR1 = %d\nR2 = %d\n", IR.opcode, GetIR_R0(), GetIR_R1(), GetIR_R2());

    DoInstruction[IR.opcode]();
    printf("\nR[0] = %d\n\n", R[0]);  
    
    return 0;
}
