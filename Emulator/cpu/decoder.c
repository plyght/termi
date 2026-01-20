#include "decoder.h"
#include <string.h>

#define BITS(insn, hi, lo) (((insn) >> (lo)) & ((1u << ((hi) - (lo) + 1)) - 1))
#define BIT(insn, bit) (((insn) >> (bit)) & 1)
#define SEXT(val, bits)                                                                            \
    ((int64_t)((val) | (((val) & (1ull << ((bits) - 1))) ? ~((1ull << (bits)) - 1) : 0)))

static int decode_data_proc_imm(uint32_t insn, arm64_insn_t *d)
{
    uint8_t op0 = BITS(insn, 25, 23);

    if (op0 == 0x2) {
        d->sf = BIT(insn, 31);
        d->rd = BITS(insn, 4, 0);
        d->rn = BITS(insn, 9, 5);
        uint8_t opc = BIT(insn, 30);
        d->imm = BITS(insn, 21, 10);
        d->shift = BITS(insn, 23, 22) * 12;

        if (opc == 0) {
            d->type = ARM64_INSN_ADD_IMM;
        } else {
            d->type = ARM64_INSN_SUB_IMM;
        }
        return 1;
    }

    if (op0 == 0x5) {
        d->sf = BIT(insn, 31);
        uint8_t opc = BITS(insn, 30, 29);
        d->rd = BITS(insn, 4, 0);
        d->imm = BITS(insn, 20, 5);
        d->shift = BITS(insn, 22, 21) * 16;

        if (opc == 0x2) {
            d->type = ARM64_INSN_MOVZ;
        } else if (opc == 0x3) {
            d->type = ARM64_INSN_MOVK;
        } else if (opc == 0x0) {
            d->type = ARM64_INSN_MOVN;
        } else {
            return 0;
        }
        return 1;
    }

    if ((insn & 0x1f000000) == 0x10000000) {
        d->rd = BITS(insn, 4, 0);
        int64_t immlo = BITS(insn, 30, 29);
        int64_t immhi = BITS(insn, 23, 5);
        d->imm = (immhi << 2) | immlo;

        if (BIT(insn, 31)) {
            d->type = ARM64_INSN_ADRP;
            d->imm = SEXT(d->imm, 21) << 12;
        } else {
            d->type = ARM64_INSN_ADR;
            d->imm = SEXT(d->imm, 21);
        }
        return 1;
    }

    return 0;
}

static int decode_branch(uint32_t insn, arm64_insn_t *d)
{
    if ((insn & 0xfc000000) == 0x14000000) {
        d->type = BIT(insn, 31) ? ARM64_INSN_BL : ARM64_INSN_B;
        d->offset = SEXT(BITS(insn, 25, 0), 26) << 2;
        return 1;
    }

    if ((insn & 0xff000010) == 0x54000000) {
        d->cond = BITS(insn, 3, 0);
        d->offset = SEXT(BITS(insn, 23, 5), 19) << 2;

        switch (d->cond) {
        case 0x0:
            d->type = ARM64_INSN_BEQ;
            break;
        case 0x1:
            d->type = ARM64_INSN_BNE;
            break;
        case 0xa:
            d->type = ARM64_INSN_BGE;
            break;
        case 0xb:
            d->type = ARM64_INSN_BLT;
            break;
        case 0xc:
            d->type = ARM64_INSN_BGT;
            break;
        case 0xd:
            d->type = ARM64_INSN_BLE;
            break;
        case 0x8:
            d->type = ARM64_INSN_BHI;
            break;
        case 0x9:
            d->type = ARM64_INSN_BLS;
            break;
        default:
            return 0;
        }
        return 1;
    }

    if ((insn & 0x7e000000) == 0x34000000) {
        d->sf = BIT(insn, 31);
        d->rt = BITS(insn, 4, 0);
        d->offset = SEXT(BITS(insn, 23, 5), 19) << 2;
        d->type = BIT(insn, 24) ? ARM64_INSN_CBNZ : ARM64_INSN_CBZ;
        return 1;
    }

    if ((insn & 0xffc00000) == 0xd61f0000) {
        d->rn = BITS(insn, 9, 5);
        d->type = ARM64_INSN_BR;
        return 1;
    }

    if ((insn & 0xffc00000) == 0xd63f0000) {
        d->rn = BITS(insn, 9, 5);
        d->type = ARM64_INSN_BLR;
        return 1;
    }

    if ((insn & 0xfffffc1f) == 0xd65f0000) {
        d->rn = BITS(insn, 9, 5);
        d->type = ARM64_INSN_RET;
        return 1;
    }

    return 0;
}

static int decode_load_store(uint32_t insn, arm64_insn_t *d)
{
    uint8_t op0 = BITS(insn, 31, 28);
    uint8_t op1 = BITS(insn, 26, 26);
    uint8_t op2 = BITS(insn, 24, 23);
    uint8_t op3 = BITS(insn, 21, 21);
    uint8_t op4 = BITS(insn, 11, 10);

    if (op1 == 1 && op2 == 0 && op3 == 0) {
        d->sf = BIT(insn, 30);
        uint8_t opc = BITS(insn, 23, 22);
        d->rt = BITS(insn, 4, 0);
        d->rn = BITS(insn, 9, 5);
        d->imm = BITS(insn, 21, 10);

        uint8_t size = BITS(insn, 31, 30);
        int scale = size;
        d->offset = d->imm << scale;

        if (opc == 0x1) {
            d->type = size == 3
                          ? ARM64_INSN_LDR_IMM
                          : (size == 2 ? ARM64_INSN_LDR_IMM
                                       : (size == 1 ? ARM64_INSN_LDRH_IMM : ARM64_INSN_LDRB_IMM));
        } else if (opc == 0x0) {
            d->type = size == 3
                          ? ARM64_INSN_STR_IMM
                          : (size == 2 ? ARM64_INSN_STR_IMM
                                       : (size == 1 ? ARM64_INSN_STRH_IMM : ARM64_INSN_STRB_IMM));
        } else {
            return 0;
        }
        return 1;
    }

    if ((insn & 0x3b000000) == 0x29000000) {
        d->sf = BIT(insn, 31);
        d->rt = BITS(insn, 4, 0);
        d->rt2 = BITS(insn, 14, 10);
        d->rn = BITS(insn, 9, 5);
        d->imm = SEXT(BITS(insn, 21, 15), 7) << (2 + d->sf);

        d->type = BIT(insn, 22) ? ARM64_INSN_LDP : ARM64_INSN_STP;
        return 1;
    }

    return 0;
}

static int decode_data_proc_reg(uint32_t insn, arm64_insn_t *d)
{
    if ((insn & 0x1f000000) == 0x0b000000) {
        d->sf = BIT(insn, 31);
        uint8_t opc = BITS(insn, 30, 29);
        d->rd = BITS(insn, 4, 0);
        d->rn = BITS(insn, 9, 5);
        d->rm = BITS(insn, 20, 16);
        d->shift = BITS(insn, 15, 10);

        if (opc == 0) {
            d->type = ARM64_INSN_ADD_REG;
        } else if (opc == 2) {
            d->type = ARM64_INSN_SUB_REG;
        } else if (opc == 1) {
            d->type = ARM64_INSN_SUB_REG;
        } else {
            return 0;
        }
        return 1;
    }

    if ((insn & 0x1f200000) == 0x0a000000) {
        d->sf = BIT(insn, 31);
        uint8_t opc = BITS(insn, 30, 29);
        d->rd = BITS(insn, 4, 0);
        d->rn = BITS(insn, 9, 5);
        d->rm = BITS(insn, 20, 16);

        if (opc == 0) {
            d->type = ARM64_INSN_AND_REG;
        } else if (opc == 1) {
            d->type = ARM64_INSN_ORR_REG;
        } else if (opc == 2) {
            d->type = ARM64_INSN_EOR_REG;
        } else {
            return 0;
        }
        return 1;
    }

    if ((insn & 0x7fe00000) == 0x1ac00000) {
        d->sf = BIT(insn, 31);
        d->rd = BITS(insn, 4, 0);
        d->rn = BITS(insn, 9, 5);
        d->rm = BITS(insn, 20, 16);

        uint8_t op = BITS(insn, 15, 10);
        if (op == 0x02) {
            d->type = ARM64_INSN_UDIV;
        } else if (op == 0x03) {
            d->type = ARM64_INSN_SDIV;
        } else if (op == 0x00) {
            d->type = ARM64_INSN_MUL;
            d->ra = BITS(insn, 14, 10);
        } else {
            return 0;
        }
        return 1;
    }

    if ((insn & 0x7fe0ffe0) == 0x2a0003e0) {
        d->sf = BIT(insn, 31);
        d->rd = BITS(insn, 4, 0);
        d->rm = BITS(insn, 20, 16);
        d->type = ARM64_INSN_MOV_REG;
        return 1;
    }

    if ((insn & 0x7f800000) == 0x6b000000) {
        d->sf = BIT(insn, 31);
        d->rd = BITS(insn, 4, 0);
        d->rn = BITS(insn, 9, 5);
        d->imm = BITS(insn, 21, 10);
        d->type = ARM64_INSN_CMP_IMM;
        return 1;
    }

    if ((insn & 0x7fe0001f) == 0x6b00001f) {
        d->sf = BIT(insn, 31);
        d->rn = BITS(insn, 9, 5);
        d->rm = BITS(insn, 20, 16);
        d->type = ARM64_INSN_CMP_REG;
        return 1;
    }

    return 0;
}

static int decode_system(uint32_t insn, arm64_insn_t *d)
{
    if ((insn & 0xffe0001f) == 0xd4000001) {
        d->imm = BITS(insn, 20, 5);
        d->type = ARM64_INSN_SVC;
        return 1;
    }

    if ((insn & 0xffe0001f) == 0xd4200000) {
        d->imm = BITS(insn, 20, 5);
        d->type = ARM64_INSN_BRK;
        return 1;
    }

    if ((insn & 0xffe0001f) == 0xd4400000) {
        d->imm = BITS(insn, 20, 5);
        d->type = ARM64_INSN_HLT;
        return 1;
    }

    if (insn == 0xd503201f) {
        d->type = ARM64_INSN_NOP;
        return 1;
    }

    return 0;
}

int arm64_decode(uint32_t insn, arm64_insn_t *decoded)
{
    memset(decoded, 0, sizeof(arm64_insn_t));
    decoded->raw = insn;

    uint8_t op0 = BITS(insn, 28, 25);

    if (op0 == 0x2 || op0 == 0x3) {
        return decode_branch(insn, decoded);
    }

    if ((op0 & 0x5) == 0x4) {
        return decode_load_store(insn, decoded);
    }

    if ((op0 & 0x7) == 0x5) {
        return decode_data_proc_reg(insn, decoded);
    }

    if ((op0 & 0x3) == 0x0 || (op0 & 0x3) == 0x1) {
        int ret = decode_data_proc_imm(insn, decoded);
        if (ret)
            return ret;
    }

    if ((op0 & 0xf) == 0xa || (op0 & 0xf) == 0xb) {
        return decode_system(insn, decoded);
    }

    decoded->type = ARM64_INSN_UNKNOWN;
    return 0;
}
