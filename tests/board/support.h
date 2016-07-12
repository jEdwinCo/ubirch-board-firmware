/*!
 * @file
 * @brief Board test support.
 *
 * Support for testing the board
 *
 * @author Matthias L. Jugel
 * @date   2016-07-12
 *
 * @copyright &copy; 2016 ubirch GmbH (https://ubirch.com)
 *
 * @section LICENSE
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
#ifndef UBIRCH_BOARD_TEST_SUPPORT_H
#define UBIRCH_BOARD_TEST_SUPPORT_H

#include <stdbool.h>

typedef void (*systick_callback_t)(bool on);

bool yesno(char *prompt);
void enter(char *prefix, ...);
void ok(char *prefix, bool r);

void test_audio(void);
void test_rgb(void);
void test_quectel();
void test_gpio();

#endif //UBIRCH_BOARD_TEST_SUPPORT_H
