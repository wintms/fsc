/**************************************************************************
***************************************************************************
*** **
*** (c)Copyright 2025 Dell Inc. **
*** **
*** All Rights Reserved. **
*** **
*** **
*** File Name: OEMPLD.c **
*** Description: Implementation of OEM PLD/FPGA access functions. **
*** **
***************************************************************************
***************************************************************************
**************************************************************************/

#include "PDKDefs.h"
#include "hal_hw.h"

#include "OEMPLD.h"

/**
 * @fn OEM_ReadWritePLD
 * @brief Read/Write PLD register
 * @param pld_id   - PLD number:
 *                   PLD_ID_FPGA
 *                   PLD_ID_BaseBoard
 *                   PLD_ID_FanBoard
 *                   PLD_ID_COMe
 * @param reg      - PLD register, 8 bits
 * @param data_in  - PLD data to write
 * @param data_out - Buffer to store read PLD data
 * @param mode     - Read / Write mode:
 *                   PLD_ReadRegister / PLD_WriteRegister
 *
 * @return 0 on success, -1 on error.
 */
int OEM_ReadWritePLD(INT8U pld_id, INT8U reg, INT8U data_in, INT8U *data_out, INT8U mode)
{
    _NEAR_ INT8U* outBuffer;
    _NEAR_ INT8U* inBuffer;
    INT8U temp[2] = {0};
    INT8U i2cBusId = 0xff;
    INT8U slaveAddr = 0xff;
    char busName[64] = {0};
    int retval = -1;

    inBuffer = temp;
    outBuffer = temp;

    switch( pld_id )
    {
        case PLD_ID_FPGA:
            i2cBusId  = I2C_BUS_FPGA;
            slaveAddr = I2C_ADDR_FPGA;
            break;

        case PLD_ID_BaseBoard:
            i2cBusId  = I2C_BUS_BASE_CPLD;
            slaveAddr = I2C_ADDR_BASE_CPLD;
            break;

        case PLD_ID_FanBoard:
            i2cBusId  = I2C_BUS_FAN_CPLD;
            slaveAddr = I2C_ADDR_FAN_CPLD;
            break;

        case PLD_ID_COMe:
            i2cBusId  = I2C_BUS_COME_CPLD;
            slaveAddr = I2C_ADDR_COME_CPLD;
            break;

        default:
            return -1;
    }

    sprintf(busName,"/dev/i2c-%d",i2cBusId);

    if(mode == PLD_ReadRegister)
    {
        outBuffer[0] = reg;

        if( g_HALI2CHandle[HAL_I2C_RW] != NULL )
        {
            retval = ( ( int( * )( char *, u8, u8 *, u8 *, size_t, size_t ) )g_HALI2CHandle[HAL_I2C_RW] )( busName, slaveAddr, outBuffer, inBuffer, 1, 1 );
            if ( retval < 0 )
            {
                return -1;
            }
        }

        *data_out = inBuffer[0];

        return 0;
    }
    else if(mode == PLD_WriteRegister)
    {
        outBuffer[0] = reg;
        outBuffer[1] = data_in;

        if( g_HALI2CHandle[HAL_I2C_MW] != NULL )
        {
            retval = ( ( int( * )( char *, u8, u8 *, size_t ) )g_HALI2CHandle[HAL_I2C_MW] )( busName, slaveAddr, outBuffer, 2);
            if ( retval < 0 )
            {
                return -1;
            }
        }
    }
    else
    {
        return -1;
    }

    return 0;
}
