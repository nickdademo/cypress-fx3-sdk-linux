/****************************************************************************
 *
 * File: usb3lnk_regs.h
 *
 * Copyright (c) 2010-13 Cypress Semiconductor. All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION WHICH IS THE PROPERTY OF CYPRESS.
 *
 * Description:
 *   USB 3.0 Link registers for the EZ-USB FX3 Device.
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

#ifndef _INCLUDED_USB3LNK_REGS_H_
#define _INCLUDED_USB3LNK_REGS_H_

#include <cyu3types.h>

#define USB3LNK_BASE_ADDR                        (0xe0033000)

typedef struct
{
    uvint32_t lnk_conf;                           /* 0xe0033000 */
    uvint32_t lnk_intr;                           /* 0xe0033004 */
    uvint32_t lnk_intr_mask;                      /* 0xe0033008 */
    uvint32_t lnk_error_conf;                     /* 0xe003300c */
    uvint32_t lnk_error_status;                   /* 0xe0033010 */
    uvint32_t lnk_error_count;                    /* 0xe0033014 */
    uvint32_t lnk_error_count_threshold;          /* 0xe0033018 */
    uvint32_t lnk_phy_conf;                       /* 0xe003301c */
    uvint32_t reserved0[3];
    uvint32_t lnk_phy_mpll_status;                /* 0xe003302c */
    uvint32_t reserved1[3];
    uvint32_t lnk_phy_tx_trim;                    /* 0xe003303c */
    uvint32_t lnk_phy_error_conf;                 /* 0xe0033040 */
    uvint32_t lnk_phy_error_status;               /* 0xe0033044 */
    uvint32_t reserved2[2];
    uvint32_t lnk_device_power_control;           /* 0xe0033050 */
    uvint32_t lnk_ltssm_state;                    /* 0xe0033054 */
    uvint32_t reserved3[3];
    uvint32_t lnk_lfps_observe;                   /* 0xe0033064 */
    uvint32_t reserved4[52];
    uvint32_t lnk_compliance_pattern_0;           /* 0xe0033138 */
    uvint32_t lnk_compliance_pattern_1;           /* 0xe003313c */
    uvint32_t lnk_compliance_pattern_2;           /* 0xe0033140 */
    uvint32_t lnk_compliance_pattern_3;           /* 0xe0033144 */
    uvint32_t lnk_compliance_pattern_4;           /* 0xe0033148 */
    uvint32_t lnk_compliance_pattern_5;           /* 0xe003314c */
    uvint32_t lnk_compliance_pattern_6;           /* 0xe0033150 */
    uvint32_t lnk_compliance_pattern_7;           /* 0xe0033154 */
    uvint32_t lnk_compliance_pattern_8;           /* 0xe0033158 */
} USB3LNK_REGS_T, *PUSB3LNK_REGS_T;

#define USB3LNK        ((PUSB3LNK_REGS_T) USB3LNK_BASE_ADDR)


/*
   Link Configuration Register
 */
#define CY_U3P_UIB_LNK_CONF_ADDRESS                         (0xe0033000)
#define CY_U3P_UIB_LNK_CONF                                 (*(uvint32_t *)(0xe0033000))
#define CY_U3P_UIB_LNK_CONF_DEFAULT                         (0x00005040)

/*
   Link Arbitration Scheme
       0=Link Commands wins
       1=HP wins
       2=Round Robin
 */
#define CY_U3P_UIB_TX_ARBITRATION_MASK                      (0x00000003) /* <0:1> R:RW:0:No */
#define CY_U3P_UIB_TX_ARBITRATION_POS                       (0)


/*
   0: Check reserved bits in Link Control Word are 0 (LCW)
   1: Ignore reserved bits in Link Control Word (LCW)
 */
#define CY_U3P_UIB_LCW_IGNORE_RSVD                          (1u << 6) /* <6:6> R:RW:1:No */


/*
   Enable LNK State Debug Override
 */
#define CY_U3P_UIB_DEBUG_FEATURE_ENABLE                     (1u << 7) /* <7:7> R:RW:0:No */


/*
   Force PowerPresent from PHY On
 */
#define CY_U3P_UIB_FORCE_POWER_PRESENT                      (1u << 8) /* <8:8> R:RW:0:No */


/*
   Enable host LDN detection (see USB ECN#001)
 */
#define CY_U3P_UIB_LDN_DETECTION                            (1u << 9) /* <9:9> R:RW:0:No */


/*
   Hold-off Credit Advertisement until Sequence Number Advertisement Received
 */
#define CY_U3P_UIB_CREDIT_ADV_HOLDOFF                       (1u << 10) /* <10:10> R:RW:0:No */


/*
   Delay sending of first Header in a egress burst in PCLK cycles (125MHz).
    This is to give the DMA network time to warmup the data pipeline.
 */
#define CY_U3P_UIB_EPM_FIRST_DELAY_MASK                     (0x0000f000) /* <12:15> R:RW:5:No */
#define CY_U3P_UIB_EPM_FIRST_DELAY_POS                      (12)



/*
   Link Interrupt Register
 */
#define CY_U3P_UIB_LNK_INTR_ADDRESS                         (0xe0033004)
#define CY_U3P_UIB_LNK_INTR                                 (*(uvint32_t *)(0xe0033004))
#define CY_U3P_UIB_LNK_INTR_DEFAULT                         (0x00000000)

/*
   LTSSM State Change Interrupt
 */
#define CY_U3P_UIB_LTSSM_STATE_CHG                          (1u << 0) /* <0:0> RW1S:RW1C:0:No */


/*
   LGOOD Received Interrupt
 */
#define CY_U3P_UIB_LGOOD                                    (1u << 1) /* <1:1> RW1S:RW1C:0:No */


/*
   LRTY Received Interrupt
 */
#define CY_U3P_UIB_LRTY                                     (1u << 2) /* <2:2> RW1S:RW1C:0:No */


/*
   LBAD Received Interrupt
 */
#define CY_U3P_UIB_LBAD                                     (1u << 3) /* <3:3> RW1S:RW1C:0:No */


/*
   LCRD Recevied Interrupt
 */
#define CY_U3P_UIB_LCRD                                     (1u << 4) /* <4:4> RW1S:RW1C:0:No */


/*
   LGO_U1 Received Interrupt
 */
#define CY_U3P_UIB_LGO_U1                                   (1u << 5) /* <5:5> RW1S:RW1C:0:No */


/*
   LGO_U2 Received Interrupt
 */
#define CY_U3P_UIB_LGO_U2                                   (1u << 6) /* <6:6> RW1S:RW1C:0:No */


/*
   LGO_U3 Received Interrupt
 */
#define CY_U3P_UIB_LGO_U3                                   (1u << 7) /* <7:7> RW1S:RW1C:0:No */


/*
   LAU Received Interrupt
 */
#define CY_U3P_UIB_LAU                                      (1u << 8) /* <8:8> RW1S:RW1C:0:No */


/*
   LXU Received Interrupt
 */
#define CY_U3P_UIB_LXU                                      (1u << 9) /* <9:9> RW1S:RW1C:0:No */


/*
   LPMA Received Interrupt
 */
#define CY_U3P_UIB_LPMA                                     (1u << 10) /* <10:10> RW1S:RW1C:0:No */


/*
   Illegal LCW received (see LNK_CONTROL_WORD for details)
 */
#define CY_U3P_UIB_BAD_LCW                                  (1u << 11) /* <11:11> RW1S:RW1C:0:No */


/*
   Link Error Count Threshold Reached
 */
#define CY_U3P_UIB_LINK_ERROR                               (1u << 12) /* <12:12> RW1S:RW1C:0:No */


/*
   PHY Error Count Threshold Reached
 */
#define CY_U3P_UIB_PHY_ERROR                                (1u << 13) /* <13:13> RW1S:RW1C:0:No */


/*
   U2 Inactivity Timeout Interrupt
 */
#define CY_U3P_UIB_U2_INACTIVITY_TIMEOUT                    (1u << 14) /* <14:14> RW1S:RW1C:0:No */


/*
   LTSSM Transition to Polling — indicating successful SuperSpeed far-end
   receiver termination detection
 */
#define CY_U3P_UIB_LTSSM_CONNECT                            (1u << 15) /* <15:15> RW1S:RW1C:0:No */


/*
   LTSSM Transitions to SS.Disabled
 */
#define CY_U3P_UIB_LTSSM_DISCONNECT                         (1u << 16) /* <16:16> RW1S:RW1C:0:No */


/*
   LTSSM Reset Received (Hot or Warm)
 */
#define CY_U3P_UIB_LTSSM_RESET                              (1u << 17) /* <17:17> RW1S:RW1C:0:No */



/*
   Link Interrupt Mask Register
 */
#define CY_U3P_UIB_LNK_INTR_MASK_ADDRESS                    (0xe0033008)
#define CY_U3P_UIB_LNK_INTR_MASK                            (*(uvint32_t *)(0xe0033008))
#define CY_U3P_UIB_LNK_INTR_MASK_DEFAULT                    (0x00000000)

/*
   LTSSM State Change Interrupt
 */
#define CY_U3P_UIB_LTSSM_STATE_CHG                          (1u << 0) /* <0:0> R:RW:0:N/A */


/*
   LGOOD Received Interrupt
 */
#define CY_U3P_UIB_LGOOD                                    (1u << 1) /* <1:1> R:RW:0:N/A */


/*
   LRTY Received Interrupt
 */
#define CY_U3P_UIB_LRTY                                     (1u << 2) /* <2:2> R:RW:0:N/A */


/*
   LBAD Received Interrupt
 */
#define CY_U3P_UIB_LBAD                                     (1u << 3) /* <3:3> R:RW:0:N/A */


/*
   LCRD Recevied Interrupt
 */
#define CY_U3P_UIB_LCRD                                     (1u << 4) /* <4:4> R:RW:0:N/A */


/*
   LGO_U1 Received Interrupt
 */
#define CY_U3P_UIB_LGO_U1                                   (1u << 5) /* <5:5> R:RW:0:N/A */


/*
   LGO_U2 Received Interrupt
 */
#define CY_U3P_UIB_LGO_U2                                   (1u << 6) /* <6:6> R:RW:0:N/A */


/*
   LGO_U3 Received Interrupt
 */
#define CY_U3P_UIB_LGO_U3                                   (1u << 7) /* <7:7> R:RW:0:N/A */


/*
   LAU Received Interrupt
 */
#define CY_U3P_UIB_LAU                                      (1u << 8) /* <8:8> R:RW:0:N/A */


/*
   LXU Received Interrupt
 */
#define CY_U3P_UIB_LXU                                      (1u << 9) /* <9:9> R:RW:0:N/A */


/*
   LPMA Received Interrupt
 */
#define CY_U3P_UIB_LPMA                                     (1u << 10) /* <10:10> R:RW:0:N/A */


/*
   Illegal LCW received (see LNK_CONTROL_WORD for details)
 */
#define CY_U3P_UIB_BAD_LCW                                  (1u << 11) /* <11:11> R:RW:0:N/A */


/*
   Link Error Count Threshold Reached
 */
#define CY_U3P_UIB_LINK_ERROR                               (1u << 12) /* <12:12> R:RW:0:N/A */


/*
   PHY Error Count Threshold Reached
 */
#define CY_U3P_UIB_PHY_ERROR                                (1u << 13) /* <13:13> R:RW:0:N/A */


/*
   U2 Inactivity Timeout Interrupt
 */
#define CY_U3P_UIB_U2_INACTIVITY_TIMEOUT                    (1u << 14) /* <14:14> R:RW:0:N/A */


/*
   LTSSM Transitions from to Polling (Polling.Idle) to U0
 */
#define CY_U3P_UIB_LTSSM_CONNECT                            (1u << 15) /* <15:15> R:RW:0:N/A */


/*
   LTSSM Transitions to SS.Disabled
 */
#define CY_U3P_UIB_LTSSM_DISCONNECT                         (1u << 16) /* <16:16> R:RW:0:N/A */


/*
   LTSSM Reset Received (Hot or Warm)
 */
#define CY_U3P_UIB_LTSSM_RESET                              (1u << 17) /* <17:17> R:RW:0:N/A */



/*
   Link Error Counter Configuration
 */
#define CY_U3P_UIB_LNK_ERROR_CONF_ADDRESS                   (0xe003300c)
#define CY_U3P_UIB_LNK_ERROR_CONF                           (*(uvint32_t *)(0xe003300c))
#define CY_U3P_UIB_LNK_ERROR_CONF_DEFAULT                   (0x00007fff)

/*
   PENDING_HP_TIMER Timeout Count Enable
   Header Packet acknowledgement has not been received by PENDING_HP_TIMEOUT.
   [USB 3.0:  §7.2.4.1.10, p 7-21]
 */
#define CY_U3P_UIB_HP_TIMEOUT_EN                            (1u << 0) /* <0:0> R:RW:1:No */


/*
   Rx Header Sequence Number Error Count Enable
   Received Rx Header Sequence Number does not match what is expected.
   [USB 3.0:  §7.3.3.3, p 7-28]
 */
#define CY_U3P_UIB_RX_SEQ_NUM_ERR_EN                        (1u << 1) /* <1:1> R:RW:1:No */


/*
   Receive Header Packet Fail Count Enable
   Link Layer Block has failed to receive a Header Packet for three consecutive
   times.  Failures are CRC errors or spurious K-symbols.
   [USB 3.0:  §7.3.3.2, p 7-28]
 */
#define CY_U3P_UIB_RX_HP_FAIL_EN                            (1u << 2) /* <2:2> R:RW:1:No */


/*
   Missing LGOOD_n Detection Count Enable
   LGOOD_n Sequence Number does not match what is expected.
   [USB 3.0:  §7.3.4, p 7-29]
 */
#define CY_U3P_UIB_MISSING_LGOOD_EN                         (1u << 3) /* <3:3> R:RW:1:No */


/*
   Missing LCRD_x Detection Count Enable
   LCRD_x Sequence does not match what is expected.
   [USB 3.0:  §7.3.4, p 7-29]
 */
#define CY_U3P_UIB_MISSING_LCRD_EN                          (1u << 4) /* <4:4> R:RW:1:No */


/*
   CREDIT_HP_TIMER Timeout Count Enable
   Remote Rx Header Buffer Credit has not been received by CREDIT_HP_TIMEOUT.
   [USB 3.0:  §7.2.4.1.10, p 7-21…7-22]
 */
#define CY_U3P_UIB_CREDIT_HP_TIMEOUT_EN                     (1u << 5) /* <5:5> R:RW:1:No */


/*
   PM_LC_TIMER Timeout Count Enable
   This indicates that an LGO_Ux, LAU, or LXU Link Command has been missed.
   [USB 3.0:  §7.3.4, p 7-29]
 */
#define CY_U3P_UIB_PM_LC_TIMEOUT_EN                         (1u << 6) /* <6:6> R:RW:1:No */


/*
   ACK Tx Header Sequence Number Error Count Enable
   Received LGOOD_n does not match ACK Tx Header Sequence Number.
   [USB 3.0: §7.3.5, p 7-30]
 */
#define CY_U3P_UIB_TX_SEQ_NUM_ERR_EN                        (1u << 7) /* <7:7> R:RW:1:No */


/*
   Header Sequence Number Advertisement PENDING_HP_TIMER Timeout Count Enable
   PENDING_HP_TIMER timeout before receipt of Header Sequence Number LGOOD_n
   Link Command
   [USB 3.0:  §7.3.6, p 7-30]
 */
#define CY_U3P_UIB_HDR_ADV_TIMEOUT_EN                       (1u << 8) /* <8:8> R:RW:1:No */


/*
   Header Sequence Number Advertisement HP Received Error Count Enable
   Header Packet received during Header Sequence Number Advertisement
   [USB 3.0:  §7.3.6, p 7-30]
 */
#define CY_U3P_UIB_HDR_ADV_HP_EN                            (1u << 9) /* <9:9> R:RW:1:No */


/*
   Header Sequence Number Advertisement LCRD_x Received Error Count Enable
   LCRD_x Link Command received during Header Sequence Number Advertisement
   [USB 3.0:   §7.3.6, p 7-30]
 */
#define CY_U3P_UIB_HDR_ADV_LCRD_EN                          (1u << 10) /* <10:10> R:RW:1:No */


/*
   Header Sequence Number Advertisement LGO_Ux Received Error Count Enable
   LGO_Ux Link Command received during Header Sequence Number Advertisement
   [USB 3.0:  §7.3.6, p 7-30]
 */
#define CY_U3P_UIB_HDR_ADV_LGO_EN                           (1u << 11) /* <11:11> R:RW:1:No */


/*
   Rx Header Buffer Credit Advertisement CREDIT_HP_TIMER Timeout Count Enable
   CREDIT_HP_TIMER timeout before receipt of LCRD_x Link Command during Rx
   Header Buffer Credit Advertisement
   [USB 3.0:  §7.3.7, p 7-30…7-31]
 */
#define CY_U3P_UIB_CREDIT_ADV_TIMEOUT_EN                    (1u << 12) /* <12:12> R:RW:1:No */


/*
   Rx Header Buffer Credit Advertisement HP Received Error Count Enable
   Header Packet received during Rx Header Buffer Credit Advertisement.
   [USB 3.0:  §7.3.7, p 7-30…7-31]
 */
#define CY_U3P_UIB_CREDIT_ADV_HP_EN                         (1u << 13) /* <13:13> R:RW:1:No */


/*
   Rx Header Buffer Credit Advertisement LGO_Ux Received Error Count Enable
   LGO_Ux Link Command received during Rx Header Buffer Credit Advertisement.
   [USB 3.0:  §7.3.7, p 7-30…7-31]
 */
#define CY_U3P_UIB_CREDIT_ADV_LGO_EN                        (1u << 14) /* <14:14> R:RW:1:No */



/*
   Link Error Status Register
 */
#define CY_U3P_UIB_LNK_ERROR_STATUS_ADDRESS                 (0xe0033010)
#define CY_U3P_UIB_LNK_ERROR_STATUS                         (*(uvint32_t *)(0xe0033010))
#define CY_U3P_UIB_LNK_ERROR_STATUS_DEFAULT                 (0x00000000)

/*
   Indicates this error (see LNK_ERROR_CONF for description) occurred since
   this bit was last cleared by firmware.
 */
#define CY_U3P_UIB_HP_TIMEOUT_EV                            (1u << 0) /* <0:0> RW1S:RW1C:0:No */


/*
   Indicates this error (see LNK_ERROR_CONF for description) occurred since
   this bit was last cleared by firmware.
 */
#define CY_U3P_UIB_RX_SEQ_NUM_ERR_EV                        (1u << 1) /* <1:1> RW1S:RW1C:0:No */


/*
   Indicates this error (see LNK_ERROR_CONF for description) occurred since
   this bit was last cleared by firmware.
 */
#define CY_U3P_UIB_RX_HP_FAIL_EV                            (1u << 2) /* <2:2> RW1S:RW1C:0:No */


/*
   Indicates this error (see LNK_ERROR_CONF for description) occurred since
   this bit was last cleared by firmware.
 */
#define CY_U3P_UIB_MISSING_LGOOD_EV                         (1u << 3) /* <3:3> RW1S:RW1C:0:No */


/*
   Indicates this error (see LNK_ERROR_CONF for description) occurred since
   this bit was last cleared by firmware.
 */
#define CY_U3P_UIB_MISSING_LCRD_EV                          (1u << 4) /* <4:4> RW1S:RW1C:0:No */


/*
   Indicates this error (see LNK_ERROR_CONF for description) occurred since
   this bit was last cleared by firmware.
 */
#define CY_U3P_UIB_CREDIT_HP_TIMEOUT_EV                     (1u << 5) /* <5:5> RW1S:RW1C:0:No */


/*
   Indicates this error (see LNK_ERROR_CONF for description) occurred since
   this bit was last cleared by firmware.
 */
#define CY_U3P_UIB_PM_LC_TIMEOUT_EV                         (1u << 6) /* <6:6> RW1S:RW1C:0:No */


/*
   Indicates this error (see LNK_ERROR_CONF for description) occurred since
   this bit was last cleared by firmware.
 */
#define CY_U3P_UIB_TX_SEQ_NUM_ERR_EV                        (1u << 7) /* <7:7> RW1S:RW1C:0:No */


/*
   Indicates this error (see LNK_ERROR_CONF for description) occurred since
   this bit was last cleared by firmware.
 */
#define CY_U3P_UIB_HDR_ADV_TIMEOUT_EV                       (1u << 8) /* <8:8> RW1S:RW1C:0:No */


/*
   Indicates this error (see LNK_ERROR_CONF for description) occurred since
   this bit was last cleared by firmware.
 */
#define CY_U3P_UIB_HDR_ADV_HP_EV                            (1u << 9) /* <9:9> RW1S:RW1C:0:No */


/*
   Indicates this error (see LNK_ERROR_CONF for description) occurred since
   this bit was last cleared by firmware.
 */
#define CY_U3P_UIB_HDR_ADV_LCRD_EV                          (1u << 10) /* <10:10> RW1S:RW1C:0:No */


/*
   Indicates this error (see LNK_ERROR_CONF for description) occurred since
   this bit was last cleared by firmware.
 */
#define CY_U3P_UIB_HDR_ADV_LGO_EV                           (1u << 11) /* <11:11> RW1S:RW1C:0:No */


/*
   Indicates this error (see LNK_ERROR_CONF for description) occurred since
   this bit was last cleared by firmware.
 */
#define CY_U3P_UIB_CREDIT_ADV_TIMEOUT_EV                    (1u << 12) /* <12:12> RW1S:RW1C:0:No */


/*
   Indicates this error (see LNK_ERROR_CONF for description) occurred since
   this bit was last cleared by firmware.
 */
#define CY_U3P_UIB_CREDIT_ADV_HP_EV                         (1u << 13) /* <13:13> RW1S:RW1C:0:No */


/*
   Indicates this error (see LNK_ERROR_CONF for description) occurred since
   this bit was last cleared by firmware.
 */
#define CY_U3P_UIB_CREDIT_ADV_LGO_EV                        (1u << 14) /* <14:14> RW1S:RW1C:0:No */



/*
   Error Counter Register
 */
#define CY_U3P_UIB_LNK_ERROR_COUNT_ADDRESS                  (0xe0033014)
#define CY_U3P_UIB_LNK_ERROR_COUNT                          (*(uvint32_t *)(0xe0033014))
#define CY_U3P_UIB_LNK_ERROR_COUNT_DEFAULT                  (0x00000000)

/*
   The Link Error Count keeps track of the number of errors for which the
   Link Layer Block had to transition to the Recovery State before resuming
   normal operation.  Each error class is enablable (default on) to allow
   for debugging purposes.
 */
#define CY_U3P_UIB_LINK_ERROR_COUNT_MASK                    (0x0000ffff) /* <0:15> RW:RW:0:No */
#define CY_U3P_UIB_LINK_ERROR_COUNT_POS                     (0)


/*
   Count of receive errors from the USB 3.0 PHY.  This is for debug purposes.
 */
#define CY_U3P_UIB_PHY_ERROR_COUNT_MASK                     (0xffff0000) /* <16:31> RW:RW:0:No */
#define CY_U3P_UIB_PHY_ERROR_COUNT_POS                      (16)



/*
   Error Count Thresholds
 */
#define CY_U3P_UIB_LNK_ERROR_COUNT_THRESHOLD_ADDRESS        (0xe0033018)
#define CY_U3P_UIB_LNK_ERROR_COUNT_THRESHOLD                (*(uvint32_t *)(0xe0033018))
#define CY_U3P_UIB_LNK_ERROR_COUNT_THRESHOLD_DEFAULT        (0x00000000)

/*
   Link Error Count Threshold for Interrupt Generation
 */
#define CY_U3P_UIB_LINK_ERROR_THRESHOLD_MASK                (0x0000ffff) /* <0:15> R:RW:0:No */
#define CY_U3P_UIB_LINK_ERROR_THRESHOLD_POS                 (0)


/*
   PHY Error Count Threshold for Interrupt Generation
 */
#define CY_U3P_UIB_PHY_ERROR_THRESHOLD_MASK                 (0xffff0000) /* <16:31> R:RW:0:No */
#define CY_U3P_UIB_PHY_ERROR_THRESHOLD_POS                  (16)



/*
   USB 3.0 PHY Configuration
 */
#define CY_U3P_UIB_LNK_PHY_CONF_ADDRESS                     (0xe003301c)
#define CY_U3P_UIB_LNK_PHY_CONF                             (*(uvint32_t *)(0xe003301c))
#define CY_U3P_UIB_LNK_PHY_CONF_DEFAULT                     (0x00202005)

/*
   PHY Operation Mode
       01 = USB Super Speed
 */
#define CY_U3P_UIB_PHY_MODE_MASK                            (0x00000003) /* <0:1> R:R:1:No */
#define CY_U3P_UIB_PHY_MODE_POS                             (0)


/*
   PHY Elasticity Buffer Operation Mode
       0 = half full
       1 = empty
 */
#define CY_U3P_UIB_ELASTICIY_BUFFER_MODE                    (1u << 2) /* <2:2> R:RW:1:No */


/*
   PHY TxDetectRx/Loopback Override
 */
#define CY_U3P_UIB_TXDETECTRX_LB_OVR                        (1u << 3) /* <3:3> R:RW:0:No */


/*
   PHY TxDetectRx/Loopback Override Value
 */
#define CY_U3P_UIB_TXDETECTRX_LB_OVR_VAL                    (1u << 4) /* <4:4> R:RW:0:No */


/*
   PHY TxElecIdle Override
 */
#define CY_U3P_UIB_TXELECIDLE_OVR                           (1u << 5) /* <5:5> R:RW:0:No */


/*
   PHY TxElecIdle Override Value
 */
#define CY_U3P_UIB_TXELECIDLE_OVR_VAL                       (1u << 6) /* <6:6> R:RW:0:No */


/*
   PHY TxCompliance Override
 */
#define CY_U3P_UIB_TXCOMPLIANCE_OVR                         (1u << 7) /* <7:7> R:R:0:No */


/*
   PHY TxCompliance Override Value
 */
#define CY_U3P_UIB_TXCOMPLIANCE_OVR_VAL                     (1u << 8) /* <8:8> R:R:0:No */


/*
   PHY TxOnesZeros Override
 */
#define CY_U3P_UIB_TXONESZEROS_OVR                          (1u << 9) /* <9:9> R:RW:0:No */


/*
   PHY TxOnesZeros Override Value
 */
#define CY_U3P_UIB_TXONESZEROS_OVR_VAL                      (1u << 10) /* <10:10> R:RW:0:No */


/*
   PHY RxPolarity Override
 */
#define CY_U3P_UIB_RXPOLARITY_OVR                           (1u << 11) /* <11:11> R:RW:0:No */


/*
   PHY RxPolarity Override Value
 */
#define CY_U3P_UIB_RXPOLARITY_OVR_VAL                       (1u << 12) /* <12:12> R:RW:0:No */


/*
   PHY RxEqTraining Override
 */
#define CY_U3P_UIB_RXEQ_TRAINING_OVR                        (1u << 13) /* <13:13> R:RW:1:No */


/*
   PHY RxEqTraining Override Value
 */
#define CY_U3P_UIB_RXEQ_TRAINING_OVR_VAL                    (1u << 14) /* <14:14> R:RW:0:No */


/*
   PHY PIPE RESET# Override
 */
#define CY_U3P_UIB_PHY_RESET_N_OVR                          (1u << 15) /* <15:15> R:RW:0:No */


/*
   PHY PIPE RESET# Override Value
 */
#define CY_U3P_UIB_PHY_RESET_N_OVR_VAL                      (1u << 16) /* <16:16> R:RW:0:No */


/*
   PHY PowerDown Override
 */
#define CY_U3P_UIB_PHY_POWERDOWN_OVR                        (1u << 17) /* <17:17> R:RW:0:No */


/*
   PHY PowerDown Override Value
 */
#define CY_U3P_UIB_PHY_POWERDOWN_OVR_VAL_MASK               (0x000c0000) /* <18:19> R:RW:0:No */
#define CY_U3P_UIB_PHY_POWERDOWN_OVR_VAL_POS                (18)


/*
   PHY Rate Override
 */
#define CY_U3P_UIB_PHY_RATE_OVR                             (1u << 20) /* <20:20> R:R:0:No */


/*
   PHY Rate Override Value
 */
#define CY_U3P_UIB_PHY_RATE_OVR_VAL                         (1u << 21) /* <21:21> R:R:1:No */


/*
   PHY Transmitter De-emphasis Override
 */
#define CY_U3P_UIB_PHY_TX_DEEMPH_OVR                        (1u << 22) /* <22:22> R:RW:0:No */


/*
   PHY Transmitter De-emphasis Override Value
 */
#define CY_U3P_UIB_PHY_TX_DEEMPH_OVR_VAL_MASK               (0x01800000) /* <23:24> R:RW:0:No */
#define CY_U3P_UIB_PHY_TX_DEEMPH_OVR_VAL_POS                (23)


/*
   PHY Transmitter Voltage Levels
 */
#define CY_U3P_UIB_PHY_TX_MARGIN_MASK                       (0x0e000000) /* <25:27> R:RW:0:No */
#define CY_U3P_UIB_PHY_TX_MARGIN_POS                        (25)


/*
   PHY Transmitter Voltage Swing Level 0=full swing 1=low swing
 */
#define CY_U3P_UIB_TXSWING                                  (1u << 28) /* <28:28> R:RW:0:No */


/*
   PHY Receiver Termination Override
 */
#define CY_U3P_UIB_RX_TERMINATION_OVR                       (1u << 29) /* <29:29> R:RW:0:No */


/*
   PHY Receiver Termination Override Value 0=removed 1=present
 */
#define CY_U3P_UIB_RX_TERMINATION_OVR_VAL                   (1u << 30) /* <30:30> R:RW:0:No */


/*
   PHY Receiver Termination Enable
 */
#define CY_U3P_UIB_RX_TERMINATION_ENABLE                    (1u << 31) /* <31:31> R:RW:0:No */



/*
   USB 3.0 PHY MPLL Status
 */
#define CY_U3P_UIB_LNK_PHY_MPLL_STATUS_ADDRESS              (0xe003302c)
#define CY_U3P_UIB_LNK_PHY_MPLL_STATUS                      (*(uvint32_t *)(0xe003302c))
#define CY_U3P_UIB_LNK_PHY_MPLL_STATUS_DEFAULT              (0x00910400)

/*
   MPLL Frequency Multiplier Control
   Default values (based onclock crystal frequency):
   19.2MHz ? 0x02
   26MHz ? 0x60
   38.4MHz ? 0x41
   52MHz ? 0x30
 */
#define CY_U3P_UIB_MPLL_MULTIPLIER_MASK                     (0x000003f8) /* <3:9> RW:R:H:No */
#define CY_U3P_UIB_MPLL_MULTIPLIER_POS                      (3)


/*
   Spread Spectrum Enable
 */
#define CY_U3P_UIB_SSC_EN                                   (1u << 10) /* <10:10> R:RW:1:No */


/*
   Spread Spectrum Clock Range
 */
#define CY_U3P_UIB_SSC_RANGE_MASK                           (0x00001800) /* <11:12> R:RW:0:No */
#define CY_U3P_UIB_SSC_RANGE_POS                            (11)


/*
   Spread Spectrum Reference Clock Shifting
 */
#define CY_U3P_UIB_SSC_REF_CLK_SEL_MASK                     (0x001fe000) /* <13:20> RW:R:0x88:No */
#define CY_U3P_UIB_SSC_REF_CLK_SEL_POS                      (13)


/*
   USB 3.0 PHY Reference Clock Enable for SSP PHY
   Enables the reference clock to the PHY prescalar.  This signal must remain
   de-asserted until the reference clock is stable.
 */
#define CY_U3P_UIB_REF_SSP_EN                               (1u << 21) /* <21:21> R:RW:0:No */


/*
   USB 3.0 PHY Input Reference Clock Divider Control
 */
#define CY_U3P_UIB_REF_CLKDIV2                              (1u << 22) /* <22:22> RW:R:0:No */


/*
   PHY output signal ref_clkreq_n
   Note: this signal is not available on FX3 and will always return 1.
 */
#define CY_U3P_UIB_REF_CLKREQ_N                             (1u << 23) /* <23:23> RW:R:1:No */



/*
   USB 3.0 PHY Transmitter Config
 */
#define CY_U3P_UIB_LNK_PHY_TX_TRIM_ADDRESS                  (0xe003303c)
#define CY_U3P_UIB_LNK_PHY_TX_TRIM                          (*(uvint32_t *)(0xe003303c))
#define CY_U3P_UIB_LNK_PHY_TX_TRIM_DEFAULT                  (0x0d3a5015)

/*
   TX de-emphasis at 3.5dB
 */
#define CY_U3P_UIB_PCS_TX_DEEMPH_3P5DB_MASK                 (0x0000003f) /* <0:5> R:RW:21:No */
#define CY_U3P_UIB_PCS_TX_DEEMPH_3P5DB_POS                  (0)


/*
   TX de-emphasis at 6dB
 */
#define CY_U3P_UIB_PCS_TX_DEEMPH_6DB_MASK                   (0x00001f80) /* <7:12> R:RW:32:No */
#define CY_U3P_UIB_PCS_TX_DEEMPH_6DB_POS                    (7)


/*
   TX Amplitude (Full Swing Mode)
 */
#define CY_U3P_UIB_PCS_TX_SWING_FULL_MASK                   (0x001fc000) /* <14:20> R:RW:105:No */
#define CY_U3P_UIB_PCS_TX_SWING_FULL_POS                    (14)


/*
   TX Amplitude (Low Swing Mode)
 */
#define CY_U3P_UIB_PCS_TX_SWING_LOW_MASK                    (0x0fe00000) /* <21:27> R:RW:105:No */
#define CY_U3P_UIB_PCS_TX_SWING_LOW_POS                     (21)



/*
   PHY Error Counter Configuration
 */
#define CY_U3P_UIB_LNK_PHY_ERROR_CONF_ADDRESS               (0xe0033040)
#define CY_U3P_UIB_LNK_PHY_ERROR_CONF                       (*(uvint32_t *)(0xe0033040))
#define CY_U3P_UIB_LNK_PHY_ERROR_CONF_DEFAULT               (0x000001ff)

/*
   Enable Counting of 8b/10b Decode Errors
       (RxStatus == 3'b100)
 */
#define CY_U3P_UIB_PHY_ERROR_DECODE_EN                      (1u << 0) /* <0:0> R:RW:1:No */


/*
   Enable Counting of Elastic Buffer Overflow
       (RxStatus == 3'b101)
 */
#define CY_U3P_UIB_PHY_ERROR_EB_OVR_EN                      (1u << 1) /* <1:1> R:RW:1:No */


/*
   Enable Counting of Elastic Buffer Underflow
       (RxStatus == 3'b110)
 */
#define CY_U3P_UIB_PHY_ERROR_EB_UND_EN                      (1u << 2) /* <2:2> R:RW:1:No */


/*
   Enable Counting of Receive Disparity Error
       (RxStatus == 3'b111)
 */
#define CY_U3P_UIB_PHY_ERROR_DISPARITY_EN                   (1u << 3) /* <3:3> R:RW:1:No */


/*
   Enable Counting of Receive CRC-5 Error
 */
#define CY_U3P_UIB_RX_ERROR_CRC5_EN                         (1u << 4) /* <4:4> R:RW:1:No */


/*
   Enable Counting of Receive CRC-16 Error
 */
#define CY_U3P_UIB_RX_ERROR_CRC16_EN                        (1u << 5) /* <5:5> R:RW:1:No */


/*
   Enable Counting of Receive CRC-32 Error
 */
#define CY_U3P_UIB_RX_ERROR_CRC32_EN                        (1u << 6) /* <6:6> R:RW:1:No */


/*
   Enable Counting of Training Sequence Error
 */
#define CY_U3P_UIB_TRAINING_ERROR_EN                        (1u << 7) /* <7:7> R:RW:1:No */


/*
   Enable Counting of PHY Lock Loss
      Lock Indicator To Be Determined
 */
#define CY_U3P_UIB_PHY_LOCK_EN                              (1u << 8) /* <8:8> R:RW:1:No */



/*
   PHY Error Status Register
 */
#define CY_U3P_UIB_LNK_PHY_ERROR_STATUS_ADDRESS             (0xe0033044)
#define CY_U3P_UIB_LNK_PHY_ERROR_STATUS                     (*(uvint32_t *)(0xe0033044))
#define CY_U3P_UIB_LNK_PHY_ERROR_STATUS_DEFAULT             (0x00000000)

/*
   Indicates this error (see LNK_ERROR_CONF for description) occurred since
   this bit was last cleared by firmware.
 */
#define CY_U3P_UIB_PHY_ERROR_DECODE_EV                      (1u << 0) /* <0:0> RW1S:RW1C:0:No */


/*
   Indicates this error (see LNK_ERROR_CONF for description) occurred since
   this bit was last cleared by firmware.
 */
#define CY_U3P_UIB_PHY_ERROR_EB_OVR_EV                      (1u << 1) /* <1:1> RW1S:RW1C:0:No */


/*
   Indicates this error (see LNK_ERROR_CONF for description) occurred since
   this bit was last cleared by firmware.
 */
#define CY_U3P_UIB_PHY_ERROR_EB_UND_EV                      (1u << 2) /* <2:2> RW1S:RW1C:0:No */


/*
   Indicates this error (see LNK_ERROR_CONF for description) occurred since
   this bit was last cleared by firmware.
 */
#define CY_U3P_UIB_PHY_ERROR_DISPARITY_EV                   (1u << 3) /* <3:3> RW1S:RW1C:0:No */


/*
   Indicates this error (see LNK_ERROR_CONF for description) occurred since
   this bit was last cleared by firmware.
 */
#define CY_U3P_UIB_RX_ERROR_CRC5_EV                         (1u << 4) /* <4:4> RW1S:RW1C:0:No */


/*
   Indicates this error (see LNK_ERROR_CONF for description) occurred since
   this bit was last cleared by firmware.
 */
#define CY_U3P_UIB_RX_ERROR_CRC16_EV                        (1u << 5) /* <5:5> RW1S:RW1C:0:No */


/*
   Indicates this error (see LNK_ERROR_CONF for description) occurred since
   this bit was last cleared by firmware.
 */
#define CY_U3P_UIB_RX_ERROR_CRC32_EV                        (1u << 6) /* <6:6> RW1S:RW1C:0:No */


/*
   Indicates this error (see LNK_ERROR_CONF for description) occurred since
   this bit was last cleared by firmware.
 */
#define CY_U3P_UIB_TRAINING_ERROR_EV                        (1u << 7) /* <7:7> RW1S:RW1C:0:No */


/*
   Indicates this error (see LNK_ERROR_CONF for description) occurred since
   this bit was last cleared by firmware.
 */
#define CY_U3P_UIB_PHY_LOCK_EV                              (1u << 8) /* <8:8> RW1S:RW1C:0:No */



/*
   USB 3.0 Device Power State Control
 */
#define CY_U3P_UIB_LNK_DEVICE_POWER_CONTROL_ADDRESS         (0xe0033050)
#define CY_U3P_UIB_LNK_DEVICE_POWER_CONTROL                 (*(uvint32_t *)(0xe0033050))
#define CY_U3P_UIB_LNK_DEVICE_POWER_CONTROL_DEFAULT         (0x00000000)

/*
   Transmit LGO_U1 - Request to go to U1 Power State (send LGO_U1)
   This bit is cleared by h/w when the LCW is transmitted.
 */
#define CY_U3P_UIB_TX_U1                                    (1u << 0) /* <0:0> RW0C:RW1S:0:No */


/*
   Transmit LGO_U2 - Request to go to U2 Power State (send LGO_U2)
   This bit is cleared by h/w when the LCW is transmitted.
 */
#define CY_U3P_UIB_TX_U2                                    (1u << 1) /* <1:1> RW0C:RW1S:0:No */


/*
   Transmit LGO_U3 - Request to go to U3 Power State (send LGO_U3)
   This bit is cleared by h/w when the LCW is transmitted.
   Note that an upstream port is not allowed to initiate entry to U3, so
   this should not be used for device mode.
   [USB 3.0:  §7.2.4.2.4, p 7-25]
 */
#define CY_U3P_UIB_TX_U3                                    (1u << 2) /* <2:2> RW0C:RW1S:0:No */


/*
   LGO_U1 Received - Request to go to U1 Power State
   This bit is cleared by h/w concurrent with TX_LAU/TX_LXU being cleared.
 */
#define CY_U3P_UIB_RX_U1                                    (1u << 4) /* <4:4> RW:R:0:No */


/*
   LGO_U2 Received - Request to go to U2 Power State, clear to NAK (send
   LXU)
   This bit is cleared by h/w concurrent with TX_LAU/TX_LXU being cleared.
 */
#define CY_U3P_UIB_RX_U2                                    (1u << 5) /* <5:5> RW:R:0:No */


/*
   LGO_U3 Received - Request to go to U3 Power State, clear to NAK (send
   LXU)
   This bit is cleared by h/w concurrent with TX_LAU/TX_LXU being cleared.
   Note that an upstream port is not allowed to reject entry to U3.
   [USB 3.0:  §7.2.4.2.4, p 7-25]
 */
#define CY_U3P_UIB_RX_U3                                    (1u << 6) /* <6:6> RW:R:0:No */


/*
   Transmit LAU (ACK) in response to RX_U1/RX_U2/RX_U3.
   Transition to requested power state (LTSSM)
   This bit is cleared when the acknowledgement is sent.
 */
#define CY_U3P_UIB_TX_LAU                                   (1u << 7) /* <7:7> RW0C:RW1S:0:No */


/*
   Transmit LXU (NAK) in response to RX_U1/RX_U2/RX_U3.
   Do not transition to requested power state (LTSSM)
   This bit is cleared when the acknowledgement is sent.
 */
#define CY_U3P_UIB_TX_LXU                                   (1u << 8) /* <8:8> RW0C:RW1S:0:No */


/*
   Exit Low Power State
   This bit is cleared by h/w when the Link Layer has exited U1/U2/U3.
 */
#define CY_U3P_UIB_EXIT_LP                                  (1u << 9) /* <9:9> RW0C:RW1S:0:No */


/*
   When host requests transition to U1, automatically accept (send LAU) or
   rejects (send LXU) depending on pending activity.  The interrupt RX_U1
   is still raised for firmware to monitor, take additional power saving
   actions.
 */
#define CY_U3P_UIB_AUTO_U1                                  (1u << 26) /* <26:26> R:RW:0:No */


/*
   When host requests transition to U2, automatically accept (send LAU) or
   rejects (send LXU) depending on pending activity. The interrupt RX_U2
   is still raised for firmware to monitor, take additional power saving
   actions.
 */
#define CY_U3P_UIB_AUTO_U2                                  (1u << 27) /* <27:27> R:RW:0:No */


/*
   When host requests transition to U1, automatically reject (send LXU).
   The interrupt RX_U1 is still raised for firmware to monitor, take additional
   actions.
   This bit must be cleared by firmware when FORCE_PM_ACCEPT is received
   from host.
 */
#define CY_U3P_UIB_NO_U1                                    (1u << 28) /* <28:28> R:RW:0:No */


/*
   When host requests transition to U2, automatically reject (send LXU).
   The interrupt RX_U2 is still raised for firmware to monitor, take additional
   actions. This bit must be cleared by firmware when FORCE_PM_ACCEPT is
   received from host.
 */
#define CY_U3P_UIB_NO_U2                                    (1u << 29) /* <29:29> R:RW:0:No */


/*
   When host requests transition to U1, automatically accept (send LAU).
   The interrupt RX_U1 is still raised for firmware to monitor, take additional
   power saving actions.
 */
#define CY_U3P_UIB_YES_U1                                   (1u << 30) /* <30:30> R:RW:0:No */


/*
   When host requests transition to U2, automatically accept (send LAU).
   The interrupt RX_U2 is still raised for firmware to monitor, take additional
   power saving actions.
 */
#define CY_U3P_UIB_YES_U2                                   (1u << 31) /* <31:31> R:RW:0:No */



/*
   Link Training Status State Machine (LTSSM) State
 */
#define CY_U3P_UIB_LNK_LTSSM_STATE_ADDRESS                  (0xe0033054)
#define CY_U3P_UIB_LNK_LTSSM_STATE                          (*(uvint32_t *)(0xe0033054))
#define CY_U3P_UIB_LNK_LTSSM_STATE_DEFAULT                  (0x00000000)

/*
   LTSSM State
   See USB3LNK_LTSSM Tab for more details.
 */
#define CY_U3P_UIB_LTSSM_STATE_MASK                         (0x0000003f) /* <0:5> RW:R:0:No */
#define CY_U3P_UIB_LTSSM_STATE_POS                          (0)


/*
   LTSSM State from FW (if LTSSM_OVERRIDE_ENABLE == 1 or LTSSM_OVERRIDE_GO
   == 1)
 */
#define CY_U3P_UIB_LTSSM_OVERRIDE_VALUE_MASK                (0x00000fc0) /* <6:11> R:RW:0:No */
#define CY_U3P_UIB_LTSSM_OVERRIDE_VALUE_POS                 (6)


/*
   FW Control of LTSSM State
   Setting this bit will cause the LTSSM State Machine to transition to the
   state in LTSSM_OVERRIDE_VALUE and remain there.
 */
#define CY_U3P_UIB_LTSSM_OVERRIDE_EN                        (1u << 12) /* <12:12> R:RW:0:No */


/*
   FW Setting of LTSSM State
   Setting this bit will cause the LTSSM State Machine to transition to the
   value given in LTSSM_OVERRIDE_VALUE, but the state machine will not be
   held there and may transition to other states as dictated by the logic
 */
#define CY_U3P_UIB_LTSSM_OVERRIDE_GO                        (1u << 13) /* <13:13> RW0C:RW1S:0:No */


/*
   Loopback Master Enable
   When transmitting TS2 ordered sets in Polling or Recovery State, the Link
   Layer Block will enter the Loopback State as the Loopback Master if this
   bit is set.  The Link Layer Block will then exit the Loopback State when
   this bit is cleared.
   [USB 3.0:  §7.5.4.6.1, p 7-45;  §7.5.10.5.2, p 7-55]
 */
#define CY_U3P_UIB_LOOPBACK_MASTER                          (1u << 14) /* <14:14> R:RW:0:No */


/*
   Scrambling Disable
   When transmitting TS2 ordered sets in Polling or Recovery State, the Link
   Layer Block will set the Disable Scrambling bit.
   [USB 3.0:  §7.5.4.6.1, p 7-45]
 */
#define CY_U3P_UIB_DISABLE_SCRAMBLING                       (1u << 15) /* <15:15> R:RW:0:No */


/*
   Loopback Master Error Detected
 */
#define CY_U3P_UIB_LOOPBACK_ERROR                           (1u << 16) /* <16:16> RW:R:0:No */


/*
   Loopback Master Good
   Transmit sequence is being received correctly.
 */
#define CY_U3P_UIB_LOOPBACK_GOOD                            (1u << 17) /* <17:17> RW:R:0:No */


/*
   Freeze LTSSM to allow FW to inspect its current state.
   Setting this bit will cease all header packet transmission.  Incoming
   header packets will still be received and acknowledged but no more header
   packets will be accepted for transmission from the protocol layer.  It
   is expected that soon after this bit is set RX a queue will fill up and
   become stable for firmware to inspect.
 */
#define CY_U3P_UIB_LTSSM_FREEZE                             (1u << 31) /* <31:31> R:RW:0:No */



/*
   LFPS Receiver Observability
 */
#define CY_U3P_UIB_LNK_LFPS_OBSERVE_ADDRESS                 (0xe0033064)
#define CY_U3P_UIB_LNK_LFPS_OBSERVE                         (*(uvint32_t *)(0xe0033064))
#define CY_U3P_UIB_LNK_LFPS_OBSERVE_DEFAULT                 (0x00000000)

/*
   LFPS Sequence detected since last cleared by CPU
 */
#define CY_U3P_UIB_POLLING_DET                              (1u << 0) /* <0:0> RW1S:RW0C:0:No */


/*
   LFPS Sequence detected since last cleared by CPU
 */
#define CY_U3P_UIB_PING_DET                                 (1u << 1) /* <1:1> RW1S:RW0C:0:No */


/*
   LFPS Sequence detected since last cleared by CPU
 */
#define CY_U3P_UIB_RESET_DET                                (1u << 2) /* <2:2> RW1S:RW0C:0:No */


/*
   LFPS Sequence detected since last cleared by CPU
 */
#define CY_U3P_UIB_U1_EXIT_DET                              (1u << 3) /* <3:3> RW1S:RW0C:0:No */


/*
   LFPS Sequence detected since last cleared by CPU
 */
#define CY_U3P_UIB_U2_EXIT_DET                              (1u << 4) /* <4:4> RW1S:RW0C:0:No */


/*
   LFPS Sequence detected since last cleared by CPU
 */
#define CY_U3P_UIB_U3_EXIT_DET                              (1u << 5) /* <5:5> RW1S:RW0C:0:No */


/*
   LFPS Sequence detected since last cleared by CPU
 */
#define CY_U3P_UIB_LOOPBACK_DET                             (1u << 6) /* <6:6> RW1S:RW0C:0:No */


/*
   Number of LFPS Polling Bursts Received since last Polling.LFPS entry
 */
#define CY_U3P_UIB_POLLING_LFPS_RCVD_MASK                   (0x000f0000) /* <16:19> RW:R:0:No */
#define CY_U3P_UIB_POLLING_LFPS_RCVD_POS                    (16)


/*
   Number of LFPS Polling Bursts Sent since last Polling.LFPS entry
 */
#define CY_U3P_UIB_POLLING_LFPS_SENT_MASK                   (0x00f00000) /* <20:23> RW:R:0:No */
#define CY_U3P_UIB_POLLING_LFPS_SENT_POS                    (20)



/*
   Compliance Pattern CP0
 */
#define CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_0_ADDRESS         (0xe0033138)
#define CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_0                 (*(uvint32_t *)(0xe0033138))
#define CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_0_DEFAULT         (0x00000600)

/*
   Compliance Pattern
 */
#define CY_U3P_UIB_CP_MASK                                  (0x000000ff) /* <0:7> R:RW:0x00:No */
#define CY_U3P_UIB_CP_POS                                   (0)


/*
   Symbol Type - 0: Data (D), 1: Symbol (K)
 */
#define CY_U3P_UIB_K_D                                      (1u << 8) /* <8:8> R:RW:0:No */


/*
   Scramble On/Off
 */
#define CY_U3P_UIB_SCRAMBLED                                (1u << 9) /* <9:9> R:RW:1:No */


/*
   De-emphasis On/Off
 */
#define CY_U3P_UIB_DEEMPHASIS                               (1u << 10) /* <10:10> R:RW:1:No */


/*
   LFPS On/Off
 */
#define CY_U3P_UIB_LFPS                                     (1u << 11) /* <11:11> R:RW:0:No */


/*
   Enable TXONESZEROS (PIPE PHY Transmit Signal)
 */
#define CY_U3P_UIB_TXONESZEROS                              (1u << 12) /* <12:12> R:RW:0:No */



/*
   Compliance Pattern CP1
 */
#define CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_1_ADDRESS         (0xe003313c)
#define CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_1                 (*(uvint32_t *)(0xe003313c))
#define CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_1_DEFAULT         (0x0000044a)

/*
   Compliance Pattern
 */
#define CY_U3P_UIB_CP_MASK                                  (0x000000ff) /* <0:7> R:RW:0x4A:No */
#define CY_U3P_UIB_CP_POS                                   (0)


/*
   Symbol Type - 0: Data (D), 1: Symbol (K)
 */
#define CY_U3P_UIB_K_D                                      (1u << 8) /* <8:8> R:RW:0:No */


/*
   Scramble On/Off
 */
#define CY_U3P_UIB_SCRAMBLED                                (1u << 9) /* <9:9> R:RW:0:No */


/*
   De-emphasis On/Off
 */
#define CY_U3P_UIB_DEEMPHASIS                               (1u << 10) /* <10:10> R:RW:1:No */


/*
   LFPS On/Off
 */
#define CY_U3P_UIB_LFPS                                     (1u << 11) /* <11:11> R:RW:0:No */


/*
   Enable TXONESZEROS (PIPE PHY Transmit Signal)
 */
#define CY_U3P_UIB_TXONESZEROS                              (1u << 12) /* <12:12> R:RW:0:No */



/*
   Compliance Pattern CP2
 */
#define CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_2_ADDRESS         (0xe0033140)
#define CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_2                 (*(uvint32_t *)(0xe0033140))
#define CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_2_DEFAULT         (0x00000478)

/*
   Compliance Pattern
 */
#define CY_U3P_UIB_CP_MASK                                  (0x000000ff) /* <0:7> R:RW:0x78:No */
#define CY_U3P_UIB_CP_POS                                   (0)


/*
   Symbol Type - 0: Data (D), 1: Symbol (K)
 */
#define CY_U3P_UIB_K_D                                      (1u << 8) /* <8:8> R:RW:0:No */


/*
   Scramble On/Off
 */
#define CY_U3P_UIB_SCRAMBLED                                (1u << 9) /* <9:9> R:RW:0:No */


/*
   De-emphasis On/Off
 */
#define CY_U3P_UIB_DEEMPHASIS                               (1u << 10) /* <10:10> R:RW:1:No */


/*
   LFPS On/Off
 */
#define CY_U3P_UIB_LFPS                                     (1u << 11) /* <11:11> R:RW:0:No */


/*
   Enable TXONESZEROS (PIPE PHY Transmit Signal)
 */
#define CY_U3P_UIB_TXONESZEROS                              (1u << 12) /* <12:12> R:RW:0:No */



/*
   Compliance Pattern CP3
 */
#define CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_3_ADDRESS         (0xe0033144)
#define CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_3                 (*(uvint32_t *)(0xe0033144))
#define CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_3_DEFAULT         (0x000005bc)

/*
   Compliance Pattern
 */
#define CY_U3P_UIB_CP_MASK                                  (0x000000ff) /* <0:7> R:RW:0xBC:No */
#define CY_U3P_UIB_CP_POS                                   (0)


/*
   Symbol Type - 0: Data (D), 1: Symbol (K)
 */
#define CY_U3P_UIB_K_D                                      (1u << 8) /* <8:8> R:RW:1:No */


/*
   Scramble On/Off
 */
#define CY_U3P_UIB_SCRAMBLED                                (1u << 9) /* <9:9> R:RW:0:No */


/*
   De-emphasis On/Off
 */
#define CY_U3P_UIB_DEEMPHASIS                               (1u << 10) /* <10:10> R:RW:1:No */


/*
   LFPS On/Off
 */
#define CY_U3P_UIB_LFPS                                     (1u << 11) /* <11:11> R:RW:0:No */


/*
   Enable TXONESZEROS (PIPE PHY Transmit Signal)
 */
#define CY_U3P_UIB_TXONESZEROS                              (1u << 12) /* <12:12> R:RW:0:No */



/*
   Compliance Pattern CP4
 */
#define CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_4_ADDRESS         (0xe0033148)
#define CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_4                 (*(uvint32_t *)(0xe0033148))
#define CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_4_DEFAULT         (0x00000c00)

/*
   Compliance Pattern
 */
#define CY_U3P_UIB_CP_MASK                                  (0x000000ff) /* <0:7> R:RW:0x00:No */
#define CY_U3P_UIB_CP_POS                                   (0)


/*
   Symbol Type - 0: Data (D), 1: Symbol (K)
 */
#define CY_U3P_UIB_K_D                                      (1u << 8) /* <8:8> R:RW:0:No */


/*
   Scramble On/Off
 */
#define CY_U3P_UIB_SCRAMBLED                                (1u << 9) /* <9:9> R:RW:0:No */


/*
   De-emphasis On/Off
 */
#define CY_U3P_UIB_DEEMPHASIS                               (1u << 10) /* <10:10> R:RW:1:No */


/*
   LFPS On/Off
 */
#define CY_U3P_UIB_LFPS                                     (1u << 11) /* <11:11> R:RW:1:No */


/*
   Enable TXONESZEROS (PIPE PHY Transmit Signal)
 */
#define CY_U3P_UIB_TXONESZEROS                              (1u << 12) /* <12:12> R:RW:0:No */



/*
   Compliance Pattern CP5
 */
#define CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_5_ADDRESS         (0xe003314c)
#define CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_5                 (*(uvint32_t *)(0xe003314c))
#define CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_5_DEFAULT         (0x000005fc)

/*
   Compliance Pattern
 */
#define CY_U3P_UIB_CP_MASK                                  (0x000000ff) /* <0:7> R:RW:0xFC:No */
#define CY_U3P_UIB_CP_POS                                   (0)


/*
   Symbol Type - 0: Data (D), 1: Symbol (K)
 */
#define CY_U3P_UIB_K_D                                      (1u << 8) /* <8:8> R:RW:1:No */


/*
   Scramble On/Off
 */
#define CY_U3P_UIB_SCRAMBLED                                (1u << 9) /* <9:9> R:RW:0:No */


/*
   De-emphasis On/Off
 */
#define CY_U3P_UIB_DEEMPHASIS                               (1u << 10) /* <10:10> R:RW:1:No */


/*
   LFPS On/Off
 */
#define CY_U3P_UIB_LFPS                                     (1u << 11) /* <11:11> R:RW:0:No */


/*
   Enable TXONESZEROS (PIPE PHY Transmit Signal)
 */
#define CY_U3P_UIB_TXONESZEROS                              (1u << 12) /* <12:12> R:RW:0:No */



/*
   Compliance Pattern CP6
 */
#define CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_6_ADDRESS         (0xe0033150)
#define CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_6                 (*(uvint32_t *)(0xe0033150))
#define CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_6_DEFAULT         (0x000001fc)

/*
   Compliance Pattern
 */
#define CY_U3P_UIB_CP_MASK                                  (0x000000ff) /* <0:7> R:RW:0xFC:No */
#define CY_U3P_UIB_CP_POS                                   (0)


/*
   Symbol Type - 0: Data (D), 1: Symbol (K)
 */
#define CY_U3P_UIB_K_D                                      (1u << 8) /* <8:8> R:RW:1:No */


/*
   Scramble On/Off
 */
#define CY_U3P_UIB_SCRAMBLED                                (1u << 9) /* <9:9> R:RW:0:No */


/*
   De-emphasis On/Off
 */
#define CY_U3P_UIB_DEEMPHASIS                               (1u << 10) /* <10:10> R:RW:0:No */


/*
   LFPS On/Off
 */
#define CY_U3P_UIB_LFPS                                     (1u << 11) /* <11:11> R:RW:0:No */


/*
   Enable TXONESZEROS (PIPE PHY Transmit Signal)
 */
#define CY_U3P_UIB_TXONESZEROS                              (1u << 12) /* <12:12> R:RW:0:No */



/*
   Compliance Pattern CP7
 */
#define CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_7_ADDRESS         (0xe0033154)
#define CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_7                 (*(uvint32_t *)(0xe0033154))
#define CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_7_DEFAULT         (0x00001400)

/*
   Compliance Pattern
 */
#define CY_U3P_UIB_CP_MASK                                  (0x000000ff) /* <0:7> R:RW:0x00:No */
#define CY_U3P_UIB_CP_POS                                   (0)


/*
   Symbol Type - 0: Data (D), 1: Symbol (K)
 */
#define CY_U3P_UIB_K_D                                      (1u << 8) /* <8:8> R:RW:0:No */


/*
   Scramble On/Off
 */
#define CY_U3P_UIB_SCRAMBLED                                (1u << 9) /* <9:9> R:RW:0:No */


/*
   De-emphasis On/Off
 */
#define CY_U3P_UIB_DEEMPHASIS                               (1u << 10) /* <10:10> R:RW:1:No */


/*
   LFPS On/Off
 */
#define CY_U3P_UIB_LFPS                                     (1u << 11) /* <11:11> R:RW:0:No */


/*
   Enable TXONESZEROS (PIPE PHY Transmit Signal)
 */
#define CY_U3P_UIB_TXONESZEROS                              (1u << 12) /* <12:12> R:RW:1:No */



/*
   Compliance Pattern CP8
 */
#define CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_8_ADDRESS         (0xe0033158)
#define CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_8                 (*(uvint32_t *)(0xe0033158))
#define CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_8_DEFAULT         (0x00001000)

/*
   Compliance Pattern
 */
#define CY_U3P_UIB_CP_MASK                                  (0x000000ff) /* <0:7> R:RW:0x00:No */
#define CY_U3P_UIB_CP_POS                                   (0)


/*
   Symbol Type - 0: Data (D), 1: Symbol (K)
 */
#define CY_U3P_UIB_K_D                                      (1u << 8) /* <8:8> R:RW:0:No */


/*
   Scramble On/Off
 */
#define CY_U3P_UIB_SCRAMBLED                                (1u << 9) /* <9:9> R:RW:0:No */


/*
   De-emphasis On/Off
 */
#define CY_U3P_UIB_DEEMPHASIS                               (1u << 10) /* <10:10> R:RW:0:No */


/*
   LFPS On/Off
 */
#define CY_U3P_UIB_LFPS                                     (1u << 11) /* <11:11> R:RW:0:No */


/*
   Enable TXONESZEROS (PIPE PHY Transmit Signal)
 */
#define CY_U3P_UIB_TXONESZEROS                              (1u << 12) /* <12:12> R:RW:1:No */



#endif /* _INCLUDED_USB3LNK_REGS_H_ */

/*[]*/

