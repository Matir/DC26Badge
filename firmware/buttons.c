#include "buttons.h"
#include "nrf_log.h"

static void handle_joystick_button(uint8_t pin_no, uint8_t button_action);
static void handle_ble_button(uint8_t pin_no, uint8_t button_action);

static bool joystick_enabled = 0;
static joystick_actions_t *joystick_actions = NULL;
static ble_callback_t *ble_accept_cb = NULL;

void buttons_init(joystick_actions_t *actions) {
  joystick_actions = actions;
  const static app_button_cfg_t buttons[] = {
    DEF_BUTTON(JOYSTICK_UP, handle_joystick_button),
    /*
    DEF_BUTTON(JOYSTICK_CENTER, handle_joystick_button),
    DEF_BUTTON(JOYSTICK_LEFT, handle_joystick_button),
    DEF_BUTTON(JOYSTICK_RIGHT, handle_joystick_button),
    DEF_BUTTON(JOYSTICK_DOWN, handle_joystick_button),
    */
  };
  APP_ERROR_CHECK(app_button_init(buttons, ARRAY_SIZE(buttons), 50));
  APP_ERROR_CHECK(app_button_enable());
}

void joystick_set_enable(uint8_t enabled) {
  joystick_enabled = enabled;
}

void buttons_set_ble_callback(ble_callback_t *ble_cb) {
  ble_accept_cb = ble_cb;
}

static void handle_joystick_button(uint8_t pin_no, uint8_t button_action) {
  if(!joystick_enabled) {
    if (pin_no == BUTTON_BLE_PAIR || pin_no == BUTTON_BLE_REJECT)
      handle_ble_button(pin_no, button_action);
    return;
  }
  // TODO: support for constant scrolling?
  if (button_action != APP_BUTTON_PUSH)
    return;
  NRF_LOG_INFO("Joystick button pressed: %d", (uint32_t)pin_no);
  switch (pin_no) {
    case JOYSTICK_UP:
      break;
    case JOYSTICK_DOWN:
      break;
    case JOYSTICK_LEFT:
      break;
    case JOYSTICK_RIGHT:
      break;
    case JOYSTICK_CENTER:
      break;
    default:
      NRF_LOG_INFO("Unknown joystick movement: %d", pin_no);
  }
}

static void handle_ble_button(uint8_t pin_no, uint8_t button_action) {
  if (button_action != APP_BUTTON_PUSH)
    return;
  NRF_LOG_INFO("BLE button pressed: %d", (uint32_t)pin_no);
  if (!ble_accept_cb)
    return;
  ble_accept_cb(pin_no == BUTTON_BLE_PAIR ? 1 : 0);
}
