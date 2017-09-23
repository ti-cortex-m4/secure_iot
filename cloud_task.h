//*****************************************************************************
//
// cloud_task.h - Task to connect and communicate to the cloud server. This
// task also manages board level user switch and LED function for this app.
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

#ifndef __CLOUD_TASK_H__
#define __CLOUD_TASK_H__

//*****************************************************************************
//
// ToDo USER STEP:
// To configure proxy network during compile time, uncomment the label
// "SET_PROXY" and "PROXY_ADDR".  Define the IP address and port number of the
// desired proxy server by using the label "PROXY_ADDR" in the format
// "<IP Address>:<Port No.>" as shown below.
//
//*****************************************************************************
//#define SET_PROXY
//#define PROXY_ADDR              "192.168.1.80:80"

//*****************************************************************************
//
// ToDo USER STEP:
// Define the URL of the desired NTP server using the label "NTP_SERVER_IP".
//
//*****************************************************************************
#define NTP_SERVER_URL          "time.nist.gov"
#define NTP_SERVER_PORT         123

//*****************************************************************************
//
// Exosite Server IP address and Port number.
//
//*****************************************************************************
#define EXOSITE_ADDR            "m2.exosite.com:443"

//*****************************************************************************
//
// Labels that define size of the MAC Address.
//
//*****************************************************************************
#define MAC_ADDRESS_LENGTH      12

//*****************************************************************************
//
// Labels that define size of the buffers that hold the provision request and
// the CIK.
//
//*****************************************************************************
#define EXOSITE_LENGTH          65
#define EXOSITE_CIK_LENGTH      40

//*****************************************************************************
//
// Labels that define the number of Alias that will be received from the server
// and size of the value returned.
//
//*****************************************************************************
#define ALIAS_PROCESSING        3
#define VALUEBUF_SIZE           40

//*****************************************************************************
//
// Cloud connection states.
//
//*****************************************************************************
typedef enum
{
    Cloud_Server_Connect,
    Cloud_Activate_CIK,
    Cloud_Sync,
    Cloud_Proxy_Set,
    Cloud_Idle
} tCloudState;

//*****************************************************************************
//
// Write/Read status of an alias to/from the cloud server.
//
//*****************************************************************************
typedef enum
{
    READ_ONLY,
    WRITE_ONLY,
    READ_WRITE,
    NONE
} tReadWriteType;

extern char g_pcMACAddress[MAC_ADDRESS_LENGTH + 1];
extern uint32_t g_ui32IPAddr;
extern bool g_bServerConnect;

//*****************************************************************************
//
// Prototypes of the functions that are called from outside the cloud_task.c
// module.
//
//*****************************************************************************
extern int32_t CloudTaskInit(void);

#endif // __CLOUD_TASK_H__
