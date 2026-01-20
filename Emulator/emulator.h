#ifndef TERMI_EMULATOR_H
#define TERMI_EMULATOR_H

#include "cpu/arm64_cpu.h"
#include "loader/elf_loader.h"
#include "mmu/mmu.h"

typedef struct {
    arm64_cpu_t *cpu;
    arm64_mmu_t *mmu;
} arm64_emulator_t;

arm64_emulator_t *arm64_emulator_create(void);
void arm64_emulator_destroy(arm64_emulator_t *emu);

int arm64_emulator_load_elf(arm64_emulator_t *emu, const char *path);
int arm64_emulator_load_elf_memory(arm64_emulator_t *emu, const void *data, size_t size);

void arm64_emulator_run(arm64_emulator_t *emu);
int arm64_emulator_step(arm64_emulator_t *emu);

int arm64_interpreter_step(arm64_cpu_t *cpu);
void arm64_interpreter_run(arm64_cpu_t *cpu, uint64_t max_insns);

#endif
