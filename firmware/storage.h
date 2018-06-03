#ifndef _STORAGE_H_
#define _STORAGE_H_

#include <stdint.h>

#include "fds.h"
#include "sdk_errors.h"

#define FILE_ID_METADATA          0x0001
#define RECORD_ID_DEVICE_NAME     0x0001

#define FILE_ID_MESSAGES          0x0002
#define RECORD_ID_MESSAGE_BASE    0x0001

#ifdef DEBUG
#define STORAGE_DEBUG
#endif

void storage_init();
void storage_erase_all();

// Load a message from flash
ret_code_t get_message(void *dest, int *len, uint16_t id);

// Get device name from flash
ret_code_t get_device_name(void *dest, int *len);

// Save device name to flash
ret_code_t save_device_name(void *src, const int len);

// Save message to flash
ret_code_t save_message(void *src, const int len, uint16_t id);

#endif /* _STORAGE_H_ */
