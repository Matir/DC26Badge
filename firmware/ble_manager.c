#include <stdint.h>

#include "ble_manager.h"

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

void ble_badge_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context);
uint32_t ble_badge_add_onoff_characteristic(ble_badge_service_t *svc);

static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;

void ble_stack_init() {
  APP_ERROR_CHECK(nrf_sdh_enable_request());
  uint32_t ram_start = 0;
  APP_ERROR_CHECK(nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start));
  APP_ERROR_CHECK(nrf_sdh_ble_enable(&ram_start));

  // TODO: register handlers
}

void ble_badge_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context) {
  ble_badge_service_t *svc = (ble_badge_service_t *)p_context;

  switch (p_ble_evt->header.evt_id) {
    case BLE_GAP_EVT_CONNECTED:
      m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
      break;
    case BLE_GAP_EVT_DISCONNECTED:
      m_conn_handle = BLE_CONN_HANDLE_INVALID;
      break;
    case BLE_GATTS_EVT_WRITE:
      break;
    default:
      break;
  }
}

// TODO: also get pointers to message memory
void ble_setup_badge_service(ble_badge_service_t *svc) {
  ble_uuid128_t base_uuid = {BADGE_SERVICE_BASE};
  APP_ERROR_CHECK(sd_ble_uuid_vs_add(&base_uuid, &svc->uuid_type));

  ble_uuid_t ble_uuid;
  ble_uuid.type = svc->uuid_type;
  ble_uuid.uuid = BADGE_SERVICE_UUID;

  APP_ERROR_CHECK(sd_ble_gatts_service_add(
        BLE_GATTS_SRVC_TYPE_PRIMARY,
        &ble_uuid,
        &svc->service_handle));

  // Add the characteristics
  ble_badge_add_onoff_characteristic(svc);
}

uint32_t ble_badge_add_onoff_characteristic(ble_badge_service_t *svc) {
  ble_gatts_char_md_t char_md = {0};
  ble_gatts_attr_md_t attr_md = {0};
  ble_gatts_attr_t    attr_value = {0};
  ble_uuid_t          ble_uuid;

  char_md.char_props.read = 1;
  char_md.char_props.write = 1;

  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);  /*TODO: add security */
  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm); /*TODO: add security */
  attr_md.vloc = BLE_GATTS_VLOCK_STACK;
  attr_md.rd_auth = 0;
  attr_md.wr_auth = 0;
  attr_md.vlen = 0;

  attr_value.p_uuid = &ble_uuid;
  attr_value.p_attr_md = &attr_md;
  attr_value.init_len = sizeof(uint8_t);
  attr_value.init_offs = 0;
  attr_value.max_len = sizeof(uint8_t);
  attr_value.p_value = NULL;

  ble_uuid.type = svc->uuid_type;
  ble_uuid.uuid = BADGE_ONOFF_UUID;
  return sd_ble_gatts_characteristic_add(
      svc->service_handle,
      &char_md,
      &attr_value,
      &svc->onoff_handles);
}
