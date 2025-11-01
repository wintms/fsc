/**************************************************************************
***************************************************************************
*** **
*** (c)Copyright 2025 Dell Inc.
*** **
*** All Rights Reserved.
*** **
*** **
*** File Name: OEMDBG.c
*** Description: Defines the global OEM debug array and implements debug
*** helper functions (like hex dump).
*** **
***************************************************************************
***************************************************************************
**************************************************************************/

// Standard C headers
#include <stdio.h>

// Project-specific headers
#include "Types.h"
#include "OEMDBG.h"


/*-------------------------------------------------------------------------*
 * Global Variable Definition
 *-------------------------------------------------------------------------*/

/**
 * @brief The global array storing the debug verbosity level for each OEM module.
 *
 * This is the definition that allocates memory for the array.
 * It is initialized to all zeros, meaning all debug items are disabled by default.
 */
INT8U g_OEMDebugArray[OEM_DEBUG_Item_MAX] = {0};


/*-------------------------------------------------------------------------*
 * Function Definitions
 *-------------------------------------------------------------------------*/

/**
 * @brief Prints a buffer as a hexadecimal dump to the console.
 *
 * This function is used to display the contents of a raw data buffer (e.g.,
 * I2C write/read data) in a human-readable hexadecimal format.
 *
 * @param buff Pointer to the data buffer to print.
 * @param len Length of the data buffer.
 */
void OEM_HexDump(unsigned char *buff, int len)
{
    int i = 0;
    for(i = 0; i < len; i++)
        printf("%02x ", *(buff + i));
    printf("\n");
}
