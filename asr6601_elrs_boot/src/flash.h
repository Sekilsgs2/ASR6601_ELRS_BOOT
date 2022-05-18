/**
 * @file    flash.h
 * @author  Ferenc Nemeth
 * @date    21 Dec 2018
 * @brief   This module handles the memory related functions.
 *
 *          Copyright (c) 2018 Ferenc Nemeth - https://github.com/ferenc-nemeth
 */

#ifndef FLASH_H_
#define FLASH_H_

#ifdef STM32L0xx
#include "stm32l0xx_hal.h"
#elif defined(STM32L1xx)
#include "stm32l1xx_hal.h"
#elif defined(STM32L4xx)
#include "stm32l4xx_hal.h"
#elif defined(STM32F1)
#include "stm32f1xx_hal.h"
#elif defined(STM32F3xx)
#include "stm32f3xx_hal.h"
#else
#include "tremo_flash.h"
#endif
#include <stdint.h>

/* Extern vertor start address */
extern uint32_t g_pfnVectors;
#define BL_FLASH_START ((__IO uint32_t)&g_pfnVectors)

/* Start and end addresses of the user application. */
#ifndef FLASH_BASE
#define FLASH_BASE ((uint32_t)0x08000000u)
#endif
#define FLASH_APP_OFFSET 0x8000
#define FLASH_APP_START_ADDRESS (FLASH_BASE + FLASH_APP_OFFSET)
#define FLASH_APP_END_ADDRESS ((uint32_t)0x08000000u+128*1024)


/* Status report for the functions. */
#define FLASH_OK 0x00u             /**< The action was successful. */
#ifndef HAL_FLASH_ERROR_SIZE
#define HAL_FLASH_ERROR_SIZE 0x01
#endif
#ifndef FLASH_ERROR_SIZE
#define FLASH_ERROR_SIZE 0x01u     /**< The binary is too big. */
#endif
#define FLASH_ERROR_WRITE 0x02u    /**< Writing failed. */
#define FLASH_ERROR_READBACK 0x04u /**< Writing was successful, but the content of the memory is wrong. */
#define FLASH_ERROR 0xFFu           /**< Generic error. */

typedef uint8_t flash_status;

flash_status flash_dump(void);
flash_status flash_erase(uint32_t address);
flash_status flash_erase_page_boot(uint32_t address);
flash_status flash_write(uint32_t dst_addr, uint8_t *data, uint32_t size);
flash_status flash_write_halfword(uint32_t address, uint16_t *data,
                                  uint32_t length);
void flash_jump_to_app(void);
int8_t flash_check_app_loaded(void);

#endif /* FLASH_H_ */
