#pragma once

#include "utils.h"

class ParsedOpCode
{
public:

    ParsedOpCode(
        const std::uint8_t& addr,
        const std::string& mnemonic,
        const std::size_t& length) :
        m_addr(addr),
        m_mnemonic(mnemonic),
        m_length(length),
        m_cycles(),
        m_flags(),
        m_operand1(""),
        m_operand2("")
    {
    }

    void SetOperand1(const std::string& operand)
    {
        m_operand1.assign(operand);
    }

    void SetOperand2(const std::string& operand)
    {
        m_operand2.assign(operand);
    }

    template<class Iter>
    inline void SetCycles(
        const Iter& first,
        const Iter& last)
    {
        std::copy(
            first,
            last,
            std::begin(m_cycles));

        std::sort(
            std::rbegin(m_cycles),
            std::rend(m_cycles));
    }
    
    template<class Iter>
    inline void SetFlags(
        const Iter& first,
        const Iter& last)
    {
        std::copy(
            first,
            last,
            begin(m_flags));
    }

    inline const std::uint8_t GetAddr() const
    {
        return m_addr;
    }

    inline const std::string GetMnemonic() const
    {
        return m_mnemonic;
    }

    inline const std::size_t GetLength() const
    {
        return m_length;
    }

    inline const std::string GetOperand1() const
    {
        return m_operand1;
    }

    inline const std::string GetOperand2() const
    {
        return m_operand2;
    }

    inline const bool GetFlag(const size_t& flag) const
    {
        return (m_flags[flag] != '-');
    }

    inline const size_t MaxCycles() const
    {
        return m_cycles[0];
    }

    inline const size_t MinCycles() const
    {
        return m_cycles[1];
    }

    inline const std::string tostring() const
    {
        std::stringstream str_stream;

        str_stream << uint8_helper_hex.digits[m_addr];
        str_stream << " - " << m_mnemonic << " - ";
        //str_stream << " - " << m_length << " - ";
        //str_stream << m_flags[0] << m_flags[1] << m_flags[2] << m_flags[3];
        //str_stream << " --- ";
        if (!m_operand1.empty())
            str_stream << m_operand1;

        if (!m_operand2.empty())
            str_stream << " - " << m_operand2;

        str_stream << " - MIN = " << MinCycles();
        str_stream << " - MAX = " << MaxCycles();

        return str_stream.str();
    }

private:
    std::uint8_t m_addr;
    std::string m_mnemonic;
    std::size_t m_length;
    std::array<size_t, 2> m_cycles;
    std::array<char, 4> m_flags;
    std::string m_operand1;
    std::string m_operand2;
};

std::map<uint8_t, std::shared_ptr<ParsedOpCode>> ParsedOpCodes();