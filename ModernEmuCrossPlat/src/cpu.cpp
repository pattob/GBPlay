#include "pch.h"

#include "cpu.h"

GBZ80::GBZ80(shared_ptr<MemoryMap> ram)
    : r()
    , SP(0U)
    , PC(0U)
    , m_ram(ram)
    , ref_u16()
    , ref_u8()
{
}

inline DecodedOpcode bad_op(const uint16_t location, uint8_t op, const uint8_t additional_ops = 0)
{
    return DecodedOpcode(location, op, "BAD_OP_VAL", "", 1 + additional_ops, OpcodeGroup::UNKNOWN, 0, 0);
}

DecodedOpcode GBZ80::decode_op(const uint16_t location, uint8_t op)
{
    Opcode opcode = {};
    opcode.raw = op;

    auto p = opcode.d.p();
    auto q = opcode.d.q();

    if (opcode.d.x == 0)
    {
        if (opcode.d.z == 0)
        {
            if (opcode.d.y == 0)
                // NO OP
                return DecodedOpcode(location, op, "NOP", "", 1, OpcodeGroup::CTL_MISC, 4, 0);
            else if (opcode.d.y == 1)
                // LD (u16), SP
                return DecodedOpcode(location, op, "LD", "(a16), SP", 3, OpcodeGroup::X16_LSM, 20, 0
                    , Address(Immidiate(m_ram->read(location + 1)), m_ram))
                    , Reg(&SP, "SP"));
            else if (opcode.d.y == 2)
                // STOP
                // interesting note about this... it has been seen that
                // some games/assemblers either don't fill the second half
                // of the stop command AND/OR STOP plus some input has
                // some undefined behavior
                return DecodedOpcode(location, op, "STOP", "", 2, OpcodeGroup::CTL_MISC, 4, 0,
                    make_shared<ConstVal<uint8_t>>(0));
            else if (opcode.d.y == 3)
                // JR d
                return DecodedOpcode(location, op, "JR", "i8", 2, OpcodeGroup::CTL_BR, 12, 0,
                    Immidiate(m_ram->read(location + 1)));
            else if (opcode.d.y >= 4 && opcode.d.y <= 7)
                // JR cc[y-4], d
                return DecodedOpcode(location, op, "JR", get_cc(opcode.d.y - 4) + ", i8", 2, OpcodeGroup::CTL_BR, 8, 12, 0
                    , make_shared<SignedConst8BitLocation>(m_ram, make_shared<ConstVal<uint16_t>>(location + 1)));
        }
        else if (opcode.d.z == 1)
        {
            if (q == 0)
                // LD rp[p], nn
                return DecodedOpcode(location, op, "LD",  get_rp_name(p) + ", d16", 3, OpcodeGroup::X16_LSM, 12, 0
                , get_rp(p)
                , make_shared<Const16BitLocation>(m_ram, make_shared<ConstVal<uint16_t>>(location + 1)));
            else if (q == 1)
                // ADD HL, rp[p]
                return DecodedOpcode(location, op, "ADD", get_rp_name(2)+", " + get_rp_name(p), 1, OpcodeGroup::X16_ALU, 8, 0b1110ui8
                , get_rp(2)
                , get_rp(p));
        }
        else if (opcode.d.z == 2)
        {
            if (q == 0)
            {
                
                if (p == 0)
                    // LD (BC), A
                    return DecodedOpcode(location, op, "LD", "(BC), A", 1, OpcodeGroup::X8_LSM, 8, 0
                        , make_shared<Const8BitLocation>(m_ram, get_rp(0))
                        , get_r(6));
                else if (p == 1)
                    // LD (DE), A
                    return DecodedOpcode(location, op, "LD", "(DE), A", 1, OpcodeGroup::X8_LSM, 8, 0
                        , make_shared<Const8BitLocation>(m_ram, get_rp(1))
                        , get_r(6));
                else if (p == 2)
                    // LD (HL+), A
                    return DecodedOpcode(location, op, "LD", "(HL+), A", 1, OpcodeGroup::X8_LSM, 8, 0
                        , make_shared<Const8BitLocation>(m_ram, get_rp(2))
                        , get_r(6)); // TODO: Make sure to add to the rp[2]+1 when we write this
                else if (p == 3)
                    // LD (HL-), A
                    return DecodedOpcode(location, op, "LD", "(HL-), A", 1, OpcodeGroup::X8_LSM, 8, 0
                        , make_shared<Const8BitLocation>(m_ram, get_rp(2))
                        , get_r(6)); // TODO: Make sure to sub to the rp[2]-1 when we write this
            }
            else if (q == 1)
            {
                if (p == 0)
                    // LD A, (BC)
                    return DecodedOpcode(location, op, "LD", "A, (BC)", 1, OpcodeGroup::X8_LSM, 8, 0
                    , get_r(6)
                    , make_shared<Const8BitLocation>(m_ram, get_rp(0)));
                else if (p == 1)
                    // LD A, (DE)
                    return DecodedOpcode(location, op, "LD", "A, (DE)", 1, OpcodeGroup::X8_LSM, 8, 0
                    , get_r(6)
                    , make_shared<Const8BitLocation>(m_ram, get_rp(1)));
                else if (p == 2)
                    // LD A, (HL+)
                    return DecodedOpcode(location, op, "LD", "A, (HL+)", 1, OpcodeGroup::X8_LSM, 8, 0
                        , get_r(6)
                        , make_shared<Const8BitLocation>(m_ram, get_rp(2))); // TODO: Make sure to add to the rp[2]+1 when we write this
                else if (p == 3)
                    // LD A, (HL-)
                    return DecodedOpcode(location, op, "LD", "A, (HL-)", 1, OpcodeGroup::X8_LSM, 8, 0
                        , get_r(6)
                        , make_shared<Const8BitLocation>(m_ram, get_rp(2))); // TODO: Make sure to sub to the rp[2]-1 when we write this
            }
        }
        else if (opcode.d.z == 3)
        {
            if (q == 0)
                // INC rp[p]
                return DecodedOpcode(location, op, "INC", get_rp_name(p), 1, OpcodeGroup::X16_ALU, 8, 0
                    , get_rp(p));
            else if (q == 1)
                // DEC rp[p]
                return DecodedOpcode(location, op, "DEC", get_rp_name(p), 1, OpcodeGroup::X16_ALU, 8, 0
                    , get_rp(p));
        }
        else if (opcode.d.z == 4)
        {
            if (opcode.d.y == 6)
            {
                // INC (HL)
                return DecodedOpcode(location, op, "INC", "(" + get_rp_name(2) + ")", 1, OpcodeGroup::X8_ALU, 12, 0b0111ui8
                    , make_shared<Const8BitLocation>(m_ram, get_rp(2)));
            }
            else
            {

                // INC r[y]
                return DecodedOpcode(location, op, "INC", get_r_name(opcode.d.y), 1, OpcodeGroup::X8_ALU, 4, 0b0111ui8
                    , get_r(opcode.d.y));
            }
        }
        else if (opcode.d.z == 5)
        {
            if (opcode.d.y == 6)
            {
                // DEC (HL)
                return DecodedOpcode(location, op, "DEC", "(" + get_rp_name(2) + ")", 1, OpcodeGroup::X8_ALU, 12, 0b0111ui8
                    , make_shared<Const8BitLocation>(m_ram, get_rp(2)));
            }
            else
            {

                // DEC r[y]
                return DecodedOpcode(location, op, "DEC", get_r_name(opcode.d.y), 1, OpcodeGroup::X8_ALU, 4, 0b0111ui8
                    , get_r(opcode.d.y));
            }
        }
        else if (opcode.d.z == 6)
        {
            if (opcode.d.y == 6)
            {
                // LD (HL), n
                return DecodedOpcode(location, op, "LD", "(" + get_rp_name(2) + ")" + ", u8", 2, OpcodeGroup::X8_LSM, 12, 0
                    , make_shared<Const8BitLocation>(m_ram, get_rp(2))
                    , make_shared<Const8BitLocation>(m_ram, make_shared<ConstVal<uint16_t>>(location + 1)));
            }
            else
            {
                // LD r[y], n
                return DecodedOpcode(location, op, "LD", get_r_name(opcode.d.y) + ", u8", 2, OpcodeGroup::X8_LSM, 8, 0
                    , get_r(opcode.d.y)
                    , make_shared<Const8BitLocation>(m_ram, make_shared<ConstVal<uint16_t>>(location + 1)));
            }
        }
        else if (opcode.d.z == 7)
        {
            if (opcode.d.y == 0)
                // RLCA
                return DecodedOpcode(location, op, "RLCA", "", 1, OpcodeGroup::X8_RSB, 4, 0b1111ui8);
            else if (opcode.d.y == 1)
                // RRCA
                return DecodedOpcode(location, op, "RRCA", "", 1, OpcodeGroup::X8_RSB, 4, 0b1111ui8);
            else if (opcode.d.y == 2)
                // RLA
                return DecodedOpcode(location, op, "RLA", "", 1, OpcodeGroup::X8_RSB, 4, 0b1111ui8);
            else if (opcode.d.y == 3)
                // RRA
                return DecodedOpcode(location, op, "RRA", "", 1, OpcodeGroup::X8_RSB, 4, 0b1111ui8);
            else if (opcode.d.y == 4)
                // DAA
                return DecodedOpcode(location, op, "DAA", "", 1, OpcodeGroup::X8_ALU, 4, 0b1101ui8);
            else if (opcode.d.y == 5)
                // CPL
                return DecodedOpcode(location, op, "CPL", "", 1, OpcodeGroup::X8_ALU, 4, 0b0110ui8);
            else if (opcode.d.y == 6)
                // SCF
                return DecodedOpcode(location, op, "SCF", "", 1, OpcodeGroup::X8_ALU, 4, 0b1110ui8);
            else if (opcode.d.y == 7)
                // CCF
                return DecodedOpcode(location, op, "CCF", "", 1, OpcodeGroup::X8_ALU, 4, 0b1110ui8);
        }
    }
    else if (opcode.d.x == 1)
    {
        if (opcode.d.z == 6 && opcode.d.y == 6)
            // HALT
            return DecodedOpcode(location, op, "HALT", "", 1, OpcodeGroup::CTL_MISC, 4, 0);
        else if (opcode.d.y == 6)
            // LD (HL), r[z]
            return DecodedOpcode(location, op, "LD", "(HL), " + get_r_name(opcode.d.z), 1, OpcodeGroup::X8_LSM, 8, 0
                , make_shared<Const8BitLocation>(m_ram, get_rp(2))
                , get_r(opcode.d.z));
        else if (opcode.d.z == 6)
            // LD r[y], (HL)
            return DecodedOpcode(location, op, "LD", get_r_name(opcode.d.y) + ", (HL)", 1, OpcodeGroup::X8_LSM, 8, 0
                , get_r(opcode.d.y)
                , make_shared<Const8BitLocation>(m_ram, get_rp(2)));
        else
            // LD r[y], r[z]
            return DecodedOpcode(location, op, "LD", get_r_name(opcode.d.y) + ", " + get_r_name(opcode.d.z), 1, OpcodeGroup::X8_LSM, 4, 0
                , get_r(opcode.d.y)
                , get_r(opcode.d.z));
    }
    else if (opcode.d.x == 2)
    {
        // alu[y] r[z]
        return alu_op(location, opcode);
    }
    else if (opcode.d.x == 3)
    {
        if (opcode.d.z == 0)
        {
            if (opcode.d.y >= 0 && opcode.d.y <= 3)
                // RET cc[y]
                return DecodedOpcode(location, op, "RET", get_cc(opcode.d.y), 1, OpcodeGroup::CTL_BR, 8, 20, 0);
            else if (opcode.d.y == 4)
                // LDH (0xFF00+nn), A
                return DecodedOpcode(location, op, "LDH", "(0xFF00+u8), A", 2, OpcodeGroup::X8_LSM, 12, 0,
                    make_shared<Const8BitLocation>(
                        m_ram,
                        make_shared<RegDirectOffset>(
                            0xFF00,
                            make_shared<Const8BitLocation>(
                                m_ram,
                                make_shared<ConstVal<uint16_t>>(location+1)))),
                    get_r(1)); // TODO: offset based memory access
            else if (opcode.d.y == 5)
                // ADD SP, d
                return DecodedOpcode(location, op, "ADD", "SP, i8", 2, OpcodeGroup::X16_ALU, 16, 0b1111ui8); // TODO: mixed add
            else if (opcode.d.y == 6)
                // LDH A, (0xFF00 + n)
                return DecodedOpcode(location, op, "LDH", "A, (0xFF00 + u8)", 2, OpcodeGroup::X8_LSM, 12, 0,
                    get_r(1),
                    make_shared<Const8BitLocation>(
                        m_ram,
                        make_shared<RegDirectOffset>(
                            0xFF00,
                            make_shared<Const8BitLocation>(
                                m_ram,
                                make_shared<ConstVal<uint16_t>>(location + 1))))); // TODO: offset based memory access
            else if (opcode.d.y == 7)
                // LD HL, SP + d
                return DecodedOpcode(location, op, "LD", "HL, (SP + i8)", 2, OpcodeGroup::X16_ALU, 12, 0b1111ui8); // TODO: offset based memory access
        }
        else if (opcode.d.z == 1)
        {
            if (q == 0)
                // POP rp2[p]
                if (p == 3)
                    return DecodedOpcode(location, op, "POP", get_rp2_name(p), 1, OpcodeGroup::X16_LSM, 12, 0b1111ui8
                        , get_rp2(p));
                else
                    return DecodedOpcode(location, op, "POP", get_rp2_name(p), 1, OpcodeGroup::X16_LSM, 12, 0
                        , get_rp2(p));
            else if (q == 1)
            {
                if (p == 0)
                    // RET
                    return DecodedOpcode(location, op, "RET", "", 1, OpcodeGroup::CTL_BR, 16, 0);
                else if (p == 1)
                    // RETI
                    return DecodedOpcode(location, op, "RETI", "", 1, OpcodeGroup::CTL_BR, 16, 0);
                else if (p == 2)
                    // JP HL
                    return DecodedOpcode(location, op, "JP", "HL", 1, OpcodeGroup::CTL_BR, 4, 0
                        , make_shared<Addressable16Bit>(m_ram, get_rp(2)));
                else if (p == 3)
                    // LD SP, HL
                    return DecodedOpcode(location, op, "LD", "SP, HL", 1, OpcodeGroup::X16_LSM, 8, 0
                        , get_rp(3)
                        , get_rp(2));
            }
        }
        else if (opcode.d.z == 2)
        {
            if (opcode.d.y >= 0 && opcode.d.y <= 3)
                // JP cc[y], nn
                return DecodedOpcode(location, op, "JP", get_cc(opcode.d.y) + ", a16", 3, OpcodeGroup::CTL_BR, 12, 16, 0
                    , make_shared<Addressable16Bit>(m_ram, make_shared<ConstVal<uint16_t>>(location+1)));
            else if (opcode.d.y == 4)
                // LD (0xFF00 + C), A
                return DecodedOpcode(location, op, "LD", "(0xFF00 + C), A", 1, OpcodeGroup::X8_LSM, 8, 0
                    , make_shared<Const8BitLocation>(m_ram, get_r_and_offset(0xFF00, 1))
                    , get_r(6));
            else if (opcode.d.y == 5)
                // LD (nn), A
                return DecodedOpcode(location, op, "LD", "(u16), A", 3, OpcodeGroup::X8_LSM, 16, 0
                    , make_shared<Const8BitLocation>(m_ram, make_shared<Addressable16Bit>(m_ram, make_shared<ConstVal<uint16_t>>(location + 1)))
                    , get_r(6));
            else if (opcode.d.y == 6)
                // LD A, (0xFF00 + C)
                return DecodedOpcode(location, op, "LD", "A, (0xFF00 + C)", 1, OpcodeGroup::X8_LSM, 8, 0
                    , get_r(6)
                    , make_shared<Const8BitLocation>(m_ram, get_r_and_offset(0xFF00ui16, 1)));
            else if (opcode.d.y == 7)
                // LD A, (nn)
                return DecodedOpcode(location, op, "LD", "A, (a16)", 3, OpcodeGroup::X8_LSM, 16, 0
                    , get_r(6)
                    , make_shared<Const8BitLocation>(m_ram, make_shared<Addressable16Bit>(m_ram, make_shared<ConstVal<uint16_t>>(location + 1))));
        }
        else if (opcode.d.z == 3)
        {
            if (opcode.d.y == 0)
                // JP nn
                return DecodedOpcode(location, op, "JP", "u16", 3, OpcodeGroup::CTL_BR, 16, 0
                    , make_shared<Const16BitLocation>(m_ram, make_shared<ConstVal<uint16_t>>(location + 1)));
            else if (opcode.d.y == 1)
            {
                // CD prefix data
                return perform_cb_op(location, m_ram->read(location + 1));
            }
            else if (opcode.d.y == 6)
                // DI
                return DecodedOpcode(location, op, "DI", "", 1, OpcodeGroup::CTL_MISC, 4, 0);
            else if (opcode.d.y == 7)
                // EI
                return DecodedOpcode(location, op, "EI", "", 1, OpcodeGroup::CTL_MISC, 4, 0);
        }
        else if (opcode.d.z == 4)
        {
            if (opcode.d.y >= 0 && opcode.d.y <= 3)
                // CALL cc[y], nn
                return DecodedOpcode(location, op, "CALL", get_cc(opcode.d.y) + ", a16", 3, OpcodeGroup::CTL_BR, 12, 24, 0
                    , make_shared<Addressable16Bit>(m_ram, make_shared<ConstVal<uint16_t>>(location + 1)));
        }
        else if (opcode.d.z == 5)
        {
            if (q == 0)
                // PUSH rp2[p]
                return DecodedOpcode(location, op, "PUSH", get_rp2_name(p), 1, OpcodeGroup::X16_LSM, 16, 0
                    , get_rp2(p));

            else if (q == 1 && p == 0)
                // CALL nn
                return DecodedOpcode(location, op, "CALL", "a16", 3, OpcodeGroup::CTL_BR, 24, 0
                    , make_shared<Addressable16Bit>(m_ram, make_shared<ConstVal<uint16_t>>(location + 1)));
        }
        else if (opcode.d.z == 6)
        {
            // alu[y] n
            return alu_op(location, opcode);
        }
        else if (opcode.d.z == 7)
        {
            // RST y*8
            return DecodedOpcode(location, op, "RST", itohex(opcode.d.y * 8), 1, OpcodeGroup::CTL_BR, 16, 0);
        }
    }

    return bad_op(location, opcode.raw);
}

DecodedOpcode GBZ80::alu_op(const uint16_t location, Opcode opcode)
{
    if (opcode.d.x == 2)
    {
        if (opcode.d.z == 6)
        {
            switch (opcode.d.y)
            {
            case 0:
                return DecodedOpcode(location, opcode.raw, "ADD", "A, (HL)", 1, OpcodeGroup::X8_ALU, 8, 0b1111ui8
                    , get_r(6)
                    , make_shared<Const8BitLocation>(m_ram, get_rp(2)));
            case 1:
                return DecodedOpcode(location, opcode.raw, "ADC", "A, (HL)", 1, OpcodeGroup::X8_ALU, 8, 0b1111ui8
                    , get_r(6)
                    , make_shared<Const8BitLocation>(m_ram, get_rp(2)));
            case 2:
                return DecodedOpcode(location, opcode.raw, "SUB", "A, (HL)", 1, OpcodeGroup::X8_ALU, 8, 0b1111ui8
                    , get_r(6)
                    , make_shared<Const8BitLocation>(m_ram, get_rp(2)));
            case 3:
                return DecodedOpcode(location, opcode.raw, "SBC", "A, (HL)", 1, OpcodeGroup::X8_ALU, 8, 0b1111ui8
                    , get_r(6)
                    , make_shared<Const8BitLocation>(m_ram, get_rp(2)));
            case 4:
                return DecodedOpcode(location, opcode.raw, "AND", "A, (HL)", 1, OpcodeGroup::X8_ALU, 8, 0b1111ui8
                    , get_r(6)
                    , make_shared<Const8BitLocation>(m_ram, get_rp(2)));
            case 5:
                return DecodedOpcode(location, opcode.raw, "XOR", "A, (HL)", 1, OpcodeGroup::X8_ALU, 8, 0b1111ui8
                    , get_r(6)
                    , make_shared<Const8BitLocation>(m_ram, get_rp(2)));
            case 6:
                return DecodedOpcode(location, opcode.raw, "OR", "A, (HL)", 1, OpcodeGroup::X8_ALU, 8, 0b1111ui8
                    , get_r(6)
                    , make_shared<Const8BitLocation>(m_ram, get_rp(2)));
            case 7:
                return DecodedOpcode(location, opcode.raw, "CP", "A, (HL)", 1, OpcodeGroup::X8_ALU, 8, 0b1111ui8
                    , get_r(6)
                    , make_shared<Const8BitLocation>(m_ram, get_rp(2)));
            default:
                break;
            }
        }
        else
        {
            switch (opcode.d.y)
            {
            case 0:
                return DecodedOpcode(location, opcode.raw, "ADD", "A, " + get_r_name(opcode.d.z), 1, OpcodeGroup::X8_ALU, 4, 0b1111ui8
                    , get_r(6)
                    , get_r(opcode.d.z));
            case 1:
                return DecodedOpcode(location, opcode.raw, "ADC", "A, " + get_r_name(opcode.d.z), 1, OpcodeGroup::X8_ALU, 4, 0b1111ui8
                    , get_r(6)
                    , get_r(opcode.d.z));
            case 2:
                return DecodedOpcode(location, opcode.raw, "SUB", "A, " + get_r_name(opcode.d.z), 1, OpcodeGroup::X8_ALU, 4, 0b1111ui8
                    , get_r(6)
                    , get_r(opcode.d.z));
            case 3:
                return DecodedOpcode(location, opcode.raw, "SBC", "A, " + get_r_name(opcode.d.z), 1, OpcodeGroup::X8_ALU, 4, 0b1111ui8
                    , get_r(6)
                    , get_r(opcode.d.z));
            case 4:
                return DecodedOpcode(location, opcode.raw, "AND", "A, " + get_r_name(opcode.d.z), 1, OpcodeGroup::X8_ALU, 4, 0b1111ui8
                    , get_r(6)
                    , get_r(opcode.d.z));
            case 5:
                return DecodedOpcode(location, opcode.raw, "XOR", "A, " + get_r_name(opcode.d.z), 1, OpcodeGroup::X8_ALU, 4, 0b1111ui8
                    , get_r(6)
                    , get_r(opcode.d.z));
            case 6:
                return DecodedOpcode(location, opcode.raw, "OR", "A, " + get_r_name(opcode.d.z), 1, OpcodeGroup::X8_ALU, 4, 0b1111ui8
                    , get_r(6)
                    , get_r(opcode.d.z));
            case 7:
                return DecodedOpcode(location, opcode.raw, "CP", "A, " + get_r_name(opcode.d.z), 1, OpcodeGroup::X8_ALU, 4, 0b1111ui8
                    , get_r(6)
                    , get_r(opcode.d.z));
            default:
                break;
            }
        }
    }
    else if (opcode.d.x == 3)
    {
        if (opcode.d.z == 6)
        {
            switch (opcode.d.y)
            {
            case 0:
                return DecodedOpcode(location, opcode.raw, "ADD", "A, u8", 2, OpcodeGroup::X8_ALU, 8, 0b1111ui8
                    , get_r(6)
                    , make_shared<Const8BitLocation>(m_ram, make_shared<ConstVal<uint16_t>>(location + 1)));
            case 1:
                return DecodedOpcode(location, opcode.raw, "ADC", "A, u8", 2, OpcodeGroup::X8_ALU, 8, 0b1111ui8
                    , get_r(6)
                    , make_shared<Const8BitLocation>(m_ram, make_shared<ConstVal<uint16_t>>(location + 1)));
            case 2:
                return DecodedOpcode(location, opcode.raw, "SUB", "A, u8", 2, OpcodeGroup::X8_ALU, 8, 0b1111ui8
                    , get_r(6)
                    , make_shared<Const8BitLocation>(m_ram, make_shared<ConstVal<uint16_t>>(location + 1)));
            case 3:
                return DecodedOpcode(location, opcode.raw, "SBC", "A, u8", 2, OpcodeGroup::X8_ALU, 8, 0b1111ui8
                    , get_r(6)
                    , make_shared<Const8BitLocation>(m_ram, make_shared<ConstVal<uint16_t>>(location + 1)));
            case 4:
                return DecodedOpcode(location, opcode.raw, "AND", "A, u8", 2, OpcodeGroup::X8_ALU, 8, 0b1111ui8
                    , get_r(6)
                    , make_shared<Const8BitLocation>(m_ram, make_shared<ConstVal<uint16_t>>(location + 1)));
            case 5:
                return DecodedOpcode(location, opcode.raw, "XOR", "A, u8", 2, OpcodeGroup::X8_ALU, 8, 0b1111ui8
                    , get_r(6)
                    , make_shared<Const8BitLocation>(m_ram, make_shared<ConstVal<uint16_t>>(location + 1)));
            case 6:
                return DecodedOpcode(location, opcode.raw, "OR", "A, u8", 2, OpcodeGroup::X8_ALU, 8, 0b1111ui8
                    , get_r(6)
                    , make_shared<Const8BitLocation>(m_ram, make_shared<ConstVal<uint16_t>>(location + 1)));
            case 7:
                return DecodedOpcode(location, opcode.raw, "CP", "A, u8", 2, OpcodeGroup::X8_ALU, 8, 0b1111ui8
                    , get_r(6)
                    , make_shared<Const8BitLocation>(m_ram, make_shared<ConstVal<uint16_t>>(location + 1)));
            default:
                break;
            }
        }
    }
    return bad_op(location, opcode.raw);
}

DecodedOpcode GBZ80::rot_op(const uint16_t location, Opcode opcode)
{
    switch (opcode.d.y)
    {
    case 0:
        // RLC
        if (opcode.d.z == 6)
            return DecodedOpcode(location, opcode.raw, "RLC", "(HL)", 2, OpcodeGroup::X8_RSB, 16, 0b1111ui8
                , make_shared<Const8BitLocation>(m_ram, get_rp(2)));
        else
            return DecodedOpcode(location, opcode.raw, "RLC", get_r_name(opcode.d.z), 2, OpcodeGroup::X8_RSB, 8, 0b1111ui8
                , get_r(opcode.d.z));
    case 1:
        // RRC
        if (opcode.d.z == 6)
            return DecodedOpcode(location, opcode.raw, "RRC", "(HL)", 2, OpcodeGroup::X8_RSB, 16, 0b1111ui8
                , make_shared<Const8BitLocation>(m_ram, get_rp(2)));
        else
            return DecodedOpcode(location, opcode.raw, "RRC", get_r_name(opcode.d.z), 2, OpcodeGroup::X8_RSB, 8, 0b1111ui8
                , get_r(opcode.d.z));
    case 2:
        // RL
        if (opcode.d.z == 6)
            return DecodedOpcode(location, opcode.raw, "RL", "(HL)", 2, OpcodeGroup::X8_RSB, 16, 0b1111ui8
                , make_shared<Const8BitLocation>(m_ram, get_rp(2)));
        else
            return DecodedOpcode(location, opcode.raw, "RL", get_r_name(opcode.d.z), 2, OpcodeGroup::X8_RSB, 8, 0b1111ui8
                , get_r(opcode.d.z));
    case 3:
        // RR
        if (opcode.d.z == 6)
            return DecodedOpcode(location, opcode.raw, "RR", "(HL)", 2, OpcodeGroup::X8_RSB, 16, 0b1111ui8
                , make_shared<Const8BitLocation>(m_ram, get_rp(2)));
        else
            return DecodedOpcode(location, opcode.raw, "RR", get_r_name(opcode.d.z), 2, OpcodeGroup::X8_RSB, 8, 0b1111ui8
                , get_r(opcode.d.z));
    case 4:
        // SLA
        if (opcode.d.z == 6)
            return DecodedOpcode(location, opcode.raw, "SLA", "(HL)", 2, OpcodeGroup::X8_RSB, 16, 0b1111ui8
                , make_shared<Const8BitLocation>(m_ram, get_rp(2)));
        else
            return DecodedOpcode(location, opcode.raw, "SLA", get_r_name(opcode.d.z), 2, OpcodeGroup::X8_RSB, 8, 0b1111ui8
                , get_r(opcode.d.z));
    case 5:
        // SRA
        if (opcode.d.z == 6)
            return DecodedOpcode(location, opcode.raw, "SRA", "(HL)", 2, OpcodeGroup::X8_RSB, 16, 0b1111ui8
                , make_shared<Const8BitLocation>(m_ram, get_rp(2)));
        else
            return DecodedOpcode(location, opcode.raw, "SRA", get_r_name(opcode.d.z), 2, OpcodeGroup::X8_RSB, 8, 0b1111ui8
                , get_r(opcode.d.z));
    case 6:
        // SWAP
        if (opcode.d.z == 6)
            return DecodedOpcode(location, opcode.raw, "SWAP", "(HL)", 2, OpcodeGroup::X8_RSB, 16, 0b1111ui8
                , make_shared<Const8BitLocation>(m_ram, get_rp(2)));
        else
            return DecodedOpcode(location, opcode.raw, "SWAP", get_r_name(opcode.d.z), 2, OpcodeGroup::X8_RSB, 8, 0b1111ui8
                , get_r(opcode.d.z));
    case 7:
        // SRL
        if (opcode.d.z == 6)
            return DecodedOpcode(location, opcode.raw, "SRL", "(HL)", 2, OpcodeGroup::X8_RSB, 16, 0b1111ui8
                , make_shared<Const8BitLocation>(m_ram, get_rp(2)));
        else
            return DecodedOpcode(location, opcode.raw, "SRL", get_r_name(opcode.d.z), 2, OpcodeGroup::X8_RSB, 8, 0b1111ui8
                , get_r(opcode.d.z));
    default:
        break;
    }

    return bad_op(location, opcode.raw);
}

DecodedOpcode GBZ80::perform_cb_op(const uint16_t location, uint8_t op)
{
    Opcode opcode = { op };

    switch (opcode.d.x)
    {
    case 0:
        // rot[y] r[z]
        return rot_op(location, opcode);
    case 1:
        // BIT y, r[z]
        if (opcode.d.z == 6)
            return DecodedOpcode(location, op, "BIT", simple_itoa(opcode.d.y) + ", (HL)", 2, OpcodeGroup::X8_RSB, 12, 0b0111ui8
                , make_shared<Const8BitLocation>(m_ram, get_rp(2)));
        else
            return DecodedOpcode(location, op, "BIT", simple_itoa(opcode.d.y) + ", " + get_r_name(opcode.d.z), 2, OpcodeGroup::X8_RSB, 8, 0b0111ui8
                , get_r(opcode.d.z));
    case 2:
        // RES y, r[z]
        if (opcode.d.z == 6)
            return DecodedOpcode(location, op, "RES", simple_itoa(opcode.d.y) + ", (HL)", 2, OpcodeGroup::X8_RSB, 16, 0
                , make_shared<Const8BitLocation>(m_ram, get_rp(2)));
        else
            return DecodedOpcode(location, op, "RES", simple_itoa(opcode.d.y) + ", " + get_r_name(opcode.d.z), 2, OpcodeGroup::X8_RSB, 8, 0
                , get_r(opcode.d.z));
    case 3:
        // SET y, r[z]
        if (opcode.d.z == 6)
            return DecodedOpcode(location, op, "SET", simple_itoa(opcode.d.y) + ", (HL)", 2, OpcodeGroup::X8_RSB, 16, 0
                , make_shared<Const8BitLocation>(m_ram, get_rp(2)));
        else
            return DecodedOpcode(location, op, "SET", simple_itoa(opcode.d.y) + ", " + get_r_name(opcode.d.z), 2, OpcodeGroup::X8_RSB, 8, 0
                , get_r(opcode.d.z));
    default:
        break;
    }

    return bad_op(location, op, 1);
}

// Reg16Bit()
// Reg8Bit()
// ImmediateLoad()
// Address()

DecodedOpcode GBZ80::decode_op_latest(const uint16_t location, uint8_t op)
{
    switch (op)
    {
    case 0x00ui8:
        break;
    case 0x01ui8:
        break;
    case 0x02ui8:
        break;
    case 0x03ui8:
        break;
    case 0x04ui8:
        break;
    case 0x05ui8:
        break;
    case 0x06ui8:
        break;
    case 0x07ui8:
        break;
    case 0x08ui8:
        break;
    case 0x09ui8:
        break;
    case 0x0aui8:
        break;
    case 0x0bui8:
        break;
    case 0x0cui8:
        break;
    case 0x0dui8:
        break;
    case 0x0eui8:
        break;
    case 0x0fui8:
        break;

    case 0x10ui8:
        break;
    case 0x11ui8:
        break;
    case 0x12ui8:
        break;
    case 0x13ui8:
        break;
    case 0x14ui8:
        break;
    case 0x15ui8:
        break;
    case 0x16ui8:
        break;
    case 0x17ui8:
        break;
    case 0x18ui8:
        break;
    case 0x19ui8:
        break;
    case 0x1aui8:
        break;
    case 0x1bui8:
        break;
    case 0x1cui8:
        break;
    case 0x1dui8:
        break;
    case 0x1eui8:
        break;
    case 0x1fui8:
        break;

    case 0x20ui8:
        break;
    case 0x21ui8:
        break;
    case 0x22ui8:
        break;
    case 0x23ui8:
        break;
    case 0x24ui8:
        break;
    case 0x25ui8:
        break;
    case 0x26ui8:
        break;
    case 0x27ui8:
        break;
    case 0x28ui8:
        break;
    case 0x29ui8:
        break;
    case 0x2aui8:
        break;
    case 0x2bui8:
        break;
    case 0x2cui8:
        break;
    case 0x2dui8:
        break;
    case 0x2eui8:
        break;
    case 0x2fui8:
        break;


    case 0x30ui8:
        break;
    case 0x31ui8:
        break;
    case 0x32ui8:
        break;
    case 0x33ui8:
        break;
    case 0x34ui8:
        break;
    case 0x35ui8:
        break;
    case 0x36ui8:
        break;
    case 0x37ui8:
        break;
    case 0x38ui8:
        break;
    case 0x39ui8:
        break;
    case 0x3aui8:
        break;
    case 0x3bui8:
        break;
    case 0x3cui8:
        break;
    case 0x3dui8:
        break;
    case 0x3eui8:
        break;
    case 0x3fui8:
        break;

    case 0x40ui8:
        break;
    case 0x41ui8:
        break;
    case 0x42ui8:
        break;
    case 0x43ui8:
        break;
    case 0x44ui8:
        break;
    case 0x45ui8:
        break;
    case 0x46ui8:
        break;
    case 0x47ui8:
        break;
    case 0x48ui8:
        break;
    case 0x49ui8:
        break;
    case 0x4aui8:
        break;
    case 0x4bui8:
        break;
    case 0x4cui8:
        break;
    case 0x4dui8:
        break;
    case 0x4eui8:
        break;
    case 0x4fui8:
        break;

    case 0x50ui8:
        break;
    case 0x51ui8:
        break;
    case 0x52ui8:
        break;
    case 0x53ui8:
        break;
    case 0x54ui8:
        break;
    case 0x55ui8:
        break;
    case 0x56ui8:
        break;
    case 0x57ui8:
        break;
    case 0x58ui8:
        break;
    case 0x59ui8:
        break;
    case 0x5aui8:
        break;
    case 0x5bui8:
        break;
    case 0x5cui8:
        break;
    case 0x5dui8:
        break;
    case 0x5eui8:
        break;
    case 0x5fui8:
        break;

    case 0x60ui8:
        break;
    case 0x61ui8:
        break;
    case 0x62ui8:
        break;
    case 0x63ui8:
        break;
    case 0x64ui8:
        break;
    case 0x65ui8:
        break;
    case 0x66ui8:
        break;
    case 0x67ui8:
        break;
    case 0x68ui8:
        break;
    case 0x69ui8:
        break;
    case 0x6aui8:
        break;
    case 0x6bui8:
        break;
    case 0x6cui8:
        break;
    case 0x6dui8:
        break;
    case 0x6eui8:
        break;
    case 0x6fui8:
        break;

    case 0x70ui8:
        break;
    case 0x71ui8:
        break;
    case 0x72ui8:
        break;
    case 0x73ui8:
        break;
    case 0x74ui8:
        break;
    case 0x75ui8:
        break;
    case 0x76ui8:
        break;
    case 0x77ui8:
        break;
    case 0x78ui8:
        break;
    case 0x79ui8:
        break;
    case 0x7aui8:
        break;
    case 0x7bui8:
        break;
    case 0x7cui8:
        break;
    case 0x7dui8:
        break;
    case 0x7eui8:
        break;
    case 0x7fui8:
        break;

    case 0x80ui8:
        break;
    case 0x81ui8:
        break;
    case 0x82ui8:
        break;
    case 0x83ui8:
        break;
    case 0x84ui8:
        break;
    case 0x85ui8:
        break;
    case 0x86ui8:
        break;
    case 0x87ui8:
        break;
    case 0x88ui8:
        break;
    case 0x89ui8:
        break;
    case 0x8aui8:
        break;
    case 0x8bui8:
        break;
    case 0x8cui8:
        break;
    case 0x8dui8:
        break;
    case 0x8eui8:
        break;
    case 0x8fui8:
        break;

    case 0x90ui8:
        break;
    case 0x91ui8:
        break;
    case 0x92ui8:
        break;
    case 0x93ui8:
        break;
    case 0x94ui8:
        break;
    case 0x95ui8:
        break;
    case 0x96ui8:
        break;
    case 0x97ui8:
        break;
    case 0x98ui8:
        break;
    case 0x99ui8:
        break;
    case 0x9aui8:
        break;
    case 0x9bui8:
        break;
    case 0x9cui8:
        break;
    case 0x9dui8:
        break;
    case 0x9eui8:
        break;
    case 0x9fui8:
        break;

    case 0xa0ui8:
        break;
    case 0xa1ui8:
        break;
    case 0xa2ui8:
        break;
    case 0xa3ui8:
        break;
    case 0xa4ui8:
        break;
    case 0xa5ui8:
        break;
    case 0xa6ui8:
        break;
    case 0xa7ui8:
        break;
    case 0xa8ui8:
        break;
    case 0xa9ui8:
        break;
    case 0xaaui8:
        break;
    case 0xabui8:
        break;
    case 0xacui8:
        break;
    case 0xadui8:
        break;
    case 0xaeui8:
        break;
    case 0xafui8:
        break;

    case 0xb0ui8:
        break;
    case 0xb1ui8:
        break;
    case 0xb2ui8:
        break;
    case 0xb3ui8:
        break;
    case 0xb4ui8:
        break;
    case 0xb5ui8:
        break;
    case 0xb6ui8:
        break;
    case 0xb7ui8:
        break;
    case 0xb8ui8:
        break;
    case 0xb9ui8:
        break;
    case 0xbaui8:
        break;
    case 0xbbui8:
        break;
    case 0xbcui8:
        break;
    case 0xbdui8:
        break;
    case 0xbeui8:
        break;
    case 0xbfui8:
        break;

    case 0xc0ui8:
        break;
    case 0xc1ui8:
        break;
    case 0xc2ui8:
        break;
    case 0xc3ui8:
        break;
    case 0xc4ui8:
        break;
    case 0xc5ui8:
        break;
    case 0xc6ui8:
        break;
    case 0xc7ui8:
        break;
    case 0xc8ui8:
        break;
    case 0xc9ui8:
        break;
    case 0xcaui8:
        break;
    case 0xcbui8:
        break;
    case 0xccui8:
        break;
    case 0xcdui8:
        break;
    case 0xceui8:
        break;
    case 0xcfui8:
        break;

    case 0xd0ui8:
        break;
    case 0xd1ui8:
        break;
    case 0xd2ui8:
        break;
    case 0xd3ui8:
        break;
    case 0xd4ui8:
        break;
    case 0xd5ui8:
        break;
    case 0xd6ui8:
        break;
    case 0xd7ui8:
        break;
    case 0xd8ui8:
        break;
    case 0xd9ui8:
        break;
    case 0xdaui8:
        break;
    case 0xdbui8:
        break;
    case 0xdcui8:
        break;
    case 0xddui8:
        break;
    case 0xdeui8:
        break;
    case 0xdfui8:
        break;

    case 0xe0ui8:
        break;
    case 0xe1ui8:
        break;
    case 0xe2ui8:
        break;
    case 0xe3ui8:
        break;
    case 0xe4ui8:
        break;
    case 0xe5ui8:
        break;
    case 0xe6ui8:
        break;
    case 0xe7ui8:
        break;
    case 0xe8ui8:
        break;
    case 0xe9ui8:
        break;
    case 0xeaui8:
        break;
    case 0xebui8:
        break;
    case 0xecui8:
        break;
    case 0xedui8:
        break;
    case 0xeeui8:
        break;
    case 0xefui8:
        break;

    case 0xf0ui8:
        break;
    case 0xf1ui8:
        break;
    case 0xf2ui8:
        break;
    case 0xf3ui8:
        break;
    case 0xf4ui8:
        break;
    case 0xf5ui8:
        break;
    case 0xf6ui8:
        break;
    case 0xf7ui8:
        break;
    case 0xf8ui8:
        break;
    case 0xf9ui8:
        break;
    case 0xfaui8:
        break;
    case 0xfbui8:
        break;
    case 0xfcui8:
        break;
    case 0xfdui8:
        break;
    case 0xfeui8:
        break;
    case 0xffui8:
        break;

    default:
        return bad_op(location, op);
    }
}