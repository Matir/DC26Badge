#ifndef _BUTTONS_H_
#define _BUTTONS_H_

#include <stdint.h>

#include "app_button.h"
#include "nrf_gpio.h"

#include "led_display.h"

#if defined(BOARD_PROTO)
# define JOYSTICK_CENTER     1
# define JOYSTICK_LEFT       3
# define JOYSTICK_RIGHT      9
# define JOYSTICK_UP         0
# define JOYSTICK_DOWN       2
#elif defined(BOARD_BADGE)
# define JOYSTICK_CENTER     1
# define JOYSTICK_LEFT       0
# define JOYSTICK_RIGHT      3
# define JOYSTICK_UP         2
# define JOYSTICK_DOWN       9
#else
# define JOYSTICK_CENTER     1
# define JOYSTICK_LEFT       2
# define JOYSTICK_RIGHT      3
# define JOYSTICK_UP         19
# define JOYSTICK_DOWN       4
#endif

#define BUTTON_BLE_PAIR     JOYSTICK_UP
#define BUTTON_BLE_REJECT   JOYSTICK_DOWN

typedef void ble_callback_t(uint8_t);

void buttons_init(led_display *);
void joystick_set_enable(uint8_t enable);
#define joystick_enable() joystick_set_enable(true)
#define joystick_disable() joystick_set_enable(false)
void buttons_set_ble_accept_callback(ble_callback_t *ble_cb);

#define DEF_BUTTON(n, handler) { \
  .pin_no = (n), \
  .active_state = APP_BUTTON_ACTIVE_LOW, \
  .pull_cfg = NRF_GPIO_PIN_PULLUP, \
  .button_handler = (handler), \
}

#endif /* _BUTTONS_H_ */
