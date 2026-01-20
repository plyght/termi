#include "../../Kernel/syscall/calls.h"
#include "../mmu/mmu.h"
#include "arm64_cpu.h"
#include "decoder.h"
#include <stdio.h>
#include <stdlib.h>

static void update_flags_add(arm64_cpu_t *cpu, uint64_t a, uint64_t b, uint64_t result)
{
    cpu->pstate.n = (result >> 63) & 1;
    cpu->pstate.z = (result == 0) ? 1 : 0;
    cpu->pstate.c = (result < a) ? 1 : 0;
    cpu->pstate.v = (((a ^ result) & (b ^ result)) >> 63) & 1;
}

static void update_flags_sub(arm64_cpu_t *cpu, uint64_t a, uint64_t b, uint64_t result)
{
    cpu->pstate.n = (result >> 63) & 1;
    cpu->pstate.z = (result == 0) ? 1 : 0;
    cpu->pstate.c = (a >= b) ? 1 : 0;
    cpu->pstate.v = (((a ^ b) & (a ^ result)) >> 63) & 1;
}

static int check_condition(arm64_cpu_t *cpu, uint8_t cond)
{
    switch (cond) {
    case 0x0:
        return cpu->pstate.z == 1;
    case 0x1:
        return cpu->pstate.z == 0;
    case 0xa:
        return cpu->pstate.n == cpu->pstate.v;
    case 0xb:
        return cpu->pstate.n != cpu->pstate.v;
    case 0xc:
        return cpu->pstate.z == 0 && cpu->pstate.n == cpu->pstate.v;
    case 0xd:
        return cpu->pstate.z == 1 || cpu->pstate.n != cpu->pstate.v;
    case 0x8:
        return cpu->pstate.c == 1 && cpu->pstate.z == 0;
    case 0x9:
        return cpu->pstate.c == 0 || cpu->pstate.z == 1;
    default:
        return 1;
    }
}

static int execute_instruction(arm64_cpu_t *cpu, arm64_insn_t *insn)
{
    arm64_mmu_t *mmu = (arm64_mmu_t *)cpu->mmu;

    switch (insn->type) {
    case ARM64_INSN_ADD_IMM: {
        uint64_t val = (insn->rn == 31) ? cpu->sp : cpu->x[insn->rn];
        uint64_t result = val + (insn->imm << insn->shift);
        if (insn->rd == 31) {
            cpu->sp = result;
        } else {
            cpu->x[insn->rd] = insn->sf ? result : (uint32_t)result;
        }
        break;
    }

    case ARM64_INSN_SUB_IMM: {
        uint64_t val = (insn->rn == 31) ? cpu->sp : cpu->x[insn->rn];
        uint64_t result = val - (insn->imm << insn->shift);
        if (insn->rd == 31) {
            cpu->sp = result;
        } else {
            cpu->x[insn->rd] = insn->sf ? result : (uint32_t)result;
        }
        break;
    }

    case ARM64_INSN_ADD_REG: {
        uint64_t a = (insn->rn == 31) ? cpu->sp : cpu->x[insn->rn];
        uint64_t b = cpu->x[insn->rm];
        uint64_t result = a + b;
        if (insn->rd == 31) {
            cpu->sp = result;
        } else {
            cpu->x[insn->rd] = insn->sf ? result : (uint32_t)result;
        }
        break;
    }

    case ARM64_INSN_SUB_REG: {
        uint64_t a = (insn->rn == 31) ? cpu->sp : cpu->x[insn->rn];
        uint64_t b = cpu->x[insn->rm];
        uint64_t result = a - b;
        if (insn->rd == 31) {
            cpu->sp = result;
        } else {
            cpu->x[insn->rd] = insn->sf ? result : (uint32_t)result;
        }
        break;
    }

    case ARM64_INSN_AND_REG:
        cpu->x[insn->rd] = cpu->x[insn->rn] & cpu->x[insn->rm];
        break;

    case ARM64_INSN_ORR_REG:
        cpu->x[insn->rd] = cpu->x[insn->rn] | cpu->x[insn->rm];
        break;

    case ARM64_INSN_EOR_REG:
        cpu->x[insn->rd] = cpu->x[insn->rn] ^ cpu->x[insn->rm];
        break;

    case ARM64_INSN_AND_IMM: {
        uint64_t rn_val = (insn->rn == 31) ? 0 : cpu->x[insn->rn];
        uint64_t result = rn_val & insn->imm;
        if (insn->sf == 0) {
            result &= 0xFFFFFFFF;
        }
        if (insn->rd != 31) {
            cpu->x[insn->rd] = result;
        }
        break;
    }

    case ARM64_INSN_ORR_IMM: {
        uint64_t rn_val = (insn->rn == 31) ? 0 : cpu->x[insn->rn];
        uint64_t result = rn_val | insn->imm;
        if (insn->sf == 0) {
            result &= 0xFFFFFFFF;
        }
        if (insn->rd != 31) {
            cpu->x[insn->rd] = result;
        }
        break;
    }

    case ARM64_INSN_EOR_IMM: {
        uint64_t rn_val = (insn->rn == 31) ? 0 : cpu->x[insn->rn];
        uint64_t result = rn_val ^ insn->imm;
        if (insn->sf == 0) {
            result &= 0xFFFFFFFF;
        }
        if (insn->rd != 31) {
            cpu->x[insn->rd] = result;
        }
        break;
    }

    case ARM64_INSN_ANDS_IMM: {
        uint64_t rn_val = (insn->rn == 31) ? 0 : cpu->x[insn->rn];
        uint64_t result = rn_val & insn->imm;
        if (insn->sf == 0) {
            result &= 0xFFFFFFFF;
        }
        if (insn->rd != 31) {
            cpu->x[insn->rd] = result;
        }
        cpu->pstate.z = (result == 0) ? 1 : 0;
        cpu->pstate.n = (result >> 63) & 1;
        cpu->pstate.c = 0;
        cpu->pstate.v = 0;
        break;
    }

    case ARM64_INSN_MOVZ:
        cpu->x[insn->rd] = insn->imm << insn->shift;
        break;

    case ARM64_INSN_MOVK:
        cpu->x[insn->rd] =
            (cpu->x[insn->rd] & ~(0xffffULL << insn->shift)) | (insn->imm << insn->shift);
        break;

    case ARM64_INSN_MOV_REG:
        if (insn->rd == 31) {
            cpu->sp = cpu->x[insn->rm];
        } else if (insn->rm == 31) {
            cpu->x[insn->rd] = cpu->sp;
        } else {
            cpu->x[insn->rd] = cpu->x[insn->rm];
        }
        break;

    case ARM64_INSN_LDR_IMM: {
        uint64_t addr = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]) + insn->offset;
        uint64_t val;
        if (arm64_mmu_read_u64(mmu, addr, &val) < 0)
            return -1;
        if (insn->rt == 31) {
            cpu->sp = val;
        } else {
            cpu->x[insn->rt] = val;
        }
        break;
    }

    case ARM64_INSN_STR_IMM: {
        uint64_t addr = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]) + insn->offset;
        uint64_t val = (insn->rt == 31) ? cpu->sp : cpu->x[insn->rt];
        if (arm64_mmu_write_u64(mmu, addr, val) < 0)
            return -1;
        break;
    }

    case ARM64_INSN_STP: {
        uint64_t addr = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]) + insn->imm;
        uint64_t val1 = (insn->rt == 31) ? cpu->sp : cpu->x[insn->rt];
        uint64_t val2 = (insn->rt2 == 31) ? cpu->sp : cpu->x[insn->rt2];
        if (arm64_mmu_write_u64(mmu, addr, val1) < 0)
            return -1;
        if (arm64_mmu_write_u64(mmu, addr + 8, val2) < 0)
            return -1;
        break;
    }

    case ARM64_INSN_LDP: {
        uint64_t addr = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]) + insn->imm;
        uint64_t val1, val2;
        if (arm64_mmu_read_u64(mmu, addr, &val1) < 0)
            return -1;
        if (arm64_mmu_read_u64(mmu, addr + 8, &val2) < 0)
            return -1;
        if (insn->rt != 31)
            cpu->x[insn->rt] = val1;
        if (insn->rt2 != 31)
            cpu->x[insn->rt2] = val2;
        break;
    }

    case ARM64_INSN_B:
        cpu->pc += insn->offset;
        return 0;

    case ARM64_INSN_BL:
        cpu->x[30] = cpu->pc + 4;
        cpu->pc += insn->offset;
        return 0;

    case ARM64_INSN_BR:
        cpu->pc = cpu->x[insn->rn];
        return 0;

    case ARM64_INSN_BLR:
        cpu->x[30] = cpu->pc + 4;
        cpu->pc = cpu->x[insn->rn];
        return 0;

    case ARM64_INSN_RET:
        cpu->pc = cpu->x[insn->rn];
        return 0;

    case ARM64_INSN_BEQ:
    case ARM64_INSN_BNE:
    case ARM64_INSN_BGT:
    case ARM64_INSN_BLT:
    case ARM64_INSN_BGE:
    case ARM64_INSN_BLE:
    case ARM64_INSN_BHI:
    case ARM64_INSN_BLS:
        if (check_condition(cpu, insn->cond)) {
            cpu->pc += insn->offset;
            return 0;
        }
        break;

    case ARM64_INSN_CBZ:
        if (cpu->x[insn->rt] == 0) {
            cpu->pc += insn->offset;
            return 0;
        }
        break;

    case ARM64_INSN_CBNZ:
        if (cpu->x[insn->rt] != 0) {
            cpu->pc += insn->offset;
            return 0;
        }
        break;

    case ARM64_INSN_CMP_IMM: {
        uint64_t a = cpu->x[insn->rn];
        uint64_t b = insn->imm;
        uint64_t result = a - b;
        update_flags_sub(cpu, a, b, result);
        break;
    }

    case ARM64_INSN_CMP_REG: {
        uint64_t a = cpu->x[insn->rn];
        uint64_t b = cpu->x[insn->rm];
        uint64_t result = a - b;
        update_flags_sub(cpu, a, b, result);
        break;
    }

    case ARM64_INSN_ADRP:
        cpu->x[insn->rd] = (cpu->pc & ~0xfffULL) + insn->imm;
        break;

    case ARM64_INSN_ADR:
        cpu->x[insn->rd] = cpu->pc + insn->imm;
        break;

    case ARM64_INSN_SVC: {
        int syscall_num = cpu->x[8];
        printf("🔧 [Interpreter] SVC syscall #%d (x0=%llu, x1=%llu, x2=%llu)\n", 
               syscall_num, 
               (unsigned long long)cpu->x[0], 
               (unsigned long long)cpu->x[1],
               (unsigned long long)cpu->x[2]);
        long ret = handle_syscall(syscall_num, cpu->x[0], cpu->x[1], cpu->x[2], cpu->x[3],
                                  cpu->x[4], cpu->x[5]);
        cpu->x[0] = ret;
        printf("🔧 [Interpreter] SVC returned: %ld\n", ret);
        break;
    }

    case ARM64_INSN_BRK:
    case ARM64_INSN_HLT:
        cpu->halted = true;
        return -1;

    case ARM64_INSN_NOP:
        break;

    default:
        printf("❌ [Interpreter] Unknown instruction type %d at PC=0x%llx (raw=0x%08x)\n",
               insn->type, (unsigned long long)cpu->pc, insn->raw);
        return -1;
    }

    cpu->pc += 4;
    return 0;
}

int arm64_interpreter_step(arm64_cpu_t *cpu)
{
    if (cpu->halted) {
        return -1;
    }

    arm64_mmu_t *mmu = (arm64_mmu_t *)cpu->mmu;
    uint32_t insn_raw;

    if (arm64_mmu_read_u32(mmu, cpu->pc, &insn_raw) < 0) {
        return -1;
    }

    arm64_insn_t insn;
    if (arm64_decode(insn_raw, &insn) == 0) {
        printf("❌ [Interpreter] Failed to decode instruction 0x%08x at PC=0x%llx\n",
               insn_raw, (unsigned long long)cpu->pc);
        return -1;
    }

    return execute_instruction(cpu, &insn);
}

void arm64_interpreter_run(arm64_cpu_t *cpu, uint64_t max_insns)
{
    uint64_t count = 0;

    while (!cpu->halted && (max_insns == 0 || count < max_insns)) {
        if (arm64_interpreter_step(cpu) < 0) {
            printf("[CPU] Interpreter step failed at PC=0x%llx after %llu instructions\n",
                   (unsigned long long)cpu->pc, (unsigned long long)count);
            break;
        }
        count++;
        
        if (count % 100000 == 0) {
            printf("[CPU] Executed %llu instructions, PC=0x%llx\n",
                   (unsigned long long)count, (unsigned long long)cpu->pc);
        }
    }
    
    printf("[CPU] Execution loop finished: %llu instructions executed\n",
           (unsigned long long)count);
}
