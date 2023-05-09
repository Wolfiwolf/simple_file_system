#ifndef SD_CARD_H
#define SD_CARD_H

#include <stdint.h>


void SDCard_init();

void SDCard_write_page(uint32_t page_address, uint8_t *buffer);

void SDCard_read_page(uint32_t page_address, uint8_t *buffer);

void SDCard_set_to_zero();

#endif

