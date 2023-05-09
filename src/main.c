#include "sd_card/sd_card.h"
#include <stdio.h>
#include "sfs/sfs.h"

int main(int argc, char *argv[]) {
	SDCard_init();

	SFS_init();

	return 0;
}
