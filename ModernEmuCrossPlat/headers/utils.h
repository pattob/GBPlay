#pragma once

constexpr const size_t num_digits(const size_t x, const size_t base)
{
    return (x < base) ? 1 : 1 + num_digits(x / base, base);
}

template<class _Ty>
constexpr const size_t number_in_base(const size_t base)
{
    return num_digits(static_cast<size_t>(std::numeric_limits<_Ty>::max()), base);
}

template<
    class _Ty,
    const size_t _B,
    const size_t values = number_in_base<_Ty>(_B)>
constexpr auto itoa_base(const _Ty val)
{
    _Ty valCopy(val);
    char data[values + 1] = { '\0' };
    for (auto i = values; i > 0; --i)
    {
        if ((val % static_cast<_Ty>(_B)) >= 0 && (val % static_cast<_Ty>(_B)) <= 9)
            data[i] = '0' + (val % static_cast<_Ty>(_B));
        else if ((val % static_cast<_Ty>(_B)) >= 0xA && (val % static_cast<_Ty>(_B)) <= 0xF)
            data[i] = 'A' + ((val % static_cast<_Ty>(_B)) - 0xA);

        valCopy = valCopy / static_cast<_Ty>(_B);
    }

    return data;
}

template<class _Ty>
constexpr auto itoa_16(const _Ty val)
{
    return itoa_base<_Ty, 16ull>(val);
}

template<class _Ty>
constexpr auto itoa_10(const _Ty val)
{
    return itoa_base<_Ty, 10ull>(val);
}