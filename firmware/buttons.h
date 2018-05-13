#ifndef _BUTTONS_H_
#define _BUTTONS_H_

#include <stdint.h>

#include "app_button.h"
#include "nrf_gpio.h"


#define JOYSTICK_CENTER 0
#define JOYSTICK_LEFT   0
#define JOYSTICK_RIGHT  0
#define JOYSTICK_UP     19
#define JOYSTICK_DOWN   0

#define BUTTON_BLE_PAIR JOYSTICK_UP

typedef struct {
  void (*left)(uint8_t);
  void (*right)(uint8_t);
  void (*up)(uint8_t);
  void (*down)(uint8_t);
  void (*center)(uint8_t);
} joystick_actions_t;

void buttons_init(joystick_actions_t *);
void joystick_set_enable(uint8_t enable);
#define joystick_enable() joystick_set_enable(true)
#define joystick_disable() joystick_set_enable(false)

#define DEF_BUTTON(n, handler) { \
  .pin_no = (n), \
  .active_state = APP_BUTTON_ACTIVE_LOW, \
  .pull_cfg = NRF_GPIO_PIN_PULLUP, \
  .button_handler = (handler), \
}

#endif /* _BUTTONS_H_ */
