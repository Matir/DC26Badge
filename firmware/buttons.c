#include "buttons.h"
#include "nrf_log.h"

static void handle_joystick_button(uint8_t pin_no, uint8_t button_action);
static void handle_ble_button(uint8_t pin_no, uint8_t button_action);

static bool joystick_enabled = 0;
static joystick_actions_t *joystick_actions = NULL;

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

static void handle_joystick_button(uint8_t pin_no, uint8_t button_action) {
  NRF_LOG_INFO("Joystick button pressed: %d", (uint32_t)pin_no);
  if(!joystick_enabled) {
    if (pin_no == BUTTON_BLE_PAIR)
      handle_ble_button(pin_no, button_action);
    return;
  }
}

static void handle_ble_button(uint8_t pin_no, uint8_t button_action) {
  NRF_LOG_INFO("BLE button pressed: %d", (uint32_t)pin_no);
}
