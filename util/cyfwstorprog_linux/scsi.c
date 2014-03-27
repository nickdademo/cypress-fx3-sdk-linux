#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <scsi/scsi_ioctl.h>

#include "scsi.h"

unsigned char sense_buffer[SENSE_BUFFER_SIZE];
unsigned char data_buffer[DATA_BUFFER_SIZE];

struct sg_io_hdr * init_io_hdr()
{
    struct sg_io_hdr * p_scsi_hdr = (struct sg_io_hdr *)malloc(sizeof(struct sg_io_hdr));
    memset(p_scsi_hdr, 0, sizeof(struct sg_io_hdr));
    if(p_scsi_hdr)
    {
        /* This must be set to 'S' */
        p_scsi_hdr->interface_id = 'S';
        /* The default action of the sg driver to overwrite internally the top 3 bits of the
        second SCSI command byte with the LUN associated with the file descriptor's device.
        To inhibit this action set this flag. For SCSI 3 (or later) devices,
        this internal LUN overwrite does not occur. */
        p_scsi_hdr->flags = SG_FLAG_LUN_INHIBIT;
        /* Set timeout (unit: milliseconds) */
        p_scsi_hdr->timeout = 20;
    }

    return p_scsi_hdr;
}

CyWbErrNum_t destroy_io_hdr(struct sg_io_hdr * p_hdr)
{
    if(p_hdr)
    {
        free(p_hdr);
        return CYWBERR_OK;
    }
    else
    {
        return CYWBERR_SCSI_DEALLOC_SPT_FAILED;
    }
}

void set_xfer_data(struct sg_io_hdr * p_hdr, void * data, unsigned int length)
{
    if(p_hdr)
    {
        p_hdr->dxferp = data;
        p_hdr->dxfer_len = length;
    }
}

void set_sense_data(struct sg_io_hdr * p_hdr, unsigned char * data, unsigned int length)
{
    if(p_hdr)
    {
        p_hdr->sbp = data;
        p_hdr->mx_sb_len = length;
    }
}

void show_sense_buffer(struct sg_io_hdr * hdr)
{
    unsigned char * buffer = hdr->sbp;
    int add_SENSE_BUFFER_SIZE  = 0;
    int add_sense_cnt = 0;
    int i;

    for (i = 0; i<hdr->mx_sb_len; ++i)
    {
        /* Perform end of sense data check */
        if(i > 7)
        {
            if(++add_sense_cnt > add_SENSE_BUFFER_SIZE)
            {
                break;
            }
        }

        /* Check "Additional Sense Length" field to get length of sense data */
        if(i == 7)
        {
            add_SENSE_BUFFER_SIZE = buffer[i];
        }

        fprintf(stderr, "%02x ", buffer[i]);
    }
}

bool CyWbScsi__CheckError(struct sg_io_hdr * p_hdr, uint8_t* retErrCode)
{
    if(p_hdr == NULL)
        return true;

    /* If sense info is available, check for error */
    if(p_hdr->sb_len_wr)
    {
        /* Check sense key specific valid bit */
        if((p_hdr->sbp[2] & CY_SENSE_KEY_FIELD) != 0)
        {
            *retErrCode = (p_hdr->sbp[2] & CY_SENSE_KEY_FIELD);
            return true;
        }
        else
        {
            if((p_hdr->sbp[2] & CY_FILEMARK_FIELD) != 0)
            {
                *retErrCode = CYWBERR_SCSI_CHECK_CONDITION_STATUS;
                return true;
            }
        }
    }

    return false;
}

////////////////////////
/* Mandatory Commands */
////////////////////////

// REFERENCE: "SCSI Commands Reference Manual" (Seagate, Rev. D, Dec 2010)

CyWbErrNum_t CyWbScsi__Inquiry(int fd, PINQUIRYDATA inquiryData_p)
{
    unsigned char cdb[6];
    CyWbErrNum_t errorVal = CYWBERR_OK;

    /* Initialize data structures */
    // Main control structure for SCSI generic driver
    struct sg_io_hdr * p_hdr = init_io_hdr();
    // Data buffer
    set_xfer_data(p_hdr, data_buffer, DATA_BUFFER_SIZE);
    // Sense buffer
    set_sense_data(p_hdr, sense_buffer, SENSE_BUFFER_SIZE);

    /* Setup CDB */
    cdb[0] = CYWB_SCSI_INQUIRY;     // Operation Code
    cdb[1] = 0;                     // [7:2]Reserved, [1]Obsolete, [0]EVPD
    cdb[2] = 0;                     // Page Code
    cdb[3] = 0;                     // Allocation Length (high-byte)
    cdb[4] = INQUIRYDATABUFFERSIZE; // Allocation Length (low-byte)
    cdb[5] = 0;                     // Control
    
    p_hdr->dxfer_direction = SG_DXFER_FROM_DEV;
    p_hdr->cmdp = cdb;
    p_hdr->cmd_len = sizeof(cdb);

    /* Send command */
    int ret = ioctl(fd, SG_IO, p_hdr);
    if(ret < 0)
    {
        fprintf(stderr, "ioctl: Sending SCSI Command (INQUIRY) failed: %d\n", ret);
        errorVal = CYWBERR_SCSI_SEND_FAILED;
    }
    else
    {
        bool isSenseErr = false;
        uint8_t retErrCode;

        /* Save read data */
        memcpy(inquiryData_p, p_hdr->dxferp, sizeof(INQUIRYDATA));

        isSenseErr = CyWbScsi__CheckError(p_hdr, &retErrCode);
        if(isSenseErr)
            errorVal = CYWBERR_SCSI_ERROR_IN_SENSE_DATA;
    }

    destroy_io_hdr(p_hdr);

    return errorVal;
}

CyWbErrNum_t CyWbScsi__TestUnitReady(int fd, bool *ready)
{
    unsigned char cdb[6];
    CyWbErrNum_t errorVal = CYWBERR_OK;

    /* Initialize data structures */
    // Main control structure for SCSI generic driver
    struct sg_io_hdr * p_hdr = init_io_hdr();
    // Data buffer
    set_xfer_data(p_hdr, data_buffer, DATA_BUFFER_SIZE);
    // Sense buffer
    set_sense_data(p_hdr, sense_buffer, SENSE_BUFFER_SIZE);

    /* Setup CDB */
    cdb[0] = CYWB_SCSI_TEST_UNIT_READY; // Operation Code
    cdb[1] = 0;                         // Reserved
    cdb[2] = 0;                         // Reserved
    cdb[3] = 0;                         // Reserved
    cdb[4] = 0;                         // Reserved
    cdb[5] = 0;                         // Control
    
    p_hdr->dxfer_direction = SG_DXFER_FROM_DEV;
    p_hdr->cmdp = cdb;
    p_hdr->cmd_len = sizeof(cdb);

    /* Send command */
    int ret = ioctl(fd, SG_IO, p_hdr);
    if(ret < 0)
    {
        fprintf(stderr, "ioctl: Sending SCSI Command (TEST UNIT READY) failed: %d\n", ret);
        errorVal = CYWBERR_SCSI_SEND_FAILED;
    }
    else
    {
        uint8_t retErrCode;

        *ready = CyWbScsi__CheckError(p_hdr, &retErrCode);
        if (*ready == true)
        {
            errorVal = CYWBERR_SCSI_DEVICE_NOT_READY;
        }
    }

    destroy_io_hdr(p_hdr);
    
    return errorVal;
}

CyWbErrNum_t CyWbScsi__RequestSense(int fd, PSENSE_DATA senseData_p)
{
    unsigned char cdb[6];
    CyWbErrNum_t errorVal = CYWBERR_OK;

    /* Initialize data structures */
    // Main control structure for SCSI generic driver
    struct sg_io_hdr * p_hdr = init_io_hdr();
    // Data buffer
    set_xfer_data(p_hdr, data_buffer, DATA_BUFFER_SIZE);
    // Sense buffer
    set_sense_data(p_hdr, sense_buffer, SENSE_BUFFER_SIZE);

    /* Setup CDB */
    cdb[0] = CYWB_SCSI_REQUEST_SENSE;   // Operation Code
    cdb[1] = 0;                         // [7:1]Reserved, [0]Desc
    cdb[2] = 0;                         // Reserved
    cdb[3] = 0;                         // Reserved
    cdb[4] = SENSE_BUFFER_SIZE;         // Allocation Length
    cdb[5] = 0;                         // Control
    
    p_hdr->dxfer_direction = SG_DXFER_FROM_DEV;
    p_hdr->cmdp = cdb;
    p_hdr->cmd_len = sizeof(cdb);

    /* Send command */
    int ret = ioctl(fd, SG_IO, p_hdr);
    if(ret < 0)
    {
        fprintf(stderr, "ioctl: Sending SCSI Command (REQUEST SENSE) failed: %d\n", ret);
        errorVal = CYWBERR_SCSI_SEND_FAILED;
    }
    else
    {
        bool isSenseErr = false;
        uint8_t retErrCode;

        /* Save read data */
        memcpy(senseData_p, p_hdr->dxferp, sizeof(SENSE_DATA));

        isSenseErr = CyWbScsi__CheckError(p_hdr, &retErrCode);
        if(isSenseErr)
            errorVal = CYWBERR_SCSI_ERROR_IN_SENSE_DATA;
    }

    destroy_io_hdr(p_hdr);
    
    return errorVal;
}

/* This command is currently NOT implemented in v0.01 of Cypress StorageWriter */
CyWbErrNum_t CyWbScsi__ReadCapacity(int fd, PREAD_CAPACITY_DATA capData_p)
{
    unsigned char cdb[10];
    CyWbErrNum_t errorVal = CYWBERR_OK;

    /* Initialize data structures */
    // Main control structure for SCSI generic driver
    struct sg_io_hdr * p_hdr = init_io_hdr();
    // Data buffer
    set_xfer_data(p_hdr, data_buffer, DATA_BUFFER_SIZE);
    // Sense buffer
    set_sense_data(p_hdr, sense_buffer, SENSE_BUFFER_SIZE);

    /* Setup CDB */
    cdb[0] = CYWB_SCSI_READ_CAPACITY;   // Operation Code
    cdb[1] = 0;                         // [7:1]Reserved, [0]Obsolete
    cdb[2] = 0;                         // Logical Block Address (upper byte)
    cdb[3] = 0;                         // Logical Block Address
    cdb[4] = 0;                         // Logical Block Address
    cdb[5] = 0;                         // Logical Block Address (lower byte)
    cdb[6] = 0;                         // Reserved
    cdb[7] = 0;                         // Reserved
    cdb[8] = 0;                         // [7:1]Reserved, [0]PMI
    cdb[9] = 0;                         // Control
    
    /* Send command */
    p_hdr->dxfer_direction = SG_DXFER_FROM_DEV;
    p_hdr->cmdp = cdb;
    p_hdr->cmd_len = sizeof(cdb);

    int ret = ioctl(fd, SG_IO, p_hdr);
    if(ret < 0)
    {
        fprintf(stderr, "ioctl: Sending SCSI Command (READ CAPACITY) failed: %d\n", ret);
        errorVal = CYWBERR_SCSI_SEND_FAILED;
    }
    else
    {
        bool isSenseErr = false;
        uint8_t retErrCode;

        /* Save read data */
        memcpy(capData_p, p_hdr->dxferp, sizeof(READ_CAPACITY_DATA));

        REVERSE_LONG(&capData_p->LogicalBlockAddress);
        REVERSE_LONG(&capData_p->BytesPerBlock);

        isSenseErr = CyWbScsi__CheckError(p_hdr, &retErrCode);
        if(isSenseErr)
            errorVal = CYWBERR_SCSI_ERROR_IN_SENSE_DATA;
    }

    destroy_io_hdr(p_hdr);
    
    return errorVal;
}

CyWbErrNum_t CyWbScsi__Read(int fd, uint32_t lba, uint32_t blockCount, uint8_t *readData_p)
{
    unsigned char cdb[10];
    CyWbErrNum_t errorVal = CYWBERR_OK;

    /* Initialize data structures */
    // Main control structure for SCSI generic driver
    struct sg_io_hdr * p_hdr = init_io_hdr();
    // Data buffer
    set_xfer_data(p_hdr, data_buffer, DATA_BUFFER_SIZE);
    // Sense buffer
    set_sense_data(p_hdr, sense_buffer, SENSE_BUFFER_SIZE);

    /* Setup CDB */
    cdb[0] = CYWB_SCSI_READ;                    // Operation Code
    cdb[1] = 0;                                 // [7:5]RDPROTECT, [4]DPO, [3]FUA, [2]Reserved, [1]FUA_NV, [0]Obsolete
    cdb[2] = CYGETMSB(CYGETMSW(lba));           // Logical Block Address (upper byte)
    cdb[3] = CYGETLSB(CYGETMSW(lba));           // Logical Block Address
    cdb[4] = CYGETMSB(CYGETLSW(lba));           // Logical Block Address
    cdb[5] = CYGETLSB(CYGETLSW(lba));           // Logical Block Address (lower byte)
    cdb[6] = 0;                                 // [7:5]Reserved, [4:0]Group Number
    cdb[7] = CYGETMSB(CYGETLSW(blockCount));    // Transfer Length (upper byte)
    cdb[8] = CYGETLSB(CYGETLSW(blockCount));    // Transfer Length (lower byte)
    cdb[9] = 0;                                 // Control

    /* Send command */
    p_hdr->dxfer_direction = SG_DXFER_FROM_DEV;
    p_hdr->cmdp = cdb;
    p_hdr->cmd_len = sizeof(cdb);

    int ret = ioctl(fd, SG_IO, p_hdr);
    if(ret < 0)
    {
        fprintf(stderr, "ioctl: Sending SCSI Command (READ) failed: %d\n", ret);
        errorVal = CYWBERR_SCSI_SEND_FAILED;
    }
    else
    {
        bool isSenseErr = false;
        uint8_t retErrCode;

        /* Save read data */
        memcpy(readData_p, p_hdr->dxferp, CY_WB_LOGICAL_BLOCK_SIZE_IN_BYTES*blockCount);

        isSenseErr = CyWbScsi__CheckError(p_hdr, &retErrCode);
        if(isSenseErr)
            errorVal = CYWBERR_SCSI_ERROR_IN_SENSE_DATA;
    }

    destroy_io_hdr(p_hdr);
    
    return errorVal;
}

CyWbErrNum_t CyWbScsi__Write(int fd, uint32_t lba, uint32_t blockCount, uint8_t *writeData)
{
    unsigned char cdb[10];
    CyWbErrNum_t errorVal = CYWBERR_OK;

    /* Initialize data structures */
    // Main control structure for SCSI generic driver
    struct sg_io_hdr * p_hdr = init_io_hdr();
    // Data buffer
    set_xfer_data(p_hdr, writeData, CY_WB_LOGICAL_BLOCK_SIZE_IN_BYTES * blockCount);
    // Sense buffer
    set_sense_data(p_hdr, sense_buffer, SENSE_BUFFER_SIZE);

    /* Setup CDB */
    cdb[0] = CYWB_SCSI_WRITE;                   // Operation Code
    cdb[1] = 0;                                 // [7:5]WRPROTECT, [4]DPO, [3]FUA, [2]Reserved, [1]FUA_NV, [0]Obsolete
    cdb[2] = CYGETMSB(CYGETMSW(lba));           // Logical Block Address (upper byte)
    cdb[3] = CYGETLSB(CYGETMSW(lba));           // Logical Block Address
    cdb[4] = CYGETMSB(CYGETLSW(lba));           // Logical Block Address
    cdb[5] = CYGETLSB(CYGETLSW(lba));           // Logical Block Address (lower byte)
    cdb[6] = 0;                                 // [7:5]Reserved, [4:0]Group Number
    cdb[7] = CYGETMSB(CYGETLSW(blockCount));    // Transfer Length (upper byte)
    cdb[8] = CYGETLSB(CYGETLSW(blockCount));    // Transfer Length (lower byte)
    cdb[9] = 0;                                 // Control

    p_hdr->dxfer_direction = SG_DXFER_TO_DEV;
    p_hdr->cmdp = cdb;
    p_hdr->cmd_len = sizeof(cdb);

    /* Send command */
    int ret = ioctl(fd, SG_IO, p_hdr);
    if(ret < 0)
    {
        fprintf(stderr, "ioctl: Sending SCSI Command (WRITE) failed: %d\n", ret);
        errorVal = CYWBERR_SCSI_SEND_FAILED;
    }
    else
    {
        bool isSenseErr = false;
        uint8_t retErrCode;
        
        isSenseErr = CyWbScsi__CheckError(p_hdr, &retErrCode);
        if(isSenseErr)
            errorVal = CYWBERR_SCSI_ERROR_IN_SENSE_DATA;
    }

    destroy_io_hdr(p_hdr);
    
    return errorVal;
}

//////////////////////////////////////////////////
/* Vendor Commands for FX3S Storage Boot Writer */
//////////////////////////////////////////////////

CyWbErrNum_t CyFx3sScsi__SetupPartition(int fd, uint8_t partitionNum, uint8_t type, uint8_t location, uint32_t size)
{
    unsigned char cdb[10];
    CyWbErrNum_t errorVal = CYWBERR_OK;

    /* Initialize data structures */
    // Main control structure for SCSI generic driver
    struct sg_io_hdr * p_hdr = init_io_hdr();
    // Data buffer
    set_xfer_data(p_hdr, data_buffer, DATA_BUFFER_SIZE);
    // Sense buffer
    set_sense_data(p_hdr, sense_buffer, SENSE_BUFFER_SIZE);

    /* Setup CDB */
    cdb[0] = CY_FX3S_SCSI_SET_PART_INFO;    // Operation Code
    cdb[1] = partitionNum;
    cdb[2] = type;
    cdb[3] = location;
    cdb[4] = CYGETLSB(CYGETLSW(size));      
    cdb[5] = CYGETMSB(CYGETLSW(size));
    cdb[6] = CYGETLSB(CYGETMSW(size));
    cdb[7] = CYGETMSB(CYGETMSW(size));
    cdb[8] = 0;
    cdb[9] = 0;
    
    p_hdr->dxfer_direction = SG_DXFER_FROM_DEV;
    p_hdr->cmdp = cdb;
    p_hdr->cmd_len = sizeof(cdb);

    /* Send command */
    int ret = ioctl(fd, SG_IO, p_hdr);
    if(ret < 0)
    {
        fprintf(stderr, "ioctl: Sending SCSI Command (SETUP PARTITION) failed: %d\n", ret);
        errorVal = CYWBERR_SCSI_SEND_FAILED;
    }
    else
    {
        bool isSenseErr = false;
        uint8_t retErrCode;

        isSenseErr = CyWbScsi__CheckError(p_hdr, &retErrCode);
        if(isSenseErr)
            errorVal = CYWBERR_SCSI_ERROR_IN_SENSE_DATA;
    }

    destroy_io_hdr(p_hdr);
    
    return errorVal;
}

CyWbErrNum_t CyFx3sScsi__CreatePartitions(int fd, uint8_t numParts, uint8_t * readData_p)
{
    unsigned char cdb[10];
    CyWbErrNum_t errorVal = CYWBERR_OK;

    /* Initialize data structures */
    // Main control structure for SCSI generic driver
    struct sg_io_hdr * p_hdr = init_io_hdr();
    // Data buffer
    set_xfer_data(p_hdr, data_buffer, DATA_BUFFER_SIZE);
    // Sense buffer
    set_sense_data(p_hdr, sense_buffer, SENSE_BUFFER_SIZE);

    /* Setup CDB */
    cdb[0] = CY_FX3S_SCSI_CREATE_PARTITIONS; // Operation Code
    cdb[1] = numParts;
    cdb[2] = 0;
    cdb[3] = 0;
    cdb[4] = 0;
    cdb[5] = 0;
    cdb[6] = 0;
    cdb[7] = 0;
    cdb[8] = 0;
    cdb[9] = 0;
    
    p_hdr->dxfer_direction = SG_DXFER_FROM_DEV;
    p_hdr->cmdp = cdb;
    p_hdr->cmd_len = sizeof(cdb);

    /* Send command */
    int ret = ioctl(fd, SG_IO, p_hdr);
    if(ret < 0)
    {
        fprintf(stderr, "ioctl: Sending SCSI Command (CREATE PARTITIONS) failed: %d\n", ret);
        errorVal = CYWBERR_SCSI_SEND_FAILED;
    }
    else
    {
        bool isSenseErr = false;
        uint8_t retErrCode;

        /* Save read data */
        memcpy(readData_p, p_hdr->dxferp, CY_WB_LOGICAL_BLOCK_SIZE_IN_BYTES * 1);

        isSenseErr = CyWbScsi__CheckError(p_hdr, &retErrCode);
        if(isSenseErr)
            errorVal = CYWBERR_SCSI_ERROR_IN_SENSE_DATA;
    }

    destroy_io_hdr(p_hdr);
    
    return errorVal;
}

CyWbErrNum_t CyFx3sScsi__DeletePartitions(int fd, uint8_t *readData_p)
{
    unsigned char cdb[10];
    CyWbErrNum_t errorVal = CYWBERR_OK;

    /* Initialize data structures */
    // Main control structure for SCSI generic driver
    struct sg_io_hdr * p_hdr = init_io_hdr();
    // Data buffer
    set_xfer_data(p_hdr, data_buffer, DATA_BUFFER_SIZE);
    // Sense buffer
    set_sense_data(p_hdr, sense_buffer, SENSE_BUFFER_SIZE);

    /* Setup CDB */
    cdb[0] = CY_FX3S_SCSI_DELETE_PARTITIONS; // Operation Code
    cdb[1] = 0;
    cdb[2] = 0;
    cdb[3] = 0;
    cdb[4] = 0;
    cdb[5] = 0;
    cdb[6] = 0;
    cdb[7] = 0;
    cdb[8] = 0;
    cdb[9] = 0;
    
    p_hdr->dxfer_direction = SG_DXFER_FROM_DEV;
    p_hdr->cmdp = cdb;
    p_hdr->cmd_len = sizeof(cdb);

    /* Send command */
    int ret = ioctl(fd, SG_IO, p_hdr);
    if(ret < 0)
    {
        fprintf(stderr, "ioctl: Sending SCSI Command (DELETE PARTITIONS) failed: %d\n", ret);
        errorVal = CYWBERR_SCSI_SEND_FAILED;
    }
    else
    {
        bool isSenseErr = false;
        uint8_t retErrCode;

        /* Save read data */
        memcpy(readData_p, p_hdr->dxferp, CY_WB_LOGICAL_BLOCK_SIZE_IN_BYTES * 1);

        isSenseErr = CyWbScsi__CheckError(p_hdr, &retErrCode);
        if(isSenseErr)
            errorVal = CYWBERR_SCSI_ERROR_IN_SENSE_DATA;
    }

    destroy_io_hdr(p_hdr);
    
    return errorVal;
}

CyWbErrNum_t CyFx3sScsi__FetchDeviceInfo(int fd, uint8_t *readData_p)
{
    unsigned char cdb[10];
    CyWbErrNum_t errorVal = CYWBERR_OK;

    /* Initialize data structures */
    // Main control structure for SCSI generic driver
    struct sg_io_hdr * p_hdr = init_io_hdr();
    // Data buffer
    set_xfer_data(p_hdr, data_buffer, DATA_BUFFER_SIZE);
    // Sense buffer
    set_sense_data(p_hdr, sense_buffer, SENSE_BUFFER_SIZE);

    /* Setup CDB */
    cdb[0] = CY_FX3S_SCSI_FETCH_DEVICE_INFO; // Control Code
    cdb[1] = 0;
    cdb[2] = 0;
    cdb[3] = 0;
    cdb[4] = 0;
    cdb[5] = 0;
    cdb[6] = 0;
    cdb[7] = 0;
    cdb[8] = 0;
    cdb[9] = 0;
    
    p_hdr->dxfer_direction = SG_DXFER_FROM_DEV;
    p_hdr->cmdp = cdb;
    p_hdr->cmd_len = sizeof(cdb);

    /* Send command */
    int ret = ioctl(fd, SG_IO, p_hdr);
    if(ret < 0)
    {
        fprintf(stderr, "ioctl: Sending SCSI Command (FETCH DEVICE INFO) failed: %d\n", ret);
        errorVal = CYWBERR_SCSI_SEND_FAILED;
    }
    else
    {
        bool isSenseErr = false;
        uint8_t retErrCode;

        /* Save read data */
        memcpy(readData_p, p_hdr->dxferp, CY_WB_LOGICAL_BLOCK_SIZE_IN_BYTES * 1);

        isSenseErr = CyWbScsi__CheckError(p_hdr, &retErrCode);
        if(isSenseErr)
            errorVal = CYWBERR_SCSI_ERROR_IN_SENSE_DATA;
    }

    destroy_io_hdr(p_hdr);
    
    return errorVal;
}

CyWbErrNum_t CyFx3sScsi__UsePartition(int fd, uint8_t partitionId)
{
    unsigned char cdb[10];
    CyWbErrNum_t errorVal = CYWBERR_OK;

    /* Initialize data structures */
    // Main control structure for SCSI generic driver
    struct sg_io_hdr * p_hdr = init_io_hdr();
    // Data buffer
    set_xfer_data(p_hdr, data_buffer, DATA_BUFFER_SIZE);
    // Sense buffer
    set_sense_data(p_hdr, sense_buffer, SENSE_BUFFER_SIZE);

    /* Setup CDB */
    cdb[0] = CY_FX3S_SCSI_USE_PARTITION_FOR_BOOT; // Control Code
    cdb[1] = partitionId;
    cdb[2] = 0;
    cdb[3] = 0;
    cdb[4] = 0;
    cdb[5] = 0;
    cdb[6] = 0;
    cdb[7] = 0;
    cdb[8] = 0;
    cdb[9] = 0;
    
    p_hdr->dxfer_direction = SG_DXFER_FROM_DEV;
    p_hdr->cmdp = cdb;
    p_hdr->cmd_len = sizeof(cdb);

    /* Send command */
    int ret = ioctl(fd, SG_IO, p_hdr);
    if(ret < 0)
    {
        fprintf(stderr, "ioctl: Sending SCSI Command (USE PARTITION) failed: %d\n", ret);
        errorVal = CYWBERR_SCSI_SEND_FAILED;
    }
    else
    {
        bool isSenseErr = false;
        uint8_t retErrCode;

        isSenseErr = CyWbScsi__CheckError(p_hdr, &retErrCode);
        if(isSenseErr)
            errorVal = CYWBERR_SCSI_ERROR_IN_SENSE_DATA;
    }

    destroy_io_hdr(p_hdr);
    
    return errorVal;
}

CyWbErrNum_t CyFx3sScsi__GetMmcExtendedCsd(int fd, uint8_t *readData_p)
{
    unsigned char cdb[10];
    CyWbErrNum_t errorVal = CYWBERR_OK;

    /* Initialize data structures */
    // Main control structure for SCSI generic driver
    struct sg_io_hdr * p_hdr = init_io_hdr();
    // Data buffer
    set_xfer_data(p_hdr, data_buffer, DATA_BUFFER_SIZE);
    // Sense buffer
    set_sense_data(p_hdr, sense_buffer, SENSE_BUFFER_SIZE);

    /* Setup CDB */
    cdb[0] = CY_FX3S_SCSI_GET_MMC_EXT_CSD; // Control Code
    cdb[1] = 0;
    cdb[2] = 0;
    cdb[3] = 0;
    cdb[4] = 0;
    cdb[5] = 0;
    cdb[6] = 0;
    cdb[7] = 0;
    cdb[8] = 0;
    cdb[9] = 0;
    
    p_hdr->dxfer_direction = SG_DXFER_FROM_DEV;
    p_hdr->cmdp = cdb;
    p_hdr->cmd_len = sizeof(cdb);

    /* Send command */
    int ret = ioctl(fd, SG_IO, p_hdr);
    if(ret < 0)
    {
        fprintf(stderr, "ioctl: Sending SCSI Command (GET MMC EXTENDED CSD) failed: %d\n", ret);
        errorVal = CYWBERR_SCSI_SEND_FAILED;
    }
    else
    {
        bool isSenseErr = false;
        uint8_t retErrCode;

        /* Save read data */
        memcpy(readData_p, p_hdr->dxferp, CY_WB_LOGICAL_BLOCK_SIZE_IN_BYTES * 1);

        isSenseErr = CyWbScsi__CheckError(p_hdr, &retErrCode);
        if(isSenseErr)
            errorVal = CYWBERR_SCSI_ERROR_IN_SENSE_DATA;
    }

    destroy_io_hdr(p_hdr);
    
    return errorVal;
}

CyWbErrNum_t CyFx3sScsi__EnablePartition(int fd)
{
    unsigned char cdb[10];
    CyWbErrNum_t errorVal = CYWBERR_OK;

    /* Initialize data structures */
    // Main control structure for SCSI generic driver
    struct sg_io_hdr * p_hdr = init_io_hdr();
    // Data buffer
    set_xfer_data(p_hdr, data_buffer, DATA_BUFFER_SIZE);
    // Sense buffer
    set_sense_data(p_hdr, sense_buffer, SENSE_BUFFER_SIZE);

    /* Setup CDB */
    cdb[0] = CY_FX3S_SCSI_ENABLE_PARTITION; // Control Code
    cdb[1] = 0;
    cdb[2] = 0;
    cdb[3] = 0;
    cdb[4] = 0;
    cdb[5] = 0;
    cdb[6] = 0;
    cdb[7] = 0;
    cdb[8] = 0;
    cdb[9] = 0;
    
    p_hdr->dxfer_direction = SG_DXFER_FROM_DEV;
    p_hdr->cmdp = cdb;
    p_hdr->cmd_len = sizeof(cdb);

    /* Send command */
    int ret = ioctl(fd, SG_IO, p_hdr);
    if(ret < 0)
    {
        fprintf(stderr, "ioctl: Sending SCSI Command (ENABLE PARTITION) failed: %d\n", ret);
        errorVal = CYWBERR_SCSI_SEND_FAILED;
    }
    else
    {
        bool isSenseErr = false;
        uint8_t retErrCode;

        isSenseErr = CyWbScsi__CheckError(p_hdr, &retErrCode);
        if(isSenseErr)
            errorVal = CYWBERR_SCSI_ERROR_IN_SENSE_DATA;
    }

    destroy_io_hdr(p_hdr);
    
    return errorVal;
}

CyWbErrNum_t CyFx3sScsi__WriteUserVBPHeader(int fd, uint32_t lba, uint8_t *vbpHeader)
{
    unsigned char cdb[10];
    CyWbErrNum_t errorVal = CYWBERR_OK;

    /* Initialize data structures */
    // Main control structure for SCSI generic driver
    struct sg_io_hdr * p_hdr = init_io_hdr();
    // Data buffer
    set_xfer_data(p_hdr, vbpHeader, CY_WB_LOGICAL_BLOCK_SIZE_IN_BYTES);
    // Sense buffer
    set_sense_data(p_hdr, sense_buffer, SENSE_BUFFER_SIZE);

    /* Setup CDB */
    cdb[0] = CY_FX3S_SCSI_WRITE_USER_VBP_HEADER; // Control Code
    cdb[1] = 0;
    cdb[2] = CYGETMSB(CYGETMSW(lba));
    cdb[3] = CYGETLSB(CYGETMSW(lba));
    cdb[4] = CYGETMSB(CYGETLSW(lba));
    cdb[5] = CYGETLSB(CYGETLSW(lba));
    cdb[6] = 0;
    cdb[7] = 0;
    cdb[8] = 0;
    cdb[9] = 0;
    
    p_hdr->dxfer_direction = SG_DXFER_TO_DEV;
    p_hdr->cmdp = cdb;
    p_hdr->cmd_len = sizeof(cdb);

    /* Send command */
    int ret = ioctl(fd, SG_IO, p_hdr);
    if(ret < 0)
    {
        fprintf(stderr, "ioctl: Sending SCSI Command (WRITE USER VBP HEADER) failed: %d\n", ret);
        errorVal = CYWBERR_SCSI_SEND_FAILED;
    }
    else
    {
        bool isSenseErr = false;
        uint8_t retErrCode;

        isSenseErr = CyWbScsi__CheckError(p_hdr, &retErrCode);
        if(isSenseErr)
            errorVal = CYWBERR_SCSI_ERROR_IN_SENSE_DATA;
    }

    destroy_io_hdr(p_hdr);
    
    return errorVal;
}

CyWbErrNum_t CyFx3sScsi__ReadPhysicalSector(int fd, uint32_t lba, uint8_t *readData_p)
{
    unsigned char cdb[10];
    CyWbErrNum_t errorVal = CYWBERR_OK;

    /* Initialize data structures */
    // Main control structure for SCSI generic driver
    struct sg_io_hdr * p_hdr = init_io_hdr();
    // Data buffer
    set_xfer_data(p_hdr, data_buffer, DATA_BUFFER_SIZE);
    // Sense buffer
    set_sense_data(p_hdr, sense_buffer, SENSE_BUFFER_SIZE);

    /* Setup CDB */
    cdb[0] = CY_FX3S_SCSI_READ_PHYSICAL_SECTOR; // Control Code
    cdb[1] = 0;
    cdb[2] = CYGETMSB(CYGETMSW(lba));
    cdb[3] = CYGETLSB(CYGETMSW(lba));
    cdb[4] = CYGETMSB(CYGETLSW(lba));
    cdb[5] = CYGETLSB(CYGETLSW(lba));
    cdb[6] = 0;
    cdb[7] = 0;
    cdb[8] = 0;
    cdb[9] = 0;
    
    p_hdr->dxfer_direction = SG_DXFER_FROM_DEV;
    p_hdr->cmdp = cdb;
    p_hdr->cmd_len = sizeof(cdb);

    /* Send command */
    int ret = ioctl(fd, SG_IO, p_hdr);
    if(ret < 0)
    {
        fprintf(stderr, "ioctl: Sending SCSI Command (READ PHYSICAL SECTOR) failed: %d\n", ret);
        errorVal = CYWBERR_SCSI_SEND_FAILED;
    }
    else
    {
        bool isSenseErr = false;
        uint8_t retErrCode;

        /* Save read data */
        memcpy(readData_p, p_hdr->dxferp, CY_WB_LOGICAL_BLOCK_SIZE_IN_BYTES * 1);

        isSenseErr = CyWbScsi__CheckError(p_hdr, &retErrCode);
        if(isSenseErr)
            errorVal = CYWBERR_SCSI_ERROR_IN_SENSE_DATA;
    }

    destroy_io_hdr(p_hdr);
    
    return errorVal;
}

CyWbErrNum_t CyFx3sScsi__EraseVBPHeaders(int fd, uint32_t lba)
{
    unsigned char cdb[10];
    CyWbErrNum_t errorVal = CYWBERR_OK;

    /* Initialize data structures */
    // Main control structure for SCSI generic driver
    struct sg_io_hdr * p_hdr = init_io_hdr();
    // Data buffer
    set_xfer_data(p_hdr, data_buffer, DATA_BUFFER_SIZE);
    // Sense buffer
    set_sense_data(p_hdr, sense_buffer, SENSE_BUFFER_SIZE);

    /* Setup CDB */
    cdb[0] = CY_FX3S_SCSI_ERASE_USER_VBP_HEADERS; // Control Code
    cdb[1] = 0;
    cdb[2] = CYGETMSB(CYGETMSW(lba));
    cdb[3] = CYGETLSB(CYGETMSW(lba));
    cdb[4] = CYGETMSB(CYGETLSW(lba));
    cdb[5] = CYGETLSB(CYGETLSW(lba));
    cdb[6] = 0;
    cdb[7] = 0;
    cdb[8] = 0;
    cdb[9] = 0;
    
    p_hdr->dxfer_direction = SG_DXFER_FROM_DEV;
    p_hdr->cmdp = cdb;
    p_hdr->cmd_len = sizeof(cdb);

    /* Send command */
    int ret = ioctl(fd, SG_IO, p_hdr);
    if(ret < 0)
    {
        fprintf(stderr, "ioctl: Sending SCSI Command (ERASE VBP HEADERS) failed: %d\n", ret);
        errorVal = CYWBERR_SCSI_SEND_FAILED;
    }
    else
    {
        bool isSenseErr = false;
        uint8_t retErrCode;

        isSenseErr = CyWbScsi__CheckError(p_hdr, &retErrCode);
        if(isSenseErr)
            errorVal = CYWBERR_SCSI_ERROR_IN_SENSE_DATA;
    }

    destroy_io_hdr(p_hdr);
    
    return errorVal;
}