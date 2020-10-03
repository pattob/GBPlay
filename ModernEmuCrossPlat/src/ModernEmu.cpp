// ModernEmu.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

#include "parsed_op_codes.h"
#include "cpu.h"

int main()
{
    auto opCodes = ParsedOpCodes();
    auto memory = make_shared<MemoryMap>();
    GBZ80 gba_cpu(memory);

    //while (gba_cpu.get_pc() < 0x10000)
    {
        for(const auto& opKeyValue : opCodes)
        {
            const auto opT = opKeyValue.first;
            const auto refOpCode = opKeyValue.second;
            if (opKeyValue.first != 0xcb)
            {
                auto decoded_op = gba_cpu.decode_op(gba_cpu.get_pc(), opT);

                //std::cout << decoded_op.tostring() << std::endl;
                //std::cout << opCodes[opT]->tostring() << std::endl;
                bool nameMatch = (refOpCode->GetMnemonic() == decoded_op.name);
                bool flagMatches = true;
                for (size_t f = 0; f < 4; ++f)
                {
                    flagMatches = flagMatches && (refOpCode->GetFlag(f) == decoded_op.GetFlag(f));
                    if (!flagMatches)
                        break;
                }

                bool lengthMatch = static_cast<uint8_t>(refOpCode->GetLength()) == decoded_op.length;
                bool cyclesMatch = (decoded_op.min_timing == refOpCode->MinCycles()) && (decoded_op.max_timing == refOpCode->MaxCycles());

                bool operand1Match = decoded_op.GetOprand1Name() == refOpCode->GetOperand1();
                bool operand2Match = decoded_op.GetOprand2Name() == refOpCode->GetOperand2();

                if (!nameMatch || !flagMatches || !lengthMatch || !cyclesMatch || !operand1Match || !operand2Match)
                {
                    std::cout << decoded_op.tostring() << std::endl;
                    std::cout << refOpCode->tostring() << std::endl;
                    std::cout << "NO MATCHED!!!" << std::endl;
                    std::cout << "-----------------------------" << std::endl;
                }
            }
            /*else
            {
                for (size_t sub_op = 0; sub_op <= 0xff; ++sub_op)
                {
                    memory->write(gba_cpu.get_pc() + 1, (uint8_t)sub_op);
                    auto decoded_op = gba_cpu.decode_op(gba_cpu.get_pc(), (uint8_t)op);
                    std::cout << "CD EXT --- " << decoded_op.tostring() << std::endl;
                }
            }*/
        }

        /*gba_cpu.set_pc(decoded_op.location + decoded_op.length);
        if (decoded_op.location > gba_cpu.get_pc())
            break;*/
    }
}