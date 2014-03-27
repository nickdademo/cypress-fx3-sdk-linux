/****************************************************************************
 *
 * File: gctl_regs.h
 *
 * Copyright (c) 2010-13 Cypress Semiconductor. All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION WHICH IS THE PROPERTY OF CYPRESS.
 *
 * Description:
 *   Global Control registers for the EZ-USB FX3 Device.
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

#ifndef _INCLUDED_GCTL_REGS_H_
#define _INCLUDED_GCTL_REGS_H_

#include <cyu3types.h>

#define GCTL_BASE_ADDR                           (0xe0051000)

typedef struct
{
    uvint32_t reserve[2];                         /* 0xe0051000 */
    uvint32_t iomatrix;                           /* 0xe0051008 */
    uvint32_t gpio_simple0;                       /* 0xe005100c */
    uvint32_t gpio_simple1;                       /* 0xe0051010 */
    uvint32_t gpio_complex0;                      /* 0xe0051014 */
    uvint32_t gpio_complex1;                      /* 0xe0051018 */
    uvint32_t ds;                                 /* 0xe005101c */
    uvint32_t wpu_cfg0;                           /* 0xe0051020 */
    uvint32_t wpu_cfg1;                           /* 0xe0051024 */
    uvint32_t wpd_cfg0;                           /* 0xe0051028 */
    uvint32_t wpd_cfg1;                           /* 0xe005102c */
    uvint32_t iopower;                            /* 0xe0051030 */
    uvint32_t iopwr_intr;                         /* 0xe0051034 */
    uvint32_t iopwr_intr_mask;                    /* 0xe0051038 */
    uvint32_t rsrvd0[1009];
    uvint32_t pll_cfg;                            /* 0xe0052000 */
    uvint32_t cpu_clk_cfg;                        /* 0xe0052004 */
    uvint32_t uib_core_clk;                       /* 0xe0052008 */
    uvint32_t pib_core_clk;                       /* 0xe005200c */
    uvint32_t sib0_core_clk;                      /* 0xe0052010 */
    uvint32_t sib1_core_clk;                      /* 0xe0052014 */
    uvint32_t gpio_fast_clk;                      /* 0xe0052018 */
    uvint32_t gpio_slow_clk;                      /* 0xe005201c */
    uvint32_t i2c_core_clk;                       /* 0xe0052020 */
    uvint32_t uart_core_clk;                      /* 0xe0052024 */
    uvint32_t rsrvd1;
    uvint32_t spi_core_clk;                       /* 0xe005202c */
    uvint32_t rsrvd2;
    uvint32_t i2s_core_clk;                       /* 0xe0052034 */
} GCTL_REGS_T, *PGCTL_REGS_T;

#define GCTL        ((PGCTL_REGS_T) GCTL_BASE_ADDR)


/*
   IO Matrix configuration
 */
#define CY_U3P_GCTL_IOMATRIX_ADDRESS                        (0xe0051008)
#define CY_U3P_GCTL_IOMATRIX                                (*(uvint32_t *)(0xe0051008))
#define CY_U3P_GCTL_IOMATRIX_DEFAULT                        (0x00000000)

/*
   Carkit UART configuration:
   0: Use LPP_UART (LPP_UART not available to S1)
   1: Use PIB_CTL11/PIB_CTL12 pins for carkit UART
   (enabling the USB2 PHY for Carkit UART operation must be done separately
   in UIB_PHY*).
 */
#define CY_U3P_CARKIT                                       (1u << 1) /* <1:1> R:RW:0:No */


/*
   S-Port 0 is configured with the following encoding:
   0: Connected to SIB0 (SD/MMC mode)
   1: Connected to PIB_DQ[16]..PIB_DQ[27] (GPIF-II mode)
 */
#define CY_U3P_S0CFG                                        (1u << 4) /* <4:4> R :RW:0:No */


/*
   S-Port 1 is configured with the following encoding:
   0: Connected to SIB1 (SD/MMC 8b mode)
   1: Connected to SIB1 (4b) + UART
   2: Connected to SIB1 (4b) + SPI
   3: Connected to SIB1 (4b) + I2S
   4: Connected to UART+SPI+I2S
   5: Connected to PIB_DQ+UART+I2S
 */
#define CY_U3P_S1CFG_MASK                                   (0x00000700) /* <8:10> R :RW:0:No */
#define CY_U3P_S1CFG_POS                                    (8)



/*
   GPIO Override Configuration
 */
#define CY_U3P_GCTL_GPIO_SIMPLE0_ADDRESS                    (0xe005100c)
#define CY_U3P_GCTL_GPIO_SIMPLE0                            (*(uvint32_t *)(0xe005100c))
#define CY_U3P_GCTL_GPIO_SIMPLE0_DEFAULT                    (0x00000000)

/*
   When bit <n> is set, the corresponding pin maps to simple GPIO <n>.  See
   architecture spec for detailed pin to GPIO number mapping.
 */
#define CY_U3P_OVERRIDE_L_MASK                              (0xffffffff) /* <0:31> R:RW:0:No */
#define CY_U3P_OVERRIDE_L_POS                               (0)



/*
   GPIO Override Configuration
 */
#define CY_U3P_GCTL_GPIO_SIMPLE1_ADDRESS                    (0xe0051010)
#define CY_U3P_GCTL_GPIO_SIMPLE1                            (*(uvint32_t *)(0xe0051010))
#define CY_U3P_GCTL_GPIO_SIMPLE1_DEFAULT                    (0x00000000)

/*
   When bit <n> is set, the corresponding pin maps to simple GPIO <n>.  See
   architecture spec for detailed pin to GPIO number mapping.
 */
#define CY_U3P_OVERRIDE_H_MASK                              (0x1fffffff) /* <0:28> R:RW:0:No */
#define CY_U3P_OVERRIDE_H_POS                               (0)



/*
   GPIO Override Configuration
 */
#define CY_U3P_GCTL_GPIO_COMPLEX0_ADDRESS                   (0xe0051014)
#define CY_U3P_GCTL_GPIO_COMPLEX0                           (*(uvint32_t *)(0xe0051014))
#define CY_U3P_GCTL_GPIO_COMPLEX0_DEFAULT                   (0x00000000)

/*
   When bit <n> is set, the corresponding pin maps to complex GPIO_PIN <n>
   Mod 8.  See architecture spec for detailed pin to GPIO number mapping.
    If multiple pins are mapped onto the same GPIO_PIN the behavior of the
   hardware is undefined.
 */
#define CY_U3P_OVERRIDE_L_MASK                              (0xffffffff) /* <0:31> R:RW:0:No */
#define CY_U3P_OVERRIDE_L_POS                               (0)



/*
   GPIO Override Configuration
 */
#define CY_U3P_GCTL_GPIO_COMPLEX1_ADDRESS                   (0xe0051018)
#define CY_U3P_GCTL_GPIO_COMPLEX1                           (*(uvint32_t *)(0xe0051018))
#define CY_U3P_GCTL_GPIO_COMPLEX1_DEFAULT                   (0x00000000)

/*
   When bit <n> is set, the corresponding pin maps to complex GPIO_PIN <n>
   Mod 8.  See architecture spec for detailed pin to GPIO number mapping.
    If multiple pins are mapped onto the same GPIO_PIN the behavior of the
   hardware is undefined.
 */
#define CY_U3P_OVERRIDE_H_MASK                              (0x1fffffff) /* <0:28> R:RW:0:No */
#define CY_U3P_OVERRIDE_H_POS                               (0)



/*
   IO Drive Strength Configuration
 */
#define CY_U3P_GCTL_DS_ADDRESS                              (0xe005101c)
#define CY_U3P_GCTL_DS                                      (*(uvint32_t *)(0xe005101c))
#define CY_U3P_GCTL_DS_DEFAULT                              (0x000aaaaa)

/*
   Drive strength for P-Port
   0: Quarter strength
   1: Half strength
   2: Three quarter strength
   3: Full strength
 */
#define CY_U3P_PDS_MASK                                     (0x00000003) /* <0:1> R:RW:2:No */
#define CY_U3P_PDS_POS                                      (0)


/*
   Drive strength for S-Port #0
 */
#define CY_U3P_S0DS_MASK                                    (0x0000000c) /* <2:3> R:RW:2:No */
#define CY_U3P_S0DS_POS                                     (2)


/*
   Drive strength for S-Port #1, S1VDD power domain
 */
#define CY_U3P_S1DS_MASK                                    (0x00000030) /* <4:5> R:RW:2:No */
#define CY_U3P_S1DS_POS                                     (4)


/*
   Drive strength for S-Port #1, LVDD power domain
 */
#define CY_U3P_S1LDS_MASK                                   (0x000000c0) /* <6:7> R:RW:2:No */
#define CY_U3P_S1LDS_POS                                    (6)


/*
   Drive strength for I2C pins, I2CVDD power domain
   00: slow speed 1MHz gpio (push-pull) driver
   01: I2C fast-mode 1.5-3.6V ( open-drain only)
   10: I2C fast-mode 1.2-1.5V, standard-mode 1.2-3.6V ( open-drain only)
   11: I2C fast-mode plus ( open-drain only)
 */
#define CY_U3P_I2CDS_MASK                                   (0x00000300) /* <8:9> R:RW:2:No */
#define CY_U3P_I2CDS_POS                                    (8)


/*
   Drive strength for P-Port GPIOs
 */
#define CY_U3P_PDS_G_MASK                                   (0x00000c00) /* <10:11> R:RW:2:No */
#define CY_U3P_PDS_G_POS                                    (10)


/*
   Drive strength for S-Port #0 GPIOs
 */
#define CY_U3P_S0DS_G_MASK                                  (0x00003000) /* <12:13> R:RW:2:No */
#define CY_U3P_S0DS_G_POS                                   (12)


/*
   Drive strength for S-Port #1 GPIOs, S1VDD power domain
 */
#define CY_U3P_S1DS_G_MASK                                  (0x0000c000) /* <14:15> R:RW:2:No */
#define CY_U3P_S1DS_G_POS                                   (14)


/*
   Drive strength for S-Port #1 GPIOs, LVDD power domain
 */
#define CY_U3P_S1LDS_G_MASK                                 (0x00030000) /* <16:17> R:RW:2:No */
#define CY_U3P_S1LDS_G_POS                                  (16)


/*
   Drive strength for CHGDET pin, I2CVDD power domain
 */
#define CY_U3P_I2CDS_G_MASK                                 (0x000c0000) /* <18:19> R:RW:2:No */
#define CY_U3P_I2CDS_G_POS                                  (18)



/*
   IO Pull-up Configuration
 */
#define CY_U3P_GCTL_WPU_CFG0_ADDRESS                        (0xe0051020)
#define CY_U3P_GCTL_WPU_CFG0                                (*(uvint32_t *)(0xe0051020))
#define CY_U3P_GCTL_WPU_CFG0_DEFAULT                        (0x00000000)

/*
   When set, a weak pull-up is connected for the pin associated with the
   corresponding GPIO #.  See IROS for electrical characteristics.
 */
#define CY_U3P_WPU_L_MASK                                   (0xffffffff) /* <0:31> R:RW:0:No */
#define CY_U3P_WPU_L_POS                                    (0)



/*
   IO Pull-up Configuration
 */
#define CY_U3P_GCTL_WPU_CFG1_ADDRESS                        (0xe0051024)
#define CY_U3P_GCTL_WPU_CFG1                                (*(uvint32_t *)(0xe0051024))
#define CY_U3P_GCTL_WPU_CFG1_DEFAULT                        (0x00000000)

/*
   When set, a weak pull-up is connected for the pin associated with the
   corresponding GPIO #.  See IROS for electrical characteristics.
 */
#define CY_U3P_WPU_H_MASK                                   (0x0fffffff) /* <0:27> R:RW:0:No */
#define CY_U3P_WPU_H_POS                                    (0)



/*
   IO Pull-down Configuration
 */
#define CY_U3P_GCTL_WPD_CFG0_ADDRESS                        (0xe0051028)
#define CY_U3P_GCTL_WPD_CFG0                                (*(uvint32_t *)(0xe0051028))
#define CY_U3P_GCTL_WPD_CFG0_DEFAULT                        (0x00000000)

/*
   When set, a weak pull-down is connected for the pin associated with the
   corresponding GPIO #.  See IROS for electrical characteristics.
 */
#define CY_U3P_WPD_L_MASK                                   (0xffffffff) /* <0:31> R:RW:0:No */
#define CY_U3P_WPD_L_POS                                    (0)



/*
   IO Pull-down Configuration
 */
#define CY_U3P_GCTL_WPD_CFG1_ADDRESS                        (0xe005102c)
#define CY_U3P_GCTL_WPD_CFG1                                (*(uvint32_t *)(0xe005102c))
#define CY_U3P_GCTL_WPD_CFG1_DEFAULT                        (0x00000000)

/*
   When set, a weak pull-down is connected for the pin associated with the
   corresponding GPIO #.  See IROS for electrical characteristics.
 */
#define CY_U3P_WPD_H_MASK                                   (0x0fffffff) /* <0:27> R:RW:0:No */
#define CY_U3P_WPD_H_POS                                    (0)



/*
   IO Power Observability
 */
#define CY_U3P_GCTL_IOPOWER_ADDRESS                         (0xe0051030)
#define CY_U3P_GCTL_IOPOWER                                 (*(uvint32_t *)(0xe0051030))
#define CY_U3P_GCTL_IOPOWER_DEFAULT                         (0x00000000)

/*
   Indicates all I/O power domains for this block are powered and active.
    Any time needed for internal voltages to stabilize of cells to become
   active has passed before this bit asserts.
 */
#define CY_U3P_PVDDQ                                        (1u << 0) /* <0:0> RW:R:X:No */


/*
   Indicates all I/O power domains for this block are powered and active.
    Any time needed for internal voltages to stabilize of cells to become
   active has passed before this bit asserts.
 */
#define CY_U3P_S0VDDQ                                       (1u << 2) /* <2:2> RW:R:X:No */


/*
   Indicates all I/O power domains for this block are powered and active.
    Any time needed for internal voltages to stabilize of cells to become
   active has passed before this bit asserts.
 */
#define CY_U3P_S1VDDQ                                       (1u << 3) /* <3:3> RW:R:X:No */


/*
   Indicates all I/O power domains for this block are powered and active.
    Any time needed for internal voltages to stabilize of cells to become
   active has passed before this bit asserts.
 */
#define CY_U3P_LVDDQ                                        (1u << 4) /* <4:4> RW:R:X:No */


/*
   Indicates the eFuse programming voltage pin is supplied with either 1.2V
   or 2.5V.  Presence of programming voltage at 2.5V is not detectable. 
   Programming will fail but reading succeeds when supplying this pin with
   1.2V.
 */
#define CY_U3P_EFVDDQ                                       (1u << 5) /* <5:5> RW:R:X:No */


/*
   Indicates all I/O power domains for this block are powered and active.
    Any time needed for internal voltages to stabilize of cells to become
   active has passed before this bit asserts. Note: CVDDQ is required for
   chip operation (clock and reset) - this bit will always be 1 when it can
   be accessed.
 */
#define CY_U3P_CVDDQ                                        (1u << 6) /* <6:6> RW:R:X:No */


/*
   Indicates all I/O power domains for this block are powered and active.
    Any time needed for internal voltages to stabilize of cells to become
   active has passed before this bit asserts.
 */
#define CY_U3P_I2CVDDQ                                      (1u << 7) /* <7:7> RW:R:X:No */


/*
   Indicates internal 3.3V regulator is present and stable.
 */
#define CY_U3P_USB33REG                                     (1u << 8) /* <8:8> RW:R:X:No */


/*
   Indicates internal 2.5V regulated voltage to USB2 PHY is present and stable.
 */
#define CY_U3P_USB25REG                                     (1u << 9) /* <9:9> RW:R:X:No */


/*
   Indicates VBUS voltage is present and within operating range.  Internal
   regulator voltages may not yet be stable when this pin asserts.
 */
#define CY_U3P_VBUS                                         (1u << 10) /* <10:10> RW:R:X:No */


/*
   Indicates VBAT voltage is present and within operating range.  Internal
   regulator voltages may not yet be stable when this pin asserts.
 */
#define CY_U3P_VBAT                                         (1u << 11) /* <11:11> RW:R:X:No */


/*
   Vbus level detected by regulator.: The interrupt associated with this
   can be used for detecting the over current condition when we are a A-device.
   0: Vbus Not Detected to be < 4.1V
   1: Vbus Detected to be > 4.4V
 */
#define CY_U3P_VBUS_TH                                      (1u << 12) /* <12:12> RW:R:X:No */


/*
   Indicates that internal supplies of the regulator are stable. For debug
   purposes.
   0: Internal Power is not stable
   1: Internal Power is Stable
 */
#define CY_U3P_USB_POWER_GOOD                               (1u << 13) /* <13:13> RW:R:X:No */


/*
   Trim value for USB Main Regulator output voltage.  See USB IO Subsystem
   for more details.
 */
#define CY_U3P_USB_REGULATOR_TRIM_MASK                      (0x00030000) /* <16:17> R:RW:0:No */
#define CY_U3P_USB_REGULATOR_TRIM_POS                       (16)


/*
   Defines the output value of the 3.3V regulator output. All values other
   than the default are for characterization and future use. This must not
   be used by software/firmware.
   00 : Default output of the 3.3V regulator
   01 : Regulator output is nominal 3.0V (experimental)
   10 : Regulator output is nominal 2.7V (experimental)
   11 : Not allowed
 */
#define CY_U3P_REG_CARKIT_SEL_MASK                          (0x000c0000) /* <18:19> R:RW:0:No */
#define CY_U3P_REG_CARKIT_SEL_POS                           (18)


/*
   Enables the regulator to provide different output voltage from the 3.3V
   regulator.Output value is selected by the reg_carkit_sel[1:0] bits.
   1: Enables regulator to output different voltages for the carkit mode.
   0: Does not enable regulator to provide
 */
#define CY_U3P_REG_CARKIT_EN                                (1u << 20) /* <20:20> R:RW:0:No */


/*
   Controls the 3.3V active regulator's bypass mode. This mode enables an
   external voltage to be applied to the inetrnal 3.3V node by bypassing
   the 3.3V regulator and allows the 2.5V regulator to be tested and characterized
   across a different range of input supply voltages rather than from a single
   regulated 3.3V supply. This bit has no function when the regulator is
   in standby mode.
   0: 3.3V active regulator is not in bypass mode
   1: 3.3V active regulator is in bypass mode
 */
#define CY_U3P_REG_BYPASS_EN                                (1u << 21) /* <21:21> R:RW:0:No */



/*
   IO Power Change Interrupt
 */
#define CY_U3P_GCTL_IOPWR_INTR_ADDRESS                      (0xe0051034)
#define CY_U3P_GCTL_IOPWR_INTR                              (*(uvint32_t *)(0xe0051034))
#define CY_U3P_GCTL_IOPWR_INTR_DEFAULT                      (0x00000000)

/*
   Interrupt request. Must be cleared by firmware
 */
#define CY_U3P_PVDDQ                                        (1u << 0) /* <0:0> W1S:RW1C:X:No */


/*
   Interrupt request. Must be cleared by firmware
 */
#define CY_U3P_S0VDDQ                                       (1u << 2) /* <2:2> W1S:RW1C:X:No */


/*
   Interrupt request. Must be cleared by firmware
 */
#define CY_U3P_S1VDDQ                                       (1u << 3) /* <3:3> W1S:RW1C:X:No */


/*
   Interrupt request. Must be cleared by firmware
 */
#define CY_U3P_LVDDQ                                        (1u << 4) /* <4:4> W1S:RW1C:X:No */


/*
   Interrupt request. Must be cleared by firmware
 */
#define CY_U3P_EFVDDQ                                       (1u << 5) /* <5:5> W1S:RW1C:X:No */


/*
   Interrupt request. Must be cleared by firmware.  Note: CVDDQ is required
   for chip operation (clock and reset) - this interrupt will never trigger
   when it is observable.
 */
#define CY_U3P_CVDDQ                                        (1u << 6) /* <6:6> W1S:RW1C:X:No */


/*
   Interrupt request. Must be cleared by firmware
 */
#define CY_U3P_I2CVDDQ                                      (1u << 7) /* <7:7> W1S:RW1C:X:No */


/*
   Interrupt request. Must be cleared by firmware
 */
#define CY_U3P_USB33REG                                     (1u << 8) /* <8:8> W1S:RW1C:X:No */


/*
   Interrupt request. Must be cleared by firmware
 */
#define CY_U3P_USB25REG                                     (1u << 9) /* <9:9> W1S:RW1C:X:No */


/*
   Interrupt request. Must be cleared by firmware
 */
#define CY_U3P_VBUS                                         (1u << 10) /* <10:10> W1S:RW1C:X:No */


/*
   Interrupt request. Must be cleared by firmware
 */
#define CY_U3P_VBAT                                         (1u << 11) /* <11:11> W1S:RW1C:X:No */


/*
   Interrupt request. Must be cleared by firmware
 */
#define CY_U3P_VBUS_TH                                      (1u << 12) /* <12:12> W1S:RW1C:X:No */


/*
   Interrupt request. Must be cleared by firmware
 */
#define CY_U3P_USB_POWER_GOOD                               (1u << 13) /* <13:13> W1S:RW1C:X:No */



/*
   IO Power Change Interrupt Mask
 */
#define CY_U3P_GCTL_IOPWR_INTR_MASK_ADDRESS                 (0xe0051038)
#define CY_U3P_GCTL_IOPWR_INTR_MASK                         (*(uvint32_t *)(0xe0051038))
#define CY_U3P_GCTL_IOPWR_INTR_MASK_DEFAULT                 (0x00000000)

/*
   Set to 1 to report interrupt to CPU
 */
#define CY_U3P_PVDDQ                                        (1u << 0) /* <0:0> R:RW:0:No */


/*
   Set to 1 to report interrupt to CPU
 */
#define CY_U3P_S0VDDQ                                       (1u << 2) /* <2:2> R:RW:0:No */


/*
   Set to 1 to report interrupt to CPU
 */
#define CY_U3P_S1VDDQ                                       (1u << 3) /* <3:3> R:RW:0:No */


/*
   Set to 1 to report interrupt to CPU
 */
#define CY_U3P_LVDDQ                                        (1u << 4) /* <4:4> R:RW:0:No */


/*
   Set to 1 to report interrupt to CPU
 */
#define CY_U3P_EFVDDQ                                       (1u << 5) /* <5:5> R:RW:0:No */


/*
   Set to 1 to report interrupt to CPU. Note: CVDDQ is required for chip
   operation (clock and reset) - this interrupt will never trigger when it
   is observable.
 */
#define CY_U3P_CVDDQ                                        (1u << 6) /* <6:6> R:RW:0:No */


/*
   Set to 1 to report interrupt to CPU
 */
#define CY_U3P_I2CVDDQ                                      (1u << 7) /* <7:7> R:RW:0:No */


/*
   Set to 1 to report interrupt to CPU
 */
#define CY_U3P_USB33REG                                     (1u << 8) /* <8:8> R:RW:0:No */


/*
   Set to 1 to report interrupt to CPU
 */
#define CY_U3P_USB25REG                                     (1u << 9) /* <9:9> R:RW:0:No */


/*
   Set to 1 to report interrupt to CPU
 */
#define CY_U3P_VBUS                                         (1u << 10) /* <10:10> R:RW:0:No */


/*
   Set to 1 to report interrupt to CPU
 */
#define CY_U3P_VBAT                                         (1u << 11) /* <11:11> R:RW:0:No */


/*
   Set to 1 to report interrupt to CPU
 */
#define CY_U3P_VBUS_TH                                      (1u << 12) /* <12:12> R:RW:0:No */


/*
   Set to 1 to report interrupt to CPU
 */
#define CY_U3P_USB_POWER_GOOD                               (1u << 13) /* <13:13> R:RW:0:No */



/*
   PLL Configuration Register
 */
#define CY_U3P_GCTL_PLL_CFG_ADDRESS                         (0xe0052000)
#define CY_U3P_GCTL_PLL_CFG                                 (*(uvint32_t *)(0xe0052000))
#define CY_U3P_GCTL_PLL_CFG_DEFAULT                         (0x00000000)

/*
   PLL Feedback Divider Configuration
 */
#define CY_U3P_FBDIV_MASK                                   (0x0000003f) /* <0:5> RW:RW:H:No */
#define CY_U3P_FBDIV_POS                                    (0)


/*
   PLL Output Divider Configuration
 */
#define CY_U3P_OUTDIV_MASK                                  (0x000000c0) /* <6:7> R:RW:0:No */
#define CY_U3P_OUTDIV_POS                                   (6)


/*
   PLL Input Reference Divider Configuration
 */
#define CY_U3P_REFDIV                                       (1u << 8) /* <8:8> RW:RW:H:No */


/*
   PLL Charge Pump Configuration
   0: 2.5uA
   1: 5uA
   2: 7.5uA
   3: 10uA
   The charge pump bit setting will vary depending on both the refclk frequency
   and the configuration divider bits.  This field must be set to the value
   specified in the PLL specification (Table 2).
 */
#define CY_U3P_CP_CFG_MASK                                  (0x00003000) /* <12:13> R:RW:0:No */
#define CY_U3P_CP_CFG_POS                                   (12)


/*
   Value presented on FSLC[2:0] pins on the device that identify reference
   clock frequency/crystal provided.  FSLC[2] selects between reference clock
   or crystal, FSLC[1:0] select reference clock frequence when FSLC[2]=0.
 */
#define CY_U3P_FSLC_MASK                                    (0x00070000) /* <16:18> RW:R:X:No */
#define CY_U3P_FSLC_POS                                     (16)


/*
   Asserted when PLL locks
 */
#define CY_U3P_PLL_LOCK                                     (1u << 19) /* <19:19> RW:R:0:No */



/*
   CPU and Bus Clock Configuration Register
 */
#define CY_U3P_GCTL_CPU_CLK_CFG_ADDRESS                     (0xe0052004)
#define CY_U3P_GCTL_CPU_CLK_CFG                             (*(uvint32_t *)(0xe0052004))
#define CY_U3P_GCTL_CPU_CLK_CFG_DEFAULT                     (0x00001121)

/*
   CPU Clock Divider Value.  This determines how much to divide the source
   clock selected by the SRC field of this register.  Actual divider is DIV
   + 1.  Zero (divide by 1) is illegal and will result in undefined behavior.
    In other words, the range of divider values is 2-16.
 */
#define CY_U3P_GCTL_CPUCLK_CPU_DIV_MASK                     (0x0000000f) /* <0:3> R:RW:1:No */
#define CY_U3P_GCTL_CPUCLK_CPU_DIV_POS                      (0)


/*
   Clock Source Select.  This field selects between one of the following
   four pre-stage system clocks
   00: sys16_clk (sys_clk_pll divided by 16)
   01: sys4_clk (sys_clk_pll divided by 4)
   10: sys2_clk (sys_clk_pll divided by 2)
   11: sys_clk_pll
   On power up, CPU clock will be PLL clock divided by 2, which is around
   100MHz. Expectation is that BootROM/Firmware changes this value to get
   to final CPU frequency.
 */
#define CY_U3P_GCTL_CPUCLK_SRC_MASK                         (0x00000030) /* <4:5> R:RW:2:No */
#define CY_U3P_GCTL_CPUCLK_SRC_POS                          (4)


/*
   DMA Bus Clock Divider Value.  This determines how much to divide the CPU
   clock.  Actual divider is DIV + 1.  Zero (divide by 1) is illegal and
   will result in undefined behavior.  In other words, the range of divider
   values is 2-16.
 */
#define CY_U3P_GCTL_CPUCLK_DMA_DIV_MASK                     (0x00000f00) /* <8:11> R:RW:1:No */
#define CY_U3P_GCTL_CPUCLK_DMA_DIV_POS                      (8)


/*
   MMIO Bus Clock Divider Value.  This determines how much to divide the
   CPU clock.  Actual divider is DIV + 1. Zero (divide by 1) is illegal and
   will result in undefined behavior.  In other words, the range of divider
   values is 2-16.  The following must be true: (MMIO_DIV+1)= N*(DMA_DIV+1).
 */
#define CY_U3P_GCTL_CPUCLK_MMIO_DIV_MASK                    (0x0000f000) /* <12:15> R:RW:1:No */
#define CY_U3P_GCTL_CPUCLK_MMIO_DIV_POS                     (12)


/*
   External Entity, such as the AP, can request CPU to enter debug mode by
   aserting this bit. The real purpose of this bit however is to drive edbg_o
   signal so that the ICC flow works correctly. See CDT52825
 */
#define CY_U3P_GCTL_CPUCLK_EDBG                             (1u << 16) /* <16:16> R:RW:0:No */



/*
   UIB Clock Configuration Register
 */
#define CY_U3P_GCTL_UIB_CORE_CLK_ADDRESS                    (0xe0052008)
#define CY_U3P_GCTL_UIB_CORE_CLK                            (*(uvint32_t *)(0xe0052008))
#define CY_U3P_GCTL_UIB_CORE_CLK_DEFAULT                    (0x0000000a)

/*
   Clock source for EPM section of UIB block:
   0: USB2 PHY 480MHz divided by 4 (120MHz)
   1: USB3 PHY 125MHz (Spread Spectrum Clock)
   2: Bus clock (typ 100MHz)
   3: Standby clock (typ 32KHz)
   This field drives a simple clock mux; the actual presence and configuration
   of the clock inputs used is defined in the appropriate registers.
   Note: in GTM test mode, ensure USB2 PHY clock is running for at least
   40us before selecting EPMCLK_SRC=0
 */
#define CY_U3P_GCTL_UIBCLK_EPMCLK_SRC_MASK                  (0x00000003) /* <0:1> R:RW:2:No */
#define CY_U3P_GCTL_UIBCLK_EPMCLK_SRC_POS                   (0)


/*
   Clock source for SuperSpeed section of UIB block:
   0: <not defined>
   1: USB3 PHY 125MHz (Spread Spectrum Clock)
   2: Bus clock (typ 100MHz)
   3: Standby clock (typ 32KHz)
   This field drives a simple clock mux; the actual presence and configuration
   of the clock inputs used is defined in the appropriate registers.
 */
#define CY_U3P_GCTL_UIBCLK_PCLK_SRC_MASK                    (0x0000000c) /* <2:3> R:RW:2:No */
#define CY_U3P_GCTL_UIBCLK_PCLK_SRC_POS                     (2)


/*
   Enable clock multiplexer.
 */
#define CY_U3P_GCTL_UIBCLK_CLK_EN                           (1u << 31) /* <31:31> R:RW:0:No */



/*
   PIB Clock Configuration Register
 */
#define CY_U3P_GCTL_PIB_CORE_CLK_ADDRESS                    (0xe005200c)
#define CY_U3P_GCTL_PIB_CORE_CLK                            (*(uvint32_t *)(0xe005200c))
#define CY_U3P_GCTL_PIB_CORE_CLK_DEFAULT                    (0x00000001)

/*
   Clock Divider Value.  This determines how much to divide the PLL system
   clock.  Actual divider is DIV + 1.  Zero (divide by 1) is illegal and
   will result in undefined behavior. In other words, the range of divider
   values is 2-1024.
 */
#define CY_U3P_GCTL_PIBCLK_DIV_MASK                         (0x000003ff) /* <0:9> R:RW:1:No */
#define CY_U3P_GCTL_PIBCLK_DIV_POS                          (0)


/*
   Non-Integral Divider Select.  This field adds 0.5 to the divider value
   selected by the DIV field.  This will yield divider values from 2.5 to
   256.5
 */
#define CY_U3P_GCTL_PIBCLK_HALFDIV                          (1u << 10) /* <10:10> R:RW:0:No */


/*
   Clock Source Select.  This field selects between one of the following
   four pre-stage system clocks
   00: sys16_clk (sys_clk_pll divided by 16)
   01: sys4_clk (sys_clk_pll divided by 4)
   10: sys2_clk (sys_clk_pll divided by 2)
   11: sys_clk_pll
 */
#define CY_U3P_GCTL_PIBCLK_SRC_MASK                         (0x00001800) /* <11:12> R:RW:0:No */
#define CY_U3P_GCTL_PIBCLK_SRC_POS                          (11)


/*
   Enable clock divider. Both divider itself and its output are gated.
 */
#define CY_U3P_GCTL_PIBCLK_CLK_EN                           (1u << 31) /* <31:31> R:RW:0:No */



/*
   SIB Port 0 Core Clock Configuration Register
 */
#define CY_U3P_GCTL_SIB0_CORE_CLK_ADDRESS                   (0xe0052010)
#define CY_U3P_GCTL_SIB0_CORE_CLK                           (*(uvint32_t *)(0xe0052010))
#define CY_U3P_GCTL_SIB0_CORE_CLK_DEFAULT                   (0x00000001)

/*
   Clock Divider Value.  This determines how much to divide the PLL system
   clock.  Actual divider is DIV + 1.  Zero (divide by 1) is illegal and
   will result in undefined behavior. In other words, the range of divider
   values is 2-1024.
 */
#define CY_U3P_GCTL_SIBCLK_DIV_MASK                         (0x000003ff) /* <0:9> R:RW:1:No */
#define CY_U3P_GCTL_SIBCLK_DIV_POS                          (0)


/*
   Non-Integral Divider Select.  This field adds 0.5 to the divider value
   selected by the DIV field.  This will yield divider values from 2.5 to
   1024.5
 */
#define CY_U3P_GCTL_SIBCLK_HALFDIV                          (1u << 10) /* <10:10> R:RW:0:No */


/*
   Clock Source Select.  This field selects between one of the following
   four pre-stage system clocks
   00: sys16_clk (sys_clk_pll divided by 16)
   01: sys4_clk (sys_clk_pll divided by 4)
   10: sys2_clk (sys_clk_pll divided by 2)
   11: sys_clk_pll
 */
#define CY_U3P_GCTL_SIBCLK_SRC_MASK                         (0x00001800) /* <11:12> R:RW:0:No */
#define CY_U3P_GCTL_SIBCLK_SRC_POS                          (11)


/*
   Enable clock divider. Both divider itself and its output are gated.
 */
#define CY_U3P_GCTL_SIBCLK_CLK_EN                           (1u << 31) /* <31:31> R:RW:0:No */



/*
   SIB Port 1 Core Clock Configuration Register
 */
#define CY_U3P_GCTL_SIB1_CORE_CLK_ADDRESS                   (0xe0052014)
#define CY_U3P_GCTL_SIB1_CORE_CLK                           (*(uvint32_t *)(0xe0052014))
#define CY_U3P_GCTL_SIB1_CORE_CLK_DEFAULT                   (0x00000001)


/*
   GPIO Fast Clock Configuration Register
 */
#define CY_U3P_GCTL_GPIO_FAST_CLK_ADDRESS                   (0xe0052018)
#define CY_U3P_GCTL_GPIO_FAST_CLK                           (*(uvint32_t *)(0xe0052018))
#define CY_U3P_GCTL_GPIO_FAST_CLK_DEFAULT                   (0x00000101)

/*
   Clock Divider Value.  This determines how much to divide the PLL system
   clock.  Actual divider is DIV + 1.  Zero (divide by 1) is illegal and
   will result in undefined behavior. In other words, the range of divider
   values is 2-16.
 */
#define CY_U3P_GCTL_GPIOFCLK_DIV_MASK                       (0x0000000f) /* <0:3> R:RW:1:No */
#define CY_U3P_GCTL_GPIOFCLK_DIV_POS                        (0)


/*
   Non-Integral Divider Select.  This field adds 0.5 to the divider value
   selected by the DIV field.  This will yield divider values from 2.5 to
   16.5.
   Note:  Do not use HALFDIV for FAST_CLK unless GPIO_SLOW_CLK and GPIO_SIMPLE_CLK
   are both not used.  They will not function with HALFDIV=1.
   Note: HALFDIV must never be changed after CLK_EN has been set to 1 at
   least once without first applying a hardware reset.  It can be set together
   with CLK_EN in a single register write.
 */
#define CY_U3P_GCTL_GPIOFCLK_HALFDIV                        (1u << 4) /* <4:4> R:RW:0:No */


/*
   Clock Source Select.  This field selects between one of the following
   four pre-stage system clocks
   00: sys16_clk (sys_clk_pll divided by 16)
   01: sys4_clk (sys_clk_pll divided by 4)
   10: sys2_clk (sys_clk_pll divided by 2)
   11: sys_clk_pll
 */
#define CY_U3P_GCTL_GPIOFCLK_SRC_MASK                       (0x00000060) /* <5:6> R:RW:0:No */
#define CY_U3P_GCTL_GPIOFCLK_SRC_POS                        (5)


/*
   Divider value for simple gpios relative to fast GPIO clock
   0: Divide by 2
   1: Divide by 4
   2: Divide by 16
   3: Divide by 64
 */
#define CY_U3P_GCTL_GPIOFCLK_SIMPLE_MASK                    (0x00000180) /* <7:8> R:RW:2:No */
#define CY_U3P_GCTL_GPIOFCLK_SIMPLE_POS                     (7)


/*
   Enable clock divider. Both divider itself and its output are gated.
 */
#define CY_U3P_GCTL_GPIOFCLK_CLK_EN                         (1u << 31) /* <31:31> R:RW:0:No */



/*
   GPIO Slow Clock Configuration Register
 */
#define CY_U3P_GCTL_GPIO_SLOW_CLK_ADDRESS                   (0xe005201c)
#define CY_U3P_GCTL_GPIO_SLOW_CLK                           (*(uvint32_t *)(0xe005201c))
#define CY_U3P_GCTL_GPIO_SLOW_CLK_DEFAULT                   (0x00000001)

/*
   Clock Divider Value.  This determines how much to divide the GPIO_FAST_CLK
   system clock.  Actual divider is DIV + 1.  Zero (divide by 1) is illegal
   and will result in undefined behavior. In other words, the range of divider
   values is 2-64.
 */
#define CY_U3P_GCTL_GPIOSCLK_DIV_MASK                       (0x0000003f) /* <0:5> R:RW:1:No */
#define CY_U3P_GCTL_GPIOSCLK_DIV_POS                        (0)


/*
   Enable clock divider. Both divider itself and its output are gated.
 */
#define CY_U3P_GCTL_GPIOSCLK_CLK_EN                         (1u << 31) /* <31:31> R:RW:0:No */



/*
   I2C Core Clock Configuration Register
 */
#define CY_U3P_GCTL_I2C_CORE_CLK_ADDRESS                    (0xe0052020)
#define CY_U3P_GCTL_I2C_CORE_CLK                            (*(uvint32_t *)(0xe0052020))
#define CY_U3P_GCTL_I2C_CORE_CLK_DEFAULT                    (0x00000001)

/*
   Clock Divider Value.  This determines how much to divide the PLL system
   clock.  Actual divider is DIV + 1.  Zero (divide by 1) is illegal and
   will result in undefined behavior. In other words, the range of divider
   values is 2-4096.
 */
#define CY_U3P_GCTL_I2CCLK_DIV_MASK                         (0x00000fff) /* <0:11> R:RW:1:No */
#define CY_U3P_GCTL_I2CCLK_DIV_POS                          (0)


/*
   Non-Integral Divider Select.  This field adds 0.5 to the divider value
   selected by the DIV field.  This will yield divider values from 2.5 to
   4096.5
 */
#define CY_U3P_GCTL_I2CCLK_HALFDIV                          (1u << 12) /* <12:12> R:RW:0:No */


/*
   Clock Source Select.  This field selects between one of the following
   four pre-stage system clocks
   00: sys16_clk (sys_clk_pll divided by 16)
   01: sys4_clk (sys_clk_pll divided by 4)
   10: sys2_clk (sys_clk_pll divided by 2)
   11: sys_clk_pll
 */
#define CY_U3P_GCTL_I2CCLK_SRC_MASK                         (0x00006000) /* <13:14> R:RW:0:No */
#define CY_U3P_GCTL_I2CCLK_SRC_POS                          (13)


/*
   Enable clock divider. Both divider itself and its output are gated.
 */
#define CY_U3P_GCTL_I2CCLK_CLK_EN                           (1u << 31) /* <31:31> R:RW:0:No */



/*
   UART Core Clock Configuration Register
 */
#define CY_U3P_GCTL_UART_CORE_CLK_ADDRESS                   (0xe0052024)
#define CY_U3P_GCTL_UART_CORE_CLK                           (*(uvint32_t *)(0xe0052024))
#define CY_U3P_GCTL_UART_CORE_CLK_DEFAULT                   (0x00000001)

/*
   Clock Divider Value.  This determines how much to divide the PLL system
   clock.  Actual divider is DIV + 1.  Zero (divide by 1) is illegal and
   will result in undefined behavior. In other words, the range of divider
   values is 2-65536.
 */
#define CY_U3P_GCTL_UARTCLK_DIV_MASK                        (0x0000ffff) /* <0:15> R:RW:1:No */
#define CY_U3P_GCTL_UARTCLK_DIV_POS                         (0)


/*
   Non-Integral Divider Select.  This field adds 0.5 to the divider value
   selected by the DIV field.  This will yield divider values from 2.5 to
   65536.5
 */
#define CY_U3P_GCTL_UARTCLK_HALFDIV                         (1u << 16) /* <16:16> R:RW:0:No */


/*
   Clock Source Select.  This field selects between one of the following
   four pre-stage system clocks
   00: sys16_clk (sys_clk_pll divided by 16)
   01: sys4_clk (sys_clk_pll divided by 4)
   10: sys2_clk (sys_clk_pll divided by 2)
   11: sys_clk_pll
 */
#define CY_U3P_GCTL_UARTCLK_SRC_MASK                        (0x00060000) /* <17:18> R:RW:0:No */
#define CY_U3P_GCTL_UARTCLK_SRC_POS                         (17)


/*
   Enable clock divider. Both divider itself and its output are gated.
 */
#define CY_U3P_GCTL_UARTCLK_CLK_EN                          (1u << 31) /* <31:31> R:RW:0:No */



/*
   SPI Core Clock Configuration Register
 */
#define CY_U3P_GCTL_SPI_CORE_CLK_ADDRESS                    (0xe005202c)
#define CY_U3P_GCTL_SPI_CORE_CLK                            (*(uvint32_t *)(0xe005202c))
#define CY_U3P_GCTL_SPI_CORE_CLK_DEFAULT                    (0x00000001)

/*
   Clock Divider Value.  This determines how much to divide the PLL system
   clock.  Actual divider is DIV + 1.  Zero (divide by 1) is illegal and
   will result in undefined behavior. In other words, the range of divider
   values is 2-65536.
 */
#define CY_U3P_GCTL_SPICLK_DIV_MASK                         (0x0000ffff) /* <0:15> R:RW:1:No */
#define CY_U3P_GCTL_SPICLK_DIV_POS                          (0)


/*
   Non-Integral Divider Select.  This field adds 0.5 to the divider value
   selected by the DIV field.  This will yield divider values from 2.5 to
   65536.5
 */
#define CY_U3P_GCTL_SPICLK_HALFDIV                          (1u << 16) /* <16:16> R:RW:0:No */


/*
   Clock Source Select.  This field selects between one of the following
   four pre-stage system clocks
   00: sys16_clk (sys_clk_pll divided by 16)
   01: sys4_clk (sys_clk_pll divided by 4)
   10: sys2_clk (sys_clk_pll divided by 2)
   11: sys_clk_pll
 */
#define CY_U3P_GCTL_SPICLK_SRC_MASK                         (0x00060000) /* <17:18> R:RW:0:No */
#define CY_U3P_GCTL_SPICLK_SRC_POS                          (17)


/*
   Enable clock divider. Both divider itself and its output are gated.
 */
#define CY_U3P_GCTL_SPICLK_CLK_EN                           (1u << 31) /* <31:31> R:RW:0:No */



/*
   I2S Core Clock Configuration Register
 */
#define CY_U3P_GCTL_I2S_CORE_CLK_ADDRESS                    (0xe0052034)
#define CY_U3P_GCTL_I2S_CORE_CLK                            (*(uvint32_t *)(0xe0052034))
#define CY_U3P_GCTL_I2S_CORE_CLK_DEFAULT                    (0x00000001)

/*
   Clock Divider Value.  This determines how much to divide the PLL system
   clock.  Actual divider is DIV + 1.  Zero (divide by 1) is illegal and
   will result in undefined behavior. In other words, the range of divider
   values is 2-32768.
 */
#define CY_U3P_GCTL_I2SCLK_DIV_MASK                         (0x00007fff) /* <0:14> R:RW:1:No */
#define CY_U3P_GCTL_I2SCLK_DIV_POS                          (0)


/*
   Non-Integral Divider Select.  This field adds 0.5 to the divider value
   selected by the DIV field.  This will yield divider values from 2.5 to
   32768.5
 */
#define CY_U3P_GCTL_I2SCLK_HALFDIV                          (1u << 15) /* <15:15> R:RW:0:No */


/*
   Clock Source Select.  This field selects between one of the following
   four pre-stage system clocks
   00: sys16_clk (sys_clk_pll divided by 16)
   01: sys4_clk (sys_clk_pll divided by 4)
   10: sys2_clk (sys_clk_pll divided by 2)
   11: sys_clk_pll
 */
#define CY_U3P_GCTL_I2SCLK_SRC_MASK                         (0x00030000) /* <16:17> R:RW:0:No */
#define CY_U3P_GCTL_I2SCLK_SRC_POS                          (16)


/*
   0: I2S_MCLK is an output, generated from this divider
   1: I2S_MCLK is taken as the input to this divider instead of the PLL clock
   Note: Never change this value after CLK_EN has been set without first
   issuing a reset to the device.
 */
#define CY_U3P_GCTL_I2SCLK_MCLK_IN                          (1u << 30) /* <30:30> R:RW:0:No */


/*
   Enable clock divider. Both divider itself and its output are gated.
 */
#define CY_U3P_GCTL_I2SCLK_CLK_EN                           (1u << 31) /* <31:31> R:RW:0:No */



#endif /* _INCLUDED_GCTL_REGS_H_ */

/*[]*/

