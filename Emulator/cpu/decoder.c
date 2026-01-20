#include "decoder.h"
#include <string.h>
#include <stdio.h>

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

    if (BITS(insn, 28, 23) == 0x26) {
        d->sf = BIT(insn, 31);
        uint8_t opc = BITS(insn, 30, 29);
        uint8_t N = BIT(insn, 22);
        d->imm = BITS(insn, 21, 16);
        d->shift = BITS(insn, 15, 10);
        d->rn = BITS(insn, 9, 5);
        d->rd = BITS(insn, 4, 0);

        if (N != d->sf) {
            return 0;
        }

        if (opc == 0x0) {
            d->type = ARM64_INSN_SBFM;
        } else if (opc == 0x1) {
            d->type = ARM64_INSN_BFM;
        } else if (opc == 0x2) {
            d->type = ARM64_INSN_UBFM;
        } else {
            return 0;
        }
        return 1;
    }

    if (BITS(insn, 28, 23) == 0x24) {
        d->sf = BIT(insn, 31);
        uint8_t opc = BITS(insn, 30, 29);
        uint8_t N = BIT(insn, 22);
        uint8_t immr = BITS(insn, 21, 16);
        uint8_t imms = BITS(insn, 15, 10);
        d->rn = BITS(insn, 9, 5);
        d->rd = BITS(insn, 4, 0);

        uint64_t mask = (1ULL << (imms + 1)) - 1;
        if (!d->sf) {
            mask &= 0xFFFFFFFF;
        }
        d->imm = mask;

        if (opc == 0x0) {
            d->type = ARM64_INSN_AND_IMM;
        } else if (opc == 0x1) {
            d->type = ARM64_INSN_ORR_IMM;
        } else if (opc == 0x2) {
            d->type = ARM64_INSN_EOR_IMM;
        } else if (opc == 0x3) {
            d->type = ARM64_INSN_ANDS_IMM;
        } else {
            return 0;
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

    printf("❌ [Decoder] Unknown data_proc_imm pattern: 0x%08x (op0=0x%02x)\n", insn, op0);
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

    if ((insn & 0x7e000000) == 0x36000000) {
        d->sf = BIT(insn, 31);
        d->rt = BITS(insn, 4, 0);
        d->imm = (BITS(insn, 23, 19) << 5) | BITS(insn, 31, 31);
        d->offset = SEXT(BITS(insn, 18, 5), 14) << 2;
        d->type = BIT(insn, 24) ? ARM64_INSN_TBNZ : ARM64_INSN_TBZ;
        return 1;
    }

    if ((insn & 0xfffffc1f) == 0xd61f0000) {
        d->rn = BITS(insn, 9, 5);
        d->type = ARM64_INSN_BR;
        return 1;
    }

    if ((insn & 0xfffffc1f) == 0xd63f0000) {
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
    if ((insn & 0x3f000000) == 0x39000000) {
        uint8_t size = BITS(insn, 31, 30);
        uint8_t opc = BITS(insn, 23, 22);
        d->rt = BITS(insn, 4, 0);
        d->rn = BITS(insn, 9, 5);
        uint16_t imm12 = BITS(insn, 21, 10);
        d->offset = imm12 << size;

        if (opc == 0x01) {
            if (size == 3) {
                d->type = ARM64_INSN_LDR_IMM;
            } else if (size == 2) {
                d->type = ARM64_INSN_LDR_IMM;
            } else if (size == 1) {
                d->type = ARM64_INSN_LDRH_IMM;
            } else {
                d->type = ARM64_INSN_LDRB_IMM;
            }
        } else if (opc == 0x00) {
            if (size == 3) {
                d->type = ARM64_INSN_STR_IMM;
            } else if (size == 2) {
                d->type = ARM64_INSN_STR_IMM;
            } else if (size == 1) {
                d->type = ARM64_INSN_STRH_IMM;
            } else {
                d->type = ARM64_INSN_STRB_IMM;
            }
        } else if (opc == 0x03) {
            if (size == 0) {
                d->type = ARM64_INSN_LDRSB_IMM;
            } else if (size == 1) {
                d->type = ARM64_INSN_LDRSH_IMM;
            } else if (size == 2) {
                d->type = ARM64_INSN_LDRSW_IMM;
            } else {
                return 0;
            }
        } else if (opc == 0x02) {
            if (size == 0) {
                d->type = ARM64_INSN_LDRSB_IMM;
            } else if (size == 1) {
                d->type = ARM64_INSN_LDRSH_IMM;
            } else if (size == 2) {
                d->type = ARM64_INSN_LDRSW_IMM;
            } else {
                return 0;
            }
        } else {
            return 0;
        }
        d->sf = (size == 3) ? 1 : 0;
        return 1;
    }

    if ((insn & 0x3b200000) == 0x38000000) {
        uint8_t size = BITS(insn, 31, 30);
        uint8_t opc = BITS(insn, 23, 22);
        uint8_t idx = BITS(insn, 11, 10);
        d->rt = BITS(insn, 4, 0);
        d->rn = BITS(insn, 9, 5);
        int16_t imm9 = BITS(insn, 20, 12);
        if (imm9 & 0x100) {
            imm9 |= 0xFE00;
        }
        d->offset = imm9;
        d->sf = (size == 3) ? 1 : 0;
#ifdef STANDALONE_TEST
        printf("[DEBUG] LDUR pattern matched: insn=0x%08x, size=%d, opc=%d, idx=%d\n", insn, size, opc, idx);
#endif

        if (idx == 0x0) {
            if (opc == 0x01) {
                if (size == 3 || size == 2) {
                    d->type = ARM64_INSN_LDUR;
                } else if (size == 1) {
                    d->type = ARM64_INSN_LDURH;
                } else {
                    d->type = ARM64_INSN_LDURB;
                }
            } else if (opc == 0x00) {
                if (size == 3 || size == 2) {
                    d->type = ARM64_INSN_STUR;
                } else if (size == 1) {
                    d->type = ARM64_INSN_STURH;
                } else {
                    d->type = ARM64_INSN_STURB;
                }
            } else if (opc == 0x03 || opc == 0x02) {
                if (size == 0) {
                    d->type = ARM64_INSN_LDURSB;
                } else if (size == 1) {
                    d->type = ARM64_INSN_LDURSH;
                } else if (size == 2) {
                    d->type = ARM64_INSN_LDURSW;
                } else {
                    return 0;
                }
            } else {
                return 0;
            }
        } else if (idx == 0x1) {
#ifdef STANDALONE_TEST
            printf("[DEBUG] idx=1 branch, opc=%d, size=%d\n", opc, size);
#endif
            if (opc == 0x01 && (size == 3 || size == 2)) {
#ifdef STANDALONE_TEST
                printf("[DEBUG] Setting type to LDUR_POST\n");
#endif
                d->type = ARM64_INSN_LDUR_POST;
            } else if (opc == 0x00 && (size == 3 || size == 2)) {
                d->type = ARM64_INSN_STUR_POST;
            } else {
#ifdef STANDALONE_TEST
                printf("[DEBUG] No match in idx=1, returning 0\n");
#endif
                return 0;
            }
        } else if (idx == 0x3) {
            if (opc == 0x01 && (size == 3 || size == 2)) {
                d->type = ARM64_INSN_LDUR_PRE;
            } else if (opc == 0x00 && (size == 3 || size == 2)) {
                d->type = ARM64_INSN_STUR_PRE;
            } else {
                return 0;
            }
        } else {
            return 0;
        }
        return 1;
    }

    if ((insn & 0x3f200c00) == 0x38000800) {
        uint8_t size = BITS(insn, 31, 30);
        uint8_t opc = BITS(insn, 23, 22);
        d->rt = BITS(insn, 4, 0);
        d->rn = BITS(insn, 9, 5);
        d->rm = BITS(insn, 20, 16);
        uint8_t S = BIT(insn, 12);
        d->shift = S ? size : 0;
        d->option = BITS(insn, 15, 13);

        if (opc == 0x01) {
            if (size == 3) {
                d->type = ARM64_INSN_LDR_REG;
            } else if (size == 2) {
                d->type = ARM64_INSN_LDR_REG;
            } else if (size == 1) {
                d->type = ARM64_INSN_LDR_REG;
            } else {
                d->type = ARM64_INSN_LDR_REG;
            }
        } else if (opc == 0x00) {
            d->type = ARM64_INSN_STR_REG;
        } else {
            return 0;
        }
        d->sf = (size == 3) ? 1 : 0;
        return 1;
    }

    if ((insn & 0x3f200c00) == 0x38200800) {
        uint8_t size = BITS(insn, 31, 30);
        uint8_t opc = BITS(insn, 23, 22);
        d->rt = BITS(insn, 4, 0);
        d->rn = BITS(insn, 9, 5);
        d->rm = BITS(insn, 20, 16);
        uint8_t S = BIT(insn, 12);
        d->shift = S ? size : 0;
        d->option = BITS(insn, 15, 13);
#ifdef STANDALONE_TEST
        printf("[DEBUG] FP/SIMD register pattern matched: insn=0x%08x, size=%d, opc=%d\n", insn, size, opc);
#endif

        if (opc == 0x01) {
            d->type = ARM64_INSN_LDR_VREG;
#ifdef STANDALONE_TEST
            printf("[DEBUG] Set type to ARM64_INSN_LDR_VREG, returning 1\n");
#endif
        } else if (opc == 0x00) {
            d->type = ARM64_INSN_STR_VREG;
        } else {
#ifdef STANDALONE_TEST
            printf("[DEBUG] opc not 0 or 1, returning 0\n");
#endif
            return 0;
        }
        d->sf = (size == 3) ? 1 : 0;
#ifdef STANDALONE_TEST
        printf("[DEBUG] About to return 1 from FP/SIMD handler\n");
#endif
        return 1;
    }

    if ((insn & 0x3b200000) == 0x38000400) {
        uint8_t size = BITS(insn, 31, 30);
        uint8_t opc = BITS(insn, 23, 22);
        d->rt = BITS(insn, 4, 0);
        d->rn = BITS(insn, 9, 5);
        int16_t imm9 = BITS(insn, 20, 12);
        if (imm9 & 0x100) {
            imm9 |= 0xFE00;
        }
        d->offset = imm9;

        uint8_t idx = BITS(insn, 11, 10);
        if (idx == 0x3) {
            if (opc == 0x01 && size == 3) {
                d->type = ARM64_INSN_LDR_PRE;
            } else if (opc == 0x00 && size == 3) {
                d->type = ARM64_INSN_STR_PRE;
            } else {
                return 0;
            }
        } else if (idx == 0x1) {
            if (opc == 0x01 && size == 3) {
                d->type = ARM64_INSN_LDR_POST;
            } else if (opc == 0x00 && size == 3) {
                d->type = ARM64_INSN_STR_POST;
            } else {
                return 0;
            }
        } else {
            return 0;
        }
        d->sf = (size == 3) ? 1 : 0;
        return 1;
    }

    if ((insn & 0x3b800000) == 0x29800000) {
        d->sf = BIT(insn, 31);
        d->rt = BITS(insn, 4, 0);
        d->rt2 = BITS(insn, 14, 10);
        d->rn = BITS(insn, 9, 5);
        d->imm = SEXT(BITS(insn, 21, 15), 7) << (2 + d->sf);

        if (BIT(insn, 23)) {
            d->type = BIT(insn, 22) ? ARM64_INSN_LDP_PRE : ARM64_INSN_STP_PRE;
        } else {
            d->type = BIT(insn, 22) ? ARM64_INSN_LDP_POST : ARM64_INSN_STP_POST;
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
        } else if (opc == 1) {
            d->type = ARM64_INSN_SUB_REG;
        } else if (opc == 2) {
            d->type = ARM64_INSN_SUB_REG;
        } else if (opc == 3) {
            if (d->rd == 31) {
                d->type = ARM64_INSN_CMP_REG;
            } else {
                d->type = ARM64_INSN_SUBS_REG;
            }
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

    if ((insn & 0x7fe08000) == 0x1b000000) {
        d->sf = BIT(insn, 31);
        d->rd = BITS(insn, 4, 0);
        d->rn = BITS(insn, 9, 5);
        d->rm = BITS(insn, 20, 16);
        d->ra = BITS(insn, 14, 10);

        if (BIT(insn, 15)) {
            d->type = ARM64_INSN_MSUB;
        } else {
            d->type = ARM64_INSN_MADD;
        }
        return 1;
    }

    if ((insn & 0x7fe0001f) == 0x6a00001f) {
        d->sf = BIT(insn, 31);
        d->rn = BITS(insn, 9, 5);
        d->rm = BITS(insn, 20, 16);
        d->type = ARM64_INSN_TST_REG;
        return 1;
    }

    if ((insn & 0x7fe0ffe0) == 0x2a0003e0) {
        d->sf = BIT(insn, 31);
        d->rd = BITS(insn, 4, 0);
        d->rm = BITS(insn, 20, 16);
        d->type = ARM64_INSN_MOV_REG;
        return 1;
    }

    if ((insn & 0x7fe00000) == 0x1ac02000) {
        d->sf = BIT(insn, 31);
        d->rd = BITS(insn, 4, 0);
        d->rn = BITS(insn, 9, 5);
        d->rm = BITS(insn, 20, 16);

        uint8_t op2 = BITS(insn, 11, 10);
        if (op2 == 0x0) {
            d->type = ARM64_INSN_LSL_REG;
        } else if (op2 == 0x1) {
            d->type = ARM64_INSN_LSR_REG;
        } else if (op2 == 0x2) {
            d->type = ARM64_INSN_ASR_REG;
        } else {
            return 0;
        }
        return 1;
    }

    if ((insn & 0x7fe00000) == 0x2a200000) {
        d->sf = BIT(insn, 31);
        d->rd = BITS(insn, 4, 0);
        d->rn = BITS(insn, 9, 5);
        d->rm = BITS(insn, 20, 16);

        uint8_t opc = BITS(insn, 30, 29);
        if (opc == 0x0) {
            d->type = ARM64_INSN_BIC_REG;
        } else if (opc == 0x1) {
            d->type = ARM64_INSN_ORN_REG;
        } else {
            return 0;
        }
        return 1;
    }

    if ((insn & 0x7fe0ffe0) == 0x2a2003e0) {
        d->sf = BIT(insn, 31);
        d->rd = BITS(insn, 4, 0);
        d->rm = BITS(insn, 20, 16);
        d->type = ARM64_INSN_MVN_REG;
        return 1;
    }

    if ((insn & 0x7fe0ffe0) == 0x4b0003e0) {
        d->sf = BIT(insn, 31);
        d->rd = BITS(insn, 4, 0);
        d->rm = BITS(insn, 20, 16);
        d->type = ARM64_INSN_NEG_REG;
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

    if ((insn & 0x7fe00000) == 0x1a800000) {
        d->sf = BIT(insn, 31);
        d->rd = BITS(insn, 4, 0);
        d->rn = BITS(insn, 9, 5);
        d->rm = BITS(insn, 20, 16);
        d->cond = BITS(insn, 15, 12);

        uint8_t op2 = BITS(insn, 11, 10);
        if (op2 == 0x0) {
            d->type = ARM64_INSN_CSEL;
        } else if (op2 == 0x1) {
            d->type = ARM64_INSN_CSINC;
        } else if (op2 == 0x2) {
            d->type = ARM64_INSN_CSINV;
        } else if (op2 == 0x3) {
            d->type = ARM64_INSN_CSNEG;
        } else {
            return 0;
        }
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

    if ((op0 >= 0xA && op0 <= 0xB) || op0 == 0x2 || op0 == 0x3) {
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
