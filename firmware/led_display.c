/** Driver for the HT16K33 chip. */

#include <string.h>

#include "app_scheduler.h"
#include "nrf_log.h"

#include "led_display.h"

#include "ble_gap.h"

#define CMD_WRITE_RAM 0x00
#define CMD_DIMMING 0xe0
#define CMD_OSCILLATOR 0x20
#define CMD_DISPLAY 0x80

#define DISP_UPDATE_FREQUENCY_MS 50

static inline ret_code_t display_i2c_send(
    led_display *disp, const uint8_t const *data, uint8_t len);
static void display_timer_handler(void *context);
static void display_update_callback(void *event_data, uint16_t event_size);
static void display_update(led_display *disp);

/**
 * Storage for available messages.
 * TODO: load from storage!
 */
led_message message_set[NUM_MESSAGES] = {
  {
    .message = "HACK THE PLANET   ",
    .update = MSG_SCROLL,
    .speed = 8,
  },
  {
    .message = "HACK ALL THE THINGS   ",
    .update = MSG_SCROLL,
    .speed = 8,
  }
};

/**
 * Initialize the display struct.
 */
void init_led_display(led_display *disp, nrfx_twim_t *twi_instance,
    uint8_t addr) {
  disp->addr = addr & 0x7F;
  disp->twi_instance = twi_instance;
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
  char *buf[LED_DISPLAY_WIDTH];  // Current characters

  switch (msg->update) {
    case MSG_STATIC:
      display_text(disp, (uint8_t *)msg->message);
      break;
    case MSG_SCROLL:
      memset(buf, 0, LED_DISPLAY_WIDTH);
      int len = strlen(msg->message);
      // len+1 allows screen to go blank in between iterations
      int pos = (disp->msg_pos / msg->speed) % (len+1);
      char *start = &(msg->message[pos]);
      int left = len - pos;
      if (left > LED_DISPLAY_WIDTH)
        left = LED_DISPLAY_WIDTH;
      if (left)
        memcpy(buf, start, left);
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
  }
  disp->cur_message = msg;
  disp->msg_pos = 0;
  display_update(disp);
}

/**
 * Display the BLE pairing code.
 */
void display_show_pairing_code(led_display *disp, char *pairing_code) {
  static led_message *old_message = NULL;
  static led_message pairing_message = {
    .update = MSG_STATIC,
  };
  if (!pairing_code) {
    if (old_message) {
      display_set_message(disp, old_message);
      old_message = NULL;
    }
    return;
  }
  old_message = disp->cur_message;
  memcpy(&pairing_message.message, pairing_code, BLE_GAP_PASSKEY_LEN);
  pairing_message.message[BLE_GAP_PASSKEY_LEN] = '\0';
  display_set_message(disp, &pairing_message);
}
STATIC_ASSERT(MSG_MAX_LEN > BLE_GAP_PASSKEY_LEN,
    "MSG_MAX_LEN smaller than BLE_GAP_PASSKEY_LEN");


/**
 * Next message
 */
void display_next_message(led_display *disp) {
  for(int i=0; i<NUM_MESSAGES; i++) {
    if (disp->cur_message == &message_set[i]) {
      if (++i == NUM_MESSAGES) i = 0;
      display_set_message(disp, &message_set[i]);
      return;
    }
  }
}

/**
 * Previous message
 */
void display_prev_message(led_display *disp) {
  for(int i=0; i<NUM_MESSAGES; i++) {
    if (disp->cur_message == &message_set[i]) {
      if (i-- == 0) {
        i = NUM_MESSAGES-1;
      }
      display_set_message(disp, &message_set[i]);
      return;
    }
  }
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
