#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cyfwstorprog.h"

/* Global variables */
int glCmdStatus = 0;
uint8_t glDataBuffer[512];
uint32_t glPartitionSizes[4];
char glBootImagePath[100];
char glDevicePath[100];
CyFx3PartitionData_t glPartitionData;
uint8_t glActivePartition;
uint8_t glBootPartitionLoc = 0;         // DEFAULT VALUE: USER PARTITION
uint8_t glNumUserDataPartitions = 1;    // DEFAULT VALUE: 1x USER PARTITION
CyBool glDeletePartitions = CyFalse;    // DEFAULT VALUE: Do not delete
CyBool glDisplayPartitions = CyFalse;   // DEFAULT VALUE: Do not display

enum partitionString {
        RSVD_PARTITION = 0xab,
        BOOT_PARTITION = 0xbb,
        USER_PARTITION = 0xda
};

char partStr [3][10] = {
    "Boot Area",
    "Data Area",
    "Rsvd Area"
};

char locationString [3][15] = {
    "User  Location",
    "Boot1 Location",
    "Boot2 Location"
};

char mediaType [4][5] = {
    "NONE",
    "MMC ",
    "SD  ",
    "OTHR"
};

void setPartitionSizes(char * args)
{
    uint8_t i = 0;

    char * splitter;
    splitter = strtok(args, "#");
    while ((splitter!= NULL) && i<3)
    {
        glPartitionSizes[i++] = atol(splitter);
        splitter = strtok(NULL, "#");
    }
}

void setCommandLineParams(char* params, char *args)
{
    /* BOOT PARTITION */
    if(strncmp(params,"-bootp", 6) == 0)
    {
        if(strncmp(args, "boot1", 5) == 0)
            glBootPartitionLoc = 1;
        else if(strncmp(args, "boot2", 5) == 0)
            glBootPartitionLoc = 2;
        else 
            glBootPartitionLoc = 0;
    }
    /* FIRMWARE IMAGE PATH [REQUIRED] */
    else if(strncmp(params, "-img", 4) == 0)
    {
        memset(glBootImagePath, 0, 100);
        strncpy(glBootImagePath, args, 99);
    }
    /* DEVICE PATH (e.g. /dev/sdb) [REQUIRED] */
    else if(strncmp(params, "-dev", 4) == 0)
    {
        memset(glDevicePath, 0, 100);
        strncpy(glDevicePath, args, 99);
    }
    /* NUMBER OF USER PARTITIONS */
    else if(strncmp(params, "-nuserp", 7) == 0)
    {
        /* Set Number of User Partitions (Excluding any boot partitions)
         * This will be capped to 3 for card without BOOT1/BOOT2 partitions (1 User Boot+ 3 User Data partitions).
         * This will be capped to 2 (or 1)for cards with BOOT1/BOOT2 partitions 
         * (2*BOOT1/2 + 2 User; or ; 2*BOOT1/2 + 1 User Boot+ 1 User Data Partition).
         */
        glNumUserDataPartitions = atoi(args);
        if (glNumUserDataPartitions > 3)
            glNumUserDataPartitions = 3;
    }
    /* PARTITION SIZES */
    else if(strncmp(params, "-psizes", 7) == 0)
    {
        setPartitionSizes(args);
    }
    /* DELETE PARTITIONS ONLY */
    else if(strncmp(params, "-delpart", 8) == 0)
    {   
        glDeletePartitions = CyTrue;
    }
    /* DISPLAY PARTITIONS ONLY */
    else if(strncmp(params, "-disponly", 9) == 0)
    {   
        glDisplayPartitions = CyTrue;
    }
}

CyWbErrNum_t parseCmdLineParams(int argc, char* argv[])
{
    uint8_t i;
    CyWbErrNum_t err= CYWBERR_OK;
    char * splitter;
    FILE * testExists;
    char args[100];
    char params[100];

    /* argv[0] is the path to app. Ignore it. */
    for(i = 1; i < argc; i++)
    {
        if(*argv[i]=='-')
        {
            splitter = strtok(argv[i],":");
            if(splitter!= NULL)
            {
                memset(params, 0, 100);
                memset(args, 0, 100);
                strcpy(params,splitter);
                splitter = strtok(NULL, ":");
                if (splitter != NULL)
                {

                    strcpy(args, splitter);
                }
                setCommandLineParams(params, args);
            }

        }
        else
            continue;
    }

    /* Check if required parameters were passed */
    if((strlen(glBootImagePath) == 0) && (glDeletePartitions == CyFalse) && (glDisplayPartitions == CyFalse))
    {
        fprintf(stderr, "ERR: Missing firmware image parameter.\n\r");
        return CYWBERR_MISSING_PARAMETER;
    }
    if((strlen(glDevicePath) == 0) && (glDeletePartitions == CyFalse) && (glDisplayPartitions == CyFalse))
    {
        fprintf(stderr, "ERR: Missing target device path parameter.\n\r");
        return CYWBERR_MISSING_PARAMETER;
    }

    fprintf(stderr, "MSG: Parameter Settings:\n\r");
    fprintf(stderr, "MSG: Target Device Path: %s\n\r", glDevicePath);
    if(glDeletePartitions == CyFalse && glDisplayPartitions == CyFalse)
    {
        fprintf(stderr, "MSG: Boot Image to be written: %s\n\r", glBootImagePath);

        /* Check if firmware file exists */
        if((testExists = fopen(glBootImagePath,"rb")) != NULL)
        {
            fclose(testExists);
        }
        else 
        {
            if(testExists)
                fclose(testExists);
            fprintf(stderr, "ERR: Boot Image \"%s\" cannot be read.\n\r", glBootImagePath);
            return CYWBERR_INCORRECT_PARAMETER;
        }

        fprintf(stderr, "MSG: Partition Location to write Boot Image: %s\n\r", locationString[glBootPartitionLoc]);
        fprintf(stderr, "MSG: Number of User Area Partitions Requested: %d\n\r",glNumUserDataPartitions);
        if (glPartitionSizes[0] > 0)
        {   
            fprintf(stderr, "Partition Sizes requested: ");
            for(i = 0; i < glNumUserDataPartitions; i++)
            {
                fprintf(stderr, " %d ", glPartitionSizes[i]);
            }
            fprintf(stderr, "\n\r");
        }
    }
    else if(glDeletePartitions)
    {
        fprintf(stderr, "MSG: Delete Partition Option Selected. Will exit after deleting partitions.\n\r");
    }
    else 
    {
        fprintf(stderr, "MSG: Display Only Option Selected. Will exit after displaying partitions.\n\r");
    }

    return err;
}

void populatePartitionInfo(uint8_t *data, CyFx3PartitionData_p partitionData)
{
    memcpy((uint8_t*) partitionData, data, sizeof(CyFx3PartitionData_t));
}

void displayPartitionInfo(CyFx3PartitionData_p partitionData)
{
    int i;
    fprintf(stderr, "MSG: Device Partition information:");
    fprintf(stderr, "\n\rMedia Type = %6s", mediaType[partitionData->deviceType]);
    fprintf(stderr, "\nPartition Details:");
    fprintf(stderr, "\n\rNumber \t Type \t\t    Location \t\t Size");
    uint8_t type=0;
    for(i=0; i<partitionData->numPartitions; i++)
    {
        type = partitionData->partitionElement[i].type;
        fprintf(stderr, "\n\r%d\t%10s \t%16s \t%d blocks",  i, ((type == 0xBB) ? partStr[0]: ((type == 0xDA)? partStr[1]:partStr[2])), 
            locationString[partitionData->partitionElement[i].location], partitionData->partitionElement[i].partitionSize);
    }
    fprintf(stderr, "\n");
}

uint8_t getFirstPartitionForLocation(uint8_t location, CyFx3PartitionData_p pData)
{
    uint8_t i;
    for (i = 0; i < 4; i++)
    {
        if (pData->partitionElement[i].location == location)
        {
            break;
        }
    }
    return i;
}

CyBool CyWbReadFile(uint8_t *fileName, uint8_t **fwImage, uint32_t *fwLen)
{
    FILE *fp;
    int result;  

    fp = fopen((const char *)fileName, "rb");
    if(fp == NULL)
    {
        return CyFalse;
    }

    /* Find the length of the image */
    fseek(fp, 0, SEEK_END);
    *fwLen = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    /* Allocate memory for the image */
    *fwImage = (uint8_t *)malloc (*fwLen);
    if(*fwImage == NULL)
    {
        return CyFalse;
    }

    /* Read into buffer */
    result = fread(*fwImage, *fwLen, 1, fp);
    if(result != 1)
    {
        return CyFalse;      
    }
    fclose(fp);

    return CyTrue;
}

CyBool writeImage(int dev_handle, CyWbFwImage_t *initialFwImage, uint32_t startLba)
{
    CyBool status = CyTrue;;
    uint32_t startAddr;
    uint8_t *fwBuf = initialFwImage->fwImage_p;
    uint32_t fwLen = initialFwImage->fwImageSize;
    
    /* Compute the number of bytes required to store one image (an image starts on a MMC block boundary!) */
    uint32_t imageSize = ((fwLen + CY_WB_LOGICAL_BLOCK_SIZE_IN_BYTES - 1) /
        CY_WB_LOGICAL_BLOCK_SIZE_IN_BYTES) * CY_WB_LOGICAL_BLOCK_SIZE_IN_BYTES;

    uint8_t *fwImage = (uint8_t *)malloc (imageSize);
    if(fwImage == NULL)
    {
        status = CyFalse;
        /* Free up the allocated memory. */
        if (fwBuf)
        {
            free (fwBuf);
            fwBuf = NULL;
        }
        if (fwImage)
        {
            free (fwImage);
            fwImage = NULL;
        }
        return status;
    }

    /* Ensure that the image size is less than that of the partition size */
    if(imageSize > (glPartitionData.partitionElement[glActivePartition].partitionSize*CY_WB_LOGICAL_BLOCK_SIZE_IN_BYTES))
        return CyFalse;

    /* Copy data to fwImage buffer */
    memcpy(fwImage, fwBuf, imageSize);

    /* Convert image size into number of blocks. */
    imageSize /= CY_WB_LOGICAL_BLOCK_SIZE_IN_BYTES;

    /* Write the Image to the storage device. */
    uint32_t numBlks = 128, count = 0;
    for(startAddr = startLba; startAddr < (imageSize + startLba); startAddr += numBlks)
    {
        if((startAddr + numBlks) > (imageSize+startLba))
            count = (imageSize+startLba) - startAddr;
        else
            count = numBlks;

        if((glCmdStatus = CyWbScsi__Write(dev_handle, startAddr, count, &fwImage[startAddr * 512])) != CYWBERR_OK)
        {
            status = CyFalse;
            /* Free up the allocated memory. */
            if(fwBuf)
            {
                free (fwBuf);
                fwBuf = NULL;
            }
            if(fwImage)
            {
                free (fwImage);
                fwImage = NULL;
            }
            return status;
        }
    }

    return status;
}

CyBool CyWbWriteBootImage(uint8_t *fileName, uint32_t userLBA, int devHandle)
{
    CyWbFwImage_t initialFwImage;

    /* Read the contents of the file into buffer */
    if(!CyWbReadFile(fileName, &initialFwImage.fwImage_p, &initialFwImage.fwImageSize))
    {
        /* Error */
        return CyFalse;
    }
    
    if(!initialFwImage.fwImage_p || !initialFwImage.fwImageSize)
    {
        return CyFalse;
    }

    /* Write the firmware image to the boot partition */
    if(!writeImage(devHandle, &initialFwImage, 0))
    {
        return CyFalse;
    }

    return CyTrue;
}

void PrintHelp()
{
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "cyfwstorprog -img:<FW_Image> -dev:<Device_Path> [-bootp:<Boot_Part_Location>]\n");
    fprintf(stderr, "                 [-nuserp:<Num_UserArea_Partitions>] [-psizes:<PartitionSizes>]\n");
    fprintf(stderr, "cyfwstorprog -dev:<Device_Path> -delpart\n");
    fprintf(stderr, "cyfwstorprog -dev:<Device_Path> -disponly\n");
}

int main(int argc, char * argv[])
{
    /* Parse command line parameters */
    if(parseCmdLineParams(argc, argv) != CYWBERR_OK)
    {
        PrintHelp();
        return EXIT_FAILURE;
    }

    /* STEP 1: Open device and check if it is compatible */
    int fd = open(glDevicePath, O_RDWR);
    if (fd > 0)
    {
        INQUIRYDATA inquiryData;
        memset(&inquiryData, 0, sizeof(inquiryData));

        // Send command
        if((glCmdStatus = CyWbScsi__Inquiry(fd, &inquiryData)) != CYWBERR_OK)
        {
            fprintf(stderr, "ERR: Inquiry failed (%d). Aborting.\n\r", glCmdStatus);
            close(fd);
            return EXIT_FAILURE;
        }

        // Check device
        if ((strcmp((const char *)inquiryData.VendorId, "Cypress")) || ((strcmp((const char *)inquiryData.ProductId, "StorageWriter"))))
        {
            fprintf(stderr, "ERR: Device not compatible. Aborting.\n\r");
            close(fd);
            return EXIT_FAILURE;
        }
    }
    else
    {
        fprintf(stderr, "ERR: Failed to open device: %s. Aborting.\n\r", glDevicePath);
        return EXIT_FAILURE;
    }
    fprintf(stderr, "MSG: Device successfully opened: %s\n\r", glDevicePath);

    /* STEP 2: Fetch and save device partition information */
    fprintf(stderr, "MSG: Fetching Device Partition Information... ");
    if ((glCmdStatus = CyFx3sScsi__FetchDeviceInfo(fd, glDataBuffer)) != CYWBERR_OK)
    {
        fprintf(stderr, "\n\rERR: Failed to read device information (%d). Aborting.\n\r", glCmdStatus);
        close(fd);
        return EXIT_FAILURE;
    }
    fprintf(stderr, "Done.\n\r");
    // Save partition data
    populatePartitionInfo(glDataBuffer, &glPartitionData);
    // Display partition data
    displayPartitionInfo(&glPartitionData);

    if(glDisplayPartitions == CyTrue)
    {
        fprintf(stderr, "MSG: Partiton Information Displayed. Please reset device to perform any other operations.\n\r");
        return EXIT_SUCCESS;
    }

    /* STEP 3: Check if required partition exists */
    fprintf(stderr, "MSG: Check if required partition exists (%s)... ", locationString[glBootPartitionLoc]);
    glActivePartition = getFirstPartitionForLocation(glBootPartitionLoc, &glPartitionData);
    if((glPartitionData.numPartitions == 0) || (glActivePartition > 3))
    {
        fprintf(stderr, "\n\rERR: No %s partition found on the device. Aborting.\n\r", locationString[glBootPartitionLoc]);
        close(fd);
        return EXIT_FAILURE;
    }
    fprintf(stderr, "Partition found. Done.\n\r");

    /* STEP 4: Delete existing User partitions */
    uint8_t availUserPartitions = 2;
    uint8_t userPartLoc = getFirstPartitionForLocation(0, &glPartitionData);
    if (glBootPartitionLoc == 0)
        availUserPartitions = (4-(userPartLoc+1));
    else
        availUserPartitions = (4-(userPartLoc));

    if (glNumUserDataPartitions > availUserPartitions)
    {
        fprintf(stderr, "WARNING: Only %d User area partitions will be created.\n\r", availUserPartitions);
        glNumUserDataPartitions = availUserPartitions;
    }

    fprintf(stderr, "MSG: Deleting Existing Partitions in the User area... ");
    if((glCmdStatus = CyFx3sScsi__DeletePartitions(fd, glDataBuffer))!= CYWBERR_OK)
    {
        fprintf(stderr, "\n\rERR: Unable to delete partitions on the storage device (%d). Aborting.\n\r", glCmdStatus);
        close(fd);
        return EXIT_FAILURE;
    }
    fprintf(stderr, "Done.\n\r");

    if ((glCmdStatus = CyFx3sScsi__FetchDeviceInfo(fd, glDataBuffer)) != CYWBERR_OK)
    {
        fprintf(stderr, "ERR: Failed to read device information (%d). Aborting.\n\r", glCmdStatus);
        close(fd);
        return EXIT_FAILURE;
    }
    // Save partition data
    populatePartitionInfo(glDataBuffer, &glPartitionData);
    // Display partition data
    displayPartitionInfo(&glPartitionData);

    /* Exit if DeletePartitions is set*/
    if(glDeletePartitions == CyTrue)
    {
        fprintf(stderr, "MSG: Partitions Deleted. Please reset device to perform any other operations.\n\r");
        return EXIT_SUCCESS;
    }

    /* STEP 5: Create partitions */
    uint32_t userAreaSize = glPartitionData.partitionElement[userPartLoc].partitionSize;
    uint8_t partLoc = 0;
    uint8_t userLoc = 0;
    fprintf(stderr, "MSG: Creating partitions on the device... ");
    // BOOT PARTITION
    if(glBootPartitionLoc == 0)
    {
        if((glCmdStatus = CyFx3sScsi__SetupPartition(fd, partLoc++, 0xBB, 0, 0x2800)) != CYWBERR_OK)
        {
            fprintf(stderr, "\n\rERR: Unable to setup partition of size %d sectors for Boot in User Area (%d). Aborting.\n\r", 0x2800, glCmdStatus);
            close(fd);
            return EXIT_FAILURE;
        }
        userAreaSize -= 0x2800;
    }

    // USER PARTITION(S): Setup all but last partition
    while(glNumUserDataPartitions > 1)
    {
        if (glPartitionSizes[userLoc] == 0)
            glPartitionSizes[userLoc] = userAreaSize / glNumUserDataPartitions;
        if (userAreaSize>glPartitionSizes[userLoc])
            userAreaSize -= glPartitionSizes[userLoc];
        else 
        {
            fprintf(stderr, "\n\rERR: Unable to setup partition %d of size %d sectors in User Area available %d sectors. Aborting.\n\r", partLoc, glPartitionSizes[userLoc], userAreaSize);
        }

        if ((glCmdStatus = CyFx3sScsi__SetupPartition(fd, partLoc++, 0xDA, 0, glPartitionSizes[userLoc++])) != CYWBERR_OK)
        {
            fprintf(stderr, "\n\rERR: Unable to setup partition %d of size %d sectors in User Area (%d). Aborting.\n\r", partLoc - 1, glPartitionSizes[userLoc - 1], glCmdStatus);
            close(fd);
            return EXIT_FAILURE;
        }
        glNumUserDataPartitions--;
    }
    // USER PARTITION
    if ((glCmdStatus = CyFx3sScsi__SetupPartition(fd, partLoc++, 0xDA, 0, 0x00)) != CYWBERR_OK)
    {
        fprintf(stderr, "\n\rERR: Unable to setup Last partition %d of User Area (%d). Aborting.\n\r", partLoc - 1, glCmdStatus);
        close(fd);
        return EXIT_FAILURE;
    }

    memset(glDataBuffer, 0, 512);
    if((glCmdStatus = CyFx3sScsi__CreatePartitions(fd, partLoc, glDataBuffer)) != CYWBERR_OK)
    {
        fprintf(stderr, "\n\rERR: Failed while trying to create partitions (%d). Aborting.\n\r", glCmdStatus);
        close(fd);
        return EXIT_FAILURE;
    }
    fprintf(stderr, "Done.\n\r");
    // Save partition data
    populatePartitionInfo(glDataBuffer, &glPartitionData);
    // Display partition data
    displayPartitionInfo(&glPartitionData);

    /* STEP 6: Set active partition */
    fprintf(stderr, "MSG: Setting active partition (#%d) for Boot Firmware download... ", glActivePartition);
    if((glCmdStatus = CyFx3sScsi__UsePartition(fd, glActivePartition)) != CYWBERR_OK)
    {
        fprintf(stderr, "\n\rERR: Unable to set active partition for programming firmware (%d). Aborting.\n\r", glCmdStatus);
        close(fd);
        return EXIT_FAILURE;
    }
    fprintf(stderr, "Done.\n\r");

    /* STEP 7: Program primary firmware image @ LBA=0x000 */
    fprintf(stderr, "MSG: Programming primary firmware image (%s)... ", glBootImagePath);
    if(!CyWbWriteBootImage((uint8_t*)glBootImagePath, 0, fd))
    {
        fprintf(stderr, "\n\rERR: Programming primary firmware image failed (%d). Aborting.\n\r", glCmdStatus);
        close(fd);
        return EXIT_FAILURE;
    }
    fprintf(stderr, "Done.\n\r");

    /* STEP 8: Program backup firmware image @ LBA=0x400 */
    if(glBootPartitionLoc == 0)
    {
        printf("MSG: Programming backup firmware image... ");
        if( !CyWbWriteBootImage((uint8_t*)glBootImagePath, 0x400, fd))
        {
            fprintf(stderr, "\n\rERR: Programming backup firmware image failed (%d). Aborting.\n\r", glCmdStatus);
            close(fd);
            return EXIT_FAILURE;
        }
        printf("Done.\n\r");
    }

    /* TODO: Add code to write VBP header from here and to verify both the firmware downloaded and the VBP Header */
    
    /* STEP 9: Enable boot partition */
    printf("MSG: Enabling boot partition... ");
    if((glCmdStatus = CyFx3sScsi__EnablePartition(fd)) != CYWBERR_OK)
    {
        fprintf(stderr, "\n\rERR: Failed while trying to enable boot partition (%d). Aborting.\n\r", glCmdStatus);
        close(fd);
        return EXIT_FAILURE;
    }
    printf("Done.\n\r");

    printf("SUCCESS: Programmed Image \"%s\" to Partition %d in the %s.\n\r", glBootImagePath, glActivePartition, locationString[glPartitionData.partitionElement[glActivePartition].location]);
    printf("SUCCESS: Reset device (with PMode[2:0] set to 0) to boot programmed firmware.\n\r");

    // Done: Close device and exit
    close(fd);
    return EXIT_SUCCESS;
}
