/*
## Cypress USB 3.0 Platform source file (cyu3cardmgr_fx3.c)
## ===========================
##
##  Copyright Cypress Semiconductor Corporation, 2010-2013,
##  All Rights Reserved
##  UNPUBLISHED, LICENSED SOFTWARE.
##
##  CONFIDENTIAL AND PROPRIETARY INFORMATION
##  WHICH IS THE PROPERTY OF CYPRESS.
##
##  Use of this file is governed
##  by the license agreement included in the file
##
##     <install>/license/license.txt
##
##  where <install> is the Cypress software
##  installation root directory path.
##
## ===========================
*/

#include <cyu3regs.h>
#include <cyu3sibpp.h>
#include <cyu3cardmgr.h>
#include <cyu3dma.h>
#include <cyu3socket.h>
#include <cyu3utils.h>
#include <cyu3error.h>
#include <cyu3vic.h>
#include <cyu3system.h>
#include <cyu3gpio.h>
#include <cyu3sib.h>
#include <cyfx3_api.h>

/* Summary:
   This file contains the low level card driver functions.
 */

/************************************************************************************/
/********************************GLOBAL VARIABLES************************************/
/************************************************************************************/

/* Storage device related information. */
CyU3PCardCtxt_t glCardCtxt[CY_U3P_SIB_NUM_PORTS]        = {0};
CyBool_t        glSibCardInitMode[CY_U3P_SIB_NUM_PORTS] = {CyFalse};

/* A global buffer used for read/write during the initialization and partitioning operations of the card. */
extern uint8_t glSibBuffer[];

#ifdef CYU3P_STORAGE_SDIO_SUPPORT
/* Forward declaration. */
static CyU3PReturnStatus_t
CyU3PCardMgrSdioInit (
        uint8_t portId,
        uint8_t lowVoltageSwitch);
#endif




/* SD 3.0 Tuning block data. Refer section 4.2.4.5 of the SD 3.0 spec. */
const uint32_t glTuningBlock[16] =
{
    0xFF0FFF00, 0xFFCCC3CC, 0xC33CCCFF, 0xFEFFFEEF,
    0xFFDFFFDD, 0xFFFBFFFB, 0xBFFF7FFF, 0x77F7BDEF,
    0xFFF0FFF0, 0x0FFCCC3C, 0xCC33CCCF, 0xFFEFFFEE,
    0xFFFDFFFD, 0xDFFFBFFF, 0xBBFFF7FF, 0xF77F7BDE
};

/* This function sets the SIB Core Clock */
void
CyU3PCardMgrSetClockFreq (
        uint8_t portId,
        uint16_t clkDivider )
{
    uint32_t clkValue;

    /* Disable the SIB clock before making any changes. */
    SIB->sdmmc[portId].cs |= CY_U3P_SIB_SDMMC_CLK_DIS;
    CyU3PBusyWait (1);

    /* Set the clock divider. Always derive the clock from SYS_CLK. */
    clkValue = ((clkDivider & CY_U3P_GCTL_SIBCLK_DIV_MASK) << CY_U3P_GCTL_SIBCLK_DIV_POS) |
        (0x03 << CY_U3P_GCTL_SIBCLK_SRC_POS) | CY_U3P_GCTL_SIBCLK_CLK_EN;

    if (portId == CY_U3P_SIB_PORT_0)
    {
        /* Setting the sib0 core clock */
        GCTL->sib0_core_clk = clkValue;
    }
    else
    {
        /* Setting the sib1 core clock */
        GCTL->sib1_core_clk = clkValue;
    }

    /* Re-enable the SIB clock once done. */
    CyU3PBusyWait (1);
    SIB->sdmmc[portId].cs &= ~CY_U3P_SIB_SDMMC_CLK_DIS;

    /* Wait for 100 us to let the clock stabilize. */
    CyU3PBusyWait (100);
}

/* Initializes the Card context information for the specified port. */
void
CyU3PCardMgrInitCtxt (
        uint8_t portId )
{
    /* Reset the SIB Controller */
    CyU3PSibResetSibCtrlr (portId);

    /* Set the context variables to a sane state */
    CyU3PMemSet ((uint8_t *)&glCardCtxt[portId], 0, sizeof(CyU3PCardCtxt_t));

    /* Default Values */
    glCardCtxt[portId].dataTimeOut = 0xFFFF;
    glCardCtxt[portId].removable = CyTrue;

    /* Configuring the mode register SD by default */
    SIB->sdmmc[portId].mode_cfg = ((CY_U3P_SIB_SDMMC_MODE_CFG_DEFAULT & ~CY_U3P_SIB_MODE_MASK) |
        (CY_U3P_SIB_DEV_SD << CY_U3P_SIB_MODE_POS));

    /* Ensure that all the interrupt bits are cleared and masked */
    CyU3PSibClearIntr (portId);
    SIB->sdmmc[portId].intr_mask = CY_U3P_SIB_SDMMC_INTR_MASK_DEFAULT;
}

CyU3PReturnStatus_t
CyU3PCardMgrWaitForInterrupt (
        uint8_t   portId,
        uint32_t  intr,
        uint32_t  timeout)
{
    uint32_t mask = intr | CY_U3P_SIB_CRC7_ERROR | CY_U3P_SIB_RESPTIMEOUT |
        CY_U3P_SIB_RD_DATA_TIMEOUT | CY_U3P_SIB_CRC16_ERROR;
    uint32_t stat;

    stat = SIB->sdmmc[portId].intr;
    while (((stat & mask) == 0) && (timeout > 0))
    {
        /* Do not apply any delays when waiting for a response after the storage device is
         * fully initialized.
         */
        if ((intr != CY_U3P_SIB_RCVDRES) || (glSibCardInitMode[portId]))
            CyU3PBusyWait (CY_U3P_SIB_POLLING_DELAY);

        stat = SIB->sdmmc[portId].intr;
        timeout--;
    }

    if ((stat & intr) != 0)
        return CY_U3P_SUCCESS;

    if ((stat & (CY_U3P_SIB_CRC7_ERROR | CY_U3P_SIB_CRC16_ERROR)) != 0)
        return CY_U3P_ERROR_CRC;

    return CY_U3P_ERROR_TIMEOUT;
}

static void
CyU3PCardMgrClearCmdBits (
        uint8_t portId)
{
    uint32_t sib_cs;

    /* Clear any command bits that are still set from last time. Using an umbrella check outside to
       optimize the common case. */
    sib_cs = SIB->sdmmc[portId].cs;
    if ((sib_cs & (CY_U3P_SIB_SNDCMD | CY_U3P_SIB_RDDCARD | CY_U3P_SIB_WRDCARD)) != 0)
    {
        if ((sib_cs & CY_U3P_SIB_SNDCMD) != 0)
        {
            sib_cs |= CY_U3P_SIB_CLR_SNDCMD;
        }

        if ((sib_cs & CY_U3P_SIB_RDDCARD) != 0)
        {
            sib_cs |= CY_U3P_SIB_CLR_RDDCARD;
        }

        if ((sib_cs & CY_U3P_SIB_WRDCARD) != 0)
        {
            sib_cs |= CY_U3P_SIB_CLR_WRDCARD;
        }

        /* Ensure that any stale command bits are cleared before proceeding with
           the next command. */
        do {
            SIB->sdmmc[portId].cs = sib_cs & CY_U3P_SIB_CS_REG_MASK;
            CyU3PBusyWait (CY_U3P_SIB_POLLING_DELAY);
        } while ((SIB->sdmmc[portId].cs & (CY_U3P_SIB_SNDCMD | CY_U3P_SIB_RDDCARD | CY_U3P_SIB_WRDCARD)) != 0);
    }
}

/* Send a command to the storage device using the SIB registers. */
void
CyU3PCardMgrSendCmd (
        uint8_t  portId,
        uint8_t  cmd,
        uint8_t  respLen,
        uint32_t cmdArg,
        uint32_t flags)
{
    uint32_t respFmt = CY_U3P_SIB_SDMMC_CMD_RESP_FMT_DEFAULT;
    uint32_t sib_cs;

    /* Clear all interrupts except card detect. */
    SIB->sdmmc[portId].intr = ~CY_U3P_SIB_CARD_DETECT;
    CyU3PCardMgrClearCmdBits (portId);

    /* Set the command index and arguments. */
    SIB->sdmmc[portId].cmd_idx  = cmd;
    SIB->sdmmc[portId].cmd_arg0 = cmdArg;
    respFmt &= ~CY_U3P_SIB_RESPCONF_MASK;
    respFmt |= (respLen << CY_U3P_SIB_RESPCONF_POS);
    SIB->sdmmc[portId].cmd_resp_fmt = respFmt;

    /* HW workaround: Adding a small delay before sending the next command down to prevent the
                      controller from getting stuck. */
    sib_cs = (SIB->sdmmc[portId].cs & CY_U3P_SIB_CS_CFG_MASK);
    CyU3PBusyWait (CY_U3P_SIB_SENDCMD_DELAY);

    /* Initiate the command send operation, and wait until the hardware has sent the command down. */
    SIB->sdmmc[portId].cs = (sib_cs | CY_U3P_SIB_SNDCMD | flags);
    while ((SIB->sdmmc[portId].intr & CY_U3P_SIB_SNDCMD) == 0);
}

/* Sets the block size for the media on the specified port. */
CyU3PReturnStatus_t
CyU3PCardMgrSetBlockSize (
        uint8_t portId,
        uint32_t blkLen)
{
    CyU3PReturnStatus_t status;

    /* Send CMD16 to configure the block size of the card. */
    CyU3PCardMgrSendCmd (portId, CY_U3P_SD_MMC_CMD16_SET_BLOCKLEN, CY_U3P_SD_MMC_R1_RESP_BITS, blkLen, 0);
    status = CyU3PCardMgrWaitForCmdResp (portId);
    if (status == CY_U3P_SUCCESS)
    {
        /* Update the SIB register's block length value. */
        CyU3PSibSetBlockLen (portId, blkLen);
    }

    return status;
}

/* Use the CMD13 (SEND_STATUS) command to wait until the device is in the tran state, where it
   can accept new commands. */
CyU3PReturnStatus_t
CyU3PCardMgrCheckStatus (
        uint8_t portId)
{
    uint32_t trials = 0x3FFFF;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

#ifdef CYU3P_STORAGE_SDIO_SUPPORT
    /* This operation does not apply to SDIO cards. */
    if (glCardCtxt[portId].cardType == CY_U3P_SIB_DEV_SDIO)
    {
        return status;
    }
#endif

    while (trials > 0)
    {
        CyU3PCardMgrSendCmd (portId, CY_U3P_SD_MMC_CMD13_SEND_STATUS, CY_U3P_SD_MMC_R1_RESP_BITS,
                (glCardCtxt[portId].cardRCA << 16), 0);

        status = CyU3PCardMgrWaitForCmdResp (portId);
        if (status != CY_U3P_SUCCESS)
        {
            break;
        }

        /* Check if the card's READY_FOR_DATA bit is set */
        if ((SIB->sdmmc[portId].resp_reg0 & 0x100) != 0)
        {
            break;
        }

        CyU3PBusyWait (CY_U3P_SIB_POLLING_DELAY);
        trials --;
    }

    if (status == CY_U3P_SUCCESS)
    {
        if ((SIB->sdmmc[portId].resp_reg0 & 0x100) == 0)
            status = CY_U3P_ERROR_TIMEOUT;
        else
        {
            /* Wait until the card stops signalling busy state on the DAT[0] pin. */
            while ((SIB->sdmmc[portId].status & CY_U3P_SIB_DAT0_STAT) == 0);
        }
    }

    return status;
}


/* Use CMD12 (STOP_TRANSMISSION) to stop the ongoing data transfer command. */
CyU3PReturnStatus_t
CyU3PCardMgrStopTransfer (
        uint8_t portId)
{
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    /* This operation does not apply to SDIO cards. */
#ifdef CYU3P_STORAGE_SDIO_SUPPORT
    /* No Stop transfer command for SDIO without memory*/
    if (glCardCtxt[portId].cardType == CY_U3P_SIB_DEV_SDIO)
        return status;
#endif

    /* Turn off card detect handling while doing this step. */
    SIB->sdmmc[portId].intr_mask &= (~CY_U3P_SIB_DAT3_CHANGE);

    /* Send the stop transmission command to the card. */
    CyU3PCardMgrSendCmd (portId, CY_U3P_SD_MMC_CMD12_STOP_TRANSMISSION, CY_U3P_SD_MMC_R1B_RESP_BITS, 0, 0);
    status = CyU3PCardMgrWaitForCmdResp (portId);
    if (status == CY_U3P_SUCCESS)
    {
        status = CyU3PCardMgrCheckStatus (portId);
    }

    if (glSibIntfParams[portId].cardDetType == CY_U3P_SIB_DETECT_DAT_3)
    {
        SIB->sdmmc[portId].intr_mask |= CY_U3P_SIB_DAT3_CHANGE;
    }

    return status;
}

#define CYU3P_SIB0_SD_PIN_MASK          (0x0000061E)    /* Pins 33 - 36 and 41 - 42. */
#define CYU3P_SIB1_SD_PIN_MASK          (0x000FC000)    /* Pins 46 - 51. */

CyU3PReturnStatus_t
CyU3PCardCheckAndSwitchToLowVoltageOp (
        uint8_t portId)
{

    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    uint8_t gpioId;

    /* SD 3.0: Check if S18A bit is set. */
    if (((SIB->sdmmc[portId].resp_reg0 & CY_U3P_SIB_S18A_BIT) != 0) && (glSibIntfParams[portId].lowVoltage))
    {
        gpioId = glSibIntfParams[portId].voltageSwGpio;

        /* Send CMD11. Expect R1 Response */
        CyU3PCardMgrSendCmd (portId, CY_U3P_SD_CMD11_VOLTAGE_SWITCH, CY_U3P_SD_MMC_R1_RESP_BITS, 0, 0);
        status = CyU3PCardMgrWaitForCmdResp (portId);
        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }

        /* Clock needs to be stopped for at least 5 ms, once the response has been received. */
        SIB->sdmmc[portId].cs |= CY_U3P_SIB_SDMMC_CLK_DIS;
        CyU3PThreadSleep (CY_U3P_SIB_SD_CMD11_CLKSTOP_DURATION);

        /* Now confirm that all CMD, CLK and DAT lines are low. */
        if (((portId == 0) && ((GPIO->lpp_gpio_invalue1 & CYU3P_SIB0_SD_PIN_MASK) != 0)) ||
                ((portId == 1) && ((GPIO->lpp_gpio_invalue1 & CYU3P_SIB1_SD_PIN_MASK) != 0)))
        {
            return CY_U3P_ERROR_TIMEOUT;
        }

        /* Update the GPIO to control the voltage switch, and wait for sometime to let the voltage stabilize. */
        CyU3PGpioSetValue (gpioId, glSibIntfParams[portId].lvGpioState);
        CyU3PThreadSleep (CY_U3P_SIB_SD_CMD11_CLKSTOP_DURATION);

        /* Change the mode cfg to: SDR12: SD SDR 25 MHz @1.8V. */
        SIB->sdmmc[portId].mode_cfg = ((SIB->sdmmc[portId].mode_cfg & ~CY_U3P_SIB_SIGNALING_MASK) |
                (0x02 << CY_U3P_SIB_SIGNALING_POS));

        /* Enable the SD Clk */
        SIB->sdmmc[portId].cs &= (~CY_U3P_SIB_SDMMC_CLK_DIS);
        CyU3PThreadSleep (CY_U3P_SIB_SD_CMD11_CLKACT_DURATION);

        /* The Card should drive the DAT0-3 lines high after 1ms from supplying the SD Clk. */
        if (SIB->sdmmc[portId].status & (CY_U3P_SIB_DAT0_STAT | CY_U3P_SIB_DAT3_STAT) !=
                (CY_U3P_SIB_DAT0_STAT | CY_U3P_SIB_DAT3_STAT))
        {
            return CY_U3P_ERROR_TIMEOUT;
        }

        glCardCtxt[portId].opVoltage = CY_U3P_SIB_VOLTAGE_LOW;
    }

    return status;
}

static void
MyStoreSDRegValue (
        uint8_t  portId,
        uint8_t *tgt)
{
    uint32_t  val, i;
    uint32_t *src = (uint32_t *)CY_U3P_SIB_SDMMC_RESP_REG0_ADDRESS(portId);
    for (i = 0; i < CY_U3P_SD_REG_CSD_LEN; i += 4)
    {
        val = *src++;
        tgt[i + 0] = CY_U3P_DWORD_GET_BYTE3 (val);
        tgt[i + 1] = CY_U3P_DWORD_GET_BYTE2 (val);
        tgt[i + 2] = CY_U3P_DWORD_GET_BYTE1 (val);
        tgt[i + 3] = CY_U3P_DWORD_GET_BYTE0 (val);
    }
}

/* This function is used to select Op Conditions and switch to low voltage if lowvoltageswitch =1 */
static CyU3PReturnStatus_t
CyU3PCardMgrSelOpConditionsVS (
        uint8_t portId,
        uint8_t cardType,
        uint8_t lowVoltageSwitch)
{
    uint32_t arg = 0;
    uint16_t trials;
    uint8_t  cardReady = 0;
    uint8_t  cmd = 0;
    uint8_t  respLen = 0;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

#ifdef CYU3P_STORAGE_SDIO_SUPPORT
    /* Sending the Correct Command and response for the SD and SDIO */
    if (cardType == CY_U3P_SIB_DEV_SDIO)
    {
        cmd     = CY_U3P_SD_MMC_CMD5_IO_SEND_OP_COND;
        respLen = CY_U3P_SD_MMC_R4_RESP_BITS;
    }
    else
#endif
    {
        if (glCardCtxt[portId].cardVer >= CY_U3P_SD_CARD_VER_2_0)
        {
            /* Set the HCS bit indicating that the host is capable of supporting the High Capacity cards.
             * SD 3.0: Set the S18R bit to indicate that the host supports UHS-I.
             * Set the XPC bit to 1 to indicate max. performance. */
            arg = (CY_U3P_SIB_HCS_BIT | CY_U3P_SIB_S18R_BIT | CY_U3P_SIB_XPC_BIT);
        }

        /* Use ACMD41 for SD cards, and CMD1 for eMMC devices. */
        if (cardType == CY_U3P_SIB_DEV_SD)
            cmd = CY_U3P_SD_ACMD41_SD_SEND_OP_COND;
        else
            cmd = CY_U3P_MMC_CMD1_SEND_OP_COND;
        respLen = CY_U3P_SD_MMC_R3_RESP_BITS;
    }

    CyU3PCardMgrSendCmd (portId, cmd, respLen, arg, 0);
    status = CyU3PCardMgrWaitForCmdResp (portId);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

#ifdef CYU3P_STORAGE_SDIO_SUPPORT
    if (cardType == CY_U3P_SIB_DEV_SDIO)
    {
        glCardCtxt[portId].numberOfFunction = CyU3PSdioNumberOfFunction (portId);
        glCardCtxt[portId].isMemoryPresent  = CyU3PSdioMemoryPresent (portId);

    }
#endif

    /* Check if the card has set any of the bits in the default voltage range. */
    if ((SIB->sdmmc[portId].resp_reg0 & CY_U3P_SIB_DEF_VOLT_RANGE) == 0)
    {
        return CY_U3P_ERROR_INVALID_VOLTAGE_RANGE;
    }

    /* The SD spec requires a timeout of 1s in the wait for device readyness. We are waiting for ~1.025 seconds. */
    trials = CY_U3P_SIB_DEVICE_READY_TIMEOUT;
    while (trials > 0)
    {
        switch (cardType)
        {
            case CY_U3P_SIB_DEV_SD:
                arg = 0;

                /* Send the APP_CMD command. Expect R1 response. */
                CyU3PCardMgrSendCmd (portId, CY_U3P_SD_MMC_CMD55_APP_CMD, CY_U3P_SD_MMC_R1_RESP_BITS, arg, 0);
                status = CyU3PCardMgrWaitForCmdResp (portId);
                if (status != CY_U3P_SUCCESS)
                {
                    return status;
                }

                /* Set the optimal operating voltage level (~3.3v) */
                arg = (0x3c << 16);
                if (glCardCtxt[portId].cardVer >= CY_U3P_SD_CARD_VER_2_0)
                {
                    arg |= (CY_U3P_SIB_HCS_BIT | CY_U3P_SIB_S18R_BIT | CY_U3P_SIB_XPC_BIT);
                }

                /* Send the SEND_OP_COND (ACMD41) command. Expect R3 response. */
                cmd     = CY_U3P_SD_ACMD41_SD_SEND_OP_COND;
                respLen = CY_U3P_SD_MMC_R3_RESP_BITS;
                break;

#ifdef CYU3P_STORAGE_SDIO_SUPPORT
            case CY_U3P_SIB_DEV_SDIO:
                if (glCardCtxt[portId].numberOfFunction > 0)
                {
                    if (glCardCtxt[portId].isMemoryPresent)
                        glCardCtxt[portId].cardType = CY_U3P_SIB_DEV_SDIO_COMBO;
                    else
                        glCardCtxt[portId].cardType = CY_U3P_SIB_DEV_SDIO;

                    cmd = CY_U3P_SD_MMC_CMD5_IO_SEND_OP_COND;
                    arg = (SIB->sdmmc[portId].resp_reg0 & CY_U3P_SIB_DEF_VOLT_RANGE);
                    if ((glSibIntfParams[portId].lowVoltage == CyTrue) && (lowVoltageSwitch))
                    {
                        arg |= CY_U3P_SIB_S18A_BIT;
                    }

                    respLen = CY_U3P_SD_MMC_R4_RESP_BITS;
                }
                else
                    return CY_U3P_ERROR_UNSUPPORTED_CARD;
                break;
#endif

            default:
                /* Sector mode addressing is supported. Selected voltage range is 2.7-3.6V */
                cmd     = CY_U3P_MMC_CMD1_SEND_OP_COND;
                arg     = (CY_U3P_SIB_SEC_MODE_ADDR_BIT | 0x00FF8000);
                respLen = CY_U3P_SD_MMC_R3_RESP_BITS;
                break;
        }

        CyU3PCardMgrSendCmd (portId, cmd, respLen, arg, 0);
        status = CyU3PCardMgrWaitForCmdResp (portId);
        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }

        /* 31st bit of the OCR register indicates if the card's initialization is complete. */
        cardReady = (SIB->sdmmc[portId].resp_reg0 >> CY_U3P_SIB_CARD_BUSY_BIT_POS);
        if (cardReady == 1)
        {
            break;
        }

        CyU3PThreadSleep (1);
        trials--;
    }

    if (cardReady != 1)
    {
        return CY_U3P_ERROR_TIMEOUT;
    }

    /* Save the OCR register's data for later use. */
    glCardCtxt[portId].ocrRegData = SIB->sdmmc[portId].resp_reg0;

#ifdef CYU3P_STORAGE_SDIO_SUPPORT
    if (cardType == CY_U3P_SIB_DEV_SDIO)
    {
        CyU3PThreadSleep (1);

        /* There are timeout and response failure issues seen with the Arasan SDIO 2.0 card when the OCR
           bit S18A is set in CMD5. Therefore, the Low voltage switching will only be done for SDIO cards
           if the UHS-I bit flags in the CCCR are set which will be done in the SDIO init routine*/
        if (glSibIntfParams[portId].lowVoltage == CyTrue && lowVoltageSwitch)
        {
            status = CyU3PCardCheckAndSwitchToLowVoltageOp (portId);
        }

        if (SIB->sdmmc[portId].status & (CY_U3P_SIB_DAT0_STAT | CY_U3P_SIB_DAT3_STAT) !=
                (CY_U3P_SIB_DAT0_STAT | CY_U3P_SIB_DAT3_STAT))
        {
            return CY_U3P_ERROR_TIMEOUT;
        }
    }
    else
#endif
    {
        /* Check the CCS bit. CCS = 0 -> Standard Capacity card; CCS = 1 -> High Capacity Card */
        if ((SIB->sdmmc[portId].resp_reg0 & CY_U3P_SIB_CCS_BIT) != 0)
        {
            glCardCtxt[portId].highCapacity = 1;
            status = CyU3PCardCheckAndSwitchToLowVoltageOp (portId);
        }

        /* Command to fetch the CID register */
        CyU3PCardMgrSendCmd (portId, CY_U3P_SD_MMC_CMD2_ALL_SEND_CID, CY_U3P_SD_MMC_R2_RESP_BITS, 0, 0);
        status = CyU3PCardMgrWaitForCmdResp (portId);
        if (status == CY_U3P_SUCCESS)
        {
            /* Save the CID contents for later use. */
            MyStoreSDRegValue (portId, glCardCtxt[portId].cidRegData);
        }
    }

    return status;
}

/* Function to Select Card Operating Conditions */
CyU3PReturnStatus_t
CyU3PCardMgrSelOpConditions (
        uint8_t portId,
        uint8_t cardType)
{
    return CyU3PCardMgrSelOpConditionsVS (portId, cardType, 0);
}

/* This function is used to either select or deselect a card. A response is expected
   for select command and none for deselect command. */
CyU3PReturnStatus_t
CyU3PCardMgrSelectCard (
        uint8_t portId,
        uint8_t enable)
{
    uint32_t arg = 0;
    uint8_t  respLen = 0;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    if (enable != 0)
    {
        arg     = glCardCtxt[portId].cardRCA << 16;
        respLen = CY_U3P_SD_MMC_R1B_RESP_BITS;
    }

    CyU3PCardMgrSendCmd (portId, CY_U3P_SD_MMC_CMD7_SELECT_DESELECT_CARD, respLen, arg, 0);
    if (respLen > 0)
    {
        status = CyU3PCardMgrWaitForCmdResp (portId);
    }

    return status;
}

const uint32_t SibAuSizeMap[] =
{
    /* 0 */ 0,
    /* 1 */ 16 * 1024,
    /* 2 */ 32 * 1024,
    /* 3 */ 64 * 1024,
    /* 4 */ 128 * 1024,
    /* 5 */ 256 * 1024,
    /* 6 */ 512 * 1024,
    /* 7 */ 1024 * 1024,
    /* 8 */ 2 * 1024 * 1024,
    /* 9 */ 4 * 1024 * 1024,
    /* A */ 8 * 1024 * 1024,
    /* B */ 12 * 1024 * 1024,
    /* C */ 16 * 1024 * 1024,
    /* D */ 24 * 1024 * 1024,
    /* E */ 32 * 1024 * 1024,
    /* F */ 64 * 1024 * 1024
};

/* Function used to compute the Erase Unit Size of the Card. */
CyU3PReturnStatus_t
CyU3PCardMgrGetEUSize (
        uint8_t portId)
{
    uint8_t auValue = 0;
    uint8_t sectorSize = 1;
    uint8_t eraseBlkEnable = ((glCardCtxt[portId].csdRegData[10] >> 6) & 0x01);
    CyU3PDmaBuffer_t dmaBuffer;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    if ((glCardCtxt[portId].csdRegData[4] & 0x02) == 0)
    {
        /* Erase command is not supported. */
        glCardCtxt[portId].eraseSize = 0;
        return CY_U3P_SUCCESS;
    }

    if (glCardCtxt[portId].cardVer >= CY_U3P_SD_CARD_VER_2_0)
    {
        /* Setup DMA for receving the data before sending the command. */
        dmaBuffer.buffer = glSibBuffer;
        dmaBuffer.size   = CY_U3P_SIB_BLOCK_SIZE;
        dmaBuffer.count  = CY_U3P_SIB_BLOCK_SIZE;

        status = CyU3PDmaChannelSetupRecvBuffer (CyU3PSibGetDmaChannelHandle, &dmaBuffer);
        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }

        /* Send the APP_CMD(CMD55) command. Expect R1 response. */
        CyU3PCardMgrSendCmd (portId, CY_U3P_SD_MMC_CMD55_APP_CMD, CY_U3P_SD_MMC_R1_RESP_BITS,
                (glCardCtxt[portId].cardRCA << 16), 0);
        status = CyU3PCardMgrWaitForCmdResp (portId);
        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }

        /* Set the block parameters for this read operation. */
        CyU3PSibSetActiveSocket (portId, CYU3P_SIB_INT_READ_SOCKET);
        CyU3PSibSetBlockLen (portId, 64);
        CyU3PSibSetNumBlocks (portId, 1);

        CyU3PCardMgrSendCmd (portId, CY_U3P_SD_ACMD13_SD_STATUS, CY_U3P_SD_MMC_R1_RESP_BITS,
                (glCardCtxt[portId].cardRCA << 16), CY_U3P_SIB_RDDCARD);
        status = CyU3PCardMgrWaitForCmdResp (portId);
        if (status!= CY_U3P_SUCCESS)
        {
            return status;
        }

        /* Wait for the block received interrupt */
        status = CyU3PCardMgrWaitForBlkXfer (portId, CY_U3P_SIB_BLOCKS_RECEIVED);
        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }

        /* AU_SIZE is the upper 4 bits of glSibBuffer[10]. */
        auValue = ((glSibBuffer[10] >> 4) & 0xF);
        if (auValue < 16)
        {
            glCardCtxt[portId].eraseSize = SibAuSizeMap[auValue];
        }
        else
        {
            glCardCtxt[portId].eraseSize = 0;
        }

        /* SPEED_CLASS value is stored in glSibBuffer[8]. */
        glCardCtxt[portId].cardSpeed = (glSibBuffer[8] & 0x0F) << 1;
        if (glCardCtxt[portId].cardSpeed == 8)
        {
            glCardCtxt[portId].cardSpeed = 10;
        }
        if (glCardCtxt[portId].cardSpeed > 8)
        {
            /* Reserved */
            glCardCtxt[portId].cardSpeed = 11;
        }
    }
    else
    {
        /* SD 1.1 card: Bits 39:45 of the CSD plus 1 */
        if (eraseBlkEnable == 0)
        {
            sectorSize  = ((glCardCtxt[portId].csdRegData[10] & 0x3F) << 1);
            sectorSize |= ((glCardCtxt[portId].csdRegData[11] & 0x80) >> 7);
            sectorSize ++;
        }

        glCardCtxt[portId].eraseSize = (uint32_t)sectorSize << 9;
    }

    return status;
}

/* This function is used to receive the CSD register data. */
void
CyU3PCardMgrGetCSD (
        uint8_t  portId,
        uint8_t *csd_buffer)
{
    CyU3PMemCopy (csd_buffer, &glCardCtxt[portId].csdRegData[0], CY_U3P_SD_REG_CSD_LEN);
}

/* This function is used to parse the cards CSD register data. */
CyU3PReturnStatus_t
CyU3PCardMgrReadAndParseCSD (
        uint8_t portId,
        uint8_t cardType)
{
    uint8_t  cardSizeMult = 0;
    uint8_t  readBlockLen = 0;
    uint16_t cmdClasses   = 0;
    uint32_t cardSize     = 0;
    CyU3PReturnStatus_t status;

    /* Get the CSD register with the SEND_CSD(CMD9) command. Expect R2 response. */
    CyU3PCardMgrSendCmd (portId, CY_U3P_SD_MMC_CMD9_SEND_CSD, CY_U3P_SD_MMC_R2_RESP_BITS,
            (glCardCtxt[portId].cardRCA << 16), 0);

    status = CyU3PCardMgrWaitForCmdResp (portId);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Save the CSD contents for later use. */
    MyStoreSDRegValue (portId, glCardCtxt[portId].csdRegData);

    if (cardType == CY_U3P_SIB_DEV_MMC)
    {
        /* Store the card specification version */
        glCardCtxt[portId].cardVer = ((CY_U3P_SIB_SDMMC_RESP_REG0(portId) >> 26) & 0xF);
    }
    else
    {
        /* SD card, CSD register version. */
        glCardCtxt[portId].cardVer = (CY_U3P_SIB_SDMMC_RESP_REG0(portId) >> 30);
    }

    if ((cardType == CY_U3P_SIB_DEV_SD) && (glCardCtxt[portId].cardVer > 0))
    {
        /* Compute the memory capacity of the SD 2.0, High capacity card */
        glCardCtxt[portId].numBlks = ((((CY_U3P_SIB_SDMMC_RESP_REG2(portId) >> 16) |
                        (CY_U3P_SIB_SDMMC_RESP_REG1(portId) << 16)) + 1) * 0x400);
    }
    else
    {
        /* Card Capacity computation: Refer Sec 5.3.1 of SD spec 2.0  */
        readBlockLen = ((CY_U3P_SIB_SDMMC_RESP_REG1(portId) >> 16) & 0x0F);

        cardSize = (CY_U3P_SIB_SDMMC_RESP_REG1(portId) & 0x3FF);
        cardSize <<= 2;
        cardSize |= (CY_U3P_SIB_SDMMC_RESP_REG2(portId) >> 30);

        cardSizeMult = ((CY_U3P_SIB_SDMMC_RESP_REG2(portId) >> 15) & 0x07);

        cardSize     ++;
        cardSizeMult += 2;

        /* Adjust for 512 byte blocks. */
        if (readBlockLen != 9)
        {
            cardSizeMult += (readBlockLen - 9);
        }

        glCardCtxt[portId].numBlks = (uint32_t)cardSize << cardSizeMult;
    }

    glCardCtxt[portId].blkLen = CY_U3P_SIB_BLOCK_SIZE;

    /* Check if the card is write protected. */
    glCardCtxt[portId].writeable = 1;
    if ((CY_U3P_SIB_SDMMC_RESP_REG3(portId) >> 13) & 0x01)
    {
        glCardCtxt[portId].writeable = 0;
    }

    /* Verify that the required command classes are supported. Command classes 0 and 2 are mandatory. */
    cmdClasses = CY_U3P_SIB_SDMMC_RESP_REG1(portId) >> 20;
    if ((cmdClasses & CY_U3P_SD_MMC_REQD_CMD_CLASS) != CY_U3P_SD_MMC_REQD_CMD_CLASS)
    {
        return CY_U3P_ERROR_UNSUPPORTED_CARD;
    }
    glCardCtxt[portId].ccc = cmdClasses;

    /* Check if DSR is implemented */
    if ((CY_U3P_SIB_SDMMC_RESP_REG1(portId) >> 12) & 0x01)
    {
        /* Program the DSR register to indicate the new clock frequency. */
        CyU3PCardMgrSendCmd (portId, CY_U3P_SD_MMC_CMD4_SET_DSR, 0, (CY_U3P_SIB_DSR_DEF_VAL << 16), 0);
    }

    return CY_U3P_SUCCESS;
}

/* This function continues a pending SD/MMC read/write operation. */
CyU3PReturnStatus_t
CyU3PCardMgrContinueReadWrite (
        uint8_t isRead,
        uint8_t portId,
        uint8_t socket,
        uint16_t numBlks)
{
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    uint32_t flag = CY_U3P_SIB_RDDCARD;

    /* Mask out DAT3_CHANGE interrupt while doing a write operation. */
    SIB->sdmmc[portId].intr_mask &= (~CY_U3P_SIB_DAT3_CHANGE);

    if (!isRead)
    {
        flag   = CY_U3P_SIB_WRDCARD;
        status = CyU3PCardMgrCheckStatus (portId);
        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }

        CyU3PTimerModify (&glSibCtxt[portId].writeTimer, (CY_U3P_SIB_WRITE_DATA_TIMEOUT + (numBlks * 5)), 0);
        CyU3PTimerStart (&glSibCtxt[portId].writeTimer);
    }

    /* Configure the block length and the number of blocks to be read. */
    CyU3PSibSetNumBlocks(portId, numBlks);

    /* Update the active socket number */
    CyU3PSibSetActiveSocket(portId, socket);
    SIB->sdmmc[portId].cs = (SIB->sdmmc[portId].cs & (~CY_U3P_SIB_SNDCMD) | flag);

    if (isRead)
    {
        /* Enable the SD Clock */
        SIB->sdmmc[portId].cs &= (~CY_U3P_SIB_SDMMC_CLK_DIS);
    }

    return status;
}

/* This function sets up the dma and the sib registers to write the data to the card. */
CyU3PReturnStatus_t
CyU3PCardMgrSetupWrite (
        uint8_t portId,
        uint8_t unit,
        uint8_t socket,
        uint16_t numWriteBlks,
        uint32_t blkAddr)
{
    uint8_t cmd = CY_U3P_SD_MMC_CMD25_WRITE_MULTIPLE_BLOCK;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    CyU3PSibConvertAddr(portId, blkAddr);
    /* Wait until the storage device stops signalling busy state. */
    while ((SIB->sdmmc[portId].status & CY_U3P_SIB_DAT0_STAT) == 0);

    /* If the SIB data state machine is busy, reset the SIB. */
    if (SIB->sdmmc[portId].status & CY_U3P_SIB_DATA_SM_BUSY)
    {
        CyU3PSibResetSibCtrlr (portId);
    }

    /* Configure the block length and the number of blocks to be written. */
    CyU3PSibSetNumBlocks(portId, numWriteBlks);
    CyU3PSibSetBlockLen(portId, CY_U3P_SIB_BLOCK_SIZE);

    /* Update the active socket number */
    CyU3PSibSetActiveSocket(portId, socket);

    /* Mask out DAT3_CHANGE interrupt while doing a write operation. */
    SIB->sdmmc[portId].intr_mask &= (~CY_U3P_SIB_DAT3_CHANGE);

    /* Fill in the command argument buffer with the address. */
    CyU3PCardMgrSendCmd (portId, cmd, CY_U3P_SD_MMC_R1_RESP_BITS, blkAddr, 0);
    status = CyU3PCardMgrWaitForCmdResp (portId);
    if (status == CY_U3P_SUCCESS)
    {
        CyU3PTimerModify (&glSibCtxt[portId].writeTimer, (CY_U3P_SIB_WRITE_DATA_TIMEOUT + (numWriteBlks * 5)), 0);
        CyU3PTimerStart (&glSibCtxt[portId].writeTimer);
        SIB->sdmmc[portId].cs = (SIB->sdmmc[portId].cs & (~CY_U3P_SIB_SNDCMD) | CY_U3P_SIB_WRDCARD);
    }

    return status;
}

/* This function sets up the dma and the sib registers to read data from the card. */
CyU3PReturnStatus_t
CyU3PCardMgrSetupRead (
        uint8_t portId,
        uint8_t unit,
        uint8_t socket,
        uint16_t numReadBlks,
        uint32_t blkAddr)
{
    uint8_t cmd = CY_U3P_SD_MMC_CMD18_READ_MULTIPLE_BLOCK;

    CyU3PSibConvertAddr(portId, blkAddr);
    while ((SIB->sdmmc[portId].status & CY_U3P_SIB_DAT0_STAT) == 0);

    /* If the SIB data state machine is busy, reset the SIB. */
    if (SIB->sdmmc[portId].status & CY_U3P_SIB_DATA_SM_BUSY)
    {
        CyU3PSibResetSibCtrlr (portId);
    }

    /* Configure the block length and the number of blocks to be read. */
    CyU3PSibSetNumBlocks(portId, numReadBlks);
    CyU3PSibSetBlockLen(portId, CY_U3P_SIB_BLOCK_SIZE);

    /* Update the active socket number */
    CyU3PSibSetActiveSocket(portId, socket);

    /* Mask out DAT3_CHANGE interrupt while doing a read operation. */
    SIB->sdmmc[portId].intr_mask &= (~CY_U3P_SIB_DAT3_CHANGE);

    CyU3PCardMgrSendCmd (portId, cmd, CY_U3P_SD_MMC_R1_RESP_BITS, blkAddr, CY_U3P_SIB_RDDCARD);
    return CyU3PCardMgrWaitForCmdResp (portId);
}

/* This function is does the tuning process of the SD 3.0 cards. */
CyU3PReturnStatus_t
CyU3PCardMgrSDTuning (
        uint8_t portId)
{
    CyU3PDmaBuffer_t dmaBuffer;
    CyU3PReturnStatus_t status = CY_U3P_ERROR_FAILURE;
    uint8_t cnt;

    dmaBuffer.buffer = glSibBuffer;
    dmaBuffer.size   = CY_U3P_SIB_BLOCK_SIZE;
    dmaBuffer.count  = CY_U3P_SIB_BLOCK_SIZE;
    CyU3PSibSetBlockLen (portId, CY_U3P_SIB_BLOCK_SIZE);

    /* Tuning process. Section 4.2.4.5 of SD3.0 spec. */
    for (cnt = 0; cnt < 40; cnt ++)
    {
        CyU3PDmaChannelSetupRecvBuffer (CyU3PSibGetDmaChannelHandle, &dmaBuffer);
        CyU3PCardMgrSendCmd (portId, CY_U3P_SD_CMD19_SEND_TUNING_PATTERN, CY_U3P_SD_MMC_R1_RESP_BITS, 0,
                CY_U3P_SIB_RDDCARD);

        status = CyU3PCardMgrWaitForCmdResp (portId);
        if (status != CY_U3P_SUCCESS)
        {
            continue;
        }

        /* Wait for the block received interrupt */
        status = CyU3PCardMgrWaitForBlkXfer (portId, CY_U3P_SIB_BLOCKS_RECEIVED);
        if (status == CY_U3P_SUCCESS)
        {
            if (CyU3PMemCmp (glSibBuffer, glTuningBlock, 64) == 0)
            {
                break;
            }
        }
        else
            break;
    }

    return status;
}

#ifdef CYU3P_STORAGE_SDIO_SUPPORT

/* Reset the SDIO device. */
static CyU3PReturnStatus_t
CyU3PSdioCardResetVoltageSwitch (
        uint8_t portId,
        uint8_t lowVoltage)
{
    uint8_t data;
    CyU3PReturnStatus_t status=CY_U3P_SUCCESS;

    /* Send IO Reset down to the card. */
    data   = CY_U3P_SDIO_RESET;
    status = CyU3PSdioByteReadWrite(portId, CY_U3P_SDIO_CIA_FUNCTION, CY_U3P_SDIO_WRITE, 0,
            CY_U3P_SDIO_REG_IO_ABORT, &data);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Deselect the card */
    status = CyU3PCardMgrSelectCard(portId,0);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    while ((SIB->sdmmc[portId].status & CY_U3P_SIB_DAT0_STAT) == 0);
    glCardCtxt[portId].cardRCA = 0;

    /* Reset Clocking */
    CyU3PCardMgrSetClockFreq (portId, CY_U3P_CLOCK_DIVIDER_400);
    SIB->sdmmc[portId].mode_cfg = CY_U3P_SIB_SDMMC_MODE_CFG_DEFAULT;

    /* Reinitialize the IO portions of the card. */
    status = CyU3PCardMgrSdioInit (portId, lowVoltage);
    return status;
}

/* Query Card CCCR Parameters */
CyU3PReturnStatus_t
CyU3PSdioQueryCard (
        uint8_t portId,
        CyU3PSdioCardRegs_t* data)
{

    if ((glCardCtxt[portId].cardType != CY_U3P_SIB_DEV_SDIO ) &&
            (glCardCtxt[portId].cardType != CY_U3P_SIB_DEV_SDIO_COMBO))
    {
        return CY_U3P_ERROR_NOT_SUPPORTED;
    }

    data->isMemoryPresent   = glCardCtxt[portId].isMemoryPresent;
    data->numberOfFunctions = glCardCtxt[portId].numberOfFunction;
    data->CCCRVersion       = glCardCtxt[portId].CCCRVersion;
    data->sdioVersion       = glCardCtxt[portId].sdioVersion;
    data->cardCapability    = glCardCtxt[portId].cardCapability;
    data->cardSpeed         = glCardCtxt[portId].cardSpeed;
    data->fn0BlockSize      = glCardCtxt[portId].fnBlkSize[0];
    data->addrCIS           = glCardCtxt[portId].addrCIS;
    data->manufacturerId    = glCardCtxt[portId].manufactureId;
    data->manufacturerInfo  = glCardCtxt[portId].manufactureInfo;
    data->uhsSupport        = glCardCtxt[portId].uhsSupport;
    data->supportsAsyncIntr = glCardCtxt[portId].supportsAsyncIntr;
    return CY_U3P_SUCCESS;
}

/* Reset and re-initialize the SDIO card. */
CyU3PReturnStatus_t
CyU3PSdioCardReset (
        uint8_t portId)
{
    return CyU3PSdioCardResetVoltageSwitch (portId, 0);
}

/* Initializes the SDIO Card  */
static CyU3PReturnStatus_t
CyU3PCardMgrSdioInit (
        uint8_t portId,
        uint8_t lowVoltageSwitch)
{
    CyU3PReturnStatus_t status;
    uint32_t addressCIS = 0;
    uint16_t clkDiv;
    uint8_t  data, tupleSize;
    uint8_t  is4BitSupported = 0;
    uint8_t  clkSignal;

    /* Check if Card is SDIO and Set the Voltage to 3.3V */
    status = CyU3PCardMgrSelOpConditionsVS (portId, CY_U3P_SIB_DEV_SDIO, lowVoltageSwitch);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Loop until a non-zero RCA has been published. */
    while (glCardCtxt[portId].cardRCA == 0)
    {
        /* Send CMD3 to fetch the card's RCA */
        CyU3PCardMgrSendCmd (portId, CY_U3P_SD_CMD3_SEND_RELATIVE_ADDR, CY_U3P_SD_R6_RESP_BITS, 0, 0);
        status = CyU3PCardMgrWaitForCmdResp (portId);
        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }

        if (((CY_U3P_SIB_SDMMC_RESP_IDX(portId) & CY_U3P_SIB_CMD_MASK) == CY_U3P_SD_CMD3_SEND_RELATIVE_ADDR))
        {
            glCardCtxt[portId].cardRCA = (SIB->sdmmc[portId].resp_reg0 >> 16);
        }
        else
        {
            return CY_U3P_ERROR_CARD_WRONG_RESPONSE;
        }
    }

    status = CyU3PCardMgrSelectCard (portId,1);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Read UHS Support Bytes*/
    status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION, CY_U3P_SDIO_READ, 0,
            CY_U3P_SDIO_REG_UHS_I_SUPPORT, &data);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    glCardCtxt[portId].uhsSupport = data;
    if ((glSibIntfParams[portId].lowVoltage == CyTrue) && ((data & 0x07) > 0) && (!lowVoltageSwitch))
    {
        return CyU3PSdioCardResetVoltageSwitch (portId, 1);
    }

    /* Reading the SDIO Version and CCCR Version */
    status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION, CY_U3P_SDIO_READ, 0,
            CY_U3P_SDIO_REG_CCCR_REVISION, &data);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Reading the SDIO and CCCR Version */
    glCardCtxt[portId].sdioVersion = data & 0xF0;
    glCardCtxt[portId].CCCRVersion = data & 0x0F;
    glCardCtxt[portId].writeable   = 1;

    /* Reading the SD Version */
    status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION, CY_U3P_SDIO_READ, 0,
            CY_U3P_SDIO_REG_SD_SPEC_REVISION, &data);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    glCardCtxt[portId].sdioVersion |= (data)&0x0F;

    /* Reading the Card Cabablity */
    status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION, CY_U3P_SDIO_READ, 0,
            CY_U3P_SDIO_REG_CARD_CAPABILITY, &data);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    glCardCtxt[portId].cardCapability = data;
    glCardCtxt[portId].busWidth       = CY_U3P_CARD_BUS_WIDTH_4_BIT;

    status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION, CY_U3P_SDIO_READ, 0,
            CY_U3P_SDIO_REG_CCCR_HIGH_SPEED, &data);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Clear BSSx bits */
    data &= 0xF1;
    if (((glCardCtxt[portId].uhsSupport & 0x07) > 0) && (glCardCtxt[portId].opVoltage == CY_U3P_SIB_VOLTAGE_LOW))
    {
        switch (glSibIntfParams[portId].maxFreq)
        {
            case CY_U3P_SIB_FREQ_104MHZ:
                if ((glSibIntfParams[portId].useDdr) && ((glCardCtxt[portId].uhsSupport & CY_U3P_SDIO_UHS_SDDR50) > 0))
                {
                    /* CY_U3P_SDIO_UHS_SDDR50 - Set clock upto 50MHz DDR */
                    glCardCtxt[portId].uhsIOpMode = CY_U3P_SIB_UHS_I_DDR50;
                    glCardCtxt[portId].cardSpeed  = CY_U3P_SDIO_SDDR50_SPEED;

                    data      |= 0x08;
                    clkSignal  = 0x0B;
                    clkDiv     = CY_U3P_CLOCK_DIVIDER_104M;
                }
                else
                {
                    glCardCtxt[portId].uhsIOpMode = CY_U3P_SIB_UHS_I_SDR50;
                    glCardCtxt[portId].cardSpeed  = CY_U3P_SDIO_SSDR50_SPEED;

                    data      |= 0x04;
                    clkSignal  = 0x04;
                    clkDiv     = CY_U3P_CLOCK_DIVIDER_104M;
                }
                break;

            case CY_U3P_SIB_FREQ_52MHZ:
                glCardCtxt[portId].uhsIOpMode = CY_U3P_SIB_UHS_I_SDR25;
                glCardCtxt[portId].cardSpeed  = CY_U3P_SDIO_SSDR25_SPEED;
                data      |= 0x02;
                clkSignal  = 0x03;
                clkDiv     = CY_U3P_CLOCK_DIVIDER_46M;
                break;

            default:
                glCardCtxt[portId].uhsIOpMode = CY_U3P_SIB_UHS_I_SDR25;
                glCardCtxt[portId].cardSpeed  = CY_U3P_SDIO_SSDR12_SPEED;
                clkSignal = 0x02;
                clkDiv    = CY_U3P_CLOCK_DIVIDER_20M;
                break;
        }

        is4BitSupported = 1;
    }
    else
    {
        /* Check if the card is low speed or Full speed */
        if (glCardCtxt[portId].cardCapability & CY_U3P_SDIO_CARD_CAPABLITY_LSC)
        {
            /* Low speed card, operating frequency = 400 KHz. */
            glCardCtxt[portId].cardSpeed = CY_U3P_SDIO_LOW_SPEED;
            clkSignal = 0x0;
            clkDiv    = CY_U3P_CLOCK_DIVIDER_400;

            if (data & CY_U3P_SDIO_CARD_CAPABLITY_4BLS)
            {
                /* Support the 4 Bit Mode */
                is4BitSupported = 1;
                glCardCtxt[portId].busWidth = CY_U3P_CARD_BUS_WIDTH_4_BIT;
            }
            else
            {
                is4BitSupported = 0;
                glCardCtxt[portId].busWidth = CY_U3P_CARD_BUS_WIDTH_1_BIT;
            }
        }
        else
        {
            /* Full speed or high speed card always supports 4 bit mode. */
            is4BitSupported = 1;

            if ((data & 0x01) && (glSibIntfParams[portId].maxFreq >= CY_U3P_SIB_FREQ_52MHZ))
            {
                /* High speed card, operating frequency = 46 MHz. */
                glCardCtxt[portId].cardSpeed = CY_U3P_SDIO_HIGH_SPEED;
                data      |= 0x02;
                clkSignal  = 0x01;
                clkDiv     = CY_U3P_CLOCK_DIVIDER_46M;
            }
            else
            {
                /* Full speed card, operating frequency = 20 MHz. */
                glCardCtxt[portId].cardSpeed = CY_U3P_SDIO_FULL_SPEED;
                clkSignal = 0x00;
                clkDiv    = CY_U3P_CLOCK_DIVIDER_20M;
            }
        }
    }

    status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION, CY_U3P_SDIO_WRITE, 0,
            CY_U3P_SDIO_REG_CCCR_HIGH_SPEED, &data);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Disable the SD clock. */
    SIB->sdmmc[portId].cs |= (CY_U3P_SIB_SDMMC_CLK_DIS);
    SIB->sdmmc[portId].mode_cfg = ((CY_U3P_SIB_SDMMC_MODE_CFG_DEFAULT & ~CY_U3P_SIB_MODE_MASK) |
            (clkSignal << CY_U3P_SIB_SIGNALING_POS) | (CY_U3P_SIB_DEV_SD << CY_U3P_SIB_MODE_POS) |
            (is4BitSupported << CY_U3P_SIB_DATABUSWIDTH_POS));

    /* Switch SIB frequency */
    CyU3PCardMgrSetClockFreq (portId, clkDiv);

    /* Enable the SD Clk */
    SIB->sdmmc[portId].cs &= (~CY_U3P_SIB_SDMMC_CLK_DIS);

    /* Setting the Bus width for the Card */
    if (is4BitSupported)
        data = 0x02;

    status = CyU3PSdioByteReadWrite(portId, CY_U3P_SDIO_CIA_FUNCTION, CY_U3P_SDIO_WRITE, 0,
            CY_U3P_SDIO_REG_BUS_INTERFACE_CONTROL, &data);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    status = CyU3PSdioGetCISAddress (portId, CY_U3P_SDIO_CIA_FUNCTION, &addressCIS);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    CyU3PMemSet (glSibBuffer, 0, 256);
    glCardCtxt[portId].addrCIS = addressCIS;
    status = CyU3PSdioGetTuples (portId, CY_U3P_SDIO_CIA_FUNCTION, CY_U3P_SDIO_CISTPL_MANFID, addressCIS,
            glSibBuffer,  &tupleSize);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    glCardCtxt[portId].manufactureId   = CY_U3P_MAKEWORD (glSibBuffer[1], glSibBuffer[0]);
    glCardCtxt[portId].manufactureInfo = CY_U3P_MAKEWORD (glSibBuffer[3], glSibBuffer[2]);

    CyU3PMemSet (glSibBuffer, 0, 256);
    status = CyU3PSdioGetTuples (portId, CY_U3P_SDIO_CIA_FUNCTION, CY_U3P_SDIO_CISTPL_FUNCE, addressCIS,
            glSibBuffer,  &tupleSize);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Setting the block size by reading from tuple. */
    glCardCtxt[portId].fnBlkSize[0] = CY_U3P_MAKEWORD (glSibBuffer[2], glSibBuffer[1]);
    status = CyU3PSdioSetBlockSize (portId, CY_U3P_SDIO_CIA_FUNCTION, glCardCtxt[portId].fnBlkSize[0]);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Store the SDIO device information. */
    CyU3PSibUpdateLunInfo (portId, 0, CY_U3P_SIB_LUN_DATA, CY_U3P_SIB_LOCATION_USER, 0, 0x20000);
    return status;
}

CyU3PReturnStatus_t
CyU3PSdioReadWaitEnable (
        uint8_t portId,
        uint8_t isRdWtEnable)
{
    /* Check if READ WAIT is supported. */
    if ((glCardCtxt[portId].cardCapability & CY_U3P_SDIO_CARD_CAPABLITY_SRW) == 0)
    {
        return CY_U3P_ERROR_NOT_SUPPORTED;
    }

    if (isRdWtEnable)
    {
        SIB->sdmmc[portId].cs |= CY_U3P_SIB_SDIO_READ_WAIT_EN;
    }
    else
    {
        SIB->sdmmc[portId].cs &= ~(CY_U3P_SIB_SDIO_READ_WAIT_EN);
    }

    return CY_U3P_SUCCESS;
}

/* Not Supported as this functionality is untested. */
CyU3PReturnStatus_t
CyU3PSdioSuspendResume(
        uint8_t portId,
        uint8_t functionNumber,
        uint8_t operation,
        uint8_t *isDataAvailable)
{
#if 1
    return CY_U3P_ERROR_NOT_SUPPORTED;
#else
    uint8_t data, trials = 0;
    CyU3PReturnStatus_t status;

    if ((functionNumber > 7) || (functionNumber == 0) || (isDataAvailable == NULL))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    if ((glCardCtxt[portId].cardCapability & CY_U3P_SDIO_CARD_CAPABLITY_SBS) == 0)
    {
        return CY_U3P_ERROR_NOT_SUPPORTED;
    }

    *isDataAvailable = 0;
    if (operation == CY_U3P_SDIO_RESUME)
    {
        data = functionNumber;
        status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION, CY_U3P_SDIO_WRITE,
                CY_U3P_SDIO_READ_AFTER_WRITE, CY_U3P_SDIO_REG_FUNCTION_SELECT, &data);
        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }

        SIB->sdmmc[portId].intr_mask |= CY_U3P_SIB_SDIO_INTR;
        *isDataAvailable = (data >> 7) & 1;
    }
    else
    {
        data = 0x2;

        /* Request for releasing the Bus */
        status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION, CY_U3P_SDIO_WRITE,
                CY_U3P_SDIO_READ_AFTER_WRITE, CY_U3P_SDIO_REG_BUS_SUSPEND, &data);
        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }

        SIB->sdmmc[portId].intr_mask |= CY_U3P_SIB_SDIO_INTR;
        trials = 100;

        /* Checking the Bus suspend status i.e. wait for the BS field of BUS suspend register to be 0 */
        while ((data & 1) && (trials > 0))
        {
            status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION, CY_U3P_SDIO_READ, 0,
                    CY_U3P_SDIO_REG_BUS_SUSPEND, &data);
            if (status != CY_U3P_SUCCESS)
            {
                return status;
            }

            trials--;
        }

        if ((data & 1) != 0)
        {
            return CY_U3P_ERROR_RESUME_FAILED;
        }
    }

    return status;
#endif
}

/* Abort the ongoing read or write operation */
CyU3PReturnStatus_t
CyU3PSdioAbortFunctionIO (
        uint8_t portId,
        uint8_t funcNo)
{
    CyU3PReturnStatus_t status;
    uint32_t intrMask = 0;

    /* IO Abort can only be performed on functions 1-7 */
    if ((funcNo == 0) || (funcNo > 7))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    if (glSibCtxt[portId].inUse)
    {
        intrMask = (CY_U3P_SIB_BLOCKS_RECEIVED | CY_U3P_SIB_RD_DATA_TIMEOUT | CY_U3P_SIB_BLOCK_COMP);
        SIB->sdmmc[portId].intr_mask &= intrMask;

        /* Abort the Card Reading */
        SIB->sdmmc[portId].cs |= (CY_U3P_SIB_CLR_RDDCARD | CY_U3P_SIB_CLR_WRDCARD);

        /* Writing into the IO Abort Register */
        status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION, CY_U3P_SDIO_WRITE, 0,
                CY_U3P_SDIO_REG_IO_ABORT, &funcNo);
        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }

        glSibCtxt[portId].inUse = CyFalse;
    }

    return CY_U3P_SUCCESS;
}

/* Enable of disable the Interrupt for the function */
CyU3PReturnStatus_t
CyU3PSdioInterruptControl (
        uint8_t  portId,
        uint8_t  functionNumber,
        uint8_t  operation,
        uint8_t *intEnReg)
{
    /* Check if the Interrupt is enabled with the same as above */
    CyU3PReturnStatus_t status;
    uint32_t regAddr;
    uint8_t data;

    if ((functionNumber > 7) || ((operation == CY_U3P_SDIO_CHECK_INT_ENABLE_REG) && (intEnReg == 0)))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    /* Read Current status of Interrupts on the Card */
    regAddr = CY_U3P_SDIO_REG_IO_INTR_ENABLE;
    status  = CyU3PSdioByteReadWrite(portId, CY_U3P_SDIO_CIA_FUNCTION, CY_U3P_SDIO_READ, 0, regAddr, &data);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    switch (operation)
    {
        case CY_U3P_SDIO_CHECK_INT_ENABLE_REG:
            *intEnReg = data;
            return status;

        case CY_U3P_SDIO_DISABLE_INT:
            /* Disable SDIO interrupts on FX3 if all interrupts are being turned off on the SDIO card. */
            if (functionNumber == 0)
            {
                SIB->sdmmc[portId].intr_mask &= (~CY_U3P_SIB_SDIO_INTR);
            }
            data &= (~(1 << functionNumber));
            break;

        case CY_U3P_SDIO_ENABLE_INT:
            /* Ensure SDIO interrupts are enabled on FX3 whenever any SDIO card interrupts are enabled. */
            SIB->sdmmc[portId].intr_mask |= (CY_U3P_SIB_SDIO_INTR);
            data |= ((1 << functionNumber) | CY_U3P_SDIO_ENABLE_INT);
            break;

        default:
            return CY_U3P_ERROR_BAD_CMD_ARG;
    }

    status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION, CY_U3P_SDIO_WRITE,
            CY_U3P_SDIO_READ_AFTER_WRITE, regAddr, &data);
    if ((status == CY_U3P_SUCCESS) && (intEnReg != 0))
        *intEnReg = data;

    return status;
}

/* Function to convert the error from the response into the CyU3PReturnStatus_t */
CyU3PReturnStatus_t
CyU3PSdioErrorStatus(
        uint8_t errorCode)
{
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    if (errorCode & 1)
    {
        status = CY_U3P_ERROR_BAD_CMD_ARG;
    }
    else if (errorCode & 2)
    {
        status = CY_U3P_ERROR_INVALID_FUNCTION;
    }
    else if (errorCode & 4)
    {
        status = CY_U3P_ERROR_SDIO_UNKNOWN;
    }
    else if (errorCode & 0x40)
    {
        status = CY_U3P_ERROR_ILLEGAL_CMD;
    }
    else if (errorCode & 0x80)
    {
        status = CY_U3P_ERROR_CRC;
    }
    return status;
}

/* Send the CMD52 and waits for the response R5 for reading the data */
CyU3PReturnStatus_t
CyU3PSdioByteReadWrite(
        uint8_t  portId,
        uint8_t  functionNumber,
        uint8_t  isWrite,
        uint8_t  readAfterWriteFlag,
        uint32_t registerAddr,
        uint8_t  *data)
{
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    uint32_t cmdArg = 0;
    uint32_t tmp;

    if ((glCardCtxt[portId].cardType != CY_U3P_SIB_DEV_SDIO ) &&
            (glCardCtxt[portId].cardType != CY_U3P_SIB_DEV_SDIO_COMBO))
    {
        return CY_U3P_ERROR_NOT_SUPPORTED;
    }

    if ((functionNumber > 7) || (registerAddr > 0x1FFFF) || (data == NULL))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    cmdArg |= (isWrite & 1) << 31;
    cmdArg |= (functionNumber << 28);
    cmdArg |= ((readAfterWriteFlag & 1) << 27);
    cmdArg |= ((registerAddr & 0x1FFFF) << 9);
    cmdArg |= ((*data) & 0xFF);

    CyU3PCardMgrSendCmd (portId, CY_U3P_SD_MMC_CMD52_IO_RW_DIRECT, CY_U3P_SD_MMC_R5_RESP_BITS, cmdArg, 0);
    status = CyU3PCardMgrWaitForCmdResp (portId);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    tmp = SIB->sdmmc[portId].resp_reg0;
    if ((tmp & 0x3000) == 0)
        return CY_U3P_ERROR_CARD_NOT_ACTIVE;
    if ((tmp & 0xCF00) > 0)
        return (CyU3PSdioErrorStatus ((tmp & 0xCF00) >> 8));

    if ((!isWrite) || (readAfterWriteFlag))
        *data = tmp & 0xFF;
    return status;
}

/* Get the Block Size of the IO */
CyU3PReturnStatus_t
CyU3PSdioGetBlockSize (
        uint8_t   portId,
        uint8_t   functionNumber,
        uint16_t *blockSize)
{
    uint32_t baseAddress = (functionNumber << 8);
    uint8_t data;
    CyU3PReturnStatus_t status;

    if ((blockSize == NULL) || (functionNumber > 7))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    /* Read D0 byte (lower 8 bits) of block size. */
    status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION, CY_U3P_SDIO_READ, 0,
            baseAddress | CY_U3P_SDIO_REG_FBR_IO_BLOCKSIZE_D0, &data);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    *blockSize = data;

    /* Read D1 byte (upper 8 bits) of block size. */
    status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION, CY_U3P_SDIO_READ, 0,
            baseAddress | CY_U3P_SDIO_REG_FBR_IO_BLOCKSIZE_D1, &data);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    *blockSize |= data << 8;
    glCardCtxt[portId].fnBlkSize[functionNumber] = *blockSize;
    return status;
}

/* Set Block Size for IO */
CyU3PReturnStatus_t
CyU3PSdioSetBlockSize (
        uint8_t  portId,
        uint8_t  functionNumber,
        uint16_t blockSize)
{
    CyU3PReturnStatus_t status;
    uint32_t baseAddress = (functionNumber << 8);
    uint8_t  data;

    if (functionNumber > 7)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    if ((glCardCtxt[portId].cardCapability & CY_U3P_SDIO_CARD_CAPABLITY_SMB) == 0)
    {
        return CY_U3P_ERROR_NOT_SUPPORTED;
    }

    /* Write the D0 byte (lower 8 bits) of block size */
    data = (blockSize & 0xFF);

    status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION, CY_U3P_SDIO_WRITE, 0,
            baseAddress | CY_U3P_SDIO_REG_FBR_IO_BLOCKSIZE_D0, &data);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Write the D1 byte (upper 8 bits) of bloc size */
    data = ((blockSize >> 8) & 0xFF);
    status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION, CY_U3P_SDIO_WRITE, 0,
            baseAddress | CY_U3P_SDIO_REG_FBR_IO_BLOCKSIZE_D1, &data);
    if (status == CY_U3P_ERROR_BAD_CMD_ARG)
    {
        /* This seems to fail once immediately after writing to the D0 byte on the Arasan Device. Works when retried. */
        status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION, CY_U3P_SDIO_WRITE, 0,
                baseAddress | CY_U3P_SDIO_REG_FBR_IO_BLOCKSIZE_D1, &data);
    }

    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Store block size (for use in Extended IO calls) */
    glCardCtxt[portId].fnBlkSize[functionNumber] = blockSize;
    return status;
}

/* Initiate the Read and Write in bulk or data */
CyU3PReturnStatus_t
CyU3PSdioExtendedReadWrite (
        uint8_t  portId,
        uint8_t  functionNumber,
        uint8_t  isWrite,
        uint8_t  blockMode,
        uint8_t  opCode,
        uint32_t registerAddr,
        uint16_t transferCount,
        uint16_t sckId)
{
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    uint32_t cmdArg = 0, cmdflags;
    uint16_t noOfBlocks = 0;
    uint16_t blockSize;
    uint8_t data = 0;
    uint8_t bitFunc = (1 << functionNumber);
    uint8_t trials = 0;

    if ((glCardCtxt[portId].cardType != CY_U3P_SIB_DEV_SDIO) &&
            (glCardCtxt[portId].cardType != CY_U3P_SIB_DEV_SDIO_COMBO))
    {
        return CY_U3P_ERROR_NOT_SUPPORTED;
    }

    if ((functionNumber > 7) || (registerAddr > 0x1FFFF) || (transferCount > 0x1FF))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    if (blockMode)
    {
        /* FX3S cannot handle infinite transfers (transferCount == 0). */
        if (!transferCount)
        {
            return CY_U3P_ERROR_BAD_ARGUMENT;
        }

        /* Check if card supports multi-block transfers */
        if ((glCardCtxt[portId].cardCapability & CY_U3P_SDIO_CARD_CAPABLITY_SMB) == 0)
        {
            return CY_U3P_ERROR_NOT_SUPPORTED;
        }

        noOfBlocks = transferCount;
        blockSize  = glCardCtxt[portId].fnBlkSize[functionNumber];
    }
    else
    {
        noOfBlocks = 1;
        blockSize  = (transferCount != 0) ? transferCount : 512;
    }

    /* Create command arguments*/
    cmdArg  = (isWrite & CY_U3P_SDIO_WRITE) << 31;
    cmdArg |= ((functionNumber & 7) << 28);
    cmdArg |= ((blockMode & CY_U3P_SDIO_RW_BLOCK_MODE) << 27);
    cmdArg |= ((opCode & CY_U3P_SDIO_RW_ADDR_AUTO_INCREMENT) << 26);
    cmdArg |= ((registerAddr & 0x1FFFF) << 9);
    cmdArg |= (transferCount & 0x1FF);

    if (glSibCtxt[portId].inUse == CyTrue)
    {
        return CY_U3P_ERROR_DEVICE_BUSY;
    }

    glSibCtxt[portId].inUse = CyTrue;

    /* Setup SIB for transfer */
    CyU3PSibSetNumBlocks    (portId, noOfBlocks);
    CyU3PSibSetBlockLen     (portId, blockSize);
    CyU3PSibSetActiveSocket (portId, sckId);

    /* Setup the Comand for IO */
    if (!isWrite)
    {
        cmdflags = CY_U3P_SIB_RDDCARD;
        SIB->sdmmc[portId].intr = (CY_U3P_SIB_BLOCKS_RECEIVED | CY_U3P_SIB_RD_DATA_TIMEOUT);
        glSibCtxt[portId].isRead = CyTrue;
    }
    else
    {
        cmdflags = CY_U3P_SIB_WRDCARD;
        SIB->sdmmc[portId].intr = (CY_U3P_SIB_BLOCK_COMP);
        glSibCtxt[portId].isRead = CyFalse;
    }

    /* Disable DAT3 based card detection */
    SIB->sdmmc[portId].intr_mask &= (~CY_U3P_SIB_DAT3_CHANGE);

    /* Check if the function is ready for IO */
    if (functionNumber != 0)
    {
        trials = 100;
        while (((data & bitFunc) == 0) && (trials > 0))
        {
            status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION, CY_U3P_SDIO_READ, 0,
                    CY_U3P_SDIO_REG_IO_READY, &data);
            if (status != CY_U3P_SUCCESS)
            {
                return status;
            }

            trials--;
        }

        if ((data & bitFunc) == 0)
            return CY_U3P_ERROR_INVALID_FUNCTION;
    }

    /* Send IO command */
    CyU3PCardMgrSendCmd (portId, CY_U3P_SD_MMC_CMD53_IO_RW_EXTENDED, CY_U3P_SD_MMC_R5_RESP_BITS, cmdArg, cmdflags);

    status = CyU3PCardMgrWaitForCmdResp (portId);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PSdioAbortFunctionIO (portId, functionNumber);
        return status;
    }

    data = (uint8_t)(SIB->sdmmc[portId].resp_reg0 >> 8);
    if ((data & 0x30) == 0)
    {
        return CY_U3P_ERROR_CARD_NOT_ACTIVE;
    }

    if ((data & 0xCF) != 0)
    {
        /* Sending the Appropriate Error. Masking the IO_CURRENT_STATE field in the response */
        return (CyU3PSdioErrorStatus (data & 0xCF));
    }
    return status;
}

/* Read the CIS address from the function */
CyU3PReturnStatus_t
CyU3PSdioGetCISAddress (
        uint8_t   portId,
        uint8_t   functionNumber,
        uint32_t *addressCIS)
{
    CyU3PReturnStatus_t status;
    uint32_t baseAddress = functionNumber << 8;
    uint8_t data = 0, addr[2] = {0};

    if ((addressCIS == NULL) || (functionNumber > 7))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    /* Read the CIS address bytes located at addresses 0x0n09 to 0x0nB of Function 0 (CIA),
       where n is the function number whose CIS address is being requested. */
    status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION, CY_U3P_SDIO_READ, 0,
            baseAddress | CY_U3P_SDIO_REG_FBR_CIS_PTR_DO, &data);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    addr[0] = data;

    status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION, CY_U3P_SDIO_READ, 0,
            baseAddress | CY_U3P_SDIO_REG_FBR_CIS_PTR_D1, &data);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    addr[1] = data;

    status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION, CY_U3P_SDIO_READ, 0,
            baseAddress | CY_U3P_SDIO_REG_FBR_CIS_PTR_D2, &data);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    *addressCIS = (uint32_t)((data << 16) | (addr[1] << 8) | addr[0]);
    return status;
}

/* Read the CIS register for the function */
CyU3PReturnStatus_t
CyU3PSdioGetTuples (
        uint8_t   portId,
        uint8_t   funcNo,
        uint8_t   tupleId,
        uint32_t  addrCIS,
        uint8_t  *buffer,
        uint8_t  *tupleSize)
{
    CyU3PReturnStatus_t status;
    uint8_t tuple, i, data;

    if ((buffer == NULL) || (tupleSize ==NULL))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    *tupleSize = 0;

    do
    {
        addrCIS += (*tupleSize); /* Go to the Next Link in the Chain */
        status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION, CY_U3P_SDIO_READ, 0, addrCIS++, &tuple);
        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }

        /* Check for Start of Chain and End of Chain */
        if (tuple == CY_U3P_SDIO_CISTPL_NULL)
        {
            /* First Tuple in chain. No length for this tuple. */
            continue;
        }

        if (tuple == CY_U3P_SDIO_CISTPL_END)
        {
            /* End of tuple chain */
            return CY_U3P_ERROR_TUPLE_NOT_FOUND;
        }

        /* Read the Tuple Size */
        status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION, CY_U3P_SDIO_READ, 0, addrCIS++, tupleSize);
        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }

    } while (tuple != tupleId);


    /* Found our desired tuple. Read it into the buffer. Use the byte wise read for now. */
    for (i = 0; i < *tupleSize; i++)
    {
        status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION, CY_U3P_SDIO_READ, 0, addrCIS++, &data);
        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }

        buffer[i]  = data;
    }

    return status;
}

#endif

CyU3PReturnStatus_t
CyU3PCardMgrCompleteSDInit (
        uint8_t portId)
{
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    CyU3PDmaBuffer_t    dmaBuffer;
    uint8_t             state  = 0;
    CyBool_t            freqSet = CyFalse;

    /* This variable is used in multiple SetupRecv calls and so is initialized only once. */
    dmaBuffer.buffer = glSibBuffer;
    dmaBuffer.size   = CY_U3P_SIB_BLOCK_SIZE;
    dmaBuffer.count  = 0;
    dmaBuffer.status = 0;

    CyU3PDmaChannelReset (CyU3PSibGetDmaChannelHandle);

    /* Get the Erase Unit Size */
    status = CyU3PCardMgrGetEUSize (portId);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Send the APP_CMD(CMD55) command. Expect R1 response. */
    CyU3PCardMgrSendCmd (portId, CY_U3P_SD_MMC_CMD55_APP_CMD, CY_U3P_SD_MMC_R1_RESP_BITS,
            (glCardCtxt[portId].cardRCA << 16), 0);
    status = CyU3PCardMgrWaitForCmdResp (portId);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Check the card state bits to see if the card is in the transfer state. */
    state = ((CY_U3P_SIB_SDMMC_RESP_REG0(portId) >> 9) & 0x0F);
    if (state != CY_U3P_SD_MMC_TRANSFER)
    {
        return CY_U3P_ERROR_CARD_WRONG_STATE;
    }

    CyU3PSibSetBlockLen(portId, 8);
    CyU3PSibSetNumBlocks(portId, 0x01);

    dmaBuffer.count = 0;
    status = CyU3PDmaChannelSetupRecvBuffer (CyU3PSibGetDmaChannelHandle, &dmaBuffer);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Send the SEND_SCR (ACMD51) command. Expect R1 response. */
    CyU3PCardMgrSendCmd (portId, CY_U3P_SD_ACMD51_SEND_SCR, CY_U3P_SD_MMC_R1_RESP_BITS,
            (glCardCtxt[portId].cardRCA << 16), CY_U3P_SIB_RDDCARD);
    status = CyU3PCardMgrWaitForCmdResp (portId);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Wait for the block received interrupt */
    status = CyU3PCardMgrWaitForBlkXfer (portId, CY_U3P_SIB_BLOCKS_RECEIVED);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Check if 4 bit mode is supported */
    if ((glSibBuffer[1] & 0x04) > 0)
    {
        /* Send the APP_CMD(CMD55) command. Expect R1 response. */
        CyU3PCardMgrSendCmd (portId, CY_U3P_SD_MMC_CMD55_APP_CMD, CY_U3P_SD_MMC_R1_RESP_BITS,
                (glCardCtxt[portId].cardRCA << 16), 0);
        status = CyU3PCardMgrWaitForCmdResp (portId);
        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }

        /* Send the SET_BUS_WIDTH(ACMD6) command. Expect R1 response. */
        CyU3PCardMgrSendCmd (portId, CY_U3P_SD_ACMD6_SET_BUS_WIDTH, CY_U3P_SD_MMC_R1_RESP_BITS, 0x02, 0);
        status = CyU3PCardMgrWaitForCmdResp (portId);
        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }

        /* 4 bit mode is supported, switch the bus width to 4 bits. */
        glCardCtxt[portId].busWidth = CY_U3P_CARD_BUS_WIDTH_4_BIT;

        /* Update the mode config register with the bus width being used. */
        SIB->sdmmc[portId].mode_cfg = (SIB->sdmmc[portId].mode_cfg & ~CY_U3P_SIB_DATABUSWIDTH_MASK) |
            (0x01 << CY_U3P_SIB_DATABUSWIDTH_POS);
    }

    /* If SD spec version is 1.1 or above, check for higher frequency support. */
    if ((glSibBuffer[0] & 0x0F) > 0)
    {
        /* Expect 512 bits of data on the data lines. */
        CyU3PSibSetBlockLen (portId, 0x40);

        /* Query whether high speed mode is supported.
           CMD 6 with MSB of the cmd arg set to 0 is used to query the functionality
           of the card. 0xF has no influence on the selected functions. */
        dmaBuffer.count = 0;
        status = CyU3PDmaChannelSetupRecvBuffer (CyU3PSibGetDmaChannelHandle, &dmaBuffer);
        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }
        /* Send CMD6 to query the supported functions. */
        CyU3PCardMgrSendCmd (portId, CY_U3P_SD_MMC_CMD6_SWITCH, CY_U3P_SD_MMC_R1_RESP_BITS,
                CY_U3P_SD_SW_QUERY_FUNCTIONS, CY_U3P_SIB_RDDCARD);

        status = CyU3PCardMgrWaitForCmdResp (portId);
        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }

        /* Wait for the block received interrupt */
        status = CyU3PCardMgrWaitForBlkXfer (portId, CY_U3P_SIB_BLOCKS_RECEIVED);
        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }

        /* Switch to SDR50/DDR50 only if the frequency is enabled and operating voltage is 1.8 V. */
        if ((glSibIntfParams[portId].maxFreq == CY_U3P_SIB_FREQ_104MHZ) &&
                (glCardCtxt[portId].opVoltage == CY_U3P_SIB_VOLTAGE_LOW))
        {
            if ((glSibIntfParams[portId].useDdr) && ((glSibBuffer[13] & 0x10) != 0))
            {
                dmaBuffer.count = 0;
                status = CyU3PDmaChannelSetupRecvBuffer (CyU3PSibGetDmaChannelHandle, &dmaBuffer);
                if (status != CY_U3P_SUCCESS)
                {
                    return status;
                }

                CyU3PCardMgrSendCmd (portId, CY_U3P_SD_MMC_CMD6_SWITCH, CY_U3P_SD_MMC_R1_RESP_BITS, 0x80FFFFF4,
                        CY_U3P_SIB_RDDCARD);

                status = CyU3PCardMgrWaitForCmdResp (portId);
                if (status != CY_U3P_SUCCESS)
                {
                    return status;
                }

                /* Wait for the block received interrupt */
                status = CyU3PCardMgrWaitForBlkXfer (portId, CY_U3P_SIB_BLOCKS_RECEIVED);
                if (status != CY_U3P_SUCCESS)
                {
                    return status;
                }

                /* Check whether the function was switched. */
                if ((glSibBuffer[16] & 0x0F) != 0x04)
                {
                    return CY_U3P_ERROR_CMD_NOT_SUPPORTED;
                }

                /* Keep the SD clock disabled while making frequency changes. */
                SIB->sdmmc[portId].cs |= (CY_U3P_SIB_SDMMC_CLK_DIS);
                SIB->sdmmc[portId].mode_cfg = ((SIB->sdmmc[portId].mode_cfg & ~CY_U3P_SIB_SIGNALING_MASK) |
                        0x0B << CY_U3P_SIB_SIGNALING_POS);
                SIB->sdmmc[portId].cs &= (~CY_U3P_SIB_SDMMC_CLK_DIS);

                CyU3PCardMgrSetClockFreq (portId, CY_U3P_CLOCK_DIVIDER_104M);
                glCardCtxt[portId].uhsIOpMode = CY_U3P_SIB_UHS_I_DDR50;
                glCardCtxt[portId].ddrMode = CyTrue;
                freqSet = CyTrue;
            }
            else
            {
                if ((glSibBuffer[13] & 0x04) != 0)
                {
                    dmaBuffer.count = 0;
                    status = CyU3PDmaChannelSetupRecvBuffer (CyU3PSibGetDmaChannelHandle, &dmaBuffer);
                    if (status != CY_U3P_SUCCESS)
                    {
                        return status;
                    }

                    CyU3PCardMgrSendCmd (portId, CY_U3P_SD_MMC_CMD6_SWITCH, CY_U3P_SD_MMC_R1_RESP_BITS,
                            CY_U3P_SD_SW_UHS1_PARAM, CY_U3P_SIB_RDDCARD);

                    status = CyU3PCardMgrWaitForCmdResp (portId);
                    if (status != CY_U3P_SUCCESS)
                    {
                        return status;
                    }

                    /* Wait for the block received interrupt */
                    status = CyU3PCardMgrWaitForBlkXfer (portId, CY_U3P_SIB_BLOCKS_RECEIVED);
                    if (status != CY_U3P_SUCCESS)
                    {
                        return status;
                    }

                    /* Check whether the function was switched. */
                    if ((glSibBuffer[16] & 0x0F) != 0x02)
                    {
                        return CY_U3P_ERROR_CMD_NOT_SUPPORTED;
                    }

                    /* Keep the SD clock disabled while making frequency changes. */
                    SIB->sdmmc[portId].cs       |= (CY_U3P_SIB_SDMMC_CLK_DIS);
                    SIB->sdmmc[portId].mode_cfg  = (SIB->sdmmc[portId].mode_cfg & ~CY_U3P_SIB_SIGNALING_MASK) |
                        (0x04 << CY_U3P_SIB_SIGNALING_POS);
                    SIB->sdmmc[portId].cs &= (~CY_U3P_SIB_SDMMC_CLK_DIS);

                    CyU3PCardMgrSetClockFreq (portId, CY_U3P_CLOCK_DIVIDER_104M);
                    glCardCtxt[portId].uhsIOpMode = CY_U3P_SIB_UHS_I_SDR50;
                    freqSet = CyTrue;
                }
            }

            if ((glCardCtxt[portId].uhsIOpMode == CY_U3P_SIB_UHS_I_SDR50) ||
                    (glCardCtxt[portId].uhsIOpMode == CY_U3P_SIB_UHS_I_DDR50))
            {
                status = CyU3PCardMgrSDTuning (portId);
                if (status != CY_U3P_SUCCESS)
                {
                    return status;
                }
            }
        }
        
        if (!freqSet)
        {
            if ((glSibIntfParams[portId].maxFreq >= CY_U3P_SIB_FREQ_52MHZ) &&
                    ((glSibBuffer[13] & 0x02) > 0))
            {
                /* High-speed mode is supported. Switch to it. */
                status = CyU3PDmaChannelSetupRecvBuffer (CyU3PSibGetDmaChannelHandle, &dmaBuffer);
                if (status != CY_U3P_SUCCESS)
                {
                    return status;
                }

                CyU3PCardMgrSendCmd (portId, CY_U3P_SD_MMC_CMD6_SWITCH, CY_U3P_SD_MMC_R1_RESP_BITS,
                        CY_U3P_SD_SW_HIGHSP_PARAM, CY_U3P_SIB_RDDCARD);

                status = CyU3PCardMgrWaitForCmdResp (portId);
                if (status != CY_U3P_SUCCESS)
                {
                    return status;
                }

                /* Wait for the block received interrupt */
                status = CyU3PCardMgrWaitForBlkXfer (portId, CY_U3P_SIB_BLOCKS_RECEIVED);
                if (status != CY_U3P_SUCCESS)
                {
                    return status;
                }

                /* Check whether the function was switched. */
                if ((glSibBuffer[16] & 0x0F) != 0x01)
                {
                    return CY_U3P_ERROR_CMD_NOT_SUPPORTED;
                }

                /* High-speed operating frequency. */
                CyU3PCardMgrSetClockFreq (portId, CY_U3P_CLOCK_DIVIDER_52M);
            }
        }
    }

    return status;
}

/* Initializes the SD Card. */
CyU3PReturnStatus_t
CyU3PCardMgrSdInit (
        uint8_t portId)
{
    CyU3PReturnStatus_t status;
    CyBool_t gpioVal = CyFalse;
    uint8_t  gpioId = 0;

    status = CyU3PCardMgrSelOpConditions (portId, CY_U3P_SIB_DEV_SD);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Loop until a non-zero RCA has been published. */
    while (glCardCtxt[portId].cardRCA == 0)
    {
        /* Send CMD3 to fetch the card's RCA */
        CyU3PCardMgrSendCmd (portId, CY_U3P_SD_CMD3_SEND_RELATIVE_ADDR, CY_U3P_SD_R6_RESP_BITS, 0, 0);
        status = CyU3PCardMgrWaitForCmdResp (portId);
        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }

        if (((CY_U3P_SIB_SDMMC_RESP_IDX(portId) & CY_U3P_SIB_CMD_MASK) == CY_U3P_SD_CMD3_SEND_RELATIVE_ADDR) &&
                (((CY_U3P_SIB_SDMMC_RESP_REG0(portId) >> 9) & 0x0F) == 0x02))
        {
            glCardCtxt[portId].cardRCA = (SIB->sdmmc[portId].resp_reg0 >> 16);
        }
        else
        {
            return CY_U3P_ERROR_CARD_WRONG_RESPONSE;
        }
    }

    /* Read and parse the contents of the CSD register. */
    status = CyU3PCardMgrReadAndParseCSD (portId, CY_U3P_SIB_DEV_SD);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    if (glSibIntfParams[portId].writeProtEnable == CyTrue)
    {
        if (portId == 0)
        {
            gpioId = CY_U3P_SIB0_WRPROT_GPIO;
        }
        else
        {
            gpioId = CY_U3P_SIB1_WRPROT_GPIO;
        }

        status = CyU3PGpioGetValue (gpioId, &gpioVal);
        if (status == CY_U3P_SUCCESS)
        {
            if (gpioVal != 0)
            {
                glCardCtxt[portId].writeable = 0;
            }
        }
    }

    /* Normal operating frequency for SD card. */
    if (glSibIntfParams[portId].maxFreq >= CY_U3P_SIB_FREQ_26MHZ)
        CyU3PCardMgrSetClockFreq (portId, CY_U3P_CLOCK_DIVIDER_26M);
    else
        CyU3PCardMgrSetClockFreq (portId, CY_U3P_CLOCK_DIVIDER_20M);

    /* Select the card with the SELECT/DESELECT_CARD(CMD7) command. */
    status = CyU3PCardMgrSelectCard (portId, 1);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Check if the Card Locked Bit is set */
    if ((CY_U3P_SIB_SDMMC_RESP_REG0(portId) >> 25) & 0x01)
    {
        glCardCtxt[portId].locked = 1;
    }
    else
    {
        status = CyU3PCardMgrCompleteSDInit (portId);
    }

    return status;
}

/* Change the bus width to bus_width bits */
CyU3PReturnStatus_t
CyU3PCardMgrMMCBusTest (
        uint8_t portId,
        uint8_t bus_width)
{
    CyU3PDmaBuffer_t dmaBuffer;
    CyU3PReturnStatus_t status;

    dmaBuffer.buffer = glSibBuffer;
    dmaBuffer.size   = CY_U3P_SIB_BLOCK_SIZE;
    dmaBuffer.count  = bus_width;
    dmaBuffer.status = 0;

    CyU3PDmaChannelReset (CyU3PSibGetDmaChannelHandle);
    status = CyU3PDmaChannelSetupSendBuffer (CyU3PSibGetDmaChannelHandle, &dmaBuffer);
    if (status != CY_U3P_SUCCESS)
        return status;

    /* Set the block length to bus_width bytes. Write the data pattern to be sent to MMC card. */
    CyU3PSibSetBlockLen (portId, bus_width);
    CyU3PSibSetNumBlocks (portId, 0x01);

    /* Select BUSTEST_W command (CMD19, Resp R1) */
    CyU3PSibSetActiveSocket (portId, CYU3P_SIB_INT_WRITE_SOCKET);
    CyU3PCardMgrSendCmd (portId, CY_U3P_MMC_CMD19_BUSTEST_W, CY_U3P_SD_MMC_R1_RESP_BITS, 0, 0);
    status = CyU3PCardMgrWaitForCmdResp (portId);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Write the block to the card */
    SIB->sdmmc[portId].cs |= CY_U3P_SIB_WRDCARD;
    status = CyU3PCardMgrWaitForBlkXfer (portId, CY_U3P_SIB_BLOCK_COMP);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Update the active socket id */
    CyU3PSibSetActiveSocket (portId, CYU3P_SIB_INT_READ_SOCKET);

    /* Ensure that the first bytes are cleared */
    glSibBuffer[0] = 0x00;
    glSibBuffer[1] = 0x00;

    dmaBuffer.count  = 0;
    dmaBuffer.status = 0;

    CyU3PDmaChannelReset (CyU3PSibGetDmaChannelHandle);
    status = CyU3PDmaChannelSetupRecvBuffer (CyU3PSibGetDmaChannelHandle, &dmaBuffer);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Select BUSTEST_R command (CMD14, Resp R1) */
    CyU3PCardMgrSendCmd (portId, CY_U3P_MMC_CMD14_BUSTEST_R, CY_U3P_SD_MMC_R1_RESP_BITS, 0,
            CY_U3P_SIB_RDDCARD);

    status = CyU3PCardMgrWaitForCmdResp (portId);
    if (status == CY_U3P_SUCCESS)
    {
        status = CyU3PCardMgrWaitForBlkXfer (portId, CY_U3P_SIB_BLOCKS_RECEIVED);
    }

    return status;
}

/* Do a bus test to select the best possible bus width. */
CyU3PReturnStatus_t
CyU3PCardMgrSelMmcBusWidth (
        uint8_t portId)
{
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    uint8_t busWidth     = CY_U3P_CARD_BUS_WIDTH_1_BIT;
    CyBool_t mmc8BitMode = CyFalse;

    mmc8BitMode = CyFx3DevIOIsSib8BitWide (portId);
    if (mmc8BitMode)
    {
        /* Set up a single block transfer of 8 bytes. */
        CyU3PSibSetBlockLen (portId, 0x08);

        /* Configure the SIB to assume 8 bit MMC. */
        SIB->sdmmc[portId].mode_cfg = (SIB->sdmmc[portId].mode_cfg & ~CY_U3P_SIB_DATABUSWIDTH_MASK) |
            (CY_U3P_CARD_BUS_WIDTH_8_BIT << CY_U3P_SIB_DATABUSWIDTH_POS);

        /* Refer the MMC Spec section 7.4.4 for the bus testing procedure. The Bus Test Pattern. */
        glSibBuffer[0] = 0x55;
        glSibBuffer[1] = 0xAA;

        /* Do the 8 bits bus test */
        status = CyU3PCardMgrMMCBusTest (portId, 0x08);
        if ((status == CY_U3P_SUCCESS) && (glSibBuffer[0] == 0xAA) && (glSibBuffer[1] == 0x55))
        {
            busWidth = CY_U3P_CARD_BUS_WIDTH_8_BIT;
        }
        else
        {
            /* Reset the controller and run bus test in 4 bit mode. */
            CyU3PCardMgrClearCmdBits (portId);
            CyU3PSibResetSibCtrlr (portId);
            CyU3PCardMgrCheckStatus (portId);
        }
    }

    if (busWidth == CY_U3P_CARD_BUS_WIDTH_1_BIT)
    {
        /* No 8 bit support. Try to set the bus width to 4 bits. Configure the SIB to assume 4 bit MMC. */
        SIB->sdmmc[portId].mode_cfg = (SIB->sdmmc[portId].mode_cfg & ~CY_U3P_SIB_DATABUSWIDTH_MASK) |
            (CY_U3P_CARD_BUS_WIDTH_4_BIT << CY_U3P_SIB_DATABUSWIDTH_POS);

        glSibBuffer[0] = 0x5A;

        /* Do the 4 bits bus test */
        status = CyU3PCardMgrMMCBusTest (portId, 0x04);
        if ((status == CY_U3P_SUCCESS) && (glSibBuffer[0] == 0xA5))
        {
            busWidth = CY_U3P_CARD_BUS_WIDTH_4_BIT;
        }
        else
        {
            CyU3PCardMgrClearCmdBits (portId);
            CyU3PSibResetSibCtrlr (portId);
            CyU3PCardMgrCheckStatus (portId);
        }
    }

    if (busWidth != CY_U3P_CARD_BUS_WIDTH_1_BIT)
    {
        /* Set the card's bus width */
        uint32_t tmp = 0x03B7 << 16;

        tmp |= (busWidth << 8);
        CyU3PCardMgrSendCmd (portId, CY_U3P_SD_MMC_CMD6_SWITCH, CY_U3P_SD_MMC_R1B_RESP_BITS, tmp, 0);
        status = CyU3PCardMgrWaitForCmdResp (portId);
    }
    else
    {
        CyU3PCardMgrClearCmdBits (portId);
        CyU3PSibResetSibCtrlr (portId);

        /* 8/4 bits not supported either. Change the bus width back to 1 bit. */
        SIB->sdmmc[portId].mode_cfg = (SIB->sdmmc[portId].mode_cfg & ~CY_U3P_SIB_DATABUSWIDTH_MASK);

        CyU3PDmaChannelReset (CyU3PSibGetDmaChannelHandle);
    }

    /* Update the context structure with the bus width being used. */
    glCardCtxt[portId].busWidth = busWidth;
    return status;
}

CyU3PReturnStatus_t
CyU3PCardMgrReadExtCsd (
        uint8_t  portId,
        uint8_t *buffer_p)
{
    CyU3PReturnStatus_t status;
    CyU3PDmaBuffer_t dmaBuffer;

    status = CyU3PCardMgrCheckStatus (portId);
    if (status != CY_U3P_SUCCESS)
        return status;

    CyU3PDmaChannelReset (CyU3PSibGetDmaChannelHandle);

    dmaBuffer.size   = 512;
    dmaBuffer.buffer = buffer_p;
    dmaBuffer.count  = 0;
    dmaBuffer.status = 0;
    status = CyU3PDmaChannelSetupRecvBuffer (CyU3PSibGetDmaChannelHandle, &dmaBuffer);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDmaChannelReset (CyU3PSibGetDmaChannelHandle);
        return status;
    }

    CyU3PSibSetNumBlocks (portId, 1);
    CyU3PSibSetBlockLen (portId, CY_U3P_SIB_BLOCK_SIZE);
    CyU3PSibSetActiveSocket(portId, CYU3P_SIB_INT_READ_SOCKET);

    /* Read the Ext-CSD register using CMD8. */
    CyU3PCardMgrSendCmd (portId, CY_U3P_MMC_CMD8_SEND_EXT_CSD, CY_U3P_SD_MMC_R1_RESP_BITS, 0,
            CY_U3P_SIB_RDDCARD);
    status = CyU3PCardMgrWaitForCmdResp (portId);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDmaChannelReset (CyU3PSibGetDmaChannelHandle);
        return status;
    }

    status = CyU3PCardMgrWaitForBlkXfer (portId, CY_U3P_SIB_BLOCKS_RECEIVED);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDmaChannelReset (CyU3PSibGetDmaChannelHandle);
    }

    return status;
}

static uint32_t
CyU3PCardMgrGetMMCEraseGrpSize (
        uint8_t portId)
{
    uint32_t eg_size = 0;
    uint32_t eg_mult = 0;

    /* Compute erase unit size only if the Erase commands are supported. */
    if ((glCardCtxt[portId].csdRegData[4] & 0x02) != 0)
    {
        eg_size = ((glCardCtxt[portId].csdRegData[10] >> 2) & 0x1F);
        eg_mult = ((glCardCtxt[portId].csdRegData[10] & 0x03) << 3) |
            ((glCardCtxt[portId].csdRegData[11] >> 5) & 0x07);

        /* Calculate the erase group size per the definition in the MMC CSD register. */
        eg_size = ((eg_size + 1) * (eg_mult + 1)) << 9;
    }

    return (eg_size);
}

/* This function handles the MMC card initialization */
CyU3PReturnStatus_t
CyU3PCardMgrMmcInit (
        uint8_t portId)
{
    CyU3PDmaBuffer_t dmaBuffer;
    CyU3PReturnStatus_t status;

    dmaBuffer.buffer = glSibBuffer;
    dmaBuffer.count  = 0;
    dmaBuffer.size   = CY_U3P_SIB_BLOCK_SIZE;
    dmaBuffer.status = 0;

    /* Configure for 1 bit MMC mode. */
    SIB->sdmmc[portId].mode_cfg = CY_U3P_SIB_SDMMC_MODE_CFG_DEFAULT;

    /* Some MMC cards need another reset here. */
    CyU3PCardMgrSendCmd (portId, CY_U3P_SD_MMC_CMD0_GO_IDLE_STATE, 0, 0, 0);
    CyU3PThreadSleep (CY_U3P_SIB_DEVICE_POSTRESET_DELAY);

    status = CyU3PCardMgrSelOpConditions (portId, CY_U3P_SIB_DEV_MMC);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Assigning 0x03 as RCA to the card. Send SET_RELATIVE_ADDR (CMD3). */
    SIB->sdmmc[portId].ncr     = 0x3F7F;
    glCardCtxt[portId].cardRCA = 0x03;
    CyU3PCardMgrSendCmd (portId, CY_U3P_MMC_CMD3_SET_RELATIVE_ADDR, CY_U3P_SD_MMC_R1_RESP_BITS,
            (glCardCtxt[portId].cardRCA << 16), 0);
    status = CyU3PCardMgrWaitForCmdResp (portId);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Get card parameters by reading the CSD register. */
    status = CyU3PCardMgrReadAndParseCSD (portId, CY_U3P_SIB_DEV_MMC);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Calculate the erase group size. Since we never set the ERASE_DEF bit, we can always get this from the CSD. */
    glCardCtxt[portId].eraseSize = CyU3PCardMgrGetMMCEraseGrpSize (portId);

    /* Default MMC operating frequency. */
    CyU3PCardMgrSetClockFreq (portId, CY_U3P_CLOCK_DIVIDER_20M);

    /* Select the card and put it in tranfer state (CMD7, Resp R1b) */
    status = CyU3PCardMgrSelectCard (portId, 1);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Check if the Card Locked Bit is set */
    if ((CY_U3P_SIB_SDMMC_RESP_REG0(portId) >> 25) & 0x01)
    {
        glCardCtxt[portId].locked = 1;
    }

    /* MMC Specification version */
    if (glCardCtxt[portId].cardVer > 3)
    {
        status = CyU3PCardMgrSelMmcBusWidth (portId);
        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }

        CyU3PCardMgrCheckStatus (portId);

        dmaBuffer.count = 0;
        status = CyU3PDmaChannelSetupRecvBuffer (CyU3PSibGetDmaChannelHandle, &dmaBuffer);
        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }

        /* Read the Ext-CSD register using CMD8. */
        CyU3PSibSetBlockLen (portId, CY_U3P_SIB_BLOCK_SIZE);
        CyU3PSibSetActiveSocket (portId, CYU3P_SIB_INT_READ_SOCKET);
        CyU3PCardMgrSendCmd (portId, CY_U3P_MMC_CMD8_SEND_EXT_CSD, CY_U3P_SD_MMC_R1_RESP_BITS, 0, CY_U3P_SIB_RDDCARD);

        status = CyU3PCardMgrWaitForCmdResp (portId);
        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }

        status = CyU3PCardMgrWaitForBlkXfer (portId, CY_U3P_SIB_BLOCKS_RECEIVED);
        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }

        /* Check the CardType - Refer to the MMC Spec. EXT_CSD register 196th byte field for details. */
        if ((glSibIntfParams[portId].maxFreq >= CY_U3P_SIB_FREQ_52MHZ) && (glSibBuffer[196] > 1))
        {
            /* Set high-speed mode. See the MMC Spec EXT_CSD register and the switch cmd 6 for details. */
            CyU3PCardMgrSendCmd (portId, CY_U3P_SD_MMC_CMD6_SWITCH, CY_U3P_SD_MMC_R1B_RESP_BITS, (0x03B90100), 0);
            status = CyU3PCardMgrWaitForCmdResp (portId);
            if (status != CY_U3P_SUCCESS)
            {
                return status;
            }

            while ((SIB->sdmmc[portId].status & CY_U3P_SIB_DAT0_STAT) == 0);
            CyU3PCardMgrSetClockFreq (portId, CY_U3P_CLOCK_DIVIDER_52M);
        }
        else
        {
            if (glSibIntfParams[portId].maxFreq >= CY_U3P_SIB_FREQ_26MHZ)
                CyU3PCardMgrSetClockFreq (portId, CY_U3P_CLOCK_DIVIDER_26M);
        }

        if (glSibIntfParams[portId].useDdr == CyTrue)
        {
            /* Check if the card supports the DDR Mode. */
            if ((glSibIntfParams[portId].maxFreq == CY_U3P_SIB_FREQ_104MHZ) && (glSibBuffer[196] & 0x04))
            {
                /* By default 8 bit bus width. */
                uint32_t val = 0x03B70600;

                if (glCardCtxt[portId].busWidth == CY_U3P_CARD_BUS_WIDTH_4_BIT)
                {
                    val = 0x03B70500;
                }

                /* Switch to the DDR mode. */
                CyU3PCardMgrSendCmd (portId, CY_U3P_SD_MMC_CMD6_SWITCH, CY_U3P_SD_MMC_R1B_RESP_BITS, val, 0);
                status = CyU3PCardMgrWaitForCmdResp (portId);

                if (status != CY_U3P_SUCCESS)
                    return status;

                /* Disable the SD Clk */
                while ((SIB->sdmmc[portId].status & CY_U3P_SIB_DAT0_STAT) == 0);
                SIB->sdmmc[portId].cs |= (CY_U3P_SIB_SDMMC_CLK_DIS);

                SIB->sdmmc[portId].mode_cfg = (SIB->sdmmc[portId].mode_cfg & ~CY_U3P_SIB_SIGNALING_MASK) |
                    (0x08 << CY_U3P_SIB_SIGNALING_POS);

                /* Enable the SD Clk */
                CyU3PCardMgrSetClockFreq (portId, CY_U3P_CLOCK_DIVIDER_104M);
                SIB->sdmmc[portId].cs &= (~CY_U3P_SIB_SDMMC_CLK_DIS);
                glCardCtxt[portId].ddrMode = CyTrue;
            }
        }

        if (glCardCtxt[portId].highCapacity > 0)
        {
            /* Update the capacity based on the SEC_COUNT field of the EXT_CSD register. */
            glCardCtxt[portId].numBlks = ((glSibBuffer[215] << 24) | (glSibBuffer[214] << 16)
                    | (glSibBuffer[213] << 8) | (glSibBuffer[212]));
        }
    }

    return status;
}

/* This function initializes the card by sending in a series of commands to the card.
* This function invokes the sd, mmc card specific init functions to initialize the card. */
CyU3PReturnStatus_t
CyU3PCardMgrInit (
        uint8_t portId)
{
    CyU3PReturnStatus_t status;

    /* Mark this S port in init mode. */
    glSibCardInitMode[portId] = CyTrue;

    /* Intialize the modules global variables */
    CyU3PCardMgrInitCtxt (portId);
    CyU3PCardMgrSetClockFreq (portId, CY_U3P_CLOCK_DIVIDER_400);

    CyU3PSibSetActiveSocket(portId, CYU3P_SIB_INT_READ_SOCKET);

    /* Sleep for specified time if initialization delay has been provided */
    if(glSibIntfParams[portId].cardInitDelay > 0)
        CyU3PThreadSleep(glSibIntfParams[portId].cardInitDelay);

    /* Reset the card by sending the GO_IDLE_STATE command. */
    SIB->sdmmc[portId].data_cfg = 0;
    CyU3PCardMgrSendCmd (portId, CY_U3P_SD_MMC_CMD0_GO_IDLE_STATE, 0, 0, 0);
    CyU3PThreadSleep (CY_U3P_SIB_DEVICE_POSTRESET_DELAY);

    /*
     * If the card responds to CMD8, then it is SD 2.0 compatiblie.
     * Argument 0x01 checks whether the card can operate in 2.7-3.6V.
     * 0xAA is the check pattern. Expect R7 Response.
     */
    CyU3PCardMgrSendCmd (portId, CY_U3P_SD_CMD8_SEND_IF_COND, CY_U3P_SD_R7_RESP_BITS, 0x01AA, 0);
    status = CyU3PCardMgrWaitForCmdResp (portId);
    if (status == CY_U3P_SUCCESS)
    {
        /* The card has responded to CMD8. Verify the check pattern and the voltage range
         * sent by the card.
         */
        if (((SIB->sdmmc[portId].resp_reg0 & 0xFF) == 0xAA) &&
                ((SIB->sdmmc[portId].resp_reg0 >> 8) & 0xF) == 0x01)
        {
            /* Card is a 2.0 or greater version card since it has responded to CMD8 */
            glCardCtxt[portId].cardVer = CY_U3P_SD_CARD_VER_2_0;
        }
        else
        {
            /* Non-compatible voltage range or check pattern is not correct */
            status = CY_U3P_ERROR_INVALID_VOLTAGE_RANGE;
            goto CardMgrInitDone;
        }
    }

#ifdef CYU3P_STORAGE_SDIO_SUPPORT
    /* Try to initialize the card as an SDIO-card by sending a CMD5. */
    status = CyU3PCardMgrSdioInit (portId, 0);
    if (status != CY_U3P_SUCCESS)
    {
        glCardCtxt[portId].cardType = CY_U3P_SIB_DEV_NONE;
    }

    if ((glCardCtxt[portId].cardType == CY_U3P_SIB_DEV_NONE) ||
            (glCardCtxt[portId].cardType == CY_U3P_SIB_DEV_SDIO_COMBO))
    {
#endif
        /* Try to initialize the card as an SD-card by sending an APP_CMD. Expect R1 response.*/
        CyU3PCardMgrSendCmd (portId, CY_U3P_SD_MMC_CMD55_APP_CMD, CY_U3P_SD_MMC_R1_RESP_BITS, 0, 0);
        status = CyU3PCardMgrWaitForCmdResp (portId);

        if (status == CY_U3P_SUCCESS)
        {
            status = CyU3PCardMgrSdInit (portId);
            if (status == CY_U3P_SUCCESS)
            {
                glCardCtxt[portId].cardType = CY_U3P_SIB_DEV_SD;
            }
            else
            {
                glCardCtxt[portId].cardType = CY_U3P_SIB_DEV_NONE;
            }
        }
        else if (status == CY_U3P_ERROR_TIMEOUT)
        {
            status = CyU3PCardMgrMmcInit (portId);
            if (status == CY_U3P_SUCCESS)
            {
                glCardCtxt[portId].cardType = CY_U3P_SIB_DEV_MMC;
            }
            else
            {
                glCardCtxt[portId].cardType = CY_U3P_SIB_DEV_NONE;
            }
        }
#ifdef CYU3P_STORAGE_SDIO_SUPPORT
    }
#endif

CardMgrInitDone:
    if (glCardCtxt[portId].cardType != CY_U3P_SIB_DEV_NONE)
    {
        SIB->sdmmc[portId].data_cfg    = CY_U3P_SIB_SDMMC_DATA_CFG_DEFAULT ;
        glCardCtxt[portId].dataTimeOut = 0x3FFFFF;
        SIB->sdmmc[portId].nac         = 0x00FFFFFF;

        if (glCardCtxt[portId].cardType != CY_U3P_SIB_DEV_SDIO)
        {
            SIB->sdmmc[portId].data_cfg |= CY_U3P_SIB_CARD_BUSY_DET;
            SIB->sdmmc[portId].mode_cfg |= (CY_U3P_SIB_RD_STOP_CLK_EN | CY_U3P_SIB_BLK_END_STOP_CLK);
            CyU3PCardMgrSetBlockSize (portId, CY_U3P_SIB_BLOCK_SIZE);
            CyU3PSibSetBlockLen (portId, CY_U3P_SIB_BLOCK_SIZE);
        }
        else
        {
            SIB->sdmmc[portId].ncr |= 0x7F;
        }
    }

    glSibCardInitMode[portId] = CyFalse;
    return status;
}

/* Deinitialize the card manager context */
void
CyU3PCardMgrDeInit (
        uint8_t portId)
{
    /* De-select the card. */
    CyU3PCardMgrSelectCard (portId, 0);

    /* Ensure that all the interrupt bits are cleared */
    CyU3PSibClearIntr (portId);

    /* Mask all interrupts except the card detect interrupt. */
    if (glSibIntfParams[portId].cardDetType == CY_U3P_SIB_DETECT_DAT_3)
    {
        SIB->sdmmc[portId].intr_mask = CY_U3P_SIB_DAT3_CHANGE;
    }
    else if (glSibIntfParams[portId].cardDetType == CY_U3P_SIB_DETECT_GPIO)
    {
        SIB->sdmmc[portId].intr_mask = CY_U3P_SIB_CARD_DETECT;
    }

    /* Reset the card context structure */
    CyU3PMemSet ((uint8_t *)&glCardCtxt[portId], 0, sizeof(CyU3PCardCtxt_t));

    /* Reset the port related information of the Sib `context structure */
    glSibCtxt[portId].isRead       = 0;
    glSibCtxt[portId].inUse        = 0;
    glSibCtxt[portId].partition    = 0;
    glSibCtxt[portId].numBootLuns  = 0;
    glSibCtxt[portId].numUserLuns  = 0;
}

/*[]*/

