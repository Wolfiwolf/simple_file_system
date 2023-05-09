#include "sd_card.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static uint8_t _sd_card_buffer[1000000000];

void SDCard_init() {
}

void SDCard_write_page(uint32_t page_address, uint8_t *buffer) {
	memcpy(_sd_card_buffer + page_address * 512, buffer, 512);
}

void SDCard_read_page(uint32_t page_address, uint8_t *buffer) {
	memcpy(buffer, _sd_card_buffer + page_address * 512, 512);
}

void SDCard_set_to_zero() {
	for (uint16_t i = 0; i < 512; ++i) {
		_sd_card_buffer[i] = 0;
	}
}
