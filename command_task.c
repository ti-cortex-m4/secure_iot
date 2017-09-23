//*****************************************************************************
//
// command_task.c - Task to manage messages to and from virtual COM port.
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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ti/drivers/UART.h>
#include <ti/net/network.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <xdc/cfg/global.h>
#include <xdc/runtime/System.h>
#include "Board.h"
#include "UARTUtils.h"
#include "board_funcs.h"
#include "command_task.h"
#include "cloud_task.h"
#include "ntp_time.h"
#include "priorities.h"
#include "tictactoe.h"

//*****************************************************************************
//
// The banner that is printed when the application starts..
//
//*****************************************************************************
#define BANNER                  "\n\tWelcome to the Crypto Connected "        \
                                "LaunchPad's,\n\t\tSecure Internet of "       \
                                "Things Demo.\r\n\n"
//*****************************************************************************
//
// UART console handle needed by the UART driver.
//
//*****************************************************************************
UART_Handle g_psUARTHandle;

//*****************************************************************************
//
// An array to hold the pointers to the command line arguments.
//
//*****************************************************************************
static char *g_ppcArgv[CMDLINE_MAX_ARGS + 1];

//*****************************************************************************
//
// An array to hold the command.
//
//*****************************************************************************
static char g_pcRxBuf[RX_BUF_SIZE];

//*****************************************************************************
//
// An array to hold the data to be displayed on UART console.
//
//*****************************************************************************
char g_pcTXBuf[TX_BUF_SIZE];

//*****************************************************************************
//
// Flag to keep track of TicTacToe game state.
//
//*****************************************************************************
static bool g_bGameActive = false;

//*****************************************************************************
//
// Array of possible alert messages.
//
//*****************************************************************************
char *g_ppcAlertMessages[] =
{
    "Hello World!!",
    "Testing Exosite scripting features.",
    "Log into Exosite for a quick game of tic-tac-toe!",
    NULL
};

extern tReadWriteType g_eLEDD1RW;
extern uint32_t g_ui32LEDD1;

extern char g_pcEmail[100];
extern tReadWriteType g_eEmailRW;

extern char g_pcAlert[50];
extern tReadWriteType g_eAlertRW;

//*****************************************************************************
//
// This function implements the "help" command.  It prints a simple list of the
// available commands with a brief description.
//
//*****************************************************************************
int
Cmd_help(int argc, char *argv[])
{
    tCmdLineEntry *pEntry;
    uint32_t ui32BufLen = 0;

    //
    // Print some header text.
    //
    ui32BufLen = snprintf(g_pcTXBuf, TX_BUF_SIZE,"\nAvailable commands\r\n");
    UART_write(g_psUARTHandle, g_pcTXBuf, ui32BufLen);
    ui32BufLen = snprintf(g_pcTXBuf, TX_BUF_SIZE,"------------------\r\n");
    UART_write(g_psUARTHandle, g_pcTXBuf, ui32BufLen);

    //
    // Point at the beginning of the command table.
    //
    pEntry = &g_psCmdTable[0];

    //
    // Enter a loop to read each entry from the command table.  The end of the
    // table has been reached when the command name is NULL.
    //
    while(pEntry->pcCmd)
    {
        //
        // Print the command name and the brief description.
        //
        ui32BufLen = snprintf(g_pcTXBuf, TX_BUF_SIZE,"%15s%s\r\n",
                              pEntry->pcCmd, pEntry->pcHelp);
        UART_write(g_psUARTHandle, g_pcTXBuf, ui32BufLen);

        //
        // Advance to the next entry in the table.
        //
        pEntry++;
    }

    //
    // Return success.
    //
    return(CMDLINE_SUCCESS);
}

//*****************************************************************************
//
// If already connected to Exosite server, reconnects and requests a CIK.
// If not already connected, will try to connect with Exosite server and
// check for CIK in EEPROM.  If CIK is found, an attempt is made to POST data.
// If that attempt fails, then CIK is requested.
//
// If failed to connect, failure is reported.  Use this command if CIK has not
// been acquired.  Will replace any existing CIK with a new one if acquired.
//
//*****************************************************************************
int
Cmd_activate(int argc, char *argv[])
{
    tMailboxMsg sActivateRequest;

    //
    // Set the type of request.
    //
    sActivateRequest.ui32Request = Cloud_Activate_CIK;

    //
    // Send the request message.
    //
    Mailbox_post(CmdMailbox, &sActivateRequest, BIOS_NO_WAIT);

    //
    // Return success.
    //
    return(CMDLINE_SUCCESS);
}

//*****************************************************************************
//
// The "clear" command sends an ascii control code to the UART that should
// clear the screen for most PC-side terminals.
//
//*****************************************************************************
int
Cmd_clear(int argc, char *argv[])
{
    uint32_t ui32BufLen = 0;

    ui32BufLen = snprintf(g_pcTXBuf, TX_BUF_SIZE,"\033[2J\033[H");
    UART_write(g_psUARTHandle, g_pcTXBuf, ui32BufLen);

    //
    // Return success.
    //
    return(CMDLINE_SUCCESS);
}

//*****************************************************************************
//
// The "led" command can be used to manually set the state of the two on-board
// LEDs. The new LED state will also be transmitted back to the exosite server,
// so the cloud representation of the LEDs should stay in sync with the board's
// actual behavior.
//
//*****************************************************************************
int
Cmd_led(int argc, char *argv[])
{
    uint32_t ui32BufLen;

    //
    // If we have too few arguments, or the second argument starts with 'h'
    // (like the first character of help), print out information about the
    // usage of this command.
    //
    if((argc == 2) && (argv[1][0] == 'o'))
    {
        if((argv[1][1] == 'n') || (argv[1][1] == 'f'))
        {
            g_ui32LEDD1 = (argv[1][1] == 'n') ? 1 : 0;
            g_eLEDD1RW = READ_WRITE;

            return 0;
        }
    }

    //
    // The required arguments were not passed.  So print this command/s help.
    //
    ui32BufLen = snprintf(g_pcTXBuf, TX_BUF_SIZE,"\nLED command usage:\n\n"
                          "    led <on|off>\n");
    UART_write(g_psUARTHandle, g_pcTXBuf, ui32BufLen);

    return 0;
}

//*****************************************************************************
//
// The "connect" command attempts to re-establish a link with the cloud server.
// This command can be used to connect/re-connect after a cable unplug or other
// loss of internet connectivity.  Will use the existing CIK if valid, will
// acquire a new CIK as needed.
//
//*****************************************************************************
int
Cmd_connect(int argc, char *argv[])
{
    tMailboxMsg sConnectRequest;

    //
    // Set the type of request.
    //
    sConnectRequest.ui32Request = Cloud_Server_Connect;

    //
    // Send the request message.
    //
    Mailbox_post(CmdMailbox, &sConnectRequest, BIOS_NO_WAIT);

    //
    // Return success.
    //
    return(CMDLINE_SUCCESS);
}

//*****************************************************************************
//
// The "getmac" command prints the user's current MAC address to the UART.
//
//*****************************************************************************
int
Cmd_getmac(int argc, char *argv[])
{
    uint32_t ui32BufLen;

    ui32BufLen = snprintf(g_pcTXBuf, TX_BUF_SIZE, "MAC Address: %s\n",
                          g_pcMACAddress);
    UART_write(g_psUARTHandle, g_pcTXBuf, ui32BufLen);

    return 0;
}

//*****************************************************************************
//
// The proxy command accepts a URL string as a parameter. This string is then
// used as a HTTP proxy for all future internet communicaitons.
//
//*****************************************************************************
int
Cmd_proxy(int argc, char *argv[])
{
    uint32_t ui32BufLen = 0;
    tMailboxMsg sProxyRequest;

    //
    // Check the number of arguments.
    //
    if(argc == 3)
    {
        //
        // Set the type of request.
        //
        sProxyRequest.ui32Request = Cloud_Proxy_Set;

        //
        // Copy the proxy address provided by the user to the message buffer
        // which is actually sent to the other task. Also merges the port and
        // the proxy server into a single string.
        //
        snprintf(sProxyRequest.pcBuf, 128, "%s:%s", argv[1], argv[2]);

        //
        // Send the request message.
        //
        Mailbox_post(CmdMailbox, &sProxyRequest, BIOS_NO_WAIT);
    }
    else
    {
        ui32BufLen = snprintf(g_pcTXBuf, TX_BUF_SIZE,"\nProxy configuration "
                              "help:\r\n");
        UART_write(g_psUARTHandle, g_pcTXBuf, ui32BufLen);
        ui32BufLen = snprintf(g_pcTXBuf, TX_BUF_SIZE,"    The proxy command "
                              "changes the proxy behavior of this board.\r\n");
        UART_write(g_psUARTHandle, g_pcTXBuf, ui32BufLen);
        ui32BufLen = snprintf(g_pcTXBuf, TX_BUF_SIZE,"    To disable the "
                              "proxy, type:\n\n");
        UART_write(g_psUARTHandle, g_pcTXBuf, ui32BufLen);

        ui32BufLen = snprintf(g_pcTXBuf, TX_BUF_SIZE,"    proxy off\n\n");
        UART_write(g_psUARTHandle, g_pcTXBuf, ui32BufLen);

        ui32BufLen = snprintf(g_pcTXBuf, TX_BUF_SIZE,"    To enable the proxy "
                              "with a specific proxy name and port, type\r\n");
        UART_write(g_psUARTHandle, g_pcTXBuf, ui32BufLen);
        ui32BufLen = snprintf(g_pcTXBuf, TX_BUF_SIZE,"    proxy "
                              "<proxyaddress> <portnumber>. For example:\n\n");
        UART_write(g_psUARTHandle, g_pcTXBuf, ui32BufLen);

        ui32BufLen = snprintf(g_pcTXBuf, TX_BUF_SIZE,"    proxy "
                              "www.mycompanyproxy.com 80\n\n");
        UART_write(g_psUARTHandle, g_pcTXBuf, ui32BufLen);
    }

    //
    // Return success.
    //
    return(CMDLINE_SUCCESS);
}

//*****************************************************************************
//
// The setemail command allows the user to set a contact email address to be
// used for alert messages.
//
//*****************************************************************************
int
Cmd_setemail(int argc, char *argv[])
{
    uint32_t ui32BufLen = 0;

    //
    // Check the number of arguments.
    //
    if(argc != 2)
    {
        //
        // The required arguments were not passed.  So print this command's
        // help.
        //
        ui32BufLen = snprintf(g_pcTXBuf, TX_BUF_SIZE,"\nsetemail command "
                              "usage:\n\n"
                              "    setemail yourname@example.com\n");
        UART_write(g_psUARTHandle, g_pcTXBuf, ui32BufLen);

        return 0;
    }

    //
    // Otherwise, copy the user-defined location into the global variable.
    //
    strncpy(g_pcEmail, argv[1], 100);

    //
    // Make sure that the global string remains terminated with a zero.
    //
    g_pcEmail[99] = 0;

    //
    // Mark the location as READ_WRITE, so it will get uploaded to the
    // server on the next sync.
    //
    g_eEmailRW = READ_WRITE;

    ui32BufLen = snprintf(g_pcTXBuf, TX_BUF_SIZE,"Email set to: %s\n\n",
                          g_pcEmail);
    UART_write(g_psUARTHandle, g_pcTXBuf, ui32BufLen);

    return 0;
}

//*****************************************************************************
//
// The alert command allows the user to send an alert message to the saved
// email address.
//
//*****************************************************************************
int
Cmd_alert(int argc, char *argv[])
{
    uint32_t ui32Index = 0;
    uint32_t ui32BufLen;

    //
    // Check the number of arguments.
    //
    if(argc != 2)
    {
        //
        // The required arguments were not passed.  So print this command's
        // help.
        //
        ui32BufLen = snprintf(g_pcTXBuf, TX_BUF_SIZE,"\nalert command usage:"
                              "\n");
        UART_write(g_psUARTHandle, g_pcTXBuf, ui32BufLen);

        ui32Index = 0;
        while(g_ppcAlertMessages[ui32Index] != 0)
        {
            //
            // Print a list of the available alert messages.
            //
            ui32BufLen = snprintf(g_pcTXBuf, TX_BUF_SIZE, "    alert %d: %s\n",
                                  ui32Index, g_ppcAlertMessages[ui32Index]);
            UART_write(g_psUARTHandle, g_pcTXBuf, ui32BufLen);
            ui32Index++;
        }

        return 0;
    }

    ui32Index = strtoul(argv[1], NULL, 0);
    strncpy(g_pcAlert, g_ppcAlertMessages[ui32Index], sizeof(g_pcAlert));
    g_eAlertRW = READ_WRITE;

    ui32BufLen = snprintf(g_pcTXBuf, TX_BUF_SIZE,"Alert message set. Sending "
                          "to the server on the next sync operation.\n");
    UART_write(g_psUARTHandle, g_pcTXBuf, ui32BufLen);

    return 0;
}

//*****************************************************************************
//
// The tictactoe command allows users to play a game of tic-tac-toe.
//
//*****************************************************************************
int
Cmd_tictactoe(int argc, char *argv[])
{
    g_bGameActive = true;

    GameInit();

    return 0;
}

//*****************************************************************************
//
// The ntp command allows users to provide a different NTP IP address.
//
//*****************************************************************************
int
Cmd_ntp(int argc, char *argv[])
{
    uint32_t ui32BufLen;
    tMailboxMsg sNTPIP;
    struct sockaddr_in sNTPSockAddr;
    int32_t i32Ret;

    //
    // Check the number of arguments.
    //
    if(argc == 2)
    {
        //
        // Correct number of arguments were entered.  Copy arg 1 into the local
        // buffer and process the arguments.
        //
        snprintf(sNTPIP.pcBuf, 128, "%s", argv[1]);

        //
        // Check if user entered IP address or URL.
        //
        i32Ret = inet_pton(AF_INET, sNTPIP.pcBuf, &sNTPSockAddr.sin_addr);
        if(i32Ret == 1)
        {
            //
            // IP address has been entered.  If the IP address is a non-zero
            // value, pass this information to the NTP module.
            //
            if(sNTPSockAddr.sin_addr.s_addr != 0)
            {
                sNTPIP.ui32Request = NTP_Connect;

                //
                // Send the request message and return.
                //
                Mailbox_post(CmdMailbox, &sNTPIP, BIOS_NO_WAIT);
                return 0;
            }
        }
        else
        {
            //
            // User did not enter IP address.  Assume it is a URL.
            //
            sNTPIP.ui32Request = NTP_Resolve_URL;

            //
            // Send the request message and return.
            //
            Mailbox_post(CmdMailbox, &sNTPIP, BIOS_NO_WAIT);
            return 0;
        }
    }

    //
    // Either the required arguments were not passed or wrong IP address was
    // passed.  Print help and return ensuring that nothing is done until
    // user enters the correct NTP server details.
    //
    ui32BufLen = snprintf(g_pcTXBuf, TX_BUF_SIZE,"Provide correct NTP "
                          "address to proceed.\n  Command: ntp <IP>\n    "
                          "<IP> can be in the form \"time.nist.gov\" or "
                          "\"192.168.1.1\"\n");
    UART_write(g_psUARTHandle, g_pcTXBuf, ui32BufLen);

    return 0;
}

//*****************************************************************************
//
// This is the table that holds the command names, implementing functions, and
// brief description.
//
//*****************************************************************************
tCmdLineEntry g_psCmdTable[] =
{
    { "help",      Cmd_help,      ": Display list of commands" },
    { "h",         Cmd_help,      ": alias for help" },
    { "?",         Cmd_help,      ": alias for help" },
    { "activate",  Cmd_activate,  ": Get a CIK from exosite" },
    { "alert",     Cmd_alert,     ": Send an alert to the saved email "
                                  "address."},
    { "clear",     Cmd_clear,     ": Clear the display " },
    { "connect",   Cmd_connect,   ": Tries to establish a connection with"
                                  " exosite." },
    { "getmac",    Cmd_getmac,    ": Prints the current MAC address."},
    { "led",       Cmd_led,       ": Toggle LEDs. Type \"led help\" for more "
                                  "info." },
    { "ntp",       Cmd_ntp,       ": Tries to connenct to the provided IP "
                                  "during start-up!"},
    { "proxy",     Cmd_proxy,     ": Set or disable a HTTP proxy server." },
    { "setemail",  Cmd_setemail,  ": Change the email address used for "
                                  "alerts."},
    { "tictactoe", Cmd_tictactoe, ": Play tic-tac-toe!"},
    { 0, 0, 0 }
};

//*****************************************************************************
//
// This function reads the available characters from the UART buffer and checks
// if a command is received.
//
// If CR or LF is received then it is assumend that a command is received, it
// is copied into a global RX buffer and the flag "CMD_RECEIVED" is returned.
//
// If CR or LF is not found then the flag "CMD_INCOMPLETE" is sent to
// indicate that command might be incomplete.
//
// If data is received continuously without a CR or LF and exceeds the length
// of the global RX buffer, then it is deemed as bad command and the flag
// "CMDLINE_BAD_CMD" is returned. If the UART driver reports an error then the
// flag "CMDLINE_UART_ERROR" is is returned.
//
//*****************************************************************************
int32_t
CommandReceived(uint32_t ui32RxData)
{
    int32_t i32Ret;
    static uint32_t ui32Idx = 0;

    //
    // Read data till .
    //
    while(ui32RxData)
    {
        //
        // Read one character at a time.
        //
        i32Ret = UART_read(g_psUARTHandle, g_pcRxBuf + ui32Idx, 1);
        if(i32Ret == UART_ERROR)
        {
            return CMDLINE_UART_ERROR;
        }

        //
        // Did we get CR or LF?
        //
        if((*(g_pcRxBuf + ui32Idx) == '\r') ||
           (*(g_pcRxBuf + ui32Idx) == '\n'))
        {
            //
            // Yes - Copy null character to help in string operations and
            // return success.
            //
            *(g_pcRxBuf + ui32Idx) = '\0';
            ui32Idx = 0;
            return CMD_RECEIVED;
        }

        ui32RxData--;
        ui32Idx++;

        if(ui32Idx > RX_BUF_SIZE)
        {
            ui32Idx = 0;
            memset(g_pcRxBuf, 0, RX_BUF_SIZE);
            return CMDLINE_BAD_CMD;
        }
    }

    //
    // We did not get CR or LF.  So return failure to find .
    //
    return CMD_INCOMPLETE;
}

//*****************************************************************************
//
// This function processes a command line string into arguments and executes
// the command.
//
// This function will take the supplied command line string from the global
// RX buffer and break it up into individual arguments.  The first argument is
// treated as a command and is searched for in the command table.  If the
// command is found, then the command function is called and all of the command
// line arguments are passed in the normal argc, argv form.
//
// The command table is contained in an array named "g_psCmdTable" containing
// "tCmdLineEntry" structures.  The array must be terminated with an entry
// whose "pcCmd" field contains a NULL pointer.
//
// This function returns either "CMDLINE_BAD_CMD" if the command is not found
// or "CMDLINE_TOO_MANY_ARGS" if there are more arguments than can be parsed.
// Otherwise it returns the code that was returned by the command function.
//
//*****************************************************************************
int32_t
CmdLineProcess(uint32_t ui32DataLen)
{
    char *pcChar;
    uint_fast8_t ui8Argc;
    bool bFindArg = true;
    tCmdLineEntry *psCmdEntry;

    //
    // Initialize the argument counter, and point to the beginning of the
    // command line string.
    //
    ui8Argc = 0;
    pcChar = g_pcRxBuf;

    //
    // Advance through the command line until a zero character is found.
    //
    while(*pcChar)
    {
        //
        // If there is a space, then replace it with a zero, and set the flag
        // to search for the next argument.
        //
        if(*pcChar == ' ')
        {
            *pcChar = 0;
            bFindArg = true;
        }

        //
        // Otherwise it is not a space, so it must be a character that is part
        // of an argument.
        //
        else
        {
            //
            // If bFindArg is set, then that means we are looking for the start
            // of the next argument.
            //
            if(bFindArg)
            {
                //
                // As long as the maximum number of arguments has not been
                // reached, then save the pointer to the start of this new arg
                // in the argv array, and increment the count of args, argc.
                //
                if(ui8Argc < CMDLINE_MAX_ARGS)
                {
                    g_ppcArgv[ui8Argc] = pcChar;
                    ui8Argc++;
                    bFindArg = false;
                }

                //
                // The maximum number of arguments has been reached so return
                // the error.
                //
                else
                {
                    return(CMDLINE_TOO_MANY_ARGS);
                }
            }
        }

        //
        // Advance to the next character in the command line.
        //
        pcChar++;
    }

    //
    // If one or more arguments was found, then process the command.
    //
    if(ui8Argc)
    {
        //
        // Start at the beginning of the command table, to look for a matching
        // command.
        //
        psCmdEntry = &g_psCmdTable[0];

        //
        // Search through the command table until a null command string is
        // found, which marks the end of the table.
        //
        while(psCmdEntry->pcCmd)
        {
            //
            // If this command entry command string matches argv[0], then call
            // the function for this command, passing the command line
            // arguments.
            //
            if(!strcmp(g_ppcArgv[0], psCmdEntry->pcCmd))
            {
                return(psCmdEntry->pfnCmd(ui8Argc, g_ppcArgv));
            }

            //
            // Not found, so advance to the next entry.
            //
            psCmdEntry++;
        }
    }

    //
    // Fall through to here means that no matching command was found, so return
    // an error.
    //
    return(CMDLINE_BAD_CMD);
}

//*****************************************************************************
//
// This function reports an error to the user of the command prompt.
//
//*****************************************************************************
void
CmdLineErrorHandle(int32_t i32Ret)
{
    uint32_t ui32BufLen;

    //
    // Handle the case of bad command.
    //
    if(i32Ret == CMDLINE_BAD_CMD)
    {
        ui32BufLen = snprintf(g_pcTXBuf, TX_BUF_SIZE, "Bad command! Type "
                              "\"help\" for a list of commands.\r\n");
        UART_write(g_psUARTHandle, g_pcTXBuf, ui32BufLen);
    }

    //
    // Handle the case of too many arguments.
    //
    else if(i32Ret == CMDLINE_TOO_MANY_ARGS)
    {
        ui32BufLen = snprintf(g_pcTXBuf, TX_BUF_SIZE, "Too many arguments for "
                              "command processor!\r\n");
        UART_write(g_psUARTHandle, g_pcTXBuf, ui32BufLen);
    }

    //
    // Handle the case of too few arguments.
    //
    else if(i32Ret == CMDLINE_TOO_FEW_ARGS)
    {
        ui32BufLen = snprintf(g_pcTXBuf, TX_BUF_SIZE, "Too few arguments for "
                              "command processor!\r\n");
        UART_write(g_psUARTHandle, g_pcTXBuf, ui32BufLen);
    }

    //
    // Handle the case of UART read error.
    //
    else if(i32Ret == CMDLINE_UART_ERROR)
    {
        ui32BufLen = snprintf(g_pcTXBuf, TX_BUF_SIZE, "UART Read error!\r\n");
        UART_write(g_psUARTHandle, g_pcTXBuf, ui32BufLen);
    }
}

//*****************************************************************************
//
// This task manages the COM port.  This function is created statically using
// the project's .cfg file.
//
//*****************************************************************************
void CommandTask(unsigned int arg0, unsigned int arg1)
{
    UART_Params sUARTParams;
    int32_t i32RxCount;
    int32_t i32Ret;
    uint32_t ui32BufLen;
    tMailboxMsg sCloudMbox;

    //
    // Create a UART parameters instance.  The default parameter are 115200
    // baud, 8 data bits, 1 stop bit and no parity.
    //
    UART_Params_init(&sUARTParams);

    //
    // Modify some of the default parameters for this application.
    //
    sUARTParams.readReturnMode = UART_RETURN_FULL;
    sUARTParams.readEcho = UART_ECHO_ON;

    //
    // Configure UART0 with the above parameters.
    //
    g_psUARTHandle = UART_open(Board_UART0, &sUARTParams);
    if (g_psUARTHandle == NULL)
    {
        System_printf("Error opening the UART\r\n");
    }

    //
    // Print the banner to the debug console
    //
    UART_write(g_psUARTHandle, BANNER, sizeof(BANNER));
    System_printf(BANNER);

    //
    // Get MAC address.  If unsuccessful, exit.
    //
    if(GetMacAddress(g_pcMACAddress, sizeof(g_pcMACAddress)) == false)
    {
        //
        // Return error.
        //
        snprintf(g_pcTXBuf, TX_BUF_SIZE, "Failed to get MAC address.  Exiting."
                 "\n");
        System_printf(g_pcTXBuf);
        BIOS_exit(1);
    }

    //
    // Print Mac Address.
    //
    ui32BufLen = snprintf(g_pcTXBuf, TX_BUF_SIZE, "MAC Address: %s\n",
                          g_pcMACAddress);
    UART_write(g_psUARTHandle, g_pcTXBuf, ui32BufLen);
    System_printf(g_pcTXBuf);

    //
    // Print help instructions.  These will be erased when we have new data
    // from cloud task to print.
    //
    ui32BufLen = snprintf(g_pcTXBuf, TX_BUF_SIZE, "Acquiring IP address...");
    UART_write(g_psUARTHandle, g_pcTXBuf, ui32BufLen);

    //
    // Loop forever receiving commands.
    //
    while(1)
    {
        //
        // Check if data is available to be read from UART.
        //
        UART_control(g_psUARTHandle, UART_CMD_GETRXCOUNT, (void *)&i32RxCount);
        if(i32RxCount > 0)
        {
            //
            // Yes - See if we got a command.
            //
            i32Ret = CommandReceived((uint32_t)i32RxCount);
            if(i32Ret == CMD_RECEIVED)
            {
                //
                // Yes - Process it.  If in TicTacToe game mode, allow the
                // games's state machine to process states that are dependent
                // on commands.
                //
                if(g_bGameActive == true)
                {
                    if(AdvanceGameState(g_pcRxBuf, true) == true)
                    {
                        //
                        // Game completed.  Adjust the flag accordingly.
                        //
                        g_bGameActive = false;
                    }
                }
                else
                {
                    i32Ret = CmdLineProcess(i32Ret);
                    memset(g_pcRxBuf, 0, RX_BUF_SIZE);
                }
            }

            //
            // Did we receive any error?
            //
            if((i32Ret != CMD_INCOMPLETE) || (i32Ret != CMDLINE_SUCCESS))
            {
                //
                // Yes - Handle the error.
                //
                if(g_bGameActive != true)
                {
                    CmdLineErrorHandle(i32Ret);
                }
            }

            //
            // Print prompt except when full command is not received.
            //
            if((g_bGameActive != true) && (i32Ret != CMD_INCOMPLETE))
            {
                UART_write(g_psUARTHandle, "> ", 2);
            }
        }

        //
        // If in TicTacToe game mode, allow the games's state machine to
        // process this states that are not dependent on user input.
        //
        if(g_bGameActive == true)
        {
            if(AdvanceGameState(NULL, false) == true)
            {
                //
                // Game completed.  Adjust the flag accordingly and
                // print prompt.
                //
                g_bGameActive = false;
                UART_write(g_psUARTHandle, "> ", 2);
            }
        }

        //
        // Check if we received any data to be printed to UART from Cloud task.
        //
        if(Mailbox_pend(CloudMailbox, &sCloudMbox, 100) == true)
        {
            //
            // Yes - Stop the TicTacToe game if it is still running.
            //
            if(g_bGameActive == true)
            {
                g_bGameActive = false;
                ui32BufLen = snprintf(g_pcTXBuf, TX_BUF_SIZE, "\nExiting the "
                                      "TicTacToe Game due to an error.\n");
                UART_write(g_psUARTHandle, g_pcTXBuf, ui32BufLen);
                System_printf(g_pcTXBuf);
            }

            //
            // Get the length of the data to be printed and pass this
            // information to the UART driver to print.  Don't forget to erase
            // the command prompt before printing the information received.
            //
            ui32BufLen = strlen(sCloudMbox.pcBuf);
            if(sCloudMbox.ui32Request != Cmd_Prompt_No_Erase)
            {
                UART_write(g_psUARTHandle, "\033[1K\r", 5);
            }
            UART_write(g_psUARTHandle, sCloudMbox.pcBuf, ui32BufLen);

            //
            // Print prompt for all cases except when the Cloud task wants to
            // indicate progress by printing dots.
            //
            if(sCloudMbox.ui32Request == Cmd_Prompt_Print)
            {
                //
                // Print prompt.
                //
                UART_write(g_psUARTHandle, "> ", 2);
            }
        }
    }
}
