#include <stdint.h>
#include <string.h>

#include "ble_manager.h"
#include "led_display.h"

#include "nordic_common.h"
#include "nrf.h"
#include "nrf_log.h"
#include "app_error.h"
#include "ble.h"
#include "ble_srv_common.h"
#include "ble_advertising.h"
#include "ble_advdata.h"
#include "ble_conn_params.h"
#include "ble_lesc.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "peer_manager.h"
#include "fds.h"

#if DEBUG_BLE
# define EVT_DEBUG NRF_LOG_INFO
#else
# define EVT_DEBUG(...)
#endif


NRF_BLE_GATT_DEF(m_gatt);
NRF_BLE_QWR_DEF(m_qwr);
BLE_ADVERTISING_DEF(m_advertising);

static void ble_setup_badge_service(led_display *disp);
static void ble_error_handler(uint32_t nrf_error);
static void ble_badge_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context);
static uint32_t ble_badge_add_onoff_characteristic();
static uint32_t ble_badge_add_message_characteristics();
static uint32_t ble_badge_add_message_characteristic(led_message *msg, uint16_t idx);
static void ble_badge_handle_onoff_write(uint8_t val);
static void conn_params_init();
static void gap_params_init();
static void advertising_init();
static void advertising_start();
static void peer_manager_init();

char *ble_evt_decode(uint16_t code);

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
  peer_manager_init();
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
  APP_ERROR_CHECK(ble_badge_add_message_characteristics());

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

/** Conn params error handler */
static void conn_params_error_handler(uint32_t nrf_error) {
  NRF_LOG_ERROR("Error updating connections params: %d", nrf_error);
  APP_ERROR_HANDLER(nrf_error);
}

/** Conn params evt handler */
static void on_conn_params_evt(ble_conn_params_evt_t *p_evt) {
  if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED) {
    NRF_LOG_WARNING("Failed negotiating connection parameters.");
    sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
    return;
  }
}

/** Connection parameters */
static void conn_params_init() {
  ble_conn_params_init_t cp_init = {0};

  cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
  cp_init.next_conn_params_update_delay = NEXT_CONN_PARAMS_UPDATE_DELAY;
  cp_init.max_conn_params_update_count = MAX_CONN_PARAMS_UPDATE_COUNT;
  cp_init.start_on_notify_cccd_handle = BLE_GATT_HANDLE_INVALID;
  cp_init.error_handler = conn_params_error_handler;
  cp_init.evt_handler = on_conn_params_evt;
  cp_init.disconnect_on_fail = false;  // TODO: debugging

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
      EVT_DEBUG("Connected");
      m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
      break;
    case BLE_GAP_EVT_DISCONNECTED:
      EVT_DEBUG("Disconnected");
      m_conn_handle = BLE_CONN_HANDLE_INVALID;
      break;
    case BLE_GATTS_EVT_WRITE:
      EVT_DEBUG("GATTS Write event");
      if (p_ble_evt->evt.gatts_evt.params.write.handle ==
          ble_badge_svc.onoff_handles.value_handle) {
        ble_badge_handle_onoff_write(
            p_ble_evt->evt.gatts_evt.params.write.data[0]);
        break;
      }
      break;
    case BLE_GATTS_EVT_TIMEOUT:
      EVT_DEBUG("GATTS Timeout");
      APP_ERROR_CHECK(sd_ble_gap_disconnect(
            p_ble_evt->evt.gatts_evt.conn_handle,
            BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION));
      break;
    case BLE_GATTC_EVT_TIMEOUT:
      EVT_DEBUG("GATTC Timeout");
      APP_ERROR_CHECK(sd_ble_gap_disconnect(
            p_ble_evt->evt.gattc_evt.conn_handle,
            BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION));
      break;
    case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
      EVT_DEBUG("PHY Update request.");
      ble_gap_phys_t const phys =
      {
        .rx_phys = BLE_GAP_PHY_AUTO,
        .tx_phys = BLE_GAP_PHY_AUTO,
      };
      APP_ERROR_CHECK(sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys));
      break;
    default:
      EVT_DEBUG("Unhandled BLE event: %s", (uint32_t)ble_evt_decode(p_ble_evt->header.evt_id));
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
  char_md.char_props.write_wo_resp = 1;
  char_md.p_char_user_desc = (uint8_t *)char_desc;
  char_md.char_user_desc_size = strlen(char_desc);
  char_md.char_user_desc_max_size = char_md.char_user_desc_size;

#if BLE_SECURITY
  BLE_GAP_CONN_SEC_MODE_SET_LESC_ENC_WITH_MITM(&attr_md.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_LESC_ENC_WITH_MITM(&attr_md.write_perm);
#else
  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);  /*TODO: add security */
  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm); /*TODO: add security */
#endif
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

static uint32_t ble_badge_add_message_characteristics() {
  uint32_t rv;
  for(uint16_t i=0; i<NUM_MESSAGES; i++) {
    if ((rv = ble_badge_add_message_characteristic(&message_set[i], i)) != 0)
      return rv;
  }
  return rv;
}

static uint32_t ble_badge_add_message_characteristic(led_message *msg, uint16_t idx) {
  ble_gatts_char_md_t char_md = {0};
  ble_gatts_attr_md_t attr_md = {0};
  ble_gatts_attr_t attr_value = {0};
  ble_uuid_t ble_uuid;
  static char char_desc[] = "Message";

  char_md.char_props.read = 1;
  char_md.char_props.write = 1;
  char_md.char_props.write_wo_resp = 1;
  char_md.p_char_user_desc = (uint8_t *)char_desc;
  char_md.char_user_desc_size = strlen(char_desc);
  char_md.char_user_desc_max_size = char_md.char_user_desc_size;

#if BLE_SECURITY
  BLE_GAP_CONN_SEC_MODE_SET_LESC_ENC_WITH_MITM(&attr_md.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_LESC_ENC_WITH_MITM(&attr_md.write_perm);
#else
  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);  /*TODO: add security */
  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm); /*TODO: add security */
#endif
  attr_md.vloc = BLE_GATTS_VLOC_USER;
  attr_md.rd_auth = 0;
  attr_md.wr_auth = 0;
  attr_md.vlen = 1;

  attr_value.p_uuid = &ble_uuid;
  attr_value.p_attr_md = &attr_md;
  attr_value.init_len = sizeof(led_message);
  attr_value.max_len = sizeof(led_message);
  attr_value.p_value = (void *)msg;

  ble_uuid.type = ble_badge_svc.uuid_type;
  ble_uuid.uuid = BADGE_MSG_UUID_FIRST + idx;
  return sd_ble_gatts_characteristic_add(
      ble_badge_svc.service_handle,
      &char_md,
      &attr_value,
      &ble_badge_svc.message_handles[idx]);
}

static void pm_evt_handler(pm_evt_t const *p_evt) {
  switch (p_evt->evt_id) {
    case PM_EVT_BONDED_PEER_CONNECTED:
      EVT_DEBUG("PM_EVT_BONDED_PEER_CONNECTED: peer_id=%d", p_evt->peer_id);
      break;
    case PM_EVT_CONN_SEC_START:
      EVT_DEBUG("PM_EVT_CONN_SEC_START: peer_id=%d", p_evt->peer_id);
      break;
    case PM_EVT_CONN_SEC_SUCCEEDED:
      EVT_DEBUG("PM_EVT_CONN_SEC_SUCCEEDED: conn_handle=%d, procedure=%d",
          p_evt->conn_handle, p_evt->params.conn_sec_succeeded.procedure);
      break;
    case PM_EVT_CONN_SEC_FAILED:
      EVT_DEBUG("PM_EVT_CONN_SEC_FAILED: conn_handle=%d, error=%d",
          p_evt->conn_handle, p_evt->params.conn_sec_failed.error);
      break;
    case PM_EVT_CONN_SEC_CONFIG_REQ:
      // Already bonded, why did we get this?
      {
        pm_conn_sec_config_t conn_sec_config = {.allow_repairing = false};
        pm_conn_sec_config_reply(p_evt->conn_handle, &conn_sec_config);
      }
      break;
    case PM_EVT_STORAGE_FULL:
      // garbage collect
      fds_gc();
      break;
    case PM_EVT_PEER_DATA_UPDATE_FAILED:
      APP_ERROR_CHECK(p_evt->params.peer_data_update_failed.error);
      break;
    case PM_EVT_PEER_DELETE_FAILED:
      APP_ERROR_CHECK(p_evt->params.peer_delete_failed.error);
      break;
    case PM_EVT_PEERS_DELETE_FAILED:
      APP_ERROR_CHECK(p_evt->params.peers_delete_failed_evt.error);
      break;
    case PM_EVT_ERROR_UNEXPECTED:
      APP_ERROR_CHECK(p_evt->params.error_unexpected.error);
      break;
    default:
      break;
  }
}

// Setup our peer manager
static void peer_manager_init() {
  ble_gap_sec_params_t sec_params = {
    .bond             = SEC_PARAM_BOND,
    .mitm             = SEC_PARAM_MITM,
    .lesc             = SEC_PARAM_LESC,
    .keypress         = SEC_PARAM_KEYPRESS,
    .io_caps          = SEC_PARAM_IO_CAPABILITIES,
    .oob              = SEC_PARAM_OOB,
    .min_key_size     = SEC_PARAM_MIN_KEY_SIZE,
    .max_key_size     = SEC_PARAM_MAX_KEY_SIZE,
    .kdist_own        = {
      .enc = 1,
      .id = 1,
    },
    .kdist_peer       = {
      .enc = 1,
      .id = 1,
    }
  };
  APP_ERROR_CHECK(pm_init());
  APP_ERROR_CHECK(pm_sec_params_set(&sec_params));
  APP_ERROR_CHECK(pm_register(pm_evt_handler));
  APP_ERROR_CHECK(ble_lesc_ecc_keypair_generate_and_set());
}
