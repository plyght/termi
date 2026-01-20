#ifndef TERMI_ARM64_DECODER_H
#define TERMI_ARM64_DECODER_H

#include <stdint.h>

typedef enum {
    ARM64_INSN_UNKNOWN = 0,

    ARM64_INSN_ADD_IMM,
    ARM64_INSN_SUB_IMM,
    ARM64_INSN_ADD_REG,
    ARM64_INSN_SUB_REG,
    ARM64_INSN_AND_REG,
    ARM64_INSN_ORR_REG,
    ARM64_INSN_EOR_REG,
    ARM64_INSN_LSL_REG,
    ARM64_INSN_LSR_REG,
    ARM64_INSN_ASR_REG,
    ARM64_INSN_MUL,
    ARM64_INSN_UDIV,
    ARM64_INSN_SDIV,

    ARM64_INSN_MOV_REG,
    ARM64_INSN_MOVZ,
    ARM64_INSN_MOVK,
    ARM64_INSN_MOVN,

    ARM64_INSN_LDR_IMM,
    ARM64_INSN_LDRB_IMM,
    ARM64_INSN_LDRH_IMM,
    ARM64_INSN_LDRSB_IMM,
    ARM64_INSN_LDRSH_IMM,
    ARM64_INSN_LDRSW_IMM,
    ARM64_INSN_STR_IMM,
    ARM64_INSN_STRB_IMM,
    ARM64_INSN_STRH_IMM,

    ARM64_INSN_LDR_REG,
    ARM64_INSN_STR_REG,

    ARM64_INSN_LDP,
    ARM64_INSN_STP,

    ARM64_INSN_B,
    ARM64_INSN_BL,
    ARM64_INSN_BR,
    ARM64_INSN_BLR,
    ARM64_INSN_RET,

    ARM64_INSN_BEQ,
    ARM64_INSN_BNE,
    ARM64_INSN_BGT,
    ARM64_INSN_BLT,
    ARM64_INSN_BGE,
    ARM64_INSN_BLE,
    ARM64_INSN_BHI,
    ARM64_INSN_BLS,

    ARM64_INSN_CBZ,
    ARM64_INSN_CBNZ,
    ARM64_INSN_TBZ,
    ARM64_INSN_TBNZ,

    ARM64_INSN_CMP_IMM,
    ARM64_INSN_CMP_REG,
    ARM64_INSN_TST_REG,

    ARM64_INSN_CSEL,
    ARM64_INSN_CSINC,
    ARM64_INSN_CSINV,
    ARM64_INSN_CSNEG,

    ARM64_INSN_ADRP,
    ARM64_INSN_ADR,

    ARM64_INSN_SVC,
    ARM64_INSN_BRK,
    ARM64_INSN_HLT,

    ARM64_INSN_NOP,
} arm64_insn_type_t;

typedef struct {
    arm64_insn_type_t type;
    uint32_t raw;

    uint8_t rd;
    uint8_t rn;
    uint8_t rm;
    uint8_t ra;
    uint8_t rt;
    uint8_t rt2;

    uint64_t imm;
    uint8_t shift;

    uint8_t sf;
    uint8_t cond;

    int64_t offset;
} arm64_insn_t;

int arm64_decode(uint32_t insn, arm64_insn_t *decoded);

#endif
