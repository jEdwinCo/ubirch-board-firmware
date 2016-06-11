/*
 * ubirch#1 timer driver code.
 *
 * Based on https://github.com/mbedmicro/mbed/blob/master/libraries/mbed/targets/hal/TARGET_Freescale/TARGET_KSDK2_MCUS/TARGET_K64F/us_ticker.c
 *
 * @author Matthias L. Jugel
 * @date 2016-04-06
 *
 * Copyright 2016 ubirch GmbH (https://ubirch.com)
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
#include <fsl_pit.h>
#include "timer.h"

static bool initialized = false;

void PIT0_IRQHandler() {
  // clear interrupt flag
  PIT_ClearStatusFlags(PIT, kPIT_Chnl_3, kPIT_TimerFlag);
  PIT_DisableInterrupts(PIT, kPIT_Chnl_3, kPIT_TimerInterruptEnable);

  // stop the timer
  PIT_StopTimer(PIT, kPIT_Chnl_3);
  PIT_StopTimer(PIT, kPIT_Chnl_2);

  // create continue event
  __SEV();
}

void timer_init() {
  if (initialized) return;
  initialized = true;

  pit_config_t pitConfig;
  PIT_GetDefaultConfig(&pitConfig);
  PIT_Init(PIT, &pitConfig);

  PIT_SetTimerPeriod(PIT, kPIT_Chnl_0, (uint32_t) USEC_TO_COUNT(1U, CLOCK_GetFreq(kCLOCK_BusClk)) - 1);
  PIT_SetTimerPeriod(PIT, kPIT_Chnl_1, 0xFFFFFFFF);
  PIT_SetTimerChainMode(PIT, kPIT_Chnl_1, true);

  PIT_StartTimer(PIT, kPIT_Chnl_0);
  PIT_StartTimer(PIT, kPIT_Chnl_1);

  PIT_SetTimerPeriod(PIT, kPIT_Chnl_2, (uint32_t) USEC_TO_COUNT(1U, CLOCK_GetFreq(kCLOCK_BusClk)) - 1);
  PIT_SetTimerPeriod(PIT, kPIT_Chnl_3, 0xFFFFFFFF);
  PIT_SetTimerChainMode(PIT, kPIT_Chnl_3, true);
  EnableIRQ(PIT0_IRQn);
}

uint32_t timer_read() {
  if (!initialized) timer_init();
  return ~(PIT_GetCurrentTimerCount(PIT, kPIT_Chnl_1));
}

// TODO: handle longer than 71 minute interrupt times (chaining both timers)
void timer_set_interrupt(uint32_t us) {
  if (!initialized) timer_init();

  PIT_StopTimer(PIT, kPIT_Chnl_3);
  PIT_StopTimer(PIT, kPIT_Chnl_2);
  PIT_SetTimerPeriod(PIT, kPIT_Chnl_3, (uint32_t) us);
  PIT_EnableInterrupts(PIT, kPIT_Chnl_3, kPIT_TimerInterruptEnable);
  PIT_StartTimer(PIT, kPIT_Chnl_3);
  PIT_StartTimer(PIT, kPIT_Chnl_2);
}

extern void timer_timeout(uint32_t us);
extern uint32_t timer_timeout_remaining();

void delay(uint32_t ms) {
  if (ms > (UINT32_MAX - 1) / 1000) return;
  if (!initialized) timer_init();

  timer_timeout(ms * 1000);
  while (timer_timeout_remaining()) { __WFE(); }
}
