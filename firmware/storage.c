#include "storage.h"

#include <stdint.h>
#include <string.h>

#include "nrf_log.h"

static void storage_evt_handler(const fds_evt_t * const p_evt);
static ret_code_t storage_get(void *dest, int *len, const uint16_t file, const uint16_t record);
static ret_code_t storage_save(void *src, const int len, const uint16_t file, const uint16_t record_key);

static volatile int storage_init_done = 0;

void storage_init() {
  fds_register(storage_evt_handler);
  NRF_LOG_INFO("Initializing FDS...");
  APP_ERROR_CHECK(fds_init());
  while(!storage_init_done);
  NRF_LOG_INFO("FDS initialized.");
}

static void storage_evt_handler(const fds_evt_t * const p_fds_evt) {
  switch (p_fds_evt->id) {
    case FDS_EVT_INIT:
      storage_init_done = 1;
      break;
    default:
      // Not doing anything here.
      break;
  }
}

ret_code_t get_message(void *dest, int *len, uint16_t id) {
  return storage_get(dest, len, FILE_ID_MESSAGES, RECORD_ID_MESSAGE_BASE + id);
}

ret_code_t get_device_name(void *dest, int *len) {
  return storage_get(dest, len, FILE_ID_METADATA, RECORD_ID_DEVICE_NAME);
}

static ret_code_t storage_get(void *dest, int *len, const uint16_t file, const uint16_t record) {
  if (dest == NULL || len == NULL) {
    return NRF_ERROR_INVALID_PARAM;
  }

  ret_code_t rv;
  fds_record_desc_t record_desc;
  fds_find_token_t find_token;

  rv = fds_record_find(
      file,
      record,
      &record_desc,
      &find_token);
  if (rv != FDS_SUCCESS) {
    return rv;
  }

  fds_flash_record_t flash_record;
  rv = fds_record_open(&record_desc, &flash_record);
  if (rv != FDS_SUCCESS) {
    return rv;
  }

  int length_bytes = flash_record.p_header->length_words * 4;
  if (length_bytes < *len) {
    *len = length_bytes;
  }

  // Copy out the message data
  memcpy((char *)dest, flash_record.p_data, *len);

  rv = fds_record_close(&record_desc);
  if (rv != FDS_SUCCESS) {
    return rv;
  }

  return NRF_SUCCESS;
}

ret_code_t save_message(void *src, const int len, uint16_t id) {
  return storage_save(src, len, FILE_ID_MESSAGES, RECORD_ID_MESSAGE_BASE + id);
}

ret_code_t save_device_name(void *src, const int len) {
  return storage_save(src, len, FILE_ID_METADATA, RECORD_ID_DEVICE_NAME);
}

static ret_code_t storage_save(void *src, const int len, const uint16_t file, const uint16_t record_key) {
  if (src == NULL) {
    return NRF_ERROR_INVALID_PARAM;
  }

  // Set up the record
  fds_record_t record = {
    .file_id = file,
    .key = record_key,
    .data = {
      .p_data = src,
      .length_words = len/4,
    },
  };

  // Round up if necessary
  if (len % 4) {
    record.data.length_words++;
  }

  fds_record_desc_t record_desc;

  return fds_record_write(&record_desc, &record);
}
