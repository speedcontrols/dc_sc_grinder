#ifndef __EEPROM_FLASH_DRIVER__
#define __EEPROM_FLASH_DRIVER__

#include "main.h"

// 2K page for stm32f072 => 2K bank => 4K total
#define EEPROM_EMU_BANK_PAGES  1
#define EEPROM_EMU_BANK_SIZE   (FLASH_PAGE_SIZE * EEPROM_EMU_BANK_PAGES)
#define EEPROM_EMU_START_PAGE  (FLASH_PAGE_NB - EEPROM_EMU_BANK_PAGES * 2)
#define EEPROM_EMU_FLASH_START (FLASH_BASE + (EEPROM_EMU_START_PAGE * FLASH_PAGE_SIZE))

/*
#define XSTR(x) STR(x)
#define STR(x) #x
#pragma message "$$$$$ EEPROM EMU MESSAGE: " XSTR(EEPROM_EMU_START_PAGE)
*/

class EepromFlashDriver
{
    uint32_t ee_flash_start_page;
    uint32_t ee_flash_start_address;

public:
    enum { BankSize = EEPROM_EMU_BANK_SIZE };

    EepromFlashDriver()
    {
        // FLASH_PAGE_NB is dynamic variable.
        // Cache constatnt for faster access.
        ee_flash_start_page = EEPROM_EMU_START_PAGE;
        ee_flash_start_address = EEPROM_EMU_FLASH_START;
    }

    void erase(uint8_t bank)
    {
        FLASH_EraseInitTypeDef s_eraseinit;
        s_eraseinit.TypeErase = FLASH_TYPEERASE_PAGES;
        s_eraseinit.Page      = ee_flash_start_page + (EEPROM_EMU_BANK_PAGES * bank);
        s_eraseinit.NbPages   = EEPROM_EMU_BANK_PAGES;
        uint32_t page_error   = 0;

        HAL_FLASH_Unlock();
        HAL_FLASHEx_Erase(&s_eraseinit, &page_error);
        HAL_FLASH_Lock();
    }

    FLASH_EE_RECORD read(uint8_t bank, uint32_t addr)
    {
        uint32_t abs_addr = ee_flash_start_address + bank*EEPROM_EMU_BANK_SIZE + addr;

        FLASH_EE_RECORD r;
        r.raw32[0] = *((uint32_t *)abs_addr);
        r.raw32[1] = *((uint32_t *)(abs_addr + 4));

        return r;
    }

    void write(uint8_t bank, uint32_t addr, FLASH_EE_RECORD r)
    {
        uint32_t flash_addr = ee_flash_start_address + bank*EEPROM_EMU_BANK_SIZE + addr;

        HAL_FLASH_Unlock();
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, flash_addr, r.raw64);
        HAL_FLASH_Lock();
    }
};

#endif
