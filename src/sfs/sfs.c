#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include "sd_card/sd_card.h"

// This function must read the 512 byte page on the specified page address.
// If your storage device does not have pages you can simulate them by 
// reading 512 bytes from the address page_address * 512.
static void read_page(uint32_t page_address, uint8_t *buffer) {
	SDCard_read_page(page_address, buffer);
}

// This function must write the 512 byte page on the specified page address.
// If your storage device does not have pages you can simulate them by 
// writing 512 bytes to the address page_address * 512.
static void write_page(uint32_t page_address, uint8_t *buffer) {
	SDCard_write_page(page_address, buffer);
}

struct BlockMetaData {
	uint32_t page;
	uint32_t owner;
	uint32_t size_taken;
	uint32_t crc;
};

struct FileMetaData {
	uint32_t owner;
	uint32_t last_page;
	uint32_t offset;
	uint64_t size;
};

#define SFS_STORAGE_SIZE 16000000
#define SFS_PAGE_SIZE 512
#define SFS_META_DATA_SIZE 16
#define SFS_DATA_BLOCKS_START ((uint32_t)((SFS_STORAGE_SIZE / SFS_PAGE_SIZE) * SFS_META_DATA_SIZE) / SFS_PAGE_SIZE + 1)
#define SFS_META_BLOCKS_START 1

static uint32_t file_name_to_owner(const char *file_name);
static uint32_t get_new_page(uint32_t owner, uint32_t size_taken);
static void set_page_size_taken(uint32_t page, uint32_t size_taken);
static void delete_page(uint32_t page);
static void move_page(uint32_t src_page, uint32_t dest_page);
static uint32_t get_next_taken_page(uint32_t start_page);

static uint8_t _page[512];
static uint32_t _num_of_pages;
static struct FileMetaData _files[16];
static uint32_t _num_of_files;


void SFS_init() {
	read_page(0, _page);
	memcpy(&_num_of_pages, _page, 4);

	_num_of_pages = 0;
	_num_of_files = 0;

	uint32_t meta_count = 0;
	uint32_t page_index = SFS_META_BLOCKS_START;
	for (; ;) {
		read_page(page_index, _page);
		struct BlockMetaData meta_data;

		bool is_over = false;
		for (uint16_t j = 0; j < 512; ) {
			if (meta_count == _num_of_pages) {
				is_over = true;
				break;
			}
			memcpy(&meta_data.page, _page + j, 4);
			j += 4;
			memcpy(&meta_data.owner, _page + j, 4);
			j += 4;
			memcpy(&meta_data.size_taken, _page + j, 4);
			j += 4;
			memcpy(&meta_data.crc, _page + j, 4);
			j += 4;

			if (meta_data.owner == 0) continue;
			meta_count++;


			bool file_exists = false;
			for (uint8_t k = 0; k < _num_of_files; ++k) {
				if (_files[k].owner == meta_data.owner) {
					_files[k].last_page = meta_data.page;
					_files[k].offset = meta_data.size_taken;
					_files[k].size += _files[k].offset;
					file_exists = true;
				}
			}

			if (!file_exists) {
				_files[_num_of_files].owner = meta_data.owner;
				_files[_num_of_files].last_page = meta_data.page;
				_files[_num_of_files].offset = meta_data.size_taken;
				_files[_num_of_files].size = _files[_num_of_files].offset;
				_num_of_files++;
			}
		}
		page_index++;

		if (is_over) break;
	}
}

void SFS_create(const char *file_name) {
	uint32_t owner = file_name_to_owner(file_name);

	_files[_num_of_files].owner = owner;
	_files[_num_of_files].last_page = get_new_page(owner, 0);
	_files[_num_of_files].offset = 0;
	_files[_num_of_files].size = 0;

	_num_of_files++;
}

void SFS_delete(const char *file_name) {
	uint32_t owner = file_name_to_owner(file_name);

	uint8_t file_index = 0;
	for (uint8_t i = 0; i < _num_of_files; ++i) {
		if (_files[i].owner == owner) {
			file_index = i;
			_files[i].owner = 0;
			_files[i].last_page = 0;
			_files[i].offset = 0;
			_files[i].size = 0;
			break;
		}
	}

	for (uint8_t i = file_index; i < _num_of_files - 1; ++i) {
		_files[i].owner = _files[i + 1].owner;
		_files[i].last_page = _files[i + 1].last_page;
		_files[i].offset = _files[i + 1].offset;
		_files[i].size = _files[i + 1].size;
	}

	_num_of_files--;

	uint32_t meta_count = 0;
	uint32_t page_index = SFS_META_BLOCKS_START;
	for (; ;) {
		read_page(page_index, _page);
		struct BlockMetaData meta_data;

		bool is_over = false;
		for (uint16_t j = 0; j < 512; ) {
			if (meta_count == _num_of_pages + 1) {
				is_over = true;
				break;
			}
			memcpy(&meta_data.page, _page + j, 4);
			j += 4;
			memcpy(&meta_data.owner, _page + j, 4);
			j += 4;
			memcpy(&meta_data.size_taken, _page + j, 4);
			j += 4;
			memcpy(&meta_data.crc, _page + j, 4);
			j += 4;

			if (meta_data.owner == 0) continue;

			meta_count++;

			if (meta_data.owner == owner) {
				delete_page(meta_data.page);
			}

		}
		page_index++;

		if (is_over) break;
	}

	read_page(0, _page);
	memcpy(_page, &_num_of_pages, 4);
	write_page(0, _page);
}

void SFS_delete_all() {
	_num_of_files = 0;
	_num_of_pages = 0;

	read_page(0, _page);
	memcpy(_page, &_num_of_pages, 4);
	write_page(0, _page);
}

uint64_t SFS_size(const char *file_name) {
	uint32_t owner = file_name_to_owner(file_name);

	for (uint8_t i = 0; i < _num_of_files; ++i) {
		if (_files[i].owner == owner) return _files[i].size;
	}

	return 0;
}

bool SFS_exists(const char *file_name) {
	uint32_t owner = file_name_to_owner(file_name);

	for (uint8_t i = 0; i < _num_of_files; ++i) {
		if (_files[i].owner == owner) return true;
	}

	return false;
}

void SFS_write(const char *file_name, uint8_t *buffer, uint32_t data_len) {
	uint32_t owner = file_name_to_owner(file_name);

	uint8_t file_index = 0;
	uint32_t page;
	uint32_t offset;
	for (uint8_t i = 0; i < _num_of_files; ++i) {
		if (owner == _files[i].owner) {
			file_index = i;
			page = _files[i].last_page;
			offset = _files[i].offset;
			break;
		}
	}

	if (offset + data_len > SFS_PAGE_SIZE) {
		uint32_t fist_part_size = SFS_PAGE_SIZE - offset;
		uint32_t last_part_size = (offset + data_len) % SFS_PAGE_SIZE;
		uint32_t num_of_middle_parts = (offset + data_len) / SFS_PAGE_SIZE - 1;


		read_page(SFS_DATA_BLOCKS_START +  page, _page);
		memcpy(_page + offset, buffer, fist_part_size);
		write_page(SFS_DATA_BLOCKS_START +  page, _page);
		set_page_size_taken(page, 512);

		for (uint32_t i = 0; i < num_of_middle_parts; ++i) {
			page = get_new_page(owner, 512);
			read_page(SFS_DATA_BLOCKS_START +  page, _page);
			memcpy(_page, buffer + fist_part_size + i * SFS_PAGE_SIZE, SFS_PAGE_SIZE);
			write_page(SFS_DATA_BLOCKS_START +  page, _page);
		}

		page = get_new_page(owner, last_part_size);
		read_page(SFS_DATA_BLOCKS_START +  page, _page);
		memcpy(_page, buffer + fist_part_size + num_of_middle_parts * SFS_PAGE_SIZE, last_part_size);
		write_page(SFS_DATA_BLOCKS_START +  page, _page);

		_files[file_index].last_page = page;
		_files[file_index].offset = last_part_size;

	} else {
		read_page(SFS_DATA_BLOCKS_START + page, _page);
		memcpy(_page + offset, buffer, data_len);
		write_page(SFS_DATA_BLOCKS_START + page, _page);

		set_page_size_taken(page, offset + data_len);

		_files[file_index].offset += data_len;
	}


	_files[file_index].size += data_len;
}

void SFS_write_to_offset(const char *file_name, uint8_t *buffer, uint32_t data_len, uint64_t offset) {
	uint32_t owner = file_name_to_owner(file_name);

	uint32_t overwrite_data_len = SFS_size(file_name) - offset;
	uint32_t add_data_len = data_len - overwrite_data_len;

	uint32_t first_page = offset / SFS_PAGE_SIZE;
	uint32_t first_page_offset = offset % SFS_PAGE_SIZE;
	uint32_t first_page_size = SFS_PAGE_SIZE - first_page_offset;

	uint32_t last_page = (offset + overwrite_data_len) / SFS_PAGE_SIZE;
	uint32_t last_page_size = (offset + overwrite_data_len) % SFS_PAGE_SIZE;

	if (last_page_size == 0) {
		last_page--;
		last_page_size = 512;
	}

	uint32_t num_of_middle_pages = (overwrite_data_len - first_page_size - last_page_size) / 512;


	if (first_page == last_page) {
		first_page_size = overwrite_data_len;
		last_page_size = 0;
		num_of_middle_pages = 0;
	}


	uint32_t file_page_count = 0;
	uint8_t reading_page[512];
	bool start_middle = false;
	uint32_t middle_count = 0;

	uint32_t meta_count = 0;
	uint32_t page_index = SFS_META_BLOCKS_START;
	for (; ;) {
		read_page(page_index, _page);
		struct BlockMetaData meta_data;

		bool is_over = false;
		for (uint16_t j = 0; j < 512; ) {
			if (meta_count == _num_of_pages) {
				is_over = true;
				break;
			}
			memcpy(&meta_data.page, _page + j, 4);
			j += 4;
			memcpy(&meta_data.owner, _page + j, 4);
			j += 4;
			memcpy(&meta_data.size_taken, _page + j, 4);
			j += 4;
			memcpy(&meta_data.crc, _page + j, 4);
			j += 4;
			meta_count++;


			if (meta_data.owner == owner) {
				if (file_page_count == first_page) {
					read_page(SFS_DATA_BLOCKS_START + meta_data.page, reading_page);
					memcpy(reading_page + first_page_offset, buffer, first_page_size);
					write_page(SFS_DATA_BLOCKS_START + meta_data.page, reading_page);

					if (num_of_middle_pages == 0) {
						is_over = true;
					} else {
						start_middle = true;
					}
				} else if (file_page_count == last_page) {
					read_page(SFS_DATA_BLOCKS_START + meta_data.page, reading_page);
					memcpy(reading_page, buffer + first_page_size + middle_count * SFS_PAGE_SIZE, last_page_size);
					write_page(SFS_DATA_BLOCKS_START + meta_data.page, reading_page);
					start_middle = false;
					is_over = true;
				} else if (start_middle) {
					read_page(SFS_DATA_BLOCKS_START + meta_data.page, reading_page);
					memcpy(reading_page, buffer + first_page_size + middle_count * SFS_PAGE_SIZE, SFS_PAGE_SIZE);
					write_page(SFS_DATA_BLOCKS_START + meta_data.page, reading_page);
					middle_count++;
				}

				file_page_count++;
			}

		}
		page_index++;

		if (is_over) break;
	}

	SFS_write(file_name, buffer + overwrite_data_len, add_data_len);
}

void SFS_read(const char *file_name, uint8_t *buffer, uint32_t data_len, uint64_t offset) {
	uint32_t owner = file_name_to_owner(file_name);

	uint32_t first_page = offset / SFS_PAGE_SIZE;
	uint32_t first_page_offset = offset % SFS_PAGE_SIZE;
	uint32_t first_page_size = SFS_PAGE_SIZE - first_page_offset;

	uint32_t last_page = (offset + data_len) / SFS_PAGE_SIZE;
	uint32_t last_page_size = (offset + data_len) % SFS_PAGE_SIZE;

	if (last_page_size == 0) {
		last_page--;
		last_page_size = 512;
	}

	uint32_t num_of_middle_pages = (data_len - first_page_size - last_page_size) / 512;


	if (first_page == last_page) {
		first_page_size = data_len;
		last_page_size = 0;
		num_of_middle_pages = 0;
	}


	uint32_t file_page_count = 0;
	uint8_t reading_page[512];
	bool start_middle = false;
	uint32_t middle_count = 0;

	uint32_t meta_count = 0;
	uint32_t page_index = SFS_META_BLOCKS_START;
	for (; ;) {
		read_page(page_index, _page);
		struct BlockMetaData meta_data;

		bool is_over = false;
		for (uint16_t j = 0; j < 512; ) {
			if (meta_count == _num_of_pages) {
				is_over = true;
				break;
			}
			memcpy(&meta_data.page, _page + j, 4);
			j += 4;
			memcpy(&meta_data.owner, _page + j, 4);
			j += 4;
			memcpy(&meta_data.size_taken, _page + j, 4);
			j += 4;
			memcpy(&meta_data.crc, _page + j, 4);
			j += 4;
			if (meta_data.owner == 0) continue;
			meta_count++;


			if (meta_data.owner == owner) {
				if (file_page_count == first_page) {
					read_page(SFS_DATA_BLOCKS_START + meta_data.page, reading_page);
					memcpy(buffer, reading_page + first_page_offset, first_page_size);
					if (num_of_middle_pages == 0) {
						is_over = true;
					} else {
						start_middle = true;
					}
				} else if (file_page_count == last_page) {
					read_page(SFS_DATA_BLOCKS_START + meta_data.page, reading_page);
					memcpy(buffer + first_page_size + middle_count * SFS_PAGE_SIZE, reading_page, last_page_size);
					start_middle = false;
					is_over = true;
				} else if (start_middle) {
					read_page(SFS_DATA_BLOCKS_START + meta_data.page, reading_page);
					memcpy(buffer + first_page_size + middle_count * SFS_PAGE_SIZE, reading_page, SFS_PAGE_SIZE);
					middle_count++;
				}

				file_page_count++;
			}

		}
		page_index++;

		if (is_over) break;
	}
}

void SFS_defragment() {
	uint32_t meta_count = 0;
	uint32_t page_index = SFS_META_BLOCKS_START;
	for (; ;) {
		read_page(page_index, _page);
		struct BlockMetaData meta_data;

		bool is_over = false;
		for (uint16_t j = 0; j < 512; ) {
			if (meta_count == _num_of_pages) {
				is_over = true;
				break;
			}
			memcpy(&meta_data.page, _page + j, 4);
			j += 4;
			memcpy(&meta_data.owner, _page + j, 4);
			j += 4;
			memcpy(&meta_data.size_taken, _page + j, 4);
			j += 4;
			memcpy(&meta_data.crc, _page + j, 4);
			j += 4;
			if (meta_data.owner == 0) continue;
			meta_count++;


			// FILE CODE

			if (meta_data.owner == 0) {
				uint32_t next_page = get_next_taken_page(meta_data.page);

				if (next_page == 0) {
					_num_of_pages = meta_count - 1;
					return;
				}

				move_page(next_page, meta_data.page);
				read_page(page_index, _page);
			}

		}
		page_index++;

		if (is_over) break;
	}
}

static uint32_t file_name_to_owner(const char *file_name) {
	uint32_t owner = 0;
	uint8_t i = 0;
	uint8_t ch = file_name[i];
	do {
		owner += ch * powf(2, i);

		ch = file_name[i];
		++i;
	} while(ch != 0);

	return owner;
}

static uint32_t get_new_page(uint32_t owner, uint32_t size_taken) {
	struct BlockMetaData meta_data;
	meta_data.owner = owner;
	meta_data.page = _num_of_pages;
	meta_data.size_taken = size_taken;

	uint32_t meta_page = (_num_of_pages * SFS_META_DATA_SIZE) / SFS_PAGE_SIZE;
	uint32_t meta_offset = (_num_of_pages * SFS_META_DATA_SIZE) % SFS_PAGE_SIZE;

	read_page(SFS_META_BLOCKS_START + meta_page, _page);

	memcpy(_page + meta_offset, &meta_data.page, 4);
	meta_offset += 4;
	memcpy(_page + meta_offset, &meta_data.owner, 4);
	meta_offset += 4;
	memcpy(_page + meta_offset, &meta_data.size_taken, 4);

	write_page(SFS_META_BLOCKS_START + meta_page, _page);


	_num_of_pages++;

	read_page(0, _page);
	memcpy(_page, &_num_of_pages, 4);
	write_page(0, _page);

	return meta_data.page;
}

static void set_page_size_taken(uint32_t page, uint32_t size_taken) {
	uint32_t meta_page = (page * SFS_META_DATA_SIZE) / SFS_PAGE_SIZE;
	uint32_t meta_offset = (page * SFS_META_DATA_SIZE) % SFS_PAGE_SIZE;
	read_page(SFS_META_BLOCKS_START + meta_page, _page);


	uint32_t temp_size_taken = 0;
	meta_offset += 4;
	meta_offset += 4;
	memcpy(_page + meta_offset, &size_taken, 4);


	write_page(SFS_META_BLOCKS_START + meta_page, _page);
}

static void delete_page(uint32_t page) {
	uint32_t meta_page = (page * SFS_META_DATA_SIZE) / SFS_PAGE_SIZE;
	uint32_t meta_offset = (page * SFS_META_DATA_SIZE) % SFS_PAGE_SIZE;
	read_page(SFS_META_BLOCKS_START + meta_page, _page);

	uint32_t owner = 0;
	meta_offset += 4;
	memcpy(_page + meta_offset, &owner, 4);

	write_page(SFS_META_BLOCKS_START + meta_page, _page);

	_num_of_pages--;
}

static void move_page(uint32_t src_page, uint32_t dest_page) {
	uint32_t meta_page = (src_page * SFS_META_DATA_SIZE) / SFS_PAGE_SIZE;
	uint32_t meta_offset = (src_page * SFS_META_DATA_SIZE) % SFS_PAGE_SIZE;

	read_page(SFS_META_BLOCKS_START + meta_page, _page);

	struct BlockMetaData meta_data;

	memcpy(&meta_data.page, _page + meta_offset, 4);
	meta_offset += 4;
	memcpy(&meta_data.owner, _page + meta_offset, 4);
	meta_offset += 4;
	memcpy(&meta_data.size_taken, _page + meta_offset, 4);
	meta_offset += 4;
	memcpy(&meta_data.crc, _page + meta_offset, 4);

	delete_page(src_page);

	meta_page = (dest_page * SFS_META_DATA_SIZE) / SFS_PAGE_SIZE;
	meta_offset = (dest_page * SFS_META_DATA_SIZE) % SFS_PAGE_SIZE;

	read_page(SFS_META_BLOCKS_START + meta_page, _page);

	memcpy(_page + meta_offset, &dest_page, 4);
	meta_offset += 4;
	memcpy(_page + meta_offset, &meta_data.owner, 4);
	meta_offset += 4;
	memcpy(_page + meta_offset, &meta_data.size_taken, 4);
	meta_offset += 4;
	memcpy(_page + meta_offset, &meta_data.crc, 4);

	write_page(SFS_META_BLOCKS_START + meta_page, _page);

	read_page(SFS_DATA_BLOCKS_START + src_page, _page);
	write_page(SFS_DATA_BLOCKS_START + dest_page, _page);
}

static uint32_t get_next_taken_page(uint32_t start_page) {
	uint32_t meta_page = (start_page * SFS_META_DATA_SIZE) / SFS_PAGE_SIZE;
	uint32_t meta_offset = (start_page * SFS_META_DATA_SIZE) % SFS_PAGE_SIZE;

	bool is_start = true;
	uint32_t meta_count = start_page;
	uint32_t page_index = SFS_META_BLOCKS_START + meta_page;
	for (; ;) {
		read_page(page_index, _page);
		struct BlockMetaData meta_data;

		bool is_over = false;
		for (uint16_t j = 0; j < 512; ) {
			if (is_start) {
				is_start = false;
				j = meta_offset;
			}
			if (meta_count == _num_of_pages) {
				is_over = true;
				break;
			}
			memcpy(&meta_data.page, _page + j, 4);
			j += 4;
			memcpy(&meta_data.owner, _page + j, 4);
			j += 4;
			memcpy(&meta_data.size_taken, _page + j, 4);
			j += 4;
			memcpy(&meta_data.crc, _page + j, 4);
			j += 4;
			if (meta_data.owner == 0) continue;
			meta_count++;


			if (meta_data.owner != 0) {
				return meta_data.page;
			}

		}
		page_index++;

		if (is_over) break;
	}

	return 0;
}
