//*****************************************************************************
//
// board_funcs.c - Functions to configure and manage different peripherals of
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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ti/drivers/GPIO.h>
#include "inc/hw_adc.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/adc.h"
#include "driverlib/eeprom.h"
#include "driverlib/flash.h"
#include "driverlib/sysctl.h"
#include "Board.h"
#include "board_funcs.h"
#include "cloud_task.h"

//*****************************************************************************
//
// Global counters that hold the count of buton presses since reset.
//
//*****************************************************************************
uint32_t g_ui32SW1 = 0;
uint32_t g_ui32SW2 = 0;

//*****************************************************************************
//
// Callback function for the GPIO interrupt on Board_BUTTON0.
//
//*****************************************************************************
void gpioSWFxn1(void)
{
    g_ui32SW1++;
}

//*****************************************************************************
//
// Callback function for the GPIO interrupt on Board_BUTTON10.
//
//*****************************************************************************
void gpioSWFxn2(void)
{
    g_ui32SW2++;
}

//*****************************************************************************
//
// Enables and configures ADC0 to read the internal temperature sensor into
// sample sequencer 3.
//
//*****************************************************************************
void
ConfigureADC0(void)
{
    //
    // Enable clock to ADC0.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

    //
    // Configure ADC0 Sample Sequencer 3 for processor trigger operation.
    //
    ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 0);

    //
    // Increase the hold time of this sample sequencer to account for the
    // temperature sensor erratum (ADC#09).
    //
    HWREG(ADC0_BASE + ADC_O_SSTSH3) = 0x4;

    //
    // Configure ADC0 sequencer 3 for a single sample of the temperature
    // sensor.
    //
    ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_TS | ADC_CTL_IE |
                             ADC_CTL_END);

    //
    // Enable the sequencer.
    //
    ADCSequenceEnable(ADC0_BASE, 3);

    //
    // Clear the interrupt bit for sequencer 3 to make sure it is not set
    // before the first sample is taken.
    //
    ADCIntClear(ADC0_BASE, 3);
}

//*****************************************************************************
//
// This function registers the callback functions to be called when a button is
// pressed and enables the interrupts
//
//*****************************************************************************
void
ConfigureButtons(void)
{
    //
    // Install Button callback.
    //
    GPIO_setCallback(Board_BUTTON0, gpioSWFxn1);
    GPIO_setCallback(Board_BUTTON1, gpioSWFxn2);

    //
    // Enable interrupts.
    //
    GPIO_enableInt(Board_BUTTON0);
    GPIO_enableInt(Board_BUTTON1);
}

//*****************************************************************************
//
// Returns the number of times SW1 and SW2 were pressed.
//
//*****************************************************************************
void
ReadButtons(uint32_t *buttons)
{
    *(buttons) = g_ui32SW1;
    *(++buttons) = g_ui32SW2;
}

//*****************************************************************************
//
// Calculates the internal junction temperature and returns this value.
//
//*****************************************************************************
uint16_t
ReadInternalTemp(void)
{
    uint32_t ui32Temperature;
    uint32_t ui32ADCValue;

    //
    // Take a temperature reading with the ADC.
    //
    ADCProcessorTrigger(ADC0_BASE, 3);

    //
    // Wait for the ADC to finish taking the sample
    //
    while(!ADCIntStatus(ADC0_BASE, 3, false))
    {
    }

    //
    // Clear the interrupt
    //
    ADCIntClear(ADC0_BASE, 3);

    //
    // Read the analog voltage measurement.
    //
    ADCSequenceDataGet(ADC0_BASE, 3, &ui32ADCValue);

    //
    // Convert the measurement to degrees Celcius.
    //
    ui32Temperature = ((1475 * 4096) - (2250 * ui32ADCValue)) / 40960;

    return (ui32Temperature);
}

//*****************************************************************************
//
// Get MAC address of the board from FLASH User Registers.
//
//*****************************************************************************
bool
GetMacAddress(char *pcMACAddress, uint32_t ui32MACLen)
{
    uint32_t ulUser0, ulUser1;

    //
    // Get the MAC address from User Registers.
    //
    FlashUserGet(&ulUser0, &ulUser1);

    //
    // Check if MAC address is present on the board.
    //
    if ((ulUser0 == 0xffffffff) && (ulUser1 == 0xffffffff))
    {
        //
        // If not present, return error.
        //
        return false;
    }

    //
    // Convert the 24/24 split MAC address from NV memory into a 32/16 split
    // MAC address, then convert to string in hex format.
    //
    snprintf(pcMACAddress, ui32MACLen, "%02x%02x%02x%02x%02x%02x",
             (char)((ulUser0 >>  0) & 0xff), (char)((ulUser0 >>  8) & 0xff),
             (char)((ulUser0 >> 16) & 0xff), (char)((ulUser1 >>  0) & 0xff),
             (char)((ulUser1 >>  8) & 0xff), (char)((ulUser1 >> 16) & 0xff));

    //
    // Return success.
    //
    return true;
}

//*****************************************************************************
//
// Initialize the EEPROM peripheral.
//
//*****************************************************************************
void
InitEEPROM(void)
{
    //
    // Enable the EEPROM peripheral.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_EEPROM0);

    //
    // Initialize the EEPROM
    //
    EEPROMInit();
}

//*****************************************************************************
//
// Get/Read the CIK value from EEPROM.
//
//*****************************************************************************
bool
GetCIKEEPROM(char *pcProvBuf)
{
    //
    // Read CIK from the EEPROM.
    //
    EEPROMRead((uint32_t *)pcProvBuf, (uint32_t)(EXOSITE_CIK_OFFSET),
               (uint32_t)EXOSITE_CIK_LENGTH);

    //
    // Add a trailing 0 to enable working with string functions.
    //
    pcProvBuf[EXOSITE_CIK_LENGTH] = '\0';

    //
    // Return Success.
    //
    return true;
}

//*****************************************************************************
//
// Save/Write the CIK value to EEPROM.
//
//*****************************************************************************
bool
SaveCIKEEPROM(char *pcProvBuf)
{
    uint32_t ui32Len;

    //
    // Get the length of the buffer.
    //
    ui32Len = strlen(pcProvBuf);

    //
    // Check if the buffer length is the expected length.
    //
    if(ui32Len != EXOSITE_CIK_LENGTH)
    {
        //
        // Buffer length is not the expected length
        //
        return false;
    }

    //
    // Write CIK to the EEPROM.
    //
    EEPROMProgram((uint32_t *)pcProvBuf, (uint32_t)(EXOSITE_CIK_OFFSET),
                  (uint32_t)ui32Len);

    //
    // Return Success.
    //
    return true;
}

//*****************************************************************************
//
// Erase EEPROM.  This will erase everything including the CIK.
//
//*****************************************************************************
void
EraseEEPROM(void)
{
    //
    // Perform a mass erase on the EEPROM.
    //
    EEPROMMassErase();
}
