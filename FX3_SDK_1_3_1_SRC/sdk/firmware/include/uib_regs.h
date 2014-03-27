/****************************************************************************
 *
 * File: uib_regs.h
 *
 * Copyright (c) 2010-13 Cypress Semiconductor. All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION WHICH IS THE PROPERTY OF CYPRESS.
 *
 * Description:
 *   USB 2.0 (Device and Host) registers for the EZ-USB FX3 Device.
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

#ifndef _INCLUDED_UIB_REGS_H_
#define _INCLUDED_UIB_REGS_H_

#include <cyu3types.h>

#define UIB_BASE_ADDR                            (0xe0030000)

typedef struct
{
    uvint32_t intr;                               /* 0xe0030000 */
    uvint32_t intr_mask;                          /* 0xe0030004 */
    uvint32_t rsrvd0[1024];
    uvint32_t phy_clk_and_test;                   /* 0xe0031008 */
    uvint32_t reserved[2];
    uvint32_t phy_chirp;                          /* 0xe0031014 */
    uvint32_t rsrvd1[250];
    uvint32_t dev_cs;                             /* 0xe0031400 */
    uvint32_t dev_framecnt;                       /* 0xe0031404 */
    uvint32_t dev_pwr_cs;                         /* 0xe0031408 */
    uvint32_t dev_setupdat0;                      /* 0xe003140c */
    uvint32_t dev_setupdat1;                      /* 0xe0031410 */
    uvint32_t dev_toggle;                         /* 0xe0031414 */
    uvint32_t dev_epi_cs[16];                     /* 0xe0031418 */
    uvint32_t dev_epi_xfer_cnt[16];               /* 0xe0031458 */
    uvint32_t dev_epo_cs[16];                     /* 0xe0031498 */
    uvint32_t dev_epo_xfer_cnt[16];               /* 0xe00314d8 */
    uvint32_t dev_ctl_intr_mask;                  /* 0xe0031518 */
    uvint32_t dev_ctl_intr;                       /* 0xe003151c */
    uvint32_t dev_ep_intr_mask;                   /* 0xe0031520 */
    uvint32_t dev_ep_intr;                        /* 0xe0031524 */
    uvint32_t rsrvd2[182];
    uvint32_t chgdet_ctrl;                        /* 0xe0031800 */
    uvint32_t chgdet_intr;                        /* 0xe0031804 */
    uvint32_t chgdet_intr_mask;                   /* 0xe0031808 */
    uvint32_t otg_ctrl;                           /* 0xe003180c */
    uvint32_t otg_intr;                           /* 0xe0031810 */
    uvint32_t otg_intr_mask;                      /* 0xe0031814 */
    uvint32_t otg_timer;                          /* 0xe0031818 */
    uvint32_t rsrvd3[249];
    uvint32_t eepm_cs;                            /* 0xe0031c00 */
    uvint32_t iepm_cs;                            /* 0xe0031c04 */
    uvint32_t iepm_mult;                          /* 0xe0031c08 */
    uvint32_t rsrvd4[13];
    uvint32_t eepm_endpoint[16];                  /* 0xe0031c40 */
    uvint32_t iepm_endpoint[16];                  /* 0xe0031c80 */
    uvint32_t iepm_fifo;                          /* 0xe0031cc0 */
    uvint32_t rsrvd5[207];
    uvint32_t host_cs;                            /* 0xe0032000 */
    uvint32_t host_ep_intr;                       /* 0xe0032004 */
    uvint32_t host_ep_intr_mask;                  /* 0xe0032008 */
    uvint32_t host_toggle;                        /* 0xe003200c */
    uvint32_t host_shdl_cs;                       /* 0xe0032010 */
    uvint32_t host_shdl_sleep;                    /* 0xe0032014 */
    uvint32_t host_resp_base;                     /* 0xe0032018 */
    uvint32_t host_resp_cs;                       /* 0xe003201c */
    uvint32_t host_active_ep;                     /* 0xe0032020 */
    uvint32_t ohci_revision;                      /* 0xe0032024 */
    uvint32_t ohci_control;                       /* 0xe0032028 */
    uvint32_t ohci_command_status;                /* 0xe003202c */
    uvint32_t ohci_interrupt_status;              /* 0xe0032030 */
    uvint32_t ohci_interrupt_enable;              /* 0xe0032034 */
    uvint32_t ohci_interrupt_disable;             /* 0xe0032038 */
    uvint32_t ohci_fm_interval;                   /* 0xe003203c */
    uvint32_t ohci_fm_remaining;                  /* 0xe0032040 */
    uvint32_t ohci_fm_number;                     /* 0xe0032044 */
    uvint32_t ohci_periodic_start;                /* 0xe0032048 */
    uvint32_t ohci_ls_threshold;                  /* 0xe003204c */
    uvint32_t reserved1;
    uvint32_t ohci_rh_port_status;                /* 0xe0032054 */
    uvint32_t ohci_eof;                           /* 0xe0032058 */
    uvint32_t ehci_hccparams;                     /* 0xe003205c */
    uvint32_t ehci_usbcmd;                        /* 0xe0032060 */
    uvint32_t ehci_usbsts;                        /* 0xe0032064 */
    uvint32_t ehci_usbintr;                       /* 0xe0032068 */
    uvint32_t ehci_frindex;                       /* 0xe003206c */
    uvint32_t ehci_configflag;                    /* 0xe0032070 */
    uvint32_t ehci_portsc;                        /* 0xe0032074 */
    uvint32_t ehci_eof;                           /* 0xe0032078 */
    uvint32_t shdl_chng_type;                     /* 0xe003207c */
    uvint32_t shdl_state_machine;                 /* 0xe0032080 */
    uvint32_t shdl_internal_status;               /* 0xe0032084 */
    uvint32_t rsrvd6[222];
    struct
    {
        uvint32_t shdl_ohci0;                     /* 0xe0032400 */
        uvint32_t shdl_ohci1;                     /* 0xe0032404 */
        uvint32_t shdl_ohci2;                     /* 0xe0032408 */
    } ohci_shdl[64];
    uvint32_t rsrvd7[64];
    struct
    {
        uvint32_t shdl_ehci0;                     /* 0xe0032800 */
        uvint32_t shdl_ehci1;                     /* 0xe0032804 */
        uvint32_t shdl_ehci2;                     /* 0xe0032808 */
    } ehci_shdl[64];
    uvint32_t rsrvd8[5376];
    uvint32_t id;                                 /* 0xe0037f00 */
    uvint32_t power;                              /* 0xe0037f04 */
    uvint32_t rsrvd9[62];
    struct
    {
        uvint32_t dscr;                           /* 0xe0038000 */
        uvint32_t size;                           /* 0xe0038004 */
        uvint32_t count;                          /* 0xe0038008 */
        uvint32_t status;                         /* 0xe003800c */
        uvint32_t intr;                           /* 0xe0038010 */
        uvint32_t intr_mask;                      /* 0xe0038014 */
        uvint32_t rsrvd10[2];
        uvint32_t dscr_buffer;                    /* 0xe0038020 */
        uvint32_t dscr_sync;                      /* 0xe0038024 */
        uvint32_t dscr_chain;                     /* 0xe0038028 */
        uvint32_t dscr_size;                      /* 0xe003802c */
        uvint32_t rsrvd11[19];
        uvint32_t event;                          /* 0xe003807c */
    } sck[16];
    uvint32_t rsrvd12[7616];
    uvint32_t sck_intr0;                          /* 0xe003ff00 */
    uvint32_t sck_intr1;                          /* 0xe003ff04 */
    uvint32_t sck_intr2;                          /* 0xe003ff08 */
    uvint32_t sck_intr3;                          /* 0xe003ff0c */
    uvint32_t sck_intr4;                          /* 0xe003ff10 */
    uvint32_t sck_intr5;                          /* 0xe003ff14 */
    uvint32_t sck_intr6;                          /* 0xe003ff18 */
    uvint32_t sck_intr7;                          /* 0xe003ff1c */
    uvint32_t rsrvd13[56];
} UIB_REGS_T, *PUIB_REGS_T;

#define UIB        ((PUIB_REGS_T) UIB_BASE_ADDR)


/*
   USB Interrupt Register
 */
#define CY_U3P_UIB_INTR_ADDRESS                             (0xe0030000)
#define CY_U3P_UIB_INTR                                     (*(uvint32_t *)(0xe0030000))
#define CY_U3P_UIB_INTR_DEFAULT                             (0x00000000)

/*
   Host INT Status Register Interrupt
 */
#define CY_U3P_UIB_HOST_INT                                 (1u << 0) /* <0:0> W:R:X:N/A */


/*
   Host EP Interrupt
 */
#define CY_U3P_UIB_HOST_EP_INT                              (1u << 1) /* <1:1> W:R:X:N/A */


/*
   EHCI Interrupt
 */
#define CY_U3P_UIB_EHCI_INT                                 (1u << 2) /* <2:2> W:R:X:N/A */


/*
   OHCI Interrupt
 */
#define CY_U3P_UIB_OHCI_INT                                 (1u << 3) /* <3:3> W:R:X:N/A */


/*
   Device EP Interrupt
 */
#define CY_U3P_UIB_DEV_EP_INT                               (1u << 4) /* <4:4> W:R:X:N/A */


/*
   Device USB Control Interrupt
 */
#define CY_U3P_UIB_DEV_CTL_INT                              (1u << 5) /* <5:5> W:R:X:N/A */


/*
   USB OTG Interrupt
 */
#define CY_U3P_UIB_OTG_INT                                  (1u << 6) /* <6:6> W:R:X:N/A */


/*
   USB Charger Detect Interrupt
 */
#define CY_U3P_UIB_CHGDET_INT                               (1u << 7) /* <7:7> W:R:X:N/A */


/*
   SuperSpeed Link Controller Interrupt
 */
#define CY_U3P_UIB_LNK_INT                                  (1u << 8) /* <8:8> RW:R:X:N/A */


/*
   SuperSpeed Protocol Layer Interrupt
 */
#define CY_U3P_UIB_PROT_INT                                 (1u << 9) /* <9:9> RW:R:X:N/A */


/*
   SuperSpeed Device Endpoint Interrupt
 */
#define CY_U3P_UIB_PROT_EP_INT                              (1u << 10) /* <10:10> RW:R:X:N/A */


/*
   SuperSpeed Egress EPM Interrupt
 */
#define CY_U3P_UIB_EPM_URUN                                 (1u << 11) /* <11:11> RW1S:RW1C:X:N/A */


/*
   SuperSpeed Egress EPM Interrupt
 */
#define CY_U3P_UIB_EPM_URUN_TIMEOUT                         (1u << 12) /* <12:12> RW1S:RW1C:X:N/A */



/*
   USB Interrupt Mask Register
 */
#define CY_U3P_UIB_INTR_MASK_ADDRESS                        (0xe0030004)
#define CY_U3P_UIB_INTR_MASK                                (*(uvint32_t *)(0xe0030004))
#define CY_U3P_UIB_INTR_MASK_DEFAULT                        (0x00000000)


/*
   USB PHY clocks and testability configuration
 */
#define CY_U3P_UIB_PHY_CLK_AND_TEST_ADDRESS                 (0xe0031008)
#define CY_U3P_UIB_PHY_CLK_AND_TEST                         (*(uvint32_t *)(0xe0031008))
#define CY_U3P_UIB_PHY_CLK_AND_TEST_DEFAULT                 (0x88800011)

/*
   Data Bus Size
       1 = 16-bit
       0 = 8-bit
   (only 16b mode is tested)
 */
#define CY_U3P_UIB_DATABUS16_8                              (1u << 0) /* <0:0> R:RW:1:No */


/*
   Enable 480MHz Clock Output in Suspend
 */
#define CY_U3P_UIB_ONCLOCK                                  (1u << 1) /* <1:1> R:RW:0:No */


/*
   Vendor Control Register Load
   Active Low
 */
#define CY_U3P_UIB_VLOAD                                    (1u << 4) /* <4:4> R:RW:1:Yes */


/*
   MIPS PHY Enable Sampling of ID line by PHY
 */
#define CY_U3P_UIB_IDPULLUP                                 (1u << 19) /* <19:19> R:RW:0:No */


/*
   MIPS PHY Digital Value of ID line
 */
#define CY_U3P_UIB_IDDIG                                    (1u << 20) /* <20:20> RW:R:N:No */


/*
   MIPS PHY Charger Detector Power On Control
   (not tested)
 */
#define CY_U3P_UIB_CHGRDETON                                (1u << 21) /* <21:21> R:RW:0:No */


/*
   MIPS PHY Chager Detector Enable
   (not tested, used charger detection in CHGDET_CTRL instead)
 */
#define CY_U3P_UIB_CHGRDETEN                                (1u << 22) /* <22:22> R:RW:0:No */


/*
   MIPS PHY Charger Detector Mode
 */
#define CY_U3P_UIB_CHGRMODE                                 (1u << 23) /* <23:23> R:RW:1:No */


/*
   MIPS PHY Charger Detector Output (0=host detected  1=charger detected)
 */
#define CY_U3P_UIB_CHGRDET                                  (1u << 24) /* <24:24> RW:R:0:No */


/*
   Vdat source enable for charger detect
 */
#define CY_U3P_UIB_VDATSRCEEN                               (1u << 25) /* <25:25> R:RW:0:No */


/*
   Reset Value applied to USB2 PHY
 */
#define CY_U3P_UIB_RESET                                    (1u << 27) /* <27:27> R:RW:1:No */


/*
   Suspend value applied to USB2 PHY (active low)
 */
#define CY_U3P_UIB_SUSPEND_N                                (1u << 29) /* <29:29> R:RW:0:No */


/*
   MIPS PHY Enable Data Contact Detect Circuitry
 */
#define CY_U3P_UIB_ON_DCD                                   (1u << 30) /* <30:30> R:RW:0:No */


/*
   MIPS PHY Enable External Series Switch. Indicates the presence of an external
   D+/D- switch, irrespective of whether that switch is enabled. The switch
   itself is controlled by CHGDET_CTRL : ANALOG_SWITCH.
 */
#define CY_U3P_UIB_EN_SWITCH                                (1u << 31) /* <31:31> R:RW:1:No */



/*
   USB PHY Chirp control register
 */
#define CY_U3P_UIB_PHY_CHIRP_ADDRESS                        (0xe0031014)
#define CY_U3P_UIB_PHY_CHIRP                                (*(uvint32_t *)(0xe0031014))
#define CY_U3P_UIB_PHY_CHIRP_DEFAULT                        (0x00000000)

/*
   Set chirp state machine state (if OVERRIDE_FSM == 1)
      5'h00:  FULL_SPEED
      5'h01:  FULL_SPEED_SUSPEND
      5'h02:  SWITCH_XCVR_TO_HSPD
      5'h03:  CHIRP
      5'h04:  LOOK_FOR_K1
      5'h05:  LOOK_FOR_J1
      5'h06:  LOOK_FOR_K2
      5'h07:  LOOK_FOR_J2
      5'h08:  LOOK_FOR_K3
      5'h09:  LOOK_FOR_J3
      5'h0A:  SWITCH_XCVR_TO_FSPD
      5'h0B:  WAIT_END_RESET_FSPD
      5'h0D:  HIGH_SPEED
      5'h0E:  SWITCH_XCVR_TO_FSPD_CHK_RESET
      5'h0F:  CHECK_RESET
      5'h10:  HIGH_SPEED_SUSPEND
      5'h11:  WAIT_END_OF_RESUME
      5'h12:  WAIT_PP_AFTER_RESUME
 */
#define CY_U3P_UIB_CHIRP_STATE_MASK                         (0x0000001f) /* <0:4> R:RW:0:No */
#define CY_U3P_UIB_CHIRP_STATE_POS                          (0)


/*
   USB PHY termsel value (if OVERRIDE_CHIRP == 1)
 */
#define CY_U3P_UIB_FORCE_TERMSEL                            (1u << 5) /* <5:5> R:RW:0:No */


/*
   USB PHY xcvrsel value (if OVERRIDE_CHIRP == 1)
 */
#define CY_U3P_UIB_FORCE_XCVRSEL_MASK                       (0x000000c0) /* <6:7> R:RW:0:No */
#define CY_U3P_UIB_FORCE_XCVRSEL_POS                        (6)


/*
   USB PHY force chirp K (if OVERRIDE_CHIRP == 1)
 */
#define CY_U3P_UIB_FORCE_CHIRP_K                            (1u << 8) /* <8:8> R:RW:0:No */


/*
   USB PHY force chirp J (if OVERRIDE_CHIRP == 1)
 */
#define CY_U3P_UIB_FORCE_CHIRP_J                            (1u << 9) /* <9:9> R:RW:0:No */


/*
   USB PHY Receive Valid
 */
#define CY_U3P_UIB_PHY_RXVALID                              (1u << 10) /* <10:10> RW:R:N:No */


/*
   USB PHY Receive Active
 */
#define CY_U3P_UIB_PHY_RXACTIVE                             (1u << 11) /* <11:11> RW:R:N:No */


/*
   Override chirp state machine
 */
#define CY_U3P_UIB_OVERRIDE_FSM                             (1u << 30) /* <30:30> R:RW:0:No */


/*
   Override chirp outputs
 */
#define CY_U3P_UIB_OVERRIDE_CHIRP                           (1u << 31) /* <31:31> R:RW:0:No */



/*
   Device controller Master Control & Status
 */
#define CY_U3P_UIB_DEV_CS_ADDRESS                           (0xe0031400)
#define CY_U3P_UIB_DEV_CS                                   (*(uvint32_t *)(0xe0031400))
#define CY_U3P_UIB_DEV_CS_DEFAULT                           (0x00000004)

/*
   Error interrupt limit (COUNT >= LIMIT will cause UIB_ERR_INTR.ERRLIMIT
   interrupt)
 */
#define CY_U3P_UIB_ERR_LIMIT_MASK                           (0x000000ff) /* <0:7> R:RW:4:No */
#define CY_U3P_UIB_ERR_LIMIT_POS                            (0)


/*
   Number of errors detected
   To clear the error count write 0 to these bits
 */
#define CY_U3P_UIB_COUNT_MASK                               (0x0000ff00) /* <8:15> RW:RW:0:No */
#define CY_U3P_UIB_COUNT_POS                                (8)


/*
   During the USB enumeration process, the host sends a device a unique 7-bit
   address, which the USB core copies into this register. The USB Core will
   automatically respond only to its assigned address.  During the USB RESET,
   this register will be cleared to zero.
 */
#define CY_U3P_UIB_DEVICEADDR_MASK                          (0x007f0000) /* <16:22> RW:RW:0:No */
#define CY_U3P_UIB_DEVICEADDR_POS                           (16)


/*
   USB Test Mode
       000:  normal operation
       001:  Test_J
       010:  Test_K
       011:  Test_SE0_NAK
       100:  Test_Packet
   [USB 2.0, §7.1.20, p 169; §9.4.9, Table 9-7, p 259]
 */
#define CY_U3P_UIB_TEST_MODE_MASK                           (0x03800000) /* <23:25> R:RW:0:No */
#define CY_U3P_UIB_TEST_MODE_POS                            (23)


/*
   Allow device to ACK SETUP data/status phase packets
 */
#define CY_U3P_UIB_SETUP_CLR_BUSY                           (1u << 26) /* <26:26> RW1S:RW1C:0:No */


/*
   Set "1" to this bit, the HW will NAK all transfers from the host in all
   endpoint1-31.
 */
#define CY_U3P_UIB_NAKALL                                   (1u << 31) /* <31:31> R:RW:0:No */



/*
   FRAMECNT register
 */
#define CY_U3P_UIB_DEV_FRAMECNT_ADDRESS                     (0xe0031404)
#define CY_U3P_UIB_DEV_FRAMECNT                             (*(uvint32_t *)(0xe0031404))
#define CY_U3P_UIB_DEV_FRAMECNT_DEFAULT                     (0x00000000)

/*
   MICROFRAME contains a count 0-7 which indicates which of the 8 125-microsecond
   micro-frames last occurred. This register is active only when FX3 is operating
   at high speed (480 Mbits/sec).
 */
#define CY_U3P_UIB_MICROFRAME_MASK                          (0x00000007) /* <0:2> RW:R:N/A:No */
#define CY_U3P_UIB_MICROFRAME_POS                           (0)


/*
   Every millisecond the host sends a SOF token indicating “Start Of Frame,”
   along with an 11-bit incrementing frame count. The FX3 copies the frame
   count into these registers at every SOF. One use of the frame count is
   to respond to the USB SYNC_FRAME Request. If the USB core detects a missing
   or garbled SOF, it generates an internal SOF and increments USBFRAMEL-USBRAMEH.
 */
#define CY_U3P_UIB_FRAMECNT_MASK                            (0x00003ff8) /* <3:13> RW:R:N/A:No */
#define CY_U3P_UIB_FRAMECNT_POS                             (3)



/*
   Power management control and status
 */
#define CY_U3P_UIB_DEV_PWR_CS_ADDRESS                       (0xe0031408)
#define CY_U3P_UIB_DEV_PWR_CS                               (*(uvint32_t *)(0xe0031408))
#define CY_U3P_UIB_DEV_PWR_CS_DEFAULT                       (0x00000008)

/*
   Set SIGRSUME=1 to drive the “K” state onto the USB bus. This should be
   done only by a device that is capable of remote wakeup, and then only
   during the SUSPEND state. To signal RESUME, set SIGRSUME=1, waits 10-15
   ms, then set SIGRSUME=0.
 */
#define CY_U3P_UIB_SIGRSUME                                 (1u << 0) /* <0:0> R:RW:0:No */


/*
   If set to 1, disable synthesizing missing SOFs.
 */
#define CY_U3P_UIB_NOSYNSOF                                 (1u << 2) /* <2:2> R:RW:0:No */


/*
   Setting this bit to “1” will disconnect HW from the USB bus by removing
   the internal 1.5 K pull-up resistor from the D+
 */
#define CY_U3P_UIB_DISCON                                   (1u << 3) /* <3:3> R:RW:1:No */


/*
   Puts the USB device controller and PHY into suspend mode (pull up connected,
   drivers, PLLs etc turned off).
 */
#define CY_U3P_UIB_DEV_SUSPEND                              (1u << 4) /* <4:4> R:RW:0:No */


/*
   Forces the device controller to enumerate as FS-only device.
 */
#define CY_U3P_UIB_FORCE_FS                                 (1u << 6) /* <6:6> R:RW:0:No */


/*
   If HSM=1, the SIE is operating in High Speed Mode
   0-1 transition of this bit causes a HSGRANT interrupt request.
 */
#define CY_U3P_UIB_HSM                                      (1u << 7) /* <7:7> RW:R:0:No */



/*
   SETUPDAT0/1 registers
 */
#define CY_U3P_UIB_DEV_SETUPDAT0_ADDRESS                    (0xe003140c)
#define CY_U3P_UIB_DEV_SETUPDAT0                            (*(uvint32_t *)(0xe003140c))
#define CY_U3P_UIB_DEV_SETUPDAT0_DEFAULT                    (0x00000000)

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
   SETUPDAT0/1 registers
 */
#define CY_U3P_UIB_DEV_SETUPDAT1_ADDRESS                    (0xe0031410)
#define CY_U3P_UIB_DEV_SETUPDAT1                            (*(uvint32_t *)(0xe0031410))
#define CY_U3P_UIB_DEV_SETUPDAT1_DEFAULT                    (0x00000000)

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
   Data toggle for endpoints
 */
#define CY_U3P_UIB_DEV_TOGGLE_ADDRESS                       (0xe0031414)
#define CY_U3P_UIB_DEV_TOGGLE                               (*(uvint32_t *)(0xe0031414))
#define CY_U3P_UIB_DEV_TOGGLE_DEFAULT                       (0x00000100)

/*
   Endpoint
 */
#define CY_U3P_UIB_ENDPOINT_MASK                            (0x0000000f) /* <0:3> R:RW:0:No */
#define CY_U3P_UIB_ENDPOINT_POS                             (0)


/*
   1=IN, 0=OUT
 */
#define CY_U3P_UIB_IO                                       (1u << 4) /* <4:4> R:RW:0:No */


/*
   Write "1" to reset data toggle to "0".  When both R and S are set, behavior
   is undefined.
 */
#define CY_U3P_UIB_R                                        (1u << 5) /* <5:5> R:RW:0:No */


/*
   Write "1" to set data toggle to "1".  When both R and S are set, behavior
   is undefined.
 */
#define CY_U3P_UIB_S                                        (1u << 6) /* <6:6> R:RW:0:No */


/*
   Current value of toggle bit for EP selected in IO/ENDPOINT
 */
#define CY_U3P_UIB_Q                                        (1u << 7) /* <7:7> RW:R:0:No */


/*
   Indicates Q is valid for selected endpoint, may be polled in s/w.
   After writing to R/S, indicates write completion.
   This bit must be cleared by s/w to initiate an operation.
 */
#define CY_U3P_UIB_TOGGLE_VALID                             (1u << 8) /* <8:8> RW1S:RW0C:1:No */



/*
   IN Endpoint Control and Status register
 */
#define CY_U3P_UIB_DEV_EPI_CS_ADDRESS(n)                    (0xe0031418 + ((n) * (0x0004)))
#define CY_U3P_UIB_DEV_EPI_CS(n)                            (*(uvint32_t *)(0xe0031418 + ((n) * 0x0004)))
#define CY_U3P_UIB_DEV_EPI_CS_DEFAULT                       (0x00004040)

/*
   Max number of bytes transferred for each token
   0=1024 (Powerup default value = 64)
 */
#define CY_U3P_UIB_EPI_PAYLOAD_MASK                         (0x000003ff) /* <0:9> R:RW:0x40:No */
#define CY_U3P_UIB_EPI_PAYLOAD_POS                          (0)


/*
   The End Point Type (Control on EP0 only)
   00: Control
   01: Isochronous
   10: Bulk
   11: Interrupt
 */
#define CY_U3P_UIB_EPI_TYPE_MASK                            (0x00000c00) /* <10:11> R:RW:0:No */
#define CY_U3P_UIB_EPI_TYPE_POS                             (10)


/*
   Number of packets to be sent per microframe (aka high-bandwidth mode ISO).
    For this implementation only EP3 and EP7 support values other than 1.
    EP3 and EP7 support values 1..3.  This field must be 0 for non-ISO endpoints.
 */
#define CY_U3P_UIB_EPI_ISOINPKS_MASK                        (0x00003000) /* <12:13> R:RW:0:No */
#define CY_U3P_UIB_EPI_ISOINPKS_POS                         (12)


/*
   Set VALID=1 to activate an endpoint, and VALID=0 to de-activate it. All
   USB endpoints default to valid. An endpoint whose VALID bit is 0 does
   not respond to any USB traffic.
 */
#define CY_U3P_UIB_EPI_VALID                                (1u << 14) /* <14:14> R:RW:1:No */


/*
   Setting this bit causes NAK on IN transactions.
 */
#define CY_U3P_UIB_EPI_NAK                                  (1u << 15) /* <15:15> R:RW:0:No */


/*
   Set this bit to “1” to stall an endpoint, and to “0” to clear a stall.
 */
#define CY_U3P_UIB_EPI_STALL                                (1u << 16) /* <16:16> R:RW:0:No */


/*
   Set whenever an IN token was ACKed by the host.
 */
#define CY_U3P_UIB_EPI_COMMIT                               (1u << 18) /* <18:18> RW1S:RW1C:0:No */


/*
   When the host sends an IN token to any Bulk IN endpoint which does not
   have data to send, the FX3 automatically NAKs the IN token and asserts
   this interrupt.
   Note that this bit will not be set if either the Endpoint NAK or global
   NAK_ALL bits are set when the NAK is transmitted
 */
#define CY_U3P_UIB_EPI_BNAK                                 (1u << 19) /* <19:19> RW1S:RW1C:0:No */


/*
   Indicates transfer is done (UIB_EPI_XFER_CNT=0).
   This bit must be cleared by s/w.
 */
#define CY_U3P_UIB_EPI_DONE                                 (1u << 20) /* <20:20> RW1S:RW1C:0:No */


/*
   Indicates a zero length packet was returned to the host in an IN transaction.
    Must be cleared by s/w.
 */
#define CY_U3P_UIB_EPI_ZERO                                 (1u << 21) /* <21:21> RW1S:RW1C:0:No */


/*
   Indicates a shorter-than-maxsize packet was received, but UIB_EPI_XFER_CNT
   did not reach 0).
 */
#define CY_U3P_UIB_EPI_SHORT                                (1u << 22) /* <22:22> RW1S:RW1C:0:No */


/*
   The ISO_ERR is set when ISO data PIDs arrive out of sequence (applies
   to high speed only), or when an an ISO packet was dropped because no data
   was available (FS or HS)
 */
#define CY_U3P_UIB_EPI_ISOERR                               (1u << 23) /* <23:23> RW1S:RW1C:0:No */


/*
   Interrupt mask for COMMIT bit
 */
#define CY_U3P_UIB_EPI_COMMIT_MASK                          (1u << 26) /* <26:26> R:RW:0:N/A */


/*
   Interrupt mask for BNAK bit
 */
#define CY_U3P_UIB_EPI_BNAK_MASK                            (1u << 27) /* <27:27> R:RW:0:N/A */


/*
   Interrupt mask for DONE bit
 */
#define CY_U3P_UIB_EPI_DONE_MASK                            (1u << 28) /* <28:28> R:RW:0:N/A */


/*
   Interrupt mask for ZERO bit
 */
#define CY_U3P_UIB_EPI_ZERO_MASK                            (1u << 29) /* <29:29> R:RW:0:N/A */


/*
   Interrupt mask for SHORT bit
 */
#define CY_U3P_UIB_EPI_SHORT_MASK                           (1u << 30) /* <30:30> R:RW:0:N/A */


/*
   Interrupt mask for ISOERR bit
 */
#define CY_U3P_UIB_EPI_ISOERR_MASK                          (1u << 31) /* <31:31> R:RW:0:N/A */



/*
   IN Endpoint remaining transfer length register
 */
#define CY_U3P_UIB_DEV_EPI_XFER_CNT_ADDRESS(n)              (0xe0031458 + ((n) * (0x0004)))
#define CY_U3P_UIB_DEV_EPI_XFER_CNT(n)                      (*(uvint32_t *)(0xe0031458 + ((n) * 0x0004)))
#define CY_U3P_UIB_DEV_EPI_XFER_CNT_DEFAULT                 (0x00000000)

/*
   Number of bytes remaining in the transfer.  This value will never go negative
   (if more bytes are transferred than remaining in counter, counter will
   go to 0).
 */
#define CY_U3P_UIB_BYTES_REMAINING_MASK                     (0x00ffffff) /* <0:23> RW:RW:0:No */
#define CY_U3P_UIB_BYTES_REMAINING_POS                      (0)



/*
   OUT Endpoint Control and Status
 */
#define CY_U3P_UIB_DEV_EPO_CS_ADDRESS(n)                    (0xe0031498 + ((n) * (0x0004)))
#define CY_U3P_UIB_DEV_EPO_CS(n)                            (*(uvint32_t *)(0xe0031498 + ((n) * 0x0004)))
#define CY_U3P_UIB_DEV_EPO_CS_DEFAULT                       (0x00004040)

/*
   Max number of bytes transferred for each token
   0=1024 (Powerup default value = 64)
 */
#define CY_U3P_UIB_EPO_PAYLOAD_MASK                         (0x000003ff) /* <0:9> R:RW:0x40:No */
#define CY_U3P_UIB_EPO_PAYLOAD_POS                          (0)


/*
   The End Point Type (Control on EP0 only)
   00: Control
   01: Isochronous
   10: Bulk
   11: Interrupt
 */
#define CY_U3P_UIB_EPO_TYPE_MASK                            (0x00000c00) /* <10:11> R:RW:0:No */
#define CY_U3P_UIB_EPO_TYPE_POS                             (10)


/*
   Number of packets to be sent per microframe (aka high-bandwidth mode ISO).
    For this implementation only EP3 and EP7 support values other than 1.
    EP3 and EP7 support values 1..3.  This field must be 0 for non-ISO endpoints.
 */
#define CY_U3P_UIB_EPO_ISOINPKS_MASK                        (0x00003000) /* <12:13> R:RW:0:No */
#define CY_U3P_UIB_EPO_ISOINPKS_POS                         (12)


/*
   Set VALID=1 to activate an endpoint, and VALID=0 to de-activate it. All
   USB endpoints default to valid. An endpoint whose VALID bit is 0 does
   not respond to any USB traffic.
 */
#define CY_U3P_UIB_EPO_VALID                                (1u << 14) /* <14:14> R:RW:1:No */


/*
   Setting this bit causes NAK on OUT and PING transactions.
 */
#define CY_U3P_UIB_EPO_NAK                                  (1u << 15) /* <15:15> R:RW:0:No */


/*
   Set this bit to “1” to stall an endpoint, and to “0” to clear a stall.
 */
#define CY_U3P_UIB_EPO_STALL                                (1u << 16) /* <16:16> R:RW:0:No */


/*
   Indicates a packet was received in an OUT token with more bytes than PAYLOAD.
 */
#define CY_U3P_UIB_EPO_OVF                                  (1u << 17) /* <17:17> RW1S:RW1C:0:No */


/*
   Set whenever device controller ACKs an OUT token.
 */
#define CY_U3P_UIB_EPO_COMMIT                               (1u << 18) /* <18:18> RW1S:RW1C:0:No */


/*
   When the host sends a PING/OUT token to any Bulk OUT endpoint, which does
   not have an empty buffer, the FX3 automatically NAKs the token and asserts
   this interrupt.
   Note that this bit will be set if there is no empty buffer at the receipt
   of the OUT Packet and if neither the Endpoint NAK or global NAK_ALL bits
   are set when the NAK is transmitted.
 */
#define CY_U3P_UIB_EPO_BNAK                                 (1u << 19) /* <19:19> RW1S:RW1C:0:No */


/*
   Indicates transfer is done (UIB_EPI_XFER_CNT=0).
   This bit must be cleared by s/w.
 */
#define CY_U3P_UIB_EPO_DONE                                 (1u << 20) /* <20:20> RW1S:RW1C:0:No */


/*
   Indicates a zero length packet was returned to the host in an IN transaction.
    Must be cleared by s/w.
 */
#define CY_U3P_UIB_EPO_ZERO                                 (1u << 21) /* <21:21> RW1S:RW1C:0:No */


/*
   Indicates a shorter-than-maxsize packet was received, but UIB_EPI_XFER_CNT
   did not reach 0).
 */
#define CY_U3P_UIB_EPO_SHORT                                (1u << 22) /* <22:22> RW1S:RW1C:0:No */


/*
   The ISO_ERR is set when ISO data PIDs arrive out of sequence (applies
   to high speed only), or when an an ISO packet was dropped because no buffer
   space was available (FS or HS)
 */
#define CY_U3P_UIB_EPO_ISOERR                               (1u << 23) /* <23:23> RW1S:RW1C:0:No */


/*
   Interrupt mask for OVUF bit
 */
#define CY_U3P_UIB_EPO_OVF_MASK                             (1u << 25) /* <25:25> R:RW:0:N/A */


/*
   Intterupt mask for COMMIT bit
 */
#define CY_U3P_UIB_EPO_COMMIT_MASK                          (1u << 26) /* <26:26> R:RW:0:N/A */


/*
   Interrupt mask for BNAK bit
 */
#define CY_U3P_UIB_EPO_BNAK_MASK                            (1u << 27) /* <27:27> R:RW:0:N/A */


/*
   Interrupt mask for DONE bit
 */
#define CY_U3P_UIB_EPO_DONE_MASK                            (1u << 28) /* <28:28> R:RW:0:N/A */


/*
   Interrupt mask for ZERO bit
 */
#define CY_U3P_UIB_EPO_ZERO_MASK                            (1u << 29) /* <29:29> R:RW:0:N/A */


/*
   Interrupt mask for SHORT bit
 */
#define CY_U3P_UIB_EPO_SHORT_MASK                           (1u << 30) /* <30:30> R:RW:0:N/A */


/*
   Interrupt mask for ISOERR bit
 */
#define CY_U3P_UIB_EPO_ISOERR_MASK                          (1u << 31) /* <31:31> R:RW:0:N/A */



/*
   OUT Endpoint remaining transfer length register
 */
#define CY_U3P_UIB_DEV_EPO_XFER_CNT_ADDRESS(n)              (0xe00314d8 + ((n) * (0x0004)))
#define CY_U3P_UIB_DEV_EPO_XFER_CNT(n)                      (*(uvint32_t *)(0xe00314d8 + ((n) * 0x0004)))
#define CY_U3P_UIB_DEV_EPO_XFER_CNT_DEFAULT                 (0x00000000)


/*
   CONTROL interrupt mask register
 */
#define CY_U3P_UIB_DEV_CTL_INTR_MASK_ADDRESS                (0xe0031518)
#define CY_U3P_UIB_DEV_CTL_INTR_MASK                        (*(uvint32_t *)(0xe0031518))
#define CY_U3P_UIB_DEV_CTL_INTR_MASK_DEFAULT                (0x00000000)


/*
   CONTROL interrupt request register
 */
#define CY_U3P_UIB_DEV_CTL_INTR_ADDRESS                     (0xe003151c)
#define CY_U3P_UIB_DEV_CTL_INTR                             (*(uvint32_t *)(0xe003151c))
#define CY_U3P_UIB_DEV_CTL_INTR_DEFAULT                     (0x00000000)

/*
   Set whenever a SOF occurrs
 */
#define CY_U3P_UIB_SOF                                      (1u << 1) /* <1:1> RW1S:RW1C:0:No */


/*
   Set when the host suspends the USB bus (USB SUSPEND)
 */
#define CY_U3P_UIB_SUSP                                     (1u << 2) /* <2:2> RW1S:RW1C:0:No */


/*
   Set when the host has initiated USB RESET (2.5us single ended 0 on bus)
 */
#define CY_U3P_UIB_URESET                                   (1u << 3) /* <3:3> RW1S:RW1C:0:No */


/*
   Set when the host grants high speed communications.
 */
#define CY_U3P_UIB_HSGRANT                                  (1u << 4) /* <4:4> RW1S:RW1C:0:No */


/*
   Set whenever a (valid of invalid) SETUP token is received
 */
#define CY_U3P_UIB_SUTOK                                    (1u << 5) /* <5:5> RW1S:RW1C:0:No */


/*
   Set when a valid SETUP token and data is received.  Data from this token
   can be read from UIB_DEV_SETUPDAT.
 */
#define CY_U3P_UIB_SUDAV                                    (1u << 6) /* <6:6> RW1S:RW1C:0:No */


/*
   USB Error limit detect from UIB_DEV_CS (COUNT>=LIMIT)
 */
#define CY_U3P_UIB_ERRLIMIT                                 (1u << 7) /* <7:7> RW1S:RW1C:0:No */


/*
   Set when the host has initiated USB RESUME (>2.5us K state on bus)
 */
#define CY_U3P_UIB_URESUME                                  (1u << 8) /* <8:8> RW1S:RW1C:0:No */


/*
   Set when host completes Status Stage of a Control Transfer
 */
#define CY_U3P_UIB_STATUS_STAGE                             (1u << 11) /* <11:11> RW1S:RW1C:0:No */



/*
   USB EP interrupt mask register
 */
#define CY_U3P_UIB_DEV_EP_INTR_MASK_ADDRESS                 (0xe0031520)
#define CY_U3P_UIB_DEV_EP_INTR_MASK                         (*(uvint32_t *)(0xe0031520))
#define CY_U3P_UIB_DEV_EP_INTR_MASK_DEFAULT                 (0x00000000)

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
   USB EP interrupt request register
 */
#define CY_U3P_UIB_DEV_EP_INTR_ADDRESS                      (0xe0031524)
#define CY_U3P_UIB_DEV_EP_INTR                              (*(uvint32_t *)(0xe0031524))
#define CY_U3P_UIB_DEV_EP_INTR_DEFAULT                      (0x00000000)

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
   Charger Detect Control and Configuration Register
 */
#define CY_U3P_UIB_CHGDET_CTRL_ADDRESS                      (0xe0031800)
#define CY_U3P_UIB_CHGDET_CTRL                              (*(uvint32_t *)(0xe0031800))
#define CY_U3P_UIB_CHGDET_CTRL_DEFAULT                      (0x00000000)

/*
   PHY USB Charger Present
 */
#define CY_U3P_UIB_PHY_CHG_DETECTED                         (1u << 0) /* <0:0> RW:R:0:Yes */


/*
   ACA OTG ID Polling Interval in 16ms increments, 0000 = 16ms
 */
#define CY_U3P_UIB_ACA_POLL_INTERVAL_MASK                   (0x0000001e) /* <1:4> R:RW:0:Yes */
#define CY_U3P_UIB_ACA_POLL_INTERVAL_POS                    (1)


/*
   ACA Comparison Resistor Trim Override
 */
#define CY_U3P_UIB_ACA_RTRIM_OVERRIDE                       (1u << 5) /* <5:5> R:RW:0:No */


/*
   Decoded OTG ID Value
        If ACA_CONN_MODE == 0 --> Standard ACA/Charger Mode
           000 = OTG 1.3 B-Device (RID_GND:  0...1kO)
           001 = OTG 1.3 A-Device (RID_FLOAT_CHG:  >220kO)
           010 = ACA A-Device  (RID_A_CHG:  119...132kO)
           011 = ACA B-Device  (RID_B_CHG:  65...72kO)
           100 = ACA C-Device  (RID_C_CHG:  35...39kO)
        If ACA_CONN_MODE == 1 --> Motorola EMU Mode
           000 = OTG 1.3 B-Device (RID_GND:  0...1kO)
           001 = OTG 1.3 A-Device (RID_FLOAT_CHG:  >1000kO)
           010 = MPX.200 VPA (RPROP_ID1:  <10.1kO)
           011 = Non-Intelligent Charging Device (RPROP_ID2:  101...103kO)
           100 = Mid-Rate Charger (RPROP_ID3:  198...202kO)
           101 = Fast Charger (RPROP_ID4:  435.6...444.4kO)
   [Battery Charging Specification:  Table 5-3, p 29]
   [Motorola Enhanced Mini-USB (EMU) Requirements:  §4.1.2, Table 1, p 9;
   §4.2.3, Table 4, p 12;  §4.2.6, p 15...16;  Appendix A, p 24...25]
   [ll65aca25 BROS 001-47035*D: Tables 5 & 6, p 32]]
 */
#define CY_U3P_UIB_ACA_OTG_ID_VALUE_MASK                    (0x00000e00) /* <9:11> RW:R:N/A:Yes */
#define CY_U3P_UIB_ACA_OTG_ID_VALUE_POS                     (9)


/*
   Enable ACA OTG ID Detection
       0 = Disabled
       1 = Enabled
 */
#define CY_U3P_UIB_ACA_ENABLE                               (1u << 12) /* <12:12> R:RW:0:Yes */


/*
   Charger Connection Mode
       0 = Standard ACA/Charger
       1 = Motorola Enhanced Mini-USB (EMU) Requirements
 */
#define CY_U3P_UIB_ACA_CONN_MODE                            (1u << 13) /* <13:13> R:RW:0:Yes */


/*
   Carkit Adaptor Enable
   0: USB Mode (normal USB operation)
   1: USB PHY UART mode
   In UART mode D+/D- are routed (digitally) to carkit UART signals (P-Port
   pads, S-Port pads or LPP UART pads as selected by GCTL_IOMATRIX).  This
   register is relevant only if ANALOG_SWITCH=0
 */
#define CY_U3P_UIB_CARKIT                                   (1u << 14) /* <14:14> R:RW:0:Yes */


/*
   ACA ADC Output
 */
#define CY_U3P_UIB_ACA_ADC_OUT_MASK                         (0x00ff0000) /* <16:23> RW:R:N/A:Yes */
#define CY_U3P_UIB_ACA_ADC_OUT_POS                          (16)


/*
   ACA Comparison Resistor Trim
 */
#define CY_U3P_UIB_ACA_RTRIM_MASK                           (0x07000000) /* <24:26> R:RW:0:Yes */
#define CY_U3P_UIB_ACA_RTRIM_POS                            (24)


/*
   This is a set-only bit and cannot be cleared once set.  It overrides ACA_ENABLE
   and makes charger detection functionality unavailable until next power
   cycle.  This bit is used for wounding.
 */
#define CY_U3P_UIB_ACA_DISABLE                              (1u << 30) /* <30:30> R:RW1S:0:No */


/*
   PHY Charger Detection Enable
 */
#define CY_U3P_UIB_PHY_CHARGER_DETECT_EN                    (1u << 31) /* <31:31> R:RW:0:Yes */



/*
   Charger Detect Interrupt Register
 */
#define CY_U3P_UIB_CHGDET_INTR_ADDRESS                      (0xe0031804)
#define CY_U3P_UIB_CHGDET_INTR                              (*(uvint32_t *)(0xe0031804))
#define CY_U3P_UIB_CHGDET_INTR_DEFAULT                      (0x00000000)

/*
   OTG ID Change Interrupt
   Indicates that the decoded value of the USB OTG ID signal has changed.
 */
#define CY_U3P_UIB_OTG_ID_CHANGE                            (1u << 0) /* <0:0> W1S:RW1C:0:Yes */


/*
   USB Charger Detect Change Interrupt
 */
#define CY_U3P_UIB_CHG_DET_CHANGE                           (1u << 1) /* <1:1> W1S:RW1C:0:Yes */



/*
   Charger Detect Interrupt Mask Register
 */
#define CY_U3P_UIB_CHGDET_INTR_MASK_ADDRESS                 (0xe0031808)
#define CY_U3P_UIB_CHGDET_INTR_MASK                         (*(uvint32_t *)(0xe0031808))
#define CY_U3P_UIB_CHGDET_INTR_MASK_DEFAULT                 (0x00000000)

/*
   0: Mask interrupt, 1: Report interrupt to higher level
 */
#define CY_U3P_UIB_OTG_ID_CHANGE                            (1u << 0) /* <0:0> R:RW:0:N/A */


/*
   0: Mask interrupt, 1: Report interrupt to higher level
 */
#define CY_U3P_UIB_CHG_DET_CHANGE                           (1u << 1) /* <1:1> R:RW:0:N/A */



/*
   OTG Control Register
 */
#define CY_U3P_UIB_OTG_CTRL_ADDRESS                         (0xe003180c)
#define CY_U3P_UIB_OTG_CTRL                                 (*(uvint32_t *)(0xe003180c))
#define CY_U3P_UIB_OTG_CTRL_DEFAULT                         (0x00000000)

/*
   OTG Enable
 */
#define CY_U3P_UIB_OTG_ENABLE                               (1u << 0) /* <0:0> R:RW:0:No */


/*
   D+ Pullup Enable
 */
#define CY_U3P_UIB_DP_PU_EN                                 (1u << 1) /* <1:1> R:RW:0:No */


/*
   D? Pulldown Enable
 */
#define CY_U3P_UIB_DM_PD_EN                                 (1u << 2) /* <2:2> R:RW:0:No */


/*
   D+ Pulldown Enable
 */
#define CY_U3P_UIB_DP_PD_EN                                 (1u << 3) /* <3:3> R:RW:0:No */


/*
   D+ line state
 */
#define CY_U3P_UIB_DP                                       (1u << 4) /* <4:4> RW:R:0:No */


/*
   D? line state
 */
#define CY_U3P_UIB_DM                                       (1u << 5) /* <5:5> RW:R:0:No */


/*
   Vbus Valid
       0  Vbus < 4.4V
       1  Vbus > 4.7V
 */
#define CY_U3P_UIB_VBUS_VALID                               (1u << 6) /* <6:6> RW:R:0:No */


/*
   Charge Vbus
 */
#define CY_U3P_UIB_CHG_VBUS                                 (1u << 7) /* <7:7> R:RW:0:No */


/*
   Discharge Vbus
 */
#define CY_U3P_UIB_DSCHG_VBUS                               (1u << 8) /* <8:8> R:RW:0:No */


/*
   Vbus Valid for A Session
       0  Vbus < 0.8V
       1  Vbus > 2.0V
 */
#define CY_U3P_UIB_A_SESS_VALID                             (1u << 9) /* <9:9> RW:R:0:No */


/*
   Vbus Valid for B Session
       0  Vbus < 0.8V
       1  Vbus > 4.0V
 */
#define CY_U3P_UIB_B_SESS_VALID                             (1u << 10) /* <10:10> RW:R:0:No */


/*
   Vbus B Session End
       0  Vbus > 0.8V
       1  Vbus < 0.2V
 */
#define CY_U3P_UIB_B_END_SESS                               (1u << 11) /* <11:11> RW:R:0:No */


/*
   Host/Peripheral Role Select, Enables the host function.
   Complete description depends on fields: DEV_ENABLE, and SSDEV_ENABLE.
   Combined Behavior is as follows for the following combination of fields:
   DEV_ENABLE:HOST_ENABLE: SSDEV_ENABLE
   0:0:0:  No USB Functionality Enabled
   0:0:1: USB 3.0 SS Device only is enabled.
   0:1:0: OTG Host function only is enabled. Both SS and USB 1.1/2.0 device
   functions are disabled
   0:1:1: ****ILLEGAL******
   1:0:0: USB 1.1/2.0 Device function enabled. SS Device and OTG Host function
   disabled.
   1:0:1: USB 1.1/2.0 Device with USB 3.0 SS Device along with Rx Detect
   functions are allowed. OTG Host Function not allowed
   1:1:0: ***** ILLEGAL except as intermediate value during swithing between
   010 and 100 *****
   1:1:1: ****ILLEGAL******
 */
#define CY_U3P_UIB_HOST_ENABLE                              (1u << 12) /* <12:12> R:RW:0:No */


/*
   Enables the USB 1.1/2.0 device function.
   Behavior depends on settings of HOST_ENABLE, and SSDEV_ENABLE fields and
   is defined in the HOST_ENABLE field.
 */
#define CY_U3P_UIB_DEV_ENABLE                               (1u << 13) /* <13:13> R:RW:0:No */


/*
   Enables the Super Speed  device function.
   Behavior depends on settings of HOST_ENABLE, and DEV_ENABLE fields and
   is defined in the HOST_ENABLE field.
 */
#define CY_U3P_UIB_SSDEV_ENABLE                             (1u << 14) /* <14:14> R:RW:0:No */


/*
   EPM role select
   0: EPM connected to highspeed host/device
   1: EPM connected to superspeed device
   This bit is required because SSDEVE_ENABLE and DEV_ENABLE can be used
   concurrently (see USB2/USB3 interoperability in PAS).
 */
#define CY_U3P_UIB_SSEPM_ENABLE                             (1u << 15) /* <15:15> R:RW:0:No */


/*
   OTG Interrupt Register
 */
#define CY_U3P_UIB_OTG_INTR_ADDRESS                         (0xe0031810)
#define CY_U3P_UIB_OTG_INTR                                 (*(uvint32_t *)(0xe0031810))
#define CY_U3P_UIB_OTG_INTR_DEFAULT                         (0x00000000)

/*
   A_SESS_VALID Change Interrupt
   Set when A_SESS_VALID changes
 */
#define CY_U3P_UIB_A_SESS_VALID_INT                         (1u << 0) /* <0:0> RW1S:RW1C:0:Yes */


/*
   B_SESS_VALID Change Interrupt
   Set when B_SESS_VALID changes
 */
#define CY_U3P_UIB_B_SESS_VALID_INT                         (1u << 1) /* <1:1> RW1S:RW1C:0:Yes */


/*
   B_END_SESS Interrupt
   Set when B_END_SESS goes active
 */
#define CY_U3P_UIB_B_END_SESS_INT                           (1u << 2) /* <2:2> RW1S:RW1C:0:Yes */


/*
   D+ SRP Interrupt
   A B-Device initiated SRP by pulsing the D+ signal
 */
#define CY_U3P_UIB_SRP_DP_INT                               (1u << 3) /* <3:3> RW1S:RW1C:0:Yes */


/*
   VBUS SRP Interrupt
   A B-Device initiatied SRP by pulsing the VBUS line
 */
#define CY_U3P_UIB_SRP_VBUS_INT                             (1u << 4) /* <4:4> RW1S:RW1C:0:Yes */


/*
   OTG Timer Timeout Interrupt
   The OTG Timer has reached its terminal count of 0
 */
#define CY_U3P_UIB_OTG_TIMER_TIMEOUT                        (1u << 5) /* <5:5> RW1S:RW1C:0:Yes */



/*
   OTG Interrupt Mask Register
 */
#define CY_U3P_UIB_OTG_INTR_MASK_ADDRESS                    (0xe0031814)
#define CY_U3P_UIB_OTG_INTR_MASK                            (*(uvint32_t *)(0xe0031814))
#define CY_U3P_UIB_OTG_INTR_MASK_DEFAULT                    (0x00000000)

/*
   A_SESS_VALID Change Interrupt
   Set when A_SESS_VALID changes
 */
#define CY_U3P_UIB_A_SESS_VALID_INT                         (1u << 0) /* <0:0> R:RW:0:N/A */


/*
   B_SESS_VALID Change Interrupt
   Set when B_SESS_VALID changes
 */
#define CY_U3P_UIB_B_SESS_VALID_INT                         (1u << 1) /* <1:1> R:RW:0:N/A */


/*
   B_END_SESS Interrupt
   Set when B_END_SESS goes active
 */
#define CY_U3P_UIB_B_END_SESS_INT                           (1u << 2) /* <2:2> R:RW:0:N/A */


/*
   D+ SRP Interrupt
   A B-Device initiated SRP by pulsing the D+ signal
 */
#define CY_U3P_UIB_SRP_DP_INT                               (1u << 3) /* <3:3> R:RW:0:N/A */


/*
   VBUS SRP Interrupt
   A B-Device initiatied SRP by pulsing the VBUS line
 */
#define CY_U3P_UIB_SRP_VBUS_INT                             (1u << 4) /* <4:4> R:RW:0:N/A */


/*
   OTG Timer Timeout Interrupt
   The OTG Timer has reached its terminal count of 0
 */
#define CY_U3P_UIB_OTG_TIMER_TIMEOUT                        (1u << 5) /* <5:5> R:RW:0:N/A */



/*
   OTG Timer Register
 */
#define CY_U3P_UIB_OTG_TIMER_ADDRESS                        (0xe0031818)
#define CY_U3P_UIB_OTG_TIMER                                (*(uvint32_t *)(0xe0031818))
#define CY_U3P_UIB_OTG_TIMER_DEFAULT                        (0x00000000)

/*
   Initial counter value.  After OTG_TIMER_LOAD_VAL clocks, OTG_TIMER_TIMEOUT
   wil trigger.  Disable counter by writing 0 to this register.
 */
#define CY_U3P_UIB_OTG_TIMER_LOAD_VAL_MASK                  (0xffffffff) /* <0:31> R:RW:0:Yes */
#define CY_U3P_UIB_OTG_TIMER_LOAD_VAL_POS                   (0)



/*
   Egress EPM Retry Buffer Status
 */
#define CY_U3P_UIB_EEPM_CS_ADDRESS                          (0xe0031c00)
#define CY_U3P_UIB_EEPM_CS                                  (*(uvint32_t *)(0xe0031c00))
#define CY_U3P_UIB_EEPM_CS_DEFAULT                          (0x00030000)

/*
   Bit vector indicating which of the 16 retry buffer contain a valid packet.
   In SuperSpeed mode, this buffer functions as a circular buffer trailing
   packets that can be retried behind the WRITE_PTR.  In HighSpeed mode,
   each End Point has 1 retry buffer that may independently valid or invalid.
   These bits are cleared when the Protocol Layer 'activates' an End Point
   (as opposed to 'reactivating' it).  In SuperSpeed mode all bits are cleared
   at once, in HighSpeed mode only the bit for the Endpoint being activated
   is cleared.
 */
#define CY_U3P_UIB_VALID_PACKETS_MASK                       (0x0000ffff) /* <0:15> RW:R:0:No */
#define CY_U3P_UIB_VALID_PACKETS_POS                        (0)


/*
   This bit will repair the EPM whenever there is under run due to SYSTEM
   EPM will keep on reading the packet from SYSMEM whenever data is ready
   from SYSMEM. If an under-run occurs, EPM will raise the UIB_INTR.EPM_URUN
   interrupt.
 */
#define CY_U3P_UIB_URUN_REPAIR_EN                           (1u << 16) /* <16:16> R:RW:1:No */


/*
   If an under run occurs and the URUN_REPAIR_EN is set and this register
   bit is set, then EPM will start a timmer(16-bit counter). If the repair
   is not complete after 65535*epm clock, EPM will raise the UIB_INTR.EPM_URUN_TIMEOUT
   interrupt.
 */
#define CY_U3P_UIB_URUN_REPAIR_TIMEOUT_EN                   (1u << 17) /* <17:17> R:RW:1:No */


/*
   The End Point that the under-run occurred.
 */
#define CY_U3P_UIB_URUN_EP_NUM_MASK                         (0x003c0000) /* <18:21> RW:R:0:No */
#define CY_U3P_UIB_URUN_EP_NUM_POS                          (18)


/*
   Active Endpoint Number
 */
#define CY_U3P_UIB_EG_EPNUM_MASK                            (0xf0000000) /* <28:31> RW:R:0:No */
#define CY_U3P_UIB_EG_EPNUM_POS                             (28)



/*
   Ingress EPM Control and Status
 */
#define CY_U3P_UIB_IEPM_CS_ADDRESS                          (0xe0031c04)
#define CY_U3P_UIB_IEPM_CS                                  (*(uvint32_t *)(0xe0031c04))
#define CY_U3P_UIB_IEPM_CS_DEFAULT                          (0x00000000)

/*
   Number of bytes in packet.
 */
#define CY_U3P_UIB_READ_PTR_MASK                            (0x00000fff) /* <0:11> RW:R:0:No */
#define CY_U3P_UIB_READ_PTR_POS                             (0)


/*
   End of Transfer. Set for short packets.
 */
#define CY_U3P_UIB_WRITE_PTR_MASK                           (0x0fff0000) /* <16:27> RW:R:0:No */
#define CY_U3P_UIB_WRITE_PTR_POS                            (16)


/*
   This will flush both the Egress and Ingress EPM.
 */
#define CY_U3P_UIB_EPM_FLUSH                                (1u << 28) /* <28:28> R:RW:0:No */


/*
   This will reset the EPM Mux.
 */
#define CY_U3P_UIB_EPM_MUX_RESET                            (1u << 29) /* <29:29> R:RW:0:No */



/*
   Ingress EPM MULT function Control
 */
#define CY_U3P_UIB_IEPM_MULT_ADDRESS                        (0xe0031c08)
#define CY_U3P_UIB_IEPM_MULT                                (*(uvint32_t *)(0xe0031c08))
#define CY_U3P_UIB_IEPM_MULT_DEFAULT                        (0x00008000)

/*
   Mult Enable for EP1-15.
 */
#define CY_U3P_UIB_MULT_EN_MASK                             (0x00007fff) /* <0:14> R:RW:0:No */
#define CY_U3P_UIB_MULT_EN_POS                              (0)


/*
   This field is used to when evaluate the mult signal from ingress adapter.
   If number of packet space available in the buffer goes down by this field,
   then
   mult signal from adapter will be evaluated and if it is set the original
   buffer space(number of packets) is added to NUM_PACKETS in the IEPM_ENDPOINT
   register.
 */
#define CY_U3P_UIB_MULT_THRSHOLD_MASK                       (0x03ff8000) /* <15:25> R:RW:1:No */
#define CY_U3P_UIB_MULT_THRSHOLD_POS                        (15)



/*
   Egress EPM per Endpoint Control and Status
 */
#define CY_U3P_UIB_EEPM_ENDPOINT_ADDRESS(n)                 (0xe0031c40 + ((n) * (0x0004)))
#define CY_U3P_UIB_EEPM_ENDPOINT(n)                         (*(uvint32_t *)(0xe0031c40 + ((n) * 0x0004)))
#define CY_U3P_UIB_EEPM_ENDPOINT_DEFAULT                    (0x00000400)

/*
   Maximum packet size for this end-point.  Typically this value is 1024,
   512, 64, 1023 (last 2 for USB2 only).
 */
#define CY_U3P_UIB_PACKET_SIZE_MASK                         (0x000007ff) /* <0:10> R:RW:1024:No */
#define CY_U3P_UIB_PACKET_SIZE_POS                          (0)


/*
   Number of bytes in the current buffer.
 */
#define CY_U3P_UIB_EEPM_BYTE_COUNT_MASK                     (0x07fff800) /* <11:26> RW:R:0:No */
#define CY_U3P_UIB_EEPM_BYTE_COUNT_POS                      (11)


/*
   ZLP present in the current buffer
 */
#define CY_U3P_UIB_ZLP                                      (1u << 27) /* <27:27> RW:R:0:No */


/*
   The EPM condition used by USB block.
 */
#define CY_U3P_UIB_EEPM_EP_READY                            (1u << 30) /* <30:30> RW:R:0:No */


/*
   This bit will flush the corresponding Socket.
 */
#define CY_U3P_UIB_SOCKET_FLUSH                             (1u << 31) /* <31:31> R:RW:0:No */



/*
   Ingress EPM Per Endpoint Control and Status
 */
#define CY_U3P_UIB_IEPM_ENDPOINT_ADDRESS(n)                 (0xe0031c80 + ((n) * (0x0004)))
#define CY_U3P_UIB_IEPM_ENDPOINT(n)                         (*(uvint32_t *)(0xe0031c80 + ((n) * 0x0004)))
#define CY_U3P_UIB_IEPM_ENDPOINT_DEFAULT                    (0x00000400)

/*
   Maximum packet size for this end-point.  Typically this value is 1024,
   512, 64, 1023 (last 2 for USB2 only).
 */
#define CY_U3P_UIB_PACKET_SIZE_MASK                         (0x000007ff) /* <0:10> R:RW:1024:No */
#define CY_U3P_UIB_PACKET_SIZE_POS                          (0)


/*
   Number of packets that are guaranteed to fit in the remaining buffer space
   of the current and next buffers.  If the computed number of packets available
   is larger than 16, this number will be assumed to be 16 in the protocol
   block.
 */
#define CY_U3P_UIB_NUM_IN_PACKETS_MASK                      (0x003ff800) /* <11:21> RW:R:0:No */
#define CY_U3P_UIB_NUM_IN_PACKETS_POS                       (11)


/*
   The EPM condition used by USB block.
 */
#define CY_U3P_UIB_EP_READY                                 (1u << 22) /* <22:22> RW:R:0:No */


/*
   Number of odd byte packets that can fit in the DMA buffer. Only valid
   if OddMaxPktSizeEn is set.
 */
#define CY_U3P_UIB_ODD_MAX_NUM_PKTS_MASK                    (0x1f000000) /* <24:28> R:RW:0:No */
#define CY_U3P_UIB_ODD_MAX_NUM_PKTS_POS                     (24)


/*
   If this bit is enabled, then at the time of calculation of number of packets
   space in current DMA buffer, OddMaxNumPkts will over-ride the hardware
   calculation.
 */
#define CY_U3P_UIB_ODD_MAX_PKT_SIZE_EN                      (1u << 29) /* <29:29> R:RW:0:No */


/*
   Configure end-of-packet signalling to DMA adapter.
   0: Send EOP at the end of a full packet and EOT for short/zlp packets
   1: Send EOT at the end of every packet
   Setting this bit to 1 is useful for variable size packet endpoints only.
 */
#define CY_U3P_UIB_EOT_EOP                                  (1u << 30) /* <30:30> R:RW:0:No */


/*
   This bit will flush the corresponding Socket.
 */
#define CY_U3P_UIB_SOCKET_FLUSH                             (1u << 31) /* <31:31> R:RW:0:No */



/*
   Ingress EPM FIFO Entry
 */
#define CY_U3P_UIB_IEPM_FIFO_ADDRESS                        (0xe0031cc0)
#define CY_U3P_UIB_IEPM_FIFO                                (*(uvint32_t *)(0xe0031cc0))
#define CY_U3P_UIB_IEPM_FIFO_DEFAULT                        (0x00000000)

/*
   Number of bytes in the packet.
 */
#define CY_U3P_UIB_BYTES_MASK                               (0x000007ff) /* <0:10> RW:R:0:No */
#define CY_U3P_UIB_BYTES_POS                                (0)


/*
   End of Transfer. Set for by the protocol layer short and zero length packets;
   forwarded to DMA Adapter.
 */
#define CY_U3P_UIB_EOT                                      (1u << 11) /* <11:11> RW:R:0:No */


/*
   Endpoint number for this packet
 */
#define CY_U3P_UIB_IN_EPNUM_MASK                            (0x0000f000) /* <12:15> RW:R:0:No */
#define CY_U3P_UIB_IN_EPNUM_POS                             (12)


/*
   Entry is valid
 */
#define CY_U3P_UIB_EP_VALID                                 (1u << 16) /* <16:16> RW:R:0:No */



/*
   Host controller command and status bits
 */
#define CY_U3P_UIB_HOST_CS_ADDRESS                          (0xe0032000)
#define CY_U3P_UIB_HOST_CS                                  (*(uvint32_t *)(0xe0032000))
#define CY_U3P_UIB_HOST_CS_DEFAULT                          (0x01000000)

/*
   This register contains device address to which host wishes to communicate.
   Host sends this address to device using set_address command. This address
   also used by SIE to append this address with different tokens.
 */
#define CY_U3P_UIB_DEV_ADDR_MASK                            (0x0000007f) /* <0:6> R:RW:0:No */
#define CY_U3P_UIB_DEV_ADDR_POS                             (0)


/*
   This register is used for frame fit calculation in the host contoller.
   The recommended values are listed in below:
   EHCI Mode:             212
   OHCI(FULL Speed): 190
   OHCI(Low Speed):     20
 */
#define CY_U3P_UIB_FRAME_FIT_OFFSET_MASK                    (0x007fff00) /* <8:22> R:RW:0:No */
#define CY_U3P_UIB_FRAME_FIT_OFFSET_POS                     (8)


/*
   0: EHCI_INTR.HOST_SYS_ERR_IE works as documented (normal)
   1: HOST_SYS_ERR functionality is disabled
 */
#define CY_U3P_UIB_DISABLE_EHCI_HOSTERR                     (1u << 23) /* <23:23> R:RW:0:No */


/*
   0: In a case where Device is sending a short packet and the total byte
   count does NOT reach zero, host does not deactivate that EP.
   1: In a case where Device is sending a short packet and the total byte
   count does NOT reach zero, host deactivates that EP.
 */
#define CY_U3P_UIB_DEACTIVATE_ON_IN_EP_SHORT_PKT            (1u << 24) /* <24:24> R:RW:1:No */



/*
   Host End Point Interrupt Register
 */
/*
   This register contains interrupt status bit for 32 EPs. CPU reads this
   register to determine, which EP has triggered interrupt.
 */
#define CY_U3P_UIB_HOST_EP_INTR_ADDRESS                     (0xe0032004)
#define CY_U3P_UIB_HOST_EP_INTR                             (*(uvint32_t *)(0xe0032004))
#define CY_U3P_UIB_HOST_EP_INTR_DEFAULT                     (0x00000000)

/*
   Interrupt Requests for OUT endpoints 0..15 when the EP is deactivated
   by HC.
 */
#define CY_U3P_UIB_EPO_IRQ_TOP_MASK                         (0x0000ffff) /* <0:15> RW1S:RW1C:0:No */
#define CY_U3P_UIB_EPO_IRQ_TOP_POS                          (0)


/*
   Interrupt Requests for IN endpoints 0..15 when the EP is deactivated by
   HC.
 */
#define CY_U3P_UIB_EPI_IRQ_TOP_MASK                         (0xffff0000) /* <16:31> RW1S:RW1C:0:No */
#define CY_U3P_UIB_EPI_IRQ_TOP_POS                          (16)



/*
   Host End Point Interrupt Mask register
 */
/*
   Controls whether host interrupts are reported to the CPU.
 */
#define CY_U3P_UIB_HOST_EP_INTR_MASK_ADDRESS                (0xe0032008)
#define CY_U3P_UIB_HOST_EP_INTR_MASK                        (*(uvint32_t *)(0xe0032008))
#define CY_U3P_UIB_HOST_EP_INTR_MASK_DEFAULT                (0x00000000)

/*
   1= Report interrupt to CPU
 */
#define CY_U3P_UIB_EPO_IRQ_MASK_MASK                        (0x0000ffff) /* <0:15> R:RW:0:N/A */
#define CY_U3P_UIB_EPO_IRQ_MASK_POS                         (0)


/*
   1= Report interrupt to CPU
 */
#define CY_U3P_UIB_EPI_IRQ_MASK_MASK                        (0xffff0000) /* <16:31> R:RW:0:N/A */
#define CY_U3P_UIB_EPI_IRQ_MASK_POS                         (16)



/*
   Data toggle for endpoints
 */
#define CY_U3P_UIB_HOST_TOGGLE_ADDRESS                      (0xe003200c)
#define CY_U3P_UIB_HOST_TOGGLE                              (*(uvint32_t *)(0xe003200c))
#define CY_U3P_UIB_HOST_TOGGLE_DEFAULT                      (0x00000000)


/*
   Scheduler memory pointer register
 */
#define CY_U3P_UIB_HOST_SHDL_CS_ADDRESS                     (0xe0032010)
#define CY_U3P_UIB_HOST_SHDL_CS                             (*(uvint32_t *)(0xe0032010))
#define CY_U3P_UIB_HOST_SHDL_CS_DEFAULT                     (0x00000000)

/*
   Asynchronous list pointer 0.  Indicates the first Async schedule entry
   number in schedule 0. This pointer is used for the lower portion of the
   scheduler memory (location 0-95).
 */
#define CY_U3P_UIB_BULK_CNTRL_PTR0_MASK                     (0x000000ff) /* <0:7> R:RW:0:No */
#define CY_U3P_UIB_BULK_CNTRL_PTR0_POS                      (0)


/*
   Asynchronous list pointer 1.  Indicates the first Async schedule entry
   number in schedule 1. This pointer is used for the upper portion of the
   scheduler memory (location 96-191).
 */
#define CY_U3P_UIB_BULK_CNTRL_PTR1_MASK                     (0x0000ff00) /* <8:15> R:RW:0:No */
#define CY_U3P_UIB_BULK_CNTRL_PTR1_POS                      (8)


/*
   This bit is set by software to indiacte that only the Async schedule has
   been changed, and the scheduler may flip to the alternate schedule at
   the next microframe boundary. This bit is cleared by hardware upon swithcing
   to new schedule.
 */
#define CY_U3P_UIB_ASYNC_SHDL_CHNG                          (1u << 16) /* <16:16> RW0C:RW1S:0:No */


/*
   This bit is set by software to indicate that periodic schedule has been
   changed, and the scheduler may flip to the alternate schedule at the next
   frame boundary. This bit is cleared by hardware upon switching to new
   schedule.
 */
#define CY_U3P_UIB_PERI_SHDL_CHNG                           (1u << 17) /* <17:17> RW0C:RW1S:0:No */


/*
   This bit will inform the software about with portion of the memory should
   be used for periodic list update both for EHCI and OHCI.
   0: Indicates that the scheduler is currently using the lower portion of
   the
       scheduler memory for processign the periodic list with the starting
       location of 'd0.
   1: Indicates that the scheduler is currently using the upper portion of
   the
       scheduler memory for processign the periodic list with the starting
       location of  'd96.
 */
#define CY_U3P_UIB_PERI_SHDL_STATUS                         (1u << 18) /* <18:18> RW:R:0:No */


/*
   This bit will inform the software about with portion of the memory should
   be used for Async/non-periodic list update both for EHCI and OHCI.
   0: Indicates that the scheduler is currently using the lower portion of
   the scheduler memory for processign the Async/Non-periodic list with the
   starting location of ASYNC_PTR0 register.
   1: Indicates that the scheduler is currently using the upper portion of
   the scheduler memory for processign the Async/Non-periodic list with the
   starting location of  AYSNC_PTR1 register.
 */
#define CY_U3P_UIB_ASYNC_SHDL_STATUS                        (1u << 19) /* <19:19> RW:R:0:No */



/*
   Scheduler sleep register
 */
#define CY_U3P_UIB_HOST_SHDL_SLEEP_ADDRESS                  (0xe0032014)
#define CY_U3P_UIB_HOST_SHDL_SLEEP                          (*(uvint32_t *)(0xe0032014))
#define CY_U3P_UIB_HOST_SHDL_SLEEP_DEFAULT                  (0x00000258)

/*
   This bit will enable the sleep feature for the EHCI.
 */
#define CY_U3P_UIB_ASYNC_SLEEP_EN                           (1u << 0) /* <0:0> R:RW:0:No */


/*
   When the Async list is empty and the ASYNC_SLEEP_EN is set then the scheduler
   will stop traversing the async list for the amount of ASYNC_SLEEP_TIMMER*sie
   clock(30Mhz).
   Per EHCI spec the sleep time should be 10usec.
 */
#define CY_U3P_UIB_ASYNC_SLEEP_TIMMER_MASK                  (0x000003fe) /* <1:9> R:RW:0x12C:No */
#define CY_U3P_UIB_ASYNC_SLEEP_TIMMER_POS                   (1)



/*
   Response base address register
 */
#define CY_U3P_UIB_HOST_RESP_BASE_ADDRESS                   (0xe0032018)
#define CY_U3P_UIB_HOST_RESP_BASE                           (*(uvint32_t *)(0xe0032018))
#define CY_U3P_UIB_HOST_RESP_BASE_DEFAULT                   (0x00000000)

/*
   Base address where scheduler responses are written into memory.  Responses
   are written in order of completion, wrapping at end of buffer (see UIB_HOST_RESP_CS)
 */
#define CY_U3P_UIB_BASE_ADDRESS_MASK                        (0xffffffff) /* <0:31> R:RW:0:No */
#define CY_U3P_UIB_BASE_ADDRESS_POS                         (0)



/*
   Scheduler response command and control
 */
#define CY_U3P_UIB_HOST_RESP_CS_ADDRESS                     (0xe003201c)
#define CY_U3P_UIB_HOST_RESP_CS                             (*(uvint32_t *)(0xe003201c))
#define CY_U3P_UIB_HOST_RESP_CS_DEFAULT                     (0x00000000)

/*
   Base address where scheduler responses are written into memory.  Responses
   are written in order of completion, wrapping at end of buffer (see UIB_HOST_RESP_CS)
 */
#define CY_U3P_UIB_MAX_ENTRY_MASK                           (0x000000ff) /* <0:7> R:RW:0:No */
#define CY_U3P_UIB_MAX_ENTRY_POS                            (0)


/*
   Response entry which, when written, would constitute an overflow error.
   See ERROR bit.
 */
#define CY_U3P_UIB_LIMIT_MASK                               (0x0000ff00) /* <8:15> R:RW:0:No */
#define CY_U3P_UIB_LIMIT_POS                                (8)


/*
   Position at which next schedule response entry will be written (not itself
   a valid entry).
 */
#define CY_U3P_UIB_WR_PTR_MASK                              (0x00ff0000) /* <16:23> RW:R:0:No */
#define CY_U3P_UIB_WR_PTR_POS                               (16)


/*
   This is a condition used counting the packet on USB bus.  The packet counter
   is used for the RESP_RATE specified in the scheduler memory.
   0: The packet counter should increment when the device does not NAK.
        The response from device could be: ACK, STALL, NYET, PID_ERROR,
        Data toggle mismatch, CRC16_ERROR, Time-Out.
   1: The packet counter should increment when the transaction is successful.
        The successful transaction definition is:
        IN-Token: No CRC16/PID/PHY Error, No toggle mismatch, No Babble,
             No STALL, No NYET, No NAK, No Timeout
        OUT-TOKEN: No CRC16/PID/PHY Error, No STALL, No NAK,
             No NYET for PING token
 */
#define CY_U3P_UIB_WR_RESP_COND                             (1u << 24) /* <24:24> R:RW:0:No */


/*
   Hardware will set this bit as a scheduler response is written with WR_PTR=LIMIT
   (this indicates and overflow in response memory).  Software must clear
   this bit by writing 0 to it.
 */
#define CY_U3P_UIB_LIM_ERROR                                (1u << 31) /* <31:31> RW1S:RW0C:0:No */



/*
   Active Endpoint Register
 */
#define CY_U3P_UIB_HOST_ACTIVE_EP_ADDRESS                   (0xe0032020)
#define CY_U3P_UIB_HOST_ACTIVE_EP                           (*(uvint32_t *)(0xe0032020))
#define CY_U3P_UIB_HOST_ACTIVE_EP_DEFAULT                   (0x00000000)

/*
   This indicates if the OUT-EP is active or not. If there is a new schedule
   entry, this register needs to be Updated after the ASYNC_SHDL_CHNG or
   PERI_SHDL_CHNG is being set. Software should first clear the corresponding
   active bit upon HOST_EP_INTR interrupt and then read HOST_EP_DEACTIVATE
   to clear it.
 */
#define CY_U3P_UIB_OUT_EP_ACTIVE_MASK                       (0x0000ffff) /* <0:15> R:RW:0:No */
#define CY_U3P_UIB_OUT_EP_ACTIVE_POS                        (0)


/*
   This indicates if the IN-EP is active or not. If there is a new schedule
   entry, this register needs to be Updated after the ASYNC_SHDL_CHNG or
   PERI_SHDL_CHNG is being set. Software should first clear the corresponding
   active bit upon HOST_EP_INTR interrupt and then read HOST_EP_DEACTIVATE
   to clear it.
 */
#define CY_U3P_UIB_IN_EP_ACTIVE_MASK                        (0xffff0000) /* <16:31> R:RW:0:No */
#define CY_U3P_UIB_IN_EP_ACTIVE_POS                         (16)



/*
   OHCI Host controller Revision Number
 */
#define CY_U3P_UIB_OHCI_REVISION_ADDRESS                    (0xe0032024)
#define CY_U3P_UIB_OHCI_REVISION                            (*(uvint32_t *)(0xe0032024))
#define CY_U3P_UIB_OHCI_REVISION_DEFAULT                    (0x00000010)

/*
   Revision
 */
#define CY_U3P_UIB_REV_MASK                                 (0x000000ff) /* <0:7> R:R:0x10:No */
#define CY_U3P_UIB_REV_POS                                  (0)



/*
   Host controller operating mode control
 */
#define CY_U3P_UIB_OHCI_CONTROL_ADDRESS                     (0xe0032028)
#define CY_U3P_UIB_OHCI_CONTROL                             (*(uvint32_t *)(0xe0032028))
#define CY_U3P_UIB_OHCI_CONTROL_DEFAULT                     (0x00000000)

/*
   ControlBulkServiceRatio (Not directly supported in this IP The service
   allocation is not supported, however the Mult/MMult fields of the scheduler
   memory can be used to support the bandwidth allocation).
 */
#define CY_U3P_UIB_CBSR_MASK                                (0x00000003) /* <0:1> R:RW:0:No */
#define CY_U3P_UIB_CBSR_POS                                 (0)


/*
   PeriodicListEnable
 */
#define CY_U3P_UIB_PLE                                      (1u << 2) /* <2:2> R:RW:0:No */


/*
   IsochronousEnable
   Note: PLE and IE must both be set to 1 for the periodic list to be enabled.
    There is no difference in behavior between these two bits.
 */
#define CY_U3P_UIB_IE                                       (1u << 3) /* <3:3> R:RW:0:No */


/*
   ControlListEnable
 */
#define CY_U3P_UIB_CLE                                      (1u << 4) /* <4:4> R:RW:0:No */


/*
   BulkListEnable
 */
#define CY_U3P_UIB_BLE                                      (1u << 5) /* <5:5> R:RW:0:No */


/*
   HostControllerFunctionalState
 */
#define CY_U3P_UIB_HCFS_MASK                                (0x000000c0) /* <6:7> RW:RW:0:No */
#define CY_U3P_UIB_HCFS_POS                                 (6)


/*
   InterruptRouting
   Not implemented in this IP.
 */
#define CY_U3P_UIB_IR                                       (1u << 8) /* <8:8> R:R:0:No */


/*
   RemoteWakeupConnected
   Not implemented in this IP.
 */
#define CY_U3P_UIB_RWC                                      (1u << 9) /* <9:9> R:R:0:No */


/*
   RemoteWakeupEnable
   Not implemented in this IP.
 */
#define CY_U3P_UIB_RWE                                      (1u << 10) /* <10:10> R:R:0:No */



/*
   Command and Status Register
 */
#define CY_U3P_UIB_OHCI_COMMAND_STATUS_ADDRESS              (0xe003202c)
#define CY_U3P_UIB_OHCI_COMMAND_STATUS                      (*(uvint32_t *)(0xe003202c))
#define CY_U3P_UIB_OHCI_COMMAND_STATUS_DEFAULT              (0x00080000)

/*
   N/A Use the UIB_POWER.RESETN register
 */
#define CY_U3P_UIB_HCR                                      (1u << 0) /* <0:0> R:R:0:No */


/*
   ControlListFilled (Not implemented in this IP)
 */
#define CY_U3P_UIB_CLF                                      (1u << 1) /* <1:1> R:R:0:No */


/*
   BulkListFilled (Not implemented in this IP)
 */
#define CY_U3P_UIB_BLF                                      (1u << 2) /* <2:2> R:R:0:No */


/*
   OwnershipChangeRequest (Not implemented in this IP)
 */
#define CY_U3P_UIB_OCR                                      (1u << 3) /* <3:3> R:R:0:No */


/*
   SchedulingOverrunCount
 */
#define CY_U3P_UIB_SOC_MASK                                 (0x00030000) /* <16:17> RW:R:0:No */
#define CY_U3P_UIB_SOC_POS                                  (16)


/*
   Run/Stop (Replacing the OwnerShipChangeRequest)
   0: Stop
   1: Run
 */
#define CY_U3P_UIB_RS                                       (1u << 18) /* <18:18> R:RW:0:No */


/*
   This bit is a zero whenever the Run/Stop bit is a one.
   The HC sets this bit to one after it has stopped executing as a result
   of the Run/Stop bit being set to 0 by software.
 */
#define CY_U3P_UIB_HC_HALTED                                (1u << 19) /* <19:19> RW:R:1:No */



/*
   OHCI host controller interrupt status
 */
#define CY_U3P_UIB_OHCI_INTERRUPT_STATUS_ADDRESS            (0xe0032030)
#define CY_U3P_UIB_OHCI_INTERRUPT_STATUS                    (*(uvint32_t *)(0xe0032030))
#define CY_U3P_UIB_OHCI_INTERRUPT_STATUS_DEFAULT            (0x00000000)

/*
   SchedulingOverrun
 */
#define CY_U3P_UIB_SO                                       (1u << 0) /* <0:0> RW1S:RW1C:0:No */


/*
   WritebackDoneHead (Not implemented in this IP)
 */
#define CY_U3P_UIB_WDH                                      (1u << 1) /* <1:1> R:R:0:No */


/*
   StartofFrame
 */
#define CY_U3P_UIB_SF                                       (1u << 2) /* <2:2> RW1S:RW1C:0:No */


/*
   ResumeDetected
 */
#define CY_U3P_UIB_RD                                       (1u << 3) /* <3:3> RW1S:RW1C:0:No */


/*
   UnrecoverableError (Not implemented in this IP)
 */
#define CY_U3P_UIB_UE                                       (1u << 4) /* <4:4> R:R:0:No */


/*
   FrameNumberOverflow
 */
#define CY_U3P_UIB_FNO                                      (1u << 5) /* <5:5> RW1S:RW1C:0:No */


/*
   RootHubStatusChange
   (Note: We will need this bit , as we are implementing HcRhPortStatus register.)
 */
#define CY_U3P_UIB_RHSC                                     (1u << 6) /* <6:6> W:R:0:No */


/*
   OwnershipChange (Not implemented in this IP)
 */
#define CY_U3P_UIB_OC                                       (1u << 30) /* <30:30> R:R:0:No */



/*
   OHCI Interrupt Enable Register
 */
/*
   This register can be used to enable any of the OHCI interrupts. To clear
   an interrupt enable bit, write to UIB_OHCI_INTERRUPT_DISABLE.
 */
#define CY_U3P_UIB_OHCI_INTERRUPT_ENABLE_ADDRESS            (0xe0032034)
#define CY_U3P_UIB_OHCI_INTERRUPT_ENABLE                    (*(uvint32_t *)(0xe0032034))
#define CY_U3P_UIB_OHCI_INTERRUPT_ENABLE_DEFAULT            (0x00000000)

/*
   SchedulingOverrun
 */
#define CY_U3P_UIB_SO                                       (1u << 0) /* <0:0> R:RW1S:0:N/A */


/*
   WritebackDoneHead (not implemented in this IP)
 */
#define CY_U3P_UIB_WDH                                      (1u << 1) /* <1:1> R:R:0:N/A */


/*
   StartofFrame
 */
#define CY_U3P_UIB_SF                                       (1u << 2) /* <2:2> R:RW1S:0:N/A */


/*
   ResumeDetected
 */
#define CY_U3P_UIB_RD                                       (1u << 3) /* <3:3> R:RW1S:0:N/A */


/*
   UnrecoverableError (not implemented in this IP)
 */
#define CY_U3P_UIB_UE                                       (1u << 4) /* <4:4> R:R:0:N/A */


/*
   FrameNumberOverflow
 */
#define CY_U3P_UIB_FNO                                      (1u << 5) /* <5:5> R:RW1S:0:N/A */


/*
   RootHubStatusChange
 */
#define CY_U3P_UIB_RHSC                                     (1u << 6) /* <6:6> R:RW1S:0:N/A */


/*
   OwnershipChange (Not implemented in this IP)
 */
#define CY_U3P_UIB_OC                                       (1u << 30) /* <30:30> R:R:0:N/A */


/*
   Master Interrupt Enable
 */
#define CY_U3P_UIB_MIE                                      (1u << 31) /* <31:31> R:RW1S:0:N/A */



/*
   OHCI Interrupt Disable Register
 */
/*
   This register can be used to disable any of the OHCI interrupts. To clear
   an interrupt enable bit, write to UIB_OHCI_INTERRUPT_ENABLE.
 */
#define CY_U3P_UIB_OHCI_INTERRUPT_DISABLE_ADDRESS           (0xe0032038)
#define CY_U3P_UIB_OHCI_INTERRUPT_DISABLE                   (*(uvint32_t *)(0xe0032038))
#define CY_U3P_UIB_OHCI_INTERRUPT_DISABLE_DEFAULT           (0x00000000)

/*
   SchedulingOverrun
 */
#define CY_U3P_UIB_SO                                       (1u << 0) /* <0:0> R:RW1C:0:N/A */


/*
   WritebackDoneHead (not implemented in this IP)
 */
#define CY_U3P_UIB_WDH                                      (1u << 1) /* <1:1> R:R:0:N/A */


/*
   StartofFrame
 */
#define CY_U3P_UIB_SF                                       (1u << 2) /* <2:2> R:RW1C:0:N/A */


/*
   ResumeDetected
 */
#define CY_U3P_UIB_RD                                       (1u << 3) /* <3:3> R:RW1C:0:N/A */


/*
   UnrecoverableError (not implemented in this IP)
 */
#define CY_U3P_UIB_UE                                       (1u << 4) /* <4:4> R:R:0:N/A */


/*
   FrameNumberOverflow
 */
#define CY_U3P_UIB_FNO                                      (1u << 5) /* <5:5> R:RW1C:0:N/A */


/*
   RootHubStatusChange
 */
#define CY_U3P_UIB_RHSC                                     (1u << 6) /* <6:6> R:RW1C:0:N/A */


/*
   OwnershipChange (Not implemented in this IP)
 */
#define CY_U3P_UIB_OC                                       (1u << 30) /* <30:30> R:R:0:N/A */


/*
   Master Interrupt Enable
 */
#define CY_U3P_UIB_MIE                                      (1u << 31) /* <31:31> R:RW1C:0:N/A */



/*
   OHCI frame control information
 */
#define CY_U3P_UIB_OHCI_FM_INTERVAL_ADDRESS                 (0xe003203c)
#define CY_U3P_UIB_OHCI_FM_INTERVAL                         (*(uvint32_t *)(0xe003203c))
#define CY_U3P_UIB_OHCI_FM_INTERVAL_DEFAULT                 (0x27f0752f)

/*
   FrameInterval
 */
#define CY_U3P_UIB_FI_MASK                                  (0x00007fff) /* <0:14> R:RW:0x752F:No */
#define CY_U3P_UIB_FI_POS                                   (0)


/*
   FSLargestDataPacket
 */
#define CY_U3P_UIB_FSMPS_MASK                               (0x7fff0000) /* <16:30> R:RW:0x27F0:No */
#define CY_U3P_UIB_FSMPS_POS                                (16)


/*
   FrameIntervalToggle
 */
#define CY_U3P_UIB_FIT                                      (1u << 31) /* <31:31> R:RW:0:No */



/*
   Current value of remaining frame count.
 */
#define CY_U3P_UIB_OHCI_FM_REMAINING_ADDRESS                (0xe0032040)
#define CY_U3P_UIB_OHCI_FM_REMAINING                        (*(uvint32_t *)(0xe0032040))
#define CY_U3P_UIB_OHCI_FM_REMAINING_DEFAULT                (0x00000000)

/*
   FrameRemaining
 */
#define CY_U3P_UIB_FR_MASK                                  (0x00007fff) /* <0:14> RW:R:0:No */
#define CY_U3P_UIB_FR_POS                                   (0)


/*
   FrameRemainingToggle
 */
#define CY_U3P_UIB_FRT                                      (1u << 31) /* <31:31> RW:R:0:No */



/*
   Full speed frame number register
 */
#define CY_U3P_UIB_OHCI_FM_NUMBER_ADDRESS                   (0xe0032044)
#define CY_U3P_UIB_OHCI_FM_NUMBER                           (*(uvint32_t *)(0xe0032044))
#define CY_U3P_UIB_OHCI_FM_NUMBER_DEFAULT                   (0x00000000)

/*
   FrameNumber
 */
#define CY_U3P_UIB_FN_MASK                                  (0x0000ffff) /* <0:15> RW:R:0h:No */
#define CY_U3P_UIB_FN_POS                                   (0)



/*
   Value indicating time where HC should start executing periodic schedule.
 */
#define CY_U3P_UIB_OHCI_PERIODIC_START_ADDRESS              (0xe0032048)
#define CY_U3P_UIB_OHCI_PERIODIC_START                      (*(uvint32_t *)(0xe0032048))
#define CY_U3P_UIB_OHCI_PERIODIC_START_DEFAULT              (0x00006977)

/*
   PeriodicStart.
   The Default is: 90% * FI = 90% * h752F = h6977
 */
#define CY_U3P_UIB_PS_MASK                                  (0x0000ffff) /* <0:15> R:RW:0x6977:No */
#define CY_U3P_UIB_PS_POS                                   (0)



/*
   Value which is compared to the FrameRemaining field prior to initiating
   a Low Speed .
 */
#define CY_U3P_UIB_OHCI_LS_THRESHOLD_ADDRESS                (0xe003204c)
#define CY_U3P_UIB_OHCI_LS_THRESHOLD                        (*(uvint32_t *)(0xe003204c))
#define CY_U3P_UIB_OHCI_LS_THRESHOLD_DEFAULT                (0x00000628)

/*
   LSThreshold
 */
#define CY_U3P_UIB_LST_MASK                                 (0x00000fff) /* <0:11> R:RW:0x628:No */
#define CY_U3P_UIB_LST_POS                                  (0)



/*
   Root Hub Port Status Register
 */
#define CY_U3P_UIB_OHCI_RH_PORT_STATUS_ADDRESS              (0xe0032054)
#define CY_U3P_UIB_OHCI_RH_PORT_STATUS                      (*(uvint32_t *)(0xe0032054))
#define CY_U3P_UIB_OHCI_RH_PORT_STATUS_DEFAULT              (0x00000100)

/*
   (read) CurrentConnectStatus
   (write) ClearPortEnable
 */
#define CY_U3P_UIB_RHP_CCS                                  (1u << 0) /* <0:0> RW:R:0:No */


/*
   (read) PortEnableStatus.
   (write) SetPortEnable
 */
#define CY_U3P_UIB_RHP_PES                                  (1u << 1) /* <1:1> RW:RW1S:0:No */


/*
   (read) PortSuspendStatus
   (write) SetPortSuspend
 */
#define CY_U3P_UIB_RHP_PSS                                  (1u << 2) /* <2:2> RW0C:RW1S:0:No */


/*
   (read) PortOverCurrentIndicator
   (write) ClearSuspendStatus
   Not implemented in this IP
 */
#define CY_U3P_UIB_RHP_POCI                                 (1u << 3) /* <3:3> R:R:0:No */


/*
   (read) PortResetStatus
   (write) SetPortReset
 */
#define CY_U3P_UIB_RHP_PRS                                  (1u << 4) /* <4:4> RW0C:RW1S:0:No */


/*
   (read) PortPowerStatus
   (write) SetPortPower
   Not implemented in this IP
 */
#define CY_U3P_UIB_RHP_PPS                                  (1u << 8) /* <8:8> R:R:1:No */


/*
   (read) LowSpeedDeviceAttached
   (write) ClearPortPower
   Not implemented in this IP
 */
#define CY_U3P_UIB_RHP_LSDA                                 (1u << 9) /* <9:9> RW:R:0:No */


/*
   ConnectStatusChange
 */
#define CY_U3P_UIB_RHP_CSC                                  (1u << 16) /* <16:16> RW:RW1C:0:No */


/*
   PortEnableStatusChange
 */
#define CY_U3P_UIB_RHP_PESC                                 (1u << 17) /* <17:17> RW1S:RW1C:0:No */


/*
   PortSuspendStatusChange
 */
#define CY_U3P_UIB_RHP_PSSC                                 (1u << 18) /* <18:18> RW1S:RW1C:0:No */


/*
   PortOverCurrentIndicatorChange
   Not implemented in this IP
 */
#define CY_U3P_UIB_RHP_OCIC                                 (1u << 19) /* <19:19> R:R:0:No */


/*
   PortResetStatusChange
 */
#define CY_U3P_UIB_RHP_PRSC                                 (1u << 20) /* <20:20> RW1S:RW1C:0:No */


/*
   Port Resume (virtual register of PSS from firmware).
   For an OHCI resume initiated by the host, firmware clears the PSS bit.
   However, this bit is not supposed to go low until the resume is complete.
    The PORT_RESUME_FW bit will give us a way to signal to the Host Logic
   that it should do a resume and then go clear the PSS bit. There is no
   real functionality being added, this is only trying to emulate the OHCI
   interface better.
 */
#define CY_U3P_UIB_RHP_PORT_RESUME_FW                       (1u << 31) /* <31:31> RW:RW:0:No */



/*
   OHCI End of Frame Times
 */
#define CY_U3P_UIB_OHCI_EOF_ADDRESS                         (0xe0032058)
#define CY_U3P_UIB_OHCI_EOF                                 (*(uvint32_t *)(0xe0032058))
#define CY_U3P_UIB_OHCI_EOF_DEFAULT                         (0x00190050)

/*
   EOF1 Time  (default:  32 bit times * 2.5 clocks/bit => 80)
 */
#define CY_U3P_UIB_EOF1_MASK                                (0x0000ffff) /* <0:15> R:RW:0x0050:No */
#define CY_U3P_UIB_EOF1_POS                                 (0)


/*
   EOF2 Time  (default:  10 bit times * 2.5 clocks/bit => 25)
 */
#define CY_U3P_UIB_EOF2_MASK                                (0xffff0000) /* <16:31> R:RW:0x0019:No */
#define CY_U3P_UIB_EOF2_POS                                 (16)



/*
   Multiple Mode control (time-base bit functionality), addressing capability.
 */
#define CY_U3P_UIB_EHCI_HCCPARAMS_ADDRESS                   (0xe003205c)
#define CY_U3P_UIB_EHCI_HCCPARAMS                           (*(uvint32_t *)(0xe003205c))
#define CY_U3P_UIB_EHCI_HCCPARAMS_DEFAULT                   (0x00000000)

/*
   64-bit Addressing Capability
 */
#define CY_U3P_UIB_ADDR_64_BIT_CAP                          (1u << 0) /* <0:0> R:R:0:No */


/*
   Programmable Frame List Flag (This value is ignored in this implementation.
   The CPU programs the Scheduler memory to schedule events and appropriate
   number of Descriptors)
 */
#define CY_U3P_UIB_FRAME_LIST_FLAG                          (1u << 1) /* <1:1> R:R:0:No */


/*
   Asynchronous Schedule Park Capability
 */
#define CY_U3P_UIB_ASYNC_PARK_CAP                           (1u << 2) /* <2:2> R:RW:0:No */


/*
   Isochronous Scheduling Threshold (In this implementation the scheduler
   will cache 1 micro-frame worth of data. Appropriate value will be programmed
   to ensure correct functionality)
 */
#define CY_U3P_UIB_ISO_SHDL_THR_MASK                        (0x000000f0) /* <4:7> R:R:0:No */
#define CY_U3P_UIB_ISO_SHDL_THR_POS                         (4)


/*
   EHCI Extended Capabilities Pointer
   (Not implemented in this IP)
 */
#define CY_U3P_UIB_EECP_MASK                                (0x0000ff00) /* <8:15> R:R:0:No */
#define CY_U3P_UIB_EECP_POS                                 (8)



/*
   EHCI Command Register.
 */
#define CY_U3P_UIB_EHCI_USBCMD_ADDRESS                      (0xe0032060)
#define CY_U3P_UIB_EHCI_USBCMD                              (*(uvint32_t *)(0xe0032060))
#define CY_U3P_UIB_EHCI_USBCMD_DEFAULT                      (0x00080000)

/*
   Run/Stop
 */
#define CY_U3P_UIB_EHCI_USBCMD_RS                           (1u << 0) /* <0:0> RW:RW:0:No */


/*
   N/A Use the UIB_POWER.RESETN register
 */
#define CY_U3P_UIB_EHCI_USBCMD_HCRESET                      (1u << 1) /* <1:1> R:R:0:No */


/*
   Frame List Size
   We are not using Frame list. The CPU programs the Scheduler memory to
   schdule events and appropriate number of Descriptors
 */
#define CY_U3P_UIB_EHCI_USBCMD_FRAME_LST_SIZE_MASK          (0x0000000c) /* <2:3> R:RW:0:No */
#define CY_U3P_UIB_EHCI_USBCMD_FRAME_LST_SIZE_POS           (2)


/*
   Periodic Schedule Enable
 */
#define CY_U3P_UIB_EHCI_USBCMD_PER_SHDL_EN                  (1u << 4) /* <4:4> R:RW:0:No */


/*
   Asynchronous Schedule Enable
 */
#define CY_U3P_UIB_EHCI_USBCMD_ASYNC_SHDL_EN                (1u << 5) /* <5:5> R:RW:0:No */


/*
   Interrupt on Async Advance Doorbell (not implemented in this IP)
 */
#define CY_U3P_UIB_EHCI_USBCMD_INT_ASYNC_ADV                (1u << 6) /* <6:6> R:RW:0:No */


/*
   Light Host Controller Reset (not implemented in this IP)
 */
#define CY_U3P_UIB_EHCI_USBCMD_LIGHT_HOST_CTRL_RST          (1u << 7) /* <7:7> R:RW:0:No */


/*
   Asynchronous Schedule Park Mode Count
 */
#define CY_U3P_UIB_EHCI_USBCMD_ASYNC_SHDL_PRK_CNT_MASK      (0x00000300) /* <8:9> R:RW:0:No */
#define CY_U3P_UIB_EHCI_USBCMD_ASYNC_SHDL_PRK_CNT_POS       (8)


/*
   Asynchronous Schedule Park Mode Enable
 */
#define CY_U3P_UIB_EHCI_USBCMD_ASYNC_SHDL_PRK_EN            (1u << 11) /* <11:11> R:RW:0:No */


/*
   Interrupt Threshold Control
 */
#define CY_U3P_UIB_EHCI_USBCMD_INT_THRESHOLD_CTRL_MASK      (0x00ff0000) /* <16:23> R:RW:8:No */
#define CY_U3P_UIB_EHCI_USBCMD_INT_THRESHOLD_CTRL_POS       (16)



/*
   Pending interrupts and various states of the Host Controller
 */
#define CY_U3P_UIB_EHCI_USBSTS_ADDRESS                      (0xe0032064)
#define CY_U3P_UIB_EHCI_USBSTS                              (*(uvint32_t *)(0xe0032064))
#define CY_U3P_UIB_EHCI_USBSTS_DEFAULT                      (0x00001000)

/*
   USB Interrupt.
   Note: This field will not assert for SETUP+IN(STATUS) qTDs with no data
   phase.  The work around is to use the HOST_EP_INTR[0] along with the transaction
   response that gets written into SRAM for HCD.
   
 */
#define CY_U3P_UIB_EHCI_USBSTS_USBINT                       (1u << 0) /* <0:0> RW1S:RW1C:0:No */


/*
   USB Error Interrupt
 */
#define CY_U3P_UIB_EHCI_USBSTS_USBERRINT                    (1u << 1) /* <1:1> RW1S:RW1C:0:No */


/*
   Port Change Detect
 */
#define CY_U3P_UIB_EHCI_USBSTS_PORT_CHNG_DET                (1u << 2) /* <2:2> RW1S:RW1C:0:No */


/*
   Frame List Rollover (not implemented in this IP)
 */
#define CY_U3P_UIB_EHCI_USBSTS_FRAME_LIST_RLVR              (1u << 3) /* <3:3> R:R:0:No */


/*
   Host System Error
   EHCI over scheduling Status
 */
#define CY_U3P_UIB_EHCI_USBSTS_HOST_SYS_ERR                 (1u << 4) /* <4:4> RW1S:RW1C:0:No */


/*
   Interrupt on Async Advance
   Not implemented in this IP
 */
#define CY_U3P_UIB_EHCI_USBSTS_INT_ASYNC_ADV                (1u << 5) /* <5:5> R:R:0:No */


/*
   This bit is a zero whenever the Run/Stop bit is a one. The HC sets this
   bit to one after it has stopped executing as a result of the Run/Stop
   bit being set to 0 by software.
 */
#define CY_U3P_UIB_EHCI_USBSTS_HC_HALTED                    (1u << 12) /* <12:12> RW:R:1:No */


/*
   Reclamation
 */
#define CY_U3P_UIB_EHCI_USBSTS_RECLAMATION                  (1u << 13) /* <13:13> RW:R:0:No */


/*
   Periodic Schedule Status
 */
#define CY_U3P_UIB_EHCI_USBSTS_PER_SHDL_ST                  (1u << 14) /* <14:14> RW:R:0:No */


/*
   Asynchronous Schedule Status
 */
#define CY_U3P_UIB_EHCI_USBSTS_ASYNC_SHDL_ST                (1u << 15) /* <15:15> RW:R:0:No */



/*
   EHCI Interrupt Register
 */
#define CY_U3P_UIB_EHCI_USBINTR_ADDRESS                     (0xe0032068)
#define CY_U3P_UIB_EHCI_USBINTR                             (*(uvint32_t *)(0xe0032068))
#define CY_U3P_UIB_EHCI_USBINTR_DEFAULT                     (0x00000000)

/*
   USB Interrupt Enable
 */
#define CY_U3P_UIB_USBINT_IE                                (1u << 0) /* <0:0> R:RW:0:N/A */


/*
   USB Error Interrupt Enable
 */
#define CY_U3P_UIB_USBERRINT_IE                             (1u << 1) /* <1:1> R:RW:0:N/A */


/*
   Port Change Interrupt Enable
 */
#define CY_U3P_UIB_PORT_CHANGE_DET_IE                       (1u << 2) /* <2:2> R:RW:0:N/A */


/*
   Frame List Rollover Enable
   Not implemented in this IP
 */
#define CY_U3P_UIB_FRAME_LIST_RL_IE                         (1u << 3) /* <3:3> R:R:0:N/A */


/*
   Host System Error Interrupt Enable
 */
#define CY_U3P_UIB_HOST_SYS_ERR_IE                          (1u << 4) /* <4:4> R:RW:0:N/A */


/*
   Interrupt on Async Advance
   Not implemented in this IP
 */
#define CY_U3P_UIB_ASYNC_SHDL_EN                            (1u << 5) /* <5:5> R:R:0:N/A */



/*
   Frame Index Register
 */
#define CY_U3P_UIB_EHCI_FRINDEX_ADDRESS                     (0xe003206c)
#define CY_U3P_UIB_EHCI_FRINDEX                             (*(uvint32_t *)(0xe003206c))
#define CY_U3P_UIB_EHCI_FRINDEX_DEFAULT                     (0x00000000)

/*
   The value indicates the Frame on with the scheduler is operating on.This
   value is also used to achieve interrupt endpoint polling duration.
 */
#define CY_U3P_UIB_FRINDEX_MASK                             (0x00003fff) /* <0:13> RW:R:0:No */
#define CY_U3P_UIB_FRINDEX_POS                              (0)



/*
   Configure Flag Register
 */
#define CY_U3P_UIB_EHCI_CONFIGFLAG_ADDRESS                  (0xe0032070)
#define CY_U3P_UIB_EHCI_CONFIGFLAG                          (*(uvint32_t *)(0xe0032070))
#define CY_U3P_UIB_EHCI_CONFIGFLAG_DEFAULT                  (0x00000000)

/*
   Configure Flag
 */
#define CY_U3P_UIB_CF                                       (1u << 0) /* <0:0> R:RW:0:No */



/*
   Port status and control Register.
 */
#define CY_U3P_UIB_EHCI_PORTSC_ADDRESS                      (0xe0032074)
#define CY_U3P_UIB_EHCI_PORTSC                              (*(uvint32_t *)(0xe0032074))
#define CY_U3P_UIB_EHCI_PORTSC_DEFAULT                      (0x00002000)

/*
   Current Connect Status
 */
#define CY_U3P_UIB_PORT_CONNECT                             (1u << 0) /* <0:0> RW:R:0:No */


/*
   Connect Status Change
 */
#define CY_U3P_UIB_PORT_CONNECT_C                           (1u << 1) /* <1:1> RW1S:RW1C:0:No */


/*
   Port Enabled/Disabled
 */
#define CY_U3P_UIB_PORT_EN                                  (1u << 2) /* <2:2> RW:RW0C:0:No */


/*
   Port Enable/Disable Change
 */
#define CY_U3P_UIB_PORT_EN_C                                (1u << 3) /* <3:3> RW1S:RW1C:0:No */


/*
   Over-current Active
   Not implemented in this IP
 */
#define CY_U3P_UIB_OC_ACTIVE                                (1u << 4) /* <4:4> R:R:0:No */


/*
   Over-current Change
   Not implemented in this IP
 */
#define CY_U3P_UIB_OC_CHNG                                  (1u << 5) /* <5:5> R:R:0:No */


/*
   Force Port Resume
 */
#define CY_U3P_UIB_F_PORT_RESUME                            (1u << 6) /* <6:6> RW:RW:0:No */


/*
   Suspend
 */
#define CY_U3P_UIB_PORT_SUSPEND                             (1u << 7) /* <7:7> RW:RW:0:No */


/*
   Port Reset
   Firmware sets this bit to initiate Port Reset and subsequently writes
   a 0 to it to initiate the end of Port Reset.  Once the Host has completed
   Port Reset, it will clear this bit.  Firmware must poll this bit to detemine
   the end of the Port Reset, and also whether the attached device is High-Speed
   (PORT_EN == 1).
   [EHCI:  §2.3.9, Table 2-16, p 28; §4.2.2,p 56]
 */
#define CY_U3P_UIB_PORT_RESET                               (1u << 8) /* <8:8> RW:RW1S:0:No */


/*
   Line Status
 */
#define CY_U3P_UIB_EHCI_LINE_STATE_MASK                     (0x00000c00) /* <10:11> RW:R:0:No */
#define CY_U3P_UIB_EHCI_LINE_STATE_POS                      (10)


/*
   Port Power
   Not implemented in this IP
 */
#define CY_U3P_UIB_PP                                       (1u << 12) /* <12:12> R:R:0:No */


/*
   Port Owner
 */
#define CY_U3P_UIB_PORT_OWNER                               (1u << 13) /* <13:13> RW:RW:1:No */


/*
   Port Indicator Control
   Not implemented in this IP. Firmware will implement these with GPIOs if
   needed.
 */
#define CY_U3P_UIB_PORT_INDI_CNTRL_MASK                     (0x0000c000) /* <14:15> R:RW:0:No */
#define CY_U3P_UIB_PORT_INDI_CNTRL_POS                      (14)


/*
   Port Test Control.
   Not implemented in this IP.
 */
#define CY_U3P_UIB_PORT_TEST_CTRL_MASK                      (0x000f0000) /* <16:19> R:RW:0:No */
#define CY_U3P_UIB_PORT_TEST_CTRL_POS                       (16)


/*
   Wake on Connect Enable.
   Not implemented in this IP.
 */
#define CY_U3P_UIB_WKCNNT_E                                 (1u << 20) /* <20:20> R:RW:0:No */


/*
   Wake on Disconnect Enable.
   Not implemented in this IP.
 */
#define CY_U3P_UIB_WKDSCNNT_E                               (1u << 21) /* <21:21> R:RW:0:No */


/*
   Wake on Over-current Enable.
   Not implemented in this IP.
 */
#define CY_U3P_UIB_WKOC_E                                   (1u << 22) /* <22:22> R:RW:0:No */


/*
   Hardware Initiated Resume Active
 */
#define CY_U3P_UIB_PORT_RESUME_HW                           (1u << 30) /* <30:30> RW:R:0:No */


/*
   Port Reset (virtual register of PORT_RESET from firmware)
 */
#define CY_U3P_UIB_PORT_RESET_FW                            (1u << 31) /* <31:31> R:RW:0:No */



/*
   EHCI End of Frame Times
 */
#define CY_U3P_UIB_EHCI_EOF_ADDRESS                         (0xe0032078)
#define CY_U3P_UIB_EHCI_EOF                                 (*(uvint32_t *)(0xe0032078))
#define CY_U3P_UIB_EHCI_EOF_DEFAULT                         (0x00040023)

/*
   EOF1 Time  (default:  560 bit times / 16 bits/clock -> 35)
 */
#define CY_U3P_UIB_EOF1_MASK                                (0x0000ffff) /* <0:15> R:RW:0x0023:No */
#define CY_U3P_UIB_EOF1_POS                                 (0)


/*
   EOF2 Time  (default:    64 bit times / 16 bits/clock ->   4)
 */
#define CY_U3P_UIB_EOF2_MASK                                (0xffff0000) /* <16:31> R:RW:0x0004:No */
#define CY_U3P_UIB_EOF2_POS                                 (16)



/*
   Scheduler Change Type Register
 */
#define CY_U3P_UIB_SHDL_CHNG_TYPE_ADDRESS                   (0xe003207c)
#define CY_U3P_UIB_SHDL_CHNG_TYPE                           (*(uvint32_t *)(0xe003207c))
#define CY_U3P_UIB_SHDL_CHNG_TYPE_DEFAULT                   (0x00000000)

/*
   0: Update at Micro-frame boundary. 1: Update at Frame boundary
 */
#define CY_U3P_UIB_EP0_CHNG_TYPE_MASK                       (0x0000ffff) /* <0:15> R:RW:0:No */
#define CY_U3P_UIB_EP0_CHNG_TYPE_POS                        (0)


/*
   0: Update at Micro-frame boundary. 1: Update at Frame boundary
 */
#define CY_U3P_UIB_EPI_CHNG_TYPE_MASK                       (0xffff0000) /* <16:31> R:RW:0:No */
#define CY_U3P_UIB_EPI_CHNG_TYPE_POS                        (16)



/*
   Scheduler State machine
 */
#define CY_U3P_UIB_SHDL_STATE_MACHINE_ADDRESS               (0xe0032080)
#define CY_U3P_UIB_SHDL_STATE_MACHINE                       (*(uvint32_t *)(0xe0032080))
#define CY_U3P_UIB_SHDL_STATE_MACHINE_DEFAULT               (0x00000001)

/*
   Idle
 */
#define CY_U3P_UIB_IDLE                                     (1u << 0) /* <0:0> RW:R:1:No */


/*
   Load Pointer State
 */
#define CY_U3P_UIB_LOAD_PTR                                 (1u << 1) /* <1:1> RW:R:0:No */


/*
   Fetch State
 */
#define CY_U3P_UIB_FETCH                                    (1u << 2) /* <2:2> RW:R:0:No */


/*
   Scratch Read state0
 */
#define CY_U3P_UIB_SCRATCH_READ0                            (1u << 3) /* <3:3> RW:R:0:No */


/*
   Scratch Read state1
 */
#define CY_U3P_UIB_SCRATCH_READ1                            (1u << 4) /* <4:4> RW:R:0:No */


/*
   Scratch Read state2
 */
#define CY_U3P_UIB_SCRATCH_READ2                            (1u << 5) /* <5:5> RW:R:0:No */


/*
   Load scheduler memory state
 */
#define CY_U3P_UIB_LOAD_SHDL_MEM                            (1u << 6) /* <6:6> RW:R:0:No */


/*
   Read EP0 state0
 */
#define CY_U3P_UIB_READ_EP0_0                               (1u << 7) /* <7:7> RW:R:0:No */


/*
   Read EP0 state1
 */
#define CY_U3P_UIB_READ_EP0_1                               (1u << 8) /* <8:8> RW:R:0:No */


/*
   First Eval State
 */
#define CY_U3P_UIB_FIRST_EVAL                               (1u << 9) /* <9:9> RW:R:0:No */


/*
   Async Sleep state
 */
#define CY_U3P_UIB_ASYNC_SLEEP                              (1u << 10) /* <10:10> RW:R:0:No */


/*
   Execute State
 */
#define CY_U3P_UIB_EXECUTE                                  (1u << 11) /* <11:11> RW:R:0:No */


/*
   Wait for TP status state
 */
#define CY_U3P_UIB_WAIT_TP_STUPD                            (1u << 12) /* <12:12> RW:R:0:No */


/*
   Last Eval State
 */
#define CY_U3P_UIB_LAST_EVAL                                (1u << 13) /* <13:13> RW:R:0:No */


/*
   Scheduler write state
 */
#define CY_U3P_UIB_SHDL_WRITE                               (1u << 14) /* <14:14> RW:R:0:No */


/*
   Scratch Write state1
 */
#define CY_U3P_UIB_SCRATCH_WRITE1                           (1u << 15) /* <15:15> RW:R:0:No */


/*
   Scratch Write state2
 */
#define CY_U3P_UIB_SCRATCH_WRITE2                           (1u << 16) /* <16:16> RW:R:0:No */


/*
   Wait for scheduler enable state
 */
#define CY_U3P_UIB_WAIT_SHDL_EN                             (1u << 17) /* <17:17> RW:R:0:No */


/*
   Wait for EOF state
 */
#define CY_U3P_UIB_WAIT_EOF                                 (1u << 18) /* <18:18> RW:R:0:No */



/*
   Scheduler Internal Status
 */
#define CY_U3P_UIB_SHDL_INTERNAL_STATUS_ADDRESS             (0xe0032084)
#define CY_U3P_UIB_SHDL_INTERNAL_STATUS                     (*(uvint32_t *)(0xe0032084))
#define CY_U3P_UIB_SHDL_INTERNAL_STATUS_DEFAULT             (0x00000001)

/*
   EP Done
 */
#define CY_U3P_UIB_EP_DONE                                  (1u << 0) /* <0:0> RW:R:1:No */


/*
   Frame Fit
 */
#define CY_U3P_UIB_FRAME_FIT                                (1u << 1) /* <1:1> RW:R:0:No */


/*
   EP0 SETUP state
 */
#define CY_U3P_UIB_EP0_SETUP                                (1u << 2) /* <2:2> RW:R:0:No */


/*
   EP0 OUT state
 */
#define CY_U3P_UIB_EP0_OUT                                  (1u << 3) /* <3:3> RW:R:0:No */


/*
   EP0 IN state
 */
#define CY_U3P_UIB_EP0_IN                                   (1u << 4) /* <4:4> RW:R:0:No */


/*
   TP Ack
 */
#define CY_U3P_UIB_TP_ACK                                   (1u << 5) /* <5:5> RW:R:0:No */


/*
   TP NAK
 */
#define CY_U3P_UIB_TP_NAK                                   (1u << 6) /* <6:6> RW:R:0:No */


/*
   TP NYET
 */
#define CY_U3P_UIB_TP_NYET                                  (1u << 7) /* <7:7> RW:R:0:No */


/*
   TP STALL
 */
#define CY_U3P_UIB_TP_STALL                                 (1u << 8) /* <8:8> RW:R:0:No */


/*
   TP DT mismatch
 */
#define CY_U3P_UIB_TP_DT_MISMATCH                           (1u << 9) /* <9:9> RW:R:0:No */


/*
   TP CRC16 Error
 */
#define CY_U3P_UIB_TP_CRC16_ERROR                           (1u << 10) /* <10:10> RW:R:0:No */


/*
   TP PHY rxerror
 */
#define CY_U3P_UIB_TP_PHY_ERROR                             (1u << 11) /* <11:11> RW:R:0:No */


/*
   TP Port error
 */
#define CY_U3P_UIB_TP_PORT_ERROR                            (1u << 12) /* <12:12> RW:R:0:No */


/*
   TP Babble
 */
#define CY_U3P_UIB_TP_BABBLE                                (1u << 13) /* <13:13> RW:R:0:No */


/*
   TP PID error
 */
#define CY_U3P_UIB_TP_PID_ERROR                             (1u << 14) /* <14:14> RW:R:0:No */


/*
   TP timeout
 */
#define CY_U3P_UIB_TP_TIMEOUT                               (1u << 15) /* <15:15> RW:R:0:No */


/*
   TP EPM under-run
 */
#define CY_U3P_UIB_TP_EPM_UNDERRUN                          (1u << 16) /* <16:16> RW:R:0:No */


/*
   TP EPM over-run
 */
#define CY_U3P_UIB_TP_EPM_OVERRUN                           (1u << 17) /* <17:17> RW:R:0:No */



/*
   Scheduler memory, OHCI format
 */
#define CY_U3P_UIB_SHDL_OHCI0_ADDRESS(n)                    (0xe0032400 + ((n) * (0x000c)))
#define CY_U3P_UIB_SHDL_OHCI0(n)                            (*(uvint32_t *)(0xe0032400 + ((n) * 0x000c)))
#define CY_U3P_UIB_SHDL_OHCI0_DEFAULT                       (0x00000000)

/*
   [3:0]: EP number, Values:0-15
    [4]: EP Direction, 1: OUT   0: IN
   The direction bit is not used for EP0. Scheduler uses the EP0_code to
   determine the direction of the EP0 transaction.
 */
#define CY_U3P_UIB_OHCI0_EPND_MASK                          (0x0000001f) /* <0:4> R:RW:0:No */
#define CY_U3P_UIB_OHCI0_EPND_POS                           (0)


/*
   The End Point Type.
   00: Control
   01: Isochronous
   10: Bulk
   11: Interrupt
 */
#define CY_U3P_UIB_OHCI0_EPT_MASK                           (0x00000060) /* <5:6> R:RW:0:No */
#define CY_U3P_UIB_OHCI0_EPT_POS                            (5)


/*
   This field is used only for non-ISO endpoints.
   0: Scheduler will set the active bit to zero when the total byte count
   reaches zero.
   1: Scheduler will set the active bit to zero when the total byte count
   is zero and a ZLP has been sent/received.
 */
#define CY_U3P_UIB_OHCI0_ZPLEN                              (1u << 7) /* <7:7> R:RW:0:No */


/*
   0: Not End of the Period/Asynchronous list
   1: End of the Period/Asynchronous list
   This bit indicates that there are no more valid entries in the current
   list.
 */
#define CY_U3P_UIB_OHCI0_T                                  (1u << 8) /* <8:8> R:RW:0:No */


/*
   This will indicate that the EP has been halted.
   Skip this qTD and move to next qTD.
 */
#define CY_U3P_UIB_OHCI0_HALT                               (1u << 9) /* <9:9> R:RW:0:No */


/*
   Should be programmed to zero for OHCI.
   This field is a counter the host controller decrements whenever a transaction
   for the endpoint associated with this queue head results in a Nak or Nyet
   response.
 */
#define CY_U3P_UIB_OHCI0_NAK_CNT_MASK                       (0x00003c00) /* <10:13> R:RW:0:No */
#define CY_U3P_UIB_OHCI0_NAK_CNT_POS                        (10)


/*
   This field is a 2-bit down counter that keeps track of the number of consecutive
   Errors detected while executing this qTD.
   If this field is programmed with a non-zero value during setup, the Host
   Controller decrements the count and writes it back to the qTD if the transaction
   fails. If the counter counts from one to zero, the Host Controller marks
   the qTD inactive, sets the Halted bit to a one and error status bit for
   the error that caused CERR to decrement to zero. An interrupt will be
   generated if the USB Error Interrupt Enable bit in the USBINTR register
   is set to a one. If HCD programs this field to zero during setup, the
   Host Controller will not count errors for this qTD and there will be no
   limit on the retries of this qTD.
 */
#define CY_U3P_UIB_OHCI0_CERR_MASK                          (0x0000c000) /* <14:15> R:RW:0:No */
#define CY_U3P_UIB_OHCI0_CERR_POS                           (14)


/*
   0: For ISO-OUT when EPM is empty for that EP then a ZLP will be issued
   by TP.
        For ISO-IN when EPM is full for that EP then and ISO-IN will be issued.
     1: For ISO-OUT when EPM is empty for that EP then that entry will be
   skipped.
        For ISO-IN when EPM is full for that EP then that entry will be skipped.
 */
#define CY_U3P_UIB_OHCI0_ISO_EPM                            (1u << 16) /* <16:16> R:RW:0:No */


/*
   High-Bandwidth Pipe Multiplier. This field is a multiplier used to key
   the host controller as the number of successive packets the host controller
   may submit to the endpoint in the current execution. This field should
   be programmed according to the EPM configuration for that EP. This field
   will get decremented only if the transfer is successful. If not successful,
   the original value will be reloaded.
   00: Reserved. A zero in this field yields undefined results.
   01: One transaction to be issued for this endpoint per micro-frame
   10: Two transactions to be issued for this endpoint per micro-frame
   11: Three transactions to be issued for this endpoint per micro-frame
 */
#define CY_U3P_UIB_OHCI0_MULT_MASK                          (0x00060000) /* <17:18> R:RW:0:No */
#define CY_U3P_UIB_OHCI0_MULT_POS                           (17)


/*
   Should be programmed to zero for OHCI.
   Nak Count Reload (RL). This field contains a value, which is used by the
   host controller to reload the Nak Counter field.
 */
#define CY_U3P_UIB_OHCI0_RL_MASK                            (0x00780000) /* <19:22> R:RW:0:No */
#define CY_U3P_UIB_OHCI0_RL_POS                             (19)


/*
   Should be programmed to zero for OHCI.
   This filed enables the host to issue Ping token when Direction is OUT
   and it is high-speed.
   0: Do not issue ping token for high-speed OUT
   1: Do issue ping token for high-speed OUT
 */
#define CY_U3P_UIB_OHCI0_PING                               (1u << 23) /* <23:23> R:RW:0:No */


/*
   Should be programmed to zero for OHCI.
   The host controller uses the value of the three low-order bits of the
   FRINDEX register as an index into a bit position in this bit vector. If
   the µFrame S-mask field has a one at the indexed bit position then this
   queue head is a candidate for transaction execution.
 */
#define CY_U3P_UIB_OHCI0_UFRAME_SMASK_MASK                  (0xff000000) /* <24:31> R:RW:0:No */
#define CY_U3P_UIB_OHCI0_UFRAME_SMASK_POS                   (24)



/*
   Scheduler memory, OHCI format
 */
#define CY_U3P_UIB_SHDL_OHCI1_ADDRESS(n)                    (0xe0032404 + ((n) * (0x000c)))
#define CY_U3P_UIB_SHDL_OHCI1(n)                            (*(uvint32_t *)(0xe0032404 + ((n) * 0x000c)))
#define CY_U3P_UIB_SHDL_OHCI1_DEFAULT                       (0x00000000)

/*
   This directly corresponds to the maximum packet size of the associated
   endpoint (wMaxPacketSize). The maximum value this field may contain is
   0x400 (1024).
 */
#define CY_U3P_UIB_OHCI1_MAX_PKT_SIZE_MASK                  (0x000007ff) /* <0:10> R:RW:0:No */
#define CY_U3P_UIB_OHCI1_MAX_PKT_SIZE_POS                   (0)


/*
   00h: Reserved.
   01h:   1 ms (Every     frame)
   02h:   2 ms (Every  2 frames)
   04h:   4 ms (Every  4 frames)
   08h:   8 ms (Every  8 frames)
   10h: 16 ms (Every 16 frames)
   20h: 32 ms (Every 32 frames)
   This will be on top of the Interrupt Schedule Mask in EHCI case.
 */
#define CY_U3P_UIB_OHCI1_POLLING_RATE_MASK                  (0x0007f800) /* <11:18> R:RW:0:No */
#define CY_U3P_UIB_OHCI1_POLLING_RATE_POS                   (11)


/*
   After how many packet the response should be written into SRAM.
   The packet counter increments based on the condition specified in the
   UIB_HOST_RESP_CS.WR_RESP_COND.
 */
#define CY_U3P_UIB_OHCI1_RESP_RATE_MASK                     (0x07f80000) /* <19:26> R:RW:0:No */
#define CY_U3P_UIB_OHCI1_RESP_RATE_POS                      (19)


/*
   This filed is used to expand the MULT field to support the OHCI ControlBulkServiceRatio
   bandwidth allocation.
 */
#define CY_U3P_UIB_OHCI1_MMULT_MASK                         (0x18000000) /* <27:28> R:RW:0:No */
#define CY_U3P_UIB_OHCI1_MMULT_POS                          (27)


/*
   This bit will disable any error that would cause an EP to be de-activated.
    This bit should be used only for Isochronous Endpoint.
 */
#define CY_U3P_UIB_OHCI1_BYPASS_ERROR                       (1u << 29) /* <29:29> R:RW:0:No */


/*
   This field represents the EP0 type of transaction:
   00: Reserved
   01: Setup+Data Phase(OUT)+Status Phase(IN)
   10: Setup+Data Phase(IN)+Status Phase(OUT)
   11: Setup+Status Phase(IN)
 */
#define CY_U3P_UIB_OHCI1_EP0_CODE_MASK                      (0xc0000000) /* <30:31> R:RW:0:No */
#define CY_U3P_UIB_OHCI1_EP0_CODE_POS                       (30)



/*
   Scheduler memory, OHCI format
 */
#define CY_U3P_UIB_SHDL_OHCI2_ADDRESS(n)                    (0xe0032408 + ((n) * (0x000c)))
#define CY_U3P_UIB_SHDL_OHCI2(n)                            (*(uvint32_t *)(0xe0032408 + ((n) * 0x000c)))
#define CY_U3P_UIB_SHDL_OHCI2_DEFAULT                       (0x00000000)

/*
   Total number of byte count for the transaction.
 */
#define CY_U3P_UIB_OHCI2_TOTAL_BYTE_COUNT_MASK              (0x0000ffff) /* <0:15> R:RW:0:No */
#define CY_U3P_UIB_OHCI2_TOTAL_BYTE_COUNT_POS               (0)


/*
   0: Packet mode
   1: Stream mode
 */
#define CY_U3P_UIB_OHCI2_TRNS_MODE                          (1u << 16) /* <16:16> R:RW:0:No */


/*
   Should be programmed to zero for OHCI.
   After IOC_rate*resp_rate the scheduler will issue interrupt at the next
   interrupt threshold (UIB_EHCI_USBCMD.INT_THRESHOLD_CTRL)
 */
#define CY_U3P_UIB_OHCI2_IOC_RATE_MASK                      (0x01fe0000) /* <17:24> R:RW:0:No */
#define CY_U3P_UIB_OHCI2_IOC_RATE_POS                       (17)



/*
   Scheduler memory, EHCI format
 */
#define CY_U3P_UIB_SHDL_EHCI0_ADDRESS(n)                    (0xe0032800 + ((n) * (0x000c)))
#define CY_U3P_UIB_SHDL_EHCI0(n)                            (*(uvint32_t *)(0xe0032800 + ((n) * 0x000c)))
#define CY_U3P_UIB_SHDL_EHCI0_DEFAULT                       (0x00000000)

/*
   [3:0]: EP number, Values:0-15
    [4]: EP Direction, 1: OUT   0: IN
   The direction bit is not used for EP0. Scheduler uses the EP0_code to
   determine the direction of the EP0 transaction.
 */
#define CY_U3P_UIB_EHCI0_EPND_MASK                          (0x0000001f) /* <0:4> R:RW:0:No */
#define CY_U3P_UIB_EHCI0_EPND_POS                           (0)


/*
   The End Point Type.
   00: Control
   01: Isochronous
   10: Bulk
   11: Interrupt
 */
#define CY_U3P_UIB_EHCI0_EPT_MASK                           (0x00000060) /* <5:6> R:RW:0:No */
#define CY_U3P_UIB_EHCI0_EPT_POS                            (5)


/*
   This field is used only for non-ISO endpoints.
   0: Scheduler will set the active bit to zero when the total byte count
   reaches zero.
   1: Scheduler will set the active bit to zero when the total byte count
   is zero and a ZLP has been sent/received.
 */
#define CY_U3P_UIB_EHCI0_ZPLEN                              (1u << 7) /* <7:7> R:RW:0:No */


/*
   0: Not End of the Period/Asynchronous list
   1: End of the Period/Asynchronous list
   This bit indicates that there are no more valid entries in the current
   list.
 */
#define CY_U3P_UIB_EHCI0_T                                  (1u << 8) /* <8:8> R:RW:0:No */


/*
   This will indicate that the EP has been halted.
   Skip this qTD and move to next qTD.
 */
#define CY_U3P_UIB_EHCI0_HALT                               (1u << 9) /* <9:9> R:RW:0:No */


/*
   This field is a counter the host controller decrements whenever a transaction
   for the endpoint associated with this queue head results in a Nak or Nyet
   response.
 */
#define CY_U3P_UIB_EHCI0_NAK_CNT_MASK                       (0x00003c00) /* <10:13> R:RW:0:No */
#define CY_U3P_UIB_EHCI0_NAK_CNT_POS                        (10)


/*
   This field is a 2-bit down counter that keeps track of the number of consecutive
   Errors detected while executing this qTD.
   If this field is programmed with a non-zero value during setup, the Host
   Controller decrements the count and writes it back to the qTD if the transaction
   fails. If the counter counts from one to zero, the Host Controller marks
   the qTD inactive, sets the Halted bit to a one and error status bit for
   the error that caused CERR to decrement to zero. An interrupt will be
   generated if the USB Error Interrupt Enable bit in the USBINTR register
   is set to a one. If HCD programs this field to zero during setup, the
   Host Controller will not count errors for this qTD and there will be no
   limit on the retries of this qTD.
 */
#define CY_U3P_UIB_EHCI0_CERR_MASK                          (0x0000c000) /* <14:15> R:RW:0:No */
#define CY_U3P_UIB_EHCI0_CERR_POS                           (14)


/*
   0: For ISO-OUT when EPM is empty for that EP then a ZLP will be issued
   by TP.
        For ISO-IN when EPM is full for that EP then and ISO-IN will be issued.
     1: For ISO-OUT when EPM is empty for that EP then that entry will be
   skipped.
        For ISO-IN when EPM is full for that EP then that entry will be skipped.
 */
#define CY_U3P_UIB_EHCI0_ISO_EPM                            (1u << 16) /* <16:16> R:RW:0:No */


/*
   High-Bandwidth Pipe Multiplier. This field is a multiplier used to key
   the host controller as the number of successive packets the host controller
   may submit to the endpoint in the current execution.
   This field should be programmed according to the EPM configuration for
   that EP.
   This field will get decremented only if the transfer is successful. If
   not successful, the original value will be reloaded.
   00: Reserved. A zero in this field yields undefined results.
   01: One transaction to be issued for this endpoint per micro-frame
   10: Two transactions to be issued for this endpoint per micro-frame
   11: Three transactions to be issued for this endpoint per micro-frame
 */
#define CY_U3P_UIB_EHCI0_MULT_MASK                          (0x00060000) /* <17:18> R:RW:0:No */
#define CY_U3P_UIB_EHCI0_MULT_POS                           (17)


/*
   Nak Count Reload (RL). This field contains a value, which is used by the
   host controller to reload the Nak Counter field.
 */
#define CY_U3P_UIB_EHCI0_RL_MASK                            (0x00780000) /* <19:22> R:RW:0:No */
#define CY_U3P_UIB_EHCI0_RL_POS                             (19)


/*
   This filed enables the host to issue Ping token when Direction is OUT
   and it is high-speed.
   0: Do not issue ping token for high-speed OUT
   1: Do issue ping token for high-speed OUT
 */
#define CY_U3P_UIB_EHCI0_PING                               (1u << 23) /* <23:23> R:RW:0:No */


/*
   The host controller uses the value of the three low-order bits of the
   FRINDEX register as an index into a bit position in this bit vector. If
   the µFrame S-mask field has a one at the indexed bit position then this
   queue head is a candidate for transaction execution.
 */
#define CY_U3P_UIB_EHCI0_UFRAME_SMASK_MASK                  (0xff000000) /* <24:31> R:RW:0:No */
#define CY_U3P_UIB_EHCI0_UFRAME_SMASK_POS                   (24)



/*
   Scheduler memory, EHCI format
 */
#define CY_U3P_UIB_SHDL_EHCI1_ADDRESS(n)                    (0xe0032804 + ((n) * (0x000c)))
#define CY_U3P_UIB_SHDL_EHCI1(n)                            (*(uvint32_t *)(0xe0032804 + ((n) * 0x000c)))
#define CY_U3P_UIB_SHDL_EHCI1_DEFAULT                       (0x00000000)

/*
   This directly corresponds to the maximum packet size of the associated
   endpoint (wMaxPacketSize). The maximum value this field may contain is
   0x400 (1024).
 */
#define CY_U3P_UIB_EHCI1_MAX_PKT_SIZE_MASK                  (0x000007ff) /* <0:10> R:RW:0:No */
#define CY_U3P_UIB_EHCI1_MAX_PKT_SIZE_POS                   (0)


/*
   00h: Reserved.
   01h:   1 ms (Every     frame)
   02h:   2 ms (Every  2 frames)
   04h:   4 ms (Every  4 frames)
   08h:   8 ms (Every  8 frames)
   10h: 16 ms (Every 16 frames)
   20h: 32 ms (Every 32 frames)
   This will be on top of the Interrupt Schedule Mask in EHCI case.
 */
#define CY_U3P_UIB_EHCI1_POLLING_RATE_MASK                  (0x0007f800) /* <11:18> R:RW:0:No */
#define CY_U3P_UIB_EHCI1_POLLING_RATE_POS                   (11)


/*
   After how many packet the response should be written into SRAM.
   The packet counter increments based on the condition specified in the
   UIB_HOST_RESP_CS.WR_RESP_COND.
 */
#define CY_U3P_UIB_EHCI1_RESP_RATE_MASK                     (0x07f80000) /* <19:26> R:RW:0:No */
#define CY_U3P_UIB_EHCI1_RESP_RATE_POS                      (19)


/*
   Should be programmed to zero for EHCI.
   This filed is used to expand the MULT field to support the OHCI ControlBulkServiceRatio
   bandwidth allocation.
 */
#define CY_U3P_UIB_EHCI1_MMULT_MASK                         (0x18000000) /* <27:28> R:RW:0:No */
#define CY_U3P_UIB_EHCI1_MMULT_POS                          (27)


/*
   This bit will disable any error that would cause an EP to be de-activated.
    This bit should be used only for Isochronous Endpoint.
 */
#define CY_U3P_UIB_EHCI1_BYPASS_ERROR                       (1u << 29) /* <29:29> R:RW:0:No */


/*
   This field represents the EP0 type of transaction:
   00: Reserved
   01: Setup+Data Phase(OUT)+Status Phase(IN)
   10: Setup+Data Phase(IN)+Status Phase(OUT)
   11: Setup+Status Phase(IN)
 */
#define CY_U3P_UIB_EHCI1_EP0_CODE_MASK                      (0xc0000000) /* <30:31> R:RW:0:No */
#define CY_U3P_UIB_EHCI1_EP0_CODE_POS                       (30)



/*
   Scheduler memory, EHCI format
 */
#define CY_U3P_UIB_SHDL_EHCI2_ADDRESS(n)                    (0xe0032808 + ((n) * (0x000c)))
#define CY_U3P_UIB_SHDL_EHCI2(n)                            (*(uvint32_t *)(0xe0032808 + ((n) * 0x000c)))
#define CY_U3P_UIB_SHDL_EHCI2_DEFAULT                       (0x00000000)

/*
   Total number of byte count for the transaction.
 */
#define CY_U3P_UIB_EHCI2_TOTAL_BYTE_COUNT_MASK              (0x0000ffff) /* <0:15> R:RW:0:No */
#define CY_U3P_UIB_EHCI2_TOTAL_BYTE_COUNT_POS               (0)


/*
   0: Packet mode
   1: Stream mode
 */
#define CY_U3P_UIB_EHCI2_TRNS_MODE                          (1u << 16) /* <16:16> R:RW:0:No */


/*
   After IOC_rate*resp_rate the scheduler will issue interrupt at the next
   interrupt threshold (UIB_EHCI_USBCMD.INT_THRESHOLD_CTRL)
 */
#define CY_U3P_UIB_EHCI2_IOC_RATE_MASK                      (0x01fe0000) /* <17:24> R:RW:0:No */
#define CY_U3P_UIB_EHCI2_IOC_RATE_POS                       (17)



/*
   Block Identification and Version Number
 */
#define CY_U3P_UIB_ID_ADDRESS                               (0xe0037f00)
#define CY_U3P_UIB_ID                                       (*(uvint32_t *)(0xe0037f00))
#define CY_U3P_UIB_ID_DEFAULT                               (0x00010003)

/*
   A unique number identifying the IP in the memory space.
 */
#define CY_U3P_UIB_BLOCK_ID_MASK                            (0x0000ffff) /* <0:15> R:R:0x0003:N/A */
#define CY_U3P_UIB_BLOCK_ID_POS                             (0)


/*
   Version number for the IP.
 */
#define CY_U3P_UIB_BLOCK_VERSION_MASK                       (0xffff0000) /* <16:31> R:R:0x0001:N/A */
#define CY_U3P_UIB_BLOCK_VERSION_POS                        (16)



/*
   Power, clock and reset control
 */
#define CY_U3P_UIB_POWER_ADDRESS                            (0xe0037f04)
#define CY_U3P_UIB_POWER                                    (*(uvint32_t *)(0xe0037f04))
#define CY_U3P_UIB_POWER_DEFAULT                            (0x00000000)

/*
   For blocks that must perform initialization after reset before becoming
   operational, this signal will remain de-asserted until initialization
   is complete.  In other words reading active=1 indicates block is initialized
   and ready for operation.
 */
#define CY_U3P_UIB_ACTIVE                                   (1u << 0) /* <0:0> W:R:0:No */


/*
   Active LOW reset signal for all logic in the block.  Note that reset is
   active on all flops in the block when either system reset is asserted
   (RESET# pin or SYSTEM_POWER.RESETN is asserted) or this signal is active.
   After setting this bit to 1, firmware shall poll and wait for the ‘active’
   bit to assert.  Reading ‘1’ from ‘resetn’ does not indicate the block
   is out of reset – this may take some time depending on initialization
   tasks and clock frequencies.
 */
#define CY_U3P_UIB_RESETN                                   (1u << 31) /* <31:31> R:RW:0:No */



/*
   Descriptor Chain Pointer
 */
#define CY_U3P_UIB_SCK_DSCR_ADDRESS(n)                      (0xe0038000 + ((n) * (0x0080)))
#define CY_U3P_UIB_SCK_DSCR(n)                              (*(uvint32_t *)(0xe0038000 + ((n) * 0x0080)))
#define CY_U3P_UIB_SCK_DSCR_DEFAULT                         (0x00000000)

/*
   Descriptor number of currently active descriptor.  A value of 0xFFFF designates
   no (more) active descriptors available.  When activating a socket CPU
   shall write number of first descriptor in here. Only modify this field
   when go_suspend=1 or go_enable=0
 */
#define CY_U3P_UIB_DSCR_NUMBER_MASK                         (0x0000ffff) /* <0:15> RW:RW:X:N/A */
#define CY_U3P_UIB_DSCR_NUMBER_POS                          (0)


/*
   Number of descriptors still left to process.  This value is unrelated
   to actual number of descriptors in the list.  It is used only to generate
   an interrupt to the CPU when the value goes low or zero (or both).  When
   this value reaches 0 it will wrap around to 255.  The socket will not
   suspend or be otherwise affected unless the descriptor chains ends with
   0xFFFF descriptor number.
 */
#define CY_U3P_UIB_DSCR_COUNT_MASK                          (0x00ff0000) /* <16:23> RW:RW:X:N/A */
#define CY_U3P_UIB_DSCR_COUNT_POS                           (16)


/*
   The low watermark for dscr_count.  When dscr_count is equal or less than
   dscr_low the status bit dscr_is_low is set and an interrupt can be generated
   (depending on int mask).
 */
#define CY_U3P_UIB_DSCR_LOW_MASK                            (0xff000000) /* <24:31> R:RW:X:N/A */
#define CY_U3P_UIB_DSCR_LOW_POS                             (24)



/*
   Transfer Size Register
 */
#define CY_U3P_UIB_SCK_SIZE_ADDRESS(n)                      (0xe0038004 + ((n) * (0x0080)))
#define CY_U3P_UIB_SCK_SIZE(n)                              (*(uvint32_t *)(0xe0038004 + ((n) * 0x0080)))
#define CY_U3P_UIB_SCK_SIZE_DEFAULT                         (0x00000000)

/*
   The number of bytes or buffers (depends on unit bit in SCK_STATUS) that
   are part of this transfer.  A value of 0 signals an infinite/undetermined
   transaction size.
   Valid data bytes remaining in the last buffer beyond the transfer size
   will be read by socket and passed on to the core. FW must ensure that
   no additional bytes beyond the transfer size are present in the last buffer.
 */
#define CY_U3P_UIB_TRANS_SIZE_MASK                          (0xffffffff) /* <0:31> R:RW:X:N/A */
#define CY_U3P_UIB_TRANS_SIZE_POS                           (0)



/*
   Transfer Count Register
 */
#define CY_U3P_UIB_SCK_COUNT_ADDRESS(n)                     (0xe0038008 + ((n) * (0x0080)))
#define CY_U3P_UIB_SCK_COUNT(n)                             (*(uvint32_t *)(0xe0038008 + ((n) * 0x0080)))
#define CY_U3P_UIB_SCK_COUNT_DEFAULT                        (0x00000000)

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
#define CY_U3P_UIB_TRANS_COUNT_MASK                         (0xffffffff) /* <0:31> RW:RW:X:N/A */
#define CY_U3P_UIB_TRANS_COUNT_POS                          (0)



/*
   Socket Status Register
 */
#define CY_U3P_UIB_SCK_STATUS_ADDRESS(n)                    (0xe003800c + ((n) * (0x0080)))
#define CY_U3P_UIB_SCK_STATUS(n)                            (*(uvint32_t *)(0xe003800c + ((n) * 0x0080)))
#define CY_U3P_UIB_SCK_STATUS_DEFAULT                       (0x04e00000)

/*
   Number of available (free for ingress, occupied for egress) descriptors
   beyond the current one.  This number is incremented by the adapter whenever
   an event is received on this socket and decremented whenever it activates
   a new descriptor. This value is used to create a signal to the IP Cores
   that indicates at least one buffer is available beyond the current one
   (sck_more_buf_avl).
 */
#define CY_U3P_UIB_AVL_COUNT_MASK                           (0x0000001f) /* <0:4> RW:RW:0:N/A */
#define CY_U3P_UIB_AVL_COUNT_POS                            (0)


/*
   Minimum number of available buffers required by the adapter before activating
   a new one.  This can be used to guarantee a minimum number of buffers
   available with old data to implement rollback.  If AVL_ENABLE, the socket
   will remain in STALL state until AVL_COUNT>=AVL_MIN.
 */
#define CY_U3P_UIB_AVL_MIN_MASK                             (0x000003e0) /* <5:9> R:RW:0:N/A */
#define CY_U3P_UIB_AVL_MIN_POS                              (5)


/*
   Enables the functioning of AVL_COUNT and AVL_MIN. When 0, it will disable
   both stalling on AVL_MIN and generation of the sck_more_buf_avl signal
   described above.
 */
#define CY_U3P_UIB_AVL_ENABLE                               (1u << 10) /* <10:10> R:RW:0:N/A */


/*
   Internal operating state of the socket.  This field is used for debugging
   and to safely modify active sockets (see go_suspend).
 */
#define CY_U3P_UIB_STATE_MASK                               (0x00038000) /* <15:17> RW:R:0:N/A */
#define CY_U3P_UIB_STATE_POS                                (15)

/*
   Descriptor state. This is the default initial state indicating the descriptor
   registers are NOT valid in the Adapter. The Adapter will start loading
   the descriptor from memory if the socket becomes enabled and not suspended.
   Suspend has no effect on any other state.
 */
#define CY_U3P_UIB_STATE_DESCR                              (0)
/*
   Stall state. Socket is stalled waiting for data to be loaded into the
   Fetch Queue or waiting for an event.
 */
#define CY_U3P_UIB_STATE_STALL                              (1)
/*
   Active state. Socket is available for core data transfers.
 */
#define CY_U3P_UIB_STATE_ACTIVE                             (2)
/*
   Event state. Core transfer is done. Descriptor is being written back to
   memory and an event is being generated if enabled.
 */
#define CY_U3P_UIB_STATE_EVENT                              (3)
/*
   Check states. An active socket gets here based on the core’s EOP request
   to check the Transfer size and determine whether the buffer should be
   wrapped up. Depending on result, socket will either go back to Active
   state or move to the Event state.
 */
#define CY_U3P_UIB_STATE_CHECK1                             (4)
/*
   Socket is suspended
 */
#define CY_U3P_UIB_STATE_SUSPENDED                          (5)
/*
   Check states. An active socket gets here based on the core’s EOP request
   to check the Transfer size and determine whether the buffer should be
   wrapped up. Depending on result, socket will either go back to Active
   state or move to the Event state.
 */
#define CY_U3P_UIB_STATE_CHECK2                             (6)
/*
   Waiting for confirmation that event was sent.
 */
#define CY_U3P_UIB_STATE_WAITING                            (7)

/*
   Indicates the socket received a ZLP
 */
#define CY_U3P_UIB_ZLP_RCVD                                 (1u << 18) /* <18:18> RW:R:0:N/A */


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
#define CY_U3P_UIB_SUSPENDED                                (1u << 19) /* <19:19> RW:R:0:N/A */


/*
   Indicates the socket is currently enabled when asserted.  After go_enable
   is changed, it may take some time for enabled to make the same change.
    This value can be polled to determine this fact.
 */
#define CY_U3P_UIB_ENABLED                                  (1u << 20) /* <20:20> RW:R:0:N/A */


/*
   Enable (1) or disable (0) truncating of BYTE_COUNT when TRANS_COUNT+BYTE_COUNT>=TRANS_SIZE.
    When enabled, ensures that an ingress transfer never contains more bytes
   then allowed.  This function is needed to implement burst-based prototocols
   that can only transmit full bursts of more than 1 byte.
 */
#define CY_U3P_UIB_TRUNCATE                                 (1u << 21) /* <21:21> R:RW:1:N/A */


/*
   Enable (1) or disable (0) sending of produce events from any descriptor
   in this socket.  If 0, events will be suppressed, and the descriptor will
   not be copied back into memory when completed.
 */
#define CY_U3P_UIB_EN_PROD_EVENTS                           (1u << 22) /* <22:22> R:RW:1:N/A */


/*
   Enable (1) or disable (0) sending of consume events from any descriptor
   in this socket.  If 0, events will be suppressed, and the descriptor will
   not be copied back into memory when completed.
 */
#define CY_U3P_UIB_EN_CONS_EVENTS                           (1u << 23) /* <23:23> R:RW:1:N/A */


/*
   When set, the socket will suspend before activating a descriptor with
   BYTE_COUNT<BUFFER_SIZE.
   This is relevant for egress sockets only.
 */
#define CY_U3P_UIB_SUSP_PARTIAL                             (1u << 24) /* <24:24> R:RW:0:N/A */


/*
   When set, the socket will suspend before activating a descriptor with
   TRANS_COUNT+BUFFER_SIZE>=TRANS_SIZE.  This is relevant for both ingress
   and egress sockets.
 */
#define CY_U3P_UIB_SUSP_LAST                                (1u << 25) /* <25:25> R:RW:0:N/A */


/*
   When set, the socket will suspend when trans_count >= trans_size.  This
   equation is checked (and hence the socket will suspend) only at the boundary
   of buffers and packets (ie. buffer wrapup or EOP assertion).
 */
#define CY_U3P_UIB_SUSP_TRANS                               (1u << 26) /* <26:26> R:RW:1:N/A */


/*
   When set, the socket will suspend after wrapping up the first buffer with
   dscr.eop=1.  Note that this function will work the same for both ingress
   and egress sockets.
 */
#define CY_U3P_UIB_SUSP_EOP                                 (1u << 27) /* <27:27> R:RW:0:N/A */


/*
   Setting this bit will forcibly wrap-up a socket, whether it is out of
   data or not.  This option is intended mainly for ingress sockets, but
   works also for egress sockets.  Any remaining data in fetch buffers is
   ignored, in write buffers is flushed.  Transaction and buffer counts are
   updated normally, and suspend behavior also happens normally (depending
   on various other settings in this register).G45
 */
#define CY_U3P_UIB_WRAPUP                                   (1u << 28) /* <28:28> RW0C:RW1S:0:N/A */


/*
   Indicates whether descriptors (1) or bytes (0) are counted by trans_count
   and trans_size.  Descriptors are counting regardless of whether they contain
   any data or have eop set.
 */
#define CY_U3P_UIB_UNIT                                     (1u << 29) /* <29:29> R:RW:0:N/A */


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
#define CY_U3P_UIB_GO_SUSPEND                               (1u << 30) /* <30:30> R:RW:0:N/A */


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
#define CY_U3P_UIB_GO_ENABLE                                (1u << 31) /* <31:31> R:RW:0:N/A */



/*
   Socket Interrupt Request Register
 */
#define CY_U3P_UIB_SCK_INTR_ADDRESS(n)                      (0xe0038010 + ((n) * (0x0080)))
#define CY_U3P_UIB_SCK_INTR(n)                              (*(uvint32_t *)(0xe0038010 + ((n) * 0x0080)))
#define CY_U3P_UIB_SCK_INTR_DEFAULT                         (0x00000000)

/*
   Indicates that a produce event has been received or transmitted since
   last cleared.
 */
#define CY_U3P_UIB_PRODUCE_EVENT                            (1u << 0) /* <0:0> W1S:RW1C:0:N/A */


/*
   Indicates that a consume event has been received or transmitted since
   last cleared.
 */
#define CY_U3P_UIB_CONSUME_EVENT                            (1u << 1) /* <1:1> W1S:RW1C:0:N/A */


/*
   Indicates that dscr_count has fallen below its watermark dscr_low.  If
   dscr_count wraps around to 255 dscr_is_low will remain asserted until
   cleared by s/w
 */
#define CY_U3P_UIB_DSCR_IS_LOW                              (1u << 2) /* <2:2> W1S:RW1C:0:N/A */


/*
   Indicates the no descriptor is available.  Not available means that the
   current descriptor number is 0xFFFF.  Note that this bit will remain asserted
   until cleared by s/w, regardless of whether a new descriptor number is
   loaded.
 */
#define CY_U3P_UIB_DSCR_NOT_AVL                             (1u << 3) /* <3:3> W1S:RW1C:0:N/A */


/*
   Indicates the socket has stalled, waiting for an event signaling its descriptor
   has become available. Note that this bit will remain asserted until cleared
   by s/w, regardless of whether the socket resumes.
 */
#define CY_U3P_UIB_STALL                                    (1u << 4) /* <4:4> W1S:RW1C:0:N/A */


/*
   Indicates the socket has gone into suspend mode.  This may be caused by
   any hardware initiated condition (e.g. DSCR_NOT_AVL, any of the SUSP_*)
   or by setting GO_SUSPEND=1.  Note that this bit will remain asserted until
   cleared by s/w, regardless of whether the suspend condition is resolved.
   Note that the socket resumes only when SCK_INTR[9:5]=0 and GO_SUSPEND=0.
 */
#define CY_U3P_UIB_SUSPEND                                  (1u << 5) /* <5:5> W1S:RW1C:0:N/A */


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
#define CY_U3P_UIB_ERROR                                    (1u << 6) /* <6:6> W1S:RW1C:0:N/A */


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
#define CY_U3P_UIB_TRANS_DONE                               (1u << 7) /* <7:7> W1S:RW1C:0:N/A */


/*
   Indicates that the (egress) socket was suspended because of SUSP_PARTIAL
   condition.  Note that the socket resumes only when SCK_INTR[9:5]=0 and
   GO_SUSPEND=0.
 */
#define CY_U3P_UIB_PARTIAL_BUF                              (1u << 8) /* <8:8> W1S:RW1C:0:N/A */


/*
   Indicates that the socket was suspended because of SUSP_LAST condition.
    Note that the socket resumes only when SCK_INTR[9:5]=0 and GO_SUSPEND=0.
 */
#define CY_U3P_UIB_LAST_BUF                                 (1u << 9) /* <9:9> W1S:RW1C:0:N/A */



/*
   Socket Interrupt Mask Register
 */
#define CY_U3P_UIB_SCK_INTR_MASK_ADDRESS(n)                 (0xe0038014 + ((n) * (0x0080)))
#define CY_U3P_UIB_SCK_INTR_MASK(n)                         (*(uvint32_t *)(0xe0038014 + ((n) * 0x0080)))
#define CY_U3P_UIB_SCK_INTR_MASK_DEFAULT                    (0x00000000)

/*
   1: Report interrupt to CPU
 */
#define CY_U3P_UIB_PRODUCE_EVENT                            (1u << 0) /* <0:0> R:RW:0:N/A */


/*
   1: Report interrupt to CPU
 */
#define CY_U3P_UIB_CONSUME_EVENT                            (1u << 1) /* <1:1> R:RW:0:N/A */


/*
   1: Report interrupt to CPU
 */
#define CY_U3P_UIB_DSCR_IS_LOW                              (1u << 2) /* <2:2> R:RW:0:N/A */


/*
   1: Report interrupt to CPU
 */
#define CY_U3P_UIB_DSCR_NOT_AVL                             (1u << 3) /* <3:3> R:RW:0:N/A */


/*
   1: Report interrupt to CPU
 */
#define CY_U3P_UIB_STALL                                    (1u << 4) /* <4:4> R:RW:0:N/A */


/*
   1: Report interrupt to CPU
 */
#define CY_U3P_UIB_SUSPEND                                  (1u << 5) /* <5:5> R:RW:0:N/A */


/*
   1: Report interrupt to CPU
 */
#define CY_U3P_UIB_ERROR                                    (1u << 6) /* <6:6> R:RW:0:N/A */


/*
   1: Report interrupt to CPU
 */
#define CY_U3P_UIB_TRANS_DONE                               (1u << 7) /* <7:7> R:RW:0:N/A */


/*
   1: Report interrupt to CPU
 */
#define CY_U3P_UIB_PARTIAL_BUF                              (1u << 8) /* <8:8> R:RW:0:N/A */


/*
   1: Report interrupt to CPU
 */
#define CY_U3P_UIB_LAST_BUF                                 (1u << 9) /* <9:9> R:RW:0:N/A */



/*
   Descriptor buffer base address register
 */
#define CY_U3P_UIB_DSCR_BUFFER_ADDRESS(n)                   (0xe0038020 + ((n) * (0x0080)))
#define CY_U3P_UIB_DSCR_BUFFER(n)                           (*(uvint32_t *)(0xe0038020 + ((n) * 0x0080)))
#define CY_U3P_UIB_DSCR_BUFFER_DEFAULT                      (0x00000000)

/*
   The base address of the buffer where data is written.  This address is
   not necessarily word-aligned to allow for header/trailer/length modification.
 */
#define CY_U3P_UIB_BUFFER_ADDR_MASK                         (0xffffffff) /* <0:31> RW:RW:X:N/A */
#define CY_U3P_UIB_BUFFER_ADDR_POS                          (0)



/*
   Descriptor synchronization pointers register
 */
#define CY_U3P_UIB_DSCR_SYNC_ADDRESS(n)                     (0xe0038024 + ((n) * (0x0080)))
#define CY_U3P_UIB_DSCR_SYNC(n)                             (*(uvint32_t *)(0xe0038024 + ((n) * 0x0080)))
#define CY_U3P_UIB_DSCR_SYNC_DEFAULT                        (0x00000000)

/*
   The socket number of the consuming socket to which the produce event shall
   be sent.
   If cons_ip and cons_sck matches the socket's IP and socket number then
   the matching socket becomes consuming socket.
 */
#define CY_U3P_UIB_CONS_SCK_MASK                            (0x000000ff) /* <0:7> RW:RW:X:N/A */
#define CY_U3P_UIB_CONS_SCK_POS                             (0)


/*
   The IP number of the consuming socket to which the produce event shall
   be sent.  Use 0x3F to designate ARM CPU (so software) as consumer; the
   event will be lost in this case and an interrupt should also be generated
   to observe this condition.
 */
#define CY_U3P_UIB_CONS_IP_MASK                             (0x00003f00) /* <8:13> RW:RW:X:N/A */
#define CY_U3P_UIB_CONS_IP_POS                              (8)


/*
   Enable sending of a consume events from this descriptor only.  Events
   are sent only if SCK_STATUS.en_consume_ev=1.  When events are disabled,
   the adapter also does not update the descriptor in memory to clear its
   occupied bit.
 */
#define CY_U3P_UIB_EN_CONS_EVENT                            (1u << 14) /* <14:14> RW:RW:X:N/A */


/*
   Enable generation of a consume event interrupt for this descriptor only.
    This interrupt will only be seen by the CPU if SCK_STATUS.int_mask has
   this interrupt enabled as well.  Note that this flag influences the logging
   of the interrupt in SCK_STATUS; it has no effect on the reporting of the
   interrupt to the CPU like SCK_STATUS.int_mask does.
 */
#define CY_U3P_UIB_EN_CONS_INT                              (1u << 15) /* <15:15> RW:RW:X:N/A */


/*
   The socket number of the producing socket to which the consume event shall
   be sent. If prod_ip and prod_sck matches the socket's IP and socket number
   then the matching socket becomes consuming socket.
 */
#define CY_U3P_UIB_PROD_SCK_MASK                            (0x00ff0000) /* <16:23> RW:RW:X:N/A */
#define CY_U3P_UIB_PROD_SCK_POS                             (16)


/*
   The IP number of the producing socket to which the consume event shall
   be sent. Use 0x3F to designate ARM CPU (so software) as producer; the
   event will be lost in this case and an interrupt should also be generated
   to observe this condition.
 */
#define CY_U3P_UIB_PROD_IP_MASK                             (0x3f000000) /* <24:29> RW:RW:X:N/A */
#define CY_U3P_UIB_PROD_IP_POS                              (24)


/*
   Enable sending of a produce events from this descriptor only.  Events
   are sent only if SCK_STATUS.en_produce_ev=1.  If 0, events will be suppressed,
   and the descriptor will not be copied back into memory when completed.
 */
#define CY_U3P_UIB_EN_PROD_EVENT                            (1u << 30) /* <30:30> RW:RW:X:N/A */


/*
   Enable generation of a produce event interrupt for this descriptor only.
   This interrupt will only be seen by the CPU if SCK_STATUS. int_mask has
   this interrupt enabled as well.  Note that this flag influences the logging
   of the interrupt in SCK_STATUS; it has no effect on the reporting of the
   interrupt to the CPU like SCK_STATUS.int_mask does.
 */
#define CY_U3P_UIB_EN_PROD_INT                              (1u << 31) /* <31:31> RW:RW:X:N/A */



/*
   Descriptor Chain Pointers Register
 */
#define CY_U3P_UIB_DSCR_CHAIN_ADDRESS(n)                    (0xe0038028 + ((n) * (0x0080)))
#define CY_U3P_UIB_DSCR_CHAIN(n)                            (*(uvint32_t *)(0xe0038028 + ((n) * 0x0080)))
#define CY_U3P_UIB_DSCR_CHAIN_DEFAULT                       (0x00000000)

/*
   Descriptor number of the next task for consumer. A value of 0xFFFF signals
   end of this list.
 */
#define CY_U3P_UIB_RD_NEXT_DSCR_MASK                        (0x0000ffff) /* <0:15> RW:RW:X:N/A */
#define CY_U3P_UIB_RD_NEXT_DSCR_POS                         (0)


/*
   Descriptor number of the next task for producer. A value of 0xFFFF signals
   end of this list.
 */
#define CY_U3P_UIB_WR_NEXT_DSCR_MASK                        (0xffff0000) /* <16:31> RW:RW:X:N/A */
#define CY_U3P_UIB_WR_NEXT_DSCR_POS                         (16)



/*
   Descriptor Size Register
 */
#define CY_U3P_UIB_DSCR_SIZE_ADDRESS(n)                     (0xe003802c + ((n) * (0x0080)))
#define CY_U3P_UIB_DSCR_SIZE(n)                             (*(uvint32_t *)(0xe003802c + ((n) * 0x0080)))
#define CY_U3P_UIB_DSCR_SIZE_DEFAULT                        (0x00000000)

/*
   A marker that is provided by s/w and can be observed by the IP.  It's
   meaning is defined by the IP that uses it.  This bit has no effect on
   the other DMA mechanisms.
 */
#define CY_U3P_UIB_MARKER                                   (1u << 0) /* <0:0> RW:RW:X:N/A */


/*
   A marker indicating this descriptor refers to the last buffer of a packet
   or transfer. Packets/transfers may span more than one buffer.  The producing
   IP provides this marker by providing the EOP signal to its DMA adapter.
    The consuming IP observes this marker by inspecting its EOP return signal
   from its own DMA adapter. For more information see section on packets,
   buffers and transfers in DMA chapter.
 */
#define CY_U3P_UIB_EOP                                      (1u << 1) /* <1:1> RW:RW:X:N/A */


/*
   Indicates the buffer data is valid (0) or in error (1).
 */
#define CY_U3P_UIB_BUFFER_ERROR                             (1u << 2) /* <2:2> RW:RW:X:N/A */


/*
   Indicates the buffer is in use (1) or empty (0).  A consumer will interpret
   this as:
   0: Buffer is empty, wait until filled.
   1: Buffer has data that can be consumed
   A produce will interpret this as:
   0: Buffer is ready to be filled
   1: Buffer is occupied, wait until empty
 */
#define CY_U3P_UIB_BUFFER_OCCUPIED                          (1u << 3) /* <3:3> RW:RW:X:N/A */


/*
   The size of the buffer in multiples of 16 bytes
 */
#define CY_U3P_UIB_BUFFER_SIZE_MASK                         (0x0000fff0) /* <4:15> RW:RW:X:N/A */
#define CY_U3P_UIB_BUFFER_SIZE_POS                          (4)


/*
   The number of data bytes present in the buffer.  An occupied buffer is
   not always full, in particular when variable length packets are transferred.
 */
#define CY_U3P_UIB_BYTE_COUNT_MASK                          (0xffff0000) /* <16:31> RW:RW:X:N/A */
#define CY_U3P_UIB_BYTE_COUNT_POS                           (16)



/*
   Event Communication Register
 */
#define CY_U3P_UIB_EVENT_ADDRESS(n)                         (0xe003807c + ((n) * (0x0080)))
#define CY_U3P_UIB_EVENT(n)                                 (*(uvint32_t *)(0xe003807c + ((n) * 0x0080)))
#define CY_U3P_UIB_EVENT_DEFAULT                            (0x00000000)

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
#define CY_U3P_UIB_SCK_INTR0_ADDRESS                        (0xe003ff00)
#define CY_U3P_UIB_SCK_INTR0                                (*(uvint32_t *)(0xe003ff00))
#define CY_U3P_UIB_SCK_INTR0_DEFAULT                        (0x00000000)

/*
   Socket <x> asserts interrupt when bit <x> is set in this vector.  Multiple
   bits may be set to 1 simultaneously.
   This register is only as wide as the number of socket in the adapter;
   256 is just the maximum width.  All other bits always return 0.
 */
#define CY_U3P_UIB_SCKINTR_L_MASK                           (0xffffffff) /* <0:31> W:R:0:N/A */
#define CY_U3P_UIB_SCKINTR_L_POS                            (0)



/*
   Socket Interrupt Request Register
 */
#define CY_U3P_UIB_SCK_INTR1_ADDRESS                        (0xe003ff04)
#define CY_U3P_UIB_SCK_INTR1                                (*(uvint32_t *)(0xe003ff04))
#define CY_U3P_UIB_SCK_INTR1_DEFAULT                        (0x00000000)

/*
   Socket <x> asserts interrupt when bit <x> is set in this vector.  Multiple
   bits may be set to 1 simultaneously.
   This register is only as wide as the number of socket in the adapter;
   256 is just the maximum width.  All other bits always return 0.
 */
#define CY_U3P_UIB_SCKINTR_H_L_MASK                         (0xffffffff) /* <0:31> W:R:0:N/A */
#define CY_U3P_UIB_SCKINTR_H_L_POS                          (0)



/*
   Socket Interrupt Request Register
 */
#define CY_U3P_UIB_SCK_INTR2_ADDRESS                        (0xe003ff08)
#define CY_U3P_UIB_SCK_INTR2                                (*(uvint32_t *)(0xe003ff08))
#define CY_U3P_UIB_SCK_INTR2_DEFAULT                        (0x00000000)

/*
   Socket <x> asserts interrupt when bit <x> is set in this vector.  Multiple
   bits may be set to 1 simultaneously.
   This register is only as wide as the number of socket in the adapter;
   256 is just the maximum width.  All other bits always return 0.
 */
#define CY_U3P_UIB_SCKINTR_H_L_MASK                         (0xffffffff) /* <0:31> W:R:0:N/A */
#define CY_U3P_UIB_SCKINTR_H_L_POS                          (0)



/*
   Socket Interrupt Request Register
 */
#define CY_U3P_UIB_SCK_INTR3_ADDRESS                        (0xe003ff0c)
#define CY_U3P_UIB_SCK_INTR3                                (*(uvint32_t *)(0xe003ff0c))
#define CY_U3P_UIB_SCK_INTR3_DEFAULT                        (0x00000000)

/*
   Socket <x> asserts interrupt when bit <x> is set in this vector.  Multiple
   bits may be set to 1 simultaneously.
   This register is only as wide as the number of socket in the adapter;
   256 is just the maximum width.  All other bits always return 0.
 */
#define CY_U3P_UIB_SCKINTR_H_L_MASK                         (0xffffffff) /* <0:31> W:R:0:N/A */
#define CY_U3P_UIB_SCKINTR_H_L_POS                          (0)



/*
   Socket Interrupt Request Register
 */
#define CY_U3P_UIB_SCK_INTR4_ADDRESS                        (0xe003ff10)
#define CY_U3P_UIB_SCK_INTR4                                (*(uvint32_t *)(0xe003ff10))
#define CY_U3P_UIB_SCK_INTR4_DEFAULT                        (0x00000000)

/*
   Socket <x> asserts interrupt when bit <x> is set in this vector.  Multiple
   bits may be set to 1 simultaneously.
   This register is only as wide as the number of socket in the adapter;
   256 is just the maximum width.  All other bits always return 0.
 */
#define CY_U3P_UIB_SCKINTR_H_L_MASK                         (0xffffffff) /* <0:31> W:R:0:N/A */
#define CY_U3P_UIB_SCKINTR_H_L_POS                          (0)



/*
   Socket Interrupt Request Register
 */
#define CY_U3P_UIB_SCK_INTR5_ADDRESS                        (0xe003ff14)
#define CY_U3P_UIB_SCK_INTR5                                (*(uvint32_t *)(0xe003ff14))
#define CY_U3P_UIB_SCK_INTR5_DEFAULT                        (0x00000000)

/*
   Socket <x> asserts interrupt when bit <x> is set in this vector.  Multiple
   bits may be set to 1 simultaneously.
   This register is only as wide as the number of socket in the adapter;
   256 is just the maximum width.  All other bits always return 0.
 */
#define CY_U3P_UIB_SCKINTR_H_L_MASK                         (0xffffffff) /* <0:31> W:R:0:N/A */
#define CY_U3P_UIB_SCKINTR_H_L_POS                          (0)



/*
   Socket Interrupt Request Register
 */
#define CY_U3P_UIB_SCK_INTR6_ADDRESS                        (0xe003ff18)
#define CY_U3P_UIB_SCK_INTR6                                (*(uvint32_t *)(0xe003ff18))
#define CY_U3P_UIB_SCK_INTR6_DEFAULT                        (0x00000000)

/*
   Socket <x> asserts interrupt when bit <x> is set in this vector.  Multiple
   bits may be set to 1 simultaneously.
   This register is only as wide as the number of socket in the adapter;
   256 is just the maximum width.  All other bits always return 0.
 */
#define CY_U3P_UIB_SCKINTR_H_L_MASK                         (0xffffffff) /* <0:31> W:R:0:N/A */
#define CY_U3P_UIB_SCKINTR_H_L_POS                          (0)



/*
   Socket Interrupt Request Register
 */
#define CY_U3P_UIB_SCK_INTR7_ADDRESS                        (0xe003ff1c)
#define CY_U3P_UIB_SCK_INTR7                                (*(uvint32_t *)(0xe003ff1c))
#define CY_U3P_UIB_SCK_INTR7_DEFAULT                        (0x00000000)

/*
   Socket <x> asserts interrupt when bit <x> is set in this vector.  Multiple
   bits may be set to 1 simultaneously.
   This register is only as wide as the number of socket in the adapter;
   256 is just the maximum width.  All other bits always return 0.
 */
#define CY_U3P_UIB_SCKINTR_H_MASK                           (0xffffffff) /* <0:31> W:R:0:N/A */
#define CY_U3P_UIB_SCKINTR_H_POS                            (0)



#endif /* _INCLUDED_UIB_REGS_H_ */

/*[]*/

