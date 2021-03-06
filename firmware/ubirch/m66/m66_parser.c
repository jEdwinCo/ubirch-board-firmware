/*
 * AT command parser.
 *
 * @author Matthias L. Jugel
 * @date 2016-04-09
 *
 * @copyright &copy; 2015 ubirch GmbH (https://ubirch.com)
 *
 * ```
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * ```
 */

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <fsl_common.h>
#include "m66_core.h"
#include "m66_parser.h"
#include "m66_debug.h"
#include <board.h>
#include <ubirch/timer.h>

int modem_check_urc(const char *line) {
  size_t len = strlen(line);
  for (int i = 0; M66_URC[i] != NULL; i++) {
    const char *urc = M66_URC[i];
    size_t urc_len = strlen(M66_URC[i]);
    if (len >= urc_len && !strncmp(urc, line, urc_len)) {
      CSTDEBUG("GSM INFO !! [%02d] %s\r\n", i, line);
      return i;
    }
  }
  return -1;
}

void modem_send(const char *pattern, ...) {
  char cmd[BOARD_CELL_BUFSIZE];

  // cleanup the input buffer and check for URC messages
  uint32_t remaining = timer_timeout_remaining();
  while (modem_readline(cmd, BOARD_CELL_BUFSIZE - 1, 100)) modem_check_urc(cmd);
  timer_set_timeout(remaining);

  cmd[0] = '\0';

  va_list ap;
  va_start(ap, pattern);
  vsnprintf(cmd, BOARD_CELL_BUFSIZE, pattern, ap);
  va_end(ap);

  CIODEBUG("GSM (%02d) <- '%s'\r\n", strlen(cmd), cmd);
  modem_writeline(cmd);
}

bool modem_expect_urc(int n, uint32_t timeout) {
  char response[128] = {0};
  bool urc_found = false;
  do {
    const size_t len = modem_readline(response, BOARD_CELL_BUFSIZE - 1, timeout);
    if (!len) break;
    int r = modem_check_urc(response);
    urc_found = r == n;
#ifndef NDEBUG
    if (r == 0 && !urc_found) {
      CIODEBUG("GSM .... ?? ");
      for (int i = 0; i < len; i++) CIODEBUG("%02x '%c' ", *(response + i), *(response + i));
      CIODEBUG("\r\n");
    }
#endif
  } while (!urc_found);
  return urc_found;
}

bool modem_expect(const char *expected, uint32_t timeout) {
  char response[255] = {0};
  size_t len, expected_len = strlen(expected);
  while (true) {
    len = modem_readline(response, BOARD_CELL_BUFSIZE - 1, timeout);
    if (len == 0) return false;
    if (modem_check_urc(response) >= 0) continue;
    CIODEBUG("GSM (%02d) -> '%s'\r\n", len, response);
    return strncmp(expected, (const char *) response, MIN(len, expected_len)) == 0;
  }
}

int modem_expect_scan(const char *pattern, uint32_t timeout, ...) {
  char response[BOARD_CELL_BUFSIZE];
  va_list ap;
  do {
    modem_readline(response, BOARD_CELL_BUFSIZE - 1, timeout);
  } while (modem_check_urc(response) != -1);
  CIODEBUG("GSM (%02d) -> '%s'\r\n", strlen(response), response);

  va_start(ap, timeout);
  int matched = vsscanf(response, pattern, ap);
  va_end(ap);

  return matched;
}

