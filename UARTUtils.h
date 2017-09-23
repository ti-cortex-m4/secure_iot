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
 *  @file       UARTUtils.h
 *
 *  @brief      UART related utilities
 *
 *  The UARTUtils header file should be included in an application as follows:
 *  @code
 *  #include <UARTUtils.h>
 *  @endcode
 *
 *  ============================================================================
 */

#ifndef __UARTUtils_H
#define __UARTUtils_H

#ifdef __cplusplus
extern "C" {
#endif

#include <xdc/std.h>

#include <stdio.h>

#if defined(__TI_COMPILER_VERSION__)
#include <file.h>
#endif

/*!
 *  @brief  Open the UART to be used with LoggerIdle
 *
 *  This function opens the UART to be used by the LoggerIdle plugged function.
 *
 *  @param      index UART to use for LoggerIdle's send function from the ports
 *                    array. Matches the same number uart base (0 -> UART_BASE0).
 */
extern void UARTUtils_loggerIdleInit(unsigned int index);

/*!
 *  @brief  Sends Log data out the UART
 *
 *  This function should not be called directly, it will be called by the
 *  LoggerIdle module during Idle.
 *
 *  @param    a    Pointer to unsigned char buffer
 *  @param    len  length of unsigned char buffer
 *  @return        Number of bytes sent
 *
 *  To configure LoggerIdle to upload via UART in the .cfg:
 *  @code
 *  var LoggingSetup = xdc.useModule('ti.uia.sysbios.LoggingSetup');
 *  LoggingSetup.loggerType = LoggingSetup.UploadMode_IDLE;
 *  var LoggerIdle = xdc.useModule('ti.uia.sysbios.LoggerIdle');
 *  LoggerIdle.transportFxn = "&UARTUtils_loggerIdleSend";
 *  LoggerIdle.transportType = LoggerIdle.TransportType_UART;
 *  @endcode
 *
 */
extern Int UARTUtils_loggerIdleSend(UChar *a, Int len);

/*!
 *  @brief   close function for add_device rts function
 *
 *  This function should not be called directly
 */
extern int UARTUtils_deviceclose(int fd);

/*!
 *  @brief   seek function for add_device rts function
 *
 *  This function should not be called directly
 */
extern off_t UARTUtils_devicelseek(int fd, off_t offset, int origin);

/*!
 *  @brief   open function for add_device rts function
 *
 *  This function should not be called directly
 */
extern int UARTUtils_deviceopen(const char *path, unsigned flags, int mode);

/*!
 *  @brief   read function for add_device rts function
 *
 *  This function should not be called directly
 */
extern int UARTUtils_deviceread(int fd, char *buffer, unsigned size);

/*!
 *  @brief   rename function for add_device rts function
 *
 *  This function should not be called directly
 */
extern int UARTUtils_devicerename(const char *old_name, const char *new_name);

/*!
 *  @brief   unlink function for add_device rts function
 *
 *  This function should not be called directly
 */
extern int UARTUtils_deviceunlink(const char *path);

/*!
 *  @brief   write function for add_device rts function
 *
 *  This function should not be called directly
 */
extern int UARTUtils_devicewrite(int fd, const char *buffer, unsigned size);

/*!
 *  @brief   abort function called within System_abort
 *
 *  This function should not be called directly
 */
extern Void UARTUtils_systemAbort(String str);

/*!
 *  @brief  Opens the UART to be used with SysCallback
 *
 *  This function opens the UART to be used by the SysCallback plugged functions.
 *
 *  @param      index UART to use for System output from ports array. Matches the
 *                    same number uart base (0 -> UART_BASE0).
 */
extern void UARTUtils_systemInit(unsigned int index);

/*!
 *  @brief   putch function for System_printf
 *
 *  This function should not be called directly
 */
extern Void UARTUtils_systemPutch(Char a);

/*!
 *  @brief   ready function for System module
 *
 *  This function should not be called directly
 */
extern Bool UARTUtils_systemReady();

#ifdef __cplusplus
}
#endif

#endif /* __UARTUtils_H */
