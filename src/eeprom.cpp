#include "eeprom.h"

#include "eeprom_emu_template.h"
#include "eeprom_flash_driver.h"

EepromEmu<EepromFlashDriver, 0x0001> eeprom;

float eeprom_float_read(uint32_t addr, float dflt) {
    return eeprom.read_float(addr, dflt);
}

void eeprom_float_write(uint32_t addr, float val) {
    eeprom.write_float(addr, val);
}

uint32_t eeprom_uint32_read(uint32_t addr, uint32_t dflt) {
    return eeprom.read_u32(addr, dflt);
}

void eeprom_uint32_write(uint32_t addr, uint32_t val) {
    eeprom.write_u32(addr, val);
}
