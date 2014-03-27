/*
 ## Cypress FX3 Boot Firmware Example Source file (usb_boot.c)
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

#include "cyfx3usb.h"
#include "cyfx3device.h"
#include "cyfx3utils.h"

/*
 * Note: Address of 4 KB DMA scratch buffer used for USB data transfers. This is located outside of the
 * 32 KB region allocated for the boot firmware code and data, and is expected to overlap the DMA buffer
 * region used by the full FX3 firmware image.
 *
 * Turn on the CYMEM_256K pre-processor definition to build this binary for the CYUSB3011/CYUSB3012 devices
 * that only have 256 KB of System RAM.
 */
#ifdef CYMEM_256K
#define USB_DMA_BUF_ADDRESS     (0x40037000)
#else
#define USB_DMA_BUF_ADDRESS     (0x40077000)
#endif

typedef enum
{
    eStall = 0,     /* Send STALL */
    eDataIn,        /* Data IN Stage */
    eDataOut,       /* Data Out Stage */
    eStatus         /* Status Stage */
} eUsbStage;

typedef int (*PFI)();

#define USB_SETUP_DIR               (0x80) /* 0x80 = To Host */
#define USB_SETUP_MASK              (0x60) /* Used to mask off request type */
#define USB_STANDARD_REQUEST        (0x00) /* Standard Request */
#define USB_VENDOR_REQUEST          (0x40) /* Vendor Request */
#define USB_REQ_MASK                (0x3F) /* USB Request mask */
#define USB_REQ_DEV                 (0)    /* Device Request */
#define USB_REQ_INTERFACE           (1)    /* Interface Request */
#define USB_REQ_ENDPOINT            (2)    /* Endpoint Request */
#define USB_SET_INTERFACE           (11)
#define USB_SC_SET_SEL              (0x30) /* Set system exit latency. */
#define USB_SC_SET_ISOC_DELAY       (0x31)

#define SELF_POWERED  (0x01)
#define REMOTE_WAKEUP (0x02)
#define U1_ENABLE     (0x04)
#define U2_ENABLE     (0x08)
#define LTM_ENABLE    (0x10)

uint8_t  glUsbState = 0;
uint8_t  gConfig = 0;      /* Variable to hold the config info. */
uint8_t  gAltSetting = 0;  /* Variable to hold the interface info. */
uint8_t  gUsbDevStatus = 0;
uint8_t  glCheckForDisconnect = 0;
uint8_t  glInCompliance = 0;
uint16_t gDevStatus __attribute__ ((aligned (4))) = 0;

/* 4KB of buffer area used for control endpoint transfers. */
#define gpUSBData                   (uint8_t*)(USB_DMA_BUF_ADDRESS)
#define USB_DATA_BUF_SIZE           (1024*4)

CyU3PUsbDescrPtrs   *gpUsbDescPtr; /* Pointer to the USB Descriptors */
CyFx3BootUsbEp0Pkt_t gEP0;

extern uint8_t gbDevDesc[];
extern uint8_t gbCfgDesc[];
extern uint8_t gbDevQualDesc[];
extern uint8_t gbLangIDDesc[];
extern uint8_t gbManufactureDesc[];
extern uint8_t gbProductDesc[];
extern uint8_t gbSerialNumDesc[];
extern uint8_t gbSsDevDesc[];
extern uint8_t gbBosDesc[];
extern uint8_t gbSsConfigDesc[];
extern uint8_t gbFsConfigDesc[];

extern void myMemCopy(uint8_t *d, uint8_t *s, int cnt);

/* Function to handle the GET_STATUS Standard request. */
int 
myGetStatus (
        void
        )
{
    gDevStatus = 0;

    switch (gEP0.bmReqType & USB_REQ_MASK)
    {
        case USB_REQ_DEV:
            if (CyFx3BootUsbGetSpeed () == CY_FX3_BOOT_SUPER_SPEED)
            {
                gDevStatus  = (gpUsbDescPtr->usbSSConfigDesc_p[7] & 0x40) ? 1 : 0;
                gDevStatus |= gUsbDevStatus;
            }
            else
            {
                gDevStatus  = (gpUsbDescPtr->usbHSConfigDesc_p[7] & 0x40) ? 1 : 0;
                gDevStatus |= gUsbDevStatus;
            }
            break;

        case USB_REQ_INTERFACE:
            if (!gConfig)
            {
                return eStall;
            }
            break;

        case USB_REQ_ENDPOINT:
            if (CyFx3BootUsbGetEpCfg (gEP0.bIdx0, 0, (CyBool_t *)&gDevStatus) != 0)
            {
                return eStall;
            }
            break;
        default: 
            return eStall;
    }

    *(uint16_t*)gEP0.pData = gDevStatus;
    return eDataIn;
}

/* Function to handle the GET_DESCRIPTOR Standard request */
int 
myGetDescriptor (
        void
        )
{
    uint32_t len = 0;
    uint8_t *p = 0;
    uint8_t *cfg_p = 0;
    uint8_t usbSpeed;

    usbSpeed = CyFx3BootUsbGetSpeed ();

    gpUsbDescPtr->usbHSConfigDesc_p[1] = CY_U3P_USB_CONFIG_DESCR;
    gpUsbDescPtr->usbFSConfigDesc_p[1] = CY_U3P_USB_CONFIG_DESCR;

    if (usbSpeed == CY_FX3_BOOT_HIGH_SPEED)
    {
        cfg_p = (uint8_t*)gpUsbDescPtr->usbHSConfigDesc_p;
        len = ((gpUsbDescPtr->usbHSConfigDesc_p[3] << 8) | gpUsbDescPtr->usbHSConfigDesc_p[2]);
    }
    else if (usbSpeed == CY_FX3_BOOT_SUPER_SPEED)
    {
        cfg_p = (uint8_t*)gpUsbDescPtr->usbSSConfigDesc_p;
        len = ((gpUsbDescPtr->usbSSConfigDesc_p[3] << 8) | gpUsbDescPtr->usbSSConfigDesc_p[2]);
    }
    else if (usbSpeed == CY_FX3_BOOT_FULL_SPEED)
    {
        cfg_p = (uint8_t*)gpUsbDescPtr->usbFSConfigDesc_p;
        len = ((gpUsbDescPtr->usbFSConfigDesc_p[3] << 8) | gpUsbDescPtr->usbFSConfigDesc_p[2]);
    }

    switch (gEP0.bVal1)
    {
        case CY_U3P_USB_DEVICE_DESCR:
            {
                if ((usbSpeed == CY_FX3_BOOT_HIGH_SPEED) || (usbSpeed == CY_FX3_BOOT_FULL_SPEED))
                {
                    p = (uint8_t*)gpUsbDescPtr->usbDevDesc_p; 
                    len = gpUsbDescPtr->usbDevDesc_p[0];
                }
                else if (usbSpeed == CY_FX3_BOOT_SUPER_SPEED)
                {
                    p = (uint8_t*)gpUsbDescPtr->usbSSDevDesc_p; 
                    len = gpUsbDescPtr->usbSSDevDesc_p[0];
                }
                break;
            }
        case CY_U3P_BOS_DESCR:
            {
                p = (uint8_t *)gpUsbDescPtr->usbSSBOSDesc_p; 
                len = (gpUsbDescPtr->usbSSBOSDesc_p[3] << 8) | gpUsbDescPtr->usbSSBOSDesc_p[2];
                break; 
            }
        case CY_U3P_USB_CONFIG_DESCR:
            {
                p = cfg_p;
                break;
            }
        case CY_U3P_USB_DEVQUAL_DESCR:
            {
                if ((usbSpeed == CY_FX3_BOOT_HIGH_SPEED)  || (usbSpeed == CY_FX3_BOOT_FULL_SPEED))
                {
                    p = (uint8_t*)gpUsbDescPtr->usbDevQualDesc_p;
                    len = gpUsbDescPtr->usbDevQualDesc_p[0];
                    break;
                }
                return eStall;
            }
        case CY_U3P_USB_STRING_DESCR:
            {
                /* Ensure that we do not index past the limit of the array. */
                if (gEP0.bVal0 < CY_FX3_USB_MAX_STRING_DESC_INDEX)
                {
                    p = (uint8_t*)gpUsbDescPtr->usbStringDesc_p[gEP0.bVal0];
                    if (p != 0)
                        len = p[0];
                }
                else
                    return eStall;
                break;
            }
        case CY_U3P_USB_OTHERSPEED_DESCR:
            {
                if (usbSpeed == CY_FX3_BOOT_HIGH_SPEED)
                {
                    gpUsbDescPtr->usbFSConfigDesc_p[1] = CY_U3P_USB_OTHERSPEED_DESCR;
                    p = (uint8_t*)gpUsbDescPtr->usbFSConfigDesc_p;

                    len = ((gpUsbDescPtr->usbFSConfigDesc_p[3] < 8) | gpUsbDescPtr->usbFSConfigDesc_p[2]);

                    if (len > gEP0.wLen)
                    {
                        len = gEP0.wLen;
                    }
                }
                else if (usbSpeed == CY_FX3_BOOT_FULL_SPEED)
                {
                    gpUsbDescPtr->usbHSConfigDesc_p[1] = CY_U3P_USB_OTHERSPEED_DESCR;
                    p = (uint8_t*)gpUsbDescPtr->usbHSConfigDesc_p;
                    len = ((gpUsbDescPtr->usbHSConfigDesc_p[3] < 8) | gpUsbDescPtr->usbHSConfigDesc_p[2]);
                    
                    if (len > gEP0.wLen)
                    {
                        len = gEP0.wLen;
                    }
                }
            }
            break;
        default: 
            {
                return eStall;
            }
    }

    if (p != 0)
    {
        myMemCopy (gpUSBData, p, len);
        if (gEP0.wLen > len)
        {
            gEP0.wLen = len;
        }

        return eDataIn;
    }
    else
        /* Stall EP0 if the descriptor sought is not available. */
        return eStall;
}

/* Function to handle the SET_CONFIG Standard request */
int 
mySetConfig (
        void
        )
{
    uint8_t usbSpeed = 0;
    uint32_t retVal  = 0;
    CyFx3BootUsbEpConfig_t epCfg;

    if ((gEP0.bVal0 == 0) || (gEP0.bVal0 == 1))
    {
        glUsbState = gEP0.bVal0;
        gConfig = gEP0.bVal0;

        /* Get the Bus speed */
        usbSpeed = CyFx3BootUsbGetSpeed();

        epCfg.pcktSize = 512;
        /* Based on the Bus Speed configure the endpoint packet size */
        if (usbSpeed == CY_FX3_BOOT_HIGH_SPEED)
        {
            epCfg.pcktSize = 512;
        }
        if (usbSpeed == CY_FX3_BOOT_SUPER_SPEED)
        {
            epCfg.pcktSize = 1024;
        }
        if (usbSpeed == CY_FX3_BOOT_FULL_SPEED)
        {
            epCfg.pcktSize = 64;
        }

        /* Producer Endpoint configuration */
        epCfg.enable = 1;
        epCfg.epType = CY_FX3_BOOT_USB_EP_BULK;
        epCfg.burstLen = 1;
        epCfg.streams = 0;
        epCfg.isoPkts = 0;

        /* Configure the Endpoint */
        retVal = CyFx3BootUsbSetEpConfig(0x01, &epCfg);
        if (retVal != 0)
        {
            /* TODO: Error Handling */
            return eStall;
        }

        /* Consumer Endpoint configuration */
        epCfg.enable = 1;
        epCfg.epType = CY_FX3_BOOT_USB_EP_BULK;
        epCfg.burstLen = 1;
        epCfg.streams = 0;
        epCfg.isoPkts = 0;

        /* Configure the Endpoint */
        retVal = CyFx3BootUsbSetEpConfig (0x81, &epCfg);
        if (retVal != 0)
        {
            /* TODO: Error Handling */
            return eStall;
        }

        return eStatus;
    }

    return eStall;
}

/* Function to handle the GET_INTERFACE Standard request */
int 
myGetInterface (
        void
        )
{
    if (gConfig == 0)
    {
        return eStall;
    }

    gEP0.pData = (uint8_t *)&gAltSetting;
    return eDataIn;
}

/* Function to handle the SET_INTERFACE Standard request */
int 
mySetInterface (
        void
        )
{
    gAltSetting = gEP0.bVal0; 
    return eStatus; 
}

/* This function returns stall for not supported requests. */
int 
myStall (
        void
        )
{
    return eStall; 
}

/* Function to handle the SET_ADDRESS Standard request. */
int 
mySetAddress (
        void
        )
{
    return eStatus; 
}

/* Function to handle the CLEAR_FEATURE Standard request. */
int 
myClearFeature (
        void
        )
{
    /* All of the actual handling for the CLEAR_FEATURE request is done in the API.
       We only need to update the device status flags here.
     */
    if (CyFx3BootUsbSetClrFeature (0, (CyBool_t)glUsbState, &gEP0) != 0)
    {
        return eStall;
    }

    if (gEP0.bmReqType == USB_REQ_DEV)
    {
        /* Update the device status flags as required. */
        if (CyFx3BootUsbGetSpeed () == CY_FX3_BOOT_SUPER_SPEED)
        {
            switch (gEP0.bVal0)
            {
            case 48:
                gUsbDevStatus &= ~U1_ENABLE;
                break;
            case 49:
                gUsbDevStatus &= ~U2_ENABLE;
                break;
            case 50:
                gUsbDevStatus &= ~LTM_ENABLE;
                break;
            default:
                break;
            }
        }
        else
        {
            if (gEP0.bVal0 == 1)
            {
                gUsbDevStatus &= ~REMOTE_WAKEUP;
            }
        }
    }

    return eStatus;
}

/* Function to handle the SET_FEATURE Standard request. */
int 
mySetFeature (
        void
        ) 
{
    /* All of the actual handling for the SET_FEATURE command is done in the API.
     * We only need to update the device status flags here.
     */
    if (CyFx3BootUsbSetClrFeature (1, (CyBool_t)glUsbState, &gEP0) != 0)
    {
        return eStall;
    }

    if (gEP0.bmReqType == USB_REQ_DEV)
    {
        /* Update the device status flags as required. */
        if (CyFx3BootUsbGetSpeed () == CY_FX3_BOOT_SUPER_SPEED)
        {
            switch (gEP0.bVal0)
            {
            case 48:
                gUsbDevStatus |= U1_ENABLE;
                break;
            case 49:
                gUsbDevStatus |= U2_ENABLE;
                break;
            case 50:
                gUsbDevStatus |= LTM_ENABLE;
                break;
            default:
                break;
            }
        }
        else
        {
            if (gEP0.bVal0 == 1)
            {
                gUsbDevStatus |= REMOTE_WAKEUP;
            }
        }
    }
    return eStatus;
}

/* Function to handle the GET_CONFIG Standard request. */
int 
myGetConfig (
        void
        ) 
{
    gEP0.pData = (uint8_t *)&gConfig;
    return eDataIn; 
}

const PFI chapter9_cmds[] =
{
    myGetStatus,        /* USB_GET_STATUS         0 */
    myClearFeature,     /* USB_CLEAR_FEATURE      1 */
    myStall,            /* Reserved               2 */
    mySetFeature,       /* USB_SET_FEATURE        3 */
    myStall,            /* Reserved               4 */
    mySetAddress,       /* USB_SET_ADDRESS        5 */
    myGetDescriptor,    /* USB_GET_DESCRIPTOR     6 */
    myStall,            /* USB_SET_DESCRIPTOR     7 */
    myGetConfig,        /* USB_GET_CONFIGURATION  8 */
    mySetConfig,        /* USB_SET_CONFIGURATION  9 */
    myGetInterface,     /* USB_GET_INTERFACE     10 */
    mySetInterface,     /* USB_SET_INTERFACE     11 */
};

/* This function validates the addresses being written to/read from 
   Return Value:
    0 - Address is valid
   -1 - Address is not valid
*/
int
myCheckAddress (
        uint32_t address, 
        uint32_t len
        )
{
    if (address & 3) 
    {
        /* expect long word boundary */
        return -1; 
    }

    len += address;

    if ((address >= CY_FX3_BOOT_SYSMEM_BASE1) && (len <= CY_FX3_BOOT_SYSMEM_END))
    {
        return 0;
    }

    if (len <= CY_FX3_BOOT_ITCM_END)
    {
        return 0;
    }

    return -1;
}

/* Function to handle the vendor commands. */
void 
myVendorCmdHandler (
        void
        )
{
    int stage;
    int status;
    uint32_t address  = ((gEP0.bIdx1 << 24) | (gEP0.bIdx0 << 16) | (gEP0.bVal1 << 8) | (gEP0.bVal0));
    uint16_t len  = gEP0.wLen;
    uint16_t bReq = gEP0.bReq;
    uint16_t dir  = gEP0.bmReqType & USB_SETUP_DIR;

    if (len > USB_DATA_BUF_SIZE)
    {
        CyFx3BootUsbStall (0, CyTrue, CyFalse);
        return;
    }

    if (dir)
    {
        stage = eDataIn;
    }
    else
    {
        stage = eDataOut;
    }

    /* Vendor Command 0xA0 handling */
    if (bReq == 0xA0)
    { 
	/* Note: This is a command issued by the CyControl Application to detect the legacy products.
	   As we are an FX3 device we stall the endpoint to indicate that this is not a legacy device.
	 */
	if (address == 0xE600)
	{
	    /* Stall the Endpoint */
	    CyFx3BootUsbStall (0, CyTrue, CyFalse);
	    return;
	}

        status = myCheckAddress (address, len);
        if (len == 0)
        {	
            /* Mask the USB Interrupts and Disconnect the USB Phy. */
            CyFx3BootUsbConnect (CyFalse, CyTrue);
            /* Transfer to Program Entry */    
            CyFx3BootJumpToProgramEntry (address);
            return;
        }

        if (status < 0)
        { 
            /* Stall the endpoint */
            CyFx3BootUsbStall (0, CyTrue, CyFalse);
            return;
        }

        /* Validate the SYSMEM address being accessed */
        if ((address >= CY_FX3_BOOT_SYSMEM_BASE1) && (address < CY_FX3_BOOT_SYSMEM_END))
        {
            gEP0.pData = (uint8_t*)address;
        }        

        CyFx3BootUsbAckSetup ();

        if (eDataIn == stage)
        {   
            if ((address + gEP0.wLen) <= CY_FX3_BOOT_ITCM_END)
            {   
                myMemCopy(gEP0.pData, (uint8_t *)address , len);
            }

            status = CyFx3BootUsbDmaXferData (0x80, (uint32_t)gEP0.pData, gEP0.wLen, CY_FX3_BOOT_WAIT_FOREVER);
            if (status != CY_FX3_BOOT_SUCCESS)
            {
                /* USB DMA Transfer failed. Stall the Endpoint. */
                CyFx3BootUsbStall (0, CyTrue, CyFalse);
                return;
            }
        }
        else if (stage == eDataOut)
        {
            status = CyFx3BootUsbDmaXferData (0x00, (uint32_t)gEP0.pData, gEP0.wLen, CY_FX3_BOOT_WAIT_FOREVER);
            if (status != CY_FX3_BOOT_SUCCESS)
            {
                /* USB DMA Transfer failed. Stall the Endpoint. */
                CyFx3BootUsbStall (0, CyTrue, CyFalse);
                return;
            }

            /* Validate ITCM Memory */
            if ((address + gEP0.wLen) <= CY_FX3_BOOT_ITCM_END)
            {
                /* Avoid writing to the interrupt table. */
                if (address < 0xFF) {
                    gEP0.pData += 0xFF-address;
                    gEP0.wLen -= 0xFF-address;
                    address = 0xFF;
                }
                myMemCopy((uint8_t *)address, gEP0.pData, gEP0.wLen); 
            }
        }
        return;
    }

    /* No other requests are supported. Stall the Endpoint */
    CyFx3BootUsbStall (0, CyTrue, CyFalse);
    return;
}

/* Setup Data handler */
void 
mySetupDataHandler (
        uint32_t setupDat0,
        uint32_t setupDat1
        )
{
    uint32_t *p;
    int status = eStall;

    p = (uint32_t*)&gEP0;
    p[0] = setupDat0;
    p[1] = setupDat1;

    gEP0.pData = gpUSBData;

    switch (gEP0.bmReqType & USB_SETUP_MASK)
    {
        case USB_STANDARD_REQUEST: 
            if (gEP0.bReq <= USB_SET_INTERFACE) 
            {
                status = (*chapter9_cmds[gEP0.bReq])();
            }
            else
            {
                if (gEP0.bReq == USB_SC_SET_SEL)
                {
                    if ((CyFx3BootUsbGetSpeed () == CY_FX3_BOOT_SUPER_SPEED) && (gEP0.bIdx0 == 0) &&
                            (gEP0.bIdx1 == 0) && (gEP0.bVal0 == 0) && (gEP0.bVal1 == 0) && (gEP0.wLen == 6))
                    {
                        gEP0.wLen = 32;
                        status = eDataOut;
                    }
                    else
                    {
                        status = eStall;
                    }
                }
                else if (gEP0.bReq == USB_SC_SET_ISOC_DELAY)
                {
                    status = eStatus;
                    if ((CyFx3BootUsbGetSpeed () != CY_FX3_BOOT_SUPER_SPEED) || (gEP0.bIdx0 != 0) ||
                            (gEP0.bIdx1 != 0) || (gEP0.wLen != 0))
                    {
                        status = eStall;
                    }
                }
                else
                {
                    status = eStall;
                }
            }
            break;

        case USB_VENDOR_REQUEST: 
            myVendorCmdHandler();
            return;
    }

    switch (status)
    {
        case eDataIn:            
            CyFx3BootUsbAckSetup ();
            status = CyFx3BootUsbDmaXferData (0x80, (uint32_t)gEP0.pData, gEP0.wLen, 1000);
            if (status != CY_FX3_BOOT_SUCCESS)
            {
                CyFx3BootUsbStall (0, CyTrue, CyFalse);
            }
            break; 
        case eDataOut:
            CyFx3BootUsbAckSetup ();
            status = CyFx3BootUsbDmaXferData (0x00, (uint32_t)gEP0.pData, gEP0.wLen, CY_FX3_BOOT_WAIT_FOREVER);
            if (status != CY_FX3_BOOT_SUCCESS)
            {
                CyFx3BootUsbStall (0, CyTrue, CyFalse);
            }
            break; 
        case eStatus: 
            CyFx3BootUsbAckSetup ();
            break;
        default: 
            CyFx3BootUsbStall (0, CyTrue, CyFalse);
            return;
    }

    return;
}

/* USB Event Handler. This function is invoked from the USB ISR and as such MUST not be 
   blocked. 
*/
void 
myUsbEventCallback (
        CyFx3BootUsbEventType_t event  /* USB event */
        )
{
    if (event == CY_FX3_BOOT_USB_RESET)
    {
        gConfig              = 0;
        gAltSetting          = 0;
        gUsbDevStatus        = 0;
        glUsbState           = 0;
        glInCompliance       = 0;
    }

    if ((event == CY_FX3_BOOT_USB_CONNECT) ||
        (event == CY_FX3_BOOT_USB_DISCONNECT))
    {
        glUsbState    = 0;
        gUsbDevStatus = 0;
    }

    if (event == CY_FX3_BOOT_USB_IN_SS_DISCONNECT)
    {
        glCheckForDisconnect = CyTrue;
    }

    if (event == CY_FX3_BOOT_USB_COMPLIANCE)
    {
        glInCompliance = CyTrue;
    }

    return;
}

void
myUsbBoot ()
{
    CyFx3BootErrorCode_t apiRetStatus;
    CyBool_t no_renum = CyFalse;

    gConfig              = 0;
    gAltSetting          = 0;
    gUsbDevStatus        = 0;
    glUsbState           = 0;
    glCheckForDisconnect = 0;
    glInCompliance       = 0;

    /* Enable this code for using the USB Bootloader */
    apiRetStatus = CyFx3BootUsbStart (CyTrue, myUsbEventCallback);
    if (apiRetStatus == CY_FX3_BOOT_ERROR_NO_REENUM_REQUIRED)
        no_renum = CyTrue;

    CyFx3BootRegisterSetupCallback (mySetupDataHandler);

    if (!no_renum)
    {
        /* Note: When the no_renum option in the boot firmware is being used, the firmware library
                 makes copies of all the descriptors to pass to the full firmware. The copied data
                 is retained for use the next time the boot firmware starts up. The USB descriptors
                 should not be repeatedly set on each firmware execution to prevent memory overflow
                 in this copied data structure.
         */
        CyFx3BootUsbSetDesc (CY_U3P_USB_SET_SS_DEVICE_DESCR, 0, (uint8_t *)gbSsDevDesc);
        CyFx3BootUsbSetDesc (CY_U3P_USB_SET_FS_CONFIG_DESCR, 0, (uint8_t *)gbFsConfigDesc);
        CyFx3BootUsbSetDesc (CY_U3P_USB_SET_SS_CONFIG_DESCR, 0, (uint8_t *)gbSsConfigDesc);
        CyFx3BootUsbSetDesc (CY_U3P_USB_SET_SS_BOS_DESCR, 0, (uint8_t *)gbBosDesc);

        CyFx3BootUsbSetDesc (CY_U3P_USB_SET_HS_DEVICE_DESCR, 0, (uint8_t *)gbDevDesc);
        CyFx3BootUsbSetDesc (CY_U3P_USB_SET_DEVQUAL_DESCR, 0, (uint8_t *)gbDevQualDesc);
        CyFx3BootUsbSetDesc (CY_U3P_USB_SET_HS_CONFIG_DESCR, 0, (uint8_t *)gbCfgDesc);

        CyFx3BootUsbSetDesc (CY_U3P_USB_SET_STRING_DESCR, 0, (uint8_t *)gbLangIDDesc);
        CyFx3BootUsbSetDesc (CY_U3P_USB_SET_STRING_DESCR, 1, (uint8_t *)gbManufactureDesc);
        CyFx3BootUsbSetDesc (CY_U3P_USB_SET_STRING_DESCR, 2, (uint8_t *)gbProductDesc);
        CyFx3BootUsbSetDesc (CY_U3P_USB_SET_STRING_DESCR, 3, (uint8_t *)gbSerialNumDesc);

        gpUsbDescPtr = CyFx3BootUsbGetDesc ();
        CyFx3BootUsbConnect (CyTrue, CyTrue);
    }
    else
    {
        gpUsbDescPtr = CyFx3BootUsbGetDesc ();

        /* Assume that configuration 1 has been selected by the host. */
        gEP0.bVal0 = 1;
        mySetConfig ();
    }
}

