#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <locale.h>
#include <time.h>
#include <math.h>

#define SAMPLE_SIZE 20          // Will take roughly 6 minutes, depending on the processor.
#define ITERATION_SIZE 40000
#define TOTAL_ITERATIONS ((size_t)ITERATION_SIZE * ITERATION_SIZE)

volatile uint32_t reg[256];

union {
    struct {
        uint8_t opcode;
        uint8_t op1;
        uint8_t op2;
        uint8_t op3;
    };
    struct {
        uint8_t  opcode_i;
        uint8_t  op1_i;
        uint16_t op2_i;
    };
    uint32_t data;
} instr;

void bitmask_ADD()
{
    reg[(instr.data & 0x0000FF00) >> 8] =
        reg[(instr.data & 0x00FF0000) >> 16] +
        reg[(instr.data & 0xFF000000) >> 24];
}

void bitmask_LDI()
{
    reg[(instr.data & 0x0000FF00) >> 8] =
        reg[(instr.data & 0xFFFF0000) >> 16];
}

void union_ADD()
{
    reg[instr.op1] = reg[instr.op2] + reg[instr.op3];
}

void union_LDI()
{
    reg[instr.op1_i] = instr.op2_i;
}

int main(int argc, char **argv)
{
    clock_t exp_start, start, end;
    double duration;

    srand(time(NULL));
    exp_start = clock();

    instr.opcode = 0x00;
    instr.op1    = 0x00;
    instr.op2    = 0x00;
    instr.op3    = 0x00;

    double avg_bitmask = 0.0;
    double avg_union = 0.0;

    printf("Running %'lld mock virtual instructions per sample...\n", TOTAL_ITERATIONS);

    for (int sample = 1; sample <= SAMPLE_SIZE; sample++)
    {
        printf("Sample %d...\n", sample);

        // Time it using bitmask decoding.
        start = clock();

        for (int i = 0; i < ITERATION_SIZE; i++)
        {
            for (int j = 0; j < ITERATION_SIZE; j++)
            {
                instr.data = j % UINT32_MAX;
                if (j % 2 == 1) 
                    bitmask_ADD();
                else 
                    bitmask_LDI();
            }
        }

        end = clock();

        duration = (double)(end - start) / CLOCKS_PER_SEC;
        avg_bitmask += duration;
        printf("\tBitmask: %.4f\n", duration);

        // Time it using union.
        start = clock();

        for (int i = 0; i < ITERATION_SIZE; i++)
        {
            for (int j = 0; j < ITERATION_SIZE; j++)
            {
                instr.data = j % UINT32_MAX;
                if (j % 2 == 1) 
                    union_ADD();
                else 
                    union_LDI();
            }
        }

        end = clock();

        duration = (double)(end - start) / CLOCKS_PER_SEC;
        avg_union += duration;
        printf("\tUnion: %.4f\n", duration);
    }

    duration = (double)(clock() - exp_start) / CLOCKS_PER_SEC;
    printf("Time elapsed: %.2fs", duration);

    avg_bitmask /= SAMPLE_SIZE;
    avg_union /= SAMPLE_SIZE;
    double avg_bitmask_ratio = avg_bitmask / avg_union;
    double avg_union_ratio = avg_union / avg_bitmask;

    printf("\nBitmask encoding: avg %.4fs, avg ratio %.4f\n", 
        avg_bitmask, avg_bitmask_ratio);

    printf("Union accessing:  avg %.4fs, avg ratio %.4f\n", 
        avg_union, avg_union_ratio);

    printf("Unions are %.2f%% %s than bitmasking.\n", 
        fabs((avg_bitmask_ratio - 1.0) * 100.0), 
        avg_union < avg_bitmask ? "faster" : "slower");

    return 0;
}
