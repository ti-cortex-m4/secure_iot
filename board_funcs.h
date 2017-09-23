//*****************************************************************************
//
// board_funcs.h - Functions to configure and manage different peripherals of
// the board.  These functions could be used by multiple tasks.
//
// Copyright (c) 2015 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
//
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
//
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
//
//*****************************************************************************

#ifndef __BOARD_FUNC_H__
#define __BOARD_FUNC_H__

//*****************************************************************************
//
// Label that defines the EEPROM offset where CIK is sotred.
//
//*****************************************************************************
#define EXOSITE_CIK_OFFSET      0

//*****************************************************************************
//
// Prototypes of the functions that are called from outside the board_funcs.c
// module.
//
//*****************************************************************************
extern void ConfigureADC0(void);
extern void ConfigureButtons(void);
extern void ReadButtons(uint32_t *buttons);
extern uint16_t ReadInternalTemp(void);
extern bool GetMacAddress(char *pcMACAddress, uint32_t ui32MACLen);
extern void InitEEPROM(void);
extern bool GetCIKEEPROM(char *pcProvBuf);
extern bool SaveCIKEEPROM(char *pcProvBuf);

#endif // __BOARD_FUNC_H__
