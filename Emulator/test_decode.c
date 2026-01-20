#include <stdio.h>
#include <stdint.h>

#define BIT(x, n) (((x) >> (n)) & 1)
#define BITS(x, high, low) (((x) >> (low)) & ((1ULL << ((high) - (low) + 1)) - 1))

int main() {
    uint32_t insn = 0xf8627805;
    
    printf("Instruction: 0x%08x\n", insn);
    printf("Binary: ");
    for (int i = 31; i >= 0; i--) {
        printf("%d", (insn >> i) & 1);
        if (i % 4 == 0) printf(" ");
    }
    printf("\n\n");
    
    // Check pattern 0x3f200c00 == 0x38000800 (GP register)
    uint32_t mask1 = 0x3f200c00;
    uint32_t exp1 = 0x38000800;
    uint32_t result1 = insn & mask1;
    printf("Pattern 1 (GP): mask=0x%08x, expected=0x%08x, result=0x%08x, match=%d\n",
           mask1, exp1, result1, result1 == exp1);
    
    // Check pattern 0x3f200c00 == 0x38200800 (FP/SIMD register)
    uint32_t mask2 = 0x3f200c00;
    uint32_t exp2 = 0x38200800;
    uint32_t result2 = insn & mask2;
    printf("Pattern 2 (FP): mask=0x%08x, expected=0x%08x, result=0x%08x, match=%d\n",
           mask2, exp2, result2, result2 == exp2);
    
    printf("\nField extraction:\n");
    printf("size (bits 31-30): %d\n", BITS(insn, 31, 30));
    printf("V (bit 26): %d\n", BIT(insn, 26));
    printf("opc (bits 23-22): %d\n", BITS(insn, 23, 22));
    printf("Rm (bits 20-16): %d\n", BITS(insn, 20, 16));
    printf("option (bits 15-13): %d\n", BITS(insn, 15, 13));
    printf("S (bit 12): %d\n", BIT(insn, 12));
    printf("Rn (bits 9-5): %d\n", BITS(insn, 9, 5));
    printf("Rt (bits 4-0): %d\n", BITS(insn, 4, 0));
    
    return 0;
}
