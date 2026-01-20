#include "emulator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

arm64_emulator_t *arm64_emulator_create(void)
{
    arm64_emulator_t *emu = malloc(sizeof(arm64_emulator_t));
    if (!emu)
        return NULL;

    emu->mmu = arm64_mmu_create();
    if (!emu->mmu) {
        free(emu);
        return NULL;
    }

    emu->cpu = malloc(sizeof(arm64_cpu_t));
    if (!emu->cpu) {
        arm64_mmu_destroy(emu->mmu);
        free(emu);
        return NULL;
    }

    arm64_cpu_init(emu->cpu, emu->mmu);

    return emu;
}

void arm64_emulator_destroy(arm64_emulator_t *emu)
{
    if (!emu)
        return;

    if (emu->cpu) {
        free(emu->cpu);
    }

    if (emu->mmu) {
        arm64_mmu_destroy(emu->mmu);
    }

    free(emu);
}

int arm64_emulator_load_elf(arm64_emulator_t *emu, const char *path)
{
    elf_load_info_t info;

    if (elf_load(path, emu->mmu, &info) < 0) {
        return -1;
    }

    emu->cpu->pc = info.entry_point;
    emu->cpu->sp = info.stack_top;

    return 0;
}

int arm64_emulator_load_elf_memory(arm64_emulator_t *emu, const void *data, size_t size)
{
    elf_load_info_t info;

    if (elf_load_memory(data, size, emu->mmu, &info) < 0) {
        return -1;
    }

    emu->cpu->pc = info.entry_point;
    emu->cpu->sp = info.stack_top;

    return 0;
}

void arm64_emulator_run(arm64_emulator_t *emu)
{
    printf("[CPU] Starting execution at PC=0x%llx, SP=0x%llx\n",
           (unsigned long long)emu->cpu->pc, (unsigned long long)emu->cpu->sp);
    
    arm64_interpreter_run(emu->cpu, 0);
    
    if (emu->cpu->halted) {
        printf("[CPU] Execution halted, exit_code=%d\n", emu->cpu->exit_code);
    } else {
        printf("[CPU] Execution stopped\n");
    }
}

int arm64_emulator_step(arm64_emulator_t *emu)
{
    return arm64_interpreter_step(emu->cpu);
}
