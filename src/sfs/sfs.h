#ifndef SFS_H
#define SFS_H

#include <stdint.h>
#include <stdbool.h>

void SFS_init();

void SFS_create(const char *file_name);

void SFS_delete(const char *file_name);

void SFS_delete_all();

uint64_t SFS_size(const char *file_name);

bool SFS_exists(const char *file_name);

void SFS_write(const char *file_name, uint8_t *buffer, uint32_t data_len);

void SFS_write_to_offset(const char *file_name, uint8_t *buffer, uint32_t data_len, uint64_t offset);

void SFS_read(const char *file_name, uint8_t *buffer, uint32_t data_len, uint64_t offset);

void SFS_defragment();

#endif
