/****************************************************************************
 *
 * File: pmmc_regs.h
 * Version: Hans-RegMap_v159.xls
 *
 * Copyright (c) 2009-10 Cypress Semiconductor. All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION WHICH IS THE PROPERTY OF CYPRESS.
 *
 * Description:
 *   Cypress West Bridge Benicia header file.
 *
 *   This file is auto generated from WestBridge Benicia spreadsheet.
 *   DO NOT MODIFY THIS FILE
 *
 * Use of this file is governed by the license agreement included in the file
 *
 *   <install>/license/license.txt
 *
 * where <install> is the Cypress software installation root directory path.
 *
 ****************************************************************************/

#ifndef _INCLUDED_PMMC_REGS_H_
#define _INCLUDED_PMMC_REGS_H_

#include <cyu3types.h>

#define PMMC_BASE_ADDR                           (0xe0011000)

typedef struct
{
    uvint32_t config;                             /* 0xe0011000 */
    uvint32_t dir_sock;                           /* 0xe0011004 */
    uvint32_t multi_block_pm;                     /* 0xe0011008 */
    uvint32_t intr;                               /* 0xe001100c */
    uvint32_t intr_mask;                          /* 0xe0011010 */
    uvint32_t wbusy_cfg;                          /* 0xe0011014 */
    uvint32_t wbusy_count;                        /* 0xe0011018 */
    uvint32_t cmd0_arg;                           /* 0xe001101c */
    uvint32_t idx;                                /* 0xe0011020 */
    uvint32_t arg;                                /* 0xe0011024 */
    uvint32_t blk_cmd;                            /* 0xe0011028 */
    uvint32_t blk_addr;                           /* 0xe001102c */
    uvint32_t busy;                               /* 0xe0011030 */
    uvint32_t block_count;                        /* 0xe0011034 */
    uvint32_t block_len;                          /* 0xe0011038 */
    uvint32_t sck_direction;                      /* 0xe001103c */
    uvint32_t sck_not_rdy_mask;                   /* 0xe0011040 */
    uvint32_t reg_block_len;                      /* 0xe0011044 */
    uvint32_t cid0;                               /* 0xe0011048 */
    uvint32_t cid1;                               /* 0xe001104c */
    uvint32_t cid2;                               /* 0xe0011050 */
    uvint32_t cid3;                               /* 0xe0011054 */
    uvint32_t rca;                                /* 0xe0011058 */
    uvint32_t dsr;                                /* 0xe001105c */
    uvint32_t csd0;                               /* 0xe0011060 */
    uvint32_t csd1;                               /* 0xe0011064 */
    uvint32_t csd2;                               /* 0xe0011068 */
    uvint32_t csd3;                               /* 0xe001106c */
    uvint32_t ocr;                                /* 0xe0011070 */
    uvint32_t csr;                                /* 0xe0011074 */
    uvint32_t ext_csd0;                           /* 0xe0011078 */
    uvint32_t ext_csd1;                           /* 0xe001107c */
    uvint32_t ext_csd2;                           /* 0xe0011080 */
    uvint32_t ext_csd3;                           /* 0xe0011084 */
    uvint32_t ext_csd4;                           /* 0xe0011088 */
    uvint32_t ext_csd5;                           /* 0xe001108c */
    uvint32_t rsrvd0[4];
    uvint32_t control;                            /* 0xe00110a0 */
    uvint32_t rd_abort_wait;                      /* 0xe00110a4 */
} PMMC_REGS_T, *PPMMC_REGS_T;

#define PMMC        ((PPMMC_REGS_T) PMMC_BASE_ADDR)


/*
   MMC Configuration and Status Register
 */
#define CY_U3P_PIB_PMMC_CONFIG_ADDRESS                      (0xe0011000)
#define CY_U3P_PIB_PMMC_CONFIG                              (*(uvint32_t *)(0xe0011000))
#define CY_U3P_PIB_PMMC_CONFIG_DEFAULT                      (0x00008100)

/*
   Current state of the state machine.  A write to this register will forcibly
   change the state of the state machine.
 */
#define CY_U3P_PIB_PMMC_CURRENT_STATE_MASK                  (0x0000000f) /* <0:3> RW:RW:0:Yes */
#define CY_U3P_PIB_PMMC_CURRENT_STATE_POS                   (0)


/*
   Previous state of the state machine before receiving the current command.
 */
#define CY_U3P_PIB_PMMC_PREVIOUS_STATE_MASK                 (0x000000f0) /* <4:7> RW:R:0:No */
#define CY_U3P_PIB_PMMC_PREVIOUS_STATE_POS                  (4)


/*
   This NoTA version is reflected on PP_IDENTIFICATION.VERSION field
 */
#define CY_U3P_PIB_PMMC_NOTA_VERSION_MASK                   (0x0000ff00) /* <8:15> R:RW:0x81:No */
#define CY_U3P_PIB_PMMC_NOTA_VERSION_POS                    (8)


/*
   MMC Transient Error Code. To be further defined in BROS.
   This field will log the first error that occurrs and will not be overwritten
   until PMMC_INTR.MMC_TRAN_ERR is cleared.
 */
#define CY_U3P_PIB_PMMC_MMC_TRAN_ERR_CODE_MASK              (0x00ff0000) /* <16:23> RW:R:0:No */
#define CY_U3P_PIB_PMMC_MMC_TRAN_ERR_CODE_POS               (16)

/*
   Received address is not defined as per the MMC address map - Refer to
   GCG-150
 */
#define CY_U3P_PIB_PMMC_MMC_TRAN_ERR_CODE_ADDRESS_OUT_OF_RANGE    (1)
/*
   1) Multi block command is used to access registers
   2) Multi-Register region is accessed and received address is not word
   aligned or block length is not word aligned
   3) Single-Register region is accessed and received address is not 512
   byte aligned or block length is not word aligned
 */
#define CY_U3P_PIB_PMMC_MMC_TRAN_ERR_CODE_ADDRESS_MISALIGN    (2)
/*
   1) SET_BLOCKLEN command tries to set a block length more than that specified
   by WRITE_BL_LEN field of PMMC_CSD register
   2) READ block command is received and the last SET_BLOCKLEN command set
   a length more than tha specified by READ_BL_LEN field of PMMC_CSD register
 */
#define CY_U3P_PIB_PMMC_MMC_TRAN_ERR_CODE_BLOCK_LEN_ERROR    (3)
/*
   Received command has CRC error
 */
#define CY_U3P_PIB_PMMC_MMC_TRAN_ERR_CODE_COM_CRC_ERROR     (4)
/*
   Received command is illegal
 */
#define CY_U3P_PIB_PMMC_MMC_TRAN_ERR_CODE_ILLEGAL_COMMAND    (5)

/*
   Selects between PMMC NOTA and block based mode.
   0: P-port MMC in block based mode
   1: P-port MMC in NoTA Mode
   Note that  NoTA and block based modes have a different MMC address memory
   map for sockets as described in architecture spec.
   HW sets this bit when IDENTIFICATION register is written immediately after
   CARD_SELECT for the first time after HW_RESET or power up.
 */
#define CY_U3P_PIB_PMMC_NOTA_MODE                           (1u << 24) /* <24:24> RW:R:0:Yes */



/*
   PMMC direct sociket numbers
 */
#define CY_U3P_PIB_PMMC_DIR_SOCK_ADDRESS                    (0xe0011004)
#define CY_U3P_PIB_PMMC_DIR_SOCK                            (*(uvint32_t *)(0xe0011004))
#define CY_U3P_PIB_PMMC_DIR_SOCK_DEFAULT                    (0x00000000)

/*
   Direct write socket number
 */
#define CY_U3P_PIB_PMMC_WR_SOCK_MASK                        (0x0000001f) /* <0:4> R:RW:0:No */
#define CY_U3P_PIB_PMMC_WR_SOCK_POS                         (0)


/*
   Direct read socket number
 */
#define CY_U3P_PIB_PMMC_RD_SOCK_MASK                        (0x00001f00) /* <8:12> R:RW:0:No */
#define CY_U3P_PIB_PMMC_RD_SOCK_POS                         (8)


/*
   Enable for direct access
   0: Disable direct access
   1: Enable direct access
 */
#define CY_U3P_PIB_PMMC_ACCESS_ENABLE                       (1u << 31) /* <31:31> R:RW:0:No */



/*
   PMMC multi block packet mode configuration register
 */
#define CY_U3P_PIB_PMMC_MULTI_BLOCK_PM_ADDRESS              (0xe0011008)
#define CY_U3P_PIB_PMMC_MULTI_BLOCK_PM                      (*(uvint32_t *)(0xe0011008))
#define CY_U3P_PIB_PMMC_MULTI_BLOCK_PM_DEFAULT              (0x00000000)

/*
   0: Pack one block per buffer in packet mode.
   1: Pack more than one block in a buffer in packet mode.
 */
#define CY_U3P_PIB_PMMC_SOCK_MASK                           (0xffffffff) /* <0:31> R:RW:0:No */
#define CY_U3P_PIB_PMMC_SOCK_POS                            (0)



/*
   PIB PMMC Interrupt Cause Register
 */
#define CY_U3P_PIB_PMMC_INTR_ADDRESS                        (0xe001100c)
#define CY_U3P_PIB_PMMC_INTR                                (*(uvint32_t *)(0xe001100c))
#define CY_U3P_PIB_PMMC_INTR_DEFAULT                        (0x00000000)

/*
   CMD0(GO_IDLE) command received in all states other than BOOT state. In
   BOOT state BOOT_EXIT is provided.
   Firmware aborts any previously on going command on receiving GO_IDLE.
 */
#define CY_U3P_PIB_PMMC_GO_IDLE                             (1u << 0) /* <0:0> W1S:RW1C:0:Yes */


/*
   SEND_OP_COND
   HW execution, Response sent in NID cycles
 */
#define CY_U3P_PIB_PMMC_CMD1                                (1u << 1) /* <1:1> W1S:RW1C:0:Yes */


/*
   SET_DSR
   FW may program IO drive strengths if possible.
 */
#define CY_U3P_PIB_PMMC_CMD4                                (1u << 2) /* <2:2> W1S:RW1C:0:Yes */


/*
   CMD5(SLEEP) command places Benicia in Suspend state.
 */
#define CY_U3P_PIB_PMMC_CMD5_SLEEP                          (1u << 3) /* <3:3> W1S:RW1C:0:Yes */


/*
   CMD5(AWAKE) command gets Benicia out of Suspend state.
 */
#define CY_U3P_PIB_PMMC_CMD5_AWAKE                          (1u << 4) /* <4:4> W1S:RW1C:0:Yes */


/*
   FW sets the EXT_CSD fields to program bus-width or speed.FW to set SWITCH_ERROR
   for other commands.
 */
#define CY_U3P_PIB_PMMC_CMD6                                (1u << 5) /* <5:5> W1S:RW1C:0:Yes */


/*
   STOP_TRANSMISSION
   HW+FW execution, response initiated by HW or FW depending on ‘card-state’.
 */
#define CY_U3P_PIB_PMMC_CMD12                               (1u << 6) /* <6:6> W1S:RW1C:0:Yes */


/*
   GO_INACTIVE_STATE
   FW execution
 */
#define CY_U3P_PIB_PMMC_CMD15                               (1u << 7) /* <7:7> W1S:RW1C:0:Yes */


/*
   SET_BLOCKLEN
   HW execution, FW may use this indication to monitor block length update.
 */
#define CY_U3P_PIB_PMMC_CMD16                               (1u << 8) /* <8:8> W1S:RW1C:0:Yes */


/*
   Write command received on direct write socket
 */
#define CY_U3P_PIB_PMMC_WR_DIRECT                           (1u << 16) /* <16:16> W1S:RW1C:0:Yes */


/*
   The addressed socket is not active during data write. FW reads the command
   argument to determine which socket is being written. This can happen for
   second block onwards. This interrupt is provided for all sockets (including
   direct socket) that have the SCK_NOT_RDY_MASK enabled.
 */
#define CY_U3P_PIB_PMMC_WR_SCK_NOT_RDY                      (1u << 17) /* <17:17> W1S:RW1C:0:Yes */


/*
   Read command received on direct read socket.
 */
#define CY_U3P_PIB_PMMC_RD_DIRECT                           (1u << 18) /* <18:18> W1S:RW1C:0:Yes */


/*
   Addressed socket is not active during data read. FW reads the command
   argument to determine which socket is being read. This interrupt is provided
   for all sockets (including direct socket) that have the SCK_NOT_RDY_MASK
   enabled.
 */
#define CY_U3P_PIB_PMMC_RD_SCK_NOT_RDY                      (1u << 19) /* <19:19> W1S:RW1C:0:Yes */


/*
   An error occurred in the MMC controller. FW clears this bit after handling
   the eror. The error code is indicated in PMMC_CONFIG.MMC_TRAN_ERR_CODE
 */
#define CY_U3P_PIB_PMMC_MMC_TRAN_ERR                        (1u << 20) /* <20:20> W1S:RW1C:0:Yes */


/*
   GO_PRE_IDLE received in any state other than PRE_BOOT
 */
#define CY_U3P_PIB_PMMC_GO_PRE_IDLE                         (1u << 27) /* <27:27> W1S:RW1C:0:Yes */


/*
   Entry to eMMC boot detected via CMD_LINE_LOW or CMD0(BOOT_INITIATION)
   command in PRE_BOOT state
 */
#define CY_U3P_PIB_PMMC_BOOT_ENTRY                          (1u << 28) /* <28:28> W1S:RW1C:0:Yes */


/*
   Exit from eMMC boot detected via CMD_LINE_HIGH or receiving CMD0 in BOOT
   state.
 */
#define CY_U3P_PIB_PMMC_BOOT_EXIT                           (1u << 29) /* <29:29> W1S:RW1C:0:Yes */


/*
   This interrupt is generated as soon a supported (i.e. legal), CRC error-free
   command is received. This interrupt is added for debug purpose. This bit
   will be masked during normal operation.
 */
#define CY_U3P_PIB_PMMC_NEW_CMD_RCVD                        (1u << 30) /* <30:30> W1S:RW1C:0:Yes */


/*
   State change after receiving a command
 */
#define CY_U3P_PIB_PMMC_STATE_CHANGE                        (1u << 31) /* <31:31> W1S:RW1C:0:Yes */



/*
   PIB PMMC Interrupt Mask Register
 */
#define CY_U3P_PIB_PMMC_INTR_MASK_ADDRESS                   (0xe0011010)
#define CY_U3P_PIB_PMMC_INTR_MASK                           (*(uvint32_t *)(0xe0011010))
#define CY_U3P_PIB_PMMC_INTR_MASK_DEFAULT                   (0x00000000)

/*
   Enables interrupt reporting to CPU when 1.
   Does not affect interrupt recording in PMMC_INTR.
 */
#define CY_U3P_PIB_PMMC_GO_IDLE                             (1u << 0) /* <0:0> R:RW:0:No */


/*
   Same
 */
#define CY_U3P_PIB_PMMC_CMD1                                (1u << 1) /* <1:1> R:RW:0:No */


/*
   Same
 */
#define CY_U3P_PIB_PMMC_CMD4                                (1u << 2) /* <2:2> R:RW:0:No */


/*
   Same
 */
#define CY_U3P_PIB_PMMC_CMD5_SLEEP                          (1u << 3) /* <3:3> R:RW:0:No */


/*
   Same
 */
#define CY_U3P_PIB_PMMC_CMD5_AWAKE                          (1u << 4) /* <4:4> R:RW:0:No */


/*
   Same
 */
#define CY_U3P_PIB_PMMC_CMD6                                (1u << 5) /* <5:5> R:RW:0:No */


/*
   Same
 */
#define CY_U3P_PIB_PMMC_CMD12                               (1u << 6) /* <6:6> R:RW:0:No */


/*
   Same
 */
#define CY_U3P_PIB_PMMC_CMD15                               (1u << 7) /* <7:7> R:RW:0:No */


/*
   Same
 */
#define CY_U3P_PIB_PMMC_CMD16                               (1u << 8) /* <8:8> R:RW:0:No */


/*
   Same
 */
#define CY_U3P_PIB_PMMC_WR_DIRECT                           (1u << 16) /* <16:16> R:RW:0:No */


/*
   Same
 */
#define CY_U3P_PIB_PMMC_WR_SCK_NOT_RDY                      (1u << 17) /* <17:17> R:RW:0:No */


/*
   Same
 */
#define CY_U3P_PIB_PMMC_RD_DIRECT                           (1u << 18) /* <18:18> R:RW:0:No */


/*
   Same
 */
#define CY_U3P_PIB_PMMC_RD_SCK_NOT_RDY                      (1u << 19) /* <19:19> R:RW:0:No */


/*
   Same
 */
#define CY_U3P_PIB_PMMC_MMC_TRAN_ERR                        (1u << 20) /* <20:20> R:RW:0:No */


/*
   Same
 */
#define CY_U3P_PIB_PMMC_GO_PRE_IDLE                         (1u << 27) /* <27:27> R:RW:0:No */


/*
   Same
 */
#define CY_U3P_PIB_PMMC_BOOT_ENTRY                          (1u << 28) /* <28:28> R:RW:0:No */


/*
   Same
 */
#define CY_U3P_PIB_PMMC_BOOT_EXIT                           (1u << 29) /* <29:29> R:RW:0:No */


/*
   Same
 */
#define CY_U3P_PIB_PMMC_NEW_CMD_RCVD                        (1u << 30) /* <30:30> R:RW:0:No */


/*
   Same
 */
#define CY_U3P_PIB_PMMC_STATE_CHANGE                        (1u << 31) /* <31:31> R:RW:0:No */



/*
   Configuration for busy indication
 */
#define CY_U3P_PIB_PMMC_WBUSY_CFG_ADDRESS                   (0xe0011014)
#define CY_U3P_PIB_PMMC_WBUSY_CFG                           (*(uvint32_t *)(0xe0011014))
#define CY_U3P_PIB_PMMC_WBUSY_CFG_DEFAULT                   (0x00000000)

/*
   0: Busy is deasserted immediately after WBUSY_COUNT cycles.
   1: Busy is deasserted when socket indicates ready to receive, i.e., when
   an empty buffer is available for AP to write.
 */
#define CY_U3P_PIB_PMMC_WBUSY_MASK                          (0xffffffff) /* <0:31> R:RW:0:No */
#define CY_U3P_PIB_PMMC_WBUSY_POS                           (0)



/*
   Busy time counter
 */
#define CY_U3P_PIB_PMMC_WBUSY_COUNT_ADDRESS                 (0xe0011018)
#define CY_U3P_PIB_PMMC_WBUSY_COUNT                         (*(uvint32_t *)(0xe0011018))
#define CY_U3P_PIB_PMMC_WBUSY_COUNT_DEFAULT                 (0x00000000)

/*
   Number of PIB_CLK cycles to wait before deasserting BUSY.
 */
#define CY_U3P_PIB_PMMC_WBUSY_COUNT_MASK                    (0x000000ff) /* <0:7> R:RW:0:No */
#define CY_U3P_PIB_PMMC_WBUSY_COUNT_POS                     (0)


/*
   Additional number of cycles for read response when data is already available.
   Note that PMMC HW always at least provides 2 cycles delay.
 */
#define CY_U3P_PIB_PMMC_NAC_MIN_MASK                        (0x0000ff00) /* <8:15> R:RW:0:No */
#define CY_U3P_PIB_PMMC_NAC_MIN_POS                         (8)



/*
   CMD0 argument
 */
#define CY_U3P_PIB_PMMC_CMD0_ARG_ADDRESS                    (0xe001101c)
#define CY_U3P_PIB_PMMC_CMD0_ARG                            (*(uvint32_t *)(0xe001101c))
#define CY_U3P_PIB_PMMC_CMD0_ARG_DEFAULT                    (0x00000000)

/*
   Last received CMD0 argument
 */
#define CY_U3P_PIB_PMMC_CMD0_ARG_MASK                       (0xffffffff) /* <0:31> W:R:0:No */
#define CY_U3P_PIB_PMMC_CMD0_ARG_POS                        (0)



/*
   PMMC Command Register
 */
#define CY_U3P_PIB_PMMC_IDX_ADDRESS                         (0xe0011020)
#define CY_U3P_PIB_PMMC_IDX                                 (*(uvint32_t *)(0xe0011020))
#define CY_U3P_PIB_PMMC_IDX_DEFAULT                         (0x00000000)

/*
   Received 6-bit command.
 */
#define CY_U3P_PIB_PMMC_CMD_MASK                            (0x0000003f) /* <0:5> W:R:0:No */
#define CY_U3P_PIB_PMMC_CMD_POS                             (0)


/*
   Transmission bit (as received)
 */
#define CY_U3P_PIB_PMMC_TR_BIT                              (1u << 6) /* <6:6> W:R:0:No */


/*
   Start bit (as received)
 */
#define CY_U3P_PIB_PMMC_ST_BIT                              (1u << 7) /* <7:7> W:R:0:No */


/*
   End-bit (as received)
 */
#define CY_U3P_PIB_PMMC_END_BIT                             (1u << 8) /* <8:8> W:R:0:No */


/*
   CRC7 bits.
 */
#define CY_U3P_PIB_PMMC_CRC7_MASK                           (0x0000fe00) /* <9:15> W:R:0:No */
#define CY_U3P_PIB_PMMC_CRC7_POS                            (9)



/*
   PMMC Command Argument Register
 */
#define CY_U3P_PIB_PMMC_ARG_ADDRESS                         (0xe0011024)
#define CY_U3P_PIB_PMMC_ARG                                 (*(uvint32_t *)(0xe0011024))
#define CY_U3P_PIB_PMMC_ARG_DEFAULT                         (0x00000000)

/*
   32-bit arguments coming in command.
 */
#define CY_U3P_PIB_PMMC_ARG_MASK                            (0xffffffff) /* <0:31> W:R:0:No */
#define CY_U3P_PIB_PMMC_ARG_POS                             (0)



/*
   PIB PMMC Last Block Read/Write Command Register
 */
#define CY_U3P_PIB_PMMC_BLK_CMD_ADDRESS                     (0xe0011028)
#define CY_U3P_PIB_PMMC_BLK_CMD                             (*(uvint32_t *)(0xe0011028))
#define CY_U3P_PIB_PMMC_BLK_CMD_DEFAULT                     (0x00000000)

/*
   Received 6-bit command.
 */
#define CY_U3P_PIB_PMMC_CMD_MASK                            (0x0000003f) /* <0:5> W:R:0:No */
#define CY_U3P_PIB_PMMC_CMD_POS                             (0)


/*
   Transmission bit (as received)
 */
#define CY_U3P_PIB_PMMC_TR_BIT                              (1u << 6) /* <6:6> W:R:0:No */


/*
   Start bit (as received)
 */
#define CY_U3P_PIB_PMMC_ST_BIT                              (1u << 7) /* <7:7> W:R:0:No */


/*
   End-bit (as received)
 */
#define CY_U3P_PIB_PMMC_END_BIT                             (1u << 8) /* <8:8> W:R:0:No */


/*
   CRC7 bits.
 */
#define CY_U3P_PIB_PMMC_CRC7_MASK                           (0x0000fe00) /* <9:15> W:R:0:No */
#define CY_U3P_PIB_PMMC_CRC7_POS                            (9)



/*
   PIB PMMC Last Block Read/Write Command Argument Register
 */
#define CY_U3P_PIB_PMMC_BLK_ADDR_ADDRESS                    (0xe001102c)
#define CY_U3P_PIB_PMMC_BLK_ADDR                            (*(uvint32_t *)(0xe001102c))
#define CY_U3P_PIB_PMMC_BLK_ADDR_DEFAULT                    (0x00000000)

/*
   32-bit arguments coming in command.
 */
#define CY_U3P_PIB_PMMC_ARG_MASK                            (0xffffffff) /* <0:31> W:R:0:No */
#define CY_U3P_PIB_PMMC_ARG_POS                             (0)



/*
   PMMC programming busy Register
 */
#define CY_U3P_PIB_PMMC_BUSY_ADDRESS                        (0xe0011030)
#define CY_U3P_PIB_PMMC_BUSY                                (*(uvint32_t *)(0xe0011030))
#define CY_U3P_PIB_PMMC_BUSY_DEFAULT                        (0x00000000)

/*
   Indicates busy status of DAT0.
   This field is reset when CMD0(GO_PRE_IDLE, GO_IDLE) is received.
 */
#define CY_U3P_PIB_PMMC_BUSY_STAT                           (1u << 0) /* <0:0> RW:R:0:Yes */


/*
   CPU clears busy status by writing '1' to this bit.
 */
#define CY_U3P_PIB_PMMC_BUSY_CLEAR                          (1u << 1) /* <1:1> RW0C:RW1S:0:Yes */



/*
   PMMC Block count register
 */
#define CY_U3P_PIB_PMMC_BLOCK_COUNT_ADDRESS                 (0xe0011034)
#define CY_U3P_PIB_PMMC_BLOCK_COUNT                         (*(uvint32_t *)(0xe0011034))
#define CY_U3P_PIB_PMMC_BLOCK_COUNT_DEFAULT                 (0x00000000)

/*
   Block count set by host using SET_BLOCK_COUNT, counting down as data transfers.
   This field is reset when CMD0(GO_PRE_IDLE, GO_IDLE) is received.
 */
#define CY_U3P_PIB_PMMC_BLOCK_CNT_MASK                      (0x0000ffff) /* <0:15> RW:R:0:Yes */
#define CY_U3P_PIB_PMMC_BLOCK_CNT_POS                       (0)



/*
   PMMC Block length Register
 */
#define CY_U3P_PIB_PMMC_BLOCK_LEN_ADDRESS                   (0xe0011038)
#define CY_U3P_PIB_PMMC_BLOCK_LEN                           (*(uvint32_t *)(0xe0011038))
#define CY_U3P_PIB_PMMC_BLOCK_LEN_DEFAULT                   (0x00000200)

/*
   Block length set by host using SET_BLOCKLEN command; Block length is set
   by BootROM or FW during eMMC boot operation. Note that this register is
   not affected if Argument Bit[31] = 1 for SET_BLOCKLEN command.
 */
#define CY_U3P_PIB_PMMC_BLOCK_LEN_MASK                      (0xffffffff) /* <0:31> RW:RW:512:Yes */
#define CY_U3P_PIB_PMMC_BLOCK_LEN_POS                       (0)



/*
   PMMC socket direction register
 */
#define CY_U3P_PIB_PMMC_SCK_DIRECTION_ADDRESS               (0xe001103c)
#define CY_U3P_PIB_PMMC_SCK_DIRECTION                       (*(uvint32_t *)(0xe001103c))
#define CY_U3P_PIB_PMMC_SCK_DIRECTION_DEFAULT               (0x00000000)

/*
   This bit specifies if a socket is ingress or egress socket.
   0: Egress socket
   1: Ingress socket
   Note: Sockets 16 thru 31 should programmed as ingress sockets only.
   FW needs to program this bit before enabling the socket.
 */
#define CY_U3P_PIB_PMMC_DIRECTION_MASK                      (0xffffffff) /* <0:31> R:RW:0:No */
#define CY_U3P_PIB_PMMC_DIRECTION_POS                       (0)



/*
   PMMC socket not ready interrupt mask
 */
#define CY_U3P_PIB_PMMC_SCK_NOT_RDY_MASK_ADDRESS            (0xe0011040)
#define CY_U3P_PIB_PMMC_SCK_NOT_RDY_MASK                    (*(uvint32_t *)(0xe0011040))
#define CY_U3P_PIB_PMMC_SCK_NOT_RDY_MASK_DEFAULT            (0x00000000)

/*
   This bit specifies if RD/WR SCK_NOT_RDY interrupt for a socket is masked
   or not.
   0: Mask the interrupt
   1: Enable the interrupt
 */
#define CY_U3P_PIB_PMMC_INTR_ENABLE_MASK                    (0xffffffff) /* <0:31> R:RW:0:No */
#define CY_U3P_PIB_PMMC_INTR_ENABLE_POS                     (0)



/*
   PMMC socket not ready interrupt mask
 */
#define CY_U3P_PIB_PMMC_REG_BLOCK_LEN_ADDRESS               (0xe0011044)
#define CY_U3P_PIB_PMMC_REG_BLOCK_LEN                       (*(uvint32_t *)(0xe0011044))
#define CY_U3P_PIB_PMMC_REG_BLOCK_LEN_DEFAULT               (0x00000002)

/*
   This field specifies the block length for register read/writes.
   Note that this register is not affected if Argument Bit[30] = 1 for SET_BLOCKLEN
   command.
 */
#define CY_U3P_PIB_PMMC_REG_BLOCK_LEN_MASK                  (0xffffffff) /* <0:31> RW:R:2:No */
#define CY_U3P_PIB_PMMC_REG_BLOCK_LEN_POS                   (0)



/*
   PIB MMC Card Identification Register (CID)
 */
#define CY_U3P_PIB_PMMC_CID0_ADDRESS                        (0xe0011048)
#define CY_U3P_PIB_PMMC_CID0                                (*(uvint32_t *)(0xe0011048))
#define CY_U3P_PIB_PMMC_CID0_DEFAULT                        (0x00000001)

/*
   Not used, always 1 for valid CID’s
 */
#define CY_U3P_PIB_PMMC_NOT_USED                            (1u << 0) /* <0:0> R:R:1:No */


/*
   CRC7 checksum
 */
#define CY_U3P_PIB_PMMC_CRC_MASK                            (0x000000fe) /* <1:7> RW:R:0:No */
#define CY_U3P_PIB_PMMC_CRC_POS                             (1)


/*
   Manufacturing date
 */
#define CY_U3P_PIB_PMMC_MDT_MASK                            (0x0000ff00) /* <8:15> R:RW:0:No */
#define CY_U3P_PIB_PMMC_MDT_POS                             (8)


/*
   Product serial number
 */
#define CY_U3P_PIB_PMMC_PSN_L_MASK                          (0xffff0000) /* <16:31> R:RW:0:No */
#define CY_U3P_PIB_PMMC_PSN_L_POS                           (16)



/*
   PIB MMC Card Identification Register (CID)
 */
#define CY_U3P_PIB_PMMC_CID1_ADDRESS                        (0xe001104c)
#define CY_U3P_PIB_PMMC_CID1                                (*(uvint32_t *)(0xe001104c))
#define CY_U3P_PIB_PMMC_CID1_DEFAULT                        (0x00000000)

/*
   Product serial number
 */
#define CY_U3P_PIB_PMMC_PSN_H_MASK                          (0x0000ffff) /* <0:15> R:RW:0:No */
#define CY_U3P_PIB_PMMC_PSN_H_POS                           (0)


/*
   Product revision
 */
#define CY_U3P_PIB_PMMC_PRV_MASK                            (0x00ff0000) /* <16:23> R:RW:0:No */
#define CY_U3P_PIB_PMMC_PRV_POS                             (16)


/*
   Product name
 */
#define CY_U3P_PIB_PMMC_PNM_L_MASK                          (0xff000000) /* <24:31> R:RW:0:No */
#define CY_U3P_PIB_PMMC_PNM_L_POS                           (24)



/*
   PIB MMC Card Identification Register (CID)
 */
#define CY_U3P_PIB_PMMC_CID2_ADDRESS                        (0xe0011050)
#define CY_U3P_PIB_PMMC_CID2                                (*(uvint32_t *)(0xe0011050))
#define CY_U3P_PIB_PMMC_CID2_DEFAULT                        (0x00000000)

/*
   Product name
 */
#define CY_U3P_PIB_PMMC_PNM_H_L_MASK                        (0xffffffff) /* <0:31> R:RW:0:No */
#define CY_U3P_PIB_PMMC_PNM_H_L_POS                         (0)



/*
   PIB MMC Card Identification Register (CID)
 */
#define CY_U3P_PIB_PMMC_CID3_ADDRESS                        (0xe0011054)
#define CY_U3P_PIB_PMMC_CID3                                (*(uvint32_t *)(0xe0011054))
#define CY_U3P_PIB_PMMC_CID3_DEFAULT                        (0x00000000)

/*
   Product name
 */
#define CY_U3P_PIB_PMMC_PNM_H_MASK                          (0x000000ff) /* <0:7> R:RW:0:No */
#define CY_U3P_PIB_PMMC_PNM_H_POS                           (0)


/*
   OEM/Application ID
 */
#define CY_U3P_PIB_PMMC_OID_MASK                            (0x0000ff00) /* <8:15> R:RW:0:No */
#define CY_U3P_PIB_PMMC_OID_POS                             (8)


/*
   Card/BGA
 */
#define CY_U3P_PIB_PMMC_CBX_MASK                            (0x00030000) /* <16:17> R:RW:0:No */
#define CY_U3P_PIB_PMMC_CBX_POS                             (16)


/*
   Manufacturer ID
 */
#define CY_U3P_PIB_PMMC_MID_MASK                            (0xff000000) /* <24:31> R:RW:0:No */
#define CY_U3P_PIB_PMMC_MID_POS                             (24)



/*
   PIB MMC Relative Card Address Register (RCA)
 */
#define CY_U3P_PIB_PMMC_RCA_ADDRESS                         (0xe0011058)
#define CY_U3P_PIB_PMMC_RCA                                 (*(uvint32_t *)(0xe0011058))
#define CY_U3P_PIB_PMMC_RCA_DEFAULT                         (0x00000001)

/*
   Relative card address. Set by AP using SET_RCA command and set by Benicia
   FW when getting out of standby.
 */
#define CY_U3P_PIB_PMMC_RCA_MASK                            (0x0000ffff) /* <0:15> RW:RW:1:Yes */
#define CY_U3P_PIB_PMMC_RCA_POS                             (0)



/*
   PIB MMC Drive Strength Register (DSR)
 */
#define CY_U3P_PIB_PMMC_DSR_ADDRESS                         (0xe001105c)
#define CY_U3P_PIB_PMMC_DSR                                 (*(uvint32_t *)(0xe001105c))
#define CY_U3P_PIB_PMMC_DSR_DEFAULT                         (0x00000404)

/*
   Drive Strengths
 */
#define CY_U3P_PIB_PMMC_DSR_MASK                            (0x0000ffff) /* <0:15> RW:RW:0x0404:Yes */
#define CY_U3P_PIB_PMMC_DSR_POS                             (0)



/*
   PIB MMC Card Specific Data Register (CSD)
 */
#define CY_U3P_PIB_PMMC_CSD0_ADDRESS                        (0xe0011060)
#define CY_U3P_PIB_PMMC_CSD0                                (*(uvint32_t *)(0xe0011060))
#define CY_U3P_PIB_PMMC_CSD0_DEFAULT                        (0x00000001)

/*
    Not used, always 1 for valid CSDs
 */
#define CY_U3P_PIB_CSD0_NOT_USED                            (1u << 0) /* <0:0> R:R:1:No */


/*
    Computed and set by hardware
 */
#define CY_U3P_PIB_CSD0_CRC_MASK                            (0x000000fe) /* <1:7> RW:R:0:No */
#define CY_U3P_PIB_CSD0_CRC_POS                             (1)


/*
    0: None
 */
#define CY_U3P_PIB_CSD0_ECC_MASK                            (0x00000300) /* <8:9> R:RW:0:No */
#define CY_U3P_PIB_CSD0_ECC_POS                             (8)


/*
    Value = 3; (Unknown)
 */
#define CY_U3P_PIB_CSD0_FILE_FORMAT_MASK                    (0x00000c00) /* <10:11> R:RW:0:No */
#define CY_U3P_PIB_CSD0_FILE_FORMAT_POS                     (10)


/*
   N/A. Value to be set by firmware.
 */
#define CY_U3P_PIB_CSD0_TMP_WRITE_PROTECT                   (1u << 12) /* <12:12> R:RW:0:No */


/*
   N/A. Value to be set by firmware.
 */
#define CY_U3P_PIB_CSD0_PERM_WRITE_PROTECT                  (1u << 13) /* <13:13> R:RW:0:No */


/*
   N/A. Value to be set by firmware.
 */
#define CY_U3P_PIB_CSD0_COPY                                (1u << 14) /* <14:14> R:RW:0:No */


/*
   N/A. Value to be set by firmware.
 */
#define CY_U3P_PIB_CSD0_FILE_FORMAT_GRP                     (1u << 15) /* <15:15> R:RW:0:No */


/*
   N/A. Value to be set by firmware.
 */
#define CY_U3P_PIB_CSD0_CONTENT_PROT_APP                    (1u << 16) /* <16:16> R:RW:0:No */


/*
   N/A. Value =1
 */
#define CY_U3P_PIB_CSD0_WRITE_BL_PARTIAL                    (1u << 21) /* <21:21> R:RW:0:No */


/*
   Maximum write block length: This field is used to verify block length
   in SET_BLOCKLEN command. FW should program this to a minimum of 512 Bytes.
 */
#define CY_U3P_PIB_CSD0_WRITE_BL_LEN_MASK                   (0x03c00000) /* <22:25> R:RW:0:No */
#define CY_U3P_PIB_CSD0_WRITE_BL_LEN_POS                    (22)


/*
   N/A. Value = 1.
 */
#define CY_U3P_PIB_CSD0_R2W_FACTOR_MASK                     (0x1c000000) /* <26:28> R:RW:0:No */
#define CY_U3P_PIB_CSD0_R2W_FACTOR_POS                      (26)


/*
   0: None
 */
#define CY_U3P_PIB_CSD0_DEFAULT_ECC_MASK                    (0x60000000) /* <29:30> R:RW:0:No */
#define CY_U3P_PIB_CSD0_DEFAULT_ECC_POS                     (29)


/*
    N/A. Value=0
 */
#define CY_U3P_PIB_CSD0_WP_GRP_ENABLE                       (1u << 31) /* <31:31> R:RW:0:No */



/*
   PIB MMC Card Specific Data Register (CSD)
 */
#define CY_U3P_PIB_PMMC_CSD1_ADDRESS                        (0xe0011064)
#define CY_U3P_PIB_PMMC_CSD1                                (*(uvint32_t *)(0xe0011064))
#define CY_U3P_PIB_PMMC_CSD1_DEFAULT                        (0x00000000)

/*
    N/A. Value=0
 */
#define CY_U3P_PIB_CSD1_WP_GRP_SIZE_MASK                    (0x0000001f) /* <0:4> R:RW:0:No */
#define CY_U3P_PIB_CSD1_WP_GRP_SIZE_POS                     (0)


/*
    N/A. Value=0
 */
#define CY_U3P_PIB_CSD1_ERASE_GRP_MULT_MASK                 (0x000003e0) /* <5:9> R:RW:0:No */
#define CY_U3P_PIB_CSD1_ERASE_GRP_MULT_POS                  (5)


/*
    N/A. Value=0
 */
#define CY_U3P_PIB_CSD1_ERASE_GRP_SIZE_MASK                 (0x00007c00) /* <10:14> R:RW:0:No */
#define CY_U3P_PIB_CSD1_ERASE_GRP_SIZE_POS                  (10)


/*
   To be set to accommodate the address space defined in memory Map.
 */
#define CY_U3P_PIB_CSD1_C_SIZE_MULT_MASK                    (0x00038000) /* <15:17> R:RW:0:No */
#define CY_U3P_PIB_CSD1_C_SIZE_MULT_POS                     (15)


/*
    IROS to specify.
 */
#define CY_U3P_PIB_CSD1_VDD_R_CUR_MIN_MASK                  (0x001c0000) /* <18:20> R:RW:0:No */
#define CY_U3P_PIB_CSD1_VDD_R_CUR_MIN_POS                   (18)


/*
    IROS to specify.
 */
#define CY_U3P_PIB_CSD1_VDD_R_CUR_MAX_MASK                  (0x00e00000) /* <21:23> R:RW:0:No */
#define CY_U3P_PIB_CSD1_VDD_R_CUR_MAX_POS                   (21)


/*
    IROS to specify.
 */
#define CY_U3P_PIB_CSD1_VDD_W_CUR_MIN_MASK                  (0x07000000) /* <24:26> R:RW:0:No */
#define CY_U3P_PIB_CSD1_VDD_W_CUR_MIN_POS                   (24)


/*
    IROS to specify.
 */
#define CY_U3P_PIB_CSD1_VDD_W_CUR_MAX_MASK                  (0x38000000) /* <27:29> R:RW:0:No */
#define CY_U3P_PIB_CSD1_VDD_W_CUR_MAX_POS                   (27)


/*
   To be set to accommodate the address space defined in memory Map.
 */
#define CY_U3P_PIB_CSD1_C_SIZE_L_MASK                       (0xc0000000) /* <30:31> R:RW:0:No */
#define CY_U3P_PIB_CSD1_C_SIZE_L_POS                        (30)



/*
   PIB MMC Card Specific Data Register (CSD)
 */
#define CY_U3P_PIB_PMMC_CSD2_ADDRESS                        (0xe0011068)
#define CY_U3P_PIB_PMMC_CSD2                                (*(uvint32_t *)(0xe0011068))
#define CY_U3P_PIB_PMMC_CSD2_DEFAULT                        (0x00000000)

/*
   To be set to accommodate the address space defined in memory Map.
 */
#define CY_U3P_PIB_CSD2_C_SIZE_H_MASK                       (0x000003ff) /* <0:9> R:RW:0:No */
#define CY_U3P_PIB_CSD2_C_SIZE_H_POS                        (0)


/*
   DSR not implemented. Value = 0.
 */
#define CY_U3P_PIB_CSD2_DSR_IMP                             (1u << 12) /* <12:12> R:RW:0:No */


/*
   Read may cross block boundary. Value = 1.
 */
#define CY_U3P_PIB_CSD2_READ_BLK_MISALIGN                   (1u << 13) /* <13:13> R:RW:0:No */


/*
   Write may cross block boundary. Value = 1.
 */
#define CY_U3P_PIB_CSD2_WRITE_BLK_MISALIGN                  (1u << 14) /* <14:14> R:RW:0:No */


/*
   Allowed to read smaller block lengths, down to 1 B. Value = 1
 */
#define CY_U3P_PIB_CSD2_READ_BL_PARTIAL                     (1u << 15) /* <15:15> R:RW:0:No */


/*
   Maximum read block length. This value is 9, 10 or 11 for transfer block
   size 512, 1K and 2K bytes respectively. FW should program this to a minimum
   of 512 bytes
 */
#define CY_U3P_PIB_CSD2_READ_BL_LEN_MASK                    (0x000f0000) /* <16:19> R:RW:0:No */
#define CY_U3P_PIB_CSD2_READ_BL_LEN_POS                     (16)


/*
   Class 0, Class 2, Class 4 and Class 10
 */
#define CY_U3P_PIB_CSD2_CCC_MASK                            (0xfff00000) /* <20:31> R:RW:0:No */
#define CY_U3P_PIB_CSD2_CCC_POS                             (20)



/*
   PIB MMC Card Specific Data Register (CSD)
 */
#define CY_U3P_PIB_PMMC_CSD3_ADDRESS                        (0xe001106c)
#define CY_U3P_PIB_PMMC_CSD3                                (*(uvint32_t *)(0xe001106c))
#define CY_U3P_PIB_PMMC_CSD3_DEFAULT                        (0x00000000)

/*
   32h when operating at max. of 25 MHz
   5Ah when operating at 50 MHz. TBD if HW has to set this.
 */
#define CY_U3P_PIB_CSD3_TRAN_SPEED_MASK                     (0x000000ff) /* <0:7> R:RW:0:No */
#define CY_U3P_PIB_CSD3_TRAN_SPEED_POS                      (0)


/*
   Read access time in clock cycles
 */
#define CY_U3P_PIB_CSD3_NSAC_MASK                           (0x0000ff00) /* <8:15> R:RW:0:No */
#define CY_U3P_PIB_CSD3_NSAC_POS                            (8)


/*
   Data read access time – 1
 */
#define CY_U3P_PIB_CSD3_TAAC_MASK                           (0x00ff0000) /* <16:23> R:RW:0:No */
#define CY_U3P_PIB_CSD3_TAAC_POS                            (16)


/*
   This field applicable for PMMC only.
 */
#define CY_U3P_PIB_CSD3_SPEC_VERS_MASK                      (0x3c000000) /* <26:29> R:RW:0:No */
#define CY_U3P_PIB_CSD3_SPEC_VERS_POS                       (26)


/*
   V1.0 CSD_STRUCTURE is supported. Value = 00b
 */
#define CY_U3P_PIB_CSD3_CSD_STRUCTURE_MASK                  (0xc0000000) /* <30:31> R:RW:0:No */
#define CY_U3P_PIB_CSD3_CSD_STRUCTURE_POS                   (30)



/*
   PIB MMC Operation Conditions Register (OCR)
 */
#define CY_U3P_PIB_PMMC_OCR_ADDRESS                         (0xe0011070)
#define CY_U3P_PIB_PMMC_OCR                                 (*(uvint32_t *)(0xe0011070))
#define CY_U3P_PIB_PMMC_OCR_DEFAULT                         (0x00ff8080)

/*
   FW programs this field to indicate dual voltage operation and byte mode
   operation. Default Value (0x00FF8080) indicates sector address.
 */
#define CY_U3P_PIB_PMMC_OCR_MASK                            (0x7fffffff) /* <0:30> R:RW:0x00FF8080:No */
#define CY_U3P_PIB_PMMC_OCR_POS                             (0)


/*
   Card status after a CMD0 or power-up:
   0: Card status busy
   1: Card status ready
   FW sets this bit to 1 when it has finished initialization.
   This field is reset when CMD0(GO_PRE_IDLE, GO_IDLE) is received.
 */
#define CY_U3P_PIB_PMMC_OCR_STATUS                          (1u << 31) /* <31:31> RW0C:RW1S:0:Yes */



/*
   PIB MMC Card Status Register (CS)
 */
#define CY_U3P_PIB_PMMC_CSR_ADDRESS                         (0xe0011074)
#define CY_U3P_PIB_PMMC_CSR                                 (*(uvint32_t *)(0xe0011074))
#define CY_U3P_PIB_PMMC_CSR_DEFAULT                         (0x00000000)

/*
   N/A Not Implemented
 */
#define CY_U3P_PIB_CSR_APP_CMD                              (1u << 5) /* <5:5> R:R:0:No */


/*
   Set by FW on error, after interpretation of SWITCH command. Cleared by
   HW if set, when sent out in R1 response.
 */
#define CY_U3P_PIB_CSR_SWITCH_ERROR                         (1u << 7) /* <7:7> RW0C:RW1S:0:Yes */


/*
   Set by HW, with AND of SOCK_STAT of unmasked write sockets. This is a
   status bit and is not cleared by HW on sending R1 response.
 */
#define CY_U3P_PIB_CSR_READY_FOR_DATA                       (1u << 8) /* <8:8> RW:R:0:Yes */


/*
   Current state of the card set by HW.
   FW can set the state by writing to PMMC_CONFIG register.
   This is a status field and is not cleared by HW on sending R1 response.
 */
#define CY_U3P_PIB_CSR_CURRENT_STATE_MASK                   (0x00001e00) /* <9:12> RW:R:0:Yes */
#define CY_U3P_PIB_CSR_CURRENT_STATE_POS                    (9)


/*
   N/A Not Implemented
 */
#define CY_U3P_PIB_CSR_ERASE_RESET                          (1u << 13) /* <13:13> R:R:0:No */


/*
   N/A Not Implemented
 */
#define CY_U3P_PIB_CSR_WP_ERASE_SKIP                        (1u << 15) /* <15:15> R:R:0:No */


/*
   CMD26 and CMD27 are not supported. Set by HW.
   Cleared by HW if set, when sent out in R1 response.
 */
#define CY_U3P_PIB_CSR_CID_CSD_OVERWRITE                    (1u << 16) /* <16:16> RW:R:0:Yes */


/*
   N/A Not Implemented
 */
#define CY_U3P_PIB_CSR_OVERRUN                              (1u << 17) /* <17:17> R:R:0:No */


/*
   N/A Not Implemented
 */
#define CY_U3P_PIB_CSR_UNDERRUN                             (1u << 18) /* <18:18> R:R:0:No */


/*
   A generic card error set by FW.
   Cleared by HW if set, when sent out in R1 response.
 */
#define CY_U3P_PIB_CSR_ERROR                                (1u << 19) /* <19:19> RW0C:RW1S:0:Yes */


/*
   N/A Not Implemented
 */
#define CY_U3P_PIB_CSR_CC_ERROR                             (1u << 20) /* <20:20> R:R:0:No */


/*
   Set by FW when there is there is an ECC error in storage access..
   Cleared by HW if set, when sent out in R1 response.
 */
#define CY_U3P_PIB_CSR_CARD_ECC_FAILED                      (1u << 21) /* <21:21> RW0C:RW1S:0:Yes */


/*
   Command not legal for the card state. Set by HW.
   Cleared by HW if set, when sent out in R1 response.
 */
#define CY_U3P_PIB_CSR_ILLEGAL_COMMAND                      (1u << 22) /* <22:22> RW:R:0:Yes */


/*
   Set by HW when CRC check of a command fails.
   Cleared by HW if set, when sent out in R1 response.
 */
#define CY_U3P_PIB_CSR_COM_CRC_ERROR                        (1u << 23) /* <23:23> RW:R:0:Yes */


/*
   N/A Not Implemented
 */
#define CY_U3P_PIB_CSR_LOCK_UNLOCK_FAILED                   (1u << 24) /* <24:24> R:R:0:No */


/*
   N/A Not Implemented
 */
#define CY_U3P_PIB_CSR_CARD_IS_LOCKED                       (1u << 25) /* <25:25> R:R:0:No */


/*
   Set by FW when it detects write protect violation for direct writes.
   Cleared by HW if set, when sent out in R1 response.
 */
#define CY_U3P_PIB_CSR_WP_VIOLATION                         (1u << 26) /* <26:26> RW0C:RW1S:0:yes */


/*
   N/A Not Implemented
 */
#define CY_U3P_PIB_CSR_ERASE_PARAM                          (1u << 27) /* <27:27> R:R:0:No */


/*
   N/A Not Implemented
 */
#define CY_U3P_PIB_CSR_ERASE_SEQ_ERROR                      (1u << 28) /* <28:28> R:R:0:No */


/*
   Set by HW or FW when HW or FW detects that argument of a SET_BLOCKLEN
   command exceeds the maximum allowed value.  Cleared by HW if set, when
   sent out in R1 response.
 */
#define CY_U3P_PIB_CSR_BLOCK_LEN_ERROR                      (1u << 29) /* <29:29> RW:RW1S :0:Yes */


/*
   Set by HW when register address is not word aligned. Set by FW if access
   is not aligned.
   Cleared by HW if set, when sent out in R1 response.
 */
#define CY_U3P_PIB_CSR_ADDRESS_MISALIGN                     (1u << 30) /* <30:30> RW:RW1S :0:Yes */


/*
   The command's address argument was out of the allowed range. Set by HW
   or FW when HW or FW detects this condition.
   Cleared by HW if set, when sent out in R1 response.
 */
#define CY_U3P_PIB_CSR_ADDRESS_OUT_OF_RANGE                 (1u << 31) /* <31:31> RW:RW1S :0:Yes */



/*
   PIB Extended CSD Register(EXT_CSD)
 */
#define CY_U3P_PIB_PMMC_EXT_CSD0_ADDRESS                    (0xe0011078)
#define CY_U3P_PIB_PMMC_EXT_CSD0                            (*(uvint32_t *)(0xe0011078))
#define CY_U3P_PIB_PMMC_EXT_CSD0_DEFAULT                    (0x00000000)

/*
   PMMC data bus width (default 0)
   0: 1 bit data bus
   1: 4 bit data bus
   2: 8 bit data bus
   HW uses this field before any data transfer other than BOOT state data
   transfer.
 */
#define CY_U3P_PIB_EXT_CSD0_BUS_WIDTH_MASK                  (0x000000ff) /* <0:7> R:RW:0:No */
#define CY_U3P_PIB_EXT_CSD0_BUS_WIDTH_POS                   (0)


/*
   Set to 1 for high speed timing
 */
#define CY_U3P_PIB_EXT_CSD0_HS_TIMING_MASK                  (0x0000ff00) /* <8:15> R:RW:0:No */
#define CY_U3P_PIB_EXT_CSD0_HS_TIMING_POS                   (8)


/*
   Default: 0x4 ==> 200 mA
 */
#define CY_U3P_PIB_EXT_CSD0_POWER_CLASS_MASK                (0x00ff0000) /* <16:23> R:RW:0:No */
#define CY_U3P_PIB_EXT_CSD0_POWER_CLASS_POS                 (16)


/*
   Command set revision
 */
#define CY_U3P_PIB_EXT_CSD0_CMD_SET_REV_MASK                (0xff000000) /* <24:31> R:RW:0:No */
#define CY_U3P_PIB_EXT_CSD0_CMD_SET_REV_POS                 (24)



/*
   PIB Extended CSD Register(EXT_CSD)
 */
#define CY_U3P_PIB_PMMC_EXT_CSD1_ADDRESS                    (0xe001107c)
#define CY_U3P_PIB_PMMC_EXT_CSD1                            (*(uvint32_t *)(0xe001107c))
#define CY_U3P_PIB_PMMC_EXT_CSD1_DEFAULT                    (0x00000000)

/*
   Command Set  default 0x0 ==> Std MMC
 */
#define CY_U3P_PIB_EXT_CSD1_CMD_SET_MASK                    (0x000000ff) /* <0:7> R:RW:0:No */
#define CY_U3P_PIB_EXT_CSD1_CMD_SET_POS                     (0)


/*
   Extended CSD revision Default :0x2
 */
#define CY_U3P_PIB_EXT_CSD1_EXT_CSD_REV_MASK                (0x0000ff00) /* <8:15> R:RW:0:No */
#define CY_U3P_PIB_EXT_CSD1_EXT_CSD_REV_POS                 (8)


/*
   CSD Structure Version Default 0x02 ==> MMC4.2
 */
#define CY_U3P_PIB_EXT_CSD1_CSD_STRUCTURE_MASK              (0x00ff0000) /* <16:23> R:RW:0:No */
#define CY_U3P_PIB_EXT_CSD1_CSD_STRUCTURE_POS               (16)


/*
   Card Type Default: 0x3 è Supports 52 and 26 MHz
 */
#define CY_U3P_PIB_EXT_CSD1_CARD_TYPE_MASK                  (0xff000000) /* <24:31> R:RW:0:No */
#define CY_U3P_PIB_EXT_CSD1_CARD_TYPE_POS                   (24)



/*
   PIB Extended CSD Register(EXT_CSD)
 */
#define CY_U3P_PIB_PMMC_EXT_CSD2_ADDRESS                    (0xe0011080)
#define CY_U3P_PIB_PMMC_EXT_CSD2                            (*(uvint32_t *)(0xe0011080))
#define CY_U3P_PIB_PMMC_EXT_CSD2_DEFAULT                    (0x00000000)

/*
   Default 0x4 ==> 200 mA
 */
#define CY_U3P_PIB_EXT_CSD2_PWR_CL_52_195_MASK              (0x000000ff) /* <0:7> R:RW:0:No */
#define CY_U3P_PIB_EXT_CSD2_PWR_CL_52_195_POS               (0)


/*
   Default: 0x4 ==> 200 mA
 */
#define CY_U3P_PIB_EXT_CSD2_PWR_CL_26_195_MASK              (0x0000ff00) /* <8:15> R:RW:0:No */
#define CY_U3P_PIB_EXT_CSD2_PWR_CL_26_195_POS               (8)


/*
   Default: 0 ==> 200 mA
 */
#define CY_U3P_PIB_EXT_CSD2_PWR_CL_52_360_MASK              (0x00ff0000) /* <16:23> R:RW:0:No */
#define CY_U3P_PIB_EXT_CSD2_PWR_CL_52_360_POS               (16)


/*
   Default: 0 ==> 200 mA
 */
#define CY_U3P_PIB_EXT_CSD2_PWR_CL_26_360_MASK              (0xff000000) /* <24:31> R:RW:0:No */
#define CY_U3P_PIB_EXT_CSD2_PWR_CL_26_360_POS               (24)



/*
   PIB Extended CSD Register(EXT_CSD)
 */
#define CY_U3P_PIB_PMMC_EXT_CSD3_ADDRESS                    (0xe0011084)
#define CY_U3P_PIB_PMMC_EXT_CSD3                            (*(uvint32_t *)(0xe0011084))
#define CY_U3P_PIB_PMMC_EXT_CSD3_DEFAULT                    (0x00000000)

/*
   Minimum Write  performance for 4bit @26MHz 0x14 ==> 6 MBps Default: 0x14
 */
#define CY_U3P_PIB_EXT_CSD3_MIN_PERF_R_4_26_MASK            (0x000000ff) /* <0:7> R:RW:0:No */
#define CY_U3P_PIB_EXT_CSD3_MIN_PERF_R_4_26_POS             (0)


/*
   Minimum Write  performance for 4bit @26MHz 0x14 ==> 6 MBps Default: 0x14
 */
#define CY_U3P_PIB_EXT_CSD3_MIN_PERF_W_4_26_MASK            (0x0000ff00) /* <8:15> R:RW:0:No */
#define CY_U3P_PIB_EXT_CSD3_MIN_PERF_W_4_26_POS             (8)


/*
   Minimum Write Performance for 8bit @26MHz / 4bit @52MHz Default: 0x32
   ==> 15 MBps
 */
#define CY_U3P_PIB_EXT_CSD3_MIN_PERF_R_8_26_4_52_MASK       (0x00ff0000) /* <16:23> R:RW:0:No */
#define CY_U3P_PIB_EXT_CSD3_MIN_PERF_R_8_26_4_52_POS        (16)


/*
   Minimum Write Performance for 8bit @26MHz / 4bit @52MHz Default: 0x32
   ==> 15 MBps
 */
#define CY_U3P_PIB_EXT_CSD3_MIN_PERF_W_8_26_4_52_MASK       (0xff000000) /* <24:31> R:RW:0:No */
#define CY_U3P_PIB_EXT_CSD3_MIN_PERF_W_8_26_4_52_POS        (24)



/*
   PIB Extended CSD Register(EXT_CSD)
 */
#define CY_U3P_PIB_PMMC_EXT_CSD4_ADDRESS                    (0xe0011088)
#define CY_U3P_PIB_PMMC_EXT_CSD4                            (*(uvint32_t *)(0xe0011088))
#define CY_U3P_PIB_PMMC_EXT_CSD4_DEFAULT                    (0x00000000)

/*
   Minimum Read Performance for 8 bit @52MHz 0x64 ==> 30 MBps Default: 0x64
 */
#define CY_U3P_PIB_EXT_CSD4_MIN_PERF_R_8_52_MASK            (0x000000ff) /* <0:7> R:RW:0:No */
#define CY_U3P_PIB_EXT_CSD4_MIN_PERF_R_8_52_POS             (0)


/*
   Minimum Write Performance for 8bit @52MHz  0x64 ==> 30 MBps Default: 0x64
 */
#define CY_U3P_PIB_EXT_CSD4_MIN_PERF_W_8_52_MASK            (0x0000ff00) /* <8:15> R:RW:0:No */
#define CY_U3P_PIB_EXT_CSD4_MIN_PERF_W_8_52_POS             (8)


/*
   Default: 0x400000 ==> 2GB card
 */
#define CY_U3P_PIB_EXT_CSD4_SEC_COUNT_L_MASK                (0xffff0000) /* <16:31> R:RW:0:No */
#define CY_U3P_PIB_EXT_CSD4_SEC_COUNT_L_POS                 (16)



/*
   PIB Extended CSD Register(EXT_CSD)
 */
#define CY_U3P_PIB_PMMC_EXT_CSD5_ADDRESS                    (0xe001108c)
#define CY_U3P_PIB_PMMC_EXT_CSD5                            (*(uvint32_t *)(0xe001108c))
#define CY_U3P_PIB_PMMC_EXT_CSD5_DEFAULT                    (0x00000000)

/*
   Default: 0x400000 ==> 2GB card
 */
#define CY_U3P_PIB_EXT_CSD5_SEC_COUNT_H_MASK                (0x0000ffff) /* <0:15> R:RW:0:No */
#define CY_U3P_PIB_EXT_CSD5_SEC_COUNT_H_POS                 (0)


/*
   Default: 0
 */
#define CY_U3P_PIB_EXT_CSD5_S_CMD_SET_MASK                  (0x00ff0000) /* <16:23> R:RW:0:No */
#define CY_U3P_PIB_EXT_CSD5_S_CMD_SET_POS                   (16)



/*
   PMMC Control register
 */
#define CY_U3P_PIB_PMMC_CONTROL_ADDRESS                     (0xe00110a0)
#define CY_U3P_PIB_PMMC_CONTROL                             (*(uvint32_t *)(0xe00110a0))
#define CY_U3P_PIB_PMMC_CONTROL_DEFAULT                     (0x00000000)

/*
   When CPU writes changes this bit from 0 to 1, PMMC HW emits a Boot-ACK
   sequence "<Start-bit>010<End-bit>". SW must clear this bit. Clearing this
   bit has no other effect other than writing a zero to this bit.
 */
#define CY_U3P_PIB_PMMC_SEND_BOOT_ACK                       (1u << 0) /* <0:0> R:RW:0:Yes */



/*
   PMMC Read abort / Direct-Write wait register
 */
#define CY_U3P_PIB_PMMC_RD_ABORT_WAIT_ADDRESS               (0xe00110a4)
#define CY_U3P_PIB_PMMC_RD_ABORT_WAIT                       (*(uvint32_t *)(0xe00110a4))
#define CY_U3P_PIB_PMMC_RD_ABORT_WAIT_DEFAULT               (0x00000000)

/*
   Set by HW to indicate for which socket(s) CMD12 was received during read
   or direct write. Cleared by FW to indicate socket has been cleaned up.
   Bit <n> pertains to socket <n>
 */
#define CY_U3P_PIB_PMMC_SOCK_MASK                           (0xffffffff) /* <0:31> RW1S:RW1C:0:yes */
#define CY_U3P_PIB_PMMC_SOCK_POS                            (0)



#endif /* _INCLUDED_PMMC_REGS_H_ */

/*[]*/

