#include "sd_card/sd_card.h"
#include <stdio.h>
#include "sfs/sfs.h"

int main(int argc, char *argv[]) {
	SDCard_init();

	SFS_init();

	SFS_create("test");
	SFS_create("test2");


	uint8_t data[2000];
	uint8_t read_data[2000];
	for (uint16_t i = 0; i < 2000; ++i) {
		data[i] = i % 33;
		read_data[i] = 0;
	}


	for (uint16_t i = 0; i < 200; ++i) {
		SFS_write("test", data, 1500);
		SFS_write("test2", data, 1500);
	}

	for (uint16_t i = 0; i < 200; ++i) {
		SFS_read("test2", read_data, 1500, 1500);
		for (uint16_t j = 0; j < 200; ++j) {
			if (read_data[j] != data[j]) {
				printf("Mismatch! %d != %d\n", data[i], read_data[i]);
			}
		}
	}
	return 0;
}
