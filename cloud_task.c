//*****************************************************************************
//
// cloud_task.c - Task to connect and communicate to the cloud server.  This
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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ti/drivers/GPIO.h>
#include <ti/net/http/httpcli.h>
#include <ti/net/http/sswolfssl.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <xdc/cfg/global.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>
#include "Board.h"
#include "board_funcs.h"
#include "certificate.h"
#include "cloud_task.h"
#include "command_task.h"
#include "ntp_time.h"
#include "priorities.h"
#include "tictactoe.h"

//*****************************************************************************
//
// Stack size of the Cloud task.  This task needs a big stack as WolfSSL
// function calls require bigger stack.
//
//*****************************************************************************
#define STACK_CLOUD_TASK        20000

//*****************************************************************************
//
// Sys BIOS Frequency is again define here as it is deeded to calculate time
// since start.
//
//*****************************************************************************
#define BIOS_TICK_RATE          1000

//*****************************************************************************
//
// Defines used by POST and GET requests.
//
//*****************************************************************************
#define PROVISION_URI           "/provision/activate"
#define EXOSITE_URI             "/onep:v1/stack/alias"
#define EXOSITE_HOSTNAME        "m2.exosite.com"
#define EXOSITE_CONTENT_TYPE    "application/x-www-form-urlencoded; "         \
                                "charset=utf-8"
#define EXOSITE_TYPE            "X-Exosite-CIK"

//*****************************************************************************
//
// Global resource to store Exosite CIK.
//
//*****************************************************************************
static char g_pcExositeCIK[EXOSITE_CIK_LENGTH + 1];

//*****************************************************************************
//
// Global resource to store MAC Address.
//
//*****************************************************************************
char g_pcMACAddress[MAC_ADDRESS_LENGTH + 1];

//*****************************************************************************
//
// Global resource to store IP Address.
//
//*****************************************************************************
uint32_t g_ui32IPAddr;

//*****************************************************************************
//
// An array to hold the data to be displayed on UART console.
//
//*****************************************************************************
tMailboxMsg g_sDebug;

//*****************************************************************************
//
// The following field(s) are set by using the API HTTPCli_setRequestFields.
// These fields are added automatically to the buffer when the API
// HTTPCli_sendRequest is called.
//
//*****************************************************************************
HTTPCli_Field g_psFields[2] =
{
    {"Host", EXOSITE_HOSTNAME},
    {NULL, NULL}
};

//*****************************************************************************
//
// The alias sent to Exosite server with POST request.
//
//*****************************************************************************
const char g_ppcPOSTAlias[7][15] =
{
    "usrsw1",
    "usrsw2",
    "jtemp",
    "ontime",
    "gamestate",
    "ledd1",
    "emailaddr"
};

//*****************************************************************************
//
// The alias values requested from Exosite server with GET request.
//
//*****************************************************************************
const char g_ppcGETAlias[ALIAS_PROCESSING][15] =
{
    "ledd1",
    "emailaddr",
    "gamestate"
};

//*****************************************************************************
//
// The alias sent to Exosite server while provisioning for a CIK.
//
//*****************************************************************************
const char g_ppcProvAlias[3][10] =
{
    "vendor",
    "model",
    "sn"
};

//*****************************************************************************
//
// Global resources to hold tictactoe game state and read/write mode.
//
//*****************************************************************************
uint32_t g_ui32BoardState = 0;
tReadWriteType g_eBoardStaeRW = WRITE_ONLY;

//*****************************************************************************
//
// Global resources to hold led state and read/write mode.
//
//*****************************************************************************
uint32_t g_ui32LEDD1 = 0;
uint32_t g_ui32LastLEDD1 = 0;
tReadWriteType g_eLEDD1RW = READ_WRITE;

//*****************************************************************************
//
// Global resources to hold email address and read/write mode.
//
//*****************************************************************************
char g_pcEmail[100];
tReadWriteType g_eEmailRW = READ_WRITE;

//*****************************************************************************
//
// Global resources to hold alerts and read/write mode.
//
//*****************************************************************************
char g_pcAlert[50];
tReadWriteType g_eAlertRW = NONE;

//*****************************************************************************
//
// Global resource to hold cloud connection states.
//
//*****************************************************************************
tCloudState g_ui32State = Cloud_Idle;

//*****************************************************************************
//
// Global resource to indicate the state of the connection to the cloud server.
//
//*****************************************************************************
bool g_bServerConnect = false;

//*****************************************************************************
//
// Global resource to hold IP address and proxy setting.
//
//*****************************************************************************
#ifdef SET_PROXY
char g_pcIP[50] = PROXY_ADDR;
bool g_bProxy = 1;
#else
char g_pcIP[50] = EXOSITE_ADDR;
bool g_bProxy = 0;
#endif

//*****************************************************************************
//
// This function creates a HTTP client instance and connect to the Exosite
// Server.
//
//*****************************************************************************
int32_t
ServerConnect(HTTPCli_Handle cli)
{
    int32_t i32Ret = 0;
    struct sockaddr_in sSockAddr;
    uint32_t ui32Retry = 0;
    char * pcDebug;

    g_sDebug.ui32Request = Cmd_Prompt_Print;
    pcDebug = g_sDebug.pcBuf;

    //
    // Set-up a socket to communicate with Exosite server.
    //
    i32Ret = HTTPCli_initSockAddr((struct sockaddr *)&sSockAddr,
                                  g_pcIP, 0);
    if(i32Ret != 0)
    {
        //
        // Failed to create socket.  Report error and return.
        //
        snprintf(pcDebug, TX_BUF_SIZE, "Failed to resolve host name. Check "
                 "proxy server settings. Ecode: %d.\n", i32Ret);
        Mailbox_post(CloudMailbox, &g_sDebug, BIOS_NO_WAIT);
        System_printf(pcDebug);
        return (-1);
    }

    if(g_bProxy == true)
    {
        //
        // If proxy settings are needed, set them here.
        //
        HTTPCli_setProxy((struct sockaddr *)&sSockAddr);
    }

    g_sDebug.ui32Request = Cmd_Prompt_No_Print;
    snprintf(pcDebug, TX_BUF_SIZE, "Connecting to server...");
    Mailbox_post(CloudMailbox, &g_sDebug, 100);
    System_printf(pcDebug);
    System_printf("\n");
    g_sDebug.ui32Request = Cmd_Prompt_Print;

    while(ui32Retry < 5)
    {
        //
        // Create an HTTP client instance.
        //
        HTTPCli_construct(cli);

        //
        // Set-up headers that are to be sent automatically with GET/POST
        // request.
        //
        HTTPCli_setRequestFields(cli, g_psFields);

        //
        // Connect a socket to Exosite server in secure mode.
        //
        i32Ret = HTTPCli_connect(cli, (struct sockaddr *)&sSockAddr,
                                 HTTPCli_TYPE_TLS, NULL);
        if(i32Ret == 0)
        {
            //
            // Success.  Return from the loop.
            //
            snprintf(pcDebug, TX_BUF_SIZE, "Connected to Exosite server.\n");
            Mailbox_post(CloudMailbox, &g_sDebug, 100);
            System_printf(pcDebug);
            return (0);
        }
        else
        {
            //
            // If we failed to connect, display this information and try for
            // a few more times.
            //
            g_sDebug.ui32Request = Cmd_Prompt_No_Print;
            snprintf(pcDebug, TX_BUF_SIZE, "Failed to connect to server, "
                     "ecode: %d. Retrying...", i32Ret);
            Mailbox_post(CloudMailbox, &g_sDebug, BIOS_NO_WAIT);
            System_printf(pcDebug);
            g_sDebug.ui32Request = Cmd_Prompt_Print;

            ui32Retry ++;

            //
            // Deconstruct the HTTP client instance.
            //
            HTTPCli_destruct(cli);

            //
            // Sleep for a sec between each trial.
            //
            Task_sleep(1000);
        }
    }

    //
    // Failed to connect so return with error.
    //
    snprintf(pcDebug, TX_BUF_SIZE, "Failed to connect to server after 5 "
             "trials.\n");
    Mailbox_post(CloudMailbox, &g_sDebug, BIOS_NO_WAIT);
    System_printf(pcDebug);
    return -1;
}

//*****************************************************************************
//
// Disconnect from the Exosite Server and destroy the HTTP client instance.
//
//*****************************************************************************
void
ServerDisconnect(HTTPCli_Handle cli)
{
    HTTPCli_disconnect(cli);
}

//*****************************************************************************
//
// Disconnect and Connect back to the Exosite Server.
//
//*****************************************************************************
void
ServerReconnect(HTTPCli_Handle cli)
{
    ServerDisconnect(cli);
    ServerConnect(cli);
}

//*****************************************************************************
//
// This function tries to read a CIK from non-volatile memory and reports if
// one if found or not.  If a CIK is present in NVM then it is copied to global
// resource.
//
//*****************************************************************************
bool
GetCIK(HTTPCli_Handle cli)
{
    char pcCIK[EXOSITE_CIK_LENGTH + 1];
    char * pcDebug;

    g_sDebug.ui32Request = Cmd_Prompt_Print;
    pcDebug = g_sDebug.pcBuf;

    if(GetCIKEEPROM(pcCIK) == true)
    {
        if(pcCIK != NULL)
        {
            //
            // CIK Found.  Update the global resource with CIK found in NVM.
            //
            memcpy(g_pcExositeCIK, pcCIK, EXOSITE_CIK_LENGTH + 1);

            //
            // Report this to user.
            //
            snprintf(pcDebug, TX_BUF_SIZE, "CIK found in EEPROM %s\n",
                     g_pcExositeCIK);
            Mailbox_post(CloudMailbox, &g_sDebug, BIOS_NO_WAIT);
            System_printf(pcDebug);

            //
            // Return success.
            //
            return true;
        }
    }

    //
    // CIK not found. Report this.
    //
    snprintf(pcDebug, TX_BUF_SIZE, "No CIK found in EEPROM.\n");
    Mailbox_post(CloudMailbox, &g_sDebug, BIOS_NO_WAIT);
    System_printf(pcDebug);

    //
    // Return failure.
    //
    return false;
}

//*****************************************************************************
//
// Set the proxy value if requested by the user.
//
//*****************************************************************************
void
CloudProxySet(HTTPCli_Handle cli, char *pcProxy)
{
    char * pcPort;

    //
    // Check if the buffer has proxy address.
    //
    if(pcProxy == NULL)
    {
        //
        // No - Return without doing anything.
        //
        g_ui32State = Cloud_Idle;
        return;
    }

    //
    // Check if a port number was passed.
    //
    pcPort = strstr(pcProxy, ":");
    if(pcPort == NULL)
    {
        //
        // No - Return without doing anything.
        //
        g_ui32State = Cloud_Idle;
        return;
    }

    //
    // Copy the proxy address to global resource
    //
    strncpy(g_pcIP, pcProxy, 50);

    //
    // Set the global proxy flag.
    //
    g_bProxy = 1;

    return;
}

//*****************************************************************************
//
// Populate the request body for a POST request to get the CIK.  The request
// body will have the provisioning information that Exosite server seeks.
//
//*****************************************************************************
int32_t
BuildProvInfo(const char *pcVendorName, const char *pcBoard,
              char *pui8MACAddress, char *pcProvBuf, uint32_t ui32ProvBufLen)
{
    uint32_t ui32BufLen = 0;

    //
    // Build the request body for provisioning.
    //
    ui32BufLen = snprintf(pcProvBuf, ui32ProvBufLen, "%s=%s",
                          g_ppcProvAlias[0], pcVendorName);
    ui32BufLen += snprintf((pcProvBuf + ui32BufLen),
                           (ui32ProvBufLen - ui32BufLen), "&%s=%s",
                           g_ppcProvAlias[1], pcBoard);
    ui32BufLen += snprintf((pcProvBuf + ui32BufLen),
                           (ui32ProvBufLen - ui32BufLen), "&%s=%s",
                           g_ppcProvAlias[2], pui8MACAddress);

    //
    // Indicate Success.
    //
    return 0;
}

//*****************************************************************************
//
// Returns time since reset, in ui32PrevTicks.
//
//*****************************************************************************
uint32_t
ReadOnTime(void)
{
    static uint32_t ui32Seconds = 0;
    static uint32_t ui32PrevTicks = 0;
    uint32_t ui32Ticks = 0;

    ui32Ticks = Clock_getTicks();

    //
    // Calculate seconds since reset by accounting for roolover of ticks.
    //
    if(ui32Ticks > ui32PrevTicks)
    {
        ui32Seconds += ((ui32Ticks - ui32PrevTicks) / BIOS_TICK_RATE);
    }
    else
    {
        ui32Seconds += ((((uint32_t)(0xFFFFFFFF)) - ui32PrevTicks +
                         ui32Ticks) / BIOS_TICK_RATE);
    }
    ui32PrevTicks = ui32Ticks;

    return (ui32Seconds);
}

//*****************************************************************************
//
// Builds the Request Body for the POST request.
//
//*****************************************************************************
void
GetRequestBody(char* pcDataBuf, uint32_t ui32DataBufLen)
{
    uint16_t ui16Temp = 0;
    uint32_t pui32Buttons[2] = {0, 0};
    uint32_t ui32DataLen = 0;
    uint32_t ui32OnTime = 0;

    ui16Temp = ReadInternalTemp();
    ReadButtons(pui32Buttons);
    ui32OnTime = ReadOnTime();
    ui32DataLen = snprintf(pcDataBuf, ui32DataBufLen, "usrsw1=%d&usrsw2=%d",
                           pui32Buttons[0], pui32Buttons[1]);
    ui32DataLen += snprintf((pcDataBuf + ui32DataLen),
                            (ui32DataBufLen - ui32DataLen), "&jtemp=%u",
                            ui16Temp);
    ui32DataLen += snprintf((pcDataBuf + ui32DataLen),
                            (ui32DataBufLen - ui32DataLen), "&ontime=%d",
                            ui32OnTime);
    if((g_eBoardStaeRW == READ_WRITE) || (g_eBoardStaeRW == WRITE_ONLY))
    {
        ui32DataLen += snprintf((pcDataBuf + ui32DataLen),
                                (ui32DataBufLen - ui32DataLen), "&gamestate="
                                "0x%x", g_ui32BoardState);
    }
    if((g_eLEDD1RW == READ_WRITE) || (g_eLEDD1RW == WRITE_ONLY))
    {
        ui32DataLen += snprintf((pcDataBuf + ui32DataLen),
                                (ui32DataBufLen - ui32DataLen), "&ledd1=%d",
                                g_ui32LEDD1);
    }
    if((g_eEmailRW == READ_WRITE) || (g_eEmailRW == WRITE_ONLY))
    {
        ui32DataLen += snprintf((pcDataBuf + ui32DataLen),
                                (ui32DataBufLen - ui32DataLen),
                                "&emailaddr=%s", g_pcEmail);
    }
    if((g_eAlertRW == READ_WRITE) || (g_eAlertRW == WRITE_ONLY))
    {
        ui32DataLen += snprintf((pcDataBuf + ui32DataLen),
                                (ui32DataBufLen - ui32DataLen),
                                "&alert=%s", g_pcAlert);
    }
    if(g_eLEDD1RW == READ_WRITE)
    {
        g_eLEDD1RW = READ_ONLY;
    }
    if(g_eBoardStaeRW == READ_WRITE)
    {
        g_eBoardStaeRW = READ_ONLY;
    }
    if(g_eEmailRW == READ_WRITE)
    {
        g_eEmailRW = READ_ONLY;
    }
    if(g_eAlertRW == READ_WRITE)
    {
        g_eAlertRW = NONE;
    }
}

//*****************************************************************************
//
// Builds the Alias List that can be sent with the POST request.
//
//*****************************************************************************
void
GetAliasList(char* pcDataBuf, uint32_t ui32DataBufLen)
{
    uint32_t ui32DataLen = 0;

    ui32DataLen = snprintf(pcDataBuf, ui32DataBufLen, "?location");
    if(g_eLEDD1RW == READ_ONLY)
    {
        ui32DataLen += snprintf((pcDataBuf + ui32DataLen),
                                (ui32DataBufLen - ui32DataLen), "&ledd1");
    }
    if(g_eBoardStaeRW == READ_ONLY)
    {
        ui32DataLen += snprintf((pcDataBuf + ui32DataLen),
                                (ui32DataBufLen - ui32DataLen), "&gamestate");
    }
    if(g_eEmailRW == READ_ONLY)
    {
        ui32DataLen += snprintf((pcDataBuf + ui32DataLen),
                                (ui32DataBufLen - ui32DataLen), "&emailaddr");
    }
}

//*****************************************************************************
//
// Update cloud data.
//
//*****************************************************************************
void
UpdateCloudData(void)
{
    if(g_ui32LEDD1 != g_ui32LastLEDD1)
    {
        g_ui32LastLEDD1 = g_ui32LEDD1;
        if(g_ui32LEDD1)
        {
            GPIO_write(Board_LED0, Board_LED_ON);
        }
        else
        {
            GPIO_write(Board_LED0, Board_LED_OFF);
        }
    }
}

//*****************************************************************************
//
// Handles the response Body for the GET request.  For now, this only controls
// the LEDs.
//
//*****************************************************************************
void
ProcessResponseBody(char *pcBuf)
{
    uint32_t ui32Index, j;
    char *pcValueStart = NULL;
    char ppcValueBuf[ALIAS_PROCESSING][VALUEBUF_SIZE];

    //
    // For now only look for the first two values of the Alias.
    //
    for(ui32Index = 0; ui32Index < ALIAS_PROCESSING; ui32Index++)
    {
        //
        // Search the alias in the buffer.
        //
        pcValueStart = strstr(pcBuf, g_ppcGETAlias[ui32Index]);

        //
        // If we could not find the alias in the buffer, continue to the next
        // value.
        //
        if(!pcValueStart)
        {
            continue;
        }

        //
        // Find the equals-sign, which should be just before the start of the
        // value.
        //
        pcValueStart = strstr(pcValueStart, "=");

        //
        // If we could not find the equals-sign in the buffer, continue to the
        // next value.
        //
        if(!pcValueStart)
        {
            continue;
        }

        //
        // Advance to the first character of the value.
        //
        pcValueStart++;

        //
        // Loop through the buffer to reach the end of the input value and copy
        // characters to the destination string.
        //
        j = 0;
        while(j < VALUEBUF_SIZE)
        {
            //
            // Check for the end of the value string.
            //
            if((pcValueStart[j] == '&') ||
               (pcValueStart[j] == 0))
            {
                //
                // If we have reached the end of the value, null-terminate the
                // destination string, and return.
                //
                ppcValueBuf[ui32Index][j] = 0;
                break;
            }
            else
            {
                ppcValueBuf[ui32Index][j] = pcValueStart[j];
            }

            j++;
        }
    }
    for(ui32Index = 0; ui32Index < ALIAS_PROCESSING; ui32Index++)
    {
        if(strncmp(g_ppcGETAlias[ui32Index], "ledd1", 15) == 0)
        {
            if(g_eLEDD1RW == READ_ONLY)
            {
                g_ui32LEDD1 = (ppcValueBuf[ui32Index][0] == '1') ? 1 : 0;
            }
        }
        else if(strncmp(g_ppcGETAlias[ui32Index], "emailaddr", 15) == 0)
        {
            if(g_eEmailRW == READ_ONLY)
            {
                strncpy(g_pcEmail, ppcValueBuf[ui32Index], sizeof(g_pcEmail));
                g_pcEmail[99] = '\0';
            }
        }
        else if(strncmp(g_ppcGETAlias[ui32Index], "gamestate", 15) == 0)
        {
            if(g_eBoardStaeRW == READ_ONLY)
            {
                g_ui32BoardState = strtoul(ppcValueBuf[ui32Index], NULL, 0);
            }
        }
    }
}

//*****************************************************************************
//
// This function handles the server reported error message from functions like
// ExositeActivate, ExositeWrite and ExositeRead.
//
//*****************************************************************************
void CloudHandleError(int32_t i32Ret, tCloudState *pui32State)
{
    char * pcDebug;

    g_sDebug.ui32Request = Cmd_Prompt_Print;
    pcDebug = g_sDebug.pcBuf;

    //
    // Check if error handling is needed.
    //
    if(i32Ret <= 0)
    {
        //
        // No - Then retun without doing anything.
        //
        return;
    }

    //
    // Check if we got a 409 (Conflict) or 404 (Not Found) error.
    //
    if((i32Ret == HTTPStd_CONFLICT) || (i32Ret == HTTPStd_NOT_FOUND))
    {
        //
        // Yes - Print the reason for this message and try after 10 secs to get
        // a new CIK.
        //
        snprintf(pcDebug, TX_BUF_SIZE, "CloudError: Server sent %s error."
                 "\n    Check if board is added to the Exosite server.\n",
                 ((i32Ret == HTTPStd_CONFLICT) ? "409" : "404"));
        Mailbox_post(CloudMailbox, &g_sDebug, BIOS_NO_WAIT);
        System_printf(pcDebug);

        snprintf(pcDebug, TX_BUF_SIZE, "    Re-enable the device for "
                 "provisioning on Exosite server.\n    Retrying in 10 secs..."
                 "\n");
        Mailbox_post(CloudMailbox, &g_sDebug, BIOS_NO_WAIT);
        System_printf(pcDebug);

        //
        // Sleep for 10 secs and set flag to request a new CIK.
        //
        Task_sleep(10000);
        *pui32State = Cloud_Activate_CIK;
    }

    //
    // Check if we got a 401 Not Found error.
    //
    else if(i32Ret == HTTPStd_UNAUTHORIZED)
    {
        //
        // No or invalid CIK used during communication with server.
        // Hence should try to get a valid CIK from server.
        //
        memset(g_pcExositeCIK, 0, EXOSITE_CIK_LENGTH + 1);
        *pui32State = Cloud_Activate_CIK;
        snprintf(pcDebug, TX_BUF_SIZE, "CloudError: Server sent 401 error."
                 "\n    Invalid CIK used. Trying to acquire a new CIK.\n");
        Mailbox_post(CloudMailbox, &g_sDebug, BIOS_NO_WAIT);
        System_printf(pcDebug);
    }

    //
    // We got some unknown error from server.
    //
    else
    {
        //
        // Report error and try again.
        //
        snprintf(pcDebug, TX_BUF_SIZE, "CloudError: Server returned : %d "
                 "during : %d action. Retrying\n", i32Ret, *pui32State);
        Mailbox_post(CloudMailbox, &g_sDebug, BIOS_NO_WAIT);
        System_printf(pcDebug);
    }
}

//*****************************************************************************
//
// Get CIK from Exosite.  The following headers and request boady
// are sent by this function.
//
// POST /provision/activate HTTP/1.1
// Host: m2.exosite.com
// Content-Type: application/x-www-form-urlencoded; charset=utf-8
// Content-Length: <length>
//
// <alias 1>=<value 1>
//
//*****************************************************************************
int32_t
ExositeActivate(HTTPCli_Handle cli)
{
    char pcExositeProvBuf[EXOSITE_LENGTH];
    char pcLen[4];
    bool bMoreFlag;
    int32_t i32Ret = 0;
    uint32_t ui32Status = 0;

    //
    // Assemble the provisioning information.
    //
    if(BuildProvInfo("texasinstruments", "ek-tm4c129exl", g_pcMACAddress,
                     pcExositeProvBuf, EXOSITE_LENGTH) != 0)
    {
        //
        // Return error.
        //
        return (-2);
    }

    //
    // Compute length of the Provision request body.
    //
    snprintf(pcLen, sizeof(pcLen), "%d", strlen(pcExositeProvBuf));

    //
    // Make HTTP 1.1 POST request.  The following headers are automatically
    // sent with the POST request.
    //
    // POST /provision/activate HTTP/1.1
    // Host: m2.exosite.com
    //
    i32Ret = HTTPCli_sendRequest(cli, HTTPStd_POST, PROVISION_URI, true);
    if(i32Ret < 0)
    {
        return (i32Ret);
    }

    //
    // Send content type header.
    //
    // Content-type: <type>
    // <blank line>
    //
    i32Ret = HTTPCli_sendField(cli, HTTPStd_FIELD_NAME_CONTENT_TYPE,
                            EXOSITE_CONTENT_TYPE, false);
    if(i32Ret < 0)
    {
        return (i32Ret);
    }

    //
    // Send content length header.
    //
    // Content-Length: <length>
    // <blank line>
    //
    i32Ret = HTTPCli_sendField(cli, HTTPStd_FIELD_NAME_CONTENT_LENGTH, pcLen,
                            true);
    if(i32Ret < 0)
    {
        return (i32Ret);
    }

    //
    // Send the request body.
    //
    // <alias 1>=<value 1>&<alias 2...>=<value 2...>&<alias n>=<value n>
    //
    i32Ret = HTTPCli_sendRequestBody(cli, pcExositeProvBuf,
                                  strlen(pcExositeProvBuf));
    if(i32Ret < 0)
    {
        return (i32Ret);
    }

    //
    // Get the Exosite server's response status.
    //
    i32Ret = HTTPCli_getResponseStatus(cli);
    if(i32Ret < 0)
    {
        return (i32Ret);
    }
    ui32Status = (uint32_t)(i32Ret);

    //
    // Flush the remaining headers.  No field headers set using
    // HTTPCli_setResponseFieldId(), so a call to
    // HTTPCli_getResponseField() should drop all headers and return
    // HTTPCli_FIELD_ID_END.
    //
    i32Ret = HTTPCli_getResponseField(cli, pcExositeProvBuf,
                                   sizeof(pcExositeProvBuf), &bMoreFlag);
    if((i32Ret != HTTPCli_FIELD_ID_END) && (i32Ret != HTTPCli_FIELD_ID_DUMMY))
    {
        return (i32Ret);
    }

    //
    // Did Exosite respond with a status other than HTTP_NO_CONTENT?
    //
    if(ui32Status != HTTPStd_NO_CONTENT)
    {
        //
        // Yes - Then extract the Response body into a buffer.
        //
        do
        {
            i32Ret = HTTPCli_readResponseBody(cli, pcExositeProvBuf,
                                              sizeof(pcExositeProvBuf),
                                              &bMoreFlag);
            if(i32Ret < 0)
            {
                return (i32Ret);
            }
            else if(i32Ret)
            {
                if(i32Ret < sizeof(pcExositeProvBuf))
                {
                    pcExositeProvBuf[i32Ret] = '\0';
                }
            }
        } while (bMoreFlag);
    }

    //
    // Did Exosite respond with a status other than HTTP_OK?
    //
    if(ui32Status != HTTPStd_OK)
    {
        //
        // Yes - Return with the response status.
        //
        return (ui32Status);
    }

    //
    // The response body is the CIK.  Save it in EEPROM for future use.
    //
    SaveCIKEEPROM(pcExositeProvBuf);

    //
    // Read back the CIK from EEPROM into global resource.
    //
    if(GetCIKEEPROM(g_pcExositeCIK) != true)
    {
        //
        // Error reading back CIK.  Return the error message.
        //
        return(-3);
    }

    //
    // Compare the value read back from EEPROM with the value received from
    // Exosite.
    //
    if(strncmp(g_pcExositeCIK, pcExositeProvBuf, EXOSITE_CIK_LENGTH) != 0)
    {
        //
        // Values don't match.  Return the error message.
        //
        return(-4);
    }

    //
    // Print CIK value.
    //
    g_sDebug.ui32Request = Cmd_Prompt_Print;
    snprintf(g_sDebug.pcBuf, TX_BUF_SIZE, "CIK acquired: %s\r\n",
             g_pcExositeCIK);
    Mailbox_post(CloudMailbox, &g_sDebug, BIOS_NO_WAIT);
    System_printf(g_sDebug.pcBuf);

    //
    // No error encountered.  Return success.
    //
    return(0);
}

//*****************************************************************************
//
// Writes (or POSTs) data to Exosite.  The following headers and request boady
// are sent by this function.
//
// POST /onep:v1/stack/alias HTTP/1.1
// Host: m2.exosite.com
// X-Exosite-CIK: <CIK>
// Content-Type: application/x-www-form-urlencoded; charset=utf-8
// Content-Length: <length>
//
// <alias 1>=<value 1>&<alias 2...>=<value 2...>&<alias n>=<value n>
//
//*****************************************************************************
int32_t
ExositeWrite(HTTPCli_Handle cli)
{
    int32_t i32Ret = 0;
    uint32_t ui32Status = 0;
    char pcDataBuf[128];
    char pcLen[8];
    bool bMoreFlag;

    //
    // Make sure that CIK is filled before proceeding.
    //
    if(g_pcExositeCIK[0] == '\0')
    {
        //
        // CIK is not populated.  Return this error.
        //
        return -1;
    }

    //
    // Fill-up the request body and the content length.
    //
    GetRequestBody(pcDataBuf, sizeof(pcDataBuf));
    snprintf(pcLen, sizeof(pcLen), "%d", strlen(pcDataBuf));

    //
    // Make HTTP 1.1 POST request.  The following headers are automatically
    // sent with the POST request.
    //
    // POST /onep:v1/stack/alias HTTP/1.1
    // Host: m2.exosite.com
    //
    i32Ret = HTTPCli_sendRequest(cli, HTTPStd_POST, EXOSITE_URI, true);
    if(i32Ret < 0)
    {
        return (i32Ret);
    }

    //
    // Send X-Exosite-CIK header
    //
    // X-Exosite-CIK: <CIK>
    // <blank line>
    //
    i32Ret = HTTPCli_sendField(cli, EXOSITE_TYPE, g_pcExositeCIK, false);
    if(i32Ret < 0)
    {
        return (i32Ret);
    }

    //
    // Send content type header
    //
    // Content-Type: application/x-www-form-urlencoded; charset=utf-8
    // <blank line>
    //
    i32Ret = HTTPCli_sendField(cli, HTTPStd_FIELD_NAME_CONTENT_TYPE,
                            EXOSITE_CONTENT_TYPE, false);
    if(i32Ret < 0)
    {
        return (i32Ret);
    }

    //
    // Send content length header
    //
    // Content-Length: <length>
    // <blank line>
    //
    i32Ret = HTTPCli_sendField(cli, HTTPStd_FIELD_NAME_CONTENT_LENGTH, pcLen,
                            true);
    if(i32Ret < 0)
    {
        return (i32Ret);
    }

    //
    // Send the request body.
    //
    // <alias 1>=<value 1>&<alias 2...>=<value 2...>&<alias n>=<value n>
    //
    i32Ret = HTTPCli_sendRequestBody(cli, pcDataBuf, strlen(pcDataBuf));
    if(i32Ret < 0)
    {
        return (i32Ret);
    }

    //
    // Get the response status and back it up.
    //
    i32Ret = HTTPCli_getResponseStatus(cli);
    if(i32Ret < 0)
    {
        return (i32Ret);
    }
    ui32Status = (uint32_t)(i32Ret);

    //
    // Flush the remaining headers.  No field headers set using
    // HTTPCli_setResponseFieldId(), so a call to
    // HTTPCli_getResponseField() should drop all headers and return
    // HTTPCli_FIELD_ID_END or HTTPCli_FIELD_ID_DUMMY.
    //
    // Date: <date>
    // Server: <server>
    // Connection: Close
    // Content-Length: 0
    // <blank line>
    //
    i32Ret = HTTPCli_getResponseField(cli, pcDataBuf, sizeof(pcDataBuf),
                                      &bMoreFlag);
    if((i32Ret != HTTPCli_FIELD_ID_END) && (i32Ret != HTTPCli_FIELD_ID_DUMMY))
    {
        return (i32Ret);
    }

    //
    // Did Exosite respond with an undesired status?
    //
    if(ui32Status != HTTPStd_NO_CONTENT)
    {
        //
        // Yes - Now extract the Response body into a buffer.  Even though we
        // don't need this response body, we need to clean up for next HTTP
        // request.
        //
        do
        {
            i32Ret = HTTPCli_readResponseBody(cli, pcDataBuf,
                                              sizeof(pcDataBuf), &bMoreFlag);
            if(i32Ret < 0)
            {
                return (i32Ret);
            }
            else if(i32Ret)
            {
                if(i32Ret < sizeof(pcDataBuf))
                {
                    pcDataBuf[i32Ret] = '\0';
                }
            }
        } while (bMoreFlag);

        //
        // Yes - Return the response status.
        //
        return (ui32Status);
    }

    //
    // Received the desired response status from server.  Hence return 0.
    //
    return(0);
}

//*****************************************************************************
//
// Reads (or GETs) data from Exosite.  The following headers are sent by this
// function.
//
// GET /onep:v1/stack/alias?ledd1&ledd2&location HTTP/1.1
// Host: m2.exosite.com
// X-Exosite-CIK: <CIK>
// Accept: application/x-www-form-urlencoded; charset=utf-8
//
//*****************************************************************************
int32_t
ExositeRead(HTTPCli_Handle cli)
{
    int32_t i32Ret = 0;
    uint32_t ui32Status = 0;
    uint32_t ui32BufLen = 0;
    char pcRecBuf[128];
    bool bMoreFlag;

    //
    // Make sure that CIK is filled before proceeding.
    //
    if(g_pcExositeCIK[0] == '\0')
    {
        //
        // CIK is not populated.  Return this error.
        //
        return -1;
    }

    //
    // Copy Exosite URI into a buffer.
    //
    ui32BufLen = snprintf(pcRecBuf, sizeof(pcRecBuf), EXOSITE_URI);

    //
    // Get the alias list whose values we need from the cloud server.
    //
    GetAliasList((pcRecBuf + ui32BufLen), (sizeof(pcRecBuf) - ui32BufLen));

    //
    // Make HTTP 1.1 GET request.  The following headers are automatically
    // sent with the GET request.
    //
    // GET /onep:v1/stack/alias?ledd1&location&gamestate&emailaddr HTTP/1.1
    // Host: m2.exosite.com
    //
    i32Ret = HTTPCli_sendRequest(cli, HTTPStd_GET, pcRecBuf, true);
    if(i32Ret < 0)
    {
        return (i32Ret);
    }

    //
    // Send X-Exosite-CIK header
    //
    // X-Exosite-CIK: <CIK>
    // <blank line>
    //
    i32Ret = HTTPCli_sendField(cli, EXOSITE_TYPE, g_pcExositeCIK, false);
    if(i32Ret < 0)
    {
        return (i32Ret);
    }

    //
    // Send accept header
    //
    // Accept: <type>
    // <blank line>
    //
    i32Ret = HTTPCli_sendField(cli, HTTPStd_FIELD_NAME_ACCEPT,
                               EXOSITE_CONTENT_TYPE, true);
    if(i32Ret < 0)
    {
        return (i32Ret);
    }

    //
    // Get the response status and back it up.
    //
    i32Ret = HTTPCli_getResponseStatus(cli);
    if(i32Ret < 0)
    {
        return (i32Ret);
    }
    ui32Status = i32Ret;

    //
    // Flush the remaining headers.  No field headers set using
    // HTTPCli_setResponseFieldId(), so a call to
    // HTTPCli_getResponseField() should drop all headers and return
    // HTTPCli_FIELD_ID_END or HTTPCli_FIELD_ID_DUMMY.
    //
    i32Ret = HTTPCli_getResponseField(cli, pcRecBuf, sizeof(pcRecBuf),
                                      &bMoreFlag);
    if((i32Ret != HTTPCli_FIELD_ID_END) && (i32Ret != HTTPCli_FIELD_ID_DUMMY))
    {
        return (i32Ret);
    }

    //
    // Did Exosite respond with an undesired response?
    //
    if(ui32Status != HTTPStd_OK)
    {
        //
        // Yes - Return with the response status.
        //
        return (ui32Status);
    }

    //
    // Extract the Response body into a buffer.
    //
    do
    {
        i32Ret = HTTPCli_readResponseBody(cli, pcRecBuf, sizeof(pcRecBuf),
                                          &bMoreFlag);
        if(i32Ret < 0)
        {
            return (i32Ret);
        }
        else if(i32Ret)
        {
            if(i32Ret < sizeof(pcRecBuf))
            {
                pcRecBuf[i32Ret] = '\0';
            }
        }
    } while (bMoreFlag);

    //
    // Parse the response body and perform necessary actions based on the
    // content.
    //
    ProcessResponseBody(pcRecBuf);

    //
    // Received the desired response status from server.  Hence return 0.
    //
    return(0);
}

//*****************************************************************************
//
// This task is the main task that runs the interface to cloud for this app.
// It also manages the LEDs and Buttons for the board.
//
//*****************************************************************************
void CloudTask(unsigned int arg0, unsigned int arg1)
{
    int32_t i32Ret = 0;
    int32_t i32SocError = 0;
    HTTPCli_Struct cli;
    WOLFSSL_CTX *ctx;
    tMailboxMsg sCommandRequest;
    bool bStatus;
    char * pcDebug;
    uint32_t ui32LED2 = Board_LED_OFF;

    g_sDebug.ui32Request = Cmd_Prompt_Print;
    pcDebug = g_sDebug.pcBuf;

    //
    // Print IP address.
    //
    snprintf(pcDebug, TX_BUF_SIZE, "IP Address acquired: %d.%d.%d.%d\n",
             (g_ui32IPAddr & 0xFF), ((g_ui32IPAddr >> 8) & 0xFF),
             ((g_ui32IPAddr >> 16) & 0xFF), ((g_ui32IPAddr >> 24) & 0xFF));
    Mailbox_post(CloudMailbox, &g_sDebug, BIOS_NO_WAIT);

    //
    // Synchronize system time with an NTP server.  Current time is needed to
    // verify server's SSL certificate.
    do
    {
        i32Ret = SyncNTPServer();
    }while(i32Ret != 0);

    //
    // Setup the WolfSSL parameters
    //
    wolfSSL_Init();

    //
    // Create new WolfSSL instance.
    //
    ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());
    if(ctx == NULL)
    {
        //
        // Report error and exit application.
        //
        //System_printf("CloudTask: SSL_CTX_new error.\n");
        snprintf(pcDebug, TX_BUF_SIZE, "CloudTask: SSL_CTX_new error. "
                 "Exiting.\n");
        Mailbox_post(CloudMailbox, &g_sDebug, BIOS_NO_WAIT);

        //
        // Sleep a few moments to allow the command task to print the message
        // before exiting.
        //
        Task_sleep(100);

        System_printf(pcDebug);
        BIOS_exit(1);
    }

    //
    // Load the server certificate.  This certificate is used during handshake
    // process to validate server credentials.
    //
    if(wolfSSL_CTX_load_verify_buffer(ctx, ca_cert, sizeof_ca_cert,
                                      SSL_FILETYPE_ASN1) != SSL_SUCCESS)
    {
        //System_printf("CloudTask: Error loading ca_cert_der_2048\n");
        //
        // Report error and exit application.
        //
        snprintf(pcDebug, TX_BUF_SIZE, "CloudTask: Error loading "
                 "ca_cert_der_2048. Exiting.\n");
        Mailbox_post(CloudMailbox, &g_sDebug, BIOS_NO_WAIT);

        //
        // Sleep a few moments to allow the command task to print the message
        // before exiting.
        //
        Task_sleep(100);

        System_printf(pcDebug);
        BIOS_exit(1);
    }

    //
    // Set-up the secure communication parameters.
    //
    SSWolfssl_setContext(ctx);

    //
    // Set state machine flag to try connecting to the cloud server.
    //
    g_ui32State = Cloud_Server_Connect;

    while (1)
    {
        //
        // Check if we received any notification from the Command task.
        //
        bStatus = Mailbox_pend(CmdMailbox, &sCommandRequest, BIOS_NO_WAIT);
        if(bStatus)
        {
            g_ui32State = (tCloudState) (sCommandRequest.ui32Request);
        }

        //
        // Update the different board resources.
        //
        UpdateCloudData();

        switch(g_ui32State)
        {
            case Cloud_Server_Connect:
            {
                if(g_bServerConnect == true)
                {
                    //
                    // If already connected, disconnect before trying to
                    // reconnect.
                    //
                    ServerDisconnect(&cli);
                }

                //
                // Create a secure socket and try to connect with the cloud
                // server.
                //
                if(ServerConnect(&cli) != 0)
                {
                    //
                    // Unsuccesfull in connecting to the server.  Jump to Idle
                    // state and wait for user command.
                    //
                    g_ui32State = Cloud_Idle;
                }
                else
                {
                    //
                    // Success.  Set the global resource to indicate connection
                    // to cloud server.
                    //
                    g_bServerConnect = true;

                    //
                    // See if a valid CIK exists in NVM.
                    //
                    if(GetCIK(&cli) == true)
                    {
                        //
                        // Yes - Try to send information to the server with
                        // this CIK.
                        //
                        g_ui32State = Cloud_Sync;
                    }
                    else
                    {
                        //
                        // No - Get a new CIK.
                        //
                        g_ui32State = Cloud_Activate_CIK;
                    }
                }

                break;
            }

            case Cloud_Activate_CIK:
            {
                //
                // If we are not yet connected to the cloud server, break and
                // connect to server first.
                //
                if(g_bServerConnect != true)
                {
                    g_ui32State = Cloud_Server_Connect;
                    break;
                }

                //
                // We don't have a valid CIK.  We would have tried to connect
                // to server with an invalid CIK, so reconnect to the server
                // before trying to request a new CIK.
                //
                ServerReconnect(&cli);

                //
                // Request a new CIK.
                //
                i32Ret = ExositeActivate(&cli);
                if(i32Ret == 0)
                {
                    //
                    // We acquired new CIK so communicate with the server.
                    //
                    g_ui32State = Cloud_Sync;
                }

                break;
            }

            case Cloud_Sync:
            {
                //
                // Gather and send relevant data to Exosite server.
                //
                i32Ret = ExositeWrite(&cli);
                if(i32Ret != 0)
                {
                    //
                    // We got an error, so break to handle error.
                    //
                    break;
                }

                //
                // Read data from Exosite server and process it.
                //
                i32Ret = ExositeRead(&cli);
                if(i32Ret != 0)
                {
                    //
                    // We got an error, so break to handle error.
                    //
                    break;
                }

                //
                // Blink LED to indicate that communication is occuring in
                // SSL/TLS mode.
                //
                ui32LED2 ^= Board_LED_ON;
                GPIO_write(Board_LED1, ui32LED2);

                //
                // We were successful in communicating with cloud server.
                // Continue to do this.
                //
                g_ui32State = Cloud_Sync;
                break;
            }

            case Cloud_Proxy_Set:
            {
                //
                // Set state machine state to connect to server and update IP
                // address and proxy state
                //
                g_ui32State = Cloud_Server_Connect;
                CloudProxySet(&cli, sCommandRequest.pcBuf);
                break;
            }

            case Cloud_Idle:
            {
                //
                // Clear errors and do nothing.
                //
                i32Ret = 0;
                break;
            }

            default:
            {
                //
                // This case should never occur.  Send debug message.
                //
                snprintf(pcDebug, TX_BUF_SIZE, "CloudTask: Default case "
                         "should never occur. Ecode: %d.\n    Retrying "
                         "connection with server.\n", i32Ret);
                Mailbox_post(CloudMailbox, &g_sDebug, BIOS_NO_WAIT);
                System_printf(pcDebug);

                //
                // Set the state variable to retry connection.
                //
                g_ui32State = Cloud_Server_Connect;
                i32Ret = 0;
                break;
            }
        }

        //
        // Check if we got a -ve error.
        //
        if(i32Ret < 0)
        {
            //
            // This means the HTTPCli module cannot be recovered.  Report the
            // error and reset cloud connection to try again.
            //
            if((i32Ret < -100) && (i32Ret > -106))
            {
                //
                // For errors in the range -100 to -106, get the socket level
                // error message and print it along with HTTPCli error.
                //
                i32SocError = HTTPCli_getSocketError(&cli);
                snprintf(pcDebug, TX_BUF_SIZE, "CloudTask: Bad response,"
                         " ecode: %d,  socket error: %d during : %d action.\n"
                         "    Resetting connection.\n", i32Ret, i32SocError,
                         g_ui32State);
            }
            else
            {
                snprintf(pcDebug, TX_BUF_SIZE, "CloudTask: Bad response,"
                         " ecode: %d during : %d action.\n    Resetting"
                         " connection.\n", i32Ret, g_ui32State);
            }
            Mailbox_post(CloudMailbox, &g_sDebug, BIOS_NO_WAIT);
            System_printf(pcDebug);

            //
            // Set state variable to reconnect to cloud server.
            //
            g_ui32State = Cloud_Server_Connect;
        }

        //
        // Handle all other errors.
        //
        else if(i32Ret > 0)
        {
            CloudHandleError(i32Ret, &g_ui32State);
        }

        //
        // Wait a second before communciating with Exosite server again.
        //
        Task_sleep(1000);
    }
}

//*****************************************************************************
//
// Initialize the CloudTask which manages communication with the cloud server.
//
//*****************************************************************************
int32_t CloudTaskInit()
{
    Task_Handle psCloudHandle;
    Task_Params sCloudTaskParams;
    Error_Block sEB;

    Error_init(&sEB);

    Task_Params_init(&sCloudTaskParams);
    sCloudTaskParams.stackSize = STACK_CLOUD_TASK;
    sCloudTaskParams.priority = PRIORITY_CLOUD_TASK;
    psCloudHandle = Task_create((Task_FuncPtr)CloudTask, &sCloudTaskParams,
                                &sEB);
    if(psCloudHandle == NULL)
    {
        return -1;
    }

    return 0;
}

