/*!
 * @file
 * @brief real time clock driver code.
 *
 * Driver for the real time clock.
 *
 * @author Matthias L. Jugel
 * @date 2016-04-06
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

#ifndef _UBIRCH_RTC_H_
#define _UBIRCH_RTC_H_

#include <fsl_rtc.h>

#ifdef __cplusplus
extern "C" {
#endif

//! interrupt handler for the real time clock
typedef void (*rtc_datetime_handler_t)(rtc_datetime_t *);

/*!
 * @brief Initialize Real Time Clock.
 */
void rtc_init();

/*! @brief Set date and time. */
void rtc_set(const rtc_datetime_t *datetime);

/*! @brief Get date and time */
void rtc_get(rtc_datetime_t *datetime);


/*! @brief Set alarm to a specific date and time. */
void rtc_set_alarm(const rtc_datetime_t *datetime);

/*! @brief Get alarm date and time */
void rtc_get_alarm(rtc_datetime_t *datetime);

/*! @brief Set alarm in n seconds. */
void rtc_set_alarm_in(const uint32_t seconds);

/*! @brief Attach a handler to the alarm. */
void rtc_attach(rtc_datetime_handler_t handler);

/*! @brief Detach alarm handler. */
static inline void rtc_detach() { rtc_attach(NULL); }

#ifdef __cplusplus
}
#endif

#endif // _UBIRCH_RTC_H_
