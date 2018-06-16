#include "storage.h"

#include <stdint.h>
#include <string.h>

#include "nrf_log.h"

#define FILE_MAX 0xFFFF

static void storage_evt_handler(const fds_evt_t * const p_evt);
static ret_code_t storage_get(void *dest, int *len, const uint16_t file, const uint16_t record);
static ret_code_t storage_save(void *src, const int len, const uint16_t file, const uint16_t record_key);
static ret_code_t maybe_gc(bool force);
static bool storage_erase_next(bool init);

static volatile int storage_init_done = 0;
static volatile bool in_erase = false;

#ifdef STORAGE_DEBUG
# define S_DBG NRF_LOG_INFO
#else
# define S_DBG(...)
#endif

void storage_init() {
  fds_register(storage_evt_handler);
  NRF_LOG_INFO("Initializing FDS...");
  APP_ERROR_CHECK(fds_init());
  while(!storage_init_done);
  NRF_LOG_INFO("FDS initialized.");
}

void storage_erase_all() {
  in_erase = true;
  storage_erase_next(true);
  while(in_erase);
}

bool storage_check_firstboot() {
  int tmpbuf;
  int len = sizeof(tmpbuf);
  ret_code_t rv = storage_get(
      &tmpbuf, &len, FILE_ID_METADATA, RECORD_ID_FIRSTBOOT);
  if (rv != NRF_SUCCESS || len != sizeof(tmpbuf))
    return true;
  return tmpbuf != FIRSTBOOT_MAGIC;
}

void storage_finish_firstboot() {
  int flag = FIRSTBOOT_MAGIC;
  storage_save(
      &flag, sizeof(flag), FILE_ID_METADATA, RECORD_ID_FIRSTBOOT);
}

// Returns true so long as more are to be done
static bool storage_erase_next(bool init) {
  static uint16_t file;
  ret_code_t rv = NRF_SUCCESS;

  if (init)
    file = 0;

  for (;file < FILE_MAX;file++) {
    fds_record_desc_t desc;
    fds_find_token_t token={0};
    rv = fds_record_find_in_file(file, &desc, &token);
    if (rv == FDS_ERR_NOT_FOUND)
      continue;

    rv = fds_file_delete(file);
    if (rv == FDS_ERR_NO_SPACE_IN_QUEUES)
      return true;
  }

  return false;
}

static void storage_evt_handler(const fds_evt_t * const p_fds_evt) {
  switch (p_fds_evt->id) {
    case FDS_EVT_INIT:
      S_DBG("Storage init done.");
      storage_init_done = 1;
      break;
    case FDS_EVT_WRITE:
    case FDS_EVT_UPDATE:
      if (p_fds_evt->result != FDS_SUCCESS) {
        NRF_LOG_ERROR("Write/updated failed!");
      } else {
        S_DBG("Write/update succeeded.");
      }
      if (p_fds_evt->result == FDS_ERR_NO_SPACE_IN_FLASH) {
        fds_gc();
      } else {
        maybe_gc(false);
      }
      break;
    case FDS_EVT_DEL_FILE:
      if (in_erase && !storage_erase_next(false)) {
        S_DBG("Done erasing, starting gc.");
        fds_gc();
      }
      break;
    case FDS_EVT_GC:
      if (in_erase) {
        S_DBG("Erase finished.");
        in_erase = false;
      }
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
  fds_record_desc_t record_desc = {0};
  fds_find_token_t find_token = {0};

  rv = fds_record_find(
      file,
      record,
      &record_desc,
      &find_token);
  if (rv != FDS_SUCCESS) {
    S_DBG("Unable to find record: %d", rv);
    return rv;
  }

  fds_flash_record_t flash_record;
  rv = fds_record_open(&record_desc, &flash_record);
  if (rv != FDS_SUCCESS) {
    S_DBG("Unable to open record: %d", rv);
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
  S_DBG("Saving: %02x %02x %02x %02x",
      ((char *)(src))[0],
      ((char *)(src))[1],
      ((char *)(src))[2],
      ((char *)(src))[3]);

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
  fds_find_token_t token = {0};
  ret_code_t rv;
  if (fds_record_find(file, record_key, &record_desc, &token) == FDS_ERR_NOT_FOUND) {
    S_DBG("Performing write.");
    rv = fds_record_write(&record_desc, &record);
  } else {
    S_DBG("Performing update.");
    rv = fds_record_update(&record_desc, &record);
  }
  if (rv != FDS_SUCCESS) {
    S_DBG("Write failed: %d", rv);
  }
  return rv;
}

static ret_code_t maybe_gc(bool force) {
  fds_stat_t stats;
  ret_code_t rv = fds_stat(&stats);
  if (rv != FDS_SUCCESS)
    return rv;
  bool must_gc = force || stats.corruption;
  must_gc = must_gc || (stats.dirty_records > 10);
  if (must_gc) {
    return fds_gc();
  }
  return FDS_SUCCESS;
}
