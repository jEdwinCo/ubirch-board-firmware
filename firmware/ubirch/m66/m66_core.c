/*
 * ubirch#1 M66 cell core functionality.
 *
 * @author Matthias L. Jugel
 * @date 2016-04-09
 *
 * @copyright &copy; 2015, 2016 ubirch GmbH (https://ubirch.com)
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

#include <drivers/fsl_lpuart.h>
#include <ubirch/timer.h>
#include <ctype.h>
#include <fsl_adc16.h>
#include "m66_core.h"
#include "m66_parser.h"
#include "m66_debug.h"

#ifndef BOARD_CELL_UART_PORT
#  error "No PORT found for cell phone chip. please configure ports/pins/clocks!"
#endif

#define VBAT_SENSE_PORT PORTD
#define VBAT_SENSE_PIN 6
#define VBAT_SENSE_ALT kPORT_PinDisabledOrAnalog
#define VBAT_SENSE_ADC ADC0
#define VBAT_SENSE_ADC_GROUP 0
#define VBAT_SENSE_CHANNEL 7
#define VBAT_SENSE_CHANNEL_MUX kADC16_ChannelMuxB

#ifndef GSM_RINGBUFFER_SIZE
#  define GSM_RINGBUFFER_SIZE 64
#endif
static uint8_t gsmUartRingBuffer[GSM_RINGBUFFER_SIZE];
static volatile int gsmRxIndex, gsmRxHead;

void BOARD_CELL_UART_IRQ_HANDLER(void) {
  if ((kLPUART_RxDataRegFullFlag) & LPUART_GetStatusFlags(BOARD_CELL_UART)) {
    uint8_t data = LPUART_ReadByte(BOARD_CELL_UART);

    // it may be necessary to create a critical section here, but
    // right now it didn't hurt us to not disable interrupts

    // __disable_irq();
    /* If ring buffer is not full, add data to ring buffer. */
    if (((gsmRxIndex + 1) % GSM_RINGBUFFER_SIZE) != gsmRxHead) {
      gsmUartRingBuffer[gsmRxIndex++] = data;
      gsmRxIndex %= GSM_RINGBUFFER_SIZE;
    }
    // __enable_irq();
  }
}

void modem_init() {
  const gpio_pin_config_t OUTTRUE = {kGPIO_DigitalOutput, true};
  const gpio_pin_config_t IN = {kGPIO_DigitalInput, false};

  // initialize BOARD_CELL pins
  CLOCK_EnableClock(BOARD_CELL_UART_PORT_CLOCK);
  PORT_SetPinMux(BOARD_CELL_UART_PORT, BOARD_CELL_UART_TX_PIN, BOARD_CELL_UART_TX_ALT);
  PORT_SetPinMux(BOARD_CELL_UART_PORT, BOARD_CELL_UART_RX_PIN, BOARD_CELL_UART_RX_ALT);

  CLOCK_EnableClock(BOARD_CELL_PIN_PORT_CLOCK);
  PORT_SetPinMux(BOARD_CELL_PIN_PORT, BOARD_CELL_STATUS_PIN, kPORT_MuxAsGpio);
  GPIO_PinInit(BOARD_CELL_PIN_GPIO, BOARD_CELL_STATUS_PIN, &IN);

#if BOARD_CELL_RESET_PIN
  PORT_SetPinMux(BOARD_CELL_PIN_PORT, BOARD_CELL_RESET_PIN, kPORT_MuxAsGpio);
  GPIO_PinInit(BOARD_CELL_PIN_GPIO, BOARD_CELL_RESET_PIN, &OUTTRUE);
#endif

  PORT_SetPinMux(BOARD_CELL_PIN_PORT, BOARD_CELL_PWRKEY_PIN, kPORT_MuxAsGpio);
  GPIO_PinInit(BOARD_CELL_PIN_GPIO, BOARD_CELL_PWRKEY_PIN, &OUTTRUE);

  // the ring identifier is optional, only use if a pin and port exists
#if BOARD_CELL_RI_PIN
  PORT_SetPinMux(BOARD_CELL_PIN_PORT, BOARD_CELL_RI_PIN, kPORT_MuxAsGpio);
  GPIO_PinInit(BOARD_CELL_PIN_GPIO, BOARD_CELL_RI_PIN, &IN);
#endif

#if BOARD_CELL_PWR_DOMAIN
  const gpio_pin_config_t OUTFALSE = {kGPIO_DigitalOutput, false};

  CLOCK_EnableClock(BOARD_CELL_PWR_EN_CLOCK);
  PORT_SetPinMux(BOARD_CELL_PWR_EN_PORT, BOARD_CELL_PWR_EN_PIN, kPORT_MuxAsGpio);
  GPIO_PinInit(BOARD_CELL_PWR_EN_GPIO, BOARD_CELL_PWR_EN_PIN, &OUTFALSE);
#endif

  PORT_SetPinMux(VBAT_SENSE_PORT,   VBAT_SENSE_PIN,   VBAT_SENSE_ALT);
  adc16_config_t adc16ConfigStruct;
  ADC16_GetDefaultConfig(&adc16ConfigStruct);
  adc16ConfigStruct.longSampleMode = kADC16_LongSampleCycle16;
  ADC16_Init(VBAT_SENSE_ADC, &adc16ConfigStruct);
  ADC16_EnableHardwareTrigger(VBAT_SENSE_ADC, false); // no hardware trigger

  // configure uart driver connected to the M66
  lpuart_config_t lpuart_config;
  LPUART_GetDefaultConfig(&lpuart_config);
  lpuart_config.baudRate_Bps = BOARD_CELL_UART_BAUD;
  lpuart_config.parityMode = kLPUART_ParityDisabled;
  lpuart_config.stopBitCount = kLPUART_OneStopBit;

  // TODO refactor this to a global function
  // since the LPuart initialization depends very much on the source clock and its
  // frequency, we do a check here and retrieve the frequency accordingly
  // The CLOCK_SetLpuartSrc() is already done during clock init.
  uint32_t lpuart_src_freq;
  switch (SIM->SOPT2 & SIM_SOPT2_LPUARTSRC_MASK) {
    case SIM_SOPT2_LPUARTSRC(3U): {
      lpuart_src_freq = CLOCK_GetInternalRefClkFreq();
      break;
    }
    case SIM_SOPT2_LPUARTSRC(1U): {
      lpuart_src_freq = CLOCK_GetPllFllSelClkFreq();
      break;
    }
    default:
    case SIM_SOPT2_LPUARTSRC(2U): {
      lpuart_src_freq = CLOCK_GetOsc0ErClkFreq();
      break;
    }
  }

  LPUART_Init(BOARD_CELL_UART, &lpuart_config, lpuart_src_freq);
  LPUART_EnableRx(BOARD_CELL_UART, true);
  LPUART_EnableTx(BOARD_CELL_UART, true);

  LPUART_EnableInterrupts(BOARD_CELL_UART, kLPUART_RxDataRegFullInterruptEnable);
  EnableIRQ(BOARD_CELL_UART_IRQ);
}


static uint16_t vbat_sense() {
  ADC16_SetChannelMuxMode(VBAT_SENSE_ADC, VBAT_SENSE_CHANNEL_MUX);
  adc16_channel_config_t adc16ChannelConfigStruct;
  adc16ChannelConfigStruct.channelNumber = VBAT_SENSE_CHANNEL;
  adc16ChannelConfigStruct.enableInterruptOnConversionCompleted = false;
  adc16ChannelConfigStruct.enableDifferentialConversion = false;
  ADC16_SetChannelConfig(VBAT_SENSE_ADC, VBAT_SENSE_ADC_GROUP, &adc16ChannelConfigStruct);
  while (true) {
    uint32_t status = ADC16_GetChannelStatusFlags(VBAT_SENSE_ADC, VBAT_SENSE_ADC_GROUP);
    if (status & kADC16_ChannelConversionDoneFlag) {
      break;
    }
  }
  uint16_t val = (uint16_t) ADC16_GetChannelConversionValue(VBAT_SENSE_ADC, VBAT_SENSE_ADC_GROUP);
  return val;
}

bool modem_enable() {
  char response[10];
  size_t len;

#if BOARD_CELL_PWR_DOMAIN
  CSTDEBUG("GSM #### -- power on\r\n");
  GPIO_WritePinOutput(BOARD_CELL_PWR_EN_GPIO, BOARD_CELL_PWR_EN_PIN, true);
  // TODO check that power has come up correctly

//  timer_set_interrupt(20000 * 1000);
//  uint16_t power;
//  do {
//    power = vbat_sense();
//    // CSTDEBUG("GSM #### -- %d\r\n", power);
//  } while(power < 2800 && timer_timeout_remaining());
//  if(vbat_sense() < 2800) return false;
#endif

  // after enabling power, power on the M66
  while (modem_read() != -1) /* clear buffer */;

  // we need to identify if the chip is already on by sending AT commands
  // send AT and just ignore the echo and OK to get into a stable state
  // sometimes there is initial noise on the serial line
  modem_send("AT");
  len = modem_readline(response, 9, 500);
  CIODEBUG("GSM (%02d) -> '%s'\r\n", len, response);
  len = modem_readline(response, 9, 500);
  CIODEBUG("GSM (%02d) -> '%s'\r\n", len, response);

  // now identify if the chip is actually on, by issue AT and expecting something
  // if we can't read a response, either AT or OK, we need to run the power on sequence
  modem_send("AT");
  len = modem_readline(response, 9, 1000);
  CIODEBUG("GSM (%02d) -> '%s'\r\n", len, response);

  if (!len) {
    CSTDEBUG("GSM #### !! trigger PWRKEY\r\n");

    // power on the cell phone chip
    GPIO_WritePinOutput(BOARD_CELL_PIN_GPIO, BOARD_CELL_PWRKEY_PIN, false);
    delay(110); // >100ms
    GPIO_WritePinOutput(BOARD_CELL_PIN_GPIO, BOARD_CELL_PWRKEY_PIN, true);
    delay(1500); // >1s
    GPIO_WritePinOutput(BOARD_CELL_PIN_GPIO, BOARD_CELL_PWRKEY_PIN, false);
  } else {
    CSTDEBUG("GSM #### !! already on\r\n");
  }

  bool is_on = false;
  // wait for the chip to boot and react to commands
  for (int i = 0; i < 5; i++) {
    modem_send("ATE0");
    // if we still have echo on, this fails and falls through to the next OK
    if ((is_on = modem_expect_OK(1000))) break;
    if ((is_on = modem_expect_OK(1000))) break;
  }

  return is_on;
}

void modem_disable() {
  // try to power down the M66, then switch off power domain
  modem_send("AT+QPOWD=1");
  modem_expect_urc(NormalPowerDown, 12000);

#if ((defined BOARD_CELL_PWR_EN_GPIO) && (defined BOARD_CELL_PWR_EN_PIN))
  CSTDEBUG("GSM #### -- power off (%d)\r\n", vbat_sense());
  GPIO_WritePinOutput(BOARD_CELL_PWR_EN_GPIO, BOARD_CELL_PWR_EN_PIN, false);
#endif
}

int modem_read() {
  if ((gsmRxHead % GSM_RINGBUFFER_SIZE) == gsmRxIndex) return -1;
  int c = gsmUartRingBuffer[gsmRxHead++];
  gsmRxHead %= GSM_RINGBUFFER_SIZE;
  return c;
}

size_t modem_read_binary(uint8_t *buffer, size_t max, uint32_t timeout) {
  timer_set_timeout(timeout * 1000);
  size_t idx = 0;
  while (idx < max) {
    if (!timer_timeout_remaining()) break;
    int c = modem_read();
    if (c == -1) {
      // nothing in the buffer, allow some sleep
      __WFI();
      continue;
    }
    if (max - idx) buffer[idx++] = (uint8_t) c;
  }

  return idx;
}

size_t modem_readline(char *buffer, size_t max, uint32_t timeout) {
  timer_set_timeout(timeout * 1000);
  size_t idx = 0;
  while (idx < max) {
    if (!timer_timeout_remaining()) break;

    int c = modem_read();
    if (c == -1) {
      // nothing in the buffer, allow some sleep
      __WFI();
      continue;
    }

    if (c == '\r') continue;
    if (c == '\n') {
      if (!idx) {
        idx = 0;
        continue;
      }
      break;
    }
    if (max - idx && isprint(c)) buffer[idx++] = (char) c;
  }

  buffer[idx] = 0;
  return idx;
}

void modem_write(const uint8_t *buffer, size_t size) {
  LPUART_WriteBlocking(BOARD_CELL_UART, buffer, size);
}

void modem_writeline(const char *buffer) {
  modem_write((const uint8_t *) buffer, strlen(buffer));
  modem_write((const uint8_t *) "\r\n", 2);
}

