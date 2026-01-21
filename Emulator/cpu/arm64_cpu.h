#ifndef TERMI_ARM64_CPU_H
#define TERMI_ARM64_CPU_H

#include <stdbool.h>
#include <stdint.h>

#define ARM64_NUM_REGS 31
#define ARM64_FP_NUM_REGS 32

typedef struct {
    uint64_t n;
    uint64_t z;
    uint64_t c;
    uint64_t v;
} arm64_pstate_t;

typedef union {
    uint8_t b[16];
    uint16_t h[8];
    uint32_t s[4];
    uint64_t d[2];
    __uint128_t q;
} arm64_simd_reg_t;

typedef struct {
    uint64_t x[ARM64_NUM_REGS];
    uint64_t sp;
    uint64_t pc;

    arm64_pstate_t pstate;

    arm64_simd_reg_t v[ARM64_FP_NUM_REGS];
    uint32_t fpcr;
    uint32_t fpsr;

    uint64_t tpidr_el0;
    uint64_t exclusive_addr;

    void *mmu;

    bool halted;
    int exit_code;
} arm64_cpu_t;

typedef enum {
    ARM64_REG_X0 = 0,
    ARM64_REG_X1,
    ARM64_REG_X2,
    ARM64_REG_X3,
    ARM64_REG_X4,
    ARM64_REG_X5,
    ARM64_REG_X6,
    ARM64_REG_X7,
    ARM64_REG_X8,
    ARM64_REG_X9,
    ARM64_REG_X10,
    ARM64_REG_X11,
    ARM64_REG_X12,
    ARM64_REG_X13,
    ARM64_REG_X14,
    ARM64_REG_X15,
    ARM64_REG_X16,
    ARM64_REG_X17,
    ARM64_REG_X18,
    ARM64_REG_X19,
    ARM64_REG_X20,
    ARM64_REG_X21,
    ARM64_REG_X22,
    ARM64_REG_X23,
    ARM64_REG_X24,
    ARM64_REG_X25,
    ARM64_REG_X26,
    ARM64_REG_X27,
    ARM64_REG_X28,
    ARM64_REG_X29,
    ARM64_REG_X30,
    ARM64_REG_SP = 31,
    ARM64_REG_PC = 32
} arm64_reg_num_t;

void arm64_cpu_init(arm64_cpu_t *cpu, void *mmu);
void arm64_cpu_reset(arm64_cpu_t *cpu);
void arm64_cpu_set_reg(arm64_cpu_t *cpu, int reg, uint64_t value);
uint64_t arm64_cpu_get_reg(arm64_cpu_t *cpu, int reg);
void arm64_cpu_set_pc(arm64_cpu_t *cpu, uint64_t pc);
uint64_t arm64_cpu_get_pc(arm64_cpu_t *cpu);

#endif
