
#ifndef PS2_H
#define PS2_H

#include <stdint.h>

uint8_t convert_scan_code_ibm_to_hid(uint8_t c);

uint16_t ps2decode(uint8_t *state, uint8_t c);
uint16_t ps2decode001(uint8_t *state, uint8_t c);
uint16_t ps2decode002(uint8_t *state, uint8_t c);

#endif // PS2_H
