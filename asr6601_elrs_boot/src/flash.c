/**
 * @file    flash.c
 * @author  Ferenc Nemeth
 * @date    21 Dec 2018
 * @brief   This module handles the memory related functions.
 *
 *          Copyright (c) 2018 Ferenc Nemeth - https://github.com/ferenc-nemeth
 */
#include <string.h>
#include "stdlib.h"
#include "flash.h"
#include "main.h"
#include "uart.h"
#include "utilities.h"
#include "tremo_regs.h"
#include "tremo_rcc.h"
#include "tremo_uart.h"
#include "tremo_gpio.h"
#include "tremo_delay.h"

#define CM4_IRQ_VECT_BASE           0xE000ED08
#define CM4_IRQ_EN                  0xE000E100
#define CM4_IRQ_CLR                 0xE000E180

#ifndef DUMPING
#define DUMPING 0
#endif

#define FLASH_OP_BEGIN()    __disable_irq()
#define FLASH_OP_END()      __enable_irq()

#ifndef FLASH_TYPEPROGRAM_HALFWORD
#define FLASH_TYPEPROGRAM_HALFWORD 0 // should fail
#endif

/* Function pointer for jumping to user application. */
typedef void (*fnc_ptr)(void);

flash_status flash_dump(void)
{
  return FLASH_OK;
}


/**
 * @brief   This function erases the memory.
 * @param   address: First address to be erased (the last is the end of the
 * flash).
 * @return  status: Report about the success of the erasing.
 */
flash_status flash_erase(uint32_t address)
{
	uint32_t size = (128*1024)-(32*1024);
	
	if (address < FLASH_APP_START_ADDRESS)
		return FLASH_ERROR;
	
    while(size){
        FLASH_OP_BEGIN();
        if(flash_erase_page(address)<0) {
            FLASH_OP_END();
            return FLASH_ERROR;
        }
        FLASH_OP_END();
        if(size >= FLASH_PAGE_SIZE){
            size -= FLASH_PAGE_SIZE;
            address += FLASH_PAGE_SIZE;
        }else{
            size = 0;
        }
    }
		return FLASH_OK;
}

/**
 * @brief   This function flashes the memory.
 * @param   address: First address to be written to.
 * @param   *data:   Array of the data that we want to write.
 * @param   *length: Size of the array.
 * @return  status: Report about the success of the writing.
 */

flash_status flash_write(uint32_t dst_addr, uint8_t *data, uint32_t size)
{
	int ret = 0;
		if (dst_addr < FLASH_APP_START_ADDRESS)
		return FLASH_ERROR;
		
    if(FLASH_LINE_SIZE == size){
        FLASH_OP_BEGIN();
        ret = flash_program_line(dst_addr, data);
        FLASH_OP_END();
    }else{
        FLASH_OP_BEGIN();
        ret = flash_program_bytes(dst_addr, data, size);
        FLASH_OP_END();
    }
		if (ret!=0)
			return FLASH_ERROR;
  return FLASH_OK;
}

/**
 * @brief   Actually jumps to the user application.
 * @param   void
 * @return  void
 */

void flash_jump_to_app(void)
{

  if (flash_check_app_loaded() < 0) {
    NVIC_SystemReset();
  }

  fnc_ptr jump_to_app;
  jump_to_app = (fnc_ptr)(*(volatile uint32_t *)(FLASH_APP_START_ADDRESS + 4u));
  delay_ms(1000);
  uart_deinit_boot();
  rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOA, false);
  rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOB, false);
  rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOC, false);
  rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOD, false);
	SysTick->CTRL  &= ~SysTick_CTRL_ENABLE_Msk; 

  TREMO_REG_WR(CM4_IRQ_CLR, 0xFFFFFFFF); // close interrupts
	TREMO_REG_WR(CM4_IRQ_VECT_BASE, FLASH_APP_START_ADDRESS);
  jump_to_app();

  while(1)
    ;
}

int8_t flash_check_app_loaded(void)
{
  /* Check if app is already loaded */
  uintptr_t app_stack = *(volatile uintptr_t *)(FLASH_APP_START_ADDRESS);
  uintptr_t app_reset = *(volatile uintptr_t *)(FLASH_APP_START_ADDRESS + 4u);
  uintptr_t app_nmi = *(volatile uintptr_t *)(FLASH_APP_START_ADDRESS + 8u);
  if (((uint16_t)(app_stack >> 20) == 0x200) &&
      ((uint16_t)((app_reset >> 16) & 0xFFFC) == 0x0800) &&
      ((uint16_t)((app_nmi >> 16) & 0xFFFC) == 0x0800)) {
    return 0;
  }
  return -1;
}
