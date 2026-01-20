#include "arm64_cpu.h"
#include <string.h>

void arm64_cpu_init(arm64_cpu_t *cpu, void *mmu)
{
    memset(cpu, 0, sizeof(arm64_cpu_t));
    cpu->mmu = mmu;
    cpu->halted = false;
    cpu->exit_code = 0;
}

void arm64_cpu_reset(arm64_cpu_t *cpu)
{
    memset(cpu->x, 0, sizeof(cpu->x));
    cpu->sp = 0;
    cpu->pc = 0;
    cpu->pstate.n = 0;
    cpu->pstate.z = 0;
    cpu->pstate.c = 0;
    cpu->pstate.v = 0;
    memset(cpu->v, 0, sizeof(cpu->v));
    cpu->fpcr = 0;
    cpu->fpsr = 0;
    cpu->tpidr_el0 = 0;
    cpu->halted = false;
    cpu->exit_code = 0;
}

void arm64_cpu_set_reg(arm64_cpu_t *cpu, int reg, uint64_t value)
{
    if (reg >= 0 && reg < ARM64_NUM_REGS) {
        cpu->x[reg] = value;
    } else if (reg == ARM64_REG_SP) {
        cpu->sp = value;
    } else if (reg == ARM64_REG_PC) {
        cpu->pc = value;
    }
}

uint64_t arm64_cpu_get_reg(arm64_cpu_t *cpu, int reg)
{
    if (reg >= 0 && reg < ARM64_NUM_REGS) {
        return cpu->x[reg];
    } else if (reg == ARM64_REG_SP) {
        return cpu->sp;
    } else if (reg == ARM64_REG_PC) {
        return cpu->pc;
    }
    return 0;
}

void arm64_cpu_set_pc(arm64_cpu_t *cpu, uint64_t pc)
{
    cpu->pc = pc;
}

uint64_t arm64_cpu_get_pc(arm64_cpu_t *cpu)
{
    return cpu->pc;
}
