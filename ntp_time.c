//*****************************************************************************
//
// ntp_time.c - Utilities to synchronize time with NTP server.
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

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ti/ndk/nettools/sntp/sntp.h>
#include <ti/net/network.h>
#include <ti/net/http/httpcli.h>
#include <ti/net/http/httpstd.h>
#include <ti/net/http/ssock.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/hal/Seconds.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Task.h>
#include <xdc/cfg/global.h>
#include <xdc/runtime/System.h>
#include "cloud_task.h"
#include "command_task.h"
#include "ntp_time.h"

//*****************************************************************************
//
// Number of seconds to wait for sync with NTP server.
//
//***************************************************************************
#define NTP_TIMEOUT             10

//*****************************************************************************
//
// Semaphore used to synchronize events between StartNTPServer() and the
// call back function TimeUpdateHook().
//
//*****************************************************************************
static Semaphore_Handle g_sSemHandle = NULL;

//*****************************************************************************
//
// Global resource to hold NTP connection states.
//
//*****************************************************************************
uint32_t g_ui32NTPState = NTP_Init;

extern tMailboxMsg g_sDebug;

//*****************************************************************************
//
// A callback function that will be called by SNTP module upon successful time
// synchronization with an NTP server.  This function will be called after a
// call to SNTP_forceTimeSync() API.
//
//*****************************************************************************
void TimeUpdateHook(void *p)
{
   Semaphore_post(g_sSemHandle);
}

//*****************************************************************************
//
// This function connects with the NTP time server to get the current time.
// The current time is needed by WolfSSL for certificate verification.  This
// function is called from the cloud task context.  It creates another task
// that tries to synchronize time with NTP server once every 30 minutes.  But
// here the sntp module is stopped immediately after time is synced once with
// NTP server.
//
//*****************************************************************************
int32_t
StartNTPServer(struct in_addr sNTPIP)
{
    struct sockaddr_in ntpAddr;
    time_t sTime;
    Semaphore_Params sSemParams;
    uint32_t ui32Idx;
    char * pcDebug;

    g_sDebug.ui32Request = Cmd_Prompt_Print;
    pcDebug = g_sDebug.pcBuf;

    //
    // Inform user about the action about to be performed.
    //
    g_sDebug.ui32Request = Cmd_Prompt_No_Print;
    snprintf(pcDebug, TX_BUF_SIZE, "Connecting to NTP server");
    Mailbox_post(CloudMailbox, &g_sDebug, BIOS_NO_WAIT);
    System_printf(pcDebug);
    g_sDebug.ui32Request = Cmd_Prompt_Print;

    //
    // Create a socket to connect with NTP server.
    //
    ntpAddr.sin_family = AF_INET;
    ntpAddr.sin_port = htons(NTP_SERVER_PORT);
    ntpAddr.sin_addr = sNTPIP;

    //
    // Start the sntp modulue by passing the necessary parameters to set and
    // get seconds since epoch.  Also pass a call back function to inform when
    // synced with NTP server.
    //
    if (!SNTP_start(Seconds_get, Seconds_set, TimeUpdateHook,
                    (struct sockaddr *)&ntpAddr, 1, 0))
    {
        //
        // We got a error.  Clean up the sntp module and report error.
        //
        SNTP_stop();

        snprintf(pcDebug, TX_BUF_SIZE, "Failed to start SNTP module.\n");
        Mailbox_post(CloudMailbox, &g_sDebug, BIOS_NO_WAIT);
        System_printf(pcDebug);
        return -1;
    }

    //
    // Create a binary semaphore that indicates the syncing with NTP server.
    //
    Semaphore_Params_init(&sSemParams);
    sSemParams.mode = Semaphore_Mode_BINARY;
    g_sSemHandle = Semaphore_create(0, &sSemParams, NULL);
    if (g_sSemHandle == NULL)
    {
        //
        // We got a error.  Clean up the sntp module.
        //
        SNTP_stop();

        //
        // Report error and exit application.
        //
        snprintf(pcDebug, TX_BUF_SIZE, "NTP_Time: Failed: to create semaphore."
                 "\n");
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
    // Force sync with NTP server.  Ensure that SNTP_forceTimeSync() is not
    // called more than once in any 15 second time period.
    //
    SNTP_forceTimeSync();

    //
    // Wait for a few seconds for semaphore to be posted by the call back
    // function indicating a sync with NTP server.  To improve responsiveness,
    // pend for 1 second at a time, then check if we were successful.
    //
    for(ui32Idx = 0; ui32Idx < NTP_TIMEOUT; ui32Idx++)
    {
        //
        // If semaphore was posted by the call back function, break from the
        // loop.
        //
        if(Semaphore_pend(g_sSemHandle, 1000) == true)
        {
            break;
        }

        //
        // Show progress to user.
        //
        g_sDebug.ui32Request = Cmd_Prompt_No_Erase;
        snprintf(pcDebug, TX_BUF_SIZE, ".");
        Mailbox_post(CloudMailbox, &g_sDebug, BIOS_NO_WAIT);
        System_printf(pcDebug);
        g_sDebug.ui32Request = Cmd_Prompt_Print;
    }

    //
    // If we timed out, clean up and return with error.
    //
    if(ui32Idx >= NTP_TIMEOUT)
    {
        snprintf(pcDebug, TX_BUF_SIZE, "\rFailed to Sync time with NTP server "
                "after %d seconds\n", ui32Idx);
        Mailbox_post(CloudMailbox, &g_sDebug, BIOS_NO_WAIT);
        System_printf(pcDebug);

        //
        // Stop SNTP module.
        //
        SNTP_stop();
        return 1;
    }

    //
    // Time successfully synchronized.  Get time and print it.
    //
    sTime = time(NULL);
    snprintf(pcDebug, TX_BUF_SIZE, "\rCurrent Date/Time is %s\n",
             ctime(&sTime));
    Mailbox_post(CloudMailbox, &g_sDebug, BIOS_NO_WAIT);
    System_printf(pcDebug);

    //
    // To avoid a bug in the SNTP module that causes a fault after 30 minutes,
    // when the SNTP task tries to re-sync with the NTP server, stop the SNTP
    // module for now.
    //
    SNTP_stop();

    return 0;
}

//*****************************************************************************
//
// Resolve the IP address of the URL provided.
//
//*****************************************************************************
int32_t
ResolveNTPURL(struct sockaddr * sNTPSockAddr, char * pcNTPServer)
{
    int32_t i32Ret;;

    //
    // Set-up a socket to resolve NTP server's IP.
    //
    i32Ret = HTTPCli_initSockAddr(sNTPSockAddr, pcNTPServer, 0);
    if(i32Ret != 0)
    {
        //
        // Failed to Resolve IP.  Report error.
        //
        return -1;
    }

    return 0;
}

//*****************************************************************************
//
// This function ensures that the system time is synchronized with an NTP
// server.  Initially this function tries to resolve the default NTP server URL
// followed by connecting to the NTP server to get the current time and date.
// If either of the above steps fail, user is prompted to enter NTP server's
// URL or IP address before retyring to connect again.  This function will
// return success only on synchronizing the system time with NTP server.
//
//*****************************************************************************
int32_t
SyncNTPServer(void)
{
    struct sockaddr_in sNTPSockAddr;
    char pcNTPServer[128];
    int32_t i32Ret;
    tMailboxMsg sNTPCmdVal;
    uint32_t ui32IPAddr;
    char * pcDebug;

    g_sDebug.ui32Request = Cmd_Prompt_Print;
    pcDebug = g_sDebug.pcBuf;

    switch(g_ui32NTPState)
    {
        case NTP_Init:
        {
            //
            // Copy the default NTP server URL into local buffer and set state
            // variable to resolve the URL's IP address.
            //
            strncpy(pcNTPServer, NTP_SERVER_URL, sizeof(pcNTPServer));
            g_ui32NTPState = NTP_Resolve_URL;
        }

        case NTP_Resolve_URL:
        {
            //
            // Inform Command task that we are resolving the IP address of an
            // NTP server URL.
            //
            g_sDebug.ui32Request = Cmd_Prompt_No_Print;
            snprintf(pcDebug, TX_BUF_SIZE, "Resolving IP address of %s...",
                     pcNTPServer);
            Mailbox_post(CloudMailbox, &g_sDebug, 100);
            System_printf(pcDebug);
            g_sDebug.ui32Request = Cmd_Prompt_Print;

            //
            // Resolve the IP address of the NTP server.
            //
            if(ResolveNTPURL((struct sockaddr *)&sNTPSockAddr, pcNTPServer) ==
               0)
            {
                //
                // Success.  Inform the user the resolved IP and set state
                // variable to connect with the NTP server.
                //
                ui32IPAddr = (uint32_t) (sNTPSockAddr.sin_addr.s_addr);
                snprintf(pcDebug, TX_BUF_SIZE, "\rNTP IP resolved to %d.%d.%d."
                         "%d\n", (ui32IPAddr & 0xFF),
                         ((ui32IPAddr >> 8) & 0xFF),
                         ((ui32IPAddr >> 16) & 0xFF),
                         ((ui32IPAddr >> 24) & 0xFF));
                Mailbox_post(CloudMailbox, &g_sDebug, 100);
                System_printf(pcDebug);

                g_ui32NTPState = NTP_Connect;
            }
            else
            {
                //
                // Failure.  Inform the user about failure and set state
                // variable to prompt user to enter NTP server details.
                //
                snprintf(pcDebug, TX_BUF_SIZE, "\rFailed to resolve IP "
                         "address of %s.\n", pcNTPServer);
                Mailbox_post(CloudMailbox, &g_sDebug, 100);
                System_printf(pcDebug);

                g_ui32NTPState = NTP_Prompt_User;
            }
            break;
        }

        case NTP_Prompt_User:
        {
            //
            // Prompt user to enter an NTP server URL or IP address.
            //
            snprintf(pcDebug, TX_BUF_SIZE, "Provide correct NTP address to "
                     "proceed.\n  Command: ntp <IP>\n    <IP> can be in the "
                     "form \"time.nist.gov\" or \"192.168.1.1\"\n");
            Mailbox_post(CloudMailbox, &g_sDebug, BIOS_NO_WAIT);

            //
            // Wait for ever till user provides correct NTP server details.
            //
            Mailbox_pend(CmdMailbox, &sNTPCmdVal, BIOS_WAIT_FOREVER);

            //
            // Set the state variable based on user input.
            //
            g_ui32NTPState = sNTPCmdVal.ui32Request;

            //
            // Set the necessary variables before jumping to the new state.
            //
            if(sNTPCmdVal.ui32Request == NTP_Connect)
            {
                //
                // If user provided an IP address, set the necessary variable
                // before jumping to "NTP_Connect" state.
                //
                i32Ret = inet_pton(AF_INET, sNTPCmdVal.pcBuf,
                                   &sNTPSockAddr.sin_addr);
            }
            else if(sNTPCmdVal.ui32Request == NTP_Resolve_URL)
            {
                //
                // If user provided an URL, set the necessary buffer before
                // jumping to "NTP_Resolve_URL" state.
                //
                strncpy(pcNTPServer, sNTPCmdVal.pcBuf, sizeof(pcNTPServer));
            }
            else
            {
                //
                // Error.  Should never get here.  Prompt user to try again.
                //
                g_ui32NTPState = NTP_Prompt_User;
            }

            break;
        }

        case NTP_Connect:
        {
            i32Ret = StartNTPServer(sNTPSockAddr.sin_addr);
            if(i32Ret < 0)
            {
                //
                // Failed to start the sntp module.  Retry.
                //
                g_ui32NTPState = NTP_Init;
            }
            else if(i32Ret > 0)
            {
                //
                // Timed out syncing with the NTP server.  Prompt user to enter
                // a different NTP server.
                //
                g_ui32NTPState = NTP_Prompt_User;
            }
            else
            {
                //
                // Success.
                //
                return 0;
            }
            break;
        }

        case NTP_Idle:
        default:
        {
            //
            // This case should never occur.  Send debug message.
            //
            snprintf(pcDebug, TX_BUF_SIZE, "ntp_time: NTP_Idle or default "
                     "case should never occur.\n");
            Mailbox_post(CloudMailbox, &g_sDebug, BIOS_NO_WAIT);
            System_printf(pcDebug);

            //
            // Set the state variable to retry connection.
            //
            g_ui32NTPState = NTP_Init;
            break;
        }
    }

    //
    // We are still not done.  So return 1.
    //
    return 1;
}
