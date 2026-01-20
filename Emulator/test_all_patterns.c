#include <stdio.h>
#include <stdint.h>

#define BIT(x, n) (((x) >> (n)) & 1)
#define BITS(x, high, low) (((x) >> (low)) & ((1ULL << ((high) - (low) + 1)) - 1))

int main() {
    uint32_t insn = 0xf8627805;
    
    printf("Testing instruction 0x%08x against all decoder patterns:\n\n", insn);
    
    struct {
        const char *name;
        uint32_t mask;
        uint32_t expected;
    } patterns[] = {
        {"LDR/STR unsigned imm", 0x3f000000, 0x39000000},
        {"LDUR/STUR family", 0x3b200000, 0x38000000},
        {"LDR/STR GP register", 0x3f200c00, 0x38000800},
        {"LDR/STR FP register", 0x3f200c00, 0x38200800},
        {"LDR/STR pre/post idx", 0x3b200000, 0x38000400},
        {NULL, 0, 0}
    };
    
    for (int i = 0; patterns[i].name != NULL; i++) {
        uint32_t result = insn & patterns[i].mask;
        int match = (result == patterns[i].expected);
        printf("%-30s: mask=0x%08x, exp=0x%08x, res=0x%08x, %s\n",
               patterns[i].name, patterns[i].mask, patterns[i].expected, 
               result, match ? "MATCH" : "no match");
    }
    
    return 0;
}
