/****************************************************************************
 *
 * File: usb3prot_regs.h
 *
 * Copyright (c) 2010-13 Cypress Semiconductor. All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION WHICH IS THE PROPERTY OF CYPRESS.
 *
 * Description:
 *   USB 3.0 Protocol registers for the EZ-USB FX3 Device.
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

#ifndef _INCLUDED_USB3PROT_REGS_H_
#define _INCLUDED_USB3PROT_REGS_H_

#include <cyu3types.h>

#define USB3PROT_BASE_ADDR                       (0xe0033400)

typedef struct
{
    uvint32_t prot_cs;                            /* 0xe0033400 */
    uvint32_t prot_intr;                          /* 0xe0033404 */
    uvint32_t prot_intr_mask;                     /* 0xe0033408 */
    uvint32_t reserved0[3];
    uvint32_t prot_lmp_port_capability_timer;     /* 0xe0033418 */
    uvint32_t prot_lmp_port_configuration_timer;  /* 0xe003341c */
    uvint32_t reserved1[2];
    uvint32_t prot_framecnt;                      /* 0xe0033428 */
    uvint32_t reserved2;
    uvint32_t prot_itp_time;                      /* 0xe0033430 */
    uvint32_t prot_itp_timestamp;                 /* 0xe0033434 */
    uvint32_t prot_setupdat0;                     /* 0xe0033438 */
    uvint32_t prot_setupdat1;                     /* 0xe003343c */
    uvint32_t prot_seq_num;                       /* 0xe0033440 */
    uvint32_t reserved3[6];
    uvint32_t prot_lmp_received;                  /* 0xe003345c */
    uvint32_t reserved4[5];
    uvint32_t prot_ep_intr;                       /* 0xe0033474 */
    uvint32_t prot_ep_intr_mask;                  /* 0xe0033478 */
    uvint32_t rsrvd0[33];
    uvint32_t prot_epi_cs1[16];                   /* 0xe0033500 */
    uvint32_t prot_epi_cs2[16];                   /* 0xe0033540 */
    uvint32_t prot_epi_unmapped_stream[16];       /* 0xe0033580 */
    uvint32_t prot_epi_mapped_stream[16];         /* 0xe00335c0 */
    uvint32_t prot_epo_cs1[16];                   /* 0xe0033600 */
    uvint32_t prot_epo_cs2[16];                   /* 0xe0033640 */
    uvint32_t prot_epo_unmapped_stream[16];       /* 0xe0033680 */
    uvint32_t prot_epo_mapped_stream[16];         /* 0xe00336c0 */
    uvint32_t prot_stream_error_disable;          /* 0xe0033700 */
    uvint32_t prot_stream_error_status;           /* 0xe0033704 */
} USB3PROT_REGS_T, *PUSB3PROT_REGS_T;

#define USB3PROT        ((PUSB3PROT_REGS_T) USB3PROT_BASE_ADDR)


/*
   Protocol Layer Control and Status
 */
#define CY_U3P_UIB_PROT_CS_ADDRESS                          (0xe0033400)
#define CY_U3P_UIB_PROT_CS                                  (*(uvint32_t *)(0xe0033400))
#define CY_U3P_UIB_PROT_CS_DEFAULT                          (0xc0e80000)

/*
   During the USB enumeration process, the host sends a device a unique 7-bit
   address, which the USB core copies into this register. The USB Core will
   automatically respond only to its assigned address.  During the USB RESET,
   this register will be cleared to zero.
 */
#define CY_U3P_UIB_SS_DEVICEADDR_MASK                       (0x0000007f) /* <0:6> RW:R:0:No */
#define CY_U3P_UIB_SS_DEVICEADDR_POS                        (0)


/*
   TBD
 */
#define CY_U3P_UIB_SS_TEST_MODE_MASK                        (0x0000ff80) /* <7:15> R:RW:0:No */
#define CY_U3P_UIB_SS_TEST_MODE_POS                         (7)


/*
   Allow device to ACK SETUP status phase packets
 */
#define CY_U3P_UIB_SS_SETUP_CLR_BUSY                        (1u << 16) /* <16:16> RW1S:RW1C:0:No */


/*
   Set "1" to this bit, the HW will send NRDY all transfers from the host
   in all endpoint1-31.
 */
#define CY_U3P_UIB_SS_NRDY_ALL                              (1u << 17) /* <17:17> R:RW:0:No */


/*
   Ingress TP response transmit buffer threshold for almost full flag.  When
   buffer contains TP_THRESHOLD items or more, controller will stop issuing
   credits to host.  This field must be larger than 0.  The transmit buffer
   can hold up to 64 responses.
 */
#define CY_U3P_UIB_SS_TP_THRESHOLD_MASK                     (0x00fc0000) /* <18:23> R:RW:58:No */
#define CY_U3P_UIB_SS_TP_THRESHOLD_POS                      (18)


/*
   This bit is used to infom the protocol of what the response to incoming
   TP/DPH should be after warm/host reset. This register will be used by
   Protocol until the LINK_INTR.LTSSM_RESET is cleared by CPU.
   0: Issue NRDY
   1: Ignore TP
 */
#define CY_U3P_UIB_SS_PROT_HOST_RESET_RESP                  (1u << 24) /* <24:24> R:RW:0:No */


/*
   This bit indicates if the seq numbers are EP based or Stream ID(Socket)
   based
   0: EP based
   1: Stream ID(Socket) based
 */
#define CY_U3P_UIB_SS_SEQ_NUM_CONFIG                        (1u << 25) /* <25:25> R:RW:0:No */


/*
   This bit will control the idle detection logic in the protocol.
   0: Logic is not disabled.
   1: Logic is disabled.
 */
#define CY_U3P_UIB_SS_DISABLE_IDLE_DET                      (1u << 26) /* <26:26> R:RW:0:No */


/*
   This timer indicates how long protocol should wait for data (MULT is enabled)
   to be available by EPM before terminating a burst.
   Protocol will multiply the value programmed by 4.
 */
#define CY_U3P_UIB_SS_MULT_TIMER_MASK                       (0xf8000000) /* <27:31> R:RW:0x18:No */
#define CY_U3P_UIB_SS_MULT_TIMER_POS                        (27)



/*
   Protocol Layer Interrupt Register
 */
#define CY_U3P_UIB_PROT_INTR_ADDRESS                        (0xe0033404)
#define CY_U3P_UIB_PROT_INTR                                (*(uvint32_t *)(0xe0033404))
#define CY_U3P_UIB_PROT_INTR_DEFAULT                        (0x00000000)

/*
   A LMP was received and placed in PROT_LMP_PACKET_RX.  The LMP may have
   been recognized and processed as well (leading to other interrupts in
   this register).
 */
#define CY_U3P_UIB_LMP_RCV_EV                               (1u << 0) /* <0:0> RW1S:RW1C:0:No */


/*
   An unkown LMP was received and placed in PROT_LMP_PACKET_RX.  The LMP
   was not recognized and no response LMP was sent back.
 */
#define CY_U3P_UIB_LMP_UNKNOWN_EV                           (1u << 1) /* <1:1> RW1S:RW1C:0:No */


/*
   A Port Capabilities LMP was received.  A response may have been sent automatically
   depending on settings for PROT_LMP_PORT_CAPABILITIES_TIMER.
 */
#define CY_U3P_UIB_LMP_PORT_CAP_EV                          (1u << 2) /* <2:2> RW1S:RW1C:0:No */


/*
   A Port Configuration LMP was received.  A response may have been sent
   automatically depending on settings for PROT_LMP_PORT_CONFIGURATION_TIMER.
 */
#define CY_U3P_UIB_LMP_PORT_CFG_EV                          (1u << 3) /* <3:3> RW1S:RW1C:0:No */


/*
   The Port Capabilities LMP Timer expired
 */
#define CY_U3P_UIB_TIMEOUT_PORT_CAP_EV                      (1u << 4) /* <4:4> RW1S:RW1C:0:No */


/*
   The Port Configuraiton LMP Timer expired
 */
#define CY_U3P_UIB_TIMEOUT_PORT_CFG_EV                      (1u << 5) /* <5:5> RW1S:RW1C:0:No */


/*
   The Ping Timer expired
 */
#define CY_U3P_UIB_TIMEOUT_PING_EV                          (1u << 6) /* <6:6> RW1S:RW1C:0:No */


/*
   The Host Ack Response Timer expired
 */
#define CY_U3P_UIB_TIMEOUT_HOST_ACK_EV                      (1u << 7) /* <7:7> RW1S:RW1C:0:No */


/*
   Set whenever a ITP(SOF) occurrs
 */
#define CY_U3P_UIB_ITP_EV                                   (1u << 8) /* <8:8> RW1S:RW1C:0:No */


/*
   Set whenever a (valid of invalid) SETUP DPP is received that is not a
   set_address.  The set_address DPP is handled entirely in hardware and
   does not require any firmware intervention.
 */
#define CY_U3P_UIB_SUTOK_EV                                 (1u << 9) /* <9:9> RW1S:RW1C:0:No */


/*
   Set whenever an ACK TP is received with HE=1
   [USB 3.0:  section 8.5.1, Table 8-12, p 8-13]
 */
#define CY_U3P_UIB_HOST_ERR_EV                              (1u << 10) /* <10:10> RW1S:RW1C:0:No */


/*
   Set when host completes Status Stage of a Control Transfer
 */
#define CY_U3P_UIB_STATUS_STAGE                             (1u << 11) /* <11:11> RW1S:RW1C:0:No */


/*
   Set whenever a LMP port capability is received but the Link Speed is not
   "1" or
   Num HP buffer is not "4" or bit zero of the Direction is not "1".
 */
#define CY_U3P_UIB_LMP_INVALID_PORT_CAP_EV                  (1u << 12) /* <12:12> RW1S:RW1C:0:No */


/*
   Set whenever a LMP port configuration is received but the Link Speed is
   not "1".
 */
#define CY_U3P_UIB_LMP_INVALID_PORT_CFG_EV                  (1u << 13) /* <13:13> RW1S:RW1C:0:No */


/*
   Device sets this interrupt based on the following conditions:
   1: When Host sends more data than it is suppose to in ingress.
   2: When Device sends more data that it suppose to in egress.
   3: During STATUS Stage, host sends/asks to/for data from device.
   Device will come out of stall condition when it receives a valid SETUP(SUTOK_EV
   interrupt will be generated).
 */
#define CY_U3P_UIB_EP0_STALLED_EV                           (1u << 14) /* <14:14> RW1S:RW1C:0:No */


/*
   Device sets this interrupt when it receives a set address 0 command.
 */
#define CY_U3P_UIB_SET_ADDR0_EV                             (1u << 15) /* <15:15> RW1S:RW1C:0:No */



/*
   Protocol Interrupts Mask Register
 */
#define CY_U3P_UIB_PROT_INTR_MASK_ADDRESS                   (0xe0033408)
#define CY_U3P_UIB_PROT_INTR_MASK                           (*(uvint32_t *)(0xe0033408))
#define CY_U3P_UIB_PROT_INTR_MASK_DEFAULT                   (0x00000000)

/*
   1= Report interrupt to CPU
 */
#define CY_U3P_UIB_LMP_RCV_EN                               (1u << 0) /* <0:0> R:RW:0:N/A */


/*
   1= Report interrupt to CPU
 */
#define CY_U3P_UIB_LMP_UNKNOWN_EN                           (1u << 1) /* <1:1> R:RW:0:N/A */


/*
   1= Report interrupt to CPU
 */
#define CY_U3P_UIB_LMP_PORT_CAP_EN                          (1u << 2) /* <2:2> R:RW:0:N/A */


/*
   1= Report interrupt to CPU
 */
#define CY_U3P_UIB_LMP_PORT_CFG_EN                          (1u << 3) /* <3:3> R:RW:0:N/A */


/*
   1= Report interrupt to CPU
 */
#define CY_U3P_UIB_TIMEOUT_PORT_CAP_EN                      (1u << 4) /* <4:4> R:RW:0:N/A */


/*
   1= Report interrupt to CPU
 */
#define CY_U3P_UIB_TIMEOUT_PORT_CFG_EN                      (1u << 5) /* <5:5> R:RW:0:N/A */


/*
   1= Report interrupt to CPU
 */
#define CY_U3P_UIB_TIMEOUT_PING_EN                          (1u << 6) /* <6:6> R:RW:0:N/A */


/*
   1= Report interrupt to CPU
 */
#define CY_U3P_UIB_TIMEOUT_HOST_ACK_EN                      (1u << 7) /* <7:7> R:RW:0:N/A */


/*
   1= Report interrupt to CPU
 */
#define CY_U3P_UIB_ITP_EN                                   (1u << 8) /* <8:8> R:RW:0:N/A */


/*
   1= Report interrupt to CPU
 */
#define CY_U3P_UIB_SUTOK_EN                                 (1u << 9) /* <9:9> R:RW:0:N/A */


/*
   1= Report interrupt to CPU
 */
#define CY_U3P_UIB_HOST_ERR_EN                              (1u << 10) /* <10:10> R:RW:0:N/A */


/*
   1= Report interrupt to CPU
 */
#define CY_U3P_UIB_STATUS_STAGE                             (1u << 11) /* <11:11> R:RW:0:N/A */


/*
   1= Report interrupt to CPU
 */
#define CY_U3P_UIB_LMP_INVALID_PORT_CAP_EN                  (1u << 12) /* <12:12> R:RW:0:N/A */


/*
   1= Report interrupt to CPU
 */
#define CY_U3P_UIB_LMP_INVALID_PORT_CFG_EN                  (1u << 13) /* <13:13> R:RW:0:N/A */


/*
   1= Report interrupt to CPU
 */
#define CY_U3P_UIB_EP0_STALLED_EN                           (1u << 14) /* <14:14> R:RW:0:N/A */


/*
   1= Report interrupt to CPU
 */
#define CY_U3P_UIB_SET_ADDR0_EN                             (1u << 15) /* <15:15> R:RW:0:N/A */



/*
   Port Capabilites LMP Timeout Configuration
 */
#define CY_U3P_UIB_PROT_LMP_PORT_CAPABILITY_TIMER_ADDRESS    (0xe0033418)
#define CY_U3P_UIB_PROT_LMP_PORT_CAPABILITY_TIMER           (*(uvint32_t *)(0xe0033418))
#define CY_U3P_UIB_PROT_LMP_PORT_CAPABILITY_TIMER_DEFAULT    (0x000009c4)

/*
   Maximum  time  after  a  successful  warm  reset  or  a power  on  reset
    that  the  device should wait for port  capability LMP on its RX link.
   Default is 20us (2500 cycles using 125 mhz clock)
 */
#define CY_U3P_UIB_RX_TIMEOUT_MASK                          (0x00007fff) /* <0:14> R:RW:2500:No */
#define CY_U3P_UIB_RX_TIMEOUT_POS                           (0)


/*
   Disables the protocol layer to wait for port capabilities LMP
 */
#define CY_U3P_UIB_RX_DISABLE                               (1u << 15) /* <15:15> R:RW:0:No */


/*
   This for TX lane, device should send port capability within 20us of link
   initialization done. Firmware can load a timer and ask device to wait
   for that much time before sending port Capability LMP
 */
#define CY_U3P_UIB_TX_TIMEOUT_MASK                          (0x7fff0000) /* <16:30> R:RW:0:No */
#define CY_U3P_UIB_TX_TIMEOUT_POS                           (16)


/*
   Disables the protocol layer to send port capabilities LMP
 */
#define CY_U3P_UIB_TX_DISABLE                               (1u << 31) /* <31:31> R:RW:0:No */



/*
   Port Configuration LMP Timeout Configuration
 */
#define CY_U3P_UIB_PROT_LMP_PORT_CONFIGURATION_TIMER_ADDRESS    (0xe003341c)
#define CY_U3P_UIB_PROT_LMP_PORT_CONFIGURATION_TIMER        (*(uvint32_t *)(0xe003341c))
#define CY_U3P_UIB_PROT_LMP_PORT_CONFIGURATION_TIMER_DEFAULT    (0x000009c4)

/*
   Maximum  time  after  a  successful  warm  reset  or  a power  on  reset
    that  the  link  partners  should  send  the  port  configuration LMP.
    Default is 20us (2500 cycles using 125 mhz clock)
 */
#define CY_U3P_UIB_RX_TIMEOUT_MASK                          (0x00007fff) /* <0:14> R:RW:2500:No */
#define CY_U3P_UIB_RX_TIMEOUT_POS                           (0)


/*
   Disables the protocol layer to wait for port configuration LMP
 */
#define CY_U3P_UIB_RX_DISABLE                               (1u << 15) /* <15:15> R:RW:0:No */


/*
   Maximum time device protocol layer will wait after receiving Port Configuration
   LMP.
 */
#define CY_U3P_UIB_TX_TIMEOUT_MASK                          (0x7fff0000) /* <16:30> R:RW:0:No */
#define CY_U3P_UIB_TX_TIMEOUT_POS                           (16)


/*
   Disables the protocol layer to send Port configuration response LMP
 */
#define CY_U3P_UIB_TX_DISABLE                               (1u << 31) /* <31:31> R:RW:0:No */



/*
   Frame Counter Register
 */
#define CY_U3P_UIB_PROT_FRAMECNT_ADDRESS                    (0xe0033428)
#define CY_U3P_UIB_PROT_FRAMECNT                            (*(uvint32_t *)(0xe0033428))
#define CY_U3P_UIB_PROT_FRAMECNT_DEFAULT                    (0x00000000)

/*
   MICROFRAME counter which indicates which of the 8 125-microsecond micro-frames
   last occurred... This is based on ITPs recieved from Host
 */
#define CY_U3P_UIB_SS_MICROFRAME_MASK                       (0x00003fff) /* <0:13> RW:R:0:No */
#define CY_U3P_UIB_SS_MICROFRAME_POS                        (0)


/*
   The delta value in the last ITP received
 */
#define CY_U3P_UIB_DELTA_MASK                               (0x07ffc000) /* <14:26> RW:R:0:No */
#define CY_U3P_UIB_DELTA_POS                                (14)



/*
   ITP Time Free Running Counter
 */
#define CY_U3P_UIB_PROT_ITP_TIME_ADDRESS                    (0xe0033430)
#define CY_U3P_UIB_PROT_ITP_TIME                            (*(uvint32_t *)(0xe0033430))
#define CY_U3P_UIB_PROT_ITP_TIME_DEFAULT                    (0x00000000)

/*
   Current counter value.
 */
#define CY_U3P_UIB_COUNTER24_MASK                           (0x00ffffff) /* <0:23> RW:R:H:No */
#define CY_U3P_UIB_COUNTER24_POS                            (0)



/*
   ITP Time Stamp Register
 */
#define CY_U3P_UIB_PROT_ITP_TIMESTAMP_ADDRESS               (0xe0033434)
#define CY_U3P_UIB_PROT_ITP_TIMESTAMP                       (*(uvint32_t *)(0xe0033434))
#define CY_U3P_UIB_PROT_ITP_TIMESTAMP_DEFAULT               (0x00000000)

/*
   Timestamp from a free running counter at 125MHz of the last ITP reception.
 */
#define CY_U3P_UIB_TIMESTAMP_MASK                           (0x00ffffff) /* <0:23> RW:R:0:No */
#define CY_U3P_UIB_TIMESTAMP_POS                            (0)


/*
   LSBs of MICROFRAME field of ITP when timestamp was taken.
 */
#define CY_U3P_UIB_MICROFRAME_LSB_MASK                      (0xff000000) /* <24:31> RW:R:0:No */
#define CY_U3P_UIB_MICROFRAME_LSB_POS                       (24)



/*
   Received SETUP Packet Data
 */
#define CY_U3P_UIB_PROT_SETUPDAT0_ADDRESS                   (0xe0033438)
#define CY_U3P_UIB_PROT_SETUPDAT0                           (*(uvint32_t *)(0xe0033438))
#define CY_U3P_UIB_PROT_SETUPDAT0_DEFAULT                   (0x00000000)

/*
   Setup data field
 */
#define CY_U3P_UIB_SETUP_REQUEST_TYPE_MASK                  (0x000000ff) /* <0:7> RW:R:0:No */
#define CY_U3P_UIB_SETUP_REQUEST_TYPE_POS                   (0)


/*
   Setup data field
 */
#define CY_U3P_UIB_SETUP_REQUEST_MASK                       (0x0000ff00) /* <8:15> RW:R:0:No */
#define CY_U3P_UIB_SETUP_REQUEST_POS                        (8)


/*
   Setup data field
 */
#define CY_U3P_UIB_SETUP_VALUE_MASK                         (0xffff0000) /* <16:31> RW:R:0:No */
#define CY_U3P_UIB_SETUP_VALUE_POS                          (16)



/*
   Received SETUP Packet Data
 */
#define CY_U3P_UIB_PROT_SETUPDAT1_ADDRESS                   (0xe003343c)
#define CY_U3P_UIB_PROT_SETUPDAT1                           (*(uvint32_t *)(0xe003343c))
#define CY_U3P_UIB_PROT_SETUPDAT1_DEFAULT                   (0x00000000)

/*
   Setup data field
 */
#define CY_U3P_UIB_SETUP_INDEX_MASK                         (0x0000ffff) /* <0:15> RW:R:0:No */
#define CY_U3P_UIB_SETUP_INDEX_POS                          (0)


/*
   Setup data field
 */
#define CY_U3P_UIB_SETUP_LENGTH_MASK                        (0xffff0000) /* <16:31> RW:R:0:No */
#define CY_U3P_UIB_SETUP_LENGTH_POS                         (16)



/*
   Sequence Number
 */
#define CY_U3P_UIB_PROT_SEQ_NUM_ADDRESS                     (0xe0033440)
#define CY_U3P_UIB_PROT_SEQ_NUM                             (*(uvint32_t *)(0xe0033440))
#define CY_U3P_UIB_PROT_SEQ_NUM_DEFAULT                     (0x80000000)

/*
   Endpoint Number
 */
#define CY_U3P_UIB_ENDPOINT_MASK                            (0x0000000f) /* <0:3> R:RW:0:No */
#define CY_U3P_UIB_ENDPOINT_POS                             (0)


/*
   0: OUT
   1: IN
 */
#define CY_U3P_UIB_DIR                                      (1u << 4) /* <4:4> R:RW:0:No */


/*
   Packet sequence number of next packet to receive/transmit.  Set by hardware
   if COMMAND=0, set by software when COMMAND=1.
 */
#define CY_U3P_UIB_SEQUENCE_NUMBER_MASK                     (0x00001f00) /* <8:12> RW:RW:0:No */
#define CY_U3P_UIB_SEQUENCE_NUMBER_POS                      (8)


/*
   Sequence number of last packet that was transmitted (can be higher than
   SEQUENCE NUMBER).  Returned as part of a read operation.
 */
#define CY_U3P_UIB_LAST_COMMITTED_MASK                      (0x001f0000) /* <16:20> RW:R:0:No */
#define CY_U3P_UIB_LAST_COMMITTED_POS                       (16)


/*
   0: Read
   1: Write
 */
#define CY_U3P_UIB_COMMAND                                  (1u << 30) /* <30:30> R:RW:0:No */


/*
   Set by hardware when read/write operation has completed.  Must be cleared
   by software to initiate a read/write operation.
 */
#define CY_U3P_UIB_SEQ_VALID                                (1u << 31) /* <31:31> RW1S:RW0C:1:No */



/*
   Link Management Packet Received Value
 */
#define CY_U3P_UIB_PROT_LMP_RECEIVED_ADDRESS                (0xe003345c)
#define CY_U3P_UIB_PROT_LMP_RECEIVED                        (*(uvint32_t *)(0xe003345c))
#define CY_U3P_UIB_PROT_LMP_RECEIVED_DEFAULT                (0x00000000)

/*
   U2 Inactivity Timeout Value
 */
#define CY_U3P_UIB_U2_INACTIVITY_TIMEOUT_MASK               (0x000000ff) /* <0:7> RW:R:0:No */
#define CY_U3P_UIB_U2_INACTIVITY_TIMEOUT_POS                (0)


/*
   Force Link to accept LGO_Ux Link Commands
 */
#define CY_U3P_UIB_FORCE_LINKPM_ACCEPT                      (1u << 8) /* <8:8> RW:R:0:No */



/*
   Endpoint Interrupts
 */
#define CY_U3P_UIB_PROT_EP_INTR_ADDRESS                     (0xe0033474)
#define CY_U3P_UIB_PROT_EP_INTR                             (*(uvint32_t *)(0xe0033474))
#define CY_U3P_UIB_PROT_EP_INTR_DEFAULT                     (0x00000000)

/*
   Bit <x> indicates an interrupt from EPI_CS[x]
 */
#define CY_U3P_UIB_EP_IN_MASK                               (0x0000ffff) /* <0:15> RW:R:0:N/A */
#define CY_U3P_UIB_EP_IN_POS                                (0)


/*
   Bit <16+x> indicates an interrupt from EPO_CS[x]
 */
#define CY_U3P_UIB_EP_OUT_MASK                              (0xffff0000) /* <16:31> RW:R:0:N/A */
#define CY_U3P_UIB_EP_OUT_POS                               (16)



/*
   Endpoint Interrupt mask
 */
#define CY_U3P_UIB_PROT_EP_INTR_MASK_ADDRESS                (0xe0033478)
#define CY_U3P_UIB_PROT_EP_INTR_MASK                        (*(uvint32_t *)(0xe0033478))
#define CY_U3P_UIB_PROT_EP_INTR_MASK_DEFAULT                (0x00000000)

/*
   Bit <x> masks any interrupt from EPI_CS[x]
 */
#define CY_U3P_UIB_EP_IN_MASK                               (0x0000ffff) /* <0:15> R:RW:0:N/A */
#define CY_U3P_UIB_EP_IN_POS                                (0)


/*
   Bit <16+x> masks any interrupt from EPO_CS[x]
 */
#define CY_U3P_UIB_EP_OUT_MASK                              (0xffff0000) /* <16:31> R:RW:0:N/A */
#define CY_U3P_UIB_EP_OUT_POS                               (16)



/*
   SuperSpeed IN Endpoint Control and Status
 */
#define CY_U3P_UIB_PROT_EPI_CS1_ADDRESS(n)                  (0xe0033500 + ((n) * (0x0004)))
#define CY_U3P_UIB_PROT_EPI_CS1(n)                          (*(uvint32_t *)(0xe0033500 + ((n) * 0x0004)))
#define CY_U3P_UIB_PROT_EPI_CS1_DEFAULT                     (0x00000020)

/*
   Set VALID=1 to activate an endpoint, and VALID=0 to de-activate it. All
   USB endpoints default to invalid. An endpoint whose VALID bit is 0 does
   not respond to any USB traffic.
 */
#define CY_U3P_UIB_SSEPI_VALID                              (1u << 0) /* <0:0> R:RW:0:No */


/*
   Setting this bit causes NRDY on IN transactions.
 */
#define CY_U3P_UIB_SSEPI_NRDY                               (1u << 1) /* <1:1> R:RW:0:No */


/*
   Set this bit to “1” to stall an endpoint, and to “0” to clear a stall.
 */
#define CY_U3P_UIB_SSEPI_STALL                              (1u << 2) /* <2:2> R:RW:0:No */


/*
   Enables bulk stream protocol handling for this EP
 */
#define CY_U3P_UIB_SSEPI_STREAM_EN                          (1u << 3) /* <3:3> R:RW:0:No */


/*
   Per End Point Reset.
 */
#define CY_U3P_UIB_SSEPI_EP_RESET                           (1u << 4) /* <4:4> R:RW:0:No */


/*
   Issue STALL whenever Stream error occurs.
 */
#define CY_U3P_UIB_SSEPI_STREAM_ERROR_STALL_EN              (1u << 5) /* <5:5> R:RW:1:No */


/*
   Set whenever an IN token was ACKed by the host.
 */
#define CY_U3P_UIB_SSEPI_COMMIT                             (1u << 8) /* <8:8> RW1S:RW1C:0:No */


/*
   Whenever the USB3.0 does a retry it will asserts this interrupt.
 */
#define CY_U3P_UIB_SSEPI_RETRY                              (1u << 9) /* <9:9> RW1S:RW1C:0:No */


/*
   EP in flow control due to EPM not being available.
 */
#define CY_U3P_UIB_SSEPI_FLOWCONTROL                        (1u << 10) /* <10:10> RW1S:RW1C:0:No */


/*
   Nrdy was sent for a bulk stream request because of EP-stream not present
   in the mapper.
 */
#define CY_U3P_UIB_SSEPI_STREAMNRDY                         (1u << 11) /* <11:11> RW1S:RW1C:0:No */


/*
   Indicates a zero length packet was returned to the host in an IN transaction.
    Must be cleared by s/w.
 */
#define CY_U3P_UIB_SSEPI_ZERO                               (1u << 12) /* <12:12> RW1S:RW1C:0:No */


/*
   Indicates a shorter-than-maxsize packet was received, but UIB_EPI_XFER_CNT
   did not reach 0).
 */
#define CY_U3P_UIB_SSEPI_SHORT                              (1u << 13) /* <13:13> RW1S:RW1C:0:No */


/*
   Out Of Sequence Error. Anytime an ACK is received with unexpected sequence
   number request, the ACK will be dropped and intr will be raised
 */
#define CY_U3P_UIB_SSEPI_OOSERR                             (1u << 14) /* <14:14> RW1S:RW1C:0:No */


/*
   The Burst Was terminated by the host.
 */
#define CY_U3P_UIB_SSEPI_HBTERM                             (1u << 15) /* <15:15> RW1S:RW1C:0:No */


/*
   The Burst was terminated by the device when the MULT_TIMER expires.
 */
#define CY_U3P_UIB_SSEPI_DBTERM                             (1u << 16) /* <16:16> RW1S:RW1C:0:No */


/*
   Stream Error occurred.
 */
#define CY_U3P_UIB_SSEPI_STREAM_ERROR                       (1u << 17) /* <17:17> RW1S:RW1C:0:No */


/*
   The NumP for the first ACK is zero.
 */
#define CY_U3P_UIB_SSEPI_FIRST_ACK_NUMP_0                   (1u << 18) /* <18:18> RW1S:RW1C:0:No */


/*
   Interrupt mask for COMMIT bit
 */
#define CY_U3P_UIB_SSEPI_COMMIT_MASK                        (1u << 19) /* <19:19> R:RW:0:No */


/*
   Interupt mask for RETRY bit
 */
#define CY_U3P_UIB_SSEPI_RETRY_MASK                         (1u << 20) /* <20:20> R:RW:0:No */


/*
   Interrupt mask for FLOWCONTROL bit
 */
#define CY_U3P_UIB_SSEPI_FLOWCONTROL_MASK                   (1u << 21) /* <21:21> R:RW:0:No */


/*
   Interrupt mask for STREAMNRDY bit
 */
#define CY_U3P_UIB_SSEPI_STREAMNRDY_MASK                    (1u << 22) /* <22:22> R:RW:0:No */


/*
   Interrupt mask for ZERO bit
 */
#define CY_U3P_UIB_SSEPI_ZERO_MASK                          (1u << 23) /* <23:23> R:RW:0:No */


/*
   Interrupt mask for SHORT bit
 */
#define CY_U3P_UIB_SSEPI_SHORT_MASK                         (1u << 24) /* <24:24> R:RW:0:No */


/*
   Interrupt mask for OOSERR bit
 */
#define CY_U3P_UIB_SSEPI_OOSERR_MASK                        (1u << 25) /* <25:25> R:RW:0:No */


/*
   Interrupt mask for HBTERM bit
 */
#define CY_U3P_UIB_SSEPI_HBTERM_MASK                        (1u << 26) /* <26:26> R:RW:0:No */


/*
   Interrupt mask for DBTERM bit
 */
#define CY_U3P_UIB_SSEPI_DBTERM_MASK                        (1u << 27) /* <27:27> R:RW:0:No */


/*
   Interrupt mask for STREAM_ERROR bit
 */
#define CY_U3P_UIB_SSEPI_STREAM_ERROR_MASK                  (1u << 28) /* <28:28> R:RW:0:No */


/*
   Interrupt mask for FIRST_ACK_NUMP_0 bit
 */
#define CY_U3P_UIB_SSEPI_FIRST_ACK_NUMP_0_MASK              (1u << 29) /* <29:29> R:RW:0:No */



/*
   SuperSpeed IN Endpoint Control and Status
 */
#define CY_U3P_UIB_PROT_EPI_CS2_ADDRESS(n)                  (0xe0033540 + ((n) * (0x0004)))
#define CY_U3P_UIB_PROT_EPI_CS2(n)                          (*(uvint32_t *)(0xe0033540 + ((n) * 0x0004)))
#define CY_U3P_UIB_PROT_EPI_CS2_DEFAULT                     (0x00000040)

/*
   Endpoint type (EP0 suports CONTROL only)
   0: ISO
   1: INT
   2: BULK
   3: CONTROL (only valid for EP0)
 */
#define CY_U3P_UIB_SSEPI_TYPE_MASK                          (0x00000003) /* <0:1> R:RW:0:No */
#define CY_U3P_UIB_SSEPI_TYPE_POS                           (0)


/*
   Number of packets to be sent per service interval .  Maximum can be 48
   ( Max burst size* Mult field)
 */
#define CY_U3P_UIB_SSEPI_ISOINPKS_MASK                      (0x000000fc) /* <2:7> R:RW:16:No */
#define CY_U3P_UIB_SSEPI_ISOINPKS_POS                       (2)


/*
   Maximum number of packets the endpoint can send.
   (truncated to 4b, 0 means 16)
 */
#define CY_U3P_UIB_SSEPI_MAXBURST_MASK                      (0x00000f00) /* <8:11> R:RW:0:No */
#define CY_U3P_UIB_SSEPI_MAXBURST_POS                       (8)


/*
   Number of packets that need to be cleaned up by CPU whenever there is
   a BTERM interrupt.
 */
#define CY_U3P_UIB_SSEPI_BTERM_NUMP_MASK                    (0x0001f000) /* <12:16> RW:R:0:No */
#define CY_U3P_UIB_SSEPI_BTERM_NUMP_POS                     (12)



/*
   Unmapped Stream Request
 */
#define CY_U3P_UIB_PROT_EPI_UNMAPPED_STREAM_ADDRESS(n)      (0xe0033580 + ((n) * (0x0004)))
#define CY_U3P_UIB_PROT_EPI_UNMAPPED_STREAM(n)              (*(uvint32_t *)(0xe0033580 + ((n) * 0x0004)))
#define CY_U3P_UIB_PROT_EPI_UNMAPPED_STREAM_DEFAULT         (0x00000000)

/*
   The StreamID of the current stream activated (or requested to be activated)
   by the protocol layer.
 */
#define CY_U3P_UIB_STREAM_ID_MASK                           (0x0000ffff) /* <0:15> RW:R:0:No */
#define CY_U3P_UIB_STREAM_ID_POS                            (0)


/*
   Stream Protocol State Machine (SPSM) State for this EndPoint:
   
   0: Not Configured
   1: Disabled
   2: Prime Pipe
   3: DFR Prime Pipe
   4: Idle
   5: Start Stream
   6: Move Data
   7: End
   8: Error
 */
#define CY_U3P_UIB_SPSM_STATE_MASK                          (0x000f0000) /* <16:19> RW:R:0:No */
#define CY_U3P_UIB_SPSM_STATE_POS                           (16)



/*
   Mapped Streams Registers
 */
#define CY_U3P_UIB_PROT_EPI_MAPPED_STREAM_ADDRESS(n)        (0xe00335c0 + ((n) * (0x0004)))
#define CY_U3P_UIB_PROT_EPI_MAPPED_STREAM(n)                (*(uvint32_t *)(0xe00335c0 + ((n) * 0x0004)))
#define CY_U3P_UIB_PROT_EPI_MAPPED_STREAM_DEFAULT           (0x00000000)

/*
   The StreamID of the stream connected to the corresponding socket by firmware.
 */
#define CY_U3P_UIB_STREAM_ID_MASK                           (0x0000ffff) /* <0:15> R:RW:0:No */
#define CY_U3P_UIB_STREAM_ID_POS                            (0)


/*
   The Endpoint number of the stream connected to the corresponding socket
   by firmware.
 */
#define CY_U3P_UIB_EP_NUMBER_MASK                           (0x000f0000) /* <16:19> R:RW:0:No */
#define CY_U3P_UIB_EP_NUMBER_POS                            (16)


/*
   Stream is unmapped (not in use by the corresponding EP's SPSM).
 */
#define CY_U3P_UIB_UNMAPPED                                 (1u << 29) /* <29:29> RW:R:0:No */


/*
   Request to unmap this stream.  May be cleared to revert/withdaw request.
 */
#define CY_U3P_UIB_UNMAP                                    (1u << 30) /* <30:30> R:RW:0:No */


/*
   Set by firmware if a stream is mapped to the corresponding socket. If
   this bit is set, the endpoint number corresponding to this socket number
   can no longer be used in non-streaming mode (that would create a conflict
   of two endpoints wanting to use the same socket).
 */
#define CY_U3P_UIB_ENABLE                                   (1u << 31) /* <31:31> R:RW :0:No */



/*
   SuperSpeed OUT Endpoint Control and Status
 */
#define CY_U3P_UIB_PROT_EPO_CS1_ADDRESS(n)                  (0xe0033600 + ((n) * (0x0004)))
#define CY_U3P_UIB_PROT_EPO_CS1(n)                          (*(uvint32_t *)(0xe0033600 + ((n) * 0x0004)))
#define CY_U3P_UIB_PROT_EPO_CS1_DEFAULT                     (0x00000020)

/*
   Set VALID=1 to activate an endpoint, and VALID=0 to de-activate it. All
   USB endpoints default to invalid. An endpoint whose VALID bit is 0 does
   not respond to any USB traffic.
 */
#define CY_U3P_UIB_SSEPO_VALID                              (1u << 0) /* <0:0> R:RW:0:No */


/*
   Setting this bit causes NRDY on IN transactions.
 */
#define CY_U3P_UIB_SSEPO_NRDY                               (1u << 1) /* <1:1> R:RW:0:No */


/*
   Set this bit to “1” to stall an endpoint, and to “0” to clear a stall.
 */
#define CY_U3P_UIB_SSEPO_STALL                              (1u << 2) /* <2:2> R:RW:0:No */


/*
   Enables bulk stream protocol handling for this EP
 */
#define CY_U3P_UIB_SSEPO_STREAM_EN                          (1u << 3) /* <3:3> R:RW:0:No */


/*
   Per End Point Reset.
 */
#define CY_U3P_UIB_SSEPO_EP_RESET                           (1u << 4) /* <4:4> R:RW:0:No */


/*
   Issue STALL whenever Stream error occurs.
 */
#define CY_U3P_UIB_SSEPO_STREAM_ERROR_STALL_EN              (1u << 5) /* <5:5> R:RW:1:No */


/*
   Set whenever an OUT DATA was commited into the EPM.
 */
#define CY_U3P_UIB_SSEPO_COMMIT                             (1u << 8) /* <8:8> RW1S:RW1C:0:No */


/*
   Whenever the USB3.0 device does a retry it will asserts this interrupt.
 */
#define CY_U3P_UIB_SSEPO_RETRY                              (1u << 9) /* <9:9> RW1S:RW1C:0:No */


/*
   EP in flow control due to EPM not being available.
 */
#define CY_U3P_UIB_SSEPO_FLOWCONTROL                        (1u << 10) /* <10:10> RW1S:RW1C:0:No */


/*
   Nrdy was sent for a bulk stream request because of EP-stream not present
   in the mapper.
 */
#define CY_U3P_UIB_SSEPO_STREAMNRDY                         (1u << 11) /* <11:11> RW1S:RW1C:0:No */


/*
   Indicates a zero length packet was received by the device in an OUT transaction.
    Must be cleared by s/w.
 */
#define CY_U3P_UIB_SSEPO_ZERO                               (1u << 12) /* <12:12> RW1S:RW1C:0:No */


/*
   Indicates a shorter-than-maxsize packet was received.
 */
#define CY_U3P_UIB_SSEPO_SHORT                              (1u << 13) /* <13:13> RW1S:RW1C:0:No */


/*
   Out Of Sequence Error. Anytime an OUT-DATA is received with unexpected
   sequence number request, the data will be dropped and intr will be raised
 */
#define CY_U3P_UIB_SSEPO_OOSERR                             (1u << 14) /* <14:14> RW1S:RW1C:0:No */


/*
   The Burst Was terminated by the host.
 */
#define CY_U3P_UIB_SSEPO_HBTERM                             (1u << 15) /* <15:15> RW1S:RW1C:0:No */


/*
   The Burst was terminated by the device when the MULT_TIMER expires.
 */
#define CY_U3P_UIB_SSEPO_DBTERM                             (1u << 16) /* <16:16> RW1S:RW1C:0:No */


/*
   Stream Error occurred.
 */
#define CY_U3P_UIB_SSEPO_STREAM_ERROR                       (1u << 17) /* <17:17> RW1S:RW1C:0:No */


/*
   The NumP for the first ACK is zero.
 */
#define CY_U3P_UIB_SSEPO_FIRST_ACK_NUMP_0                   (1u << 18) /* <18:18> RW1S:RW1C:0:No */


/*
   Interrupt mask for COMMIT bit
 */
#define CY_U3P_UIB_SSEPO_COMMIT_MASK                        (1u << 19) /* <19:19> R:RW:0:No */


/*
   Interupt mask for RETRY bit
 */
#define CY_U3P_UIB_SSEPO_RETRY_MASK                         (1u << 20) /* <20:20> R:RW:0:No */


/*
   Interrupt mask for FLOWCONTROL bit
 */
#define CY_U3P_UIB_SSEPO_FLOWCONTROL_MASK                   (1u << 21) /* <21:21> R:RW:0:No */


/*
   Interrupt mask for STREAMNRDY bit
 */
#define CY_U3P_UIB_SSEPO_STREAMNRDY_MASK                    (1u << 22) /* <22:22> R:RW:0:No */


/*
   Interrupt mask for ZERO bit
 */
#define CY_U3P_UIB_SSEPO_ZERO_MASK                          (1u << 23) /* <23:23> R:RW:0:No */


/*
   Interrupt mask for SHORT bit
 */
#define CY_U3P_UIB_SSEPO_SHORT_MASK                         (1u << 24) /* <24:24> R:RW:0:No */


/*
   Interrupt mask for OOSERR bit
 */
#define CY_U3P_UIB_SSEPO_OOSERR_MASK                        (1u << 25) /* <25:25> R:RW:0:No */


/*
   Interrupt mask for HBTERM bit
 */
#define CY_U3P_UIB_SSEPO_HBTERM_MASK                        (1u << 26) /* <26:26> R:RW:0:No */


/*
   Interrupt mask for DBTERM bit
 */
#define CY_U3P_UIB_SSEPO_DBTERM_MASK                        (1u << 27) /* <27:27> R:RW:0:No */


/*
   Interrupt mask for STREAM_ERROR bit
 */
#define CY_U3P_UIB_SSEPO_STREAM_ERROR_MASK                  (1u << 28) /* <28:28> R:RW:0:No */


/*
   Interrupt mask for FIRST_ACK_NUMP_0 bit
 */
#define CY_U3P_UIB_SSEPO_FIRST_ACK_NUMP_0_MASK              (1u << 29) /* <29:29> R:RW:0:No */



/*
   SuperSpeed IN Endpoint Control and Status
 */
#define CY_U3P_UIB_PROT_EPO_CS2_ADDRESS(n)                  (0xe0033640 + ((n) * (0x0004)))
#define CY_U3P_UIB_PROT_EPO_CS2(n)                          (*(uvint32_t *)(0xe0033640 + ((n) * 0x0004)))
#define CY_U3P_UIB_PROT_EPO_CS2_DEFAULT                     (0x00000040)

/*
   Endpoint type (EP0 suports CONTROL only)
   0: ISO
   1: INT
   2: BULK
   3: CONTROL (only valid for EP0)
 */
#define CY_U3P_UIB_SSEPO_TYPE_MASK                          (0x00000003) /* <0:1> R:RW:0:No */
#define CY_U3P_UIB_SSEPO_TYPE_POS                           (0)


/*
   Number of packets to be sent per service interval .  Maximum can be 48
   ( Max burst size* Mult field)
 */
#define CY_U3P_UIB_SSEPO_ISOINPKS_MASK                      (0x000000fc) /* <2:7> R:RW:16:No */
#define CY_U3P_UIB_SSEPO_ISOINPKS_POS                       (2)


/*
   Maximum number of packets the endpoint can receive
   (truncated to 4b, 0 means 16)
 */
#define CY_U3P_UIB_SSEPO_MAXBURST_MASK                      (0x00000f00) /* <8:11> R:RW:0:No */
#define CY_U3P_UIB_SSEPO_MAXBURST_POS                       (8)



/*
   Unmapped Stream Request
 */
#define CY_U3P_UIB_PROT_EPO_UNMAPPED_STREAM_ADDRESS(n)      (0xe0033680 + ((n) * (0x0004)))
#define CY_U3P_UIB_PROT_EPO_UNMAPPED_STREAM(n)              (*(uvint32_t *)(0xe0033680 + ((n) * 0x0004)))
#define CY_U3P_UIB_PROT_EPO_UNMAPPED_STREAM_DEFAULT         (0x00000000)

/*
   The StreamID of the current stream activated (or requested to be activated)
   by the protocol layer.
 */
#define CY_U3P_UIB_STREAM_ID_MASK                           (0x0000ffff) /* <0:15> RW:R:0:No */
#define CY_U3P_UIB_STREAM_ID_POS                            (0)


/*
   Stream Protocol State Machine (SPSM) State for this EndPoint:
   
   0: Not Configured
   1: Disabled
   2: Prime Pipe
   3: DFR Prime Pipe
   4: Idle
   5: Start Stream
   6: Move Data
   7: End
   8: Error
 */
#define CY_U3P_UIB_SPSM_STATE_MASK                          (0x000f0000) /* <16:19> RW:R:0:No */
#define CY_U3P_UIB_SPSM_STATE_POS                           (16)



/*
   Mapped Streams Registers
 */
#define CY_U3P_UIB_PROT_EPO_MAPPED_STREAM_ADDRESS(n)        (0xe00336c0 + ((n) * (0x0004)))
#define CY_U3P_UIB_PROT_EPO_MAPPED_STREAM(n)                (*(uvint32_t *)(0xe00336c0 + ((n) * 0x0004)))
#define CY_U3P_UIB_PROT_EPO_MAPPED_STREAM_DEFAULT           (0x00000000)

/*
   The StreamID of the stream connected to the corresponding socket by firmware.
 */
#define CY_U3P_UIB_STREAM_ID_MASK                           (0x0000ffff) /* <0:15> R:RW:0:No */
#define CY_U3P_UIB_STREAM_ID_POS                            (0)


/*
   The Endpoint number of the stream connected to the corresponding socket
   by firmware.
 */
#define CY_U3P_UIB_EP_NUMBER_MASK                           (0x000f0000) /* <16:19> R:RW:0:No */
#define CY_U3P_UIB_EP_NUMBER_POS                            (16)


/*
   Stream is unmapped (not in use by the corresponding EP's SPSM).
 */
#define CY_U3P_UIB_UNMAPPED                                 (1u << 29) /* <29:29> RW:R:0:No */


/*
   Request to unmap this stream.  May be cleared to revert/withdaw request.
 */
#define CY_U3P_UIB_UNMAP                                    (1u << 30) /* <30:30> R:RW:0:No */


/*
   Set by firmware if a stream is mapped to the corresponding socket. If
   this bit is set, the endpoint number corresponding to this socket number
   can no longer be used in non-streaming mode (that would create a conflict
   of two endpoints wanting to use the same socket).
 */
#define CY_U3P_UIB_ENABLE                                   (1u << 31) /* <31:31> R:RW :0:No */



/*
   Streams Error Disable Type Registers
 */
#define CY_U3P_UIB_PROT_STREAM_ERROR_DISABLE_ADDRESS        (0xe0033700)
#define CY_U3P_UIB_PROT_STREAM_ERROR_DISABLE                (*(uvint32_t *)(0xe0033700))
#define CY_U3P_UIB_PROT_STREAM_ERROR_DISABLE_DEFAULT        (0x00000000)

/*
   This register controls the type of Stream Error that would cause the ERROR_DETECTED
   bit in the PROT_STREAM_ERROR_STATUS register to bet set.
   Setting any bit will disable an specific error type.
   [0]: Stream ID changed while in MOVE DATA stage.
   [1]:  ACK/DP PRIMEP PP is "1"
   [2]: ACK/DP NoStream PP is "1"
   [3]: ACK PRIME NumP is "0"
   [4]: ACK NoStream NumP is NOT "0"
 */
#define CY_U3P_UIB_TYPE_MASK                                (0x0000003f) /* <0:5> R:RW:0:No */
#define CY_U3P_UIB_TYPE_POS                                 (0)



/*
   Streams Error STATUS Registers
 */
#define CY_U3P_UIB_PROT_STREAM_ERROR_STATUS_ADDRESS         (0xe0033704)
#define CY_U3P_UIB_PROT_STREAM_ERROR_STATUS                 (*(uvint32_t *)(0xe0033704))
#define CY_U3P_UIB_PROT_STREAM_ERROR_STATUS_DEFAULT         (0x00000000)

/*
   The stream id when the error occurred.
 */
#define CY_U3P_UIB_ID_MASK                                  (0x0000ffff) /* <0:15> RW:R:0:No */
#define CY_U3P_UIB_ID_POS                                   (0)


/*
   The End Point number when the error occurred.
 */
#define CY_U3P_UIB_EP_NUM_MASK                              (0x000f0000) /* <16:19> RW:R:0:No */
#define CY_U3P_UIB_EP_NUM_POS                               (16)


/*
   1: IN EP, 0: OUT EP
 */
#define CY_U3P_UIB_EP_IO                                    (1u << 20) /* <20:20> RW:R:0:No */


/*
   The type of the stream error that was detected.
   [1]: Stream ID changed while in MOVE DATA stage.
   [2]:  ACK/DP PRIMEP PP is "1"
   [3]: ACK/DP NoStream PP is "1"
   [4]: ACK PRIME NumP is "0"
   [5]: ACK NoStream NumP is NOT "0"
 */
#define CY_U3P_UIB_ERROR_TYPE_MASK                          (0x07e00000) /* <21:26> RW:R:0:No */
#define CY_U3P_UIB_ERROR_TYPE_POS                           (21)


/*
   The stream state when the error occurred.
   1: Disabled
   2: Prime Pipe
   3: DFR Prime Pipe
   4: Idle
   5: Start Stream
   6: Move Data
   7: End
 */
#define CY_U3P_UIB_ERROR_STATE_MASK                         (0x38000000) /* <27:29> RW:R:0:No */
#define CY_U3P_UIB_ERROR_STATE_POS                          (27)


/*
   An Stream Error was detected.
 */
#define CY_U3P_UIB_ERROR_DETECTED                           (1u << 31) /* <31:31> RW1S:RW1C:0:No */



#endif /* _INCLUDED_USB3PROT_REGS_H_ */

/*[]*/

