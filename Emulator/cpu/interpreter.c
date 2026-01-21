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

#ifdef STANDALONE_TEST
    printf("[DEBUG] execute_instruction: type=%d, raw=0x%08x, PC=0x%llx\n",
           insn->type, insn->raw, (unsigned long long)cpu->pc);
#endif

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

    case ARM64_INSN_SUBS_REG: {
        uint64_t a = (insn->rn == 31) ? cpu->sp : cpu->x[insn->rn];
        uint64_t b = cpu->x[insn->rm];
        uint64_t result = a - b;
        
        cpu->pstate.n = (result >> 63) & 1;
        cpu->pstate.z = (result == 0) ? 1 : 0;
        cpu->pstate.c = (a >= b) ? 1 : 0;
        cpu->pstate.v = (((a ^ b) & (a ^ result)) >> 63) & 1;
        
        if (insn->rd != 31) {
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

    case ARM64_INSN_ORN_REG:
        cpu->x[insn->rd] = cpu->x[insn->rn] | ~cpu->x[insn->rm];
        break;

    case ARM64_INSN_BIC_REG:
        cpu->x[insn->rd] = cpu->x[insn->rn] & ~cpu->x[insn->rm];
        break;

    case ARM64_INSN_MVN_REG:
        cpu->x[insn->rd] = ~cpu->x[insn->rm];
        break;

    case ARM64_INSN_NEG_REG:
        cpu->x[insn->rd] = -(int64_t)cpu->x[insn->rm];
        if (insn->sf == 0) {
            cpu->x[insn->rd] &= 0xFFFFFFFF;
        }
        break;

    case ARM64_INSN_LSL_REG:
        cpu->x[insn->rd] = cpu->x[insn->rn] << (cpu->x[insn->rm] & 63);
        if (insn->sf == 0) {
            cpu->x[insn->rd] &= 0xFFFFFFFF;
        }
        break;

    case ARM64_INSN_LSR_REG:
        if (insn->sf) {
            cpu->x[insn->rd] = cpu->x[insn->rn] >> (cpu->x[insn->rm] & 63);
        } else {
            cpu->x[insn->rd] = (uint32_t)cpu->x[insn->rn] >> (cpu->x[insn->rm] & 31);
        }
        break;

    case ARM64_INSN_ASR_REG:
        if (insn->sf) {
            cpu->x[insn->rd] = (int64_t)cpu->x[insn->rn] >> (cpu->x[insn->rm] & 63);
        } else {
            cpu->x[insn->rd] = (int32_t)cpu->x[insn->rn] >> (cpu->x[insn->rm] & 31);
        }
        break;

    case ARM64_INSN_MUL: {
        uint64_t a = cpu->x[insn->rn];
        uint64_t b = cpu->x[insn->rm];
        uint64_t result = a * b;
        cpu->x[insn->rd] = insn->sf ? result : (uint32_t)result;
        break;
    }

    case ARM64_INSN_UDIV: {
        uint64_t a = cpu->x[insn->rn];
        uint64_t b = cpu->x[insn->rm];
        if (b == 0) {
            cpu->x[insn->rd] = 0;
        } else {
            uint64_t result = insn->sf ? (a / b) : ((uint32_t)a / (uint32_t)b);
            cpu->x[insn->rd] = insn->sf ? result : (uint32_t)result;
        }
        break;
    }

    case ARM64_INSN_SDIV: {
        int64_t a = (int64_t)cpu->x[insn->rn];
        int64_t b = (int64_t)cpu->x[insn->rm];
        if (b == 0) {
            cpu->x[insn->rd] = 0;
        } else {
            int64_t result = insn->sf ? (a / b) : ((int32_t)a / (int32_t)b);
            cpu->x[insn->rd] = insn->sf ? result : (uint32_t)result;
        }
        break;
    }

    case ARM64_INSN_MADD: {
        uint64_t a = cpu->x[insn->rn];
        uint64_t b = cpu->x[insn->rm];
        uint64_t c = cpu->x[insn->ra];
        uint64_t result = a * b + c;
        cpu->x[insn->rd] = insn->sf ? result : (uint32_t)result;
        break;
    }

    case ARM64_INSN_MSUB: {
        uint64_t a = cpu->x[insn->rn];
        uint64_t b = cpu->x[insn->rm];
        uint64_t c = cpu->x[insn->ra];
        uint64_t result = c - (a * b);
        cpu->x[insn->rd] = insn->sf ? result : (uint32_t)result;
        break;
    }

    case ARM64_INSN_TST_REG: {
        uint64_t result = cpu->x[insn->rn] & cpu->x[insn->rm];
        cpu->pstate.z = (result == 0) ? 1 : 0;
        cpu->pstate.n = (result >> 63) & 1;
        cpu->pstate.c = 0;
        cpu->pstate.v = 0;
        break;
    }

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

    case ARM64_INSN_MOVN:
        cpu->x[insn->rd] = ~(insn->imm << insn->shift);
        if (insn->sf == 0) {
            cpu->x[insn->rd] &= 0xFFFFFFFF;
        }
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

    case ARM64_INSN_LDRB_IMM: {
        uint64_t addr = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]) + insn->offset;
        uint8_t val;
        if (arm64_mmu_read_u8(mmu, addr, &val) < 0)
            return -1;
        cpu->x[insn->rt] = val;
        break;
    }

    case ARM64_INSN_LDRH_IMM: {
        uint64_t addr = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]) + insn->offset;
        uint16_t val;
        if (arm64_mmu_read_u16(mmu, addr, &val) < 0)
            return -1;
        cpu->x[insn->rt] = val;
        break;
    }

    case ARM64_INSN_LDRSB_IMM: {
        uint64_t addr = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]) + insn->offset;
        uint8_t val;
        if (arm64_mmu_read_u8(mmu, addr, &val) < 0)
            return -1;
        cpu->x[insn->rt] = (int64_t)(int8_t)val;
        break;
    }

    case ARM64_INSN_LDRSH_IMM: {
        uint64_t addr = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]) + insn->offset;
        uint16_t val;
        if (arm64_mmu_read_u16(mmu, addr, &val) < 0)
            return -1;
        cpu->x[insn->rt] = (int64_t)(int16_t)val;
        break;
    }

    case ARM64_INSN_LDRSW_IMM: {
        uint64_t addr = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]) + insn->offset;
        uint32_t val;
        if (arm64_mmu_read_u32(mmu, addr, &val) < 0)
            return -1;
        cpu->x[insn->rt] = (int64_t)(int32_t)val;
        break;
    }

    case ARM64_INSN_STR_IMM: {
        uint64_t addr = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]) + insn->offset;
        uint64_t val = (insn->rt == 31) ? cpu->sp : cpu->x[insn->rt];
        if (arm64_mmu_write_u64(mmu, addr, val) < 0)
            return -1;
        break;
    }

    case ARM64_INSN_STRB_IMM: {
        uint64_t addr = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]) + insn->offset;
        uint8_t val = (uint8_t)cpu->x[insn->rt];
        if (arm64_mmu_write_u8(mmu, addr, val) < 0)
            return -1;
        break;
    }

    case ARM64_INSN_STRH_IMM: {
        uint64_t addr = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]) + insn->offset;
        uint16_t val = (uint16_t)cpu->x[insn->rt];
        if (arm64_mmu_write_u16(mmu, addr, val) < 0)
            return -1;
        break;
    }

    case ARM64_INSN_LDR_REG: {
        uint64_t offset_reg = cpu->x[insn->rm];
        
        if (insn->option == 0x3) {
            offset_reg = (uint64_t)(int64_t)(int32_t)offset_reg;
        } else if (insn->option == 0x2) {
            offset_reg = (uint32_t)offset_reg;
        } else if (insn->option == 0x7) {
            offset_reg = (uint64_t)(int64_t)offset_reg;
        }
        
        uint64_t offset = offset_reg << insn->shift;
        uint64_t addr = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]) + offset;
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

    case ARM64_INSN_STR_REG: {
        uint64_t offset_reg = cpu->x[insn->rm];
        
        if (insn->option == 0x3) {
            offset_reg = (uint64_t)(int64_t)(int32_t)offset_reg;
        } else if (insn->option == 0x2) {
            offset_reg = (uint32_t)offset_reg;
        } else if (insn->option == 0x7) {
            offset_reg = (uint64_t)(int64_t)offset_reg;
        }
        
        uint64_t offset = offset_reg << insn->shift;
        uint64_t addr = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]) + offset;
        uint64_t val = (insn->rt == 31) ? cpu->sp : cpu->x[insn->rt];
        if (arm64_mmu_write_u64(mmu, addr, val) < 0)
            return -1;
        break;
    }

    case ARM64_INSN_LDR_VREG: {
#ifdef STANDALONE_TEST
        printf("[DEBUG] Executing LDR_VREG: rm=%d, rn=%d, rt=%d, option=%d, shift=%d\n",
               insn->rm, insn->rn, insn->rt, insn->option, insn->shift);
        printf("[DEBUG] Before reading offset_reg: X%d = 0x%llx\n", 
               insn->rm, (unsigned long long)cpu->x[insn->rm]);
#endif
        uint64_t offset_reg = cpu->x[insn->rm];
#ifdef STANDALONE_TEST
        printf("[DEBUG] offset_reg from X%d = 0x%llx\n", 
               insn->rm, (unsigned long long)offset_reg);
#endif
        
        if (insn->option == 0x3) {
            offset_reg = (uint64_t)(int64_t)(int32_t)offset_reg;
        } else if (insn->option == 0x2) {
            offset_reg = (uint32_t)offset_reg;
        } else if (insn->option == 0x7) {
            offset_reg = (uint64_t)(int64_t)offset_reg;
        }
        
        uint64_t offset = offset_reg << insn->shift;
        uint64_t addr = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]) + offset;
#ifdef STANDALONE_TEST
        printf("[DEBUG] LDR_VREG: offset_reg=0x%llx, offset=0x%llx, addr=0x%llx\n",
               (unsigned long long)offset_reg, (unsigned long long)offset, (unsigned long long)addr);
#endif
        uint64_t val;
        if (arm64_mmu_read_u64(mmu, addr, &val) < 0) {
#ifdef STANDALONE_TEST
            printf("[DEBUG] LDR_VREG: MMU read failed at addr=0x%llx\n", (unsigned long long)addr);
#endif
            return -1;
        }
        cpu->v[insn->rt].d[0] = val;
        cpu->v[insn->rt].d[1] = 0;
#ifdef STANDALONE_TEST
        printf("[DEBUG] LDR_VREG complete: V%d = 0x%llx\n", insn->rt, (unsigned long long)val);
#endif
        break;
    }

    case ARM64_INSN_STR_VREG: {
        uint64_t offset_reg = cpu->x[insn->rm];
        
        if (insn->option == 0x3) {
            offset_reg = (uint64_t)(int64_t)(int32_t)offset_reg;
        } else if (insn->option == 0x2) {
            offset_reg = (uint32_t)offset_reg;
        } else if (insn->option == 0x7) {
            offset_reg = (uint64_t)(int64_t)offset_reg;
        }
        
        uint64_t offset = offset_reg << insn->shift;
        uint64_t addr = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]) + offset;
        uint64_t val = cpu->v[insn->rt].d[0];
        if (arm64_mmu_write_u64(mmu, addr, val) < 0)
            return -1;
        break;
    }

    case ARM64_INSN_LDUR: {
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

    case ARM64_INSN_LDURB: {
        uint64_t addr = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]) + insn->offset;
        uint8_t val;
        if (arm64_mmu_read_u8(mmu, addr, &val) < 0)
            return -1;
        cpu->x[insn->rt] = val;
        break;
    }

    case ARM64_INSN_LDURH: {
        uint64_t addr = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]) + insn->offset;
        uint16_t val;
        if (arm64_mmu_read_u16(mmu, addr, &val) < 0)
            return -1;
        cpu->x[insn->rt] = val;
        break;
    }

    case ARM64_INSN_LDURSB: {
        uint64_t addr = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]) + insn->offset;
        uint8_t val;
        if (arm64_mmu_read_u8(mmu, addr, &val) < 0)
            return -1;
        cpu->x[insn->rt] = (int64_t)(int8_t)val;
        break;
    }

    case ARM64_INSN_LDURSH: {
        uint64_t addr = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]) + insn->offset;
        uint16_t val;
        if (arm64_mmu_read_u16(mmu, addr, &val) < 0)
            return -1;
        cpu->x[insn->rt] = (int64_t)(int16_t)val;
        break;
    }

    case ARM64_INSN_LDURSW: {
        uint64_t addr = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]) + insn->offset;
        uint32_t val;
        if (arm64_mmu_read_u32(mmu, addr, &val) < 0)
            return -1;
        cpu->x[insn->rt] = (int64_t)(int32_t)val;
        break;
    }

    case ARM64_INSN_STUR: {
        uint64_t addr = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]) + insn->offset;
        uint64_t val = (insn->rt == 31) ? cpu->sp : cpu->x[insn->rt];
        if (arm64_mmu_write_u64(mmu, addr, val) < 0)
            return -1;
        break;
    }

    case ARM64_INSN_STURB: {
        uint64_t addr = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]) + insn->offset;
        uint8_t val = (uint8_t)cpu->x[insn->rt];
        if (arm64_mmu_write_u8(mmu, addr, val) < 0)
            return -1;
        break;
    }

    case ARM64_INSN_STURH: {
        uint64_t addr = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]) + insn->offset;
        uint16_t val = (uint16_t)cpu->x[insn->rt];
        if (arm64_mmu_write_u16(mmu, addr, val) < 0)
            return -1;
        break;
    }

    case ARM64_INSN_LDUR_PRE: {
        uint64_t base = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]);
        uint64_t addr = base + insn->offset;
        if (insn->rn == 31) {
            cpu->sp = addr;
        } else {
            cpu->x[insn->rn] = addr;
        }
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

    case ARM64_INSN_LDUR_POST: {
#ifdef STANDALONE_TEST
        printf("[DEBUG] Executing LDUR_POST: rn=%d, rt=%d, offset=%ld\n", insn->rn, insn->rt, insn->offset);
#endif
        uint64_t base = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]);
#ifdef STANDALONE_TEST
        printf("[DEBUG] base=0x%llx\n", (unsigned long long)base);
#endif
        uint64_t val;
        if (arm64_mmu_read_u64(mmu, base, &val) < 0) {
#ifdef STANDALONE_TEST
            printf("[DEBUG] MMU read failed!\n");
#endif
            return -1;
        }
#ifdef STANDALONE_TEST
        printf("[DEBUG] Read val=0x%llx\n", (unsigned long long)val);
#endif
        if (insn->rt == 31) {
            cpu->sp = val;
        } else {
            cpu->x[insn->rt] = val;
        }
        uint64_t new_base = base + insn->offset;
        if (insn->rn == 31) {
            cpu->sp = new_base;
        } else {
            cpu->x[insn->rn] = new_base;
        }
#ifdef STANDALONE_TEST
        printf("[DEBUG] LDUR_POST complete, new_base=0x%llx, X%d now = 0x%llx\n", 
               (unsigned long long)new_base, insn->rt, (unsigned long long)cpu->x[insn->rt]);
        printf("[DEBUG] Register dump: X0=0x%llx X1=0x%llx X2=0x%llx X3=0x%llx\n",
               (unsigned long long)cpu->x[0], (unsigned long long)cpu->x[1],
               (unsigned long long)cpu->x[2], (unsigned long long)cpu->x[3]);
#endif
        break;
    }

    case ARM64_INSN_STUR_PRE: {
        uint64_t base = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]);
        uint64_t addr = base + insn->offset;
        if (insn->rn == 31) {
            cpu->sp = addr;
        } else {
            cpu->x[insn->rn] = addr;
        }
        uint64_t val = (insn->rt == 31) ? cpu->sp : cpu->x[insn->rt];
        if (arm64_mmu_write_u64(mmu, addr, val) < 0)
            return -1;
        break;
    }

    case ARM64_INSN_STUR_POST: {
        uint64_t base = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]);
        uint64_t val = (insn->rt == 31) ? cpu->sp : cpu->x[insn->rt];
        if (arm64_mmu_write_u64(mmu, base, val) < 0)
            return -1;
        uint64_t new_base = base + insn->offset;
        if (insn->rn == 31) {
            cpu->sp = new_base;
        } else {
            cpu->x[insn->rn] = new_base;
        }
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

    case ARM64_INSN_LDR_PRE: {
        uint64_t base = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]);
        uint64_t addr = base + insn->offset;
        if (insn->rn == 31) {
            cpu->sp = addr;
        } else {
            cpu->x[insn->rn] = addr;
        }
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

    case ARM64_INSN_LDR_POST: {
        uint64_t base = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]);
        uint64_t val;
        if (arm64_mmu_read_u64(mmu, base, &val) < 0)
            return -1;
        if (insn->rt == 31) {
            cpu->sp = val;
        } else {
            cpu->x[insn->rt] = val;
        }
        uint64_t new_base = base + insn->offset;
        if (insn->rn == 31) {
            cpu->sp = new_base;
        } else {
            cpu->x[insn->rn] = new_base;
        }
        break;
    }

    case ARM64_INSN_STR_PRE: {
        uint64_t base = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]);
        uint64_t addr = base + insn->offset;
        if (insn->rn == 31) {
            cpu->sp = addr;
        } else {
            cpu->x[insn->rn] = addr;
        }
        uint64_t val = (insn->rt == 31) ? cpu->sp : cpu->x[insn->rt];
        if (arm64_mmu_write_u64(mmu, addr, val) < 0)
            return -1;
        break;
    }

    case ARM64_INSN_STR_POST: {
        uint64_t base = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]);
        uint64_t val = (insn->rt == 31) ? cpu->sp : cpu->x[insn->rt];
        if (arm64_mmu_write_u64(mmu, base, val) < 0)
            return -1;
        uint64_t new_base = base + insn->offset;
        if (insn->rn == 31) {
            cpu->sp = new_base;
        } else {
            cpu->x[insn->rn] = new_base;
        }
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

    case ARM64_INSN_LDP_PRE: {
        uint64_t base = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]);
        uint64_t addr = base + insn->imm;
        if (insn->rn == 31) {
            cpu->sp = addr;
        } else {
            cpu->x[insn->rn] = addr;
        }
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

    case ARM64_INSN_LDP_POST: {
        uint64_t base = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]);
        uint64_t val1, val2;
        if (arm64_mmu_read_u64(mmu, base, &val1) < 0)
            return -1;
        if (arm64_mmu_read_u64(mmu, base + 8, &val2) < 0)
            return -1;
        if (insn->rt != 31)
            cpu->x[insn->rt] = val1;
        if (insn->rt2 != 31)
            cpu->x[insn->rt2] = val2;
        uint64_t new_base = base + insn->imm;
        if (insn->rn == 31) {
            cpu->sp = new_base;
        } else {
            cpu->x[insn->rn] = new_base;
        }
        break;
    }

    case ARM64_INSN_STP_PRE: {
        uint64_t base = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]);
        uint64_t addr = base + insn->imm;
        if (insn->rn == 31) {
            cpu->sp = addr;
        } else {
            cpu->x[insn->rn] = addr;
        }
        uint64_t val1 = (insn->rt == 31) ? cpu->sp : cpu->x[insn->rt];
        uint64_t val2 = (insn->rt2 == 31) ? cpu->sp : cpu->x[insn->rt2];
        if (arm64_mmu_write_u64(mmu, addr, val1) < 0)
            return -1;
        if (arm64_mmu_write_u64(mmu, addr + 8, val2) < 0)
            return -1;
        break;
    }

    case ARM64_INSN_STP_POST: {
        uint64_t base = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]);
        uint64_t val1 = (insn->rt == 31) ? cpu->sp : cpu->x[insn->rt];
        uint64_t val2 = (insn->rt2 == 31) ? cpu->sp : cpu->x[insn->rt2];
        if (arm64_mmu_write_u64(mmu, base, val1) < 0)
            return -1;
        if (arm64_mmu_write_u64(mmu, base + 8, val2) < 0)
            return -1;
        uint64_t new_base = base + insn->imm;
        if (insn->rn == 31) {
            cpu->sp = new_base;
        } else {
            cpu->x[insn->rn] = new_base;
        }
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

    case ARM64_INSN_TBZ:
        if ((cpu->x[insn->rt] & (1ULL << insn->imm)) == 0) {
            cpu->pc += insn->offset;
            return 0;
        }
        break;

    case ARM64_INSN_TBNZ:
        if ((cpu->x[insn->rt] & (1ULL << insn->imm)) != 0) {
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

    case ARM64_INSN_CSEL:
        if (check_condition(cpu, insn->cond)) {
            cpu->x[insn->rd] = cpu->x[insn->rn];
        } else {
            cpu->x[insn->rd] = cpu->x[insn->rm];
        }
        break;

    case ARM64_INSN_CSINC:
        if (check_condition(cpu, insn->cond)) {
            cpu->x[insn->rd] = cpu->x[insn->rn];
        } else {
            cpu->x[insn->rd] = cpu->x[insn->rm] + 1;
        }
        break;

    case ARM64_INSN_CSINV:
        if (check_condition(cpu, insn->cond)) {
            cpu->x[insn->rd] = cpu->x[insn->rn];
        } else {
            cpu->x[insn->rd] = ~cpu->x[insn->rm];
        }
        break;

    case ARM64_INSN_CSNEG:
        if (check_condition(cpu, insn->cond)) {
            cpu->x[insn->rd] = cpu->x[insn->rn];
        } else {
            cpu->x[insn->rd] = -(int64_t)cpu->x[insn->rm];
        }
        break;

    case ARM64_INSN_ADRP:
        cpu->x[insn->rd] = (cpu->pc & ~0xfffULL) + insn->imm;
        break;

    case ARM64_INSN_ADR:
        cpu->x[insn->rd] = cpu->pc + insn->imm;
        break;

    case ARM64_INSN_UBFM: {
        uint64_t src = cpu->x[insn->rn];
        uint8_t immr = insn->imm;
        uint8_t imms = insn->shift;
        uint8_t width = insn->sf ? 64 : 32;

        uint64_t wmask = (imms < width - 1) ? ((1ULL << (imms + 1)) - 1) : ~0ULL;
        uint64_t result = (src >> immr) & wmask;

        if (insn->sf == 0) {
            result &= 0xFFFFFFFF;
        }
        cpu->x[insn->rd] = result;
        break;
    }

    case ARM64_INSN_SBFM: {
        uint64_t src = cpu->x[insn->rn];
        uint8_t immr = insn->imm;
        uint8_t imms = insn->shift;
        uint8_t width = insn->sf ? 64 : 32;

        uint64_t wmask = (imms < width - 1) ? ((1ULL << (imms + 1)) - 1) : ~0ULL;
        uint64_t result = (src >> immr) & wmask;

        if (imms < width - 1 && (result & (1ULL << imms))) {
            result |= ~wmask;
        }

        if (insn->sf == 0) {
            result = (int32_t)result;
        }
        cpu->x[insn->rd] = result;
        break;
    }

    case ARM64_INSN_BFM: {
        uint64_t dst = cpu->x[insn->rd];
        uint64_t src = cpu->x[insn->rn];
        uint8_t immr = insn->imm;
        uint8_t imms = insn->shift;
        uint8_t width = insn->sf ? 64 : 32;

        uint64_t wmask = (imms < width - 1) ? ((1ULL << (imms + 1)) - 1) : ~0ULL;
        uint64_t tmask = wmask;

        if (immr != 0) {
            tmask = (tmask >> immr) | (tmask << (width - immr));
            tmask &= (width == 64) ? ~0ULL : ((1ULL << width) - 1);
        }

        uint64_t result = dst & ~tmask;
        result |= (src << (width - immr)) & tmask;

        if (insn->sf == 0) {
            result &= 0xFFFFFFFF;
        }
        cpu->x[insn->rd] = result;
        break;
    }

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

    case ARM64_INSN_CMN_IMM: {
        uint64_t a = cpu->x[insn->rn];
        uint64_t b = insn->imm;
        uint64_t result = a + b;
        update_flags_add(cpu, a, b, result);
        break;
    }

    case ARM64_INSN_CMN_REG: {
        uint64_t a = cpu->x[insn->rn];
        uint64_t b = cpu->x[insn->rm];
        uint64_t result = a + b;
        update_flags_add(cpu, a, b, result);
        break;
    }

    case ARM64_INSN_ADDS_IMM: {
        uint64_t a = cpu->x[insn->rn];
        uint64_t b = insn->imm;
        uint64_t result = a + b;
        update_flags_add(cpu, a, b, result);
        if (insn->rd != 31) {
            cpu->x[insn->rd] = insn->sf ? result : (uint32_t)result;
        }
        break;
    }

    case ARM64_INSN_ADDS_REG: {
        uint64_t a = cpu->x[insn->rn];
        uint64_t b = cpu->x[insn->rm];
        uint64_t result = a + b;
        update_flags_add(cpu, a, b, result);
        if (insn->rd != 31) {
            cpu->x[insn->rd] = insn->sf ? result : (uint32_t)result;
        }
        break;
    }

    case ARM64_INSN_SUBS_IMM: {
        uint64_t a = cpu->x[insn->rn];
        uint64_t b = insn->imm;
        uint64_t result = a - b;
        update_flags_sub(cpu, a, b, result);
        if (insn->rd != 31) {
            cpu->x[insn->rd] = insn->sf ? result : (uint32_t)result;
        }
        break;
    }

    case ARM64_INSN_ANDS_REG: {
        uint64_t result = cpu->x[insn->rn] & cpu->x[insn->rm];
        cpu->pstate.z = (result == 0) ? 1 : 0;
        cpu->pstate.n = (result >> 63) & 1;
        cpu->pstate.c = 0;
        cpu->pstate.v = 0;
        if (insn->rd != 31) {
            cpu->x[insn->rd] = result;
        }
        break;
    }

    case ARM64_INSN_SBC: {
        uint64_t a = cpu->x[insn->rn];
        uint64_t b = cpu->x[insn->rm];
        uint64_t result = a - b - (1 - cpu->pstate.c);
        cpu->x[insn->rd] = insn->sf ? result : (uint32_t)result;
        break;
    }

    case ARM64_INSN_ADC: {
        uint64_t a = cpu->x[insn->rn];
        uint64_t b = cpu->x[insn->rm];
        uint64_t result = a + b + cpu->pstate.c;
        cpu->x[insn->rd] = insn->sf ? result : (uint32_t)result;
        break;
    }

    case ARM64_INSN_REV: {
        uint64_t val = cpu->x[insn->rn];
        if (insn->sf) {
            cpu->x[insn->rd] = __builtin_bswap64(val);
        } else {
            cpu->x[insn->rd] = __builtin_bswap32((uint32_t)val);
        }
        break;
    }

    case ARM64_INSN_REV16: {
        uint64_t val = cpu->x[insn->rn];
        uint64_t result = 0;
        if (insn->sf) {
            result = ((val & 0x00FF00FF00FF00FFULL) << 8) | ((val & 0xFF00FF00FF00FF00ULL) >> 8);
        } else {
            result = ((val & 0x00FF00FFULL) << 8) | ((val & 0xFF00FF00ULL) >> 8);
        }
        cpu->x[insn->rd] = result;
        break;
    }

    case ARM64_INSN_CLZ: {
        uint64_t val = cpu->x[insn->rn];
        if (insn->sf) {
            cpu->x[insn->rd] = val ? __builtin_clzll(val) : 64;
        } else {
            cpu->x[insn->rd] = val ? __builtin_clz((uint32_t)val) : 32;
        }
        break;
    }

    case ARM64_INSN_RBIT: {
        uint64_t val = cpu->x[insn->rn];
        uint64_t result = 0;
        int bits = insn->sf ? 64 : 32;
        for (int i = 0; i < bits; i++) {
            if (val & (1ULL << i)) {
                result |= (1ULL << (bits - 1 - i));
            }
        }
        cpu->x[insn->rd] = result;
        break;
    }

    case ARM64_INSN_SMADDL: {
        int64_t a = (int32_t)cpu->x[insn->rn];
        int64_t b = (int32_t)cpu->x[insn->rm];
        int64_t c = cpu->x[insn->ra];
        cpu->x[insn->rd] = a * b + c;
        break;
    }

    case ARM64_INSN_UMULL: {
        uint64_t a = (uint32_t)cpu->x[insn->rn];
        uint64_t b = (uint32_t)cpu->x[insn->rm];
        cpu->x[insn->rd] = a * b;
        break;
    }

    case ARM64_INSN_SMULL: {
        int64_t a = (int32_t)cpu->x[insn->rn];
        int64_t b = (int32_t)cpu->x[insn->rm];
        cpu->x[insn->rd] = a * b;
        break;
    }

    case ARM64_INSN_SMULH: {
        __int128 a = (int64_t)cpu->x[insn->rn];
        __int128 b = (int64_t)cpu->x[insn->rm];
        __int128 result = a * b;
        cpu->x[insn->rd] = (uint64_t)(result >> 64);
        break;
    }

    case ARM64_INSN_UMULH: {
        __uint128_t a = cpu->x[insn->rn];
        __uint128_t b = cpu->x[insn->rm];
        __uint128_t result = a * b;
        cpu->x[insn->rd] = (uint64_t)(result >> 64);
        break;
    }

    case ARM64_INSN_CCMP_IMM: {
        if (check_condition(cpu, insn->cond)) {
            uint64_t a = cpu->x[insn->rn];
            uint64_t b = insn->imm;
            uint64_t result = a - b;
            update_flags_sub(cpu, a, b, result);
        } else {
            cpu->pstate.n = (insn->nzcv >> 3) & 1;
            cpu->pstate.z = (insn->nzcv >> 2) & 1;
            cpu->pstate.c = (insn->nzcv >> 1) & 1;
            cpu->pstate.v = insn->nzcv & 1;
        }
        break;
    }

    case ARM64_INSN_CCMP_REG: {
        if (check_condition(cpu, insn->cond)) {
            uint64_t a = cpu->x[insn->rn];
            uint64_t b = cpu->x[insn->imm];
            uint64_t result = a - b;
            update_flags_sub(cpu, a, b, result);
        } else {
            cpu->pstate.n = (insn->nzcv >> 3) & 1;
            cpu->pstate.z = (insn->nzcv >> 2) & 1;
            cpu->pstate.c = (insn->nzcv >> 1) & 1;
            cpu->pstate.v = insn->nzcv & 1;
        }
        break;
    }

    case ARM64_INSN_CCMN_IMM: {
        if (check_condition(cpu, insn->cond)) {
            uint64_t a = cpu->x[insn->rn];
            uint64_t b = insn->imm;
            uint64_t result = a + b;
            update_flags_add(cpu, a, b, result);
        } else {
            cpu->pstate.n = (insn->nzcv >> 3) & 1;
            cpu->pstate.z = (insn->nzcv >> 2) & 1;
            cpu->pstate.c = (insn->nzcv >> 1) & 1;
            cpu->pstate.v = insn->nzcv & 1;
        }
        break;
    }

    case ARM64_INSN_CCMN_REG: {
        if (check_condition(cpu, insn->cond)) {
            uint64_t a = cpu->x[insn->rn];
            uint64_t b = cpu->x[insn->imm];
            uint64_t result = a + b;
            update_flags_add(cpu, a, b, result);
        } else {
            cpu->pstate.n = (insn->nzcv >> 3) & 1;
            cpu->pstate.z = (insn->nzcv >> 2) & 1;
            cpu->pstate.c = (insn->nzcv >> 1) & 1;
            cpu->pstate.v = insn->nzcv & 1;
        }
        break;
    }

    case ARM64_INSN_LDAXR:
    case ARM64_INSN_LDXR: {
        uint64_t addr = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]);
        uint64_t val;
        if (insn->sf) {
            if (arm64_mmu_read_u64(mmu, addr, &val) < 0)
                return -1;
        } else {
            uint32_t val32;
            if (arm64_mmu_read_u32(mmu, addr, &val32) < 0)
                return -1;
            val = val32;
        }
        cpu->x[insn->rt] = val;
        cpu->exclusive_addr = addr;
        break;
    }

    case ARM64_INSN_LDAXRB: {
        uint64_t addr = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]);
        uint8_t val;
        if (arm64_mmu_read_u8(mmu, addr, &val) < 0)
            return -1;
        cpu->x[insn->rt] = val;
        cpu->exclusive_addr = addr;
        break;
    }

    case ARM64_INSN_LDAXRH: {
        uint64_t addr = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]);
        uint16_t val;
        if (arm64_mmu_read_u16(mmu, addr, &val) < 0)
            return -1;
        cpu->x[insn->rt] = val;
        cpu->exclusive_addr = addr;
        break;
    }

    case ARM64_INSN_STLXR:
    case ARM64_INSN_STXR: {
        uint64_t addr = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]);
        uint64_t val = cpu->x[insn->rt];
        if (insn->sf) {
            if (arm64_mmu_write_u64(mmu, addr, val) < 0)
                return -1;
        } else {
            if (arm64_mmu_write_u32(mmu, addr, (uint32_t)val) < 0)
                return -1;
        }
        cpu->x[insn->rm] = 0;
        break;
    }

    case ARM64_INSN_STLXRB: {
        uint64_t addr = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]);
        uint8_t val = (uint8_t)cpu->x[insn->rt];
        if (arm64_mmu_write_u8(mmu, addr, val) < 0)
            return -1;
        cpu->x[insn->rm] = 0;
        break;
    }

    case ARM64_INSN_STLXRH: {
        uint64_t addr = (insn->rn == 31 ? cpu->sp : cpu->x[insn->rn]);
        uint16_t val = (uint16_t)cpu->x[insn->rt];
        if (arm64_mmu_write_u16(mmu, addr, val) < 0)
            return -1;
        cpu->x[insn->rm] = 0;
        break;
    }

    case ARM64_INSN_MRS: {
        uint16_t sysreg = insn->imm;
        if (sysreg == 0x5e82) {
            cpu->x[insn->rd] = 0;
        } else if (sysreg == 0x5e80) {
            cpu->x[insn->rd] = 4;
        } else if (sysreg == 0x5e81) {
            cpu->x[insn->rd] = 0x8444c004;
        } else {
            cpu->x[insn->rd] = 0;
        }
        break;
    }

    case ARM64_INSN_MSR:
        break;

    case ARM64_INSN_DMB:
    case ARM64_INSN_DSB:
    case ARM64_INSN_ISB:
        break;

    case ARM64_INSN_FADD: {
        double a = *(double*)&cpu->v[insn->vn].d[0];
        double b = *(double*)&cpu->v[insn->vm].d[0];
        double result = a + b;
        cpu->v[insn->vd].d[0] = *(uint64_t*)&result;
        cpu->v[insn->vd].d[1] = 0;
        break;
    }

    case ARM64_INSN_FSUB: {
        double a = *(double*)&cpu->v[insn->vn].d[0];
        double b = *(double*)&cpu->v[insn->vm].d[0];
        double result = a - b;
        cpu->v[insn->vd].d[0] = *(uint64_t*)&result;
        cpu->v[insn->vd].d[1] = 0;
        break;
    }

    case ARM64_INSN_FMUL: {
        double a = *(double*)&cpu->v[insn->vn].d[0];
        double b = *(double*)&cpu->v[insn->vm].d[0];
        double result = a * b;
        cpu->v[insn->vd].d[0] = *(uint64_t*)&result;
        cpu->v[insn->vd].d[1] = 0;
        break;
    }

    case ARM64_INSN_FDIV: {
        double a = *(double*)&cpu->v[insn->vn].d[0];
        double b = *(double*)&cpu->v[insn->vm].d[0];
        double result = a / b;
        cpu->v[insn->vd].d[0] = *(uint64_t*)&result;
        cpu->v[insn->vd].d[1] = 0;
        break;
    }

    case ARM64_INSN_FNEG: {
        double a = *(double*)&cpu->v[insn->vn].d[0];
        double result = -a;
        cpu->v[insn->vd].d[0] = *(uint64_t*)&result;
        cpu->v[insn->vd].d[1] = 0;
        break;
    }

    case ARM64_INSN_FABS: {
        double a = *(double*)&cpu->v[insn->vn].d[0];
        double result = a < 0 ? -a : a;
        cpu->v[insn->vd].d[0] = *(uint64_t*)&result;
        cpu->v[insn->vd].d[1] = 0;
        break;
    }

    case ARM64_INSN_FCMP:
    case ARM64_INSN_FCMPE: {
        double a = *(double*)&cpu->v[insn->vn].d[0];
        double b = *(double*)&cpu->v[insn->vm].d[0];
        cpu->pstate.n = (a < b) ? 1 : 0;
        cpu->pstate.z = (a == b) ? 1 : 0;
        cpu->pstate.c = (a >= b) ? 1 : 0;
        cpu->pstate.v = 0;
        break;
    }

    case ARM64_INSN_FCSEL: {
        if (check_condition(cpu, insn->cond)) {
            cpu->v[insn->vd].d[0] = cpu->v[insn->vn].d[0];
        } else {
            cpu->v[insn->vd].d[0] = cpu->v[insn->vm].d[0];
        }
        cpu->v[insn->vd].d[1] = 0;
        break;
    }

    case ARM64_INSN_FCVT:
        cpu->v[insn->vd].d[0] = cpu->v[insn->vn].d[0];
        cpu->v[insn->vd].d[1] = 0;
        break;

    case ARM64_INSN_FCVTZS: {
        double a = *(double*)&cpu->v[insn->vn].d[0];
        if (insn->sf) {
            cpu->x[insn->rd] = (int64_t)a;
        } else {
            cpu->x[insn->rd] = (int32_t)a;
        }
        break;
    }

    case ARM64_INSN_SCVTF: {
        if (insn->sf) {
            double result = (double)(int64_t)cpu->x[insn->rn];
            cpu->v[insn->vd].d[0] = *(uint64_t*)&result;
        } else {
            double result = (double)(int32_t)cpu->x[insn->rn];
            cpu->v[insn->vd].d[0] = *(uint64_t*)&result;
        }
        cpu->v[insn->vd].d[1] = 0;
        break;
    }

    case ARM64_INSN_MOVI: {
        cpu->v[insn->vd].d[0] = insn->imm;
        cpu->v[insn->vd].d[1] = insn->imm;
        break;
    }

    case ARM64_INSN_DUP: {
        uint64_t val = cpu->v[insn->vn].d[0];
        cpu->v[insn->vd].d[0] = val;
        cpu->v[insn->vd].d[1] = val;
        break;
    }

    case ARM64_INSN_FMOV_REG: {
        cpu->v[insn->vd].d[0] = cpu->v[insn->vn].d[0];
        cpu->v[insn->vd].d[1] = 0;
        break;
    }

    case ARM64_INSN_FMOV_GPR_TO_VEC: {
        cpu->v[insn->vd].d[0] = cpu->x[insn->rn];
        cpu->v[insn->vd].d[1] = 0;
        break;
    }

    case ARM64_INSN_FMOV_VEC_TO_GPR: {
        cpu->x[insn->rd] = cpu->v[insn->vn].d[0];
        break;
    }

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
