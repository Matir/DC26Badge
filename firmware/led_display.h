#ifndef _LED_DISPLAY_H_
#define _LED_DISPLAY_H_

#include <stdint.h>

#include "nrfx_twim.h"
#include "app_timer.h"

#define LED_DISPLAY_WIDTH 8

#define I2C_BUF_LEN 20
#if I2C_BUF_LEN < (2*LED_DISPLAY_WIDTH+1)
# error I2C_BUF_LEN Shorter than required minimum!
#endif

#define MSG_MAX_LEN 35  // One extra byte is reserved for null

#define NUM_MESSAGES 4

typedef enum {
  MSG_STATIC,
  MSG_SCROLL,
  MSG_REPLACE,
} __attribute__ ((packed)) message_update_t;

typedef struct _led_message {
  // How this message updates
  message_update_t update;
  // How often this message updates
  uint16_t speed;
  // Contents of message
  char message[MSG_MAX_LEN+1];
} __attribute__ ((packed)) led_message;

typedef struct _led_display {
  // TWI instance to use
  nrfx_twim_t *twi_instance;
  // Buffer for I2C Operations
  uint8_t buf[I2C_BUF_LEN];
  // Device address
  uint8_t addr;
  // Is display on
  uint8_t on;
  // Brightness
  uint8_t brightness;
  // Message position
  uint16_t msg_pos;
  // Currently displayed message
  led_message *cur_message;
  // Timer ID
  app_timer_id_t timer_id;
} led_display;

extern uint16_t fontmap[128];
extern led_message message_set[NUM_MESSAGES];

void init_led_display(led_display *disp, nrfx_twim_t *twi_instance,
    uint8_t addr);
ret_code_t display_text(led_display *disp, uint8_t *text);
ret_code_t display_set_brightness(led_display *disp, uint8_t level);
ret_code_t display_mode(led_display *disp, uint8_t on, uint8_t blink);
#define display_on(disp) display_mode((disp), 1, 0)
#define display_off(disp) display_mode((disp), 0, 0)
void display_set_message(led_display *disp, led_message *msg);
void display_show_pairing_code(led_display *disp, char *pairing_code);

#endif /* _LED_DISPLAY_H_ */
