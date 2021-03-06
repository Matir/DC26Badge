#include <stdint.h>
#include <string.h>

#include "ble_manager.h"
#include "led_display.h"
#include "buttons.h"
#include "storage.h"

#include "app_error.h"
#include "app_scheduler.h"
#include "ble.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "ble_lesc.h"
#include "ble_srv_common.h"
#include "fds.h"
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#include "peer_manager.h"

#if DEBUG_BLE
# define EVT_DEBUG NRF_LOG_INFO
#else
# define EVT_DEBUG(...)
#endif

#define MASK_CHANNEL(ch_mask, ch) \
  do { \
    uint8_t local_channel = (ch); \
    (ch_mask)[local_channel/8] |= (1 << (local_channel & 0x7)); \
  } while(0);


NRF_BLE_GATT_DEF(m_gatt);
NRF_BLE_QWR_DEF(m_qwr);
BLE_ADVERTISING_DEF(m_advertising);

static void ble_advertising_setup();
static void ble_setup_badge_service(led_display *disp);
static void ble_error_handler(uint32_t nrf_error);
static void ble_badge_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context);
static uint32_t ble_badge_add_onoff_characteristic();
static uint32_t ble_badge_add_brightness_characteristic();
static uint32_t ble_badge_add_index_characteristic();
static uint32_t ble_badge_add_message_characteristics();
static uint32_t ble_badge_add_message_characteristic(led_message *msg, uint16_t idx);
static void ble_badge_handle_onoff_write(uint8_t val);
static void ble_badge_handle_brightness_write(uint8_t val);
static void ble_badge_handle_index_write(int8_t val);
static void conn_params_init();
static void gap_params_init();
static void advertising_init();
static void peer_manager_init();
static void qwr_init();
static uint16_t qwr_evt_handler(struct nrf_ble_qwr_t *p_qwr, nrf_ble_qwr_evt_t *p_evt);
static void app_save_messages(void *unused_ptr, uint16_t unused_size);

char *ble_evt_decode(uint16_t code);

static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;
static uint16_t m_pending_conn_handle = BLE_CONN_HANDLE_INVALID;
static ble_uuid_t m_adv_uuids[1] = {0};

static char device_name[32] __attribute__ ((aligned(4))) = DEVICE_NAME;
static ble_badge_service_t ble_badge_svc = {0};

void ble_stack_init(led_display *disp) {
  APP_ERROR_CHECK(nrf_sdh_enable_request());
  uint32_t ram_start = 0;
  APP_ERROR_CHECK(nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start));
  APP_ERROR_CHECK(nrf_sdh_ble_enable(&ram_start));
  NRF_LOG_INFO("SDH started, setting BLE params.");

  int device_name_len = sizeof(device_name);
  get_device_name(device_name, &device_name_len);
  device_name[sizeof(device_name)-1] = '\0';

  gap_params_init();
  conn_params_init();
  qwr_init();
  peer_manager_init();
  ble_setup_badge_service(disp);
  advertising_init();

  APP_ERROR_CHECK(nrf_ble_gatt_init(&m_gatt, NULL));
  nrf_gpio_cfg_output(ADV_LED_PIN);
  nrf_gpio_pin_set(ADV_LED_PIN); // we use low, so this is "off"

  //TODO: Add device information service
  ble_manager_start_advertising();
}

void ble_main(void) {
  APP_ERROR_CHECK(ble_lesc_service_request_handler());
}

void ble_manager_start_advertising() {
  ble_advertising_setup();
#ifdef BLE_ADVERTISE_37
  MASK_CHANNEL(m_advertising.adv_params.channel_mask, 38);
  MASK_CHANNEL(m_advertising.adv_params.channel_mask, 39);
#endif
  ret_code_t rv = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
  if (rv == NRF_ERROR_CONN_COUNT) {
    NRF_LOG_ERROR("Can't advertise while connected.");
    joystick_enable();
    return;
  }
  if (rv != NRF_SUCCESS) {
    NRF_LOG_ERROR("Error advertising: %d", rv);
    joystick_enable();
  }
}

static void ble_advertising_setup() {
  NRF_LOG_INFO("Starting advertising...");
  joystick_disable();
  display_show_pairing_code(ble_badge_svc.display, NULL);
  nrf_gpio_pin_clear(ADV_LED_PIN);
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
  APP_ERROR_CHECK(ble_badge_add_brightness_characteristic());
  APP_ERROR_CHECK(ble_badge_add_index_characteristic());
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

#if BLE_SECURITY
  BLE_GAP_CONN_SEC_MODE_SET_LESC_ENC_WITH_MITM(&sec_mode);
#else
  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);  /* TODO: add security */
#endif

  APP_ERROR_CHECK(sd_ble_gap_device_name_set(
        &sec_mode,
        (uint8_t *)device_name,
        strlen(device_name)));

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
  ble_conn_params_init_t cp_init = {
    .first_conn_params_update_delay   = FIRST_CONN_PARAMS_UPDATE_DELAY,
    .next_conn_params_update_delay    = NEXT_CONN_PARAMS_UPDATE_DELAY,
    .max_conn_params_update_count     = MAX_CONN_PARAMS_UPDATE_COUNT,
    .start_on_notify_cccd_handle      = BLE_GATT_HANDLE_INVALID,
    .error_handler                    = conn_params_error_handler,
    .evt_handler                      = on_conn_params_evt,
    .disconnect_on_fail               = true,
  };

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

static void qwr_init() {
  static uint8_t qwr_buf[256];
  nrf_ble_qwr_init_t qwr_init = {
    .error_handler = ble_error_handler,
    .mem_buffer = {
      .p_mem = qwr_buf,
      .len = 256,
    },
    .callback = qwr_evt_handler,
  };
  APP_ERROR_CHECK(nrf_ble_qwr_init(&m_qwr, &qwr_init));
}

static uint16_t qwr_evt_handler(struct nrf_ble_qwr_t *p_qwr, nrf_ble_qwr_evt_t *p_evt) {
  if (p_evt->evt_type != NRF_BLE_QWR_EVT_AUTH_REQUEST)
    return 0;
  return BLE_GATT_STATUS_SUCCESS;
}

/** Handle BLE Events */
static void ble_badge_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context) {
  switch (p_ble_evt->header.evt_id) {
    case BLE_GAP_EVT_CONNECTED:
      EVT_DEBUG("Connected");
      m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
      nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
      break;
    case BLE_GAP_EVT_DISCONNECTED:
      EVT_DEBUG("Disconnected");
      m_conn_handle = BLE_CONN_HANDLE_INVALID;
      m_pending_conn_handle = BLE_CONN_HANDLE_INVALID;
      display_show_pairing_code(ble_badge_svc.display, NULL);
      // The advertising module will restart advertising automatically,
      // so we put things back into advertising mode
      ble_advertising_setup();
      break;
    case BLE_GATTS_EVT_WRITE:
      EVT_DEBUG("GATTS Write event");
      uint16_t handle = p_ble_evt->evt.gatts_evt.params.write.handle;
      EVT_DEBUG("Handle: %d", handle);
      if (handle == ble_badge_svc.onoff_handles.value_handle) {
        ble_badge_handle_onoff_write(
            p_ble_evt->evt.gatts_evt.params.write.data[0]);
        break;
      } else if (handle == ble_badge_svc.brightness_handles.value_handle) {
        ble_badge_handle_brightness_write(
            p_ble_evt->evt.gatts_evt.params.write.data[0]);
        break;
      } else if (handle == ble_badge_svc.index_handles.value_handle) {
        ble_badge_handle_index_write(
            p_ble_evt->evt.gatts_evt.params.write.data[0]);
        break;
      } else if (p_ble_evt->evt.gatts_evt.params.write.uuid.type
            == BLE_UUID_TYPE_BLE &&
          p_ble_evt->evt.gatts_evt.params.write.uuid.uuid
            == BLE_UUID_GAP_CHARACTERISTIC_DEVICE_NAME) {
        // Update to device name
        uint16_t device_name_len = sizeof(device_name)-1;
        if (sd_ble_gap_device_name_get((uint8_t *)device_name, &device_name_len)
            == NRF_SUCCESS) {
          device_name[device_name_len] = '\0';
          save_device_name(device_name, device_name_len+1);
          // TODO: might need to fix this
          advertising_init();
        }
      } else {
        // Save all dirty messages
        app_sched_event_put(NULL, 0, app_save_messages);
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
      {
        EVT_DEBUG("PHY Update request.");
        ble_gap_phys_t const phys =
        {
          .rx_phys = BLE_GAP_PHY_AUTO,
          .tx_phys = BLE_GAP_PHY_AUTO,
        };
        APP_ERROR_CHECK(sd_ble_gap_phy_update(
              p_ble_evt->evt.gap_evt.conn_handle, &phys));
      }
      break;
    case BLE_GAP_EVT_PASSKEY_DISPLAY:
      {
        // Only display passkey when advertising
        if (m_advertising.adv_mode_current == BLE_ADV_MODE_IDLE) {
          EVT_DEBUG("Not advertising, rejecting immediately.");
          uint8_t key_type = BLE_GAP_AUTH_KEY_TYPE_NONE;
          APP_ERROR_CHECK(sd_ble_gap_auth_key_reply(
                m_pending_conn_handle, key_type, NULL));
          break;
        }
        joystick_disable();
        char passkey[BLE_GAP_PASSKEY_LEN+1] = {0};
        memcpy(&passkey, p_ble_evt->evt.gap_evt.params.passkey_display.passkey,
            BLE_GAP_PASSKEY_LEN);
        EVT_DEBUG("Passkey request, passkey=%s match_req=%d",
            nrf_log_push(passkey),
            p_ble_evt->evt.gap_evt.params.passkey_display.match_request);
        if (p_ble_evt->evt.gap_evt.params.passkey_display.match_request) {
          if (m_pending_conn_handle != BLE_CONN_HANDLE_INVALID) {
            EVT_DEBUG("Another conn handle is pending...");
            break;
          }
          m_pending_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
          display_show_pairing_code(ble_badge_svc.display, passkey);
        }
      }
      break;
    case BLE_GAP_EVT_ADV_SET_TERMINATED:
      EVT_DEBUG("ADV_SET_TERMINATED");
      nrf_gpio_pin_set(ADV_LED_PIN); // we use low, so this is "off"
      display_show_pairing_code(ble_badge_svc.display, NULL);
      joystick_enable();
      break;
    case BLE_EVT_USER_MEM_REQUEST:
      sd_ble_user_mem_reply(p_ble_evt->evt.gap_evt.conn_handle, NULL);
      break;
    case BLE_GAP_EVT_SEC_INFO_REQUEST:
      EVT_DEBUG("SEC_INFO_REQUEST");
      break;
    case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
      EVT_DEBUG("SEC_PARAMS_REQUEST");
      break;
    case BLE_GAP_EVT_AUTH_KEY_REQUEST:
      EVT_DEBUG("AUTH_KEY_REQUEST");
      break;
    case BLE_GAP_EVT_LESC_DHKEY_REQUEST:
      EVT_DEBUG("LESC_DHKEY_REQUEST");
      break;
    default:
      EVT_DEBUG("Unhandled BLE event: %s", (uint32_t)ble_evt_decode(p_ble_evt->header.evt_id));
      break;
  }
}

static void app_save_messages(void *unused_ptr, uint16_t unused_size) {
  display_save_storage();
}

void ble_match_request_respond(uint8_t matched) {
  display_show_pairing_code(ble_badge_svc.display, NULL);
  joystick_enable();
  if (m_pending_conn_handle == BLE_CONN_HANDLE_INVALID)
    return;
  uint8_t key_type;
  if (matched) {
    NRF_LOG_INFO("Accepted BLE pairing.");
    key_type = BLE_GAP_AUTH_KEY_TYPE_PASSKEY;
  } else {
    NRF_LOG_INFO("Failed BLE pairing.");
    key_type = BLE_GAP_AUTH_KEY_TYPE_NONE;
  }
  APP_ERROR_CHECK(sd_ble_gap_auth_key_reply(
        m_pending_conn_handle, key_type, NULL));
  m_pending_conn_handle = BLE_CONN_HANDLE_INVALID;
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
  char_md.char_props.write_wo_resp = 0;
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

static void ble_badge_handle_brightness_write(uint8_t val) {
  display_set_brightness(ble_badge_svc.display, val);
}

static uint32_t ble_badge_add_brightness_characteristic() {
  ble_gatts_char_md_t char_md = {0};
  ble_gatts_attr_md_t attr_md = {0};
  ble_gatts_attr_t    attr_value = {0};
  ble_uuid_t          ble_uuid;
  static char char_desc[] = "Brightness";

  char_md.char_props.read = 1;
  char_md.char_props.write = 1;
  char_md.char_props.write_wo_resp = 0;
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
  attr_md.vlen = 0;

  attr_value.p_uuid = &ble_uuid;
  attr_value.p_attr_md = &attr_md;
  attr_value.init_len = sizeof(uint8_t);
  attr_value.init_offs = 0;
  attr_value.max_len = sizeof(uint8_t);
  attr_value.p_value = &ble_badge_svc.display->brightness;

  ble_uuid.type = ble_badge_svc.uuid_type;
  ble_uuid.uuid = BADGE_BRIGHTNESS_UUID;
  return sd_ble_gatts_characteristic_add(
      ble_badge_svc.service_handle,
      &char_md,
      &attr_value,
      &ble_badge_svc.brightness_handles);
}

static void ble_badge_handle_index_write(int8_t val) {
  if (val < NUM_MESSAGES && val >= 0) {
    ble_badge_svc.display->cur_msg_idx = val;
    display_set_message(ble_badge_svc.display, &message_set[val]);
  }
}

static uint32_t ble_badge_add_index_characteristic() {
  ble_gatts_char_md_t char_md = {0};
  ble_gatts_attr_md_t attr_md = {0};
  ble_gatts_attr_t    attr_value = {0};
  ble_uuid_t          ble_uuid;
  static char char_desc[] = "Active Index";

  char_md.char_props.read = 1;
  char_md.char_props.write = 1;
  char_md.char_props.write_wo_resp = 0;
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
  attr_md.vlen = 0;

  attr_value.p_uuid = &ble_uuid;
  attr_value.p_attr_md = &attr_md;
  attr_value.init_len = sizeof(int8_t);
  attr_value.init_offs = 0;
  attr_value.max_len = sizeof(int8_t);
  attr_value.p_value = (uint8_t *)&ble_badge_svc.display->cur_msg_idx;

  ble_uuid.type = ble_badge_svc.uuid_type;
  ble_uuid.uuid = BADGE_INDEX_UUID;
  return sd_ble_gatts_characteristic_add(
      ble_badge_svc.service_handle,
      &char_md,
      &attr_value,
      &ble_badge_svc.index_handles);
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
  char_md.char_props.write_wo_resp = 0;
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
  ble_uuid.uuid = BADGE_MSG_UUID;
  ret_code_t rv = sd_ble_gatts_characteristic_add(
      ble_badge_svc.service_handle,
      &char_md,
      &attr_value,
      &ble_badge_svc.message_handles[idx]);
  if (rv != NRF_SUCCESS) {
    NRF_LOG_WARNING("Error in sd_ble_gatts_characteristic_add: %d", rv);
    return rv;
  }
  return nrf_ble_qwr_attr_register(
      &m_qwr, ble_badge_svc.message_handles[idx].value_handle);
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
      m_pending_conn_handle = BLE_CONN_HANDLE_INVALID;
      display_show_pairing_code(ble_badge_svc.display, NULL);
      // Reset the bond
      pm_peer_delete(p_evt->peer_id);
      break;
    case PM_EVT_CONN_SEC_CONFIG_REQ:
      EVT_DEBUG("PM_EVT_CONN_SEC_CONFIG_REQ");
      {
        pm_conn_sec_config_t conn_sec_config = {.allow_repairing = true};
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
    case PM_EVT_CONN_SEC_PARAMS_REQ:
      EVT_DEBUG("PM_EVT_CONN_SEC_PARAMS_REQ unhandled.");
      break;
    case PM_EVT_PEER_DATA_UPDATE_SUCCEEDED:
      EVT_DEBUG("Unhandled PM_EVT_PEER_DATA_UPDATE_SUCCEEDED");
      break;
    case PM_EVT_PEER_DELETE_SUCCEEDED:
      EVT_DEBUG("Unhandled PM_EVT_PEER_DELETE_SUCCEEDED");
      break;
    case PM_EVT_PEERS_DELETE_SUCCEEDED:
      EVT_DEBUG("Unhandled PM_EVT_PEERS_DELETE_SUCCEEDED");
      break;
    case PM_EVT_LOCAL_DB_CACHE_APPLIED:
      EVT_DEBUG("Unhandled PM_EVT_LOCAL_DB_CACHE_APPLIED");
      break;
    case PM_EVT_LOCAL_DB_CACHE_APPLY_FAILED:
      EVT_DEBUG("Unhandled PM_EVT_LOCAL_DB_CACHE_APPLY_FAILED");
      break;
    case PM_EVT_SERVICE_CHANGED_IND_SENT:
      EVT_DEBUG("Unhandled PM_EVT_SERVICE_CHANGED_IND_SENT");
      break;
    case PM_EVT_SERVICE_CHANGED_IND_CONFIRMED:
      EVT_DEBUG("Unhandled PM_EVT_SERVICE_CHANGED_IND_CONFIRMED");
      break;
    case PM_EVT_SLAVE_SECURITY_REQ:
      EVT_DEBUG("Unhandled PM_EVT_SLAVE_SECURITY_REQ");
      break;
    case PM_EVT_FLASH_GARBAGE_COLLECTED:
      EVT_DEBUG("Unhandled PM_EVT_FLASH_GARBAGE_COLLECTED");
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
  APP_ERROR_CHECK(ble_lesc_init());
  APP_ERROR_CHECK(ble_lesc_ecc_keypair_generate_and_set());
}
