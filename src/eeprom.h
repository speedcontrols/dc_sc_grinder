#ifndef __EEPROM_H__
#define __EEPROM_H__

#include <stdint.h>

float eeprom_float_read(uint32_t addr, float dflt);
void eeprom_float_write(uint32_t addr, float val);
uint32_t eeprom_uint32_read(uint32_t addr, uint32_t dflt);
void eeprom_uint32_write(uint32_t addr, uint32_t val);

#endif
