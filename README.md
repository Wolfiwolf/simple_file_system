# Simple File System
This project was made to simply interface with a SD card with a MCU because I had problems with FATFS and FreeRTOS combined. It is generalized so it can be used with any storage medium as long as it can be read by pages of 512 bytes.

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
static void read_page(uint32_t page_address, uint8_t *buffer);
```

## Usage Example
There is an example program in the example folder.
