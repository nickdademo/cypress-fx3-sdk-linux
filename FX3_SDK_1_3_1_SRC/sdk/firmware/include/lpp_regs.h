/****************************************************************************
 *
 * File: lpp_regs.h
 *
 * Copyright (c) 2010-13 Cypress Semiconductor. All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION WHICH IS THE PROPERTY OF CYPRESS.
 *
 * Description:
 *   Common Serial Peripheral registers for the EZ-USB FX3 Device.
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

#ifndef _INCLUDED_LPP_REGS_H_
#define _INCLUDED_LPP_REGS_H_

#include <cyu3types.h>

#define LPP_BASE_ADDR                            (0xe0007f00)

typedef struct
{
    uvint32_t id;                                 /* 0xe0007f00 */
    uvint32_t power;                              /* 0xe0007f04 */
    uvint32_t rsrvd0[62];
    struct
    {
        uvint32_t dscr;                           /* 0xe0008000 */
        uvint32_t size;                           /* 0xe0008004 */
        uvint32_t count;                          /* 0xe0008008 */
        uvint32_t status;                         /* 0xe000800c */
        uvint32_t intr;                           /* 0xe0008010 */
        uvint32_t intr_mask;                      /* 0xe0008014 */
        uvint32_t rsrvd1[2];
        uvint32_t dscr_buffer;                    /* 0xe0008020 */
        uvint32_t dscr_sync;                      /* 0xe0008024 */
        uvint32_t dscr_chain;                     /* 0xe0008028 */
        uvint32_t dscr_size;                      /* 0xe000802c */
        uvint32_t rsrvd2[19];
        uvint32_t event;                          /* 0xe000807c */
    } sck[8];
    uvint32_t rsrvd3[7872];
    uvint32_t sck_intr0;                          /* 0xe000ff00 */
    uvint32_t sck_intr1;                          /* 0xe000ff04 */
    uvint32_t sck_intr2;                          /* 0xe000ff08 */
    uvint32_t sck_intr3;                          /* 0xe000ff0c */
    uvint32_t sck_intr4;                          /* 0xe000ff10 */
    uvint32_t sck_intr5;                          /* 0xe000ff14 */
    uvint32_t sck_intr6;                          /* 0xe000ff18 */
    uvint32_t sck_intr7;                          /* 0xe000ff1c */
    uvint32_t rsrvd4[56];
} LPP_REGS_T, *PLPP_REGS_T;

#define LPP        ((PLPP_REGS_T) LPP_BASE_ADDR)


/*
   Block Identification and Version Number
 */
#define CY_U3P_LPP_ID_ADDRESS                               (0xe0007f00)
#define CY_U3P_LPP_ID                                       (*(uvint32_t *)(0xe0007f00))
#define CY_U3P_LPP_ID_DEFAULT                               (0x0001fffe)

/*
   A unique number identifying the IP in the memory space.
 */
#define CY_U3P_LPP_BLOCK_ID_MASK                            (0x0000ffff) /* <0:15> R:R:0xFFFE:N/A */
#define CY_U3P_LPP_BLOCK_ID_POS                             (0)


/*
   Version number for the IP.
 */
#define CY_U3P_LPP_BLOCK_VERSION_MASK                       (0xffff0000) /* <16:31> R:R:0x0001:N/A */
#define CY_U3P_LPP_BLOCK_VERSION_POS                        (16)



/*
   Power, clock and reset control
 */
#define CY_U3P_LPP_POWER_ADDRESS                            (0xe0007f04)
#define CY_U3P_LPP_POWER                                    (*(uvint32_t *)(0xe0007f04))
#define CY_U3P_LPP_POWER_DEFAULT                            (0x00000000)

/*
   For blocks that must perform initialization after reset before becoming
   operational, this signal will remain de-asserted until initialization
   is complete.  In other words reading active=1 indicates block is initialized
   and ready for operation.
 */
#define CY_U3P_LPP_ACTIVE                                   (1u << 0) /* <0:0> W:R:0:Yes */


/*
   Active LOW reset signal for all logic in the block.  Note that reset is
   active on all flops in the block when either system reset is asserted
   (RESET# pin or SYSTEM_POWER.RESETN is asserted) or this signal is active.
   After setting this bit to 1, firmware shall poll and wait for the ‘active’
   bit to assert.  Reading ‘1’ from ‘resetn’ does not indicate the block
   is out of reset – this may take some time depending on initialization
   tasks and clock frequencies.
   This bit must be asserted ('0') for at least 10us for effective reset.
 */
#define CY_U3P_LPP_RESETN                                   (1u << 31) /* <31:31> R:RW:0:No */



/*
   Descriptor Chain Pointer
 */
#define CY_U3P_LPP_SCK_DSCR_ADDRESS(n)                      (0xe0008000 + ((n) * (0x0080)))
#define CY_U3P_LPP_SCK_DSCR(n)                              (*(uvint32_t *)(0xe0008000 + ((n) * 0x0080)))
#define CY_U3P_LPP_SCK_DSCR_DEFAULT                         (0x00000000)

/*
   Descriptor number of currently active descriptor.  A value of 0xFFFF designates
   no (more) active descriptors available.  When activating a socket CPU
   shall write number of first descriptor in here. Only modify this field
   when go_suspend=1 or go_enable=0
 */
#define CY_U3P_LPP_DSCR_NUMBER_MASK                         (0x0000ffff) /* <0:15> RW:RW:X:N/A */
#define CY_U3P_LPP_DSCR_NUMBER_POS                          (0)


/*
   Number of descriptors still left to process.  This value is unrelated
   to actual number of descriptors in the list.  It is used only to generate
   an interrupt to the CPU when the value goes low or zero (or both).  When
   this value reaches 0 it will wrap around to 255.  The socket will not
   suspend or be otherwise affected unless the descriptor chains ends with
   0xFFFF descriptor number.
 */
#define CY_U3P_LPP_DSCR_COUNT_MASK                          (0x00ff0000) /* <16:23> RW:RW:X:N/A */
#define CY_U3P_LPP_DSCR_COUNT_POS                           (16)


/*
   The low watermark for dscr_count.  When dscr_count is equal or less than
   dscr_low the status bit dscr_is_low is set and an interrupt can be generated
   (depending on int mask).
 */
#define CY_U3P_LPP_DSCR_LOW_MASK                            (0xff000000) /* <24:31> R:RW:X:N/A */
#define CY_U3P_LPP_DSCR_LOW_POS                             (24)



/*
   Transfer Size Register
 */
#define CY_U3P_LPP_SCK_SIZE_ADDRESS(n)                      (0xe0008004 + ((n) * (0x0080)))
#define CY_U3P_LPP_SCK_SIZE(n)                              (*(uvint32_t *)(0xe0008004 + ((n) * 0x0080)))
#define CY_U3P_LPP_SCK_SIZE_DEFAULT                         (0x00000000)

/*
   The number of bytes or buffers (depends on unit bit in SCK_STATUS) that
   are part of this transfer.  A value of 0 signals an infinite/undetermined
   transaction size.
   Valid data bytes remaining in the last buffer beyond the transfer size
   will be read by socket and passed on to the core. FW must ensure that
   no additional bytes beyond the transfer size are present in the last buffer.
 */
#define CY_U3P_LPP_TRANS_SIZE_MASK                          (0xffffffff) /* <0:31> R:RW:X:N/A */
#define CY_U3P_LPP_TRANS_SIZE_POS                           (0)



/*
   Transfer Count Register
 */
#define CY_U3P_LPP_SCK_COUNT_ADDRESS(n)                     (0xe0008008 + ((n) * (0x0080)))
#define CY_U3P_LPP_SCK_COUNT(n)                             (*(uvint32_t *)(0xe0008008 + ((n) * 0x0080)))
#define CY_U3P_LPP_SCK_COUNT_DEFAULT                        (0x00000000)

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
#define CY_U3P_LPP_TRANS_COUNT_MASK                         (0xffffffff) /* <0:31> RW:RW:X:N/A */
#define CY_U3P_LPP_TRANS_COUNT_POS                          (0)



/*
   Socket Status Register
 */
#define CY_U3P_LPP_SCK_STATUS_ADDRESS(n)                    (0xe000800c + ((n) * (0x0080)))
#define CY_U3P_LPP_SCK_STATUS(n)                            (*(uvint32_t *)(0xe000800c + ((n) * 0x0080)))
#define CY_U3P_LPP_SCK_STATUS_DEFAULT                       (0x04e00000)

/*
   Number of available (free for ingress, occupied for egress) descriptors
   beyond the current one.  This number is incremented by the adapter whenever
   an event is received on this socket and decremented whenever it activates
   a new descriptor. This value is used to create a signal to the IP Cores
   that indicates at least one buffer is available beyond the current one
   (sck_more_buf_avl).
 */
#define CY_U3P_LPP_AVL_COUNT_MASK                           (0x0000001f) /* <0:4> RW:RW:0:N/A */
#define CY_U3P_LPP_AVL_COUNT_POS                            (0)


/*
   Minimum number of available buffers required by the adapter before activating
   a new one.  This can be used to guarantee a minimum number of buffers
   available with old data to implement rollback.  If AVL_ENABLE, the socket
   will remain in STALL state until AVL_COUNT>=AVL_MIN.
 */
#define CY_U3P_LPP_AVL_MIN_MASK                             (0x000003e0) /* <5:9> R:RW:0:N/A */
#define CY_U3P_LPP_AVL_MIN_POS                              (5)


/*
   Enables the functioning of AVL_COUNT and AVL_MIN. When 0, it will disable
   both stalling on AVL_MIN and generation of the sck_more_buf_avl signal
   described above.
 */
#define CY_U3P_LPP_AVL_ENABLE                               (1u << 10) /* <10:10> R:RW:0:N/A */


/*
   Internal operating state of the socket.  This field is used for debugging
   and to safely modify active sockets (see go_suspend).
 */
#define CY_U3P_LPP_STATE_MASK                               (0x00038000) /* <15:17> RW:R:0:N/A */
#define CY_U3P_LPP_STATE_POS                                (15)

/*
   Descriptor state. This is the default initial state indicating the descriptor
   registers are NOT valid in the Adapter. The Adapter will start loading
   the descriptor from memory if the socket becomes enabled and not suspended.
   Suspend has no effect on any other state.
 */
#define CY_U3P_LPP_STATE_DESCR                              (0)
/*
   Stall state. Socket is stalled waiting for data to be loaded into the
   Fetch Queue or waiting for an event.
 */
#define CY_U3P_LPP_STATE_STALL                              (1)
/*
   Active state. Socket is available for core data transfers.
 */
#define CY_U3P_LPP_STATE_ACTIVE                             (2)
/*
   Event state. Core transfer is done. Descriptor is being written back to
   memory and an event is being generated if enabled.
 */
#define CY_U3P_LPP_STATE_EVENT                              (3)
/*
   Check states. An active socket gets here based on the core’s EOP request
   to check the Transfer size and determine whether the buffer should be
   wrapped up. Depending on result, socket will either go back to Active
   state or move to the Event state.
 */
#define CY_U3P_LPP_STATE_CHECK1                             (4)
/*
   Socket is suspended
 */
#define CY_U3P_LPP_STATE_SUSPENDED                          (5)
/*
   Check states. An active socket gets here based on the core’s EOP request
   to check the Transfer size and determine whether the buffer should be
   wrapped up. Depending on result, socket will either go back to Active
   state or move to the Event state.
 */
#define CY_U3P_LPP_STATE_CHECK2                             (6)
/*
   Waiting for confirmation that event was sent.
 */
#define CY_U3P_LPP_STATE_WAITING                            (7)

/*
   Indicates the socket received a ZLP
 */
#define CY_U3P_LPP_ZLP_RCVD                                 (1u << 18) /* <18:18> RW:R:0:N/A */


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
#define CY_U3P_LPP_SUSPENDED                                (1u << 19) /* <19:19> RW:R:0:N/A */


/*
   Indicates the socket is currently enabled when asserted.  After go_enable
   is changed, it may take some time for enabled to make the same change.
    This value can be polled to determine this fact.
 */
#define CY_U3P_LPP_ENABLED                                  (1u << 20) /* <20:20> RW:R:0:N/A */


/*
   Enable (1) or disable (0) truncating of BYTE_COUNT when TRANS_COUNT+BYTE_COUNT>=TRANS_SIZE.
    When enabled, ensures that an ingress transfer never contains more bytes
   then allowed.  This function is needed to implement burst-based prototocols
   that can only transmit full bursts of more than 1 byte.
 */
#define CY_U3P_LPP_TRUNCATE                                 (1u << 21) /* <21:21> R:RW:1:N/A */


/*
   Enable (1) or disable (0) sending of produce events from any descriptor
   in this socket.  If 0, events will be suppressed, and the descriptor will
   not be copied back into memory when completed.
 */
#define CY_U3P_LPP_EN_PROD_EVENTS                           (1u << 22) /* <22:22> R:RW:1:N/A */


/*
   Enable (1) or disable (0) sending of consume events from any descriptor
   in this socket.  If 0, events will be suppressed, and the descriptor will
   not be copied back into memory when completed.
 */
#define CY_U3P_LPP_EN_CONS_EVENTS                           (1u << 23) /* <23:23> R:RW:1:N/A */


/*
   When set, the socket will suspend before activating a descriptor with
   BYTE_COUNT<BUFFER_SIZE.
   This is relevant for egress sockets only.
 */
#define CY_U3P_LPP_SUSP_PARTIAL                             (1u << 24) /* <24:24> R:RW:0:N/A */


/*
   When set, the socket will suspend before activating a descriptor with
   TRANS_COUNT+BUFFER_SIZE>=TRANS_SIZE.  This is relevant for both ingress
   and egress sockets.
 */
#define CY_U3P_LPP_SUSP_LAST                                (1u << 25) /* <25:25> R:RW:0:N/A */


/*
   When set, the socket will suspend when trans_count >= trans_size.  This
   equation is checked (and hence the socket will suspend) only at the boundary
   of buffers and packets (ie. buffer wrapup or EOP assertion).
 */
#define CY_U3P_LPP_SUSP_TRANS                               (1u << 26) /* <26:26> R:RW:1:N/A */


/*
   When set, the socket will suspend after wrapping up the first buffer with
   dscr.eop=1.  Note that this function will work the same for both ingress
   and egress sockets.
 */
#define CY_U3P_LPP_SUSP_EOP                                 (1u << 27) /* <27:27> R:RW:0:N/A */


/*
   Setting this bit will forcibly wrap-up a socket, whether it is out of
   data or not.  This option is intended mainly for ingress sockets, but
   works also for egress sockets.  Any remaining data in fetch buffers is
   ignored, in write buffers is flushed.  Transaction and buffer counts are
   updated normally, and suspend behavior also happens normally (depending
   on various other settings in this register).G45
 */
#define CY_U3P_LPP_WRAPUP                                   (1u << 28) /* <28:28> RW0C:RW1S:0:N/A */


/*
   Indicates whether descriptors (1) or bytes (0) are counted by trans_count
   and trans_size.  Descriptors are counting regardless of whether they contain
   any data or have eop set.
 */
#define CY_U3P_LPP_UNIT                                     (1u << 29) /* <29:29> R:RW:0:N/A */


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
#define CY_U3P_LPP_GO_SUSPEND                               (1u << 30) /* <30:30> R:RW:0:N/A */


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
#define CY_U3P_LPP_GO_ENABLE                                (1u << 31) /* <31:31> R:RW:0:N/A */



/*
   Socket Interrupt Request Register
 */
#define CY_U3P_LPP_SCK_INTR_ADDRESS(n)                      (0xe0008010 + ((n) * (0x0080)))
#define CY_U3P_LPP_SCK_INTR(n)                              (*(uvint32_t *)(0xe0008010 + ((n) * 0x0080)))
#define CY_U3P_LPP_SCK_INTR_DEFAULT                         (0x00000000)

/*
   Indicates that a produce event has been received or transmitted since
   last cleared.
 */
#define CY_U3P_LPP_PRODUCE_EVENT                            (1u << 0) /* <0:0> W1S:RW1C:0:N/A */


/*
   Indicates that a consume event has been received or transmitted since
   last cleared.
 */
#define CY_U3P_LPP_CONSUME_EVENT                            (1u << 1) /* <1:1> W1S:RW1C:0:N/A */


/*
   Indicates that dscr_count has fallen below its watermark dscr_low.  If
   dscr_count wraps around to 255 dscr_is_low will remain asserted until
   cleared by s/w
 */
#define CY_U3P_LPP_DSCR_IS_LOW                              (1u << 2) /* <2:2> W1S:RW1C:0:N/A */


/*
   Indicates the no descriptor is available.  Not available means that the
   current descriptor number is 0xFFFF.  Note that this bit will remain asserted
   until cleared by s/w, regardless of whether a new descriptor number is
   loaded.
 */
#define CY_U3P_LPP_DSCR_NOT_AVL                             (1u << 3) /* <3:3> W1S:RW1C:0:N/A */


/*
   Indicates the socket has stalled, waiting for an event signaling its descriptor
   has become available. Note that this bit will remain asserted until cleared
   by s/w, regardless of whether the socket resumes.
 */
#define CY_U3P_LPP_STALL                                    (1u << 4) /* <4:4> W1S:RW1C:0:N/A */


/*
   Indicates the socket has gone into suspend mode.  This may be caused by
   any hardware initiated condition (e.g. DSCR_NOT_AVL, any of the SUSP_*)
   or by setting GO_SUSPEND=1.  Note that this bit will remain asserted until
   cleared by s/w, regardless of whether the suspend condition is resolved.
   Note that the socket resumes only when SCK_INTR[9:5]=0 and GO_SUSPEND=0.
 */
#define CY_U3P_LPP_SUSPEND                                  (1u << 5) /* <5:5> W1S:RW1C:0:N/A */


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
#define CY_U3P_LPP_ERROR                                    (1u << 6) /* <6:6> W1S:RW1C:0:N/A */


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
#define CY_U3P_LPP_TRANS_DONE                               (1u << 7) /* <7:7> W1S:RW1C:0:N/A */


/*
   Indicates that the (egress) socket was suspended because of SUSP_PARTIAL
   condition.  Note that the socket resumes only when SCK_INTR[9:5]=0 and
   GO_SUSPEND=0.
 */
#define CY_U3P_LPP_PARTIAL_BUF                              (1u << 8) /* <8:8> W1S:RW1C:0:N/A */


/*
   Indicates that the socket was suspended because of SUSP_LAST condition.
    Note that the socket resumes only when SCK_INTR[9:5]=0 and GO_SUSPEND=0.
 */
#define CY_U3P_LPP_LAST_BUF                                 (1u << 9) /* <9:9> W1S:RW1C:0:N/A */



/*
   Socket Interrupt Mask Register
 */
#define CY_U3P_LPP_SCK_INTR_MASK_ADDRESS(n)                 (0xe0008014 + ((n) * (0x0080)))
#define CY_U3P_LPP_SCK_INTR_MASK(n)                         (*(uvint32_t *)(0xe0008014 + ((n) * 0x0080)))
#define CY_U3P_LPP_SCK_INTR_MASK_DEFAULT                    (0x00000000)

/*
   1: Report interrupt to CPU
 */
#define CY_U3P_LPP_PRODUCE_EVENT                            (1u << 0) /* <0:0> R:RW:0:N/A */


/*
   1: Report interrupt to CPU
 */
#define CY_U3P_LPP_CONSUME_EVENT                            (1u << 1) /* <1:1> R:RW:0:N/A */


/*
   1: Report interrupt to CPU
 */
#define CY_U3P_LPP_DSCR_IS_LOW                              (1u << 2) /* <2:2> R:RW:0:N/A */


/*
   1: Report interrupt to CPU
 */
#define CY_U3P_LPP_DSCR_NOT_AVL                             (1u << 3) /* <3:3> R:RW:0:N/A */


/*
   1: Report interrupt to CPU
 */
#define CY_U3P_LPP_STALL                                    (1u << 4) /* <4:4> R:RW:0:N/A */


/*
   1: Report interrupt to CPU
 */
#define CY_U3P_LPP_SUSPEND                                  (1u << 5) /* <5:5> R:RW:0:N/A */


/*
   1: Report interrupt to CPU
 */
#define CY_U3P_LPP_ERROR                                    (1u << 6) /* <6:6> R:RW:0:N/A */


/*
   1: Report interrupt to CPU
 */
#define CY_U3P_LPP_TRANS_DONE                               (1u << 7) /* <7:7> R:RW:0:N/A */


/*
   1: Report interrupt to CPU
 */
#define CY_U3P_LPP_PARTIAL_BUF                              (1u << 8) /* <8:8> R:RW:0:N/A */


/*
   1: Report interrupt to CPU
 */
#define CY_U3P_LPP_LAST_BUF                                 (1u << 9) /* <9:9> R:RW:0:N/A */



/*
   Descriptor buffer base address register
 */
#define CY_U3P_LPP_DSCR_BUFFER_ADDRESS(n)                   (0xe0008020 + ((n) * (0x0080)))
#define CY_U3P_LPP_DSCR_BUFFER(n)                           (*(uvint32_t *)(0xe0008020 + ((n) * 0x0080)))
#define CY_U3P_LPP_DSCR_BUFFER_DEFAULT                      (0x00000000)

/*
   The base address of the buffer where data is written.  This address is
   not necessarily word-aligned to allow for header/trailer/length modification.
 */
#define CY_U3P_LPP_BUFFER_ADDR_MASK                         (0xffffffff) /* <0:31> RW:RW:X:N/A */
#define CY_U3P_LPP_BUFFER_ADDR_POS                          (0)



/*
   Descriptor synchronization pointers register
 */
#define CY_U3P_LPP_DSCR_SYNC_ADDRESS(n)                     (0xe0008024 + ((n) * (0x0080)))
#define CY_U3P_LPP_DSCR_SYNC(n)                             (*(uvint32_t *)(0xe0008024 + ((n) * 0x0080)))
#define CY_U3P_LPP_DSCR_SYNC_DEFAULT                        (0x00000000)

/*
   The socket number of the consuming socket to which the produce event shall
   be sent.
   If cons_ip and cons_sck matches the socket's IP and socket number then
   the matching socket becomes consuming socket.
 */
#define CY_U3P_LPP_CONS_SCK_MASK                            (0x000000ff) /* <0:7> RW:RW:X:N/A */
#define CY_U3P_LPP_CONS_SCK_POS                             (0)


/*
   The IP number of the consuming socket to which the produce event shall
   be sent.  Use 0x3F to designate ARM CPU (so software) as consumer; the
   event will be lost in this case and an interrupt should also be generated
   to observe this condition.
 */
#define CY_U3P_LPP_CONS_IP_MASK                             (0x00003f00) /* <8:13> RW:RW:X:N/A */
#define CY_U3P_LPP_CONS_IP_POS                              (8)


/*
   Enable sending of a consume events from this descriptor only.  Events
   are sent only if SCK_STATUS.en_consume_ev=1.  When events are disabled,
   the adapter also does not update the descriptor in memory to clear its
   occupied bit.
 */
#define CY_U3P_LPP_EN_CONS_EVENT                            (1u << 14) /* <14:14> RW:RW:X:N/A */


/*
   Enable generation of a consume event interrupt for this descriptor only.
    This interrupt will only be seen by the CPU if SCK_STATUS.int_mask has
   this interrupt enabled as well.  Note that this flag influences the logging
   of the interrupt in SCK_STATUS; it has no effect on the reporting of the
   interrupt to the CPU like SCK_STATUS.int_mask does.
 */
#define CY_U3P_LPP_EN_CONS_INT                              (1u << 15) /* <15:15> RW:RW:X:N/A */


/*
   The socket number of the producing socket to which the consume event shall
   be sent. If prod_ip and prod_sck matches the socket's IP and socket number
   then the matching socket becomes consuming socket.
 */
#define CY_U3P_LPP_PROD_SCK_MASK                            (0x00ff0000) /* <16:23> RW:RW:X:N/A */
#define CY_U3P_LPP_PROD_SCK_POS                             (16)


/*
   The IP number of the producing socket to which the consume event shall
   be sent. Use 0x3F to designate ARM CPU (so software) as producer; the
   event will be lost in this case and an interrupt should also be generated
   to observe this condition.
 */
#define CY_U3P_LPP_PROD_IP_MASK                             (0x3f000000) /* <24:29> RW:RW:X:N/A */
#define CY_U3P_LPP_PROD_IP_POS                              (24)


/*
   Enable sending of a produce events from this descriptor only.  Events
   are sent only if SCK_STATUS.en_produce_ev=1.  If 0, events will be suppressed,
   and the descriptor will not be copied back into memory when completed.
 */
#define CY_U3P_LPP_EN_PROD_EVENT                            (1u << 30) /* <30:30> RW:RW:X:N/A */


/*
   Enable generation of a produce event interrupt for this descriptor only.
   This interrupt will only be seen by the CPU if SCK_STATUS. int_mask has
   this interrupt enabled as well.  Note that this flag influences the logging
   of the interrupt in SCK_STATUS; it has no effect on the reporting of the
   interrupt to the CPU like SCK_STATUS.int_mask does.
 */
#define CY_U3P_LPP_EN_PROD_INT                              (1u << 31) /* <31:31> RW:RW:X:N/A */



/*
   Descriptor Chain Pointers Register
 */
#define CY_U3P_LPP_DSCR_CHAIN_ADDRESS(n)                    (0xe0008028 + ((n) * (0x0080)))
#define CY_U3P_LPP_DSCR_CHAIN(n)                            (*(uvint32_t *)(0xe0008028 + ((n) * 0x0080)))
#define CY_U3P_LPP_DSCR_CHAIN_DEFAULT                       (0x00000000)

/*
   Descriptor number of the next task for consumer. A value of 0xFFFF signals
   end of this list.
 */
#define CY_U3P_LPP_RD_NEXT_DSCR_MASK                        (0x0000ffff) /* <0:15> RW:RW:X:N/A */
#define CY_U3P_LPP_RD_NEXT_DSCR_POS                         (0)


/*
   Descriptor number of the next task for producer. A value of 0xFFFF signals
   end of this list.
 */
#define CY_U3P_LPP_WR_NEXT_DSCR_MASK                        (0xffff0000) /* <16:31> RW:RW:X:N/A */
#define CY_U3P_LPP_WR_NEXT_DSCR_POS                         (16)



/*
   Descriptor Size Register
 */
#define CY_U3P_LPP_DSCR_SIZE_ADDRESS(n)                     (0xe000802c + ((n) * (0x0080)))
#define CY_U3P_LPP_DSCR_SIZE(n)                             (*(uvint32_t *)(0xe000802c + ((n) * 0x0080)))
#define CY_U3P_LPP_DSCR_SIZE_DEFAULT                        (0x00000000)

/*
   A marker that is provided by s/w and can be observed by the IP.  It's
   meaning is defined by the IP that uses it.  This bit has no effect on
   the other DMA mechanisms.
 */
#define CY_U3P_LPP_MARKER                                   (1u << 0) /* <0:0> RW:RW:X:N/A */


/*
   A marker indicating this descriptor refers to the last buffer of a packet
   or transfer. Packets/transfers may span more than one buffer.  The producing
   IP provides this marker by providing the EOP signal to its DMA adapter.
    The consuming IP observes this marker by inspecting its EOP return signal
   from its own DMA adapter. For more information see section on packets,
   buffers and transfers in DMA chapter.
 */
#define CY_U3P_LPP_EOP                                      (1u << 1) /* <1:1> RW:RW:X:N/A */


/*
   Indicates the buffer data is valid (0) or in error (1).
 */
#define CY_U3P_LPP_BUFFER_ERROR                             (1u << 2) /* <2:2> RW:RW:X:N/A */


/*
   Indicates the buffer is in use (1) or empty (0).  A consumer will interpret
   this as:
   0: Buffer is empty, wait until filled.
   1: Buffer has data that can be consumed
   A produce will interpret this as:
   0: Buffer is ready to be filled
   1: Buffer is occupied, wait until empty
 */
#define CY_U3P_LPP_BUFFER_OCCUPIED                          (1u << 3) /* <3:3> RW:RW:X:N/A */


/*
   The size of the buffer in multiples of 16 bytes
 */
#define CY_U3P_LPP_BUFFER_SIZE_MASK                         (0x0000fff0) /* <4:15> RW:RW:X:N/A */
#define CY_U3P_LPP_BUFFER_SIZE_POS                          (4)


/*
   The number of data bytes present in the buffer.  An occupied buffer is
   not always full, in particular when variable length packets are transferred.
 */
#define CY_U3P_LPP_BYTE_COUNT_MASK                          (0xffff0000) /* <16:31> RW:RW:X:N/A */
#define CY_U3P_LPP_BYTE_COUNT_POS                           (16)



/*
   Event Communication Register
 */
#define CY_U3P_LPP_EVENT_ADDRESS(n)                         (0xe000807c + ((n) * (0x0080)))
#define CY_U3P_LPP_EVENT(n)                                 (*(uvint32_t *)(0xe000807c + ((n) * 0x0080)))
#define CY_U3P_LPP_EVENT_DEFAULT                            (0x00000000)

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
#define CY_U3P_LPP_SCK_INTR0_ADDRESS                        (0xe000ff00)
#define CY_U3P_LPP_SCK_INTR0                                (*(uvint32_t *)(0xe000ff00))
#define CY_U3P_LPP_SCK_INTR0_DEFAULT                        (0x00000000)

/*
   Socket <x> asserts interrupt when bit <x> is set in this vector.  Multiple
   bits may be set to 1 simultaneously.
   This register is only as wide as the number of socket in the adapter;
   256 is just the maximum width.  All other bits always return 0.
 */
#define CY_U3P_LPP_SCKINTR_L_MASK                           (0xffffffff) /* <0:31> W:R:0:N/A */
#define CY_U3P_LPP_SCKINTR_L_POS                            (0)



/*
   Socket Interrupt Request Register
 */
#define CY_U3P_LPP_SCK_INTR1_ADDRESS                        (0xe000ff04)
#define CY_U3P_LPP_SCK_INTR1                                (*(uvint32_t *)(0xe000ff04))
#define CY_U3P_LPP_SCK_INTR1_DEFAULT                        (0x00000000)

/*
   Socket <x> asserts interrupt when bit <x> is set in this vector.  Multiple
   bits may be set to 1 simultaneously.
   This register is only as wide as the number of socket in the adapter;
   256 is just the maximum width.  All other bits always return 0.
 */
#define CY_U3P_LPP_SCKINTR_H_L_MASK                         (0xffffffff) /* <0:31> W:R:0:N/A */
#define CY_U3P_LPP_SCKINTR_H_L_POS                          (0)



/*
   Socket Interrupt Request Register
 */
#define CY_U3P_LPP_SCK_INTR2_ADDRESS                        (0xe000ff08)
#define CY_U3P_LPP_SCK_INTR2                                (*(uvint32_t *)(0xe000ff08))
#define CY_U3P_LPP_SCK_INTR2_DEFAULT                        (0x00000000)

/*
   Socket <x> asserts interrupt when bit <x> is set in this vector.  Multiple
   bits may be set to 1 simultaneously.
   This register is only as wide as the number of socket in the adapter;
   256 is just the maximum width.  All other bits always return 0.
 */
#define CY_U3P_LPP_SCKINTR_H_L_MASK                         (0xffffffff) /* <0:31> W:R:0:N/A */
#define CY_U3P_LPP_SCKINTR_H_L_POS                          (0)



/*
   Socket Interrupt Request Register
 */
#define CY_U3P_LPP_SCK_INTR3_ADDRESS                        (0xe000ff0c)
#define CY_U3P_LPP_SCK_INTR3                                (*(uvint32_t *)(0xe000ff0c))
#define CY_U3P_LPP_SCK_INTR3_DEFAULT                        (0x00000000)

/*
   Socket <x> asserts interrupt when bit <x> is set in this vector.  Multiple
   bits may be set to 1 simultaneously.
   This register is only as wide as the number of socket in the adapter;
   256 is just the maximum width.  All other bits always return 0.
 */
#define CY_U3P_LPP_SCKINTR_H_L_MASK                         (0xffffffff) /* <0:31> W:R:0:N/A */
#define CY_U3P_LPP_SCKINTR_H_L_POS                          (0)



/*
   Socket Interrupt Request Register
 */
#define CY_U3P_LPP_SCK_INTR4_ADDRESS                        (0xe000ff10)
#define CY_U3P_LPP_SCK_INTR4                                (*(uvint32_t *)(0xe000ff10))
#define CY_U3P_LPP_SCK_INTR4_DEFAULT                        (0x00000000)

/*
   Socket <x> asserts interrupt when bit <x> is set in this vector.  Multiple
   bits may be set to 1 simultaneously.
   This register is only as wide as the number of socket in the adapter;
   256 is just the maximum width.  All other bits always return 0.
 */
#define CY_U3P_LPP_SCKINTR_H_L_MASK                         (0xffffffff) /* <0:31> W:R:0:N/A */
#define CY_U3P_LPP_SCKINTR_H_L_POS                          (0)



/*
   Socket Interrupt Request Register
 */
#define CY_U3P_LPP_SCK_INTR5_ADDRESS                        (0xe000ff14)
#define CY_U3P_LPP_SCK_INTR5                                (*(uvint32_t *)(0xe000ff14))
#define CY_U3P_LPP_SCK_INTR5_DEFAULT                        (0x00000000)

/*
   Socket <x> asserts interrupt when bit <x> is set in this vector.  Multiple
   bits may be set to 1 simultaneously.
   This register is only as wide as the number of socket in the adapter;
   256 is just the maximum width.  All other bits always return 0.
 */
#define CY_U3P_LPP_SCKINTR_H_L_MASK                         (0xffffffff) /* <0:31> W:R:0:N/A */
#define CY_U3P_LPP_SCKINTR_H_L_POS                          (0)



/*
   Socket Interrupt Request Register
 */
#define CY_U3P_LPP_SCK_INTR6_ADDRESS                        (0xe000ff18)
#define CY_U3P_LPP_SCK_INTR6                                (*(uvint32_t *)(0xe000ff18))
#define CY_U3P_LPP_SCK_INTR6_DEFAULT                        (0x00000000)

/*
   Socket <x> asserts interrupt when bit <x> is set in this vector.  Multiple
   bits may be set to 1 simultaneously.
   This register is only as wide as the number of socket in the adapter;
   256 is just the maximum width.  All other bits always return 0.
 */
#define CY_U3P_LPP_SCKINTR_H_L_MASK                         (0xffffffff) /* <0:31> W:R:0:N/A */
#define CY_U3P_LPP_SCKINTR_H_L_POS                          (0)



/*
   Socket Interrupt Request Register
 */
#define CY_U3P_LPP_SCK_INTR7_ADDRESS                        (0xe000ff1c)
#define CY_U3P_LPP_SCK_INTR7                                (*(uvint32_t *)(0xe000ff1c))
#define CY_U3P_LPP_SCK_INTR7_DEFAULT                        (0x00000000)

/*
   Socket <x> asserts interrupt when bit <x> is set in this vector.  Multiple
   bits may be set to 1 simultaneously.
   This register is only as wide as the number of socket in the adapter;
   256 is just the maximum width.  All other bits always return 0.
 */
#define CY_U3P_LPP_SCKINTR_H_MASK                           (0xffffffff) /* <0:31> W:R:0:N/A */
#define CY_U3P_LPP_SCKINTR_H_POS                            (0)



#endif /* _INCLUDED_LPP_REGS_H_ */

/*[]*/

