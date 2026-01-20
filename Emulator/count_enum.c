#include "cpu/decoder.h"
#include <stdio.h>

int main() {
    printf("LDUR_POST = %d\n", ARM64_INSN_LDUR_POST);
    printf("STR_REG = %d\n", ARM64_INSN_STR_REG);
    return 0;
}
