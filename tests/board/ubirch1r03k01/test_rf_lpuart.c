#include <ubirch/rf-sub-ghz/rf_lpuart.h>

void test_uart(void) {
  rf_config_t *rf_config;
  rf_config = &rf_config_default;

  rf_init(rf_config);
  const char *hello = "HELLO WORLD\r\n";
  rf_send((const uint8_t *) hello, strlen(hello));
}
