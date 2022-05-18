#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "tremo_flash.h"
#include "main.h"
#include "uart.h"
#include "flash.h"
#include "xmodem.h"
#include "stk500.h"
#include "frsky.h"
#include "utilities.h"
#include "tremo_rcc.h"
#include "tremo_uart.h"
#include "tremo_gpio.h"
#include "tremo_delay.h"

#if !XMODEM && !STK500 && !FRSKY
#error "Upload protocol not defined!"
#endif


#if XMODEM

static void print_boot_header(void)
{
  /* Send welcome message on startup. */
  uart_transmit_str("\n\r==========ASR6601");

  uart_transmit_str(" ==========\n\r");
  uart_transmit_str("  Bootloader for ExpressLRS\n\r");
  uart_transmit_str("=============================\n\r");
  uart_transmit_str("==========ASR6601CB==========\n\r");
}

static int8_t boot_code_xmodem(int32_t rx_pin, int32_t tx_pin)
{
  uint8_t BLrequested = 0;
  uint8_t header[6] = {0, 0, 0, 0, 0, 0};

  uart_init_boot(UART_BAUD, 0, 0, 0, UART_INV);
  flash_dump();

  print_boot_header();
  /* If the button is pressed, then jump to the user application,
   * otherwise stay in the bootloader. */
  uart_transmit_str("Send 'bbbb' or hold down button\n\r");

#if 0 // UART ECHO DEBUG
  uint8_t _led_tmp = 0;
  while(1) {
    if (uart_receive_timeout(header, 1u, 1000) == UART_OK) {
      uart_transmit_bytes(header, 1);
    } else {
      uart_transmit_ch('F');
    }
    led_state_set(_led_tmp ? LED_FLASHING : LED_FLASHING_ALT);
    _led_tmp ^= 1;
  }
#endif

  /* Wait input from UART */
  //uart_clear();
  if (uart_receive(header, 5u) == UART_OK) {
    /* Search for magic strings */
    BLrequested = (strnstr((char *)header, "bbbb", sizeof(header)) != NULL) ? 1 : 0;
  }

#if defined(PIN_BUTTON)
  // Wait button press to access bootloader
  if (!BLrequested && BTN_READ()) {
    HAL_Delay(200); // wait debounce
    if (BTN_READ()) {
      // Button still pressed
      BLrequested = 2;
    }
  }
#endif /* PIN_BUTTON */

  if (!BLrequested) {
    return -1;
  }

#if 0
  /* Wait command from uploader script if button was preassed */
  else if (BLrequested == 2) {
    BLrequested = 0;
    uint32_t ticks = HAL_GetTick();
    uint8_t ledState = 0;
    while (1) {
      if (BLrequested < 6) {
        if (1000 <= (HAL_GetTick() - ticks)) {
          led_state_set(ledState ? LED_FLASHING : LED_FLASHING_ALT);
          ledState ^= 1;
          ticks = HAL_GetTick();
        }

        if (uart_receive_timeout(header, 1, 1000U) != UART_OK) {
          continue;
        }
        uint8_t ch = header[0];

        switch (BLrequested) {
          case 0:
            if (ch == 0xEC) BLrequested++;
            else BLrequested = 0;
            break;
          case 1:
            if (ch == 0x04) BLrequested++;
            else BLrequested = 0;
            break;
          case 2:
            if (ch == 0x32) BLrequested++;
            else BLrequested = 0;
            break;
          case 3:
            if (ch == 0x62) BLrequested++;
            else BLrequested = 0;
            break;
          case 4:
            if (ch == 0x6c) BLrequested++;
            else BLrequested = 0;
            break;
          case 5:
            if (ch == 0x0A) BLrequested++;
            else BLrequested = 0;
            break;

        }
      } else {
        /* Boot cmd => wait 'bbbb' */
        print_boot_header();
        if (uart_receive_timeout(header, 5, 2000U) != UART_OK) {
          BLrequested = 0;
          continue;
        }
        if (strstr((char *)header, "bbbb")) {
          /* Script ready for upload... */
          break;
        }
      }
    }
  }
#endif
	
	  /* Infinite loop */
  while (1) {
    //uart_clear();
    xmodem_receive();
    /* We only exit the xmodem protocol, if there are any errors.
     * In that case, notify the user and start over. */
    //uart_transmit_str("\n\rFailed... Please try again.\n\r");
  }

  return 0;
}
#endif /* XMODEM */

#ifndef BOOT_WAIT
#define BOOT_WAIT 300 // ms
#endif

static uint32_t boot_start_time;

int8_t boot_wait_timer_end(void)
{
  return (BOOT_WAIT < (GetTickCount() - boot_start_time));
}

int8_t boot_code_stk(uint32_t baud, int32_t rx_pin, int32_t tx_pin, int32_t duplexpin, uint8_t inverted)
{
  int8_t ret = 0;

  uart_init_boot(baud, 0, 0, 0, inverted);
  uart_clear();

#if 0 // UART ECHO DEBUG
  uint8_t _led_tmp = 0;
  uint8_t header[6];
  while(1) {
    if (uart_receive_timeout(header, 1u, 1000) == UART_OK) {
      uart_transmit_bytes(header, 1);
    } else {
      //uart_transmit_ch('F');
      uart_transmit_str("F\r\n");
    }
    led_state_set(_led_tmp ? LED_FLASHING : LED_FLASHING_ALT);
    _led_tmp ^= 1;
  }
#endif

  boot_start_time = GetTickCount();

  while (0 <= ret) {
#if STK500
    ret = stk500_check();
#else
    ret = frsky_check();
#endif
  }
  uart_deinit_boot();

  return ret;
}


int main(void)
{
	int ret;
	  /* Make sure the vectors are set correctly */
	
  SCB->VTOR = BL_FLASH_START;
	
	system_init();
	
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_UART0, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOA, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOB, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOC, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOD, true);
	
	__enable_irq();
	
#if XMODEM
    ret = boot_code_xmodem(0, 0);

#else /* !XMODEM */

  /* Give a bit time for OTX to bootup */
  if ((BL_FLASH_START & 0xffff) == 0x0)
    delay_ms(500);

  ret = boot_code_stk(UART_BAUD, 0, 0, 0, UART_INV);
#endif/* XMODEM */

  if (ret < 0) {
    flash_jump_to_app();
  }

  // Reset system if something went wrong
  NVIC_SystemReset();

}

#ifdef USE_FULL_ASSERT
void assert_failed(void* file, uint32_t line)
{
    (void)file;
    (void)line;

    while (1) { }
}
#endif
