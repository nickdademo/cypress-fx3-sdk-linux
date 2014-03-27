/****************************************************************************
 *
 * File: gctlaon_regs.h
 *
 * Copyright (c) 2010-13 Cypress Semiconductor. All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION WHICH IS THE PROPERTY OF CYPRESS.
 *
 * Description:
 *   Global Control Always-On registers for the EZ-USB FX3 Device.
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

#ifndef _INCLUDED_GCTLAON_REGS_H_
#define _INCLUDED_GCTLAON_REGS_H_

#include <cyu3types.h>

#define GCTLAON_BASE_ADDR                        (0xe0050000)

typedef struct
{
    uvint32_t control;                            /* 0xe0050000 */
    uvint32_t wakeup_en;                          /* 0xe0050004 */
    uvint32_t wakeup_polarity;                    /* 0xe0050008 */
    uvint32_t wakeup_event;                       /* 0xe005000c */
    uvint32_t freeze;                             /* 0xe0050010 */
    uvint32_t watchdog_cs;                        /* 0xe0050014 */
    uvint32_t watchdog_timer0;                    /* 0xe0050018 */
    uvint32_t watchdog_timer1;                    /* 0xe005001c */
} GCTLAON_REGS_T, *PGCTLAON_REGS_T;

#define GCTLAON        ((PGCTLAON_REGS_T) GCTLAON_BASE_ADDR)


/*
   Main GCTL control register for power, reset, boot
 */
#define CY_U3P_GCTL_CONTROL_ADDRESS                         (0xe0050000)
#define CY_U3P_GCTL_CONTROL                                 (*(uvint32_t *)(0xe0050000))
#define CY_U3P_GCTL_CONTROL_DEFAULT                         (0xde040001)

/*
   Indicates system woke up through a power-on-reset or RESET# pin reset
   sequence. If firmware does not clear this bit it will stay 1 even through
   software reset, standby and suspend sequences.  This bit is visible in
   PP_INIT. (connected to HardHard Reset)
 */
#define CY_U3P_GCTL_POR                                     (1u << 0) /* <0:0> RW1S:RW0C:1:No */


/*
   Indicates system woke up from a s/w induced hard reset sequence (from
   HARD_RESET_N below or PP_INIT.HARD_RESET_N).  If firmware does not clear
   this bit it will stay 1 even through standby and suspend sequences. This
   bit is visible in PP_INIT. (connected to HardHard Reset)
 */
#define CY_U3P_GCTL_SW_RESET                                (1u << 1) /* <1:1> RW1S:RW0C:0:No */


/*
   Indicates system woke up from a watchdog timer induced hard reset (see
   GCTL_WATCHDOG_CS).  If firmware does not clear this bit it will stay 1
   even through standby and suspend sequences. This bit is visible in PP_INIT.
    (connected to HardHard Reset)
 */
#define CY_U3P_GCTL_WDT_RESET                               (1u << 2) /* <2:2> RW1S:RW0C:0:No */


/*
   Indicates system woke up from standby mode (see architecture spec for
   details). If firmware does not clear this bit it will stay 1 even through
   suspend sequences. This bit is visible in PP_INIT. (connected to HardHard
   Reset)
 */
#define CY_U3P_GCTL_WAKEUP_PWR                              (1u << 3) /* <3:3> RW1S:RW0C:0:No */


/*
   Indicates system woke up from suspend state (see architecture spec for
   details). If firmware does not clear this bit it will stay 1 even through
   standby sequences. This bit is visible in PP_INIT. (connected to HardHard
   Reset)
 */
#define CY_U3P_GCTL_WAKEUP_CLK                              (1u << 4) /* <4:4> RW1S:RW0C:0:No */


/*
   This bit is used by the BootROM code to indicate that it has completed.
    BootROM sets this bit just before it jumps into the firmware start address.
    This bit can be used by a host processor to observe a completed boot
   process of a corrupt firmware image. (Connected to Partial Reset)
 */
#define CY_U3P_GCTL_BOOT_COMPLETE                           (1u << 8) /* <8:8> R:RW:0:N/A */


/*
   Indicates that the CPU is in debug mode.  This bit is tied to a ARM926
   CPU output signal and is read-only for software.  (Connected to CPU Reset)
 */
#define CY_U3P_GCTL_DEBUG_MODE                              (1u << 9) /* <9:9> RW:R:0:N/A */


/*
   Indicates that system RAM arrays are powered off when main power gate
   is switched off (standby mode) through MAIN_POWER_EN.  Default is that
   arrays remain powered up and retain information.  This bit is reset on
   pin, POR, or watchdog reset only. (Connected to HardHard Reset)
 */
#define CY_U3P_GCTL_RAM_SLEEP                               (1u << 10) /* <10:10> R:RW:0:N/A */


/*
   When 1 any interrupt cause bit set in GCTL_WAKEUP_EV will force INT# pin
   to low.  This is intended to enable forwarding of wakeup events to the
   application processor when reference clock is disabled (so AP can wakeup
   and turn it on).  When using this option, it is important to clear the
   wakeup events in order to prevent INT# to remain asserted.  This bit is
   reset on pin, POR, or watchdog reset only.  (Connected to HardHard Reset)
 */
#define CY_U3P_GCTL_WAKEUP_AP_INT                           (1u << 11) /* <11:11> R:RW:0:N/A */


/*
   Enables the wakeup interrupt to the CPU. This bit is reset on pin, POR,
   or watchdog reset only.  (Connected to HardHard Reset)
 */
#define CY_U3P_GCTL_WAKEUP_CPU_INT                          (1u << 12) /* <12:12> R:RW:0:No */


/*
   Enables BIST controller for SysMem.  This removes SysMem from system bus;
   CPU execution can only be fom ITCM/DTCM. (Connected to Partial Reset)
 */
#define CY_U3P_GCTL_SYSMEM_BIST_EN                          (1u << 14) /* <14:14> R:RW:0:No */


/*
   This bit is reset on pin, POR, or watchdog reset only.
   0: Wait for CPU to go to SBYWFI state before executing MAIN_POWER_EN=0/MAIN_CLOCK_EN=0
   1: Do not wait for CPU to go to SBYWFI state, execute immediately.
   (Connected to HardHard Reset)
 */
#define CY_U3P_GCTL_NO_SBYWFI                               (1u << 15) /* <15:15> R:RW:0:No */


/*
   Prohibits writing to GCTL_WATCHDOG registers when not equal 0.
   Requires at least two different writes to unlock.
   Writing to this field has the following effect:
   0: No effect
   1: Clears bit 0
   2: Clears bit 1
   3: Sets both bits 0 and 1
   Note that this field is 2 bits to force multiple writes only.  It represents
   only a single write protect signal protecting all WATCHDOG registers at
   the same time. (Connected to HardHard Reset)
 */
#define CY_U3P_GCTL_WDT_PROTECT_MASK                        (0x00030000) /* <16:17> R:RW:0:Yes */
#define CY_U3P_GCTL_WDT_PROTECT_POS                         (16)


/*
   USB D+/D- analog switch
   0: USB Mode. Connects D+/D- to USB PHY
   1:  Bypass mode. Connects D+/D- to SWD+/SWD- pads (analog)
   This switch takes precedence over CARKIT switch. (Connected to HardHard
   Reset).
 */
#define CY_U3P_GCTL_ANALOG_SWITCH                           (1u << 18) /* <18:18> R:RW:1:Yes */


/*
   Enables the main regulators in USB IO Subsystem.  These regulators supply
   the 2.5/3.3V to the USB2 and USB3 PHY's.  Power to the USB HS Switch is
   separate and is always supplied when VBUS/VBAT is present.  (Connected
   to Partial Reset).
 */
#define CY_U3P_GCTL_USB_POWER_EN                            (1u << 21) /* <21:21> R:RW:0:Yes */


/*
   Enables the use of VBAT in the USB IO Subsystem when both VBUS and VBAT
   supplies are present. Once set, this bit should not be cleared without
   taking
   special precautions. See the PAS for details.
   0: Use only VBUS input voltage for USB PHY/Switch regulators when both
   VBUS and VBAT supplies are present
   1: Use VBAT input voltage when present VBUS and VBAT supplies are present.
   (Connected to Partial Reset).
 */
#define CY_U3P_GCTL_USB_VBAT_EN                             (1u << 22) /* <22:22> R:RW:0:No */


/*
   Forces all IOs with hold capability (as specified in Architecture Spec)
   to sample and hold their current state (drive strength, pull up/downs,
   in/out state, drive high/low) or be forced to a given state as specified
   in GCTL_FREEZE.  This bit must be set together with MAIN_POWER_EN and
   may be set together with MAIN_CLOCK_EN).  It must be cleared after wakeup
   after IO configuration registers and port functions have been properly
   re-initialized.  This bit is reset on pin, POR, or watchdog reset only.
    (Connected to Hard Reset).
 */
#define CY_U3P_GCTL_FREEZE_IO                               (1u << 24) /* <24:24> R:RW:0:No */


/*
   Software clears this bit to turn off all clock PLLs.  Only the external
   32kHz wakeup clock is still available.  All domains remain powered and
   retain state.  System will wakeup from any enabled wakeup source (enabled
   in GCTL_WAKEUP_EN) and set WAKEUP_CLK bit.  If a write to 0 persists at
   1, then there are wakeup events that first need to be handled and cleared
   in the WAKEUP_EVENT register.  (Connected to HardHard Reset).
 */
#define CY_U3P_GCTL_MAIN_CLOCK_EN                           (1u << 25) /* <25:25> RW1S:RW0C:1:Yes */


/*
   Software clears this bit to turn off main power gates.  Only wakup and
   system memory domains (see RAM_SLEEP) will remain powered.  System will
   wakeup from any enabled wakeup source (enabled in GCTL_WAKEUP_EN) and
   set WAKEUP_PWR bit. If a write to 0 persists at '1', then there are wakeup
   events that first need to be handled and cleared in the WAKEUP_EVENT register.
    (Connected to HardHard Reset).
 */
#define CY_U3P_GCTL_MAIN_POWER_EN                           (1u << 26) /* <26:26> RW1S:RW0C:1:Yes */


/*
   Software may clear this bit to disable boot ROM access by the CPU (or
   any other peripheral).  Once cleared this bit can not be set back to 1.
   This bit is set by hardware after a CPU or a chip reset; it is not set
   after a wakeup from suspend.  (Connected to CPU Reset).
 */
#define CY_U3P_GCTL_BOOTROM_EN                              (1u << 28) /* <28:28> R:RW0C:1:Yes */


/*
   Software sets this bit to indicate to boot ROM that no new firmware shall
   be loaded.  Exact warm boot process is detailed in the boot ROM specification.
    This bit is not reset when going through a CPU_RESET or HARD_RESET. 
   It is reset only when going through a pin, POR or WATCHDOG reset.  (Connected
   to Hard Reset)
 */
#define CY_U3P_GCTL_WARM_BOOT                               (1u << 29) /* <29:29> -:RW:0:N/A */


/*
   Firmware clears this bit to effect a CPU reset (aka reboot). This bit
   will automatically re-assert as part of the reset sequence. No other blocks
   or registers expect cpu_top are affected.  The CPU will enter the boot
   ROM, that will use the WARM_BOOT flag to determine whether to reload firmware.
   This function is also available in PP_INIT.  (Connected to Partial Reset).
 */
#define CY_U3P_GCTL_CPU_RESET_N                             (1u << 30) /* <30:30> RW1S:RW0C:1:No */


/*
   Firmware clears this bit to effect a global hard reset (all blocks, all
   flops).  This is equivalent to toggling the RESET pin on the device, except
   that WARM_BOOT (above) will not be cleared). This function is also available
   in PP_INIT.  (Connected to Partial Reset).
 */
#define CY_U3P_GCTL_HARD_RESET_N                            (1u << 31) /* <31:31> R:RW0C:1:No */



/*
   Wakeup enable register
 */
#define CY_U3P_GCTL_WAKEUP_EN_ADDRESS                       (0xe0050004)
#define CY_U3P_GCTL_WAKEUP_EN                               (*(uvint32_t *)(0xe0050004))
#define CY_U3P_GCTL_WAKEUP_EN_DEFAULT                       (0x00000000)

/*
   Enables wakeup from the PIB CTL0 pin (CE# typically).  Wakeup occurs on
   any change on this pin after wakeup.
 */
#define CY_U3P_GCTL_EN_PIB_CTRL0                            (1u << 0) /* <0:0> R:RW:0:Yes */


/*
   Enable wakeup from a CMD5 on the PIB_CMD pin.  This wakeup source does
   not work from standby mode.
 */
#define CY_U3P_GCTL_EN_PIB_CMD                              (1u << 1) /* <1:1> R:RW:0:Yes */


/*
   Enables wakeup from the PIB_CLK pin.  Wakeup occurs on any change on this
   pin after wakeup.
 */
#define CY_U3P_GCTL_EN_PIB_CLK                              (1u << 2) /* <2:2> R:RW:0:Yes */


/*
   Enables wakeup from the S0 SDIO_INT pin (SD0).  Wakeup occurs when the
   level set in POL_S0_SDIO_INT is detected.
 */
#define CY_U3P_GCTL_EN_S0_SDIO_INT                          (1u << 3) /* <3:3> R:RW:0:Yes */


/*
   Enables wakeup from the S1 SDIO_INT pin (SD1).  Wakeup occurs when the
   level set in POL_S1_SDIO_INT is detected.
 */
#define CY_U3P_GCTL_EN_S1_SDIO_INT                          (1u << 4) /* <4:4> R:RW:0:Yes */


/*
   Enables wakeup from the S0S1_INS pin (card insertion).  Wakeup occurs
   when the level set in POL_S0S1_INS_INT is detected.
 */
#define CY_U3P_GCTL_EN_S0S1_INS                             (1u << 5) /* <5:5> R:RW:0:Yes */


/*
   Enables wakeup from the UART_CTS pin.  Wakeup occurs when the level set
   in POL_UART_CTS is detected.
 */
#define CY_U3P_GCTL_EN_UART_CTS                             (1u << 6) /* <6:6> R:RW:0:Yes */


/*
   Enables wakeup when USB2 D+ line transitions to from 1 to 0 (USB RESUME)
   or from 0 to 1 (CONNECT in host mode).
   This wakeup source does not work from standby mode.
 */
#define CY_U3P_GCTL_EN_UIB_DP                               (1u << 7) /* <7:7> R:RW:0:Yes */


/*
   Enables wakeup when USB2 D+ line transitions to from 0 to 1 or from 1
   to 0 (CONNECT in host mode).
   This wakeup source does not work from standby mode.
 */
#define CY_U3P_GCTL_EN_UIB_DM                               (1u << 8) /* <8:8> R:RW:0:Yes */


/*
   Enables wakeup when an impedance change is detected on the USB2 OTGID
   pin (USB ATTACH event).
   This wakeup source does not work from standby mode.
 */
#define CY_U3P_GCTL_EN_UIB_OTGID                            (1u << 9) /* <9:9> R:RW:0:Yes */


/*
   Enables wakeup when an LFPS signal appears on the USB3 SSRX pins.
   This wakeup source does not work from standby mode.
 */
#define CY_U3P_GCTL_EN_UIB_SSRX                             (1u << 10) /* <10:10> R:RW:0:Yes */


/*
   Enables wakeup when VBUS is asserted.  Works in either standby or suspend
   modes.
 */
#define CY_U3P_GCTL_EN_UIB_VBUS                             (1u << 11) /* <11:11> R:RW:0:Yes */


/*
   Enables wakeup when WatchDog #1 generates an interrupt.
 */
#define CY_U3P_GCTL_EN_WATCHDOG1                            (1u << 12) /* <12:12> R:RW:0:Yes */


/*
   Enables wakeup when WatchDog #2 generates an interrupt.
 */
#define CY_U3P_GCTL_EN_WATCHDOG2                            (1u << 13) /* <13:13> R:RW:0:Yes */



/*
   Wakeup signal polarity register
 */
#define CY_U3P_GCTL_WAKEUP_POLARITY_ADDRESS                 (0xe0050008)
#define CY_U3P_GCTL_WAKEUP_POLARITY                         (*(uvint32_t *)(0xe0050008))
#define CY_U3P_GCTL_WAKEUP_POLARITY_DEFAULT                 (0x00000800)

/*
   Polarity of the SDIO_INT signal:
   0: Wakeup when low (Applicable for SDIO interrupt wake up)
   1: Wakeup when high (Not required for SDIO interrupt wake up)
 */
#define CY_U3P_GCTL_POL_S0_SDIO_INT                         (1u << 3) /* <3:3> R:RW:0:Yes */


/*
   Polarity of the SDIO_INT signal:
   0: Wakeup when low (Applicable for SDIO interrupt wake up)
   1: Wakeup when high (Not required for SDIO interrupt wake up)
 */
#define CY_U3P_GCTL_POL_S1_SDIO_INT                         (1u << 4) /* <4:4> R:RW:0:Yes */


/*
   Polarity of the S0S1_INS signal:
   0: Wakeup when low
   1: Wakeup when high
 */
#define CY_U3P_GCTL_POL_S0S1_INS                            (1u << 5) /* <5:5> R:RW:0:Yes */


/*
   Polarity of the UART_CTS signal:
   0: Wakeup when low
   1: Wakeup when high
 */
#define CY_U3P_GCTL_POL_UART_CTS                            (1u << 6) /* <6:6> R:RW:0:Yes */


/*
   Polarity of the USB D+ signal:
   0: Wakeup when low
   1: Wakeup when high
 */
#define CY_U3P_GCTL_POL_UIB_DP                              (1u << 7) /* <7:7> R:RW:0:Yes */


/*
   Polarity of the USB D- signal:
   0: Wakeup when low
   1: Wakeup when high
 */
#define CY_U3P_GCTL_POL_UIB_DM                              (1u << 8) /* <8:8> R:RW:0:Yes */


/*
   Polarity of the VBUS signal:
   0: Wakeup when low
   1: Wakeup when high
 */
#define CY_U3P_GCTL_POL_UIB_VBUS                            (1u << 11) /* <11:11> R:RW:1:Yes */



/*
   Wakeup event register
 */
#define CY_U3P_GCTL_WAKEUP_EVENT_ADDRESS                    (0xe005000c)
#define CY_U3P_GCTL_WAKEUP_EVENT                            (*(uvint32_t *)(0xe005000c))
#define CY_U3P_GCTL_WAKEUP_EVENT_DEFAULT                    (0x00000000)

/*
   Indicates this wakeup source was the reason for system wakeup from standby/suspend
   mode.  See GCTL_WAKEUP_EN for more info.
 */
#define CY_U3P_GCTL_EV_PIB_CTRL0                            (1u << 0) /* <0:0> RW1S:RW1C:0:Yes */


/*
   Indicates this wakeup source was the reason for system wakeup from standby/suspend
   mode.  See GCTL_WAKEUP_EN for more info.
 */
#define CY_U3P_GCTL_EV_PIB_CMD                              (1u << 1) /* <1:1> RW1S:RW1C:0:Yes */


/*
   Indicates this wakeup source was the reason for system wakeup from standby/suspend
   mode.  See GCTL_WAKEUP_EN for more info.
 */
#define CY_U3P_GCTL_EV_PIB_CLK                              (1u << 2) /* <2:2> RW1S:RW1C:0:Yes */


/*
   Indicates this wakeup source was the reason for system wakeup from standby/suspend
   mode.  See GCTL_WAKEUP_EN for more info.
 */
#define CY_U3P_GCTL_EV_S0_SDIO_INT                          (1u << 3) /* <3:3> RW1S:RW1C:0:Yes */


/*
   Indicates this wakeup source was the reason for system wakeup from standby/suspend
   mode.  See GCTL_WAKEUP_EN for more info.
 */
#define CY_U3P_GCTL_EV_S1_SDIO_INT                          (1u << 4) /* <4:4> RW1S:RW1C:0:Yes */


/*
   Indicates this wakeup source was the reason for system wakeup from standby/suspend
   mode.  See GCTL_WAKEUP_EN for more info.
 */
#define CY_U3P_GCTL_EV_S0S1_INS                             (1u << 5) /* <5:5> RW1S:RW1C:0:Yes */


/*
   Indicates this wakeup source was the reason for system wakeup from standby/suspend
   mode.  See GCTL_WAKEUP_EN for more info.
 */
#define CY_U3P_GCTL_EV_UART_CTS                             (1u << 6) /* <6:6> RW1S:RW1C:0:Yes */


/*
   Indicates this wakeup source was the reason for system wakeup from standby/suspend
   mode.  See GCTL_WAKEUP_EN for more info.
 */
#define CY_U3P_GCTL_EV_UIB_DP                               (1u << 7) /* <7:7> RW1S:RW1C:0:Yes */


/*
   Indicates this wakeup source was the reason for system wakeup from standby/suspend
   mode.  See GCTL_WAKEUP_EN for more info.
 */
#define CY_U3P_GCTL_EV_UIB_DM                               (1u << 8) /* <8:8> RW1S:RW1C:0:Yes */


/*
   Indicates this wakeup source was the reason for system wakeup from standby/suspend
   mode.  See GCTL_WAKEUP_EN for more info.
 */
#define CY_U3P_GCTL_EV_UIB_OTGID                            (1u << 9) /* <9:9> RW1S:RW1C:0:Yes */


/*
   Indicates this wakeup source was the reason for system wakeup from standby/suspend
   mode.  See GCTL_WAKEUP_EN for more info.
 */
#define CY_U3P_GCTL_EV_UIB_SSRX                             (1u << 10) /* <10:10> RW1S:RW1C:0:Yes */


/*
   Indicates this wakeup source was the reason for system wakeup from standby/suspend
   mode.  See GCTL_WAKEUP_EN for more info.
 */
#define CY_U3P_GCTL_EV_UIB_VBUS                             (1u << 11) /* <11:11> RW1S:RW1C:0:Yes */


/*
   Indicates this wakeup source was the reason for system wakeup from standby/suspend
   mode.  See GCTL_WAKEUP_EN for more info.
 */
#define CY_U3P_GCTL_EV_WATCHDOG1                            (1u << 12) /* <12:12> RW1S:RW1C:0:Yes */


/*
   Indicates this wakeup source was the reason for system wakeup from standby/suspend
   mode.  See GCTL_WAKEUP_EN for more info.
 */
#define CY_U3P_GCTL_EV_WATCHDOG2                            (1u << 13) /* <13:13> RW1S:RW1C:0:Yes */



/*
   IO Freeze control register
 */
#define CY_U3P_GCTL_FREEZE_ADDRESS                          (0xe0050010)
#define CY_U3P_GCTL_FREEZE                                  (*(uvint32_t *)(0xe0050010))
#define CY_U3P_GCTL_FREEZE_DEFAULT                          (0x00000000)

/*
   Frozen state of IOs in the P domain
   0: Sample and hold state
   1: High impedance
   2: Drive 0 at full strength (outputs only)
   3: Drive 1 at full strength (outputs only)
   Note that states 2,3 will only override the output value driven to a fixed
   value, but will not change the drive mode of a pin from off to on.  In
   other words, pins that are currently inputs will remain inputs and not
   be forced to drive.
 */
#define CY_U3P_GCTL_PFRZ_MASK                               (0x00000003) /* <0:1> R:RW:0:No */
#define CY_U3P_GCTL_PFRZ_POS                                (0)


/*
   Frozen state of IOs in the S0 domain.
 */
#define CY_U3P_GCTL_S0FRZ_MASK                              (0x0000000c) /* <2:3> R:RW:0:No */
#define CY_U3P_GCTL_S0FRZ_POS                               (2)


/*
   Frozen state of IOs in the S1 domain.
 */
#define CY_U3P_GCTL_S1FRZ_MASK                              (0x00000030) /* <4:5> R:RW:0:No */
#define CY_U3P_GCTL_S1FRZ_POS                               (4)


/*
   Frozen state of IOs in the L domain.
 */
#define CY_U3P_GCTL_LFRZ_MASK                               (0x000000c0) /* <6:7> R:RW:0:No */
#define CY_U3P_GCTL_LFRZ_POS                                (6)


/*
   Controls two watchdog timers. Each timer can be used in free-running,
   interrupt or reset mode and has a selectable number of significant bits.
    Frequency is fixed at 32768Hz, and counters count down only. This register
   can be protected against unwanted writes by firmware (see GCTL_CONTROL).
 */
/*
   Once the backup clock divider was activated (BACKUP_CLK=1), special care
   must be taken to change the divider value. The procedure is as follows:
 */
/*
     1) Set GCTL_WATCHDOG_CS.BACKUP_CLK = 0
 */
/*
     2) Wait a sufficiently long amount of time to allow clock to shut off
 */
/*
     3) Change GCTL_WATCHDOG_CS.BACKUP_DIVIDER as desired and set GCTL_WATCHDOG_CS.BACKUP_CLK
   = 1
 */
/*
   The "sufficiently long amount of time" can be calculated as follows:
 */
/*
   Number of mmio clocks to wait = (backup clock period / mmio clock period)
   + 5
 */

/*
   Watchdog timers command and control
 */
/*
   
 */
#define CY_U3P_GCTL_WATCHDOG_CS_ADDRESS                     (0xe0050014)
#define CY_U3P_GCTL_WATCHDOG_CS                             (*(uvint32_t *)(0xe0050014))
#define CY_U3P_GCTL_WATCHDOG_CS_DEFAULT                     (0x00010303)

/*
   Counter mode:
   0: Free running mode, counter wraps around after 32 bits.
   1: Interrupt mode, interrupt when COUNTER & ~((~0)<<BITS) = 0.
   2: Reset mode, full chip RESET when COUNTER & ~((~0)<<BITS) = 0.
   3: Disable - counter does not run
 */
#define CY_U3P_GCTL_MODE0_MASK                              (0x00000003) /* <0:1> R:RW:3:Yes */
#define CY_U3P_GCTL_MODE0_POS                               (0)


/*
   Interrupt signal (Mode 1 only).
 */
#define CY_U3P_GCTL_INTR0                                   (1u << 2) /* <2:2> RW1S:RW1C:0:Yes */


/*
   Number of least significant bits to be used when checking for counter
   limit (only useful for MODE=1,2)
 */
#define CY_U3P_GCTL_BITS0_MASK                              (0x000000f8) /* <3:7> R:RW:0:Yes */
#define CY_U3P_GCTL_BITS0_POS                               (3)


/*
   Counter mode:
   0: Free running mode, counter wraps around after 32 bits.
   1: Interrupt mode, interrupt when COUNTER & ~((~0)<<BITS) = 0.
   2: Reset mode, full chip RESET when COUNTER & ~((~0)<<BITS) = 0.
   3: Disable - counter does not run
 */
#define CY_U3P_GCTL_MODE1_MASK                              (0x00000300) /* <8:9> R:RW:3:Yes */
#define CY_U3P_GCTL_MODE1_POS                               (8)


/*
   Interrupt signal (Mode 1 only).
 */
#define CY_U3P_GCTL_INTR1                                   (1u << 10) /* <10:10> RW1S:RW1C:0:Yes */


/*
   Number of least significant bits to be used when checking for counter
   limit (only useful for MODE=1,2)
 */
#define CY_U3P_GCTL_BITS1_MASK                              (0x0000f800) /* <11:15> R:RW:0:Yes */
#define CY_U3P_GCTL_BITS1_POS                               (11)


/*
   Divider used to generate a 'backup' 32kHz clock.  This is relevant for
   systems where no 32kHz is present as an input signal.  The external reference
   clock (OSCCLK) is divided by (BACKUP_DIVIDER+1) - in other words a value
   of 1 means divider by 2.  The behavior of the dividier is underfined when
   this value is 0.
 */
#define CY_U3P_GCTL_BACKUP_DIVIDER_MASK                     (0x7fff0000) /* <16:30> R:RW:1:No */
#define CY_U3P_GCTL_BACKUP_DIVIDER_POS                      (16)


/*
   Switches the watchdog clocks from the 32kHz clock input to a 'backup'
   32kHz clock derived from the main reference clock using BACKUP_DIVIDER.
 */
#define CY_U3P_GCTL_BACKUP_CLK                              (1u << 31) /* <31:31> R:RW:0:Yes */



/*
   Watchdog timer value 0
 */
/*
   Watchdog timer/counter.  Counts down and generates interrupt/reset when
   reaching low-limit (see GCTL_WATCHDOG_CS). Counter is free running and
   wraps around after 32 bits.  In watchdog mode this value must be reloaded
   periodically to avoid full chip reset. This register can be protected
   against unwanted writes by firmware (see GCTL_CONTROL).
 */
#define CY_U3P_GCTL_WATCHDOG_TIMER0_ADDRESS                 (0xe0050018)
#define CY_U3P_GCTL_WATCHDOG_TIMER0                         (*(uvint32_t *)(0xe0050018))
#define CY_U3P_GCTL_WATCHDOG_TIMER0_DEFAULT                 (0xffffffff)

/*
   Current counter value.  Please note that due to synchronization it may
   take up to 100us before a value written to this register can be read back.
    Earlier reads will return the previous value.
 */
#define CY_U3P_GCTL_COUNTER_MASK                            (0xffffffff) /* <0:31> R:RW:0xFFFFFFFF:Yes */
#define CY_U3P_GCTL_COUNTER_POS                             (0)



/*
   Watchdog timer value 0
 */
/*
   Watchdog timer/counter.  Counts down and generates interrupt/reset when
   reaching low-limit (see GCTL_WATCHDOG_CS). Counter is free running and
   wraps around after 32 bits.  In watchdog mode this value must be reloaded
   periodically to avoid full chip reset. This register can be protected
   against unwanted writes by firmware (see GCTL_CONTROL).
 */
#define CY_U3P_GCTL_WATCHDOG_TIMER1_ADDRESS                 (0xe005001c)
#define CY_U3P_GCTL_WATCHDOG_TIMER1                         (*(uvint32_t *)(0xe005001c))
#define CY_U3P_GCTL_WATCHDOG_TIMER1_DEFAULT                 (0xffffffff)

/*
   Current counter value.  Please note that due to synchronization it may
   take up to 100us before a value written to this register can be read back.
    Earlier reads will return the previous value.
 */
#define CY_U3P_GCTL_COUNTER_MASK                            (0xffffffff) /* <0:31> R:RW:0xFFFFFFFF:Yes */
#define CY_U3P_GCTL_COUNTER_POS                             (0)



#endif /* _INCLUDED_GCTLAON_REGS_H_ */

/*[]*/

