#include <stdint.h>
#include <string.h>

#include "ble_manager.h"
#include "led_display.h"

#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "ble.h"
#include "ble_srv_common.h"
#include "ble_advertising.h"
#include "ble_advdata.h"
#include "ble_conn_params.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"


NRF_BLE_GATT_DEF(m_gatt);
NRF_BLE_QWR_DEF(m_qwr);
BLE_ADVERTISING_DEF(m_advertising);

static void ble_setup_badge_service(led_display *disp);
static void ble_error_handler(uint32_t nrf_error);
static void ble_badge_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context);
static uint32_t ble_badge_add_onoff_characteristic();
static void ble_badge_handle_onoff_write(uint8_t val);
static void conn_params_init();
static void gap_params_init();
static void advertising_init();
static void advertising_start();

static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;
static ble_uuid_t m_adv_uuids[1] = {0};

static ble_badge_service_t ble_badge_svc = {0};

void ble_stack_init(led_display *disp) {
  APP_ERROR_CHECK(nrf_sdh_enable_request());
  uint32_t ram_start = 0;
  APP_ERROR_CHECK(nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start));
  APP_ERROR_CHECK(nrf_sdh_ble_enable(&ram_start));

  gap_params_init();
  conn_params_init();
  ble_setup_badge_service(disp);
  advertising_init();

  APP_ERROR_CHECK(nrf_ble_gatt_init(&m_gatt, NULL));

  advertising_start();
  //TODO: Add device information service
}

static void ble_setup_badge_service(led_display *disp) {
  ble_badge_svc.display = disp;

  ble_uuid128_t base_uuid = {BADGE_SERVICE_BASE};
  APP_ERROR_CHECK(sd_ble_uuid_vs_add(&base_uuid, &ble_badge_svc.uuid_type));

  ble_uuid_t ble_uuid;
  ble_uuid.type = ble_badge_svc.uuid_type;
  ble_uuid.uuid = BADGE_SERVICE_UUID;

  APP_ERROR_CHECK(sd_ble_gatts_service_add(
        BLE_GATTS_SRVC_TYPE_PRIMARY,
        &ble_uuid,
        &ble_badge_svc.service_handle));

  // Add the characteristics
  APP_ERROR_CHECK(ble_badge_add_onoff_characteristic());

  // Register event handler
  NRF_SDH_BLE_OBSERVER(
      m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_badge_on_ble_evt, NULL);
}

static void ble_error_handler(uint32_t nrf_error) {
  APP_ERROR_HANDLER(nrf_error);
}

/** GAP Parameters */
static void gap_params_init() {
  ble_gap_conn_params_t gap_conn_params;
  ble_gap_conn_sec_mode_t sec_mode;

  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);  /* TODO: add security */

  APP_ERROR_CHECK(sd_ble_gap_device_name_set(
        &sec_mode,
        (const uint8_t *)DEVICE_NAME,
        strlen(DEVICE_NAME)));

  gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
  gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
  gap_conn_params.slave_latency = SLAVE_LATENCY;
  gap_conn_params.conn_sup_timeout = CONN_SUP_TIMEOUT;

  APP_ERROR_CHECK(sd_ble_gap_ppcp_set(&gap_conn_params));
}

/** Connection parameters */
static void conn_params_init() {
  ble_conn_params_init_t cp_init = {0};

  cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
  cp_init.next_conn_params_update_delay = NEXT_CONN_PARAMS_UPDATE_DELAY;
  cp_init.max_conn_params_update_count = MAX_CONN_PARAMS_UPDATE_COUNT;
  cp_init.start_on_notify_cccd_handle = BLE_GATT_HANDLE_INVALID;
  cp_init.error_handler = ble_error_handler;
  cp_init.disconnect_on_fail = true;

  APP_ERROR_CHECK(ble_conn_params_init(&cp_init));
}

/** Advertising setup */
static void advertising_init() {
  m_adv_uuids[0].uuid = BADGE_SERVICE_UUID;
  m_adv_uuids[0].type = ble_badge_svc.uuid_type;

  ble_advertising_init_t init = {
    .advdata = {
      .name_type = BLE_ADVDATA_FULL_NAME,
      .flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE,
      .uuids_complete = {
        .uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]),
        .p_uuids = m_adv_uuids,
      }
    },
    .config = {
      .ble_adv_directed_enabled = true,
      .ble_adv_fast_enabled = true,
      .ble_adv_fast_interval = APP_ADV_FAST_INTERVAL,
      .ble_adv_fast_timeout = APP_ADV_FAST_TIMEOUT,
      .ble_adv_slow_enabled = true,
      .ble_adv_slow_interval = APP_ADV_SLOW_INTERVAL,
      .ble_adv_slow_timeout = APP_ADV_SLOW_TIMEOUT,
    },
    .error_handler = ble_error_handler,
  };

  APP_ERROR_CHECK(ble_advertising_init(&m_advertising, &init));
  ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}

/** Start the advertising */
static void advertising_start() {
  /* TODO: use bonds */
  APP_ERROR_CHECK(ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST));
}

/** Handle BLE Events */
static void ble_badge_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context) {

  switch (p_ble_evt->header.evt_id) {
    case BLE_GAP_EVT_CONNECTED:
      m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
      break;
    case BLE_GAP_EVT_DISCONNECTED:
      m_conn_handle = BLE_CONN_HANDLE_INVALID;
      break;
    case BLE_GATTS_EVT_WRITE:
      if (p_ble_evt->evt.gatts_evt.params.write.handle ==
          ble_badge_svc.onoff_handles.value_handle) {
        ble_badge_handle_onoff_write(
            p_ble_evt->evt.gatts_evt.params.write.data[0]);
        break;
      }
      break;
    default:
      break;
  }
}

static void ble_badge_handle_onoff_write(uint8_t val) {
  display_mode(ble_badge_svc.display, val & 1, 0);
}

static uint32_t ble_badge_add_onoff_characteristic() {
  ble_gatts_char_md_t char_md = {0};
  ble_gatts_attr_md_t attr_md = {0};
  ble_gatts_attr_t    attr_value = {0};
  ble_uuid_t          ble_uuid;
  static char char_desc[] = "ON/OFF";

  char_md.char_props.read = 1;
  char_md.char_props.write = 1;
  char_md.p_char_user_desc = (uint8_t *)char_desc;
  char_md.char_user_desc_size = strlen(char_desc);
  char_md.char_user_desc_max_size = char_md.char_user_desc_size;

  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);  /*TODO: add security */
  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm); /*TODO: add security */
  attr_md.vloc = BLE_GATTS_VLOC_STACK;
  attr_md.rd_auth = 0;
  attr_md.wr_auth = 0;
  attr_md.vlen = 0;

  attr_value.p_uuid = &ble_uuid;
  attr_value.p_attr_md = &attr_md;
  attr_value.init_len = sizeof(uint8_t);
  attr_value.init_offs = 0;
  attr_value.max_len = sizeof(uint8_t);
  attr_value.p_value = &ble_badge_svc.display->on;

  ble_uuid.type = ble_badge_svc.uuid_type;
  ble_uuid.uuid = BADGE_ONOFF_UUID;
  return sd_ble_gatts_characteristic_add(
      ble_badge_svc.service_handle,
      &char_md,
      &attr_value,
      &ble_badge_svc.onoff_handles);
}
