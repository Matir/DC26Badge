#include <stdint.h>
#include "app_error.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_strerror.h"

void unused_app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info) {
#ifndef DEBUG
  NVIC_SystemReset();
#else
  /*error_info_t *err = (error_info_t *)info;
  NRF_LOG_INIT(NULL);
  NRF_LOG_ERROR("Error in application: %d (%s) at %s:%d (0x%08x)",
    err->err_code,
    nrf_strerror_get(err->err_code),
    err->p_file_name,
    err->line_num,
    pc);
  NRF_LOG_FLUSH();*/
  app_error_save_and_stop(id, pc, info);
#endif // DEBUG
}
