# Simple File System
This project was made to simply interface with a SD card with a MCU because I had problems with FATFS and FreeRTOS combined. It is generalized so it can be used with any storage medium as long as it can be read by pages of 512 bytes.

## Disclaimer
The code is not fully optimized and despite the testing there can be bugs. If this library shows to be valuable to people it will be thoroughly tested and optimized.

## Instructions

All you have to do is implement the next functions:
```c
// This function must read the 512 byte page on the specified page address.
// If your storage device does not have pages you can simulate them by 
// reading 512 bytes from the address page_address * 512.
static void read_page(uint32_t page_address, uint8_t *buffer);

// This function must write the 512 byte page on the specified page address.
// If your storage device does not have pages you can simulate them by 
// writing 512 bytes to the address page_address * 512.
static void write_page(uint32_t page_address, uint8_t *buffer);
```

## Running the example
To run the example you just run the following commands:
```bash
make
./sfs_example
```

you can play around with the library by trying out different use cases in the main.c file. The storage device in this case is a simulated SD card.

## API

This function initializes the filesystem and reads the file and pages metadata.
```c
void SFS_init();
```

With this function you can create a file in the filesystem.
```c
void SFS_create(const char *file_name);
```

This function deletes the file from the system.
```c
void SFS_delete(const char *file_name);
```

This function returns the size of a file.
```c
uint64_t SFS_size(const char *file_name);
```

This function is used to write to a file.
```c
void SFS_write(const char *file_name, uint8_t *buffer, uint32_t data_len);
```

This function is used to read from a file at specified offset.
```c
void SFS_read(const char *file_name, uint8_t *buffer, uint32_t data_len, uint64_t offset);
```

This function defragments the storage device. It is wise to run it if running out of space or when there is time after a file delete.
```c
void SFS_defragment();
```
