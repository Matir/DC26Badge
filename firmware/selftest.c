#include <string.h>

#include "nrf_delay.h"
#include "nrf_log.h"

#include "buttons.h"
#include "selftest.h"

typedef struct {
  int button_id;
  char *name;
} button_info;

void run_selftest(led_display *disp) {
  NRF_LOG_INFO("Starting selftest.");
  joystick_set_selftest(true);
  if (get_buttons_pushed()) {
    NRF_LOG_INFO("Waiting for button clear.");
    while (get_buttons_pushed());
  }
  while (!get_buttons_pushed()) {
    display_selftest_next(disp);
    nrf_delay_ms(250);
  }
  if (get_buttons_pushed()) {
    NRF_LOG_INFO("Waiting for button clear.");
    while (get_buttons_pushed());
  }
  button_info buttons[] = {
    {
      JOYSTICK_CENTER,
      "CENTER"
    },
    {
      JOYSTICK_UP,
      "UP",
    },
    {
      JOYSTICK_LEFT,
      "LEFT",
    },
    {
      JOYSTICK_RIGHT,
      "RIGHT",
    },
    {
      JOYSTICK_DOWN,
      "DOWN"
    }
  };
  for (int i=0; i<ARRAY_SIZE(buttons); i++) {
    led_message lm = {
      .update = MSG_STATIC
    };
    strcpy(lm.message, buttons[i].name);
    display_set_message(disp, &lm);
    int pressed;
    while((pressed = get_buttons_pushed()) == 0)
      continue;
    if (pressed != (1 << buttons[i].button_id)) {
      NRF_LOG_INFO("SELFTEST FAILED");
      led_message fail = {
        .update = MSG_STATIC,
        .message = "FAIL"
      };
      display_set_message(disp, &fail);
      //busy loop forever!
      while(1);
    }
    while (get_buttons_pushed());
  }
  NRF_LOG_INFO("SELFTEST PASSED.");
  led_message pass = {
    .update = MSG_STATIC,
    .message = "PASS"
  };
  display_set_message(disp, &pass);
  nrf_delay_ms(1000);
  return;
}
