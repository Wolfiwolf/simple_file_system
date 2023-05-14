#include "sd_card/sd_card.h"
#include <stdio.h>
#include "sfs/sfs.h"

int main(int argc, char *argv[]) {
	SDCard_init();

	SFS_init();

	SFS_create("test");

	uint8_t data[8];
	uint8_t read_data[8];

	for (uint8_t i = 0; i < 8; ++i) {
		data[i] = i;
		read_data[i] = 0;
	}

	SFS_write("test", data, 8);
	SFS_read("test", read_data, 8, 0);

	for (uint8_t i = 0; i < 8; ++i) {
		printf("%d, ", read_data[i]);
	}
	printf("\n");

	for (uint8_t i = 0; i < 4; ++i) data[i + 4] = i;
	SFS_write_to_offset("test", data, 8, 4);

	SFS_read("test", read_data, 8, 0);

	for (uint8_t i = 0; i < 8; ++i) {
		printf("%d, ", read_data[i]);
	}

	SFS_delete("test");
	SFS_defragment();

	return 0;
}
