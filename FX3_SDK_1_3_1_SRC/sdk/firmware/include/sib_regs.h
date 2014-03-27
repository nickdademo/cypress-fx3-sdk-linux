/****************************************************************************
 *
 * File: sib_regs.h
 *
 * Copyright (c) 2010-13 Cypress Semiconductor. All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION WHICH IS THE PROPERTY OF CYPRESS.
 *
 * Description:
 *   Storage (SD/MMC/SDIO) interface registers for the EZ-USB FX3S Device.
 *
 *   This file is auto generated from register spreadsheet.
 *   DO NOT MODIFY THIS FILE
 *
 * Use of this file is governed by the license agreement included in the file
 *
 *   <install>/license/license.txt
 *
 * where <install> is the Cypress software installation root directory path.
 *
 ****************************************************************************/

#ifndef _INCLUDED_SIB_REGS_H_
#define _INCLUDED_SIB_REGS_H_

#include <cyu3types.h>

#define SIB_BASE_ADDR                            (0xe0020000)

typedef struct
{
    struct CyU3PSibRegs
    {
        uvint32_t cmd_idx;                        /* 0xe0020000 */
        uvint32_t cmd_arg0;                       /* 0xe0020004 */
        uvint32_t cmd_arg1;                       /* 0xe0020008 */
        uvint32_t resp_idx;                       /* 0xe002000c */
        uvint32_t resp_reg0;                      /* 0xe0020010 */
        uvint32_t resp_reg1;                      /* 0xe0020014 */
        uvint32_t resp_reg2;                      /* 0xe0020018 */
        uvint32_t resp_reg3;                      /* 0xe002001c */
        uvint32_t resp_reg4;                      /* 0xe0020020 */
        uvint32_t cmd_resp_fmt;                   /* 0xe0020024 */
        uvint32_t block_count;                    /* 0xe0020028 */
        uvint32_t blocklen;                       /* 0xe002002c */
        uvint32_t mode_cfg;                       /* 0xe0020030 */
        uvint32_t data_cfg;                       /* 0xe0020034 */
        uvint32_t cs;                             /* 0xe0020038 */
        uvint32_t status;                         /* 0xe002003c */
        uvint32_t intr;                           /* 0xe0020040 */
        uvint32_t intr_mask;                      /* 0xe0020044 */
        uvint32_t ncr;                            /* 0xe0020048 */
        uvint32_t ncc_nwr;                        /* 0xe002004c */
        uvint32_t nac;                            /* 0xe0020050 */
        uvint32_t rsrvd0[235];
    } sdmmc[2];
    uvint32_t rsrvd1[7616];
    uvint32_t id;                                 /* 0xe0027f00 */
    uvint32_t power;                              /* 0xe0027f04 */
    uvint32_t sram_control;                       /* 0xe0027f08 */
    uvint32_t rsrvd2[61];
    struct
    {
        uvint32_t dscr;                           /* 0xe0028000 */
        uvint32_t size;                           /* 0xe0028004 */
        uvint32_t count;                          /* 0xe0028008 */
        uvint32_t status;                         /* 0xe002800c */
        uvint32_t intr;                           /* 0xe0028010 */
        uvint32_t intr_mask;                      /* 0xe0028014 */
        uvint32_t rsrvd3[2];
        uvint32_t dscr_buffer;                    /* 0xe0028020 */
        uvint32_t dscr_sync;                      /* 0xe0028024 */
        uvint32_t dscr_chain;                     /* 0xe0028028 */
        uvint32_t dscr_size;                      /* 0xe002802c */
        uvint32_t rsrvd4[19];
        uvint32_t event;                          /* 0xe002807c */
    } sck[8];
    uvint32_t rsrvd5[7872];
    uvint32_t sck_intr0;                          /* 0xe002ff00 */
    uvint32_t sck_intr1;                          /* 0xe002ff04 */
    uvint32_t sck_intr2;                          /* 0xe002ff08 */
    uvint32_t sck_intr3;                          /* 0xe002ff0c */
    uvint32_t sck_intr4;                          /* 0xe002ff10 */
    uvint32_t sck_intr5;                          /* 0xe002ff14 */
    uvint32_t sck_intr6;                          /* 0xe002ff18 */
    uvint32_t sck_intr7;                          /* 0xe002ff1c */
    uvint32_t rsrvd6[56];
} SIB_REGS_T, *PSIB_REGS_T;

#define SIB        ((PSIB_REGS_T) SIB_BASE_ADDR)


/*
   SD/SDIO/MMC/CE-ATA Command Register
 */
#define CY_U3P_SIB_SDMMC_CMD_IDX_ADDRESS(n)                 (0xe0020000 + ((n) * (0x0400)))
#define CY_U3P_SIB_SDMMC_CMD_IDX(n)                         (*(uvint32_t *)(0xe0020000 + ((n) * 0x0400)))
#define CY_U3P_SIB_SDMMC_CMD_IDX_DEFAULT                    (0x00000000)

/*
   6-bit command   index
 */
#define CY_U3P_SIB_CMD_MASK                                 (0x0000003f) /* <0:5> R:RW:0:No */
#define CY_U3P_SIB_CMD_POS                                  (0)



/*
   SD/MMC/SDIO command argument
 */
#define CY_U3P_SIB_SDMMC_CMD_ARG0_ADDRESS(n)                (0xe0020004 + ((n) * (0x0400)))
#define CY_U3P_SIB_SDMMC_CMD_ARG0(n)                        (*(uvint32_t *)(0xe0020004 + ((n) * 0x0400)))
#define CY_U3P_SIB_SDMMC_CMD_ARG0_DEFAULT                   (0x00000000)

/*
   32-bit command argument
 */
#define CY_U3P_SIB_ARG_MASK                                 (0xffffffff) /* <0:31> R:RW:0:No */
#define CY_U3P_SIB_ARG_POS                                  (0)



/*
   SD/MMC/SDIO expanded command argument
 */
#define CY_U3P_SIB_SDMMC_CMD_ARG1_ADDRESS(n)                (0xe0020008 + ((n) * (0x0400)))
#define CY_U3P_SIB_SDMMC_CMD_ARG1(n)                        (*(uvint32_t *)(0xe0020008 + ((n) * 0x0400)))
#define CY_U3P_SIB_SDMMC_CMD_ARG1_DEFAULT                   (0x00000000)

/*
   32-bit command argument
 */
#define CY_U3P_SIB_ARG_MASK                                 (0xffffffff) /* <0:31> R:RW:0:No */
#define CY_U3P_SIB_ARG_POS                                  (0)



/*
   SD/MMC/SDIO Response Index Register
 */
#define CY_U3P_SIB_SDMMC_RESP_IDX_ADDRESS(n)                (0xe002000c + ((n) * (0x0400)))
#define CY_U3P_SIB_SDMMC_RESP_IDX(n)                        (*(uvint32_t *)(0xe002000c + ((n) * 0x0400)))
#define CY_U3P_SIB_SDMMC_RESP_IDX_DEFAULT                   (0x00000000)

/*
   6-bit command   index as received in response
 */
#define CY_U3P_SIB_CMD_MASK                                 (0x0000003f) /* <0:5> W:R:0:No */
#define CY_U3P_SIB_CMD_POS                                  (0)


/*
   Transmission bit: As received in response.
 */
#define CY_U3P_SIB_TR_BIT                                   (1u << 6) /* <6:6> W :R:0:No */


/*
   Start-bit: As received in response.
 */
#define CY_U3P_SIB_ST_BIT                                   (1u << 7) /* <7:7> W:R:0:No */



/*
   SD/MMC/SDIO Response Register
 */
#define CY_U3P_SIB_SDMMC_RESP_REG0_ADDRESS(n)               (0xe0020010 + ((n) * (0x0400)))
#define CY_U3P_SIB_SDMMC_RESP_REG0(n)                       (*(uvint32_t *)(0xe0020010 + ((n) * 0x0400)))
#define CY_U3P_SIB_SDMMC_RESP_REG0_DEFAULT                  (0x00000000)

/*
   Response bits:
   Response starts in RESP_REG0 with bit[8] of the response going into msb
   bit of the register.
   The next 32 MSBs of response is received in registers RESP_REG1, RESP_REG2,
   … RESP_REG4.
 */
#define CY_U3P_SIB_RESP_L_MASK                              (0xffffffff) /* <0:31> W:R:0:No */
#define CY_U3P_SIB_RESP_L_POS                               (0)



/*
   SD/MMC/SDIO Response Register
 */
#define CY_U3P_SIB_SDMMC_RESP_REG1_ADDRESS(n)               (0xe0020014 + ((n) * (0x0400)))
#define CY_U3P_SIB_SDMMC_RESP_REG1(n)                       (*(uvint32_t *)(0xe0020014 + ((n) * 0x0400)))
#define CY_U3P_SIB_SDMMC_RESP_REG1_DEFAULT                  (0x00000000)

/*
   Response bits:
   Response starts in RESP_REG0 with bit[8] of the response going into msb
   bit of the register.
   The next 32 MSBs of response is received in registers RESP_REG1, RESP_REG2,
   … RESP_REG4.
 */
#define CY_U3P_SIB_RESP_H_L_MASK                            (0xffffffff) /* <0:31> W:R:0:No */
#define CY_U3P_SIB_RESP_H_L_POS                             (0)



/*
   SD/MMC/SDIO Response Register
 */
#define CY_U3P_SIB_SDMMC_RESP_REG2_ADDRESS(n)               (0xe0020018 + ((n) * (0x0400)))
#define CY_U3P_SIB_SDMMC_RESP_REG2(n)                       (*(uvint32_t *)(0xe0020018 + ((n) * 0x0400)))
#define CY_U3P_SIB_SDMMC_RESP_REG2_DEFAULT                  (0x00000000)

/*
   Response bits:
   Response starts in RESP_REG0 with bit[8] of the response going into msb
   bit of the register.
   The next 32 MSBs of response is received in registers RESP_REG1, RESP_REG2,
   … RESP_REG4.
 */
#define CY_U3P_SIB_RESP_H_L_MASK                            (0xffffffff) /* <0:31> W:R:0:No */
#define CY_U3P_SIB_RESP_H_L_POS                             (0)



/*
   SD/MMC/SDIO Response Register
 */
#define CY_U3P_SIB_SDMMC_RESP_REG3_ADDRESS(n)               (0xe002001c + ((n) * (0x0400)))
#define CY_U3P_SIB_SDMMC_RESP_REG3(n)                       (*(uvint32_t *)(0xe002001c + ((n) * 0x0400)))
#define CY_U3P_SIB_SDMMC_RESP_REG3_DEFAULT                  (0x00000000)

/*
   Response bits:
   Response starts in RESP_REG0 with bit[8] of the response going into msb
   bit of the register.
   The next 32 MSBs of response is received in registers RESP_REG1, RESP_REG2,
   … RESP_REG4.
 */
#define CY_U3P_SIB_RESP_H_L_MASK                            (0xffffffff) /* <0:31> W:R:0:No */
#define CY_U3P_SIB_RESP_H_L_POS                             (0)



/*
   SD/MMC/SDIO Response Register
 */
#define CY_U3P_SIB_SDMMC_RESP_REG4_ADDRESS(n)               (0xe0020020 + ((n) * (0x0400)))
#define CY_U3P_SIB_SDMMC_RESP_REG4(n)                       (*(uvint32_t *)(0xe0020020 + ((n) * 0x0400)))
#define CY_U3P_SIB_SDMMC_RESP_REG4_DEFAULT                  (0x00000000)

/*
   Response bits:
   Response starts in RESP_REG0 with bit[8] of the response going into msb
   bit of the register.
   The next 32 MSBs of response is received in registers RESP_REG1, RESP_REG2,
   … RESP_REG4.
 */
#define CY_U3P_SIB_RESP_H_MASK                              (0xffffffff) /* <0:31> W:R:0:No */
#define CY_U3P_SIB_RESP_H_POS                               (0)



/*
   SD/MMC/SDIO Response Format
 */
#define CY_U3P_SIB_SDMMC_CMD_RESP_FMT_ADDRESS(n)            (0xe0020024 + ((n) * (0x0400)))
#define CY_U3P_SIB_SDMMC_CMD_RESP_FMT(n)                    (*(uvint32_t *)(0xe0020024 + ((n) * 0x0400)))
#define CY_U3P_SIB_SDMMC_CMD_RESP_FMT_DEFAULT               (0x007e0025)

/*
   Command length minus 1, in bits, that includes command index bits and
   argument bits. This length does not include start, transmit, CRC7 and
   end bits. The order of transmission is start bit, transmission-bit, command
   index (6 bits), argument (starting from bit 31 of SDMMC_CMD_ARG0), SDMMC_CMD_ARG1
   and finally CRC7 + end-bit.
   Note: 0 implies 1-bit length
   Confirm if this default is correct. (TBD)
 */
#define CY_U3P_SIB_CMDFRMT_MASK                             (0x0000003f) /* <0:5> R:RW:0x25:No */
#define CY_U3P_SIB_CMDFRMT_POS                              (0)


/*
   Response length in bits that does not include start, transmit, CRC7 and
   end bit.
   0 indicates that no response is expected.
   Non-zero values indicate the size in bits, of the response index and argument.
 */
#define CY_U3P_SIB_RESPCONF_MASK                            (0x07ff0000) /* <16:26> R:RW:0x7E:No */
#define CY_U3P_SIB_RESPCONF_POS                             (16)


/*
   enable CRC error injection on outgoing data crc16 field.
 */
#define CY_U3P_SIB_CORCRC                                   (1u << 27) /* <27:27> R:RW:0:No */


/*
   1: Enable CRC check in response
   0: Don’t check CRC in response.
   Pls note that end-bit of RESPONSE is never checked in hardware.
 */
#define CY_U3P_SIB_R_CRC_EN                                 (1u << 28) /* <28:28> R:RW:0:No */


/*
   0=hardware does the following:
      1. Checks start/transmit bits in the RESPONSE
      2. Expect crc7 bits, calculate and check CRC7
      3. Store CRC7 + end bit in response register
   1=hardware does the followings:
      1. Skip start/transmit bits checking RESPONSE data
      2. Skip  CRC7 + end bit
   Not Implemented
 */
#define CY_U3P_SIB_RESP_ST_TR_DISABLE                       (1u << 29) /* <29:29> R:R :0:No */


/*
   0: corrupt CRC16 for even bytes
   1: corrupt CRC16 for odd bytes
   This bit is effective during DDR mode of operation
 */
#define CY_U3P_SIB_CORCRC_ODD                               (1u << 30) /* <30:30> R:RW:0:No */


/*
   0: Compute response crc from bit 0
   1: Compute response crc from bit 8
   Option 1 is used for R2 response.
 */
#define CY_U3P_SIB_RESPCRC_START                            (1u << 31) /* <31:31> R:RW:0:No */



/*
   SD/MMC/SDIO Block Count Register
 */
#define CY_U3P_SIB_SDMMC_BLOCK_COUNT_ADDRESS(n)             (0xe0020028 + ((n) * (0x0400)))
#define CY_U3P_SIB_SDMMC_BLOCK_COUNT(n)                     (*(uvint32_t *)(0xe0020028 + ((n) * 0x0400)))
#define CY_U3P_SIB_SDMMC_BLOCK_COUNT_DEFAULT                (0x00000001)

/*
   0=4G blocks, 1-0xFFFF_FFFF = number of blocks of data to transfer before
   causing the all blocks sent or received interrupt.
 */
#define CY_U3P_SIB_NOBD_MASK                                (0xffffffff) /* <0:31> R:RW:0x01:No */
#define CY_U3P_SIB_NOBD_POS                                 (0)



/*
   SD/MMC/SDIO Block Length Register
 */
#define CY_U3P_SIB_SDMMC_BLOCKLEN_ADDRESS(n)                (0xe002002c + ((n) * (0x0400)))
#define CY_U3P_SIB_SDMMC_BLOCKLEN(n)                        (*(uvint32_t *)(0xe002002c + ((n) * 0x0400)))
#define CY_U3P_SIB_SDMMC_BLOCKLEN_DEFAULT                   (0x00000200)

/*
   Data block size in bytes. LSB is ignored for DDR mode of operation, since
   block length needs to be even.
 */
#define CY_U3P_SIB_DATABLKS_MASK                            (0xffffffff) /* <0:31> R:RW:512:No */
#define CY_U3P_SIB_DATABLKS_POS                             (0)



/*
   SD/MMC/SDIO Mode Configuration Register
 */
#define CY_U3P_SIB_SDMMC_MODE_CFG_ADDRESS(n)                (0xe0020030 + ((n) * (0x0400)))
#define CY_U3P_SIB_SDMMC_MODE_CFG(n)                        (*(uvint32_t *)(0xe0020030 + ((n) * 0x0400)))
#define CY_U3P_SIB_SDMMC_MODE_CFG_DEFAULT                   (0x10000420)

/*
   0 - DLL is used for Sample clocks and PIN clock. DLL should be used for
   frequencies beyond 26 MHz for SDR and DDR operation.
   1 - DLL is bypassed for both (input) sample and (output) pin clocks.
 */
#define CY_U3P_SIB_DLL_BYPASS_EN                            (1u << 5) /* <5:5> R:RW:1:No */


/*
   0000: DS - Default-Speed (0 - 20 MHz for MMC; 0 - 25 MHz for SD)
   0001: HS - High-Speed (0 - 52 MHz for MMC; 0 - 50 MHz for SD)
   0010: SDR12: SD SDR 25 MHz @1.8V (SD3.0 defined)
   0011: SDR25: SD SDR 50 MHz @1.8V (SD3.0 defined)
   0100: SDR50_FD: SD SDR 100 MHz @ 1.8V (SD3.0 defined)
   0101: Reserved
   0110: Reserved
   0111: Reserved
   1000: DDR52_V33_MMC: As defined in eMMC4.4
   1001: DDR52_V18_MMC: As defined in eMMC4.4
   1010: Reserved.
   1011: DDR50_V18_SD: As defined in SD3.0
   1100 - 1111: Reserved
 */
#define CY_U3P_SIB_SIGNALING_MASK                           (0x000003c0) /* <6:9> R:RW:0:No */
#define CY_U3P_SIB_SIGNALING_POS                            (6)


/*
   00: Reserved
   01: MMC (CE-ATA)
   10: SD
   11: (reserved) TBD: Document difference in existing controller here.
 */
#define CY_U3P_SIB_MODE_MASK                                (0x00000c00) /* <10:11> R:RW:1:No */
#define CY_U3P_SIB_MODE_POS                                 (10)


/*
   00: 1-bit, 01: 4-bit, 10: 8-bit, 11: reserved
 */
#define CY_U3P_SIB_DATABUSWIDTH_MASK                        (0x00003000) /* <12:13> R:RW:0:No */
#define CY_U3P_SIB_DATABUSWIDTH_POS                         (12)


/*
   Enable command completion feature (CE_ATA):
   0: Don’t detect command completion event
   1: Detect command completion event
   When EN_CMD_COMP is enable RD_END_STOP_CLK should not be enabled since
   card requires a clock edge to provide command completion signal.
   This bit should be set only when command completion is expected from card
   and not otherwise.
 */
#define CY_U3P_SIB_EN_CMD_COMP                              (1u << 14) /* <14:14> R:RW:0:No */


/*
   This enables SIB to perform detection of overflow and stop the clock to
   the SD/SDIO/MMC card during data transfer
   1: Enable stop clock
   0: Disable stop clock
   (previously STOP_CLOCK_EN)
 */
#define CY_U3P_SIB_RD_STOP_CLK_EN                           (1u << 15) /* <15:15> R:RW:0:No */


/*
   This enables SIB to perform detection of underflow and stop the clock
   to the SD/SDIO/MMC card during data transfer.
   1: Enable stop clock
   0: Disable stop clock
 */
#define CY_U3P_SIB_WR_STOP_CLK_EN                           (1u << 16) /* <16:16> R:RW:0:No */


/*
   0: CardDetect is active LOW
   1: CardDetect is active HIGH
   This value is used by hardware to reflect S0_INS/S1_INS status in the
   SDMMC_STATUS.card_detect register bit.
   Note: There will 2 such bits and FW is expected to mask the unused SDMMC_MASK.card_detect
    .
 */
#define CY_U3P_SIB_CARD_DETECT_POLARITY                     (1u << 18) /* <18:18> R:RW:0:No */


/*
   0: don’t stop clock at end of read data transfer.
   1: Stop clock at the end of read data transfer.
   When RD_END_STOP_CLK_EN=1, and a command is in progress at the end of
   read, clock will not be stopped. FW should ensure that this condition
   does not occur.
 */
#define CY_U3P_SIB_RD_END_STOP_CLK_EN                       (1u << 19) /* <19:19> R:RW:0:No */


/*
   (Reserved for memory stick; not implemented)
   0=SD/SDIO/MMC CRC16 using G(X) = X^16 + X^12 + X^5 + 1
   1=CRC16 using G(X)=X^16 + X^15 + X^2 + 1
 */
#define CY_U3P_SIB_CRC16_TYPE                               (1u << 20) /* <20:20> R:R:0:No */


/*
   Determines if CLK is to be stopped in between two blocks or if it can
   be stopped in the middle of a block transfer when either or both RD_STOP_CLK_EN
   or WR_STOP_CLK_EN is set.
   0: SDMMC interface clock can be stopped at any time when data is not available
   writing to card or space is not available when reading from card.
   1: SDMMC interface clock shall be stopped only at the end of data block
   transfer. Clock shall not be stopped in the middle of a block data transfer.
   When this bit is set buffers shall have integer number of blocks.
   BLK_END_STOP_CLK must be enabled for DDR, SDR50 and SDR104 modes of operation.
   BLK_END_STOP_CLK must be enabled whenever buufer switch time (~ 0.25 to
   1 usec) is larger than inter block gap for read.
 */
#define CY_U3P_SIB_BLK_END_STOP_CLK                         (1u << 21) /* <21:21> R:RW:0:No */


/*
   1 to indicate that boot acknowledge is expected
   0 to indicate that boot acknowledge is not expected.
 */
#define CY_U3P_SIB_EXP_BOOT_ACK                             (1u << 22) /* <22:22> R:RW:0:No */


/*
   Number of clock cycles delay after the completion of the last command
   after which the clock will be stopped. This value may be changed only
   when IDLE_STOP_CLK_EN is 0.
 */
#define CY_U3P_SIB_IDLE_STOP_CLK_DELAY_MASK                 (0x7f000000) /* <24:30> R:RW:16:No */
#define CY_U3P_SIB_IDLE_STOP_CLK_DELAY_POS                  (24)


/*
   0: Don't stop clock automatically after each command
   1: Stop clock automatically after each command, after IDLE_STOP_CLK_DELAY.
   Clock is not stopped if card indicates busy and stopped when card gets
   out of busy.
   Note(1) that DLL is shut off , and hence pin clock is off. In addition
   core clock is also gated. When a subsequent operation is initiated SIB
   will restart clocks and wait for DLL lock before proceeding with the operation.
   Note(2) this bit should be disabled if DLL is enabled when clock frequency
   is changed.
   Note (3) When clearing this bit (a) Wait for S-port to go to idle by monitoring
   SDMMCU.COMMAND_SM_BUSY and SDMMCU_DATA_SM_BUSY to go to zero. (b) Wait
   for (IDLE_STOP_CLK_DELAY + 6) core clock cycles and then set to 0. Six
   cycles are required for SIB to disable DLL clock.
 */
#define CY_U3P_SIB_IDLE_STOP_CLK_EN                         (1u << 31) /* <31:31> R:RW:0:No */



/*
   SD/MMC/SDIO Data Transfer Configuration Register
 */
#define CY_U3P_SIB_SDMMC_DATA_CFG_ADDRESS(n)                (0xe0020034 + ((n) * (0x0400)))
#define CY_U3P_SIB_SDMMC_DATA_CFG(n)                        (*(uvint32_t *)(0xe0020034 + ((n) * 0x0400)))
#define CY_U3P_SIB_SDMMC_DATA_CFG_DEFAULT                   (0x0000000d)

/*
   1=Check CRC16 on incoming blocks of data
   0=Ignore CRC16 checking on incoming data
 */
#define CY_U3P_SIB_RD_CRC_EN                                (1u << 0) /* <0:0> R:RW:1:No */


/*
   1=enable the SD/SDIO/MMC state machine to wait for busy before sending
   1st block of data
 */
#define CY_U3P_SIB_CARD_BUSY_DET                            (1u << 1) /* <1:1> R:RW:0:No */


/*
   1: Expect and check CRC response status after block of write data
   
   0: Don’t expect CRC response after a block of write data
 */
#define CY_U3P_SIB_EXPCRCR                                  (1u << 2) /* <2:2> R:RW:1:No */


/*
   1: Expect BUSY state after each block of write data
   0: Don’t expect BUSY after each block of write-data.
 */
#define CY_U3P_SIB_EXPBUSY                                  (1u << 3) /* <3:3> R:RW:1:No */



/*
   SD/MMC/SDIO Card Command and Status Register
 */
#define CY_U3P_SIB_SDMMC_CS_ADDRESS(n)                      (0xe0020038 + ((n) * (0x0400)))
#define CY_U3P_SIB_SDMMC_CS(n)                              (*(uvint32_t *)(0xe0020038 + ((n) * 0x0400)))
#define CY_U3P_SIB_SDMMC_CS_DEFAULT                         (0x00000000)

/*
   FW writes 1 to initate command Send Command
   HW writes 0 when command has been sent and response, if any, has been
   received.
   FW may clear this bit on response timeout by writing 1 to CLR_SNDCMD.
 */
#define CY_U3P_SIB_SNDCMD                                   (1u << 0) /* <0:0> RW0C:RW1S:0:Yes */


/*
   SW writes 1 to initiate Send write data to card
   HW writes 0 immeditaly after NOBD blocks have been transferred and CRC
   status received.
   FW may clear this bit by writing 1 to CLR_WRDCARD to stop the write operation;
   before clearing this bit FW has to ensure that socket is suspended at
   block boundary so that write stop s at block boundary.
 */
#define CY_U3P_SIB_WRDCARD                                  (1u << 1) /* <1:1> RW0C:RW1S:0:Yes */


/*
   SW writes 1 to initiate Read data from card; The RDDCARD bit needs to
   be set along with SNDCMD to prevent loss of data if the card responds
   faster than the CPU can set this bit after SNDCMD is set to 1.
   HW writes 0 immeditaly after NOBD blocks have been transferred.
   FW may clear this bit by writing '1' to CLR_RDDCARD to stop the read operation;
   before clearing this bit FW has to ensure that socket is suspended at
   block boundary so that read stops at block boundary.
 */
#define CY_U3P_SIB_RDDCARD                                  (1u << 2) /* <2:2> RW0C:RW1S:0:Yes */


/*
   SW writes 1 to Send Abort signal to SIB
   HW writes 0 when abort completes
   (Not implemented)
 */
#define CY_U3P_SIB_SNDABORT                                 (1u << 4) /* <4:4> R:R:0:No */


/*
   RSTCONT is used for error recovery. SW writes 1 to bring SIB FSMs to a
   known state. HW writes 0 when reset completes.
   
   Values of CFG registers are not affected by RSTCONT. Internal queues are
   flushed by RSTCONT. If a complete reset of SIB controller is required
   then SIB_POWER.RESETN should be used, which resets both SIB controllers.
 */
#define CY_U3P_SIB_RSTCONT                                  (1u << 5) /* <5:5> RW0C:RW1S:0:Yes */


/*
   SW writes 1 to initiate a CMD_LINE_LOW boot or Alternate boot command.
   With Alternate boot command CPU has to send the command.
   HW writes 0 when NOBD blocks have beed transferred or when SW writes 0
   to BOOTCMD. FW clears this bit by writing '1' to CLR_BOOTCMD to abort
   CMD_LINE_LOW or Alternate boot command operation, in cause of boot-ack
   time out or data crc error.
 */
#define CY_U3P_SIB_BOOTCMD                                  (1u << 6) /* <6:6> RW0C:RW1S:0:Yes */


/*
   SW writes 1 to send a Command Completion Disable signal to device. (For
   CE_ATA).  HW clears this bit after signal is sent FW has to ensure that
   Nrc delay is provided from last response. FW can do this by always waiting
   Nrc cycles before sending command completion disable.
 */
#define CY_U3P_SIB_CMDCOMP_DIS                              (1u << 7) /* <7:7> RW0C:RW1S:0:Yes */


/*
   SW writes 1 to clear SNDCMD. This used during a response timeout so that
   next command can be issued.
   This bit is cleared by HW when SNDCMD is cleared.
 */
#define CY_U3P_SIB_CLR_SNDCMD                               (1u << 8) /* <8:8> RW0C:RW1S:0:yes */


/*
   SW writes 1 to clear WRDCARD. This used to abort a write operation..
   This bit is cleared by HW when write operation is aborted.
 */
#define CY_U3P_SIB_CLR_WRDCARD                              (1u << 9) /* <9:9> RW0C:RW1S:0:yes */


/*
   SW writes 1 to clear RDDCARD. This used to abort a read operation. This
   bit is cleared by HW when read operation is aborted.
 */
#define CY_U3P_SIB_CLR_RDDCARD                              (1u << 10) /* <10:10> RW0C:RW1S:0:yes */


/*
   SW writes 1 to clear BOOTCMD. This used toindicate that boot operation
   is over (for both command-line-low and alternate-boot)..
   Thi bit is cleared by HW when boot is terminated.
 */
#define CY_U3P_SIB_CLR_BOOTCMD                              (1u << 11) /* <11:11> RW0C:RW1S:0:yes */


/*
   Manual enabling/disabling of the SIB clocks. Power saving mode: 1=clock
   disable, 0 = enable
   If clock is disabled no command/read/write operation will work.
   sdmmc_clk_dis is a global control for DLL input clock ( and hence PIN
   CLK) and core clock.
   This should not be changed dynamically during data phase.
 */
#define CY_U3P_SIB_SDMMC_CLK_DIS                            (1u << 14) /* <14:14> R:RW:0:Yes */


/*
   Enable Read Wait to SDIO device on DAT[2]
 */
#define CY_U3P_SIB_SDIO_READ_WAIT_EN                        (1u << 15) /* <15:15> R:RW:0:Yes */


/*
   Socket number for data read or write
 */
#define CY_U3P_SIB_SOCKET_MASK                              (0x1f000000) /* <24:28> R:RW:0:No */
#define CY_U3P_SIB_SOCKET_POS                               (24)


/*
   Configure end-of-packet signaling to DMA adapter.
   0: Set EOP in the descriptor with last byte of last block during read.
   1: Set EOP in the descriptor with last byte of every block.
 */
#define CY_U3P_SIB_EOP_EOT                                  (1u << 31) /* <31:31> R:RW:0:No */



/*
   SD/MMC/SDIO Status Regsiter
 */
#define CY_U3P_SIB_SDMMC_STATUS_ADDRESS(n)                  (0xe002003c + ((n) * (0x0400)))
#define CY_U3P_SIB_SDMMC_STATUS(n)                          (*(uvint32_t *)(0xe002003c + ((n) * 0x0400)))
#define CY_U3P_SIB_SDMMC_STATUS_DEFAULT                     (0x00008000)

/*
   1=Command has been sent
   This bit is cleared when the FW writes 1 to SNDCMD.
 */
#define CY_U3P_SIB_CMDSENT                                  (1u << 0) /* <0:0> RW:R:0:Yes */


/*
   1=Received response from the card
   This bit is cleared when FW writes 1 to SNDCMD.
 */
#define CY_U3P_SIB_RCVDRES                                  (1u << 1) /* <1:1> RW:R:0:Yes */


/*
   1=response time-out was detected(i.e. the SIB_NCR timer expired)
   This is cleared when FW writes 1 to SNDCMD
 */
#define CY_U3P_SIB_RESPTIMEOUT                              (1u << 2) /* <2:2> RW:R:0:Yes */


/*
   1=CRC7 error on received response from card
   This bit is cleared when FW writes 1 to SNDCMD
 */
#define CY_U3P_SIB_CRC7_ERROR                               (1u << 3) /* <3:3> RW:R:0:Yes */


/*
   1=the programmed # of Blocks have been sent and CRC is verified
   This bit is cleared when FW writes 1 to WRDCARD.
 */
#define CY_U3P_SIB_BLOCK_COMP                               (1u << 4) /* <4:4> RW:R:0:Yes */


/*
   1=the programmed # of Blocks have been received
   This bit is cleared when FW writes 1 to RDDCARD.
 */
#define CY_U3P_SIB_BLOCKS_RECEIVED                          (1u << 5) /* <5:5> RW:R:0:Yes */


/*
   1=Timeout while waiting for Read data
   This bit is cleared when FW writes 1 to RDDCARD..
 */
#define CY_U3P_SIB_RD_DATA_TIMEOUT                          (1u << 6) /* <6:6> RW:R:0:Yes */


/*
   1= detected CRC16 error on received data or in CRC status response at
   the end of data block write (previously named CRCDATA). This bit is cleared
   when FW writes 1 to WRDCARD or RDDCARD.
 */
#define CY_U3P_SIB_CRC16_ERROR                              (1u << 7) /* <7:7> RW:R:0:Yes */


/*
   1= detected an SDIO interrupt condition as per SDIO protocol
   This bit represents the last sample of the interrupt condition from the
   card. FW shall clear SDMMC_INTR register bit to clear interrupt condition.
   For read/write operation RDDCARD/WRDCARD must be set along with SNDCMD
   to ensure that SIB does not start the interrupt period 2 cycles after
   end-bit of command, when data is to follow (when in 4-bit transfer mode).
   For open ended multi-block reads, to avoid SDIO interrupt when NOBD becomes
   zero, RD_END_STOP_CLK may be used.
 */
#define CY_U3P_SIB_SDIO_INTR                                (1u << 8) /* <8:8> RW:R:0:Yes */


/*
   0 = No card
   1 = Card present
   S0_INS/S1_INS pin is used along with card_detect_polarity to determin
   this status bit.  Note that this pin tracks its assigned GPIO pin, regardless
   of whether it is used as a card-insert pin or not.
 */
#define CY_U3P_SIB_CARD_DETECT                              (1u << 9) /* <9:9> RW:R:X:Yes */


/*
   Data3 pin pad level
   FW should check this status only when in non-data transfer state.
 */
#define CY_U3P_SIB_DAT3_STAT                                (1u << 10) /* <10:10> RW:R:0:Yes */


/*
   1=FIFO over flow status during receiving of data. This interrupt is caused
   and bit set only if RD_STOP_CLK_EN is set to 0.
 */
#define CY_U3P_SIB_FIFO_O_DET                               (1u << 11) /* <11:11> RW:R:0:Yes */


/*
   1=FIFO under flow status during write to card. This interrupt is caused
   and bit set only if WR_STOP_CLK_EN is set to 0.
 */
#define CY_U3P_SIB_FIFO_U_DET                               (1u << 12) /* <12:12> RW:R:0:Yes */


/*
   1=Command complete received from device
   This bit is cleared when FW writes a 1 to SNDCMD
 */
#define CY_U3P_SIB_CMD_COMP                                 (1u << 13) /* <13:13> RW:R:0:Yes */


/*
   1=Boot Ack received
   This bit is cleared when EXP_BOOT_ACK is cleared by SW.
 */
#define CY_U3P_SIB_BOOT_ACK                                 (1u << 14) /* <14:14> RW:R:0:Yes */


/*
   1=DDL is not locked
   0=DLL is locked
   Set and cleared by hardware
 */
#define CY_U3P_SIB_DLL_LOST_LOCK                            (1u << 15) /* <15:15> RW:R:1:yes */


/*
   0=DDL is not locked
   1=DLL is locked
   Set and cleared by hardware
 */
#define CY_U3P_SIB_DLL_LOCKED                               (1u << 16) /* <16:16> RW:R:0:Yes */


/*
   Data0 pin pad level
   FW should check this status only when in non-data transfer state.
 */
#define CY_U3P_SIB_DAT0_STAT                                (1u << 17) /* <17:17> RW:R:0:Yes */


/*
   This error is flagged when command, response or Nrc counting is in progress
   when the last byte of last block of read-data is read with RD_END_CLK_STOP
   is enabled. This condition causes data from card to be read out of card
   and dropped. FW should re-issue read command to card to restart read command.
   This bit is cleared when RDDCARD is set by FW.
 */
#define CY_U3P_SIB_RD_END_DATA_ERROR                        (1u << 18) /* <18:18> RW:R:0:Yes */


/*
   3-bit CRC response from the card following a data write
 */
#define CY_U3P_SIB_CRCFC_MASK                               (0x07000000) /* <24:26> RW:R:0:Yes */
#define CY_U3P_SIB_CRCFC_POS                                (24)


/*
   Command State Machine Busy
 */
#define CY_U3P_SIB_COMMAND_SM_BUSY                          (1u << 27) /* <27:27> RW:R:0:Yes */


/*
   Data State Machine Busy
 */
#define CY_U3P_SIB_DATA_SM_BUSY                             (1u << 28) /* <28:28> RW:R:0:Yes */


/*
   Indicates that CRC error was encountered on odd bytes of data transfer.
   This bit may be interpreted in DDR mode data transfer only.
   This bit is to be cleared when CRC16_ERROR is cleared.
 */
#define CY_U3P_SIB_CRC16_ODD_ERROR                          (1u << 30) /* <30:30> RW:R:0:Yes */


/*
   Indicates that CRC error was encountered on even bytes of data transfer.
   This bit may be interpreted in DDR mode data transfer only.This bit is
   to be cleared when CRC16_ERROR is cleared.
 */
#define CY_U3P_SIB_CRC16_EVEN_ERROR                         (1u << 31) /* <31:31> RW:R:0:Yes */



/*
   SD/MMC/SDIO Interrup Request Register
 */
#define CY_U3P_SIB_SDMMC_INTR_ADDRESS(n)                    (0xe0020040 + ((n) * (0x0400)))
#define CY_U3P_SIB_SDMMC_INTR(n)                            (*(uvint32_t *)(0xe0020040 + ((n) * 0x0400)))
#define CY_U3P_SIB_SDMMC_INTR_DEFAULT                       (0x00000000)

/*
   1=Command has been sent
 */
#define CY_U3P_SIB_CMDSENT                                  (1u << 0) /* <0:0> W1S:RW1C:0:N/A */


/*
   1=Received response from the card
 */
#define CY_U3P_SIB_RCVDRES                                  (1u << 1) /* <1:1> W1S:RW1C:0:N/A */


/*
   1=response time-out was detected(i.e. the SIB_NCR timer expired)
 */
#define CY_U3P_SIB_RESPTIMEOUT                              (1u << 2) /* <2:2> W1S:RW1C:0:N/A */


/*
   1=CRC7 error on received response from card
 */
#define CY_U3P_SIB_CRC7_ERROR                               (1u << 3) /* <3:3> W1S:RW1C:0:N/A */


/*
   1=the programmed # of Blocks have been sent and CRC verified.
 */
#define CY_U3P_SIB_BLOCK_COMP                               (1u << 4) /* <4:4> W1S:RW1C:0:N/A */


/*
   1=the programmed # of Blocks have been received
 */
#define CY_U3P_SIB_BLOCKS_RECEIVED                          (1u << 5) /* <5:5> W1S:RW1C:0:N/A */


/*
   1=Timeout while waiting for Read data
   This rd_data_timeout also occurs boot data read times out.
 */
#define CY_U3P_SIB_RD_DATA_TIMEOUT                          (1u << 6) /* <6:6> W1S:RW1C:0:N/A */


/*
   1= detected CRC16 error on received data or in CRC status response at
   the end of data block write (previously named CRCDATA)
 */
#define CY_U3P_SIB_CRC16_ERROR                              (1u << 7) /* <7:7> W1S:RW1C:0:N/A */


/*
   1= detected an SDIO interrupt condition as per SDIO protocol
   Programmer should mask this interrupt during SD operations since this
   bit could get set under a few SD command sequences (CMD6, etc)
 */
#define CY_U3P_SIB_SDIO_INTR                                (1u << 8) /* <8:8> W1S:RW1C:0:N/A */


/*
   1=Card Insert/Remove IRQ using SD/SDIO/MMC Insert/Remove signal
   (SDMMC_STATUS.card_ins changes from 0 to 1 or 1 to 0)
 */
#define CY_U3P_SIB_CARD_DETECT                              (1u << 9) /* <9:9> W1S:RW1C:0:N/A */


/*
   1= DAT3 card insertion or card extraction interrupt detected
   (SDMMC_STATUS.dat3_stat changed from 0 to 1 or 1 to 0)
   FW is expected to mask this interrupt when dat3 line is used for data
   transfer.
 */
#define CY_U3P_SIB_DAT3_CHANGE                              (1u << 10) /* <10:10> W1S:RW1C:0:N/A */


/*
   1=FIFO over flow detected during receiving of data. This interrupt is
   caused and bit set only if RD_STOP_CLK_EN is set to 0.
 */
#define CY_U3P_SIB_FIFO_O_DET                               (1u << 11) /* <11:11> W1S:RW1C:0:N/A */


/*
   1=FIFO under flow detected during write to card. This interrupt is caused
   and bit set only if WR_STOP_CLK_EN is set to 0.
 */
#define CY_U3P_SIB_FIFO_U_DET                               (1u << 12) /* <12:12> W1S:RW1C:0:N/A */


/*
   1= Command completion interrupt (CE-ATA)
 */
#define CY_U3P_SIB_CMD_COMP                                 (1u << 13) /* <13:13> W1S:RW1C:0:N/A */


/*
   1=Boot Ack received
 */
#define CY_U3P_SIB_BOOT_ACK                                 (1u << 14) /* <14:14> W1S:RW1C:0:N/A */


/*
   1=DLL out of lock
 */
#define CY_U3P_SIB_DLL_LOST_LOCK                            (1u << 15) /* <15:15> W1S:RW1C:0:N/A */


/*
   1=DLL achieved lock
 */
#define CY_U3P_SIB_DLL_LOCKED                               (1u << 16) /* <16:16> W1S:RW1C:0:N/A */


/*
   1=DAT0 changed from 0 to 1 (positive edge detected)
   FW is expected mask this interrupt during data transfers.
 */
#define CY_U3P_SIB_DAT0_OUTOF_BUSY                          (1u << 17) /* <17:17> W1S:RW1C:0:N/A */


/*
   1=Error detected. See corresponding status field description.
 */
#define CY_U3P_SIB_RD_END_DATA_ERROR                        (1u << 18) /* <18:18> W1S:RW1C:0:N/A */



/*
   SD/MMC/SDIO Interrup Mask Register
 */
#define CY_U3P_SIB_SDMMC_INTR_MASK_ADDRESS(n)               (0xe0020044 + ((n) * (0x0400)))
#define CY_U3P_SIB_SDMMC_INTR_MASK(n)                       (*(uvint32_t *)(0xe0020044 + ((n) * 0x0400)))
#define CY_U3P_SIB_SDMMC_INTR_MASK_DEFAULT                  (0x00000000)

/*
   1 = Enable Command Sent IRQ
 */
#define CY_U3P_SIB_CMDSENT                                  (1u << 0) /* <0:0> R:RW:0:N/A */


/*
   1= Enable received-response IRQ
 */
#define CY_U3P_SIB_RCVDRES                                  (1u << 1) /* <1:1> R:RW:0:N/A */


/*
   1= Enalbe Response Timeout IRQ
 */
#define CY_U3P_SIB_RESPTIMEOUT                              (1u << 2) /* <2:2> R:RW:0:N/A */


/*
   1= Enalbe CRC7 detect Error IRQ
 */
#define CY_U3P_SIB_CRC7_ERROR                               (1u << 3) /* <3:3> R:RW:0:N/A */


/*
   1= Enable All blocks sent IRQ
 */
#define CY_U3P_SIB_BLOCK_COMP                               (1u << 4) /* <4:4> R:RW:0:N/A */


/*
   1= Enable all blocks received IRQ
 */
#define CY_U3P_SIB_BLOCKS_RECEIVED                          (1u << 5) /* <5:5> R:RW:0:N/A */


/*
   1= Enable Read Data Timeout IRQ
 */
#define CY_U3P_SIB_RD_DATA_TIMEOUT                          (1u << 6) /* <6:6> R:RW:0:N/A */


/*
   1= Enable CRC16 received data error IRQ
 */
#define CY_U3P_SIB_CRC16_ERROR                              (1u << 7) /* <7:7> R:RW:0:N/A */


/*
   1 = Enable sdio_intr IRQ
   Programmer should mask this interrupt during SD operations since this
   bit could get set under a few SD command sequences (CMD6, etc)
 */
#define CY_U3P_SIB_SDIO_INTR                                (1u << 8) /* <8:8> R:RW:0:N/A */


/*
   1= Enable card detect IRQ
 */
#define CY_U3P_SIB_CARD_DETECT                              (1u << 9) /* <9:9> R:RW:0:N/A */


/*
   1=enable DAT[3] SD/SDIO/MMC bus interrupt
 */
#define CY_U3P_SIB_DAT3_CHANGE                              (1u << 10) /* <10:10> R:RW:0:N/A */


/*
   1=FIFO over flow detect during receiving data
 */
#define CY_U3P_SIB_FIFO_O_DET                               (1u << 11) /* <11:11> R:RW:0:N/A */


/*
   1=FIFO underflow detect during writing data to card.
 */
#define CY_U3P_SIB_FIFO_U_DET                               (1u << 12) /* <12:12> R:RW:0:N/A */


/*
   1=Enable command completion interrupt
 */
#define CY_U3P_SIB_CMD_COMP                                 (1u << 13) /* <13:13> R:RW:0:N/A */


/*
   1=Boot Ack received
 */
#define CY_U3P_SIB_BOOT_ACK                                 (1u << 14) /* <14:14> R:RW:0:N/A */


/*
   1=DLL out of lock
 */
#define CY_U3P_SIB_DLL_LOST_LOCK                            (1u << 15) /* <15:15> R:RW:0:N/A */


/*
   1=DLL achieved lock
 */
#define CY_U3P_SIB_DLL_LOCKED                               (1u << 16) /* <16:16> R:RW:0:N/A */


/*
   1=enable DAT[0] SD/SDIO/MMC bus interrupt
 */
#define CY_U3P_SIB_DAT0_OUTOF_BUSY                          (1u << 17) /* <17:17> R:RW:0:N/A */


/*
   1=enable RD_END_DATA_ERROR interrupt
 */
#define CY_U3P_SIB_RD_END_DATA_ERROR                        (1u << 18) /* <18:18> R:RW:0:N/A */



/*
   SD/MMC/SDIO Command Response Timing Register #1
 */
#define CY_U3P_SIB_SDMMC_NCR_ADDRESS(n)                     (0xe0020048 + ((n) * (0x0400)))
#define CY_U3P_SIB_SDMMC_NCR(n)                             (*(uvint32_t *)(0xe0020048 + ((n) * 0x0400)))
#define CY_U3P_SIB_SDMMC_NCR_DEFAULT                        (0x00000847)

/*
   Maximum number of clock cycles between command and response (NCR)
   The value of 0 corresponds to 0 cycles
   maximum number of clock cycles between command and response (NCR) The
   user should program Counter value as "card NCR + 7" or higher.
   If response is not received within this programmed number of clock cycles
   timeout is flagged (previously MAX_CLK)
 */
#define CY_U3P_SIB_NCR_MAX_MASK                             (0x0000007f) /* <0:6> R:RW:0x47:No */
#define CY_U3P_SIB_NCR_MAX_POS                              (0)


/*
   Number of clock cycles between last bit of response and next command (NRC).
   The value of 0 corresponds to 0 cycles. (previously NUMCLK)
 */
#define CY_U3P_SIB_NRC_MIN_MASK                             (0x0000ff00) /* <8:15> R:RW:0x08:No */
#define CY_U3P_SIB_NRC_MIN_POS                              (8)



/*
   SD/MMC/SDIO Command Response Timing Register #2
 */
#define CY_U3P_SIB_SDMMC_NCC_NWR_ADDRESS(n)                 (0xe002004c + ((n) * (0x0400)))
#define CY_U3P_SIB_SDMMC_NCC_NWR(n)                         (*(uvint32_t *)(0xe002004c + ((n) * 0x0400)))
#define CY_U3P_SIB_SDMMC_NCC_NWR_DEFAULT                    (0x00000208)

/*
   Number of clock cycles between two consecutive command. (previously NUMCONSE)
 */
#define CY_U3P_SIB_NCC_MIN_MASK                             (0x0000000f) /* <0:3> R:RW:8:No */
#define CY_U3P_SIB_NCC_MIN_POS                              (0)


/*
   Number of clock cycles between command response and write data (previously
   NUMCRES)
 */
#define CY_U3P_SIB_NWR_MIN_MASK                             (0x0000ff00) /* <8:15> R:RW:2:No */
#define CY_U3P_SIB_NWR_MIN_POS                              (8)



/*
   SD/MMC/SDIO Read Timeout Register
 */
#define CY_U3P_SIB_SDMMC_NAC_ADDRESS(n)                     (0xe0020050 + ((n) * (0x0400)))
#define CY_U3P_SIB_SDMMC_NAC(n)                             (*(uvint32_t *)(0xe0020050 + ((n) * 0x0400)))
#define CY_U3P_SIB_SDMMC_NAC_DEFAULT                        (0x000fffff)

/*
   Counter value for number of cycles to wait before generating timeout when
   READ data is not received
 */
#define CY_U3P_SIB_RDTMOUT_MASK                             (0xffffffff) /* <0:31> R:RW:0xFFFFF:No */
#define CY_U3P_SIB_RDTMOUT_POS                              (0)



/*
   Block Identification and Version Number
 */
#define CY_U3P_SIB_ID_ADDRESS                               (0xe0027f00)
#define CY_U3P_SIB_ID                                       (*(uvint32_t *)(0xe0027f00))
#define CY_U3P_SIB_ID_DEFAULT                               (0x00010002)

/*
   A unique number identifying the IP in the memory space.
 */
#define CY_U3P_SIB_BLOCK_ID_MASK                            (0x0000ffff) /* <0:15> R:R:0x0002:N/A */
#define CY_U3P_SIB_BLOCK_ID_POS                             (0)


/*
   Version number for the IP.
 */
#define CY_U3P_SIB_BLOCK_VERSION_MASK                       (0xffff0000) /* <16:31> R:R:0x0001:N/A */
#define CY_U3P_SIB_BLOCK_VERSION_POS                        (16)



/*
   Power, clock and reset control
 */
#define CY_U3P_SIB_POWER_ADDRESS                            (0xe0027f04)
#define CY_U3P_SIB_POWER                                    (*(uvint32_t *)(0xe0027f04))
#define CY_U3P_SIB_POWER_DEFAULT                            (0x00000000)

/*
   For blocks that must perform initialization after reset before becoming
   operational, this signal will remain de-asserted until initialization
   is complete.  In other words reading active=1 indicates block is initialized
   and ready for operation.
 */
#define CY_U3P_SIB_ACTIVE                                   (1u << 0) /* <0:0> W:R:0:Yes */


/*
   Active LOW reset signal for all logic in the block.  Note that reset is
   active on all flops in the block when either system reset is asserted
   (RESET# pin or SYSTEM_POWER.RESETN is asserted) or this signal is active.
   After setting this bit to 1, firmware shall poll and wait for the ‘active’
   bit to assert.  Reading ‘1’ from ‘resetn’ does not indicate the block
   is out of reset – this may take some time depending on initialization
   tasks and clock frequencies.
 */
#define CY_U3P_SIB_RESETN                                   (1u << 31) /* <31:31> R:RW:0:No */



/*
   Descriptor Chain Pointer
 */
#define CY_U3P_SIB_SCK_DSCR_ADDRESS(n)                      (0xe0028000 + ((n) * (0x0080)))
#define CY_U3P_SIB_SCK_DSCR(n)                              (*(uvint32_t *)(0xe0028000 + ((n) * 0x0080)))
#define CY_U3P_SIB_SCK_DSCR_DEFAULT                         (0x00000000)

/*
   Descriptor number of currently active descriptor.  A value of 0xFFFF designates
   no (more) active descriptors available.  When activating a socket CPU
   shall write number of first descriptor in here. Only modify this field
   when go_suspend=1 or go_enable=0
 */
#define CY_U3P_SIB_DSCR_NUMBER_MASK                         (0x0000ffff) /* <0:15> RW:RW:X:N/A */
#define CY_U3P_SIB_DSCR_NUMBER_POS                          (0)


/*
   Number of descriptors still left to process.  This value is unrelated
   to actual number of descriptors in the list.  It is used only to generate
   an interrupt to the CPU when the value goes low or zero (or both).  When
   this value reaches 0 it will wrap around to 255.  The socket will not
   suspend or be otherwise affected unless the descriptor chains ends with
   0xFFFF descriptor number.
 */
#define CY_U3P_SIB_DSCR_COUNT_MASK                          (0x00ff0000) /* <16:23> RW:RW:X:N/A */
#define CY_U3P_SIB_DSCR_COUNT_POS                           (16)


/*
   The low watermark for dscr_count.  When dscr_count is equal or less than
   dscr_low the status bit dscr_is_low is set and an interrupt can be generated
   (depending on int mask).
 */
#define CY_U3P_SIB_DSCR_LOW_MASK                            (0xff000000) /* <24:31> R:RW:X:N/A */
#define CY_U3P_SIB_DSCR_LOW_POS                             (24)



/*
   Transfer Size Register
 */
#define CY_U3P_SIB_SCK_SIZE_ADDRESS(n)                      (0xe0028004 + ((n) * (0x0080)))
#define CY_U3P_SIB_SCK_SIZE(n)                              (*(uvint32_t *)(0xe0028004 + ((n) * 0x0080)))
#define CY_U3P_SIB_SCK_SIZE_DEFAULT                         (0x00000000)

/*
   The number of bytes or buffers (depends on unit bit in SCK_STATUS) that
   are part of this transfer.  A value of 0 signals an infinite/undetermined
   transaction size.
   Valid data bytes remaining in the last buffer beyond the transfer size
   will be read by socket and passed on to the core. FW must ensure that
   no additional bytes beyond the transfer size are present in the last buffer.
 */
#define CY_U3P_SIB_TRANS_SIZE_MASK                          (0xffffffff) /* <0:31> R:RW:X:N/A */
#define CY_U3P_SIB_TRANS_SIZE_POS                           (0)



/*
   Transfer Count Register
 */
#define CY_U3P_SIB_SCK_COUNT_ADDRESS(n)                     (0xe0028008 + ((n) * (0x0080)))
#define CY_U3P_SIB_SCK_COUNT(n)                             (*(uvint32_t *)(0xe0028008 + ((n) * 0x0080)))
#define CY_U3P_SIB_SCK_COUNT_DEFAULT                        (0x00000000)

/*
   The number of bytes or buffers (depends on unit bit in SCK_STATUS) that
   have been transferred through this socket so far.  If trans_size is >0
   and trans_count>=trans_size the  ‘trans_done’ bits in SCK_STATUS is both
   set.  If SCK_STATUS.susp_trans=1 the socket is also suspended and the
   ‘suspend’ bit set. This count is updated only when a descriptor is completed
   and the socket proceeds to the next one.
   Exception: When socket suspends with PARTIAL_BUF=1, this value has been
   (incorrectly) incremented by 1 (UNIT=1) or DSCR_SIZE.BYTE_COUNT (UNIT=0).
    Firmware must correct this before resuming the socket.
 */
#define CY_U3P_SIB_TRANS_COUNT_MASK                         (0xffffffff) /* <0:31> RW:RW:X:N/A */
#define CY_U3P_SIB_TRANS_COUNT_POS                          (0)



/*
   Socket Status Register
 */
#define CY_U3P_SIB_SCK_STATUS_ADDRESS(n)                    (0xe002800c + ((n) * (0x0080)))
#define CY_U3P_SIB_SCK_STATUS(n)                            (*(uvint32_t *)(0xe002800c + ((n) * 0x0080)))
#define CY_U3P_SIB_SCK_STATUS_DEFAULT                       (0x04e00000)

/*
   Number of available (free for ingress, occupied for egress) descriptors
   beyond the current one.  This number is incremented by the adapter whenever
   an event is received on this socket and decremented whenever it activates
   a new descriptor. This value is used to create a signal to the IP Cores
   that indicates at least one buffer is available beyond the current one
   (sck_more_buf_avl).
 */
#define CY_U3P_SIB_AVL_COUNT_MASK                           (0x0000001f) /* <0:4> RW:RW:0:N/A */
#define CY_U3P_SIB_AVL_COUNT_POS                            (0)


/*
   Minimum number of available buffers required by the adapter before activating
   a new one.  This can be used to guarantee a minimum number of buffers
   available with old data to implement rollback.  If AVL_ENABLE, the socket
   will remain in STALL state until AVL_COUNT>=AVL_MIN.
 */
#define CY_U3P_SIB_AVL_MIN_MASK                             (0x000003e0) /* <5:9> R:RW:0:N/A */
#define CY_U3P_SIB_AVL_MIN_POS                              (5)


/*
   Enables the functioning of AVL_COUNT and AVL_MIN. When 0, it will disable
   both stalling on AVL_MIN and generation of the sck_more_buf_avl signal
   described above.
 */
#define CY_U3P_SIB_AVL_ENABLE                               (1u << 10) /* <10:10> R:RW:0:N/A */


/*
   Internal operating state of the socket.  This field is used for debugging
   and to safely modify active sockets (see go_suspend).
 */
#define CY_U3P_SIB_STATE_MASK                               (0x00038000) /* <15:17> RW:R:0:N/A */
#define CY_U3P_SIB_STATE_POS                                (15)

/*
   Descriptor state. This is the default initial state indicating the descriptor
   registers are NOT valid in the Adapter. The Adapter will start loading
   the descriptor from memory if the socket becomes enabled and not suspended.
   Suspend has no effect on any other state.
 */
#define CY_U3P_SIB_STATE_DESCR                              (0)
/*
   Stall state. Socket is stalled waiting for data to be loaded into the
   Fetch Queue or waiting for an event.
 */
#define CY_U3P_SIB_STATE_STALL                              (1)
/*
   Active state. Socket is available for core data transfers.
 */
#define CY_U3P_SIB_STATE_ACTIVE                             (2)
/*
   Event state. Core transfer is done. Descriptor is being written back to
   memory and an event is being generated if enabled.
 */
#define CY_U3P_SIB_STATE_EVENT                              (3)
/*
   Check states. An active socket gets here based on the core’s EOP request
   to check the Transfer size and determine whether the buffer should be
   wrapped up. Depending on result, socket will either go back to Active
   state or move to the Event state.
 */
#define CY_U3P_SIB_STATE_CHECK1                             (4)
/*
   Socket is suspended
 */
#define CY_U3P_SIB_STATE_SUSPENDED                          (5)
/*
   Check states. An active socket gets here based on the core’s EOP request
   to check the Transfer size and determine whether the buffer should be
   wrapped up. Depending on result, socket will either go back to Active
   state or move to the Event state.
 */
#define CY_U3P_SIB_STATE_CHECK2                             (6)
/*
   Waiting for confirmation that event was sent.
 */
#define CY_U3P_SIB_STATE_WAITING                            (7)

/*
   Indicates the socket received a ZLP
 */
#define CY_U3P_SIB_ZLP_RCVD                                 (1u << 18) /* <18:18> RW:R:0:N/A */


/*
   Indicates the socket is currently in suspend state.  In suspend mode there
   is no active descriptor; any previously active descriptor has been wrapped
   up, copied back to memory and SCK_DSCR.dscr_number has been updated using
   DSCR_CHAIN as needed.  If the next descriptor is known (SCK_DSCR.dscr_number!=0xFFFF),
   this descriptor will be loaded after the socket resumes from suspend state.
   A socket can only be resumed by changing go_suspend from 1 to 0.  If the
   socket is suspended while go_suspend=0, it must first be set and then
   again cleared.
 */
#define CY_U3P_SIB_SUSPENDED                                (1u << 19) /* <19:19> RW:R:0:N/A */


/*
   Indicates the socket is currently enabled when asserted.  After go_enable
   is changed, it may take some time for enabled to make the same change.
    This value can be polled to determine this fact.
 */
#define CY_U3P_SIB_ENABLED                                  (1u << 20) /* <20:20> RW:R:0:N/A */


/*
   Enable (1) or disable (0) truncating of BYTE_COUNT when TRANS_COUNT+BYTE_COUNT>=TRANS_SIZE.
    When enabled, ensures that an ingress transfer never contains more bytes
   then allowed.  This function is needed to implement burst-based prototocols
   that can only transmit full bursts of more than 1 byte.
 */
#define CY_U3P_SIB_TRUNCATE                                 (1u << 21) /* <21:21> R:RW:1:N/A */


/*
   Enable (1) or disable (0) sending of produce events from any descriptor
   in this socket.  If 0, events will be suppressed, and the descriptor will
   not be copied back into memory when completed.
 */
#define CY_U3P_SIB_EN_PROD_EVENTS                           (1u << 22) /* <22:22> R:RW:1:N/A */


/*
   Enable (1) or disable (0) sending of consume events from any descriptor
   in this socket.  If 0, events will be suppressed, and the descriptor will
   not be copied back into memory when completed.
 */
#define CY_U3P_SIB_EN_CONS_EVENTS                           (1u << 23) /* <23:23> R:RW:1:N/A */


/*
   When set, the socket will suspend before activating a descriptor with
   BYTE_COUNT<BUFFER_SIZE.
   This is relevant for egress sockets only.
 */
#define CY_U3P_SIB_SUSP_PARTIAL                             (1u << 24) /* <24:24> R:RW:0:N/A */


/*
   When set, the socket will suspend before activating a descriptor with
   TRANS_COUNT+BUFFER_SIZE>=TRANS_SIZE.  This is relevant for both ingress
   and egress sockets.
 */
#define CY_U3P_SIB_SUSP_LAST                                (1u << 25) /* <25:25> R:RW:0:N/A */


/*
   When set, the socket will suspend when trans_count >= trans_size.  This
   equation is checked (and hence the socket will suspend) only at the boundary
   of buffers and packets (ie. buffer wrapup or EOP assertion).
 */
#define CY_U3P_SIB_SUSP_TRANS                               (1u << 26) /* <26:26> R:RW:1:N/A */


/*
   When set, the socket will suspend after wrapping up the first buffer with
   dscr.eop=1.  Note that this function will work the same for both ingress
   and egress sockets.
 */
#define CY_U3P_SIB_SUSP_EOP                                 (1u << 27) /* <27:27> R:RW:0:N/A */


/*
   Setting this bit will forcibly wrap-up a socket, whether it is out of
   data or not.  This option is intended mainly for ingress sockets, but
   works also for egress sockets.  Any remaining data in fetch buffers is
   ignored, in write buffers is flushed.  Transaction and buffer counts are
   updated normally, and suspend behavior also happens normally (depending
   on various other settings in this register).G45
 */
#define CY_U3P_SIB_WRAPUP                                   (1u << 28) /* <28:28> RW0C:RW1S:0:N/A */


/*
   Indicates whether descriptors (1) or bytes (0) are counted by trans_count
   and trans_size.  Descriptors are counting regardless of whether they contain
   any data or have eop set.
 */
#define CY_U3P_SIB_UNIT                                     (1u << 29) /* <29:29> R:RW:0:N/A */


/*
   Directs a socket to go into suspend mode when the current descriptor completes.
    The main use of this bit is to safely append descriptors to an active
   socket without actually suspending it (in most cases). The process is
   outlined in more detail in the architecture spec, and looks as follows:
   1: GO_SUSPEND=1
   2: modify the chain in memory
   3: check if active descriptor is 0xFFFF or last in chain
   4: if so, make corrections as neccessary (complicated)
   5: clear any pending suspend interrupts (SCK_INTR[9:5])
   6: GO_SUSPEND=0
   Note that the socket resumes only when SCK_INTR[9:5]=0 and GO_SUSPEND=0.
 */
#define CY_U3P_SIB_GO_SUSPEND                               (1u << 30) /* <30:30> R:RW:0:N/A */


/*
   Indicates whether socket is enabled.  When go_enable is cleared while
   socket is active, ongoing transfers are aborted after an unspecified amount
   of time.  No update occurs from the descriptor registers back into memory.
    When go_enable is changed from 0 to 1, the socket will reload the active
   descriptor from memory regardless of the contents of DSCR_ registers.
   The socket will not wait for an EVENT to become active if the descriptor
   is available and ready for transfer (has space or data).
   The 'enabled' bit indicates whether the socket is actually enabled or
   not.  This field lags go_enable by an short but unspecificied of time.
 */
#define CY_U3P_SIB_GO_ENABLE                                (1u << 31) /* <31:31> R:RW:0:N/A */



/*
   Socket Interrupt Request Register
 */
#define CY_U3P_SIB_SCK_INTR_ADDRESS(n)                      (0xe0028010 + ((n) * (0x0080)))
#define CY_U3P_SIB_SCK_INTR(n)                              (*(uvint32_t *)(0xe0028010 + ((n) * 0x0080)))
#define CY_U3P_SIB_SCK_INTR_DEFAULT                         (0x00000000)

/*
   Indicates that a produce event has been received or transmitted since
   last cleared.
 */
#define CY_U3P_SIB_PRODUCE_EVENT                            (1u << 0) /* <0:0> W1S:RW1C:0:N/A */


/*
   Indicates that a consume event has been received or transmitted since
   last cleared.
 */
#define CY_U3P_SIB_CONSUME_EVENT                            (1u << 1) /* <1:1> W1S:RW1C:0:N/A */


/*
   Indicates that dscr_count has fallen below its watermark dscr_low.  If
   dscr_count wraps around to 255 dscr_is_low will remain asserted until
   cleared by s/w
 */
#define CY_U3P_SIB_DSCR_IS_LOW                              (1u << 2) /* <2:2> W1S:RW1C:0:N/A */


/*
   Indicates the no descriptor is available.  Not available means that the
   current descriptor number is 0xFFFF.  Note that this bit will remain asserted
   until cleared by s/w, regardless of whether a new descriptor number is
   loaded.
 */
#define CY_U3P_SIB_DSCR_NOT_AVL                             (1u << 3) /* <3:3> W1S:RW1C:0:N/A */


/*
   Indicates the socket has stalled, waiting for an event signaling its descriptor
   has become available. Note that this bit will remain asserted until cleared
   by s/w, regardless of whether the socket resumes.
 */
#define CY_U3P_SIB_STALL                                    (1u << 4) /* <4:4> W1S:RW1C:0:N/A */


/*
   Indicates the socket has gone into suspend mode.  This may be caused by
   any hardware initiated condition (e.g. DSCR_NOT_AVL, any of the SUSP_*)
   or by setting GO_SUSPEND=1.  Note that this bit will remain asserted until
   cleared by s/w, regardless of whether the suspend condition is resolved.
   Note that the socket resumes only when SCK_INTR[9:5]=0 and GO_SUSPEND=0.
 */
#define CY_U3P_SIB_SUSPEND                                  (1u << 5) /* <5:5> W1S:RW1C:0:N/A */


/*
   Indicates the socket is suspended because of an error condition (internal
   to the adapter) – if error=1 then suspend=1 as well.  Possible error causes
   are:
   - dscr_size.buffer_error bit already set in the descriptor.
   - dscr_size.byte_count > dscr_size.buffer_size
   - core writes into an inactive socket.
   - core did not consume all the data in the buffer.
   Note that the socket resumes only when SCK_INTR[9:5]=0 and GO_SUSPEND=0.
 */
#define CY_U3P_SIB_ERROR                                    (1u << 6) /* <6:6> W1S:RW1C:0:N/A */


/*
   Indicates that TRANS_COUNT has reached the limit TRANS_SIZE.  This flag
   is only set when SUSP_TRANS=1.  Note that because TRANS_COUNT is updated
   only at the granularity of entire buffers, it is possible that TRANS_COUNT
   exceeds TRANS_SIZE before the socket suspends.  Software must detect and
   deal with these situations.  When asserting EOP to the adapter on ingress,
   the trans_count is not updated unless the socket actually suspends (see
   SUSP_TRANS).
   Note that the socket resumes only when SCK_INTR[9:5]=0 and GO_SUSPEND=0.
 */
#define CY_U3P_SIB_TRANS_DONE                               (1u << 7) /* <7:7> W1S:RW1C:0:N/A */


/*
   Indicates that the (egress) socket was suspended because of SUSP_PARTIAL
   condition.  Note that the socket resumes only when SCK_INTR[9:5]=0 and
   GO_SUSPEND=0.
 */
#define CY_U3P_SIB_PARTIAL_BUF                              (1u << 8) /* <8:8> W1S:RW1C:0:N/A */


/*
   Indicates that the socket was suspended because of SUSP_LAST condition.
    Note that the socket resumes only when SCK_INTR[9:5]=0 and GO_SUSPEND=0.
 */
#define CY_U3P_SIB_LAST_BUF                                 (1u << 9) /* <9:9> W1S:RW1C:0:N/A */



/*
   Socket Interrupt Mask Register
 */
#define CY_U3P_SIB_SCK_INTR_MASK_ADDRESS(n)                 (0xe0028014 + ((n) * (0x0080)))
#define CY_U3P_SIB_SCK_INTR_MASK(n)                         (*(uvint32_t *)(0xe0028014 + ((n) * 0x0080)))
#define CY_U3P_SIB_SCK_INTR_MASK_DEFAULT                    (0x00000000)

/*
   1: Report interrupt to CPU
 */
#define CY_U3P_SIB_PRODUCE_EVENT                            (1u << 0) /* <0:0> R:RW:0:N/A */


/*
   1: Report interrupt to CPU
 */
#define CY_U3P_SIB_CONSUME_EVENT                            (1u << 1) /* <1:1> R:RW:0:N/A */


/*
   1: Report interrupt to CPU
 */
#define CY_U3P_SIB_DSCR_IS_LOW                              (1u << 2) /* <2:2> R:RW:0:N/A */


/*
   1: Report interrupt to CPU
 */
#define CY_U3P_SIB_DSCR_NOT_AVL                             (1u << 3) /* <3:3> R:RW:0:N/A */


/*
   1: Report interrupt to CPU
 */
#define CY_U3P_SIB_STALL                                    (1u << 4) /* <4:4> R:RW:0:N/A */


/*
   1: Report interrupt to CPU
 */
#define CY_U3P_SIB_SUSPEND                                  (1u << 5) /* <5:5> R:RW:0:N/A */


/*
   1: Report interrupt to CPU
 */
#define CY_U3P_SIB_ERROR                                    (1u << 6) /* <6:6> R:RW:0:N/A */


/*
   1: Report interrupt to CPU
 */
#define CY_U3P_SIB_TRANS_DONE                               (1u << 7) /* <7:7> R:RW:0:N/A */


/*
   1: Report interrupt to CPU
 */
#define CY_U3P_SIB_PARTIAL_BUF                              (1u << 8) /* <8:8> R:RW:0:N/A */


/*
   1: Report interrupt to CPU
 */
#define CY_U3P_SIB_LAST_BUF                                 (1u << 9) /* <9:9> R:RW:0:N/A */



/*
   Descriptor buffer base address register
 */
#define CY_U3P_SIB_DSCR_BUFFER_ADDRESS(n)                   (0xe0028020 + ((n) * (0x0080)))
#define CY_U3P_SIB_DSCR_BUFFER(n)                           (*(uvint32_t *)(0xe0028020 + ((n) * 0x0080)))
#define CY_U3P_SIB_DSCR_BUFFER_DEFAULT                      (0x00000000)

/*
   The base address of the buffer where data is written.  This address is
   not necessarily word-aligned to allow for header/trailer/length modification.
 */
#define CY_U3P_SIB_BUFFER_ADDR_MASK                         (0xffffffff) /* <0:31> RW:RW:X:N/A */
#define CY_U3P_SIB_BUFFER_ADDR_POS                          (0)



/*
   Descriptor synchronization pointers register
 */
#define CY_U3P_SIB_DSCR_SYNC_ADDRESS(n)                     (0xe0028024 + ((n) * (0x0080)))
#define CY_U3P_SIB_DSCR_SYNC(n)                             (*(uvint32_t *)(0xe0028024 + ((n) * 0x0080)))
#define CY_U3P_SIB_DSCR_SYNC_DEFAULT                        (0x00000000)

/*
   The socket number of the consuming socket to which the produce event shall
   be sent.
   If cons_ip and cons_sck matches the socket's IP and socket number then
   the matching socket becomes consuming socket.
 */
#define CY_U3P_SIB_CONS_SCK_MASK                            (0x000000ff) /* <0:7> RW:RW:X:N/A */
#define CY_U3P_SIB_CONS_SCK_POS                             (0)


/*
   The IP number of the consuming socket to which the produce event shall
   be sent.  Use 0x3F to designate ARM CPU (so software) as consumer; the
   event will be lost in this case and an interrupt should also be generated
   to observe this condition.
 */
#define CY_U3P_SIB_CONS_IP_MASK                             (0x00003f00) /* <8:13> RW:RW:X:N/A */
#define CY_U3P_SIB_CONS_IP_POS                              (8)


/*
   Enable sending of a consume events from this descriptor only.  Events
   are sent only if SCK_STATUS.en_consume_ev=1.  When events are disabled,
   the adapter also does not update the descriptor in memory to clear its
   occupied bit.
 */
#define CY_U3P_SIB_EN_CONS_EVENT                            (1u << 14) /* <14:14> RW:RW:X:N/A */


/*
   Enable generation of a consume event interrupt for this descriptor only.
    This interrupt will only be seen by the CPU if SCK_STATUS.int_mask has
   this interrupt enabled as well.  Note that this flag influences the logging
   of the interrupt in SCK_STATUS; it has no effect on the reporting of the
   interrupt to the CPU like SCK_STATUS.int_mask does.
 */
#define CY_U3P_SIB_EN_CONS_INT                              (1u << 15) /* <15:15> RW:RW:X:N/A */


/*
   The socket number of the producing socket to which the consume event shall
   be sent. If prod_ip and prod_sck matches the socket's IP and socket number
   then the matching socket becomes consuming socket.
 */
#define CY_U3P_SIB_PROD_SCK_MASK                            (0x00ff0000) /* <16:23> RW:RW:X:N/A */
#define CY_U3P_SIB_PROD_SCK_POS                             (16)


/*
   The IP number of the producing socket to which the consume event shall
   be sent. Use 0x3F to designate ARM CPU (so software) as producer; the
   event will be lost in this case and an interrupt should also be generated
   to observe this condition.
 */
#define CY_U3P_SIB_PROD_IP_MASK                             (0x3f000000) /* <24:29> RW:RW:X:N/A */
#define CY_U3P_SIB_PROD_IP_POS                              (24)


/*
   Enable sending of a produce events from this descriptor only.  Events
   are sent only if SCK_STATUS.en_produce_ev=1.  If 0, events will be suppressed,
   and the descriptor will not be copied back into memory when completed.
 */
#define CY_U3P_SIB_EN_PROD_EVENT                            (1u << 30) /* <30:30> RW:RW:X:N/A */


/*
   Enable generation of a produce event interrupt for this descriptor only.
   This interrupt will only be seen by the CPU if SCK_STATUS. int_mask has
   this interrupt enabled as well.  Note that this flag influences the logging
   of the interrupt in SCK_STATUS; it has no effect on the reporting of the
   interrupt to the CPU like SCK_STATUS.int_mask does.
 */
#define CY_U3P_SIB_EN_PROD_INT                              (1u << 31) /* <31:31> RW:RW:X:N/A */



/*
   Descriptor Chain Pointers Register
 */
#define CY_U3P_SIB_DSCR_CHAIN_ADDRESS(n)                    (0xe0028028 + ((n) * (0x0080)))
#define CY_U3P_SIB_DSCR_CHAIN(n)                            (*(uvint32_t *)(0xe0028028 + ((n) * 0x0080)))
#define CY_U3P_SIB_DSCR_CHAIN_DEFAULT                       (0x00000000)

/*
   Descriptor number of the next task for consumer. A value of 0xFFFF signals
   end of this list.
 */
#define CY_U3P_SIB_RD_NEXT_DSCR_MASK                        (0x0000ffff) /* <0:15> RW:RW:X:N/A */
#define CY_U3P_SIB_RD_NEXT_DSCR_POS                         (0)


/*
   Descriptor number of the next task for producer. A value of 0xFFFF signals
   end of this list.
 */
#define CY_U3P_SIB_WR_NEXT_DSCR_MASK                        (0xffff0000) /* <16:31> RW:RW:X:N/A */
#define CY_U3P_SIB_WR_NEXT_DSCR_POS                         (16)



/*
   Descriptor Size Register
 */
#define CY_U3P_SIB_DSCR_SIZE_ADDRESS(n)                     (0xe002802c + ((n) * (0x0080)))
#define CY_U3P_SIB_DSCR_SIZE(n)                             (*(uvint32_t *)(0xe002802c + ((n) * 0x0080)))
#define CY_U3P_SIB_DSCR_SIZE_DEFAULT                        (0x00000000)

/*
   A marker that is provided by s/w and can be observed by the IP.  It's
   meaning is defined by the IP that uses it.  This bit has no effect on
   the other DMA mechanisms.
 */
#define CY_U3P_SIB_MARKER                                   (1u << 0) /* <0:0> RW:RW:X:N/A */


/*
   A marker indicating this descriptor refers to the last buffer of a packet
   or transfer. Packets/transfers may span more than one buffer.  The producing
   IP provides this marker by providing the EOP signal to its DMA adapter.
    The consuming IP observes this marker by inspecting its EOP return signal
   from its own DMA adapter. For more information see section on packets,
   buffers and transfers in DMA chapter.
 */
#define CY_U3P_SIB_EOP                                      (1u << 1) /* <1:1> RW:RW:X:N/A */


/*
   Indicates the buffer data is valid (0) or in error (1).
 */
#define CY_U3P_SIB_BUFFER_ERROR                             (1u << 2) /* <2:2> RW:RW:X:N/A */


/*
   Indicates the buffer is in use (1) or empty (0).  A consumer will interpret
   this as:
   0: Buffer is empty, wait until filled.
   1: Buffer has data that can be consumed
   A produce will interpret this as:
   0: Buffer is ready to be filled
   1: Buffer is occupied, wait until empty
 */
#define CY_U3P_SIB_BUFFER_OCCUPIED                          (1u << 3) /* <3:3> RW:RW:X:N/A */


/*
   The size of the buffer in multiples of 16 bytes
 */
#define CY_U3P_SIB_BUFFER_SIZE_MASK                         (0x0000fff0) /* <4:15> RW:RW:X:N/A */
#define CY_U3P_SIB_BUFFER_SIZE_POS                          (4)


/*
   The number of data bytes present in the buffer.  An occupied buffer is
   not always full, in particular when variable length packets are transferred.
 */
#define CY_U3P_SIB_BYTE_COUNT_MASK                          (0xffff0000) /* <16:31> RW:RW:X:N/A */
#define CY_U3P_SIB_BYTE_COUNT_POS                           (16)



/*
   Event Communication Register
 */
#define CY_U3P_SIB_EVENT_ADDRESS(n)                         (0xe002807c + ((n) * (0x0080)))
#define CY_U3P_SIB_EVENT(n)                                 (*(uvint32_t *)(0xe002807c + ((n) * 0x0080)))
#define CY_U3P_SIB_EVENT_DEFAULT                            (0x00000000)

/*
   The active descriptor number for which the event is sent.
 */
#define CY_U3P_EVENT_ACTIVE_DSCR_MASK                       (0x0000ffff) /* <0:15> RW:W:0:N/A */
#define CY_U3P_EVENT_ACTIVE_DSCR_POS                        (0)


/*
   Type of event
   0: Consume event descriptor is marked empty - BUFFER_OCCUPIED=0)
   1: Produce event descriptor is marked full = BUFFER_OCCUPIED=1)
 */
#define CY_U3P_EVENT_EVENT_TYPE                             (1u << 16) /* <16:16> RW:W:0:N/A */



/*
   Socket Interrupt Request Register
 */
#define CY_U3P_SIB_SCK_INTR0_ADDRESS                        (0xe002ff00)
#define CY_U3P_SIB_SCK_INTR0                                (*(uvint32_t *)(0xe002ff00))
#define CY_U3P_SIB_SCK_INTR0_DEFAULT                        (0x00000000)

/*
   Socket <x> asserts interrupt when bit <x> is set in this vector.  Multiple
   bits may be set to 1 simultaneously.
   This register is only as wide as the number of socket in the adapter;
   256 is just the maximum width.  All other bits always return 0.
 */
#define CY_U3P_SIB_SCKINTR_L_MASK                           (0xffffffff) /* <0:31> W:R:0:N/A */
#define CY_U3P_SIB_SCKINTR_L_POS                            (0)



/*
   Socket Interrupt Request Register
 */
#define CY_U3P_SIB_SCK_INTR1_ADDRESS                        (0xe002ff04)
#define CY_U3P_SIB_SCK_INTR1                                (*(uvint32_t *)(0xe002ff04))
#define CY_U3P_SIB_SCK_INTR1_DEFAULT                        (0x00000000)

/*
   Socket <x> asserts interrupt when bit <x> is set in this vector.  Multiple
   bits may be set to 1 simultaneously.
   This register is only as wide as the number of socket in the adapter;
   256 is just the maximum width.  All other bits always return 0.
 */
#define CY_U3P_SIB_SCKINTR_H_L_MASK                         (0xffffffff) /* <0:31> W:R:0:N/A */
#define CY_U3P_SIB_SCKINTR_H_L_POS                          (0)



/*
   Socket Interrupt Request Register
 */
#define CY_U3P_SIB_SCK_INTR2_ADDRESS                        (0xe002ff08)
#define CY_U3P_SIB_SCK_INTR2                                (*(uvint32_t *)(0xe002ff08))
#define CY_U3P_SIB_SCK_INTR2_DEFAULT                        (0x00000000)

/*
   Socket <x> asserts interrupt when bit <x> is set in this vector.  Multiple
   bits may be set to 1 simultaneously.
   This register is only as wide as the number of socket in the adapter;
   256 is just the maximum width.  All other bits always return 0.
 */
#define CY_U3P_SIB_SCKINTR_H_L_MASK                         (0xffffffff) /* <0:31> W:R:0:N/A */
#define CY_U3P_SIB_SCKINTR_H_L_POS                          (0)



/*
   Socket Interrupt Request Register
 */
#define CY_U3P_SIB_SCK_INTR3_ADDRESS                        (0xe002ff0c)
#define CY_U3P_SIB_SCK_INTR3                                (*(uvint32_t *)(0xe002ff0c))
#define CY_U3P_SIB_SCK_INTR3_DEFAULT                        (0x00000000)

/*
   Socket <x> asserts interrupt when bit <x> is set in this vector.  Multiple
   bits may be set to 1 simultaneously.
   This register is only as wide as the number of socket in the adapter;
   256 is just the maximum width.  All other bits always return 0.
 */
#define CY_U3P_SIB_SCKINTR_H_L_MASK                         (0xffffffff) /* <0:31> W:R:0:N/A */
#define CY_U3P_SIB_SCKINTR_H_L_POS                          (0)



/*
   Socket Interrupt Request Register
 */
#define CY_U3P_SIB_SCK_INTR4_ADDRESS                        (0xe002ff10)
#define CY_U3P_SIB_SCK_INTR4                                (*(uvint32_t *)(0xe002ff10))
#define CY_U3P_SIB_SCK_INTR4_DEFAULT                        (0x00000000)

/*
   Socket <x> asserts interrupt when bit <x> is set in this vector.  Multiple
   bits may be set to 1 simultaneously.
   This register is only as wide as the number of socket in the adapter;
   256 is just the maximum width.  All other bits always return 0.
 */
#define CY_U3P_SIB_SCKINTR_H_L_MASK                         (0xffffffff) /* <0:31> W:R:0:N/A */
#define CY_U3P_SIB_SCKINTR_H_L_POS                          (0)



/*
   Socket Interrupt Request Register
 */
#define CY_U3P_SIB_SCK_INTR5_ADDRESS                        (0xe002ff14)
#define CY_U3P_SIB_SCK_INTR5                                (*(uvint32_t *)(0xe002ff14))
#define CY_U3P_SIB_SCK_INTR5_DEFAULT                        (0x00000000)

/*
   Socket <x> asserts interrupt when bit <x> is set in this vector.  Multiple
   bits may be set to 1 simultaneously.
   This register is only as wide as the number of socket in the adapter;
   256 is just the maximum width.  All other bits always return 0.
 */
#define CY_U3P_SIB_SCKINTR_H_L_MASK                         (0xffffffff) /* <0:31> W:R:0:N/A */
#define CY_U3P_SIB_SCKINTR_H_L_POS                          (0)



/*
   Socket Interrupt Request Register
 */
#define CY_U3P_SIB_SCK_INTR6_ADDRESS                        (0xe002ff18)
#define CY_U3P_SIB_SCK_INTR6                                (*(uvint32_t *)(0xe002ff18))
#define CY_U3P_SIB_SCK_INTR6_DEFAULT                        (0x00000000)

/*
   Socket <x> asserts interrupt when bit <x> is set in this vector.  Multiple
   bits may be set to 1 simultaneously.
   This register is only as wide as the number of socket in the adapter;
   256 is just the maximum width.  All other bits always return 0.
 */
#define CY_U3P_SIB_SCKINTR_H_L_MASK                         (0xffffffff) /* <0:31> W:R:0:N/A */
#define CY_U3P_SIB_SCKINTR_H_L_POS                          (0)



/*
   Socket Interrupt Request Register
 */
#define CY_U3P_SIB_SCK_INTR7_ADDRESS                        (0xe002ff1c)
#define CY_U3P_SIB_SCK_INTR7                                (*(uvint32_t *)(0xe002ff1c))
#define CY_U3P_SIB_SCK_INTR7_DEFAULT                        (0x00000000)

/*
   Socket <x> asserts interrupt when bit <x> is set in this vector.  Multiple
   bits may be set to 1 simultaneously.
   This register is only as wide as the number of socket in the adapter;
   256 is just the maximum width.  All other bits always return 0.
 */
#define CY_U3P_SIB_SCKINTR_H_MASK                           (0xffffffff) /* <0:31> W:R:0:N/A */
#define CY_U3P_SIB_SCKINTR_H_POS                            (0)



#endif /* _INCLUDED_SIB_REGS_H_ */

/*[]*/

