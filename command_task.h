//*****************************************************************************
//
// command_task.h - Task to manage messages to and from virtual COM port.
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

#ifndef __COMMAND_TASK_H__
#define __COMMAND_TASK_H__

//*****************************************************************************
//
// Defines the size of the buffer used to store the command.
//
//*****************************************************************************
#define RX_BUF_SIZE             128

//*****************************************************************************
//
// Defines the size of the buffer used to store the data to be displayed on
// UART console.
//
//*****************************************************************************
#define TX_BUF_SIZE             RX_BUF_SIZE

//*****************************************************************************
//
// Defines the value that is returned on success.
//
//*****************************************************************************
#define CMDLINE_SUCCESS         (0)

//*****************************************************************************
//
// Defines the value that is returned if the command is not found.
//
//*****************************************************************************
#define CMDLINE_BAD_CMD         (-1)

//*****************************************************************************
//
// Defines the value that is returned if there are too many arguments.
//
//*****************************************************************************
#define CMDLINE_TOO_MANY_ARGS   (-2)

//*****************************************************************************
//
// Defines the value that is returned if there are too few arguments.
//
//*****************************************************************************
#define CMDLINE_TOO_FEW_ARGS    (-3)

//*****************************************************************************
//
// Defines the value that is returned if an argument is invalid.
//
//*****************************************************************************
#define CMDLINE_INVALID_ARG     (-4)

//*****************************************************************************
//
// Defines the value that is returned if unable to retreive command from UART
// buffer.
//
//*****************************************************************************
#define CMDLINE_UART_ERROR      (-5)

//*****************************************************************************
//
// Defines that inform if command was fully received or partially.
//
//*****************************************************************************
#define CMD_INCOMPLETE          (1)
#define CMD_RECEIVED            (0)

//*****************************************************************************
//
// Defines the maximum number of arguments that can be parsed.
//
//*****************************************************************************
#define CMDLINE_MAX_ARGS        8

//*****************************************************************************
//
// Command line function callback type.
//
//*****************************************************************************
typedef int (*pfnCmdLine)(int argc, char *argv[]);

//*****************************************************************************
//
// Structure for an entry in the command list table.
//
//*****************************************************************************
typedef struct
{
    //
    //! A pointer to a string containing the name of the command.
    //
    const char *pcCmd;

    //
    //! A function pointer to the implementation of the command.
    //
    pfnCmdLine pfnCmd;

    //
    //! A pointer to a string of brief help text for the command.
    //
    const char *pcHelp;
}
tCmdLineEntry;

//*****************************************************************************
//
// A structure to pass requests between the cloud task and command task using
// Mailbox.  The ui32Request element should be one of the pre-defined requests.
// The buffer can be used to pass data.
//
//*****************************************************************************
typedef struct sMailboxMsg
{
    //
    // The request identifier for this message.
    //
    uint32_t ui32Request;

    //
    // A message buffer to hold additional message data.
    //
    char pcBuf[RX_BUF_SIZE];
} tMailboxMsg;

//*****************************************************************************
//
// States to manage printing command prompt.
//
//*****************************************************************************
enum
{
    Cmd_Prompt_Print,
    Cmd_Prompt_No_Print,
    Cmd_Prompt_No_Erase
};

//*****************************************************************************
//
// This is the command table that must be provided by the application.  The
// last element of the array must be a structure whose pcCmd field contains
// a NULL pointer.
//
//*****************************************************************************
extern tCmdLineEntry g_psCmdTable[];

#endif // __COMMAND_TASK_H__
