#include "emulator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

int main(int argc, char **argv) {
    const char *busybox_path = "/Users/nicojaffer/termi/Alpine/rootfs/data/bin/busybox";
    
    if (argc > 1) {
        busybox_path = argv[1];
    }

    printf("Testing busybox at: %s\n", busybox_path);
    printf("========================================\n\n");

    FILE *f = fopen(busybox_path, "rb");
    if (!f) {
        fprintf(stderr, "Failed to open busybox\n");
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t *data = malloc(size);
    if (fread(data, 1, size, f) != size) {
        fprintf(stderr, "Failed to read busybox\n");
        fclose(f);
        free(data);
        return 1;
    }
    fclose(f);

    printf("Loaded busybox: %ld bytes\n\n", size);

    arm64_emulator_t *emu = arm64_emulator_create();
    if (!emu) {
        fprintf(stderr, "Failed to create emulator\n");
        free(data);
        return 1;
    }

    if (arm64_emulator_load_elf_memory(emu, data, size) < 0) {
        fprintf(stderr, "Failed to load ELF\n");
        arm64_emulator_destroy(emu);
        free(data);
        return 1;
    }

    printf("ELF loaded successfully\n");
    printf("Entry point: 0x%llx\n", (unsigned long long)emu->cpu->pc);
    printf("Stack pointer: 0x%llx\n\n", (unsigned long long)emu->cpu->sp);

    printf("Executing instructions...\n");
    printf("========================================\n\n");

    int max_instructions = 1000;
    for (int i = 0; i < max_instructions; i++) {
        uint64_t pc = emu->cpu->pc;
        
        if (arm64_emulator_step(emu) < 0) {
            printf("\nExecution stopped at instruction %d\n", i);
            printf("PC: 0x%llx\n", (unsigned long long)pc);
            
            uint32_t insn_raw;
            if (arm64_mmu_read_u32(emu->cpu->mmu, pc, &insn_raw) == 0) {
                printf("Failed instruction: 0x%08x\n", insn_raw);
            }
            
            break;
        }

        if (i % 100 == 0 && i > 0) {
            printf("Executed %d instructions, PC=0x%llx\n", i, (unsigned long long)emu->cpu->pc);
        }

        if (emu->cpu->halted) {
            printf("\nCPU halted after %d instructions\n", i);
            printf("Exit code: %d\n", emu->cpu->exit_code);
            break;
        }
    }

    printf("\nFinal state:\n");
    printf("PC: 0x%llx\n", (unsigned long long)emu->cpu->pc);
    printf("SP: 0x%llx\n", (unsigned long long)emu->cpu->sp);
    printf("X0: 0x%llx\n", (unsigned long long)emu->cpu->x[0]);

    arm64_emulator_destroy(emu);
    free(data);

    return 0;
}
