/*
 * Copyright (c) 2015 Texas Instruments Incorporated.  All rights reserved.
 * Software License Agreement
 *
 * Texas Instruments (TI) is supplying this software for use solely and
 * exclusively on TI's microcontroller products. The software is owned by
 * TI and/or its suppliers, and is protected under applicable copyright
 * laws. You may not combine this software with "viral" open-source
 * software in order to form a larger program.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
 * NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
 * NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
 * CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
 * DAMAGES, FOR ANY REASON WHATSOEVER.
 */

#ifndef __BOARD_H
#define __BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "EK_TM4C129EXL.h"

#define Board_initEMAC              EK_TM4C129EXL_initEMAC
#define Board_initGeneral           EK_TM4C129EXL_initGeneral
#define Board_initGPIO              EK_TM4C129EXL_initGPIO
#define Board_initI2C               EK_TM4C129EXL_initI2C
#define Board_initPWM               EK_TM4C129EXL_initPWM
#define Board_initSDSPI             EK_TM4C129EXL_initSDSPI
#define Board_initSPI               EK_TM4C129EXL_initSPI
#define Board_initUART              EK_TM4C129EXL_initUART
#define Board_initUSB               EK_TM4C129EXL_initUSB
#define Board_initUSBMSCHFatFs      EK_TM4C129EXL_initUSBMSCHFatFs
#define Board_initWatchdog          EK_TM4C129EXL_initWatchdog
#define Board_initWiFi              EK_TM4C129EXL_initWiFi

#define Board_LED_ON                EK_TM4C129EXL_LED_ON
#define Board_LED_OFF               EK_TM4C129EXL_LED_OFF
#define Board_LED0                  EK_TM4C129EXL_D1
#define Board_LED1                  EK_TM4C129EXL_D2
#define Board_LED2                  EK_TM4C129EXL_D2
#define Board_BUTTON0               EK_TM4C129EXL_USR_SW1
#define Board_BUTTON1               EK_TM4C129EXL_USR_SW2

#define Board_I2C0                  EK_TM4C129EXL_I2C7
#define Board_I2C1                  EK_TM4C129EXL_I2C8
#define Board_I2C_TMP               EK_TM4C129EXL_I2C7
#define Board_I2C_NFC               EK_TM4C129EXL_I2C7
#define Board_I2C_TPL0401           EK_TM4C129EXL_I2C7

#define Board_PWM0                  EK_TM4C129EXL_PWM0
#define Board_PWM1                  EK_TM4C129EXL_PWM0

#define Board_SDSPI0                EK_TM4C129EXL_SDSPI0
#define Board_SDSPI1                EK_TM4C129EXL_SDSPI1

#define Board_SPI0                  EK_TM4C129EXL_SPI2
#define Board_SPI1                  EK_TM4C129EXL_SPI3
#define Board_SPI_CC3100            EK_TM4C129EXL_SPI2

#define Board_USBMSCHFatFs0         EK_TM4C129EXL_USBMSCHFatFs0

#define Board_USBHOST               EK_TM4C129EXL_USBHOST
#define Board_USBDEVICE             EK_TM4C129EXL_USBDEVICE

#define Board_UART0                 EK_TM4C129EXL_UART0

#define Board_WATCHDOG0             EK_TM4C129EXL_WATCHDOG0

#define Board_WIFI                  EK_TM4C129EXL_WIFI

/* Board specific I2C addresses */
#define Board_TMP006_ADDR           (0x40)
#define Board_RF430CL330_ADDR       (0x28)
#define Board_TPL0401_ADDR          (0x40)

#ifdef __cplusplus
}
#endif

#endif /* __BOARD_H */
