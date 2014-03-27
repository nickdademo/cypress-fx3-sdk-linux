#ifndef SCSI_H_
#define SCSI_H_

#include <stdint.h>
#include <stdbool.h>

#include <scsi/sg.h>
#include <sys/ioctl.h>

#define DATA_BUFFER_SIZE                  512
#define SENSE_BUFFER_SIZE                 32
#define CY_WB_LOGICAL_BLOCK_SIZE_IN_BYTES 512
#define INQUIRYDATABUFFERSIZE             36

typedef enum CyWbScsiOpcode_t 
{
    /* Mandatory commands */
    CYWB_SCSI_INQUIRY                   = 0x12, 
    CYWB_SCSI_REQUEST_SENSE             = 0x03, 
    CYWB_SCSI_TEST_UNIT_READY           = 0x00, 
    CYWB_SCSI_READ                      = 0x28, 
    CYWB_SCSI_READ_CAPACITY             = 0x25, 
    CYWB_SCSI_READ_FORMAT_CAPACITY      = 0x23,
    CYWB_SCSI_WRITE                     = 0x2A,
    
    /* Vendor Commands for FX3S Storage Boot Writer*/
    CY_FX3S_SCSI_CREATE_PARTITIONS      = 0x60,
    CY_FX3S_SCSI_DELETE_PARTITIONS      = 0x61,    
    CY_FX3S_SCSI_FETCH_DEVICE_INFO      = 0x62,
    CY_FX3S_SCSI_SET_PART_INFO          = 0x63,
    CY_FX3S_SCSI_USE_PARTITION_FOR_BOOT = 0x64,
    CY_FX3S_SCSI_GET_MMC_EXT_CSD        = 0x65,
    CY_FX3S_SCSI_ENABLE_PARTITION       = 0x66,
    CY_FX3S_SCSI_WRITE_USER_VBP_HEADER  = 0x67,
    CY_FX3S_SCSI_READ_PHYSICAL_SECTOR   = 0x68,
    CY_FX3S_SCSI_ERASE_USER_VBP_HEADERS = 0x69
} CyWbScsiOpcode_t;

typedef enum CyWbErrNum_t
{
    CYWBERR_OK = 0,
    /* Errors in Parse parameters*/
    CYWBERR_PARSE_PARAMS_FAILED,
    CYWBERR_UNEXPECTED_KEYWORD,
    CYWBERR_UNKNOWN_PARAMETER,
    CYWBERR_MISSING_PARAMETER,
    CYWBERR_DUPLICATE_PARAMETER,
    CYWBERR_INCORRECT_PARAMETER,
    CYWBERR_MISSING_MAND_PARAMETER,
    CYWBERR_PARAM_NOT_IN_LIST,

    /* Errors in device initialise */
    CYWBERR_DEVICE_INIT_FAILED,
    CYWBERR_DEVICE_DEINIT_FAILED,

    /* Errors in SCSI command implementation*/
    CYWBERR_SCSI_SEND_FAILED,
    CYWBERR_SCSI_ALLOC_SPT_FAILED,
    CYWBERR_SCSI_DEALLOC_SPT_FAILED,
    CYWBERR_SCSI_DEVICE_NOT_READY,
    CYWBERR_SCSI_ERROR_IN_SENSE_DATA,
    CYWBERR_SCSI_INVALID_DEVICE,
    CYWBERR_SCSI_COMPARE_DATA_FAILED,
    CYWBERR_SCSI_BAD_BLOCK_SIMULATION_FAILED,
    CYWBERR_SCSI_RESTORE_BLOCK_INFO_FAILED,
    CYWBERR_SCSI_BADBLOCK,
    CYWBERR_SCSI_MORE_BAD_BLOCKS,
    CYWBERR_SCSI_ERASE_FAILED,
    CYWBERR_SCSI_WEAR_LEVEL_FAILED,
    CYWBERR_SCSI_FORCE_ECC_FAILED,
    CYWBERR_SCSI_CHECK_CONDITION_STATUS,
    CYWBERR_SCSI_NEG_TEST_FAILED,
    CYWBERR_INVALID_OP,

    /* Errors in Read-Write Tests */
    CYWBERR_LBNOTFOUND,
    CYWBERR_RW_TEST_FAILED
} CyWbErrNum_t;

#define CYGETLSB(data) ((uint8_t)((data) & 0xFF))
#define CYGETMSB(data) ((uint8_t)(((data) >> 8) & 0xFF))
#define CYGETLSW(data) ((uint16_t)((data) & 0xFFFF))
#define CYGETMSW(data) ((uint16_t)(((data) >> 16) & 0xFFFF))
#define CYMAKEWORD(data,msbVal) ((uint16_t)(((msbVal) << 8) | (data)))

/* Sense Data bitmasks */
#define CY_SENSE_KEY_FIELD  0x0F
#define CY_FILEMARK_FIELD   0xF0

/* Required Windows type definitions */
#define UCHAR uint8_t
#define ULONG uint32_t

/* INQUIRYDATA struct from Windows scsi.h header file */
typedef struct _INQUIRYDATA {
  UCHAR  DeviceType : 5;
  UCHAR  DeviceTypeQualifier : 3;
  UCHAR  DeviceTypeModifier : 7;
  UCHAR  RemovableMedia : 1;
  union {
      UCHAR  Versions;
      struct {
          UCHAR  ANSIVersion : 3;
          UCHAR  ECMAVersion : 3;
          UCHAR  ISOVersion : 2;
      };
  };
  UCHAR  ResponseDataFormat : 4;
  UCHAR  HiSupport : 1;
  UCHAR  NormACA : 1;
  UCHAR  TerminateTask : 1;
  UCHAR  AERC : 1;
  UCHAR  AdditionalLength;
  UCHAR  Reserved;
  UCHAR  Addr16 : 1;
  UCHAR  Addr32 : 1;
  UCHAR  AckReqQ: 1;
  UCHAR  MediumChanger : 1;
  UCHAR  MultiPort : 1;
  UCHAR  ReservedBit2 : 1;
  UCHAR  EnclosureServices : 1;
  UCHAR  ReservedBit3 : 1;
  UCHAR  SoftReset : 1;
  UCHAR  CommandQueue : 1;
  UCHAR  TransferDisable : 1;
  UCHAR  LinkedCommands : 1;
  UCHAR  Synchronous : 1;
  UCHAR  Wide16Bit : 1;
  UCHAR  Wide32Bit : 1;
  UCHAR  RelativeAddressing : 1;
  UCHAR  VendorId[8];
  UCHAR  ProductId[16];
  UCHAR  ProductRevisionLevel[4];
  UCHAR  VendorSpecific[20];
  UCHAR  Reserved3[40];
} INQUIRYDATA, *PINQUIRYDATA;

/* SENSE_DATA struct from Windows scsi.h header file */
typedef struct _SENSE_DATA {
  UCHAR  ErrorCode : 7;
  UCHAR  Valid : 1;
  UCHAR  SegmentNumber;
  UCHAR  SenseKey : 4;
  UCHAR  Reserved : 1;
  UCHAR  IncorrectLength : 1;
  UCHAR  EndOfMedia : 1;
  UCHAR  FileMark : 1;
  UCHAR  Information[4];
  UCHAR  AdditionalSenseLength;
  UCHAR  CommandSpecificInformation[4];
  UCHAR  AdditionalSenseCode;
  UCHAR  AdditionalSenseCodeQualifier;
  UCHAR  FieldReplaceableUnitCode;
  UCHAR  SenseKeySpecific[3];
} SENSE_DATA, *PSENSE_DATA;

/* READ_CAPACITY_DATA struct from Windows scsi.h header file */
// Data returned in Big Endian format
typedef struct _READ_CAPACITY_DATA {
  ULONG  LogicalBlockAddress;
  ULONG  BytesPerBlock;
} READ_CAPACITY_DATA, *PREAD_CAPACITY_DATA;

typedef union _FOUR_BYTE {
    struct {
        UCHAR Byte0;
        UCHAR Byte1;
        UCHAR Byte2;
        UCHAR Byte3;
    };
    ULONG AsULong;
} FOUR_BYTE, *PFOUR_BYTE;

#define REVERSE_LONG(Long) { \
    UCHAR _val; \
    PFOUR_BYTE _val2 = (PFOUR_BYTE)(Long); \
    _val = _val2->Byte3; \
    _val2->Byte3 = _val2->Byte0; \
    _val2->Byte0 = _val; \
    _val = _val2->Byte2; \
    _val2->Byte2 = _val2->Byte1; \
    _val2->Byte1 = _val; \
}

/* Helper functions */
struct sg_io_hdr * init_io_hdr(void);
CyWbErrNum_t destroy_io_hdr(struct sg_io_hdr *);
void set_xfer_data(struct sg_io_hdr * p_hdr, void * data, unsigned int length);
void set_sense_data(struct sg_io_hdr * p_hdr, unsigned char * data, unsigned int length);
void show_sense_buffer(struct sg_io_hdr * hdr);
bool CyWbScsi__CheckError(struct sg_io_hdr * p_hdr, uint8_t* retErrCode);

/* Mandatory Commands */
CyWbErrNum_t CyWbScsi__Inquiry(int fd, PINQUIRYDATA inquiryData_p);
CyWbErrNum_t CyWbScsi__TestUnitReady(int fd, bool *ready);
CyWbErrNum_t CyWbScsi__RequestSense(int fd, PSENSE_DATA senseData_p);
CyWbErrNum_t CyWbScsi__ReadCapacity(int fd, PREAD_CAPACITY_DATA capData_p);
CyWbErrNum_t CyWbScsi__Read(int fd, uint32_t lba, uint32_t blockCount, uint8_t *readData_p);
CyWbErrNum_t CyWbScsi__Write(int fd, uint32_t lba, uint32_t blockCount, uint8_t *writeData);

/* Vendor Commands for FX3S Storage Boot Writer */
CyWbErrNum_t CyFx3sScsi__SetupPartition(int fd, uint8_t partitionNum, uint8_t type, uint8_t location, uint32_t size);
CyWbErrNum_t CyFx3sScsi__CreatePartitions(int fd, uint8_t numParts, uint8_t * readData_p);
CyWbErrNum_t CyFx3sScsi__DeletePartitions(int fd, uint8_t *readData_p);
CyWbErrNum_t CyFx3sScsi__FetchDeviceInfo(int fd, uint8_t *readData_p);
CyWbErrNum_t CyFx3sScsi__UsePartition(int fd, uint8_t partitionId);
CyWbErrNum_t CyFx3sScsi__GetMmcExtendedCsd(int fd, uint8_t *readData_p);
CyWbErrNum_t CyFx3sScsi__EnablePartition(int fd);
CyWbErrNum_t CyFx3sScsi__WriteUserVBPHeader(int fd, uint32_t lba, uint8_t *vbpHeader);
CyWbErrNum_t CyFx3sScsi__ReadPhysicalSector(int fd, uint32_t lba, uint8_t *readData_p);
CyWbErrNum_t CyFx3sScsi__EraseVBPHeaders(int fd, uint32_t lba);

#endif