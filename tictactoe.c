//*****************************************************************************
//
// tictactoe.c - Provides TicTacToe game functionality.
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
#include <ti/drivers/UART.h>
#include "command_task.h"
#include "cloud_task.h"
#include "tictactoe.h"

//*****************************************************************************
//
// Definitions related to the representation of the game state.
//
//*****************************************************************************
#define PLAYER_BIT              0x80000000  // Indicates current player number.
#define REMOTE_PLAYER           0x40000000  // Setting allows remote play.

//*****************************************************************************
//
// Information relating to the current TicTacToe game state
//
//*****************************************************************************
uint32_t g_ui32LastState = 0;
uint32_t g_ui32Row = 0;
uint32_t g_ui32Col = 0;
uint32_t g_ui32Player = 0;
uint32_t g_ui32Mode = 0;
extern uint32_t g_ui32BoardState;
extern tReadWriteType g_eBoardStaeRW;

//*****************************************************************************
//
// Resources needed by this module to print to console.
//
//*****************************************************************************
extern UART_Handle g_psUARTHandle;
extern char g_pcTXBuf[TX_BUF_SIZE];

//*****************************************************************************
//
// State variable for keeping track of the game flow
//
//*****************************************************************************
enum
{
    NEW_GAME,
    SET_MODE,
    PLAY_TURN,
    GET_ROW,
    GET_COLUMN,
    REMOTE_PLAY
}
g_ui32GameState;

//*****************************************************************************
//
// Global array to track all possible winning configurations of tic-tac-toe.
//
//*****************************************************************************
uint32_t g_ui32WinConditions[] =
{
    0x7,
    0x38,
    0x1C0,
    0x49,
    0x92,
    0x124,
    0x111,
    0x54
};

#define NUM_WIN_CONDITIONS      (sizeof(g_ui32WinConditions)/sizeof(uint32_t))

//*****************************************************************************
//
// Turn prompts a user to play a single turn of tic-tac-toe, and updates the
// global game state variable accordingly. Turn will prevent collisions between
// two separate players on individual squares of the game board, and will
// re-prompt the same player in the event of invalid input.
//
//*****************************************************************************
bool
ProcessTurn(void)
{
    uint32_t ui32Move;
    uint32_t ui32Len;

    //
    //
    // If the chosen coordinates are out of range, try asking for a new set
    // of coordinates.
    //
    if(g_ui32Row > 2 || g_ui32Col > 2)
    {
        ui32Len = snprintf(g_pcTXBuf, TX_BUF_SIZE, "Invalid, try again.");
        UART_write(g_psUARTHandle, g_pcTXBuf, ui32Len);
        return false;
    }

    //
    // Otherwise, convert the coordinates to the format used by the global
    // state variable.
    //
    ui32Move = 1;
    ui32Move = ui32Move << (g_ui32Row * 3);
    ui32Move = ui32Move << (g_ui32Col * 1);

    //
    // If this space was already occupied, prompt the player for a
    // different move.
    //
    if((ui32Move & g_ui32BoardState) ||
       ((ui32Move << 16) & g_ui32BoardState))
    {
        ui32Len = snprintf(g_pcTXBuf, TX_BUF_SIZE, "Invalid, try again "
                           "(space occupied).\n");
        UART_write(g_psUARTHandle, g_pcTXBuf, ui32Len);
        return false;
    }
    else
    {
        //
        // Otherwise, the move is valid. Add it to the global state.
        //
        g_ui32BoardState |= (ui32Move << (g_ui32Player * 16));

        //
        // Also flip the player bit, to indicate that the next player should
        // move.
        //
        g_ui32BoardState ^= PLAYER_BIT;
        g_ui32Player = (g_ui32BoardState & PLAYER_BIT) ? 1 : 0;

        return true;
    }
}

//*****************************************************************************
//
// ShowBoard prints an ASCII representation of the current tic-tac-toe board to
// the UART
//
//*****************************************************************************
void
ShowBoard(void)
{
    uint32_t ui32RowNum;
    uint32_t ui32ColNum;
    uint32_t ui32MaskX;
    uint32_t ui32MaskO;
    uint32_t ui32Len = 0;

    //
    // Clear the terminal
    //
    ui32Len = snprintf(g_pcTXBuf, TX_BUF_SIZE, "\033[2J\033[H\n");
    UART_write(g_psUARTHandle, g_pcTXBuf, ui32Len);

    ui32Len = snprintf(g_pcTXBuf, TX_BUF_SIZE, "'%c' Player's turn.\n\n",
                       (g_ui32Player ? 'O' : 'X'));
    UART_write(g_psUARTHandle, g_pcTXBuf, ui32Len);

    //
    // Print out column numbers
    //
    ui32Len = snprintf(g_pcTXBuf, TX_BUF_SIZE, "   0 1 2\n");
    UART_write(g_psUARTHandle, g_pcTXBuf, ui32Len);

    //
    // Loop over rows, starting with zero
    //
    for(ui32RowNum = 0; ui32RowNum < 3; ui32RowNum++)
    {
        //
        // Print the row number
        //
        ui32Len = snprintf(g_pcTXBuf, TX_BUF_SIZE, " %d ", ui32RowNum);
        UART_write(g_psUARTHandle, g_pcTXBuf, ui32Len);

        //
        // Loop thorugh the columns
        //
        for(ui32ColNum = 0; ui32ColNum < 3; ui32ColNum++)
        {
            //
            // Convert the row/column number into the format used by the global
            // game-state variable.
            //
            ui32MaskX = 1 << (ui32RowNum * 3);
            ui32MaskX = ui32MaskX << (ui32ColNum * 1);
            ui32MaskO = ui32MaskX << 16;

            //
            // If a player has a token in this row and column, print the
            // corresponding symbol
            //
            if(g_ui32BoardState & ui32MaskX)
            {
                UART_write(g_psUARTHandle, "X", 1);
            }
            else if(g_ui32BoardState & ui32MaskO)
            {
                UART_write(g_psUARTHandle, "O", 1);
            }
            else
            {
                UART_write(g_psUARTHandle, " ", 1);
            }

            //
            // Print column separators where necessary.
            //
            if(ui32ColNum < 2)
            {
                UART_write(g_psUARTHandle, "|", 1);
            }
        }

        //
        // End this row.
        //
        UART_write(g_psUARTHandle, "\n", 1);

        //
        // Add a row separator if necessary.
        //
        if(ui32RowNum < 2)
        {
            ui32Len = snprintf(g_pcTXBuf, TX_BUF_SIZE, "   -+-+-\n");
            UART_write(g_psUARTHandle, g_pcTXBuf, ui32Len);
        }

    }

    //
    // Print an extra empty line after the last row.
    //
    UART_write(g_psUARTHandle, "\n", 1);
}

//*****************************************************************************
//
// CheckWinner checks the global state variable to see if either player has
// won, or if the game has ended in a tie. Returns a 1 if the game is over, or
// a 0 if the game should continue.
//
//*****************************************************************************
bool
CheckWinner(void)
{
    uint32_t ui32Idx;
    uint32_t ui32WinMask0;
    uint32_t ui32WinMask1;
    uint32_t ui32CatCheck;
    uint32_t ui32QuitCheck;
    uint32_t ui32Len;

    //
    // Loop through the table of win-conditions.
    //
    for(ui32Idx = 0; ui32Idx < NUM_WIN_CONDITIONS; ui32Idx++)
    {
        //
        // Get a winning board configuration from the global table, and create
        // bit masks for each player corresponding to that win condition.
        //
        ui32WinMask0 = g_ui32WinConditions[ui32Idx];
        ui32WinMask1 = g_ui32WinConditions[ui32Idx] << 16;

        //
        // If a player's pieces line up with the winning configuration, count
        // this as a win.
        //
        if((g_ui32BoardState & ui32WinMask0) == ui32WinMask0)
        {
            ui32Len = snprintf(g_pcTXBuf, TX_BUF_SIZE, "'X' Wins!\n");
            UART_write(g_psUARTHandle, g_pcTXBuf, ui32Len);
            return true;
        }
        else if((g_ui32BoardState & ui32WinMask1) == ui32WinMask1)
        {
            ui32Len = snprintf(g_pcTXBuf, TX_BUF_SIZE, "'O' Wins!\n");
            UART_write(g_psUARTHandle, g_pcTXBuf, ui32Len);
            return true;
        }
    }

    //
    // AND together the position bits for both players to see how many spaces
    // are occupied.
    //
    ui32CatCheck = ((g_ui32BoardState | (g_ui32BoardState >> 16)) & 0x1FF);

    //
    // The server will signify a "quit" request by setting all of a single
    // player's bits high. Check for one of these states, and print a message
    // if it is found.
    //
    ui32QuitCheck = (g_ui32BoardState & 0x01FF);
    if(ui32QuitCheck == 0x01FF)
    {
        ui32Len = snprintf(g_pcTXBuf, TX_BUF_SIZE, "Game ended by other "
                           "player.\n");
        UART_write(g_psUARTHandle, g_pcTXBuf, ui32Len);
        return true;
    }

    ui32QuitCheck = (g_ui32BoardState & 0x01FF0000);
    if(ui32QuitCheck == 0x01FF0000)
    {
        ui32Len= snprintf(g_pcTXBuf, TX_BUF_SIZE, "Game ended by other player."
                          "\n");
        UART_write(g_psUARTHandle, g_pcTXBuf, ui32Len);
        return true;
    }

    //
    // If all spaces are full, and no winner was detected, declare this a tie.
    //
    if(ui32CatCheck == 0x1FF)
    {
        ui32Len = snprintf(g_pcTXBuf, TX_BUF_SIZE, "It's a tie.\n");
        UART_write(g_psUARTHandle, g_pcTXBuf, ui32Len);
        return true;
    }

    //
    // If the player's pieces do not line up with a known winning
    // configuration, return a zero, indicating that no winner was found.
    //
    return false;
}

//*****************************************************************************
//
// SetGameMode reads the user input to determine whether TicTacToe will be
// played locally or online, and whether the local player will play first or
// second. This function will return a 1 if the user-selected mode setting was
// valid, or a 0 if the mode could not be selected.
//
//*****************************************************************************
int32_t
SetGameMode(char *pcRxBuf)
{
    uint32_t ui32InputMode;
    uint32_t ui32Len;

    //
    // Yes - Check if a null pointer has been returned.
    //
    if(pcRxBuf == NULL)
    {
        //
        // Yes - Print error message and return to exit game.
        //
        ui32Len = snprintf(g_pcTXBuf, TX_BUF_SIZE, "\nBad RX buf pointer "
                           "returned. Exiting the game.\n");
        UART_write(g_psUARTHandle, g_pcTXBuf, ui32Len);
        return -1;
    }

    //
    // Pull the user input from the UART, and convert it to an integer.
    //
    ui32InputMode = strtoul(pcRxBuf, NULL, 0);

    //
    // Check to make sure we have a valid mode selection.
    //
    if(ui32InputMode == 3)
    {
        //
        // If the selected mode is "online, remote player first", set the state
        // variables accordingly.
        //
        g_ui32Mode = ui32InputMode;

        //
        // Setting the REMOTE_PLAYER bit will alert the remote user interface
        // that they should make the first move. Setting the global variable
        // for the old state allows the state machine to detect when the remote
        // play has happened.
        //
        g_ui32LastState = REMOTE_PLAYER;
        g_ui32BoardState = REMOTE_PLAYER;
        g_eBoardStaeRW = READ_WRITE;

        return 1;
    }
    else if((ui32InputMode > 0) && (ui32InputMode < 4))
    {
        //
        // If the user entered a different valid choice, set up the game mode,
        // but don't request a play from the remote interface.
        //
        g_ui32Mode = ui32InputMode;
        g_ui32LastState = 0x0;
        g_ui32BoardState = 0x0;
        g_eBoardStaeRW = WRITE_ONLY;

        return 1;
    }

    //
    // Invalid input.
    //
    ui32Len = snprintf(g_pcTXBuf, TX_BUF_SIZE, "Invalid input. Try again: ");
    UART_write(g_psUARTHandle, g_pcTXBuf, ui32Len);
    return 0;
}

//*****************************************************************************
//
// This function implements a state machine for the tic-tac-toe gameplay.
//
//*****************************************************************************
bool
AdvanceGameState(char *pcRxBuf, bool bUserInput)
{
    uint32_t ui32Len;
    int32_t i32Ret;

    //
    // Check if user has an input. typed a Q, end the game.
    //
    if(bUserInput == true)
    {
        //
        // Yes - Check if a null pointer has been returned.
        //
        if(pcRxBuf == NULL)
        {
            //
            // Yes - Print error message and return to exit game.
            //
            ui32Len = snprintf(g_pcTXBuf, TX_BUF_SIZE, "\nBad RX buf pointer "
                               "returned. Exiting the game.\n");
            UART_write(g_psUARTHandle, g_pcTXBuf, ui32Len);
            return true;
        }
        else if(strncmp(pcRxBuf, "Q", 2) == 0)
        {
            //
            // This board state signals a 'quit' condition to the server.
            //
            g_ui32BoardState = 0x01FF01FF;
            g_eBoardStaeRW = WRITE_ONLY;

            //
            // Print a quit message.
            //
            ui32Len = snprintf(g_pcTXBuf, TX_BUF_SIZE, "\nGame Over.\n");
            UART_write(g_psUARTHandle, g_pcTXBuf, ui32Len);
            return true;
        }
    }

    //
    // This switch statement controls the main flow of the game.
    //
    switch(g_ui32GameState)
    {
        case NEW_GAME:
        {
            //
            // For a new game, the first step is to determine the game mode.
            // Prompt the user for a game mode via UART, and advance the state
            // to wait for the user's response.
            //
            ui32Len = snprintf(g_pcTXBuf, TX_BUF_SIZE, "\033[2J\033[H");
            ui32Len += snprintf((g_pcTXBuf + ui32Len), (TX_BUF_SIZE - ui32Len),
                                "New Game!\n");
            ui32Len += snprintf((g_pcTXBuf + ui32Len), (TX_BUF_SIZE - ui32Len),
                                "  1 - play locally\n");
            ui32Len += snprintf((g_pcTXBuf + ui32Len), (TX_BUF_SIZE - ui32Len),
                                "  2 - play online, local user starts\n");
            UART_write(g_psUARTHandle, g_pcTXBuf, ui32Len);

            ui32Len = snprintf(g_pcTXBuf, TX_BUF_SIZE, "  3 - play online, "
                               "remote user starts\n");
            ui32Len += snprintf((g_pcTXBuf + ui32Len), (TX_BUF_SIZE - ui32Len),
                                "  Q - Enter Q at any time during play to "
                                "quit.\n\n");
            ui32Len += snprintf((g_pcTXBuf + ui32Len), (TX_BUF_SIZE - ui32Len),
                                "Select an option (1-3 or Q): ");
            UART_write(g_psUARTHandle, g_pcTXBuf, ui32Len);

            g_ui32GameState = SET_MODE;
            break;
        }

        case SET_MODE:
        {
            //
            // Only continue if we have input from the user.
            //
            if(bUserInput == true)
            {
                //
                // Attempt to use the user's input to set the game mode.
                //
                i32Ret = SetGameMode(pcRxBuf);
                if(i32Ret < 0)
                {
                    //
                    // Exit game due to unexpected error.
                    //
                    return true;
                }
                else if(i32Ret > 0)
                {
                    //
                    // If the user input was valid, show the game board and
                    // advance the state to start the first turn.
                    //
                    ShowBoard();
                    g_ui32GameState = PLAY_TURN;
                }
            }

            break;
        }

        case PLAY_TURN:
        {
            //
            // Check to see if we need input from the local user. This will
            // always be true for a local game, and should be true for only a
            // single player's turns for an online game.
            //
            if(!(g_ui32BoardState & REMOTE_PLAYER))
            {
                //
                // If we're playing a local game, prompt for a row number and
                // advance the state to wait for a response.
                //
                ui32Len = snprintf(g_pcTXBuf, TX_BUF_SIZE, "Enter Row: ");
                UART_write(g_psUARTHandle, g_pcTXBuf, ui32Len);
                g_ui32GameState = GET_ROW;
            }
            else
            {
                //
                // If the local player is not supposed to move for this turn,
                // print a message to let the player know that we are waiting
                // on input from a remote player.
                //
                ui32Len = snprintf(g_pcTXBuf, TX_BUF_SIZE, "Waiting for "
                                   "remote player...\n");
                UART_write(g_psUARTHandle, g_pcTXBuf, ui32Len);
                g_ui32GameState = REMOTE_PLAY;
            }

            break;
        }

        case GET_ROW:
        {
            //
            // Only continue if we have input from the user.
            //
            if(bUserInput == true)
            {
                //
                // Convert the user's input to an integer, and store it as the
                // new row number.
                //
                g_ui32Row = strtoul(pcRxBuf, NULL, 0);

                //
                // Prompt for a column number, and advance the state to wait
                // for a response.
                //
                ui32Len = snprintf(g_pcTXBuf, TX_BUF_SIZE, "Enter Column: ");
                UART_write(g_psUARTHandle, g_pcTXBuf, ui32Len);
                g_ui32GameState = GET_COLUMN;
            }

            break;
        }

        case GET_COLUMN:
        {
            //
            // Only continue if we have input from the user.
            //
            if(bUserInput == true)
            {
                //
                // Convert the user's input to an integer, and store it as the
                // new column number.
                //
                g_ui32Col = strtoul(pcRxBuf, NULL, 0);

                //
                // Try to process the recorded row and column numbers as a
                // "move" for the current player.
                //
                if(ProcessTurn())
                {
                    //
                    // The user's input was successfully processed and added to
                    // the game state. Show the board with the new move
                    // applied.
                    //
                    ShowBoard();

                    //
                    // Check to see if this was a winning move.
                    //
                    if(CheckWinner())
                    {
                        //
                        // If so, return a 1 to signal the end of the game.
                        //
                        return true;
                    }
                    else
                    {
                        //
                        // Otherwise, the game must go on. Check to see if we
                        // have a remote player.
                        //
                        if(g_ui32Mode != 1)
                        {
                            //
                            // We have a remote player, so toggle the bit to
                            // signal that the remote player should take their
                            // turn.
                            //
                            g_ui32BoardState ^= REMOTE_PLAYER;
                        }

                        //
                        // Remember the board state, so we can tell when it
                        // gets changed.
                        //
                        g_ui32LastState = g_ui32BoardState;

                        //
                        // Set the board state to sync with the server.
                        //
                        g_eBoardStaeRW = READ_WRITE;

                        //
                        // Finally, set the game state for the next turn.
                        //
                        g_ui32GameState = PLAY_TURN;
                    }
                }
                else
                {
                    //
                    // Something was wrong with the user's input. Try prompting
                    // them again.
                    //
                    ui32Len = snprintf(g_pcTXBuf, TX_BUF_SIZE, "Enter Row: ");
                    UART_write(g_psUARTHandle, g_pcTXBuf, ui32Len);
                    g_ui32GameState = GET_ROW;
                }
            }

            break;
        }

        case REMOTE_PLAY:
        {
            //
            // If we are waiting on a remote player, check to see if the board
            // state variable has changed.
            //
            if(g_ui32BoardState != g_ui32LastState)
            {
                //
                // Set the board state to stop reading from the server.
                //
                g_eBoardStaeRW = WRITE_ONLY;

                //
                // Record the new state, so we know that it has already been
                // seen once. This is important to prevent an infinite loop if
                // the server doesn't clear the "REMOTE_PLAYER" bit.
                //
                g_ui32LastState = g_ui32BoardState;

                //
                // Make sure that the player variable is up-to-date.
                //
                g_ui32Player = (g_ui32BoardState & PLAYER_BIT) ? 1 : 0;

                //
                // If the state has changed, assume that the remote player has
                // made their move.
                //
                ShowBoard();

                //
                // Check to see if this was a winning move.
                //
                if(CheckWinner())
                {
                    //
                    // If so, return a 1 to signal the end of the game.
                    //
                    return true;
                }
                else
                {
                    //
                    // Otherwise, update the last valid state, advance to the
                    // next turn.
                    //
                    g_ui32GameState = PLAY_TURN;
                }
            }

            break;
        }
    }

    //
    // The actions for the current state have been processed, and the game has
    // not met an ending condition. Return a zero to indicate that the game is
    // not yet finished.
    //
    return false;
}

//*****************************************************************************
//
// Clears the game state, and prepares the global variables to start a new game
// of tic-tac-toe
//
//*****************************************************************************
void
GameInit(void)
{
    //
    // Set the global board state tStat variable to WRITE_ONLY, to make sure
    // that it doesn't get overwritten by content from the server side.
    //
    g_eBoardStaeRW = WRITE_ONLY;

    //
    // Empty the board, set the player value to zero (for 'X'), and set the
    // main state machine to start a new game on the next call to
    // AdvanceGameState().
    //
    g_ui32BoardState = 0;
    g_ui32Player = 0;
    g_ui32GameState = NEW_GAME;
}

