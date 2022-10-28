#ifndef __EEPROM_EMU__
#define __EEPROM_EMU__

// EEPROM emulator if FLASH. Uses append-only log & multiphase commits
// to guarantee atomic writes.

#include <stdint.h>

/* Driver
class FlashDriver {
public:
    enum { BankSize = XXXX };
    static void erase(uint8_t bank) {};

    static uint32_t read(uint8_t bank, uint16_t addr);

    // If 32bit write requires multiple operations, those MUST follow from
    // least significant bits to most significant bits.
    static uint32_t write(uint8_t bank, uint16_t addr, uint32_t data);
}
*/

/*
    Bank structure:

    [ 16 bytes bank marker ] [ 8 bytes record ] [ 8 bytes record ]...

    Bank Marker:

    - [ 0x77EE, VERSION, 0xFFFF, 0xFFFF, 4*0xFFFF ] => active, current
    - [ 0x77EE, VERSION, 0xFFFF, 0xFFFF, 4*0x0000 ] => ready to erase (!active)
    - [ 8*0xFFFF ] => erased OR in progress of transfer

    Data record:

    [ crc16, address_16, data_lo_16, data_hi_16 ]

    Note, many modern flashes use additional ECC bits and not allow old data
    partial override with zero bits. Driver should select proper storage
    sequence, depending on flash data size.

    16-bytes header needed to support 2-phase bank erase:

    - mark bank dirty (via header)
    - run page erase

    Flash page can be organized in [2|4|8] byte blocks. 2 blocks required for
    2-phase commit. 8 bytes * 2 => 16 bytes.
*/

union FLASH_EE_RECORD {
    uint64_t raw64;
    uint32_t raw32[2];
    uint16_t raw16[4];
    uint8_t raw8[8];
    struct {
        uint16_t crc16;
        uint16_t addr;
        uint32_t value;
    } s;
};

template <typename FLASH_EE_DRIVER, uint16_t VERSION = 0xCC33>
class EepromEmu
{
    enum {
        ERASED_VALUE = 0xFFFFFFFFFFFFFFFF,
        RECORD_SIZE = 8,
        BANK_HEADER_SIZE = 16,
        BANK_MARK = 0x77EE,
    };

    bool initialized = false;
    uint8_t current_bank = 0;
    uint32_t next_write_offset;

    bool is_bank_clear(uint8_t bank)
    {
        for (uint32_t i = 0; i < FLASH_EE_DRIVER::BankSize; i += RECORD_SIZE) {
            FLASH_EE_RECORD r = flash.read(bank, i);
            if (r.raw64 != ERASED_VALUE) return false;
        }
        return true;
    }

    bool is_bank_active(uint8_t bank)
    {
        FLASH_EE_RECORD r = flash.read(bank, 0);

        // Check first half of bank header (bytes 0..7)
        if (!((r.raw16[0] == BANK_MARK) && (r.raw16[1] == VERSION))) return false;

        // Check second half of bank header  (bytes 8..15)
        r = flash.read(bank, 8);
        if (r.raw64 != ERASED_VALUE) return false;

        return true;
    }

    bool is_record_valid(FLASH_EE_RECORD &r)
    {
        auto addr = r.s.addr;
        if (addr == 0 && addr == 0xFFFF) return false;

        if (r.s.crc16 != crc16(r.raw8 + 2, 6)) return false;

        return true;
    }

    void fill_record_crc(FLASH_EE_RECORD &r)
    {
        r.s.crc16 = crc16(r.raw8 + 2, 6);
    }

    void create_bank_header(uint8_t bank)
    {
        FLASH_EE_RECORD r;
        r.raw64 = ERASED_VALUE;
        r.raw16[0] = BANK_MARK;
        r.raw16[1] = VERSION;
        flash.write(bank, 0, r);
    }

    uint32_t find_write_offset()
    {
        uint32_t ofs = BANK_HEADER_SIZE;

        for (; ofs <= FLASH_EE_DRIVER::BankSize - RECORD_SIZE; ofs += RECORD_SIZE)
        {
            if (flash.read(current_bank, ofs).raw64 == ERASED_VALUE) break;
        }

        return ofs;
    }

    void move_bank(uint8_t from, uint8_t to, uint16_t ignore_addr=UINT16_MAX)
    {
        if (!is_bank_clear(to)) flash.erase(to);

        uint32_t dst_end_addr = BANK_HEADER_SIZE;

        for (uint32_t ofs = BANK_HEADER_SIZE; ofs < next_write_offset; ofs += RECORD_SIZE)
        {
            FLASH_EE_RECORD r = flash.read(from, ofs);

            // Skip variable with ignored address
            if (r.s.addr == ignore_addr) continue;

            if (!is_record_valid(r)) continue;

            bool more_fresh_exists = false;

            // Check if more fresh record exists
            for (uint32_t i = ofs + RECORD_SIZE; i < next_write_offset; i += RECORD_SIZE)
            {
                FLASH_EE_RECORD r2 = flash.read(from, i);

                // Skip different addresses
                if (r2.s.addr != r.s.addr) continue;

                // Skip invalid records
                if (!is_record_valid(r2)) continue;

                // More fresh (=> already copied) found
                more_fresh_exists = true;
                break;
            }

            if (more_fresh_exists) continue;

            flash.write(to, dst_end_addr, r);
            dst_end_addr += RECORD_SIZE;
        }

        // Mark new bank active
        create_bank_header(to);
        current_bank = to;
        next_write_offset = dst_end_addr;

        // Clean old bank in 2 steps to avoid UB: destroy header & run erase
        FLASH_EE_RECORD r;
        r.raw32[0] = 0;
        r.raw32[1] = 0;
        flash.write(from, 8, r);
        flash.erase(from);
    }

    void init()
    {
        initialized = true;

        if (is_bank_active(0)) current_bank = 0;
        else if (is_bank_active(1)) current_bank = 1;
        else
        {
            // Both banks have no valid markers => prepare first one
            if (!is_bank_clear(0)) flash.erase(0);

            create_bank_header(0);
            current_bank = 0;
        }

        next_write_offset = find_write_offset();
        return;
    }

public:
    FLASH_EE_DRIVER flash;

    uint32_t read_u32(uint32_t addr, uint32_t dflt)
    {
        if (!initialized) init();

        // Reverse scan, stop on first valid
        for (uint32_t ofs = next_write_offset;;)
        {
            if (ofs <= BANK_HEADER_SIZE) break;
            ofs -= RECORD_SIZE;

            FLASH_EE_RECORD r = flash.read(current_bank, ofs);

            if (r.s.addr != addr) continue;
            if (!is_record_valid(r)) continue;

            return r.s.value;
        }

        return dflt;
    }

    void write_u32(uint32_t addr, uint32_t val)
    {
        if (!initialized) init();

        uint8_t bank = current_bank;

        // Don't write the same value
        uint32_t previous = read_u32(addr, val+1);
        if (previous == val) return;

        // Check free space and swap banks if needed
        if (next_write_offset + RECORD_SIZE > FLASH_EE_DRIVER::BankSize)
        {
            move_bank(bank, bank ^ 1, (uint16_t)addr);
            bank ^= 1;
        }

        // Write data
        FLASH_EE_RECORD r;
        r.s.addr = addr;
        r.s.value = val;
        fill_record_crc(r);

        flash.write(bank, next_write_offset, r);
        next_write_offset += RECORD_SIZE;
    }

    float read_float(uint32_t addr, float dflt)
    {
        union { uint32_t i; float f; } x;
        x.f = dflt;
        x.i = read_u32(addr, x.i);
        return x.f;
    }

    void write_float(uint32_t addr, float val)
    {
        union { uint32_t i; float f; } x;
        x.f = val;
        write_u32(addr, x.i);
    }

    // CRC-16/CCITT-FALSE
    // "123456789" => 0x29B1
    // https://crccalc.com/
    // http://www.sunshine2k.de/coding/javascript/crc/crc_js.html
    uint16_t crc16(uint8_t* data, uint16_t len)
    {
        uint16_t crc = 0xFFFF;
        while (len--) crc = crc16_table(*data++ ^ (uint8_t)(crc >> 8)) ^ (crc << 8);
        return crc;
    }

private:
    static const uint16_t crc16_table(uint8_t idx)
    {
        static const uint16_t table[256] = {
            0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
            0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
            0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
            0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
            0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
            0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
            0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
            0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
            0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
            0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
            0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
            0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
            0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
            0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
            0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
            0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
            0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
            0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
            0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
            0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
            0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
            0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
            0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
            0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
            0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
            0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
            0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
            0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
            0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
            0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
            0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
            0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
        };
        return table[idx];
    }
};

#endif
