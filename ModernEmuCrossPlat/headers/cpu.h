#pragma once

#include "utils.h"

#include "ram.h"


union Opcode
{
    uint8_t raw;
    struct directed
    {
        uint8_t z : 3; // 000, 001, 010, 011, 100, 101, 110, 111
        uint8_t y : 3; // 000, 001, 010, 011, 100, 101, 110, 111
        uint8_t x : 2; // 00(0), 01(1), 10(2), 11(3)
        // [ xx yyy zzz]
        // 0x41 == 0100 0001 == 01 000 001

        uint8_t p() {
            return (y >> 1);
        }

        uint8_t q() {
            return y & 1;
        }
    } d;
};

enum class OpcodeGroup
{
    X8_LSM,
    X16_LSM,
    X8_ALU,
    X16_ALU,
    X8_RSB,
    CTL_BR,
    CTL_MISC,
    UNKNOWN
};

inline const std::string op_group_to_str(const OpcodeGroup group)
{
    switch (group)
    {
    case OpcodeGroup::X8_LSM:
        return "X8_LSM";
    case OpcodeGroup::X16_LSM:
        return "X16_LSM";
    case OpcodeGroup::X8_ALU:
        return "X8_ALU";
    case OpcodeGroup::X16_ALU:
        return "X16_ALU";
    case OpcodeGroup::X8_RSB:
        return "X8_RSB";
    case OpcodeGroup::CTL_BR:
        return "CTL_BR";
    case OpcodeGroup::CTL_MISC:
        return "CTL_MISC";
    case OpcodeGroup::UNKNOWN:
        return "UNKNOWN";
    default:
        return "BAD";
    }
}

class NamedVal
{
public:
    virtual const char* name() const = 0;
};

template<class _InputT>
class NoneVal : public NamedVal
{
public:

    NoneVal() noexcept
    {
    }

    NoneVal<_InputT>& operator=(NoneVal<_InputT>&& other)
    {
        other;
        return *this;
    }

    NoneVal(Reg<_InputT>&& other) noexcept
    {
        *this = &other;
    }

    const char* name() const
    {
        return "";
    }
};

template<class _InputT,
    class = typename std::enable_if<std::is_same<_InputT, uint8_t>::value || std::is_same<_InputT, uint16_t>::value>>
class Reg : public NamedVal
{
public:

    Reg(_InputT* reg, const char* name) noexcept
        : m_reg(reg)
        , m_name(name)
    {
    }

    Reg() noexcept
        : Reg(nullptr, nullptr)
    {
    }

    Reg<_InputT>& operator=(Reg<_InputT>&& other)
    {
        if (this != &other)
        {
            m_reg = other.m_reg;
            m_name = other.m_name;

            other.m_reg = nullptr;
            other.m_name = nullptr;
        }
        return *this;
    }

    Reg(Reg<_InputT>&& other) noexcept
        : Reg()
    {
        *this = std::move(other);
    }

    const char* name() const
    {
        return m_name;
    }

    const _InputT read() const
    {
        return (*m_reg);
    }

    const bool writable() const
    {
        return m_reg != nullptr;
    }

    void write(const _InputT& val)
    {
        (*m_reg) = val;
    }

private:
    _InputT* m_reg;
    const char* m_name;
};

template<class _InputT,
    class = typename std::enable_if<std::is_same<_InputT, uint8_t>::value || std::is_same<_InputT, uint16_t>::value>>
class Immidiate : public NamedVal
{
public:

    Immidiate(const _InputT& val) noexcept
        : m_val(val)
    {
    }

    Immidiate() noexcept
        : Immidiate(static_cast<_InputT>(0))
    {
    }

    Immidiate<_InputT>& operator=(Immidiate<_InputT>&& other)
    {
        if (this != &other)
        {
            m_val = other.m_val;
        }

        return *this;
    }

    Immidiate(Immidiate<_InputT>&& other) noexcept
        : Immidiate()
    {
        *this = std::move(other);
    }

    const char* name() const
    {
        return itoa_base<_InputT, 16ull>(m_val);
    }

    const _InputT read() const
    {
        return m_val;
    }

    const bool writable() const
    {
        return false;
    }

    void write(const _InputT& val)
    {
    }

private:
    _InputT m_val;
};

template<class _InputRef,
    class = typename std::enable_if<(std::is_same<_InputRef, Immidiate<uint16_t>>::value || std::is_same<_InputRef, Reg<uint16_t>>::value)>>
class Address : public NamedVal
{
public:

    Address(const _InputRef& addressLocation, std::shared_ptr<MemoryMap> ram) noexcept
        : m_addressLocation(addressLocation)
        , m_ram(ram)
    {
    }

    Address() noexcept
        : Address(_InputRef(), nullptr)
    {
    }

    Address<_InputRef>& operator=(Address<_InputRef>&& other)
    {
        if (this != &other)
        {
            m_addressLocation = std::move(other.m_addressLocation);
            m_ram = std::move(other.m_ram);
        }

        return *this;
    }

    Address(Address<_InputRef>&& other)
        : Address()
    {
        *this = std::move(other);
    }

    const char* name() const
    {
        return ("(" + string(m_addressLocation.name()) + ")");
    }

    template<typename _InputT,
        class = typename std::enable_if<std::is_same<_InputT, uint8_t>::value>>
        const uint8_t read() const
    {
        return m_ram->read(m_addressLocation.read());
    }

    template<typename _InputT,
        class = typename std::enable_if<std::is_same<_InputT, uint16_t>::value>>
        const uint16_t read()
    {
        return (static_cast<uint16_t>(m_ram->read(m_addressLocation.read())) << 8)
            | static_cast<uint16_t>(m_ram->read(m_addressLocation.read() + 1ui16));
    }

    const bool writable() const
    {
        return true;
    }

    template<class _InputT,
        class = typename std::enable_if<(std::is_same<_InputT, uint8_t>::value || std::is_same<_InputT, uint16_t>::value)>>
        void write(const _InputT& val)
    {
        if (std::is_same_v<_InputT, uint8_t>)
            m_ram->write(m_addressLocation.read(), val);

        if (std::is_same_v<_InputT, uint16_t>)
        {
            m_ram->write(m_addressLocation.read(), static_cast<uint8_t>(val >> 8) & 0xFFui8);
            m_ram->write(m_addressLocation.read() + 1ui16, static_cast<uint8_t>(val & 0xFFui16));
        }
    }

private:
    _InputRef m_addressLocation;
    std::shared_ptr<MemoryMap> m_ram;
};


class DecodedOpcode
{
public:

    DecodedOpcode(
        const uint16_t& location,
        const uint8_t& op,
        const std::string& name,
        const std::string& extras,
        const uint8_t& length,
        const OpcodeGroup& group,
        const size_t& timing,
        const uint8_t& flags,
        const NamedVal& op1,
        const NamedVal& op2)
        : location(location)
        , op(op)
        , name(name)
        , extras(extras)
        , length(length)
        , group(group)
        , min_timing(timing)
        , max_timing(timing)
        , flags({ flags })
        , operandOne(op1)
        , operandTwo(op2)
    {
    }

    DecodedOpcode(
        const uint16_t& location,
        const uint8_t& op,
        const std::string& name,
        const std::string& extras,
        const uint8_t& length,
        const OpcodeGroup& group,
        const size_t& timing,
        const uint8_t& flags,
        const NamedVal& op1)
        : DecodedOpcode(
            location,
            op,
            name,
            extras,
            length,
            group,
            timing,
            flags,
            op1,
            NoneVal<uint8_t>())
    {
    }

    DecodedOpcode(
        const uint16_t& location,
        const uint8_t& op,
        const std::string& name,
        const std::string& extras,
        const uint8_t& length,
        const OpcodeGroup& group,
        const size_t& timing,
        const uint8_t& flags)
        : DecodedOpcode(
            location,
            op,
            name,
            extras,
            length,
            group,
            timing,
            flags,
            NoneVal<uint8_t>())
    {
    }

    inline const std::string tostring() const
    {
        std::stringstream str_stream;
        str_stream << uint16_helper_hex.digits[location];
        str_stream << " -- " << uint8_helper_hex.digits[op];
        str_stream << " - " << name << " - ";
        str_stream << " - " << uint8_helper_hex.digits[length] << " - ";
        str_stream << " - " << extras << " - ";

        if (flags.f.Z)
            str_stream << "Z";
        else
            str_stream << "-";

        if (flags.f.N)
            str_stream << "N";
        else
            str_stream << "-";


        if (flags.f.H)
            str_stream << "H";
        else
            str_stream << "-";

        if (flags.f.C)
            str_stream << "C";
        else
            str_stream << "-";

        if (m_val8One)
            str_stream << " - U8_1 - " << m_val8One->tostring();
        if (m_val8Two)
            str_stream << " - U8_2 - " << m_val8Two->tostring();
        if (m_val16One)
            str_stream << " - U16_1 - " << m_val16One->tostring();
        if (m_val16Two)
            str_stream << " - U16_2 - " << m_val16Two->tostring();
        if (m_sval8One)
            str_stream << " - S8_1 - " << m_sval8One->tostring();

        str_stream << " - MIN = " << min_timing;
        str_stream << " - MAX = " << max_timing;

        return str_stream.str();
    }

    inline const bool GetFlag(const size_t& flag) const
    {
        if (flag == 0)
            return (flags.f.Z > 0);
        if (flag == 1)
            return (flags.f.N > 0);
        if (flag == 2)
            return (flags.f.H > 0);
        if (flag == 3)
            return (flags.f.C > 0);
    }

    inline const std::string GetOprand1Name() const
    {
        if (m_val8One)
            return m_val8One->tostring();

        if (m_val16One)
            return m_val16One->tostring();

        if (m_sval8One)
            return m_sval8One->tostring();

        return "";
    }

    inline const std::string GetOprand2Name() const
    {
        if (m_val8Two)
            return m_val8Two->tostring();

        if (m_val16Two)
            return m_val16Two->tostring();

        return "";
    }

private:
    uint16_t location;
    uint8_t op;
    const std::string name;
    const std::string extras;
    uint8_t length;
    OpcodeGroup group;
    size_t min_timing;
    size_t max_timing;
    union F {
        uint8_t raw;
        struct f {
            uint8_t Z : 1;
            uint8_t N : 1;
            uint8_t H : 1;
            uint8_t C : 1;
            uint8_t remains : 4;
        } f;
    } flags;
    NamedVal operandOne;
    NamedVal operandTwo;
};
/*
struct DecodedOpcode
{
    DecodedOpcode(
        const uint16_t &location,
        const uint8_t &op,
        const std::string &name,
        const std::string &extras,
        const uint8_t &length,
        const OpcodeGroup &group,
        const size_t &timing,
        const uint8_t &flags) :
        DecodedOpcode(
            location,
            op,
            name,
            extras,
            length,
            group,
            timing,
            timing,
            flags,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr)
    {}

    DecodedOpcode(
        const uint16_t& location,
        const uint8_t& op,
        const std::string& name,
        const std::string& extras,
        const uint8_t& length,
        const OpcodeGroup& group,
        const size_t& timing,
        const uint8_t& flags,
        shared_ptr<IReadWrite<uint8_t>> val8One,
        shared_ptr<IReadWrite<uint8_t>> val8Two) :
        DecodedOpcode(
            location,
            op,
            name,
            extras,
            length,
            group,
            timing,
            timing,
            flags,
            val8One,
            val8Two,
            nullptr,
            nullptr,
            nullptr)
    {}

    DecodedOpcode(
        const uint16_t& location,
        const uint8_t& op,
        const std::string& name,
        const std::string& extras,
        const uint8_t& length,
        const OpcodeGroup& group,
        const size_t& timing,
        const uint8_t& flags,
        shared_ptr<IReadWrite<uint8_t>> val8One) :
        DecodedOpcode(
            location,
            op,
            name,
            extras,
            length,
            group,
            timing,
            timing,
            flags,
            val8One,
            nullptr,
            nullptr,
            nullptr,
            nullptr)
    {}

    DecodedOpcode(
        const uint16_t& location,
        const uint8_t& op,
        const std::string& name,
        const std::string& extras,
        const uint8_t& length,
        const OpcodeGroup& group,
        const size_t& timing,
        const uint8_t& flags,
        shared_ptr<IReadWrite<int8_t>> sval8One) :
        DecodedOpcode(
            location,
            op,
            name,
            extras,
            length,
            group,
            timing,
            timing,
            flags,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            sval8One)
    {}

    DecodedOpcode(
        const uint16_t& location,
        const uint8_t& op,
        const std::string& name,
        const std::string& extras,
        const uint8_t& length,
        const OpcodeGroup& group,
        const size_t& timing,
        const uint8_t& flags,
        shared_ptr<IReadWrite<uint16_t>> val16One,
        shared_ptr<IReadWrite<uint16_t>> val16Two) :
        DecodedOpcode(
            location,
            op,
            name,
            extras,
            length,
            group,
            timing,
            timing,
            flags,
            nullptr,
            nullptr,
            val16One,
            val16Two,
            nullptr)
    {}

    DecodedOpcode(
        const uint16_t& location,
        const uint8_t& op,
        const std::string& name,
        const std::string& extras,
        const uint8_t& length,
        const OpcodeGroup& group,
        const size_t& timing,
        const uint8_t& flags,
        shared_ptr<IReadWrite<uint16_t>> val16One) :
        DecodedOpcode(
            location,
            op,
            name,
            extras,
            length,
            group,
            timing,
            timing,
            flags,
            nullptr,
            nullptr,
            val16One,
            nullptr,
            nullptr)
    {}

    DecodedOpcode(
        const uint16_t& location,
        const uint8_t& op,
        const std::string& name,
        const std::string& extras,
        const uint8_t& length,
        const OpcodeGroup& group,
        const size_t& min_timing,
        const size_t& max_timing,
        const uint8_t& flags,
        shared_ptr<IReadWrite<uint8_t>> val8One,
        shared_ptr<IReadWrite<uint8_t>> val8Two)
        : DecodedOpcode(
            location,
            op,
            name,
            extras,
            length,
            group,
            min_timing,
            max_timing,
            flags,
            val8One,
            val8Two,
            nullptr,
            nullptr,
            nullptr)
    {}

    DecodedOpcode(
        const uint16_t& location,
        const uint8_t& op,
        const std::string& name,
        const std::string& extras,
        const uint8_t& length,
        const OpcodeGroup& group,
        const size_t& min_timing,
        const size_t& max_timing,
        const uint8_t& flags,
        shared_ptr<IReadWrite<uint8_t>> val8One)
        : DecodedOpcode(
            location,
            op,
            name,
            extras,
            length,
            group,
            min_timing,
            max_timing,
            flags,
            val8One,
            nullptr,
            nullptr,
            nullptr,
            nullptr)
    {}

    DecodedOpcode(
        const uint16_t& location,
        const uint8_t& op,
        const std::string& name,
        const std::string& extras,
        const uint8_t& length,
        const OpcodeGroup& group,
        const size_t& min_timing,
        const size_t& max_timing,
        const uint8_t& flags,
        shared_ptr<IReadWrite<int8_t>> sval8One)
        : DecodedOpcode(
            location,
            op,
            name,
            extras,
            length,
            group,
            min_timing,
            max_timing,
            flags,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            sval8One)
    {}

    DecodedOpcode(
        const uint16_t& location,
        const uint8_t& op,
        const std::string& name,
        const std::string& extras,
        const uint8_t& length,
        const OpcodeGroup& group,
        const size_t& min_timing,
        const size_t& max_timing,
        const uint8_t& flags,
        shared_ptr<IReadWrite<uint16_t>> val16One,
        shared_ptr<IReadWrite<uint16_t>> val16Two)
        : DecodedOpcode(
            location,
            op,
            name,
            extras,
            length,
            group,
            min_timing,
            max_timing,
            flags,
            nullptr,
            nullptr,
            val16One,
            val16Two,
            nullptr)
    {}

    DecodedOpcode(
        const uint16_t& location,
        const uint8_t& op,
        const std::string& name,
        const std::string& extras,
        const uint8_t& length,
        const OpcodeGroup& group,
        const size_t& min_timing,
        const size_t& max_timing,
        const uint8_t& flags,
        shared_ptr<IReadWrite<uint16_t>> val16One)
        : DecodedOpcode(
            location,
            op,
            name,
            extras,
            length,
            group,
            min_timing,
            max_timing,
            flags,
            nullptr,
            nullptr,
            val16One,
            nullptr,
            nullptr)
    {}

    DecodedOpcode(
        const uint16_t& location,
        const uint8_t& op,
        const std::string& name,
        const std::string& extras,
        const uint8_t& length,
        const OpcodeGroup& group,
        const size_t& min_timing,
        const size_t& max_timing,
        const uint8_t& flags)
        : DecodedOpcode(
            location,
            op,
            name,
            extras,
            length,
            group,
            min_timing,
            max_timing,
            flags,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr)
    {}

    DecodedOpcode(
        const uint16_t &location,
        const uint8_t &op,
        const std::string &name,
        const std::string& extras,
        const uint8_t &length,
        const OpcodeGroup &group,
        const size_t &min_timing,
        const size_t &max_timing,
        const uint8_t &flags,
        shared_ptr<IReadWrite<uint8_t>> val8One,
        shared_ptr<IReadWrite<uint8_t>> val8Two,
        shared_ptr<IReadWrite<uint16_t>> val16One,
        shared_ptr<IReadWrite<uint16_t>> val16Two,
        shared_ptr<IReadWrite<int8_t>> sval8One)
        : location(location)
        , op(op)
        , name(name)
        , extras(extras)
        , length(length)
        , group(group)
        , min_timing(min_timing)
        , max_timing(max_timing)
        , flags({ flags })
        , m_val8One(val8One)
        , m_val8Two(val8Two)
        , m_val16One(val16One)
        , m_val16Two(val16Two)
        , m_sval8One(sval8One)
    {}

    inline const std::string tostring() const
    {
        std::stringstream str_stream;
        str_stream << uint16_helper_hex.digits[location];
        str_stream << " -- " << uint8_helper_hex.digits[op];
        str_stream << " - " << name << " - ";
        str_stream << " - " << uint8_helper_hex.digits[length] << " - ";
        str_stream << " - " << extras << " - ";

        if (flags.f.Z)
            str_stream << "Z";
        else
            str_stream << "-";

        if (flags.f.N)
            str_stream << "N";
        else
            str_stream << "-";


        if (flags.f.H)
            str_stream << "H";
        else
            str_stream << "-";

        if (flags.f.C)
            str_stream << "C";
        else
            str_stream << "-";

        if (m_val8One)
            str_stream << " - U8_1 - " << m_val8One->tostring();
        if (m_val8Two)
            str_stream << " - U8_2 - " << m_val8Two->tostring();
        if (m_val16One)
            str_stream << " - U16_1 - " << m_val16One->tostring();
        if (m_val16Two)
            str_stream << " - U16_2 - " << m_val16Two->tostring();
        if (m_sval8One)
            str_stream << " - S8_1 - " << m_sval8One->tostring();

        str_stream << " - MIN = " << min_timing;
        str_stream << " - MAX = " << max_timing;

        return str_stream.str();
    }

    inline const bool GetFlag(const size_t& flag) const
    {
        if (flag == 0)
            return (flags.f.Z > 0);
        if (flag == 1)
            return (flags.f.N > 0);
        if (flag == 2)
            return (flags.f.H > 0);
        if (flag == 3)
            return (flags.f.C > 0);
    }

    inline const std::string GetOprand1Name() const
    {
        if (m_val8One)
            return m_val8One->tostring();

        if (m_val16One)
            return m_val16One->tostring();

        if (m_sval8One)
            return m_sval8One->tostring();

        return "";
    }

    inline const std::string GetOprand2Name() const
    {
        if (m_val8Two)
            return m_val8Two->tostring();

        if (m_val16Two)
            return m_val16Two->tostring();

        return "";
    }

    uint16_t location;
    uint8_t op;
    const std::string name;
    const std::string extras;
    uint8_t length;
    OpcodeGroup group;
    size_t min_timing;
    size_t max_timing;
    union F {
        uint8_t raw;
        struct f {
            uint8_t Z : 1;
            uint8_t N : 1;
            uint8_t H : 1;
            uint8_t C : 1;
            uint8_t remains : 4;
        } f;
    } flags;

    shared_ptr<IReadWrite<uint8_t>> m_val8One;
    shared_ptr<IReadWrite<uint8_t>> m_val8Two;

    shared_ptr<IReadWrite<uint16_t>> m_val16One;
    shared_ptr<IReadWrite<uint16_t>> m_val16Two;

    shared_ptr<IReadWrite<int8_t>> m_sval8One;
};
*/
const std::string reg_r_names[] = { "B", "C", "D", "E", "H", "L", "A", "F" };

inline const std::string get_r_name(const uint8_t idx)
{
    return reg_r_names[idx];
}

const std::string reg_rp_names[] = { "BC", "DE", "HL" };

inline const std::string get_rp_name(const uint8_t idx)
{
    if (idx >= 0 && idx <= 2)
        return reg_rp_names[idx];
    else
        return "SP";
}

inline const std::string get_rp2_name(const uint8_t idx)
{
    if (idx >= 0 && idx <= 2)
        return reg_rp_names[idx];
    else
        return "AF";
}

const std::string cc_names[] = {"NZ", "Z", "NC", "C" };

inline const std::string get_cc(const uint8_t idx)
{
    return cc_names[idx];
}

inline const std::string simple_itoa(const uint8_t idx)
{
    return itoa_16(idx);
}

inline const std::string itohex(const uint8_t val)
{
    return itoa_16(val);
}

struct Nop
{
    static constexpr uint16_t length = 1;
    static constexpr size_t   cycles = 4;
};

class GBZ80
{
public:
    GBZ80(shared_ptr<MemoryMap> ram);

    DecodedOpcode decode_op(const uint16_t location, uint8_t op);

    DecodedOpcode decode_op_latest(const uint16_t location, uint8_t op);

    inline const uint8_t get_op_at_pc()
    {
        return m_ram->read(PC);
    }

    inline const uint16_t get_pc()
    {
        return PC;
    }

    inline void set_pc(const uint16_t new_pc)
    {
        PC = new_pc;
    }

private:

    DecodedOpcode alu_op(const uint16_t location, Opcode opcode);

    DecodedOpcode rot_op(const uint16_t location, Opcode opcode);

    DecodedOpcode perform_cb_op(const uint16_t location, uint8_t op);

    inline shared_ptr<RegDirectOffset> get_r_and_offset(uint16_t offset, uint8_t idx)
    {
        if (idx <= 5)
            return make_shared<RegDirectOffset>(offset, ref_u8[idx]);
        else if (idx == 7)
            return make_shared<RegDirectOffset>(offset, ref_u8[6]);
        else
        {
            // TODO: Still need to figure out (HL)
            // ERROR
            //std::cout << "HOW DO WE HANDLE THIS!!!! - " << static_cast<int>(idx) << std::endl;
            return make_shared<RegDirectOffset>(offset, nullptr);
        }
    }

    inline shared_ptr<RegDirect<uint8_t>> get_r(uint8_t idx) const
    {
        if (idx <= 6)
            return ref_u8[idx];
        //return RegDirect<uint8_t>(get_r_name(idx), &(r.u8_a[idx]));
        else if (idx == 7)
            return ref_u8[6];
            //return RegDirect<uint8_t>(get_r_name(idx), &(r.u8_a[6]));
        else
        {
            // TODO: Still need to figure out (HL)
            // ERROR
            //std::cout << "HOW DO WE HANDLE THIS!!!! - " << static_cast<int>(idx) << std::endl;
            return nullptr;
            //return RegDirect<uint8_t>(get_r_name(idx), nullptr);
        }
    }

    inline shared_ptr<RegDirect<uint16_t>> get_rp(uint8_t idx) const
    {
        if (idx < 3)
            return ref_u16[idx];
        else
            return ref_u16[5];
        // something with (HL) I think?
    }

    inline shared_ptr<RegDirect<uint16_t>> get_rp2(uint8_t idx) const
    {
        return ref_u16[idx];
        //return RegDirect<uint16_t>(get_rp2_name(idx), &(r.u16_a[idx]));
    }

    std::array<shared_ptr<RegDirect<uint16_t>>, 6> ref_u16;
    std::array<shared_ptr<RegDirect<uint8_t>>, 8> ref_u8;

    union registers
    {
        struct u8_reg {
            uint8_t B; // 0
            uint8_t C; // 1
            uint8_t D; // 2
            uint8_t E; // 3
            uint8_t H; // 4
            uint8_t L; // 5
            uint8_t A; // 6
            union F_reg {
                uint8_t v;
                struct flags {
                    uint8_t remains : 4;
                    uint8_t C : 1;
                    uint8_t H : 1;
                    uint8_t N : 1;
                    uint8_t Z : 1;
                } flags;
            } F; // 7
        } u8;
        struct {
            uint8_t u8_a[8];
            auto B() { return Reg(&(u8_a[0])); }
            auto C() { return Reg(&(u8_a[1])); }
            auto D() { return Reg(&(u8_a[2])); }
            auto E() { return Reg(&(u8_a[3])); }
            auto H() { return Reg(&(u8_a[4])); }
            auto L() { return Reg(&(u8_a[5])); }
            auto A() { return Reg(&(u8_a[6])); }
            auto F() { return Reg(&(u8_a[7])); }
        } ref_u8;
        struct u16_reg {
            uint16_t BC; // 0
            uint16_t DE; // 1
            uint16_t HL; // 2
            uint16_t AF; // 3
        } u16;
        struct {
            uint16_t u16_a[4];
            auto BC() { return Reg(&(u16_a[0])); }
            auto DE() { return Reg(&(u16_a[1])); }
            auto HL() { return Reg(&(u16_a[2])); }
            auto AF() { return Reg(&(u16_a[3])); }
        } ref_u16;
    } r;

    uint16_t SP;
    uint16_t PC;

    std::shared_ptr<MemoryMap> m_ram;
};



// 0000 CHNZ