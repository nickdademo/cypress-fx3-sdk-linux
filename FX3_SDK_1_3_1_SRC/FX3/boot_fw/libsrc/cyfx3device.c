/*
 ## Cypress USB 3.0 Platform source file (cyfx3device.c)
 ## ===========================
 ##
 ##  Copyright Cypress Semiconductor Corporation, 2011-2012,
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

#include "lpp_regs.h"
#include "i2c_regs.h"
#include "spi_regs.h"
#include "uart_regs.h"
#include "gpio_regs.h"
#include "gctl_regs.h"
#include "vic_regs.h"
#include "cyfx3bootloader.h"
#include "cyfx3device.h"
#include "gctlaon_regs.h"
#include "cyfx3usb.h"
#include "cyfx3iocfg.h"

#define CY_U3P_SYS_BACKUP_CLK_FREQ      (32000) /* Backup clock frequency.                  */

uint32_t glSysClkFreq = 0;
uint32_t glUsbMpllDefault = 0x910410;
uint32_t gpioList[2];

/* Bitmap used to keep track of the Serial IO block's states. */
uint8_t  glLppBlockOn = 0;
CyFx3BootIoMatrixConfig_t glBootIoCfg = 
{
    CyFalse,
    CyFalse,
    CyFalse,
    CyFalse,
    CyFalse,
    {0, 0}
};
 
extern CyBool_t glNoReenum;
uint32_t glWatchDogPeriod = 0;
#define CYU3P_WATCHDOG_MULTIPLIER       (33)
#define CYU3P_WATCHDOG_MODE             (2)

CyBool_t glBootUsbIsOn  = CyFalse;      /* Whether USB block has been left on by SDK firmware. */
CyBool_t glBootGpioIsOn = CyFalse;      /* Whether GPIO block has been left on by SDK firmware. */

/* Summary
 * This function is used to compute the checksum.
 */
void
CyFx3BootComputeChecksum (
        uint32_t *buffer,
        uint32_t  length,
        uint32_t *chkSum)
{
    uint32_t count;

    *chkSum = 0;
    for (count = 0; count < (length / 4); count++)
    {
        *chkSum += buffer[count];
    }
}

/* This function is only for the use of FX3 device */
CyFx3BootErrorCode_t
CyFx3BootDeviceConfigureIOMatrix (
        CyFx3BootIoMatrixConfig_t *cfg_p)
{
    CyU3PIoMatrixConfig_t tmp;

    if (cfg_p == 0)
        return CY_FX3_BOOT_ERROR_BAD_ARGUMENT;

    tmp.isDQ32Bit        = cfg_p->isDQ32Bit;
    tmp.useUart          = cfg_p->useUart;
    tmp.useI2C           = cfg_p->useI2C;
    tmp.useI2S           = cfg_p->useI2S;
    tmp.useSpi           = cfg_p->useSpi;
    tmp.s0Mode           = CY_U3P_SPORT_INACTIVE;
    tmp.s1Mode           = CY_U3P_SPORT_INACTIVE;
    tmp.lppMode          = CY_U3P_IO_MATRIX_LPP_DEFAULT;
    tmp.gpioSimpleEn[0]  = cfg_p->gpioSimpleEn[0];
    tmp.gpioSimpleEn[1]  = cfg_p->gpioSimpleEn[1];
    tmp.gpioComplexEn[0] = 0;
    tmp.gpioComplexEn[1] = 0;

    if (CyFx3DevIOConfigure (&tmp) == CyFalse)
        return CY_FX3_BOOT_ERROR_BAD_ARGUMENT;

    glBootIoCfg = *cfg_p;
    return CY_FX3_BOOT_SUCCESS;
}

CyFx3PartNumber_t
CyFx3BootGetPartNumber (
        void)
{
    return ((CyFx3PartNumber_t)CyFx3DevIdentifyPart ());
}

/* Function to check integrity of firmware state data structure, and update variables. */
static void
CyFx3CheckNoRenumStructure (
        void)
{
    CyFx3BootUsbDescTable_t *state_p = (CyFx3BootUsbDescTable_t *)glUsbDescPtrs;
    uint32_t length, checksum = 0;

    /* Check for structure signature. */
    if (state_p->signature != CY_WB_SIGNATURE)
    {
        state_p->leaveGpioOn = 0;
        return;
    }

    /* Check for booter revision and switchToBooter feature support. */
    if ((state_p->bootSignature != CY_FX3_BOOTER_SIGNATURE) ||
            ((state_p->revision & CY_FX3_BOOTER_REV_1_2_0) != CY_FX3_BOOTER_REV_1_2_0) ||
            (state_p->switchToBooter != 1))
    {
        state_p->revision    = CY_FX3_BOOTER_REV_1_1_1;
        state_p->leaveGpioOn = 0;
        state_p->signature   = 0xFFFFFFFF;
        return;
    }

    /* Structure length verification. */
    if ((state_p->length == 0) || (state_p->length > (0x1000 - 8)))
    {
        state_p->leaveGpioOn = 0;
        state_p->signature   = 0xFFFFFFFF;
        return;         /* Bad length. */
    }

    /* Work-around for checksum calculation bug in FX3 SDK 1.2.x and older. */
    if (state_p->revision >= CY_FX3_BOOTER_REV_1_3_0)
        length = state_p->length;
    else
        length = state_p->length & 0xFF;

    /* Structure checksum verification. */
    CyFx3BootComputeChecksum ((uint32_t *)glUsbDescPtrs, length, &checksum);
    if (checksum != *((uint32_t *)(glUsbDescPtrs + state_p->length)))
    {
        state_p->leaveGpioOn = 0;
        state_p->signature = 0xFFFFFFFF;
        return;
    }

    glBootUsbIsOn = CyTrue;
    if ((state_p->revision >= CY_FX3_BOOTER_REV_1_2_1) && (state_p->leaveGpioOn != 0))
        glBootGpioIsOn = CyTrue;
}

/* Summary:
   This function initializes the boot device.
 */
void
CyFx3BootDeviceInit (
        CyBool_t setFastSysClk)
{
    uint32_t t1, t2;
    uint32_t backupDiv = 0;
    CyBool_t watchdogEn = CyFalse;

    /* Update the CPU clock register. */
    GCTL->cpu_clk_cfg = (3 << CY_U3P_GCTL_CPUCLK_SRC_POS) | 1 | (1 << CY_U3P_GCTL_CPUCLK_DMA_DIV_POS) |
        (1 << CY_U3P_GCTL_CPUCLK_MMIO_DIV_POS);

    /* Read back to flush. */
    t1 = GCTL->cpu_clk_cfg;

    /* Wait for sometime for the clock to stabilize. */
    CyFx3BootBusyWait (10);

    /* Identify the SYS_CLK. FSLC[1:0] gives the frequency
     * and FSLC[2] gives the source. */
    t1 = ((GCTL->pll_cfg & CY_U3P_FSLC_MASK) >> CY_U3P_FSLC_POS);
    switch (t1 & 0x03)
    {
    /* FSLC * FBDIV / REFDIV / OUTDIV */
    case 0:
        backupDiv    = 19200000;
        if (setFastSysClk)
            glSysClkFreq = 403200000;   /* 19.2 MHz * 21 / 1 / 1 */
        else
            glSysClkFreq = 384000000;   /* 19.2 MHz * 20 / 1 / 1 */
        break;
    case 1:
        backupDiv        = 26000000;
        glSysClkFreq     = 416000000;   /* 26.0 MHz * 16 / 1 / 1 */
        glUsbMpllDefault = 0x910700;
        break;
    case 2:
        backupDiv        = 38400000;
        glUsbMpllDefault = 0x910608;
        if (setFastSysClk)
            glSysClkFreq = 403200000;   /* 38.4 MHz * 21 / 2 / 1 */
        else
            glSysClkFreq = 384000000;   /* 38.4 MHz * 20 / 2 / 1 */
        break;
    default:
        backupDiv        = 52000000;
        glSysClkFreq     = 416000000;   /* 52.0 MHz * 16 / 2 / 1 */
        glUsbMpllDefault = 0x910580;
        break;
    }

    /* Update the SYSCLK setting to 403.2 MHz if the reference clock source is a 19.2 MHz crystal. */
    if ((glSysClkFreq == 403200000) && ((GCTL->pll_cfg & CY_U3P_FBDIV_MASK) != 21))
    {
        GCTL->pll_cfg = GCTL->pll_cfg & ~CY_U3P_FBDIV_MASK | 21;
        CyFx3BootBusyWait (10);
        while ((GCTL->pll_cfg & CY_U3P_PLL_LOCK) == 0);
        CyFx3BootBusyWait (10);
    }

    /* If the backup clock is already enabled, no need to do anything. */
    t1 = GCTLAON->watchdog_cs;
    if (t1 & CY_U3P_GCTL_BACKUP_CLK)
        goto SkipTimerConfig;

    /* If the watchdog 0 is configured for reset, temporarily disable it. */
    if ((t1 & CY_U3P_GCTL_MODE0_MASK) == CYU3P_WATCHDOG_MODE)
    {
        watchdogEn = CyTrue;
        GCTLAON->watchdog_cs = (t1 | CY_U3P_GCTL_MODE0_MASK);
        CyFx3BootBusyWait (10);
    }

    /* Check if the 32 KHz clock input is supplied. If not, enable the backup clock. */

    /* Enable WDT0 free running mode. */
    t1 = GCTLAON->watchdog_cs;
    CyFx3BootBusyWait (10);

    GCTLAON->watchdog_cs = ((t1 & ~CY_U3P_GCTL_MODE0_MASK) | (1 << CY_U3P_GCTL_MODE0_POS));
    CyFx3BootBusyWait (10);

    t2 = GCTLAON->watchdog_timer0;

    /* Wait for about 200 us. Since the timer is supposed to be running at 32 KHz, it should have moved
       by 5 or 6 units if there is an available clock. */
    CyFx3BootBusyWait (200);
    if (t2 != GCTLAON->watchdog_timer0)
    {
        goto SkipTimerConfig;
    }

    /* The 32 KHz clock input is not available. Turn on the backup clock input. */
    GCTLAON->watchdog_cs = t1;

    /* Apply 0.5 approximation. If the actual divider is x and 
     * the integral divider is n, then
     * (x - floor(x)) < 0.5 ==> n = floor(x);
     * (x - floor(x)) >= 0.5 ==> n = floor(x) + 1;
     *
     * For this source frequency is multiplied by 4,
     * and the last two bits are used for deciding this.
     * if last 2 bits evaluate to 00 or 01 then it is floor
     * and if the last two bits evaluate to 10 or 11 then
     * it is floor + 1. Here the backupDiv represents divider - 1.
     */
    backupDiv <<= 2;
    backupDiv /= CY_U3P_SYS_BACKUP_CLK_FREQ;
    t1 = backupDiv & 0x03;
    backupDiv >>= 2;
    if (t1 < 0x10)
    {
        backupDiv--;
    }

    /* If the divider has gone beyond valid value, then set to the maximum valid divider. */
    backupDiv <<= CY_U3P_GCTL_BACKUP_DIVIDER_POS;
    if (backupDiv > CY_U3P_GCTL_BACKUP_DIVIDER_MASK)
    {
        backupDiv = CY_U3P_GCTL_BACKUP_DIVIDER_MASK;
    }

    t1 = GCTLAON->watchdog_cs;
    t1 = (t1 & ~CY_U3P_GCTL_BACKUP_DIVIDER_MASK) | backupDiv;
    GCTLAON->watchdog_cs = t1;
    CyFx3BootBusyWait (1);
    GCTLAON->watchdog_cs |= CY_U3P_GCTL_BACKUP_CLK;

    glLppBlockOn     = 0;
    glWatchDogPeriod = 0;

SkipTimerConfig:
    if (watchdogEn)
    {
        /* Assume the watchdog reset duration is 1 second. */
        CyFx3BootWatchdogConfigure (CyTrue, 1000);
    }

    CyFx3CheckNoRenumStructure ();
}

void
CyFx3BootWatchdogConfigure (
        CyBool_t enable,
        uint32_t period)
{
    uint32_t wdcs = GCTLAON->watchdog_cs;

    /* Watchdog 0 is used here, as there are no other expected uses for the watchdog timer in this library. */
    if (enable)
    {
        /* If watchdog0 is already enabled, disable and then re-enable with appropriate frequency. */
        if ((wdcs & CY_U3P_GCTL_MODE0_MASK) != CY_U3P_GCTL_MODE0_MASK)
            GCTLAON->watchdog_cs |= CY_U3P_GCTL_MODE0_MASK;

        glWatchDogPeriod = (period * CYU3P_WATCHDOG_MULTIPLIER);
        if (glWatchDogPeriod < period)
            glWatchDogPeriod = 0xFFFFFFFF;
        GCTLAON->watchdog_timer0 = glWatchDogPeriod;

        /* Configure watchdog0 as required. */
        wdcs  = (GCTLAON->watchdog_cs & ~(CY_U3P_GCTL_MODE0_MASK | CY_U3P_GCTL_BITS0_MASK));
        wdcs |= (CY_U3P_GCTL_BITS0_MASK | CYU3P_WATCHDOG_MODE);
        GCTLAON->watchdog_cs = wdcs;
    }
    else
    {
        /* Disable watchdog0 if currently enabled. */
        if ((wdcs & CY_U3P_GCTL_MODE0_MASK) != CY_U3P_GCTL_MODE0_MASK)
            GCTLAON->watchdog_cs |= CY_U3P_GCTL_MODE0_MASK;
        glWatchDogPeriod = 0;
    }
}

void
CyFx3BootWatchdogClear (
        void)
{
    /* Writing the same value to the register does not clear the timer. Continuously change the value being
       written. */
    if (glWatchDogPeriod)
    {
        if (glWatchDogPeriod & 0x01)
            glWatchDogPeriod--;
        else
            glWatchDogPeriod++;
        GCTLAON->watchdog_timer0 = glWatchDogPeriod;
    }
}

/*
  General reset for Hardware
  p = pointer to Module POWER register
*/
void reset(ulong *p)
{
    *p = 0;
    CyFx3BootBusyWait (10);
    *p |= CY_U3P_LPP_RESETN;           /* Need read modify write. */
    CyFx3BootBusyWait (100);
    while (!(*p & CY_U3P_LPP_ACTIVE));
}

#ifdef CY_USE_ARMCC
static void
CyFx3BootIcacheDisable (
                void)
{
    uint32_t val = 0;

    __asm
    {
        /* Flush the I-Cache. */
        MCR p15, 0, val, c7, c7, 0;

        /* Disable the I-Cache. */
        MRC p15, 0, val, c1, c0, 0;
        BIC val, val, #0x1000;
        MCR p15, 0, val, c1, c0, 0;
    }
}
#else
static void
CyFx3BootIcacheDisable (
        void)
{
    uint32_t val;

    __asm__
    (
        /* Flush the I-Cache. */
        "MOV %0, #0\n\t"
        "MCR p15, 0, %0, c7, c7, 0\n\t"

        /* Disable the I-Cache. */
        "MRC p15, 0, %0, c1, c0, 0\n\t"
        "BIC %0, %0, #0x1000\n\t"
        "MCR p15, 0, %0, c1, c0, 0\n\t"
        : "+r" (val)
        :
        : "memory"
    );
}
#endif

static void
CyFx3BootDisableLppPeripherals (
        CyBool_t gpioOn)
{
    if (glLppBlockOn == 0)
    {
        return;
    }

    if (glLppBlockOn & 0x01)
    {
        /* Reset the reset bit */
        I2C->lpp_i2c_power &= ~(CY_U3P_LPP_I2C_RESETN);
    }

    if (glLppBlockOn & 0x02)
    {
        SPI->lpp_spi_power &= ~(CY_U3P_LPP_SPI_RESETN);
    }

    if (glLppBlockOn & 0x04)
    {
        /* Power off UART block. */
        UART->lpp_uart_power &= ~(CY_U3P_LPP_UART_RESETN);
    }

    /* Check if we can turn off the GPIO block. */
    if (gpioOn == CyFalse)
    {
        if (glLppBlockOn & 0x08)
        {
            GPIO->lpp_gpio_power &= ~(CY_U3P_LPP_GPIO_RESETN);
        }

        /* Reset and disable the LPP block */
        LPP->power &= ~CY_U3P_LPP_RESETN;
        while (LPP->power & CY_U3P_LPP_ACTIVE);

        /* Turn off both GPIO fast clock and slow clock. */
        GCTL->gpio_slow_clk = 0;
        CyFx3BootBusyWait (1);
        GCTL->gpio_fast_clk = 0;
    }

    /* Compulsorily turn off the clocks for the I2C, SPI, and UART blocks. */
    GCTL->i2c_core_clk &= (~CY_U3P_GCTL_I2CCLK_CLK_EN);
    GCTL->spi_core_clk &= (~CY_U3P_GCTL_SPICLK_CLK_EN);     
    GCTL->uart_core_clk &= (~CY_U3P_GCTL_UARTCLK_CLK_EN);    

    glLppBlockOn = 0;
}

/*
  Jump to address:
  Only jump to valid address space
 */
static void
jump_to (
        uint32_t add,
        CyBool_t gpioOn)
{
    uint32_t temp1, temp2;

    /* Reset and disable the LPP block(s) */
    CyFx3BootDisableLppPeripherals (gpioOn);

    while (VIC->int_enable)
    {
        VIC->int_clear = 0xFFFFFFFF;
    }

    CyFx3BootIcacheDisable ();
    CyFx3BootBusyWait (10);

    temp1 = GCTLAON->watchdog_cs;
    CyFx3BootBusyWait (10);

    temp2 = 0;

    /* If watchdog 0 is in reset mode, it is allowed to continue. Otherwise it is forcible disabled. */
    if (((temp1 & CY_U3P_GCTL_MODE0_MASK) != CY_U3P_GCTL_MODE0_MASK) &&
            ((temp1 & CY_U3P_GCTL_MODE0_MASK) != CYU3P_WATCHDOG_MODE))
        temp2 = CY_U3P_GCTL_MODE0_MASK;

    /* Forcibly disable watchdog 1 in all cases. */
    if ((temp1 & CY_U3P_GCTL_MODE1_MASK) != CY_U3P_GCTL_MODE1_MASK)
        temp2 |= CY_U3P_GCTL_MODE1_MASK;

    if (temp2 != 0)
    {
        GCTLAON->watchdog_cs = temp1 | temp2;
        CyFx3BootBusyWait (10);
    }


    *(int*)PROG_ENTRY = add; /* save old address */
    GCTLAON->control |= CY_U3P_GCTL_BOOT_COMPLETE;

    jump (add);
}

/* Summary:
   This function transfers the control to the program entry specified in the 
   address parameter.
*/
void 
CyFx3BootJumpToProgramEntry (uint32_t address)
{
    CyBool_t gpioOn = CyFalse;

    if (glNoReenum)
    {
        uint32_t length = 0;
        uint32_t checkSum = 0;
        if (((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->signature == CY_WB_SIGNATURE)
        {
            length = ((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->length;
            /* 4K is allotted for the descriptor table. Checksum is also stored as 
             * part of the descriptor table. The last word of the descriptor table is used
             * to store a variable controlling the jumpback feature. Ensuring that the 
             * (length + 8) doesn't exceed 4K.
             * */
            if ((length > 0) && (length < (0x1000 - 8)))
            {
                CyFx3BootComputeChecksum ((uint32_t *)glUsbDescPtrs, length, &checkSum);
                *(uint32_t*)(glUsbDescPtrs + length) = checkSum;

                /* If the GPIO block is currently ON, and the user has asked for this to be
                   kept ON, we need to tell the code that turns off the LPP peripherals. */
                if ((glLppBlockOn & 0x08) && (((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->leaveGpioOn != 0))
                    gpioOn = CyTrue;
            }
            else
            {
                ((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->length = 0;
                ((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->signature = 0xFFFFFFFF;
            }
        }
    }

    jump_to (address, gpioOn);
}

void 
CyFx3BootDmaXferData (
        CyBool_t isRead,
        uint16_t socket,
        uint32_t address,
        uint32_t length )
{
    PSCK_T s;
    uint16_t id, sc, ch;
    PDSCR_T dscr = DSCR(2);
    uint32_t size = length;
    uint32_t dscrSync = 0;

    s = (PSCK_T)&LPP->sck[socket];

    id = ((uint32_t)s>>16) & 0x1f;     /* use bit16-20 to decode Device ID */
    ch = ((uint32_t)s>>7) & 0x1f;  /* use bit7-11  to decode channel number */

    s->status = CY_U3P_LPP_SCK_STATUS_DEFAULT;

    s->intr = 0xFF;                      /* clear all previous interrupt */
    s->dscr = DSCR_ADDR(dscr);
    s->count= 0;
    dscr->buffer = address;

    sc = (size&0xf) ? (size & cSizeMask)+0x10 : (size&cSizeMask);
    s->size = sc;

    dscrSync = (CY_U3P_LPP_EN_CONS_EVENT | CY_U3P_LPP_EN_CONS_INT | CY_U3P_LPP_EN_PROD_EVENT 
                    | CY_U3P_LPP_EN_PROD_INT);

    if (!isRead) /* 1=TX: SYSMEM to device     */
    {
        dscrSync |= ((ch << CY_U3P_LPP_CONS_SCK_POS) | (id << CY_U3P_LPP_CONS_IP_POS) |
                (CPU_SCK_NUM << CY_U3P_LPP_PROD_SCK_POS) | (CPU_IP_NUM <<
                CY_U3P_LPP_PROD_IP_POS));

        dscr->size = ((size&(cTX_EN-1))<<CY_U3P_LPP_BYTE_COUNT_POS) | sc | CY_U3P_LPP_BUFFER_OCCUPIED;
    }
    else   /* 0=RX: Device to SYSMEM          */
    {
        dscrSync |= ((CPU_SCK_NUM << CY_U3P_LPP_CONS_SCK_POS) | (CPU_IP_NUM << CY_U3P_LPP_CONS_IP_POS) |
                (ch << CY_U3P_LPP_PROD_SCK_POS) | (id <<
                CY_U3P_LPP_PROD_IP_POS));

        dscr->size = sc;
    }

    dscr->sync = dscrSync;

    /* desc sharing?          */
    dscr->chain = (DSCR_ADDR(dscr) << CY_U3P_LPP_RD_NEXT_DSCR_POS) | /* descriptor for rx      */
        (DSCR_ADDR(dscr) << CY_U3P_LPP_WR_NEXT_DSCR_POS) ; /* descriptor for tx        */

    s->status |= CY_U3P_LPP_GO_ENABLE;
}

void
CyFx3BootRetainGpioState (
        void)
{
    ((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->leaveGpioOn = 1;
}

CyFx3BootErrorCode_t
CyFx3BootGpioOverride (
        uint8_t pinNumber)
{
    if (pinNumber >= CY_FX3_BOOT_GPIO_MAX)
        return CY_FX3_BOOT_ERROR_BAD_ARGUMENT;

    if (pinNumber < 32)
    {
        glBootIoCfg.gpioSimpleEn[0] |= (1 << pinNumber);
    }
    else
    {
        glBootIoCfg.gpioSimpleEn[1] |= (1 << (pinNumber - 32));
    }

    GCTL->gpio_simple0 = glBootIoCfg.gpioSimpleEn[0];
    GCTL->gpio_simple1 = glBootIoCfg.gpioSimpleEn[1];
    return CY_FX3_BOOT_SUCCESS;
}

CyFx3BootErrorCode_t
CyFx3BootGpioRestore (
        uint8_t pinNumber)
{
    if (pinNumber >= CY_FX3_BOOT_GPIO_MAX)
        return CY_FX3_BOOT_ERROR_BAD_ARGUMENT;

    if (pinNumber < 32)
    {
        glBootIoCfg.gpioSimpleEn[0] &= ~(1 << pinNumber);
    }
    else
    {
        glBootIoCfg.gpioSimpleEn[1] &= ~(1 << (pinNumber - 32));
    }

    GCTL->gpio_simple0 = glBootIoCfg.gpioSimpleEn[0];
    GCTL->gpio_simple1 = glBootIoCfg.gpioSimpleEn[1];
    return CY_FX3_BOOT_SUCCESS;
}

/*[]*/
