#ifndef __CHECK_H
#define __CHECK_H
#include "main.h"

uint8_t mc_check_sum(uint8_t *buf, uint8_t len);
uint8_t mc_check_xor(uint8_t *buf, uint8_t len);
uint8_t mc_check_crc8(uint8_t *buf, uint8_t len);
uint8_t mc_check_crc16(uint8_t *buf, uint8_t len);

#endif
