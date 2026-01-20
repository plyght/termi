#include <stdio.h>
#include <stdint.h>

int main() {
    uint32_t insn = 0xeb04005f;
    uint32_t mask = 0x7fe0001f;
    uint32_t expected = 0x6b00001f;
    
    uint32_t result = insn & mask;
    
    printf("insn:     0x%08x\n", insn);
    printf("mask:     0x%08x\n", mask);
    printf("result:   0x%08x\n", result);
    printf("expected: 0x%08x\n", expected);
    printf("Match: %s\n", (result == expected) ? "YES" : "NO");
    
    // Binary representation
    printf("\nBinary:\n");
    for (int i = 31; i >= 0; i--) {
        printf("%d", (insn >> i) & 1);
        if (i % 4 == 0) printf(" ");
    }
    printf(" = insn\n");
    
    for (int i = 31; i >= 0; i--) {
        printf("%d", (mask >> i) & 1);
        if (i % 4 == 0) printf(" ");
    }
    printf(" = mask\n");
    
    for (int i = 31; i >= 0; i--) {
        printf("%d", (result >> i) & 1);
        if (i % 4 == 0) printf(" ");
    }
    printf(" = result\n");
    
    for (int i = 31; i >= 0; i--) {
        printf("%d", (expected >> i) & 1);
        if (i % 4 == 0) printf(" ");
    }
    printf(" = expected\n");
    
    return 0;
}
