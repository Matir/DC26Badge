/** Driver for the HT16K33 chip. */

#include <string.h>

#include "nrf_log.h"

#include "ht16k33.h"

#define CMD_WRITE_RAM 0x00
#define CMD_DIMMING 0xe0
#define CMD_OSCILLATOR 0x20
#define CMD_DISPLAY 0x80

static inline ret_code_t i2c_send(led_display *disp, const uint8_t const *data, uint8_t len);


/**
 * Initialize the display struct.
 */
void init_led_display(led_display *disp, nrfx_twim_t *twi_instance,
    uint8_t addr) {
  disp->addr = addr & 0x7F;
  disp->twi_instance = twi_instance;
  uint8_t enable = CMD_OSCILLATOR | 1;
  i2c_send(disp, &enable, 1);
}

/**
 * Send to display via I2C
 */
static inline ret_code_t i2c_send(led_display *disp, const uint8_t const *data, uint8_t len) {
  nrfx_err_t rc;
  do {
    rc = nrfx_twim_tx(disp->twi_instance, disp->addr, data, len, 0);
    NRF_LOG_INFO("I2C Send");
  } while(rc == NRFX_ERROR_BUSY);
  return rc;
}

/**
 * Set text on screen, takes exactly 8 characters
 */
ret_code_t display_text(led_display *disp, uint8_t *text) {
  int i;
  uint16_t val;

  disp->buf[0] = CMD_WRITE_RAM;  // Reset memory address for map
  for (i=1;i<=8;i++) {
    val = fontmap[text[i-1] & 0x7F];
    // Yes, low bits go first.
    disp->buf[i*2-1] = (uint8_t)(val & 0xFF);
    disp->buf[i*2] = (uint8_t)(val >> 8);
  }

  return i2c_send(disp, disp->buf, 17);
}

/**
 * Set brightness
 */
ret_code_t display_set_brightness(led_display *disp, uint8_t level) {
  level = CMD_DIMMING | (level & 0xF);
  return i2c_send(disp, &level, 1);
}

/**
 * Set on/off and blink
 */
ret_code_t display_mode(led_display *disp, uint8_t on, uint8_t blink) {
  on = on & 1;
  blink = (blink & 3) << 1;
  uint8_t message = CMD_DISPLAY | blink | on;
  return i2c_send(disp, &message, 1);
}
