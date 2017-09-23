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
/** ============================================================================
 *  @file       EK_TM4C129EXL.h
 *
 *  @brief      EK_TM4C129EXL Board Specific APIs
 *
 *  The EK_TM4C129EXL header file should be included in an application as
 *  follows:
 *  @code
 *  #include <EK_TM4C129EXL.h>
 *  @endcode
 *
 *  ============================================================================
 */

#ifndef __EK_TM4C129EXL_H
#define __EK_TM4C129EXL_H

#ifdef __cplusplus
extern "C" {
#endif

/* LEDs on EK_TM4C129EXL are active high. */
#define EK_TM4C129EXL_LED_OFF (0)
#define EK_TM4C129EXL_LED_ON  (1)

/*!
 *  @def    EK_TM4C129EXL_EMACName
 *  @brief  Enum of EMAC names on the EK_TM4C129EXL dev board
 */
typedef enum EK_TM4C129EXL_EMACName {
    EK_TM4C129EXL_EMAC0 = 0,

    EK_TM4C129EXL_EMACCOUNT
} EK_TM4C129EXL_EMACName;

/*!
 *  @def    EK_TM4C129EXL_GPIOName
 *  @brief  Enum of LED names on the EK_TM4C129EXL dev board
 */
typedef enum EK_TM4C129EXL_GPIOName {
    EK_TM4C129EXL_USR_SW1 = 0,
    EK_TM4C129EXL_USR_SW2,
    EK_TM4C129EXL_D1,
    EK_TM4C129EXL_D2,

    EK_TM4C129EXL_GPIOCOUNT
} EK_TM4C129EXL_GPIOName;

/*!
 *  @def    EK_TM4C129EXL_I2CName
 *  @brief  Enum of I2C names on the EK_TM4C129EXL dev board
 */
typedef enum EK_TM4C129EXL_I2CName {
    EK_TM4C129EXL_I2C7 = 0,
    EK_TM4C129EXL_I2C8,

    EK_TM4C129EXL_I2CCOUNT
} EK_TM4C129EXL_I2CName;

/*!
 *  @def    EK_TM4C129EXL_PWMName
 *  @brief  Enum of PWM names on the EK_TM4C129EXL dev board
 */
typedef enum EK_TM4C129EXL_PWMName {
    EK_TM4C129EXL_PWM0 = 0,

    EK_TM4C129EXL_PWMCOUNT
} EK_TM4C129EXL_PWMName;

/*!
 *  @def    EK_TM4C129EXL_SDSPIName
 *  @brief  Enum of SDSPI names on the EK_TM4C129EXL dev board
 */
typedef enum EK_TM4C129EXL_SDSPIName {
    EK_TM4C129EXL_SDSPI0 = 0,
    EK_TM4C129EXL_SDSPI1,

    EK_TM4C129EXL_SDSPICOUNT
} EK_TM4C129EXL_SDSPIName;

/*!
 *  @def    EK_TM4C129EXL_SPIName
 *  @brief  Enum of SPI names on the EK_TM4C129EXL dev board
 */
typedef enum EK_TM4C129EXL_SPIName {
    EK_TM4C129EXL_SPI2 = 0,
    EK_TM4C129EXL_SPI3,

    EK_TM4C129EXL_SPICOUNT
} EK_TM4C129EXL_SPIName;

/*!
 *  @def    EK_TM4C129EXL_UARTName
 *  @brief  Enum of UARTs on the EK_TM4C129EXL dev board
 */
typedef enum EK_TM4C129EXL_UARTName {
    EK_TM4C129EXL_UART0 = 0,

    EK_TM4C129EXL_UARTCOUNT
} EK_TM4C129EXL_UARTName;

/*!
 *  @def    EK_TM4C129EXL_USBMode
 *  @brief  Enum of USB setup function on the EK_TM4C129EXL dev board
 */
typedef enum EK_TM4C129EXL_USBMode {
    EK_TM4C129EXL_USBDEVICE,
    EK_TM4C129EXL_USBHOST
} EK_TM4C129EXL_USBMode;

/*!
 *  @def    EK_TM4C129EXL_USBMSCHFatFsName
 *  @brief  Enum of USBMSCHFatFs names on the EK_TM4C129EXL dev board
 */
typedef enum EK_TM4C129EXL_USBMSCHFatFsName {
    EK_TM4C129EXL_USBMSCHFatFs0 = 0,

    EK_TM4C129EXL_USBMSCHFatFsCOUNT
} EK_TM4C129EXL_USBMSCHFatFsName;

/*
 *  @def    EK_TM4C129EXL_WatchdogName
 *  @brief  Enum of Watchdogs on the EK_TM4C129EXL dev board
 */
typedef enum EK_TM4C129EXL_WatchdogName {
    EK_TM4C129EXL_WATCHDOG0 = 0,

    EK_TM4C129EXL_WATCHDOGCOUNT
} EK_TM4C129EXL_WatchdogName;

/*!
 *  @def    EK_TM4C129EXL_WiFiName
 *  @brief  Enum of WiFi names on the EK_TM4C129EXL dev board
 */
typedef enum EK_TM4C129EXL_WiFiName {
    EK_TM4C129EXL_WIFI = 0,

    EK_TM4C129EXL_WIFICOUNT
} EK_TM4C129EXL_WiFiName;

/*!
 *  @brief  Initialize the general board specific settings
 *
 *  This function initializes the general board specific settings.
 *  This includes:
 *     - Enable clock sources for peripherals
 */
extern void EK_TM4C129EXL_initGeneral(void);

/*!
 *  @brief Initialize board specific EMAC settings
 *
 *  This function initializes the board specific EMAC settings and
 *  then calls the EMAC_init API to initialize the EMAC module.
 *
 *  The EMAC address is programmed as part of this call.
 *
 */
extern void EK_TM4C129EXL_initEMAC(void);

/*!
 *  @brief  Initialize board specific GPIO settings
 *
 *  This function initializes the board specific GPIO settings and
 *  then calls the GPIO_init API to initialize the GPIO module.
 *
 *  The GPIOs controlled by the GPIO module are determined by the GPIO_PinConfig
 *  variable.
 */
extern void EK_TM4C129EXL_initGPIO(void);

/*!
 *  @brief  Initialize board specific I2C settings
 *
 *  This function initializes the board specific I2C settings and then calls
 *  the I2C_init API to initialize the I2C module.
 *
 *  The I2C peripherals controlled by the I2C module are determined by the
 *  I2C_config variable.
 */
extern void EK_TM4C129EXL_initI2C(void);

/*!
 *  @brief  Initialize board specific PWM settings
 *
 *  This function initializes the board specific PWM settings and then calls
 *  the PWM_init API to initialize the PWM module.
 *
 *  The PWM peripherals controlled by the PWM module are determined by the
 *  PWM_config variable.
 */
extern void EK_TM4C129EXL_initPWM(void);

/*!
 *  @brief  Initialize board specific SDSPI settings
 *
 *  This function initializes the board specific SDSPI settings and then calls
 *  the SDSPI_init API to initialize the SDSPI module.
 *
 *  The SDSPI peripherals controlled by the SDSPI module are determined by the
 *  SDSPI_config variable.
 */
extern void EK_TM4C129EXL_initSDSPI(void);

/*!
 *  @brief  Initialize board specific SPI settings
 *
 *  This function initializes the board specific SPI settings and then calls
 *  the SPI_init API to initialize the SPI module.
 *
 *  The SPI peripherals controlled by the SPI module are determined by the
 *  SPI_config variable.
 */
extern void EK_TM4C129EXL_initSPI(void);

/*!
 *  @brief  Initialize board specific UART settings
 *
 *  This function initializes the board specific UART settings and then calls
 *  the UART_init API to initialize the UART module.
 *
 *  The UART peripherals controlled by the UART module are determined by the
 *  UART_config variable.
 */
extern void EK_TM4C129EXL_initUART(void);

/*!
 *  @brief  Initialize board specific USB settings
 *
 *  This function initializes the board specific USB settings and pins based on
 *  the USB mode of operation.
 *
 *  @param      usbMode    USB mode of operation
 */
extern void EK_TM4C129EXL_initUSB(EK_TM4C129EXL_USBMode usbMode);

/*!
 *  @brief  Initialize board specific USBMSCHFatFs settings
 *
 *  This function initializes the board specific USBMSCHFatFs settings and then
 *  calls the USBMSCHFatFs_init API to initialize the USBMSCHFatFs module.
 *
 *  The USBMSCHFatFs peripherals controlled by the USBMSCHFatFs module are
 *  determined by the USBMSCHFatFs_config variable.
 */
extern void EK_TM4C129EXL_initUSBMSCHFatFs(void);

/*!
 *  @brief  Initialize board specific Watchdog settings
 *
 *  This function initializes the board specific Watchdog settings and then
 *  calls the Watchdog_init API to initialize the Watchdog module.
 *
 *  The Watchdog peripherals controlled by the Watchdog module are determined
 *  by the Watchdog_config variable.
 */
extern void EK_TM4C129EXL_initWatchdog(void);

/*!
 *  @brief  Initialize board specific WiFi settings
 *
 *  This function initializes the board specific WiFi settings and then calls
 *  the WiFi_init API to initialize the WiFi module.
 *
 *  The hardware resources controlled by the WiFi module are determined by the
 *  WiFi_config variable.
 *
 *  A SimpleLink CC3100 device or module is required and must be connected to
 *  use the WiFi driver.
 */
extern void EK_TM4C129EXL_initWiFi(void);

#ifdef __cplusplus
}
#endif

#endif /* __EK_TM4C129EXL_H */
