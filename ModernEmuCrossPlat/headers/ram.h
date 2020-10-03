#pragma once

class MemoryMap
{
public:
    MemoryMap();

    inline const uint8_t read(const uint16_t location)
    {
        return m_memory[location];
    }

    inline void write(const uint16_t location, const uint8_t val)
    {
        m_memory[location] = val;
    }

private:
    std::array<uint8_t, size_t(0x10000)> m_memory;
};