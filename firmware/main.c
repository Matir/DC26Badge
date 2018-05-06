#include "sdk_config.h"

#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrfx_twim.h"

#include "ht16k33.h"

#ifndef NRFX_TWIM0_ENABLED
# error TWIM0 is not enabled.
#endif

#ifdef TWI_ENABLED
# error Enabling TWI disables TWIM.
#endif

//TODO: move to board config

#define PIN_SCL 14
#define PIN_SDA 15

led_display display = {0};

static inline void log_init(void) {
  APP_ERROR_CHECK(NRF_LOG_INIT(NULL));
  NRF_LOG_DEFAULT_BACKENDS_INIT();
}

static inline void power_management_init(void) {
  APP_ERROR_CHECK(nrf_pwr_mgmt_init());
}

static inline void twi_init(nrfx_twim_t *master) {
  nrfx_twim_config_t config = {
    .scl = PIN_SCL,
    .sda = PIN_SDA,
    .frequency = (nrf_twim_frequency_t)NRF_TWIM_FREQ_100K,
    .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
    .hold_bus_uninit = false,
  };
  APP_ERROR_CHECK(nrfx_twim_init(master, &config, NULL, NULL));
  nrfx_twim_enable(master);

  // Config GPIOs for these pins

}

static inline void ble_stack_init() {
  APP_ERROR_CHECK(nrf_sdh_enable_request());
  uint32_t ram_start = 0;
  APP_ERROR_CHECK(nrf_sdh_ble_default_cfg_set(1, &ram_start));
  APP_ERROR_CHECK(nrf_sdh_ble_enable(&ram_start));

  // TODO: register handlers
}

int main(void) {
  nrfx_twim_t twi_master = NRFX_TWIM_INSTANCE(0);

  log_init();

  NRF_LOG_INFO("Initialized logging.");

  power_management_init();
  twi_init(&twi_master);

  NRF_LOG_INFO("Setting up BLE.");
  ble_stack_init();

  NRF_LOG_INFO("Setting up display.");

  init_led_display(&display, &twi_master, 0x70);
  display_on(&display);
  display_text(&display, (uint8_t *)"HACKCUBS");

  NRF_LOG_INFO("Entering main loop...");

  while (1) {
    if (!NRF_LOG_PROCESS())
      nrf_pwr_mgmt_run();
  }
}
