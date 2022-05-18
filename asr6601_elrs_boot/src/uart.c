/**
 * @file    uart.c
 * @author  Ferenc Nemeth
 * @date    21 Dec 2018
 * @brief   This module is a layer between the HAL UART functions and my Xmodem protocol.
 *
 *          Copyright (c) 2018 Ferenc Nemeth - https://github.com/ferenc-nemeth
 */

#include "uart.h"
#include "main.h"
#include "irq.h"
#include <string.h>
#include "utilities.h"
#include "tremo_rcc.h"
#include "tremo_uart.h"
#include "tremo_gpio.h"

/**
 * @brief   Receives data from UART.
 * @param   *data: Array to save the received data.
 * @param   length:  Size of the data.
 * @return  status: Report about the success of the receiving.
 */
uart_status uart_receive(uint8_t *data, uint16_t length)
{
  return uart_receive_timeout(data, length, UART_TIMEOUT);
}

uart_status uart_receive_timeout(uint8_t *data, uint16_t length, uint16_t timeout)
{
  uint32_t tickstart;
  tickstart = GetTickCount();
			while(length) {
        /* Check for the Timeout */
        if (timeout != HAL_MAX_DELAY) {
          if ((timeout == 0U) || ((GetTickCount() - tickstart) > timeout)) {
            return UART_ERROR;
          }
        }
				if (uart_get_flag_status(UART0, UART_FLAG_RX_FIFO_EMPTY) != SET) {
					*data++ = UART0->DR;
					length--;
				}
			}
  return UART_OK;
}

/**
 * @brief   Transmits a string to UART.
 * @param   *data: Array of the data.
 * @return  status: Report about the success of the transmission.
 */
uart_status uart_transmit_str(char *data)
{
  uint32_t length = 0u;
  /* Calculate the length. */
  while ('\0' != data[length]) {
    length++;
  }
  return uart_transmit_bytes((uint8_t *)data, length);
}

/**
 * @brief   Transmits a single char to UART.
 * @param   *data: The char.
 * @return  status: Report about the success of the transmission.
 */
uart_status uart_transmit_ch(uint8_t data)
{
  return uart_transmit_bytes(&data, 1u);
}

uart_status uart_transmit_bytes(uint8_t *data, uint32_t len)
{
  uart_status status = UART_OK;
    while (len--) {
      uart_send_data(UART0,*data++);
    }
    while (uart_get_flag_status(UART0, UART_FLAG_TX_FIFO_FULL) == SET);

  return status;
}


/**
 * @brief UART Initialization Function
 * @param None
 * @retval None
 */
void uart_init_boot(uint32_t baud, int32_t pin_rx, int32_t pin_tx,
               int32_t duplexpin, int8_t inverted)
{
	    // uart0
    gpio_set_iomux(GPIOB, GPIO_PIN_0, 1);
    gpio_set_iomux(GPIOB, GPIO_PIN_1, 1);

    /* uart config struct init */
    uart_config_t uart_config;
    uart_config_init(&uart_config);

    uart_config.baudrate = baud;
    uart_init(UART0, &uart_config);
    uart_cmd(UART0, ENABLE);
}

void uart_deinit_boot(void)
{
	while (uart_get_flag_status(UART0, UART_FLAG_TX_FIFO_EMPTY) != SET);
  uart_deinit(UART0);
}
