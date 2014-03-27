/*
 ## Cypress USB 3.0 Platform header file (cyfx3bootloader.h)
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

#ifndef __CYFX3BOOTLOADER_H__
#define __CYFX3BOOTLOADER_H__

#include "cyu3types.h"

#include <cyu3externcstart.h>

typedef volatile unsigned long  ulong;
typedef volatile unsigned long  uvlong_t;

#define wPacked(x)   *((__packed uint16_t *)x)

#define PROG_ENTRY   (0x40000000)      /* Boot Loader reserved */

#define DSCR_ADDR(x) (((uint32_t)(x) >> 4) & 0xFFFF)
#define DSCR(n)      ((PDSCR_T)(CY_FX3_BOOT_SYSMEM_BASE + 0x10 + (n*0x10)))
#define MAX_DSCR     (8)   /* Allocate upto 8 Descriptors */

#define cTX_EN       (0x8000)
#define cSizeMask    (CY_U3P_LPP_BUFFER_SIZE_MASK & (cTX_EN-1))  /* 32K max */

#define VIC_SIZE     (32)
#define GCTL_Int     (0)      /* GCTL_WAKE interrupt */
#define USB_DMAInt   (8)      /* USB DMA Socket */
#define USB_CoreInt  (9)      /* Core Interrupt */
#define USB_Ep0Int   (10)     /* EP0 is not used */
#define IOPWR_Int    (21)     /* Power interrupt */

#define CY_WB_SIGNATURE          (0x42575943)
#define CY_FX3_BOOTER_SIGNATURE  (0x42335846) /* Booter Signature */
#define CY_FX3_BOOTER_REV_1_1_1  (0x00010101) /* Booter revision 1.1.1 */
#define CY_FX3_BOOTER_REV_1_2_0  (0x00010200) /* Booter revision 1.2.0 */
#define CY_FX3_BOOTER_REV_1_2_1  (0x00010201) /* Booter revision 1.2.1 */
#define CY_FX3_BOOTER_REV_1_3_0  (0x00010300) /* Booter revision 1.3.0 */
#define CY_FX3_BOOTER_REV_1_3_1  (0x00010301) /* Booter revision 1.3.1 */

#define glUsbDescPtrs      (0x40002000)

#define CPU_IP_NUM   (0x3F)
#define CPU_SCK_NUM  (0x00)

typedef  struct
{
    uint32_t buffer;      /* Pointer to buffer used. */
    uint32_t sync;        /* Consumer, Producer binding. */
    uint32_t chain;       /* Next descriptor links. */
    uint32_t size;        /* Current and maximum sizes of buffer. */
} DSCR_T, *PDSCR_T;

typedef  struct
{
    uvint32_t dscr;        /* sck_dscr */
    uvint32_t size;        /* sck_size */
    uvint32_t count;       /* sck_count */
    uvint32_t status;      /* sck_status */
    uvint32_t intr;        /* sck_intr */
    uvint32_t intr_mask;   /* sck_intr_mask */
    uvint32_t unused1[2]; 
    uvint32_t buffer;      /* dscr_buffer */
    uvint32_t sync;        /* dscr_sync */
    uvint32_t chain;       /* dscr_chain */
    uvint32_t bsize;       /* dscr_size */
    uvint32_t unused2[19];
    uvint32_t event;                       
} SCK_T, *PSCK_T;

typedef struct CyFx3BootDescPtrs_t
{
    uint8_t descType;
    uint8_t descIndex;
    uint8_t resvd0;
    uint8_t resvd1;
    uint32_t *descPtr;
} CyFx3BootDescPtrs_t;

typedef struct CyFx3BootUsbDescTable_t
{
    uint32_t signature;
    uint32_t usbSpeed;
    uint32_t numDesc;
    uint32_t length;
    CyFx3BootDescPtrs_t descPtrs[25];
    uint32_t bootSignature;
    uint32_t revision;
    uint32_t ssConnect;
    uint32_t switchToBooter;
    uint32_t leaveGpioOn;
} CyFx3BootUsbDescTable_t;

/* prototypes  */
extern void jump(int add);
extern void reset(ulong *p);

#ifdef CY_USE_ARMCC
extern void usb_isr (void) __irq;
#else
extern void usb_isr (void);
#endif

extern void
CyFx3BootBusyWait (
        uint16_t usWait);

extern void 
CyFx3BootDmaXferData (
        CyBool_t isRead,
        uint16_t socket,
        uint32_t address,
        uint32_t length );

extern void
CyFx3BootComputeChecksum (
        uint32_t *buffer,
        uint32_t  length,
        uint32_t *chkSum);

#include <cyu3externcend.h>

#endif /* __CYFX3BOOTLOADER_H__ */

