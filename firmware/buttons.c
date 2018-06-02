#include "buttons.h"
#include "led_display.h"
#include "ble_manager.h"
#include "nrf_log.h"
#include "app_timer.h"

#define RETURN_IF(x) \
  if((x)) return

#define LONG_PRESS APP_TIMER_TICKS(2000)

static void handle_joystick_button(uint8_t pin_no, uint8_t button_action);
static void handle_ble_button(uint8_t pin_no, uint8_t button_action);

static bool joystick_enabled = 0;
static ble_callback_t *ble_accept_cb = NULL;
static led_display *display = NULL;

void buttons_init(led_display *disp) {
  display = disp;
  const static app_button_cfg_t buttons[] = {
    DEF_BUTTON(JOYSTICK_UP, handle_joystick_button),
    DEF_BUTTON(JOYSTICK_CENTER, handle_joystick_button),
    DEF_BUTTON(JOYSTICK_LEFT, handle_joystick_button),
    DEF_BUTTON(JOYSTICK_RIGHT, handle_joystick_button),
    DEF_BUTTON(JOYSTICK_DOWN, handle_joystick_button),
  };
  APP_ERROR_CHECK(app_button_init(buttons, ARRAY_SIZE(buttons), 50));
  APP_ERROR_CHECK(app_button_enable());
}

void joystick_set_enable(uint8_t enabled) {
  joystick_enabled = enabled;
}

void buttons_set_ble_accept_callback(ble_callback_t *ble_cb) {
  ble_accept_cb = ble_cb;
}

static void handle_joystick_button(uint8_t pin_no, uint8_t button_action) {
  static uint32_t button_down_time;
  if (button_action == APP_BUTTON_PUSH)
    button_down_time = app_timer_cnt_get();
  if(!joystick_enabled) {
    if (pin_no == BUTTON_BLE_PAIR || pin_no == BUTTON_BLE_REJECT)
      handle_ble_button(pin_no, button_action);
    return;
  }
  NRF_LOG_INFO("Joystick button %s: %d",
      (uint32_t)(button_action == APP_BUTTON_PUSH ? "pushed" : "released"),
      (uint32_t)pin_no);
  switch (pin_no) {
    case JOYSTICK_UP:
      display_prev_message(display);
      break;
    case JOYSTICK_DOWN:
      display_next_message(display);
      break;
    case JOYSTICK_LEFT:
      display_dec_brightness(display);
      break;
    case JOYSTICK_RIGHT:
      display_inc_brightness(display);
      break;
    case JOYSTICK_CENTER:
      RETURN_IF(button_action == APP_BUTTON_PUSH);
      if (app_timer_cnt_diff_compute(app_timer_cnt_get(), button_down_time) > LONG_PRESS)
        handle_ble_button(pin_no, button_action);
      break;
    default:
      NRF_LOG_INFO("Unknown joystick movement: %d", pin_no);
  }
}

static void handle_ble_button(uint8_t pin_no, uint8_t button_action) {
  if (pin_no == JOYSTICK_CENTER) {
    ble_manager_start_advertising();
    return;
  }
  RETURN_IF(button_action != APP_BUTTON_PUSH);
  NRF_LOG_INFO("BLE button pressed: %d", (uint32_t)pin_no);
  if (!ble_accept_cb)
    return;
  ble_accept_cb(pin_no == BUTTON_BLE_PAIR ? 1 : 0);
}
