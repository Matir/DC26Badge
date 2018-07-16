/** Driver for the HT16K33 chip. */

#include <string.h>

#include "app_scheduler.h"
#include "nrf_log.h"
#include "ble_gap.h"
#include "crc16.h"
#include "nrf_crypto.h"

#include "led_display.h"
#include "storage.h"

#define CMD_WRITE_RAM 0x00
#define CMD_DIMMING 0xe0
#define CMD_OSCILLATOR 0x20
#define CMD_DISPLAY 0x80

#define DISP_UPDATE_FREQUENCY_MS 50
#define WARGAMES_MATCH_BITS 5
#define WARGAMES_MATCH_MASK ((1 << WARGAMES_MATCH_BITS) - 1)
#define WARGAMES_MATCH(x) (((x) & WARGAMES_MATCH_MASK) == WARGAMES_MATCH_MASK)
#define WARGAMES_HOLD_TIME 0x400

#define RANDOM_BUF_SZ 128

static inline ret_code_t display_i2c_send(
    led_display *disp, const uint8_t const *data, uint8_t len);
static void display_timer_handler(void *context);
static void display_update_callback(void *event_data, uint16_t event_size);
static void display_update(led_display *disp);
static void refresh_crcs();
static uint16_t crc_message(unsigned int i);
static bool message_crc_dirty(unsigned int i);
static uint8_t getrandom();
static void cook_random();

static const char scroll_loop_separator[] = SCROLL_LOOP_SEPARATOR;

/**
 * Storage for available messages.
 */
led_message message_set[NUM_MESSAGES] = {
  {
    .message = "HACK THE PLANET",
    .update = MSG_SCROLL_LOOP,
    .speed = 8,
  },
  {
    .message = "HACK ALL THE THINGS",
    .update = MSG_SCROLL_LOOP,
    .speed = 8,
  },
  {
    .message = "WARGAMES",
    .update = MSG_WARGAMES,
    .speed = 8,
  },
  {
    .message = "ATTACKER COMMUNITY",
    .update = MSG_SCROLL_LOOP,
    .speed = 8,
  }
};

// Check for dirty messages.
static uint16_t message_crcs[NUM_MESSAGES];
// Random data when needed
static uint8_t random_data[RANDOM_BUF_SZ];
static uint8_t random_pos = 0;
// Wargames options
const static uint8_t char_options[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

/**
 * Initialize the display struct.
 */
void init_led_display(led_display *disp, nrfx_twim_t *twi_instance,
    uint8_t addr) {
  disp->addr = addr & 0x7F;
  disp->twi_instance = twi_instance;
  disp->cur_msg_idx = 0;
  disp->cur_message = &message_set[0];
  disp->msg_pos = 0;
  disp->brightness = 0;
  uint8_t enable = CMD_OSCILLATOR | 1;
  display_i2c_send(disp, &enable, 1);

  // Setup an app timer to update the display
  APP_TIMER_DEF(led_display_tmr);
  APP_ERROR_CHECK(app_timer_create(
        &led_display_tmr,
        APP_TIMER_MODE_REPEATED,
        display_timer_handler));
  APP_ERROR_CHECK(app_timer_start(
        led_display_tmr,
        APP_TIMER_TICKS(DISP_UPDATE_FREQUENCY_MS),
        (void *)disp));

  NRF_LOG_INFO("Display setup at 0x%08x", (uint32_t)disp);

  ret_code_t rv = nrf_crypto_rng_vector_generate(random_data, RANDOM_BUF_SZ);
  if (rv) {
    NRF_LOG_ERROR("Error generating random data: %d", rv);
  }
}

/**
 * Handle the timer
 */
static void display_timer_handler(void *context) {
  led_display *disp = (led_display *)(context);
  if (!disp)
    return;
  if (!disp->on)
    return;
  if (!disp->cur_message)
    return;
  ++disp->msg_pos;
  uint16_t speed = disp->cur_message->speed;
  if (speed && (disp->msg_pos % speed == 0)) {
#ifdef DISPLAY_DEBUG
    NRF_LOG_INFO("In display_timer_handler, disp: 0x%08x", (uint32_t)disp);
#endif
    APP_ERROR_CHECK(app_sched_event_put(
        (void *)&disp,
        sizeof(led_display *),
        display_update_callback));
  }
}

static void display_update_callback(void *event_data, uint16_t event_size) {
  led_display *disp = *(led_display **)(event_data);
#ifdef DISPLAY_DEBUG
  NRF_LOG_INFO("In display_update_callback, disp: 0x%08x", (uint32_t)disp);
#endif
  if (!disp)
    return;
  display_update(disp);
}

static void display_update(led_display *disp) {
#ifdef DISPLAY_DEBUG
  NRF_LOG_INFO("In display_update, disp: 0x%08x", (uint32_t)disp);
#endif
  if (!disp->cur_message)
    return;

  led_message *msg = disp->cur_message;
  char buf[LED_DISPLAY_WIDTH] = {0};  // Current characters
  int len, chunks, pos;
  char *start;

  switch (msg->update) {
    case MSG_STATIC:
      display_text(disp, (uint8_t *)msg->message);
      break;

    case MSG_SCROLL:
      len = strlen(msg->message);
      // len+1 allows screen to go blank in between iterations
      pos = (disp->msg_pos / msg->speed) % (len+1);
      start = &(msg->message[pos]);
      strncpy(buf, start, LED_DISPLAY_WIDTH);
      display_text(disp, (uint8_t *)buf);
      break;

    case MSG_REPLACE:
      chunks = strlen(msg->message) / LED_DISPLAY_WIDTH;
      pos = (disp->msg_pos / msg->speed) % (chunks+1);
      if (pos < chunks) {
        start = &(msg->message[pos*LED_DISPLAY_WIDTH]);
        strncpy(buf, start, LED_DISPLAY_WIDTH);
      }
      display_text(disp, (uint8_t *)buf);
      break;

    case MSG_WARGAMES:
      if (disp->anim_data.wargames_map == 0xFF) {
        // We have a lock!
        pos = disp->msg_pos / (msg->speed * 4);
        // Reset eventually
        if (disp->msg_pos > WARGAMES_HOLD_TIME) {
          disp->anim_data.wargames_map = 0;
        }
        if ((pos & 1) || (disp->msg_pos > (WARGAMES_HOLD_TIME/2))) {
          display_text(disp, (uint8_t *)msg->message);
        } else {
          display_text(disp, (uint8_t *)buf);
        }
      } else {
        uint8_t rand = getrandom();
        if (WARGAMES_MATCH(rand)) {
          NRF_LOG_INFO("Wargames: Matched character!");
          rand = getrandom();
          disp->anim_data.wargames_map |= (1 << (rand & 0x7));
          if (disp->anim_data.wargames_map == 0xFF)
            disp->msg_pos = 0;
        }
        for(int i=0; i<LED_DISPLAY_WIDTH; i++) {
          if (disp->anim_data.wargames_map & (1 << i)) {
            buf[i] = msg->message[i];
          } else {
            rand = getrandom();
            buf[i] = char_options[rand % 32];
          }
        }
        display_text(disp, (uint8_t *)buf);
      }
      break;

    case MSG_SCROLL_LOOP:
      len = strlen(msg->message);
      pos = (disp->msg_pos / msg->speed) %
        (len + sizeof(scroll_loop_separator) - 1);
      if (pos < len) {
        start = &(msg->message[pos]);
        strncpy(buf, start, LED_DISPLAY_WIDTH);
        len = strlen(start);
        pos = 0;
      } else {
        pos -= len;
        len = 0;
      }
      // Add the separator
      if (len < LED_DISPLAY_WIDTH) {
        start = (char *)&scroll_loop_separator[pos];
        strncpy(buf+len, start, LED_DISPLAY_WIDTH - len);
        len += strlen(start);
      }
      // Loop the beginning
      if (len < LED_DISPLAY_WIDTH)
        strncpy(buf+len, msg->message, LED_DISPLAY_WIDTH - len);
      display_text(disp, (uint8_t *)buf);
      break;

    default:
      NRF_LOG_ERROR("Unknown msg update type.");
      return;
  }
}

/**
 * Send to display via I2C
 */
static inline ret_code_t display_i2c_send(
    led_display *disp, const uint8_t const *data, uint8_t len) {
  nrfx_err_t rc;
  do {
    rc = nrfx_twim_tx(disp->twi_instance, disp->addr, data, len, 0);
  } while(rc == NRFX_ERROR_BUSY);
  return rc;
}

/**
 * Set text on screen, takes exactly LED_DISPLAY_WIDTH characters
 */
ret_code_t display_text(led_display *disp, uint8_t *text) {
  int i;
  uint16_t val;

  disp->buf[0] = CMD_WRITE_RAM;  // Reset memory address for map
  uint8_t *buf = &(disp->buf[1]);
  for (i=0;i<LED_DISPLAY_WIDTH;i++) {
    val = fontmap[text[i] & 0x7F];
    // Yes, low bits go first.
    buf[i*2] = (uint8_t)(val & 0xFF);
    buf[i*2+1] = (uint8_t)(val >> 8);
  }

  return display_i2c_send(disp, disp->buf, 17);
}

/**
 * Set brightness
 */
ret_code_t display_set_brightness(led_display *disp, uint8_t level) {
  disp->brightness = level;
  level = CMD_DIMMING | (level & 0xF);
  return display_i2c_send(disp, &level, 1);
}

/**
 * Set on/off and blink
 */
ret_code_t display_mode(led_display *disp, uint8_t on, uint8_t blink) {
  on = on & 1;
  disp->on = on;
  blink = (blink & 3) << 1;
  uint8_t message = CMD_DISPLAY | blink | on;
  return display_i2c_send(disp, &message, 1);
}

/**
 * Set a new message
 */
void display_set_message(led_display *disp, led_message *msg) {
  if (!msg) {
    msg = &message_set[0];
    disp->cur_msg_idx = 0;
  }
  disp->cur_message = msg;
  disp->msg_pos = 0;
  memset((void *)&disp->anim_data.wargames_map, 0, sizeof(disp->anim_data));
  display_update(disp);
}

/**
 * Display the BLE pairing code.
 */
void display_show_pairing_code(led_display *disp, char *pairing_code) {
  static led_message *old_message = NULL;
  static int8_t old_msg_idx;
  static led_message pairing_message = {
    .update = MSG_STATIC,
  };
  if (!pairing_code) {
    if (old_message) {
      disp->cur_msg_idx = old_msg_idx;
      display_set_message(disp, old_message);
      old_message = NULL;
    }
    return;
  }
  old_message = disp->cur_message;
  old_msg_idx = disp->cur_msg_idx;
  memcpy(&pairing_message.message, pairing_code, BLE_GAP_PASSKEY_LEN);
  pairing_message.message[BLE_GAP_PASSKEY_LEN] = '\0';
  disp->cur_msg_idx = -1;
  display_set_message(disp, &pairing_message);
}
STATIC_ASSERT(MSG_MAX_LEN > BLE_GAP_PASSKEY_LEN,
    "MSG_MAX_LEN smaller than BLE_GAP_PASSKEY_LEN");


/**
 * Next message
 */
void display_next_message(led_display *disp) {
  if (disp->cur_msg_idx == -1)
    return;
  disp->cur_msg_idx++;
  if (disp->cur_msg_idx == NUM_MESSAGES)
    disp->cur_msg_idx = 0;
  display_set_message(disp, &message_set[disp->cur_msg_idx]);
}

/**
 * Previous message
 */
void display_prev_message(led_display *disp) {
  if (disp->cur_msg_idx == -1)
    return;
  if (disp->cur_msg_idx == 0)
    disp->cur_msg_idx = NUM_MESSAGES-1;
  else
    disp->cur_msg_idx--;
  display_set_message(disp, &message_set[disp->cur_msg_idx]);
}

/**
 * Increase brightness
 */
void display_inc_brightness(led_display *disp) {
  if (disp->brightness == MAX_BRIGHTNESS)
    return;
  display_set_brightness(disp, disp->brightness + 1);
}

/**
 * Decrease brightness
 */
void display_dec_brightness(led_display *disp) {
  if (disp->brightness == 0)
    return;
  display_set_brightness(disp, disp->brightness - 1);
}

/**
 * Run a selftest step
 */
void display_selftest_next(led_display *disp) {
  static uint8_t segment = 0;
  static uint8_t dispchar = 0;

  if (++segment >= 15) {
    segment = 0;
    if (++dispchar >= LED_DISPLAY_WIDTH)
      dispchar = 0;
  }

  disp->buf[0] = CMD_WRITE_RAM;  // Reset memory address for map
  for (int i=0; i < LED_DISPLAY_WIDTH; i++) {
    if (i != dispchar) {
      disp->buf[i*2+1] = 0;
      disp->buf[i*2+2] = 0;
      continue;
    }
    disp->buf[i*2+1] = (1 << segment) & 0xFF;
    disp->buf[i*2+2] = (1 << segment) >> 8;
  }
  display_i2c_send(disp, disp->buf, 17);
}

/**
 * Load from storage
 */
ret_code_t display_load_storage() {
  for (uint16_t i=0; i<NUM_MESSAGES; i++) {
    int len = sizeof(led_message);
    ret_code_t rv = get_message(&message_set[i], &len, i);
    if (rv == NRF_SUCCESS)
      continue;
    // it's fine if they're not found
    if (rv == FDS_ERR_NOT_FOUND)
      // We can't return because there might be gaps!
      continue;
    return rv;
  }
  refresh_crcs();
  return NRF_SUCCESS;
}

/**
 * Save to storage
 */
ret_code_t display_save_storage() {
  bool any_dirty = false;
  for (uint16_t i=0; i<NUM_MESSAGES; i++) {
    if (!message_crc_dirty((unsigned int)i))
      continue;
    any_dirty = true;
    NRF_LOG_INFO("Saving message %d", i);
    int len = sizeof(led_message);
    ret_code_t rv = save_message(&message_set[i], len, i);
    if(rv == NRF_SUCCESS)
      continue;
    return rv;
  }
  if (any_dirty)
    refresh_crcs();
  return NRF_SUCCESS;
}

/**
 * Update the CRCs for the messages.
 */
static void refresh_crcs() {
  for (unsigned int i=0; i<NUM_MESSAGES; i++)
    message_crcs[i] = crc_message(i);
}

/**
 * Calculate the CRC for a message.
 */
static uint16_t crc_message(unsigned int i) {
  if (i > NUM_MESSAGES)
    return 0;
  return crc16_compute((uint8_t *)&message_set[i], sizeof(led_message), NULL);
}

/**
 * Check if a message has been changed.
 */
static bool message_crc_dirty(unsigned int i) {
  return message_crcs[i] != crc_message(i);
}

/**
 * Get random byte
 */
static uint8_t getrandom() {
  uint8_t rv = random_data[random_pos];
  random_pos++;
  random_pos %= RANDOM_BUF_SZ;
  if (!random_pos)
    cook_random();
  return rv;
}

/**
 * Update rng
 */
static void cook_random() {
  static uint8_t sauce = 0x55;
  sauce ^= random_data[sauce % RANDOM_BUF_SZ];
  for (int i=0; i<RANDOM_BUF_SZ; i++)
    random_data[i] ^= sauce;
  sauce = (sauce << 1) | (sauce >> 7);
}
