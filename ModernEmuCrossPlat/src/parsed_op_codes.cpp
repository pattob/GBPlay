#include "pch.h"

#include "parsed_op_codes.h"

const uint8_t ParseAddr(const string& addrStr)
{
    uint8_t rVal = 0;
    bool skip = true;
    for(const auto& val : addrStr)
    {
        if (skip && (val == 'x' || val == 'X'))
        {
            skip = false;
        }
        else if (!skip)
        {
            rVal = rVal << 4;
            if (val <= '9' && val >= '0')
            {
                rVal += static_cast<uint8_t>(val - '0');
            }
            else if (val <= 'F' && val >= 'A')
            {
                rVal += static_cast<uint8_t>(val - 'A') + 0xAui8;
            }
            else if (val <= 'f' && val >= 'a')
            {
                rVal += static_cast<uint8_t>(val - 'a') + 0xAui8;
            }
        }
    }

    return rVal;
}

std::map<uint8_t, std::shared_ptr<ParsedOpCode>> ParsedOpCodes()
{
    //static const char operandOne[] = "";
    static const char operandTwo[] = "operand2";

    auto opCodes = std::map<uint8_t, std::shared_ptr<ParsedOpCode>>();
    Json::Value root;
    std::vector<size_t> cycleData;
    std::vector<char> flags;
    ifstream jsonFile("C:\\Users\\Patrick\\source\\repos\\ModernEmu\\ModernEmuCrossPlat\\data\\opcodes.json");
    if (jsonFile.is_open())
    {
        jsonFile >> root;

        const auto unprefixed = root["unprefixed"];
        for (const auto& key : unprefixed.getMemberNames())
        {
            const auto& content = unprefixed[key];

            if (content.empty())
                continue;

            const auto& addrJson = content["addr"];
            const auto& mnemonicJson = content["mnemonic"];
            const auto& lengthJson = content["length"];
            const auto& cycleArray = content["cycles"];
            const auto& jsonFlags = content["flags"];
            const auto& operandOne = content["operand1"];
            const auto& operandTwo = content["operand2"];

            if (addrJson.empty() || !addrJson.isString())
                continue;

            if (mnemonicJson.empty() || !mnemonicJson.isString())
                continue;

            if (lengthJson.empty() || !lengthJson.isNumeric())
                continue;

            auto operand = make_shared<ParsedOpCode>(
                ParseAddr(addrJson.as<const char*>()),
                mnemonicJson.as<const char*>(),
                lengthJson.as<std::size_t>());

            if (cycleArray.isArray())
            {
                cycleData.clear();
                for (const auto& val : cycleArray)
                {
                    cycleData.push_back(val.as<std::size_t>());
                }

                if (cycleData.size() == 1)
                {
                    cycleData.push_back(cycleData.front());
                }

                if (!cycleData.empty())
                {
                    operand->SetCycles(
                        begin(cycleData),
                        end(cycleData));
                }
            }

            if (jsonFlags.isArray())
            {
                for (const auto& flag : jsonFlags)
                {
                    flags.push_back(flag.asCString()[0]);
                }

                operand->SetFlags(
                    begin(flags),
                    end(flags));
            }

            if (!operandOne.empty() && operandOne.isString())
            {
                operand->SetOperand1(operandOne.as<const char*>());
            }

            if (!operandTwo.empty() && operandTwo.isString())
            {
                operand->SetOperand2(operandTwo.as<const char*>());
            }

            opCodes[operand->GetAddr()] = operand;
        }

        jsonFile.close();
    }

    return opCodes;
}
