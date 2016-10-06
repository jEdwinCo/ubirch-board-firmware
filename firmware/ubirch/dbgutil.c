/*
 * SIM800H debug helpers.
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

#include <stdint.h>
#include <stddef.h>
#include <fsl_debug_console.h>

void dbg_dump(const char *prefix, const uint8_t *b, size_t size) {
  for (int i = 0; i < size; i += 16) {
    if (prefix && strlen(prefix) > 0) PRINTF("%s %06x: ", prefix, i);
    for (int j = 0; j < 16; j++) {
      if ((i + j) < size) PRINTF("%02x", b[i + j]); else PRINTF("  ");
      if ((j+1) % 2 == 0) PUTCHAR(' ');
    }
    PUTCHAR(' ');
    for (int j = 0; j < 16 && (i + j) < size; j++) {
      PUTCHAR(b[i + j] >= 0x20 && b[i + j] <= 0x7E ? b[i + j] : '.');
    }
    PRINTF("\r\n");
  }
}

void dbg_xxd(const char *prefix, const uint8_t *b, size_t size) {
  if (prefix && strlen(prefix) > 0) PRINTF("%s ", prefix);
  for (int i = 0; i < size; i++) PRINTF("%02x", b[i]);
  PRINTF("\r\n");
}
