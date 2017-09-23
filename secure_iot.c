//*****************************************************************************
//
// secure_iot.c - Example to demonstrate Secure connection to the cloud.
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
#include <ti/sysbios/BIOS.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>
#include "Board.h"
#include "board_funcs.h"
#include "cloud_task.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Secure Internet of Things Example (secure_iot)</h1>
//!
//! This application uses TI-RTOS and WolfSSL library to manage multiple tasks
//! to aggregate data that can be published to a cloud server securely.  This
//! file contains the main function and initializes the necessary peripherals
//! before handing control over to SYS/BIOS kernel of TI-RTOS.
//!
//! The tasks and their responsibilities are as follows:
//!
//! - cloud_task.c is a manager of the cloud interface.  It calls the SNTP
//!   mdoule to sync the time of the real-time clock peripheral with an NTP
//!   server, which is used by the WolfSSL library to validate the server
//!   certificate during HTTPS handshake.  It then connects to the Exosite
//!   server and manages the data transmission and reception to the Exosite
//!   server securely.  The data consists of the board and user activity that
//!   is gathered and built into a packet for transmission to the cloud.  The
//!   data received is handled as needed.  This task is created dynamically
//!   after an IP address is acquired.
//!
//! - command_task.c is a manager of the UART virtual com port connection to a
//!   local PC.  This interface allows advanced commands and data.  To access
//!   the UART0 console use the settings 115200-8-N-1.  Type help on the prompt
//!   for a list of commands.  This task is statically created.  Refer this
//!   projects .cfg file for more details.
//!
//! For additional details on TI-RTOS, refer to the TI-RTOS web page at:
//! http://www.ti.com/tool/ti-rtos
//! For additional details on WolfSSL, refer to the WolfSSL web site at:
//! https://wolfssl.com
//
//*****************************************************************************

//*****************************************************************************
//
//  This function is called by TI-RTOS NDK when IP Addr is added/deleted.
//
//*****************************************************************************
void netIPAddrHook(uint32_t ui32IPAddr, uint32_t ui32IfIdx, uint32_t ui32FAdd)
{
    static bool bFirstTime = false;

    //
    // Update the global IP address resource.
    //
    g_ui32IPAddr = ui32IPAddr;

    if (ui32FAdd && !bFirstTime)
    {
        //
        // Start HTTP Client Task after the network stack is up.
        //
        if(CloudTaskInit() < 0)
        {
            System_printf("netIPAddrHook: Failed to create CloudTask\n");
            BIOS_exit(1);
        }

        bFirstTime = true;
    }
}

//*****************************************************************************
//
// Main function.
//
//*****************************************************************************
int main(int argc, char *argv[])
{
    //
    // Call board init functions.
    //
    Board_initGeneral();
    Board_initEMAC();
    Board_initGPIO();
    Board_initUART();

    //
    // Configure ADC0.
    //
    ConfigureADC0();

    //
    // Configure the buttons
    //
    ConfigureButtons();

    //
    // Initialize EEPROM to store the CIK.
    //
    InitEEPROM();

    //
    // Start the Sys/Bios kernel.
    //
    System_printf("Starting BIOS\n");
    BIOS_start();

    //
    // Should never reach here.
    //
    return (0);
}
