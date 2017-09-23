//*****************************************************************************
//
// ntp_time.h - Utilities to sync with NTP time server.
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

#ifndef __NTP_TIME_H__
#define __NTP_TIME_H__

//*****************************************************************************
//
// Cloud connection states.
//
//*****************************************************************************
enum
{
    NTP_Init,
    NTP_Resolve_URL,
    NTP_Prompt_User,
    NTP_Connect,
    NTP_Idle
};

//*****************************************************************************
//
// Prototypes of the functions that are called from outside the ntp_time.c
// module.
//
//*****************************************************************************
extern int32_t SyncNTPServer(void);

#endif // __NTP_TIME_H__
