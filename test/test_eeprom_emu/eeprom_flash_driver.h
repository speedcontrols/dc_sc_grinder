#ifndef __EEPROM_FLASH_DRIVER__
#define __EEPROM_FLASH_DRIVER__

#define EEPROM_EMU_BANK_SIZE 1024

#include <stdint.h>

class EepromFlashDriver
{
public:
    EepromFlashDriver()
    {
        for (uint32_t i = 0; i < BankSize*2; i++) memory[i] = 0xFF;
    }

    static const uint32_t BankSize = EEPROM_EMU_BANK_SIZE;

    uint8_t memory[BankSize*2];

    void erase(uint8_t bank)
    {
        for (uint32_t i = 0; i < BankSize; i++) memory[bank*BankSize + i] = 0xFF;
    }

    FLASH_EE_RECORD read(uint8_t bank, uint32_t addr)
    {
        uint32_t ofs = bank*BankSize + addr;

        FLASH_EE_RECORD record;

        for (uint8_t i = 0; i < 8; i++) record.raw8[i] = memory[ofs + i];

        return record;
    }

    void write(uint8_t bank, uint32_t addr, FLASH_EE_RECORD &record)
    {
        uint32_t ofs = bank*BankSize + addr;

        for (uint8_t i = 0; i < 8; i++) memory[ofs + i] = record.raw8[i];
    }
};

#endif
