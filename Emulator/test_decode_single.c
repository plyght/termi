#include "cpu/decoder.h"
#include <stdio.h>

int main() {
    uint32_t insn = 0xeb04005f;
    arm64_insn_t decoded;
    
    int result = arm64_decode(insn, &decoded);
    
    printf("Instruction: 0x%08x\n", insn);
    printf("Decode result: %d (1=success, 0=failure)\n", result);
    printf("Instruction type: %d\n", decoded.type);
    printf("sf: %d, rn: %d, rm: %d, rd: %d\n", decoded.sf, decoded.rn, decoded.rm, decoded.rd);
    
    if (result == 1) {
        printf("SUCCESS: Decoded as type %d\n", decoded.type);
    } else {
        printf("FAILURE: Could not decode\n");
    }
    
    return 0;
}
