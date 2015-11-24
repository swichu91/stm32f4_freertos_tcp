/*
 * ff_usbh.c
 *
 *  Created on: 19 lis 2015
 *      Author: Mateusz
 */

#include "stdint.h"
#include "usbh_msc.h"
#include "usb_host.h"
#include "usbh_core.h"
#include "ff_headers.h"
#include "ff_sys.h"
#include "ff_usbh.h"


BaseType_t FF_USBHDiskDelete( FF_Disk_t *pxDisk )
{
	if( pxDisk != NULL )
	{
		pxDisk->ulSignature = 0;
		pxDisk->xStatus.bIsInitialised = 0;
		if( pxDisk->pxIOManager != NULL )
		{
			FF_DeleteIOManager( pxDisk->pxIOManager );
		}

		vPortFree( pxDisk );
	}

	return pdPASS;
}



int32_t FFUSBHRead( uint8_t *pucDestination, /* Destination for data being read. */
                   uint32_t ulSectorNumber, /* Sector from which to start reading data. */
                   uint32_t ulSectorCount,  /* Number of sectors to read. */
                   FF_Disk_t *pxDisk )     /* Describes the disk being read from. */
{
	int32_t lReturn;
	MSC_LUNTypeDef info;
	USBH_StatusTypeDef  status = USBH_OK;

	if( pxDisk != NULL )
	{

		if( pxDisk->ulSignature != usbhSIGNATURE )
		{
			/* The disk structure is not valid because it doesn't contain a
			magic number written to the disk when it was created. */
			lReturn = FF_ERR_IOMAN_DRIVER_FATAL_ERROR | FF_ERRFLAG;
		}
		else if( ulSectorNumber >= pxDisk->ulNumberOfSectors )
		{
			/* The start sector is not within the bounds of the disk. */
			lReturn = ( FF_ERR_IOMAN_OUT_OF_BOUNDS_WRITE | FF_ERRFLAG );
		}
		else if( ( pxDisk->ulNumberOfSectors - ulSectorNumber ) < ulSectorCount )
		{
			/* The end sector is not within the bounds of the disk. */
			lReturn = ( FF_ERR_IOMAN_OUT_OF_BOUNDS_WRITE | FF_ERRFLAG );
		}
		//else if(!pxDisk->xStatus.bIsInitialised || !pxDisk->xStatus.bIsMounted)
		//{
			//lReturn = ( FF_ERR_IOMAN_DRIVER_NOMEDIUM | FF_ERRFLAG );
		//}
		else
		{
			status = USBH_MSC_Read(&HOST_HANDLE, 0, ulSectorNumber, pucDestination, ulSectorCount);

			if(status == USBH_OK)
			{
				lReturn = FF_ERR_NONE;
			}
			else
			{
				USBH_MSC_GetLUNInfo(&HOST_HANDLE, 0, &info);

				switch (info.sense.asc)
				{
				case SCSI_ASC_LOGICAL_UNIT_NOT_READY:
				case SCSI_ASC_MEDIUM_NOT_PRESENT:
				case SCSI_ASC_NOT_READY_TO_READY_CHANGE:
				  USBH_ErrLog ("USB Disk is not ready!");
				  lReturn = FF_ERR_DEVICE_DRIVER_FAILED | FF_ERRFLAG;
				  break;

				default:
					lReturn = FF_ERR_DEVICE_DRIVER_FAILED | FF_ERRFLAG;
				  break;
				}
			}
		}
	}
	else{
		lReturn = FF_ERR_NULL_POINTER | FF_ERRFLAG;
	}

	return lReturn;

}


int32_t FFUSBHWrite( uint8_t *pucSource,      /* Source of data to be written. */
                 uint32_t ulSectorNumber, /* The first sector being written to. */
                 uint32_t ulSectorCount,  /* The number of sectors to write. */
                 FF_Disk_t *pxDisk )      /* Describes the disk being written to. */
{

	MSC_LUNTypeDef info;
	USBH_StatusTypeDef  status = USBH_OK;
	int32_t lReturn = FF_ERR_NONE;

	if( pxDisk != NULL )
	{
		if( pxDisk->ulSignature != usbhSIGNATURE )
		{
			/* The disk structure is not valid because it doesn't contain a
			magic number written to the disk when it was created. */
			lReturn = FF_ERR_IOMAN_DRIVER_FATAL_ERROR | FF_ERRFLAG;
		}
		else if( pxDisk->xStatus.bIsInitialised == pdFALSE )
		{
			/* The disk has not been initialised. */
			lReturn = FF_ERR_IOMAN_OUT_OF_BOUNDS_WRITE | FF_ERRFLAG;
		}
		else if( ulSectorNumber >= pxDisk->ulNumberOfSectors )
		{
			/* The start sector is not within the bounds of the disk. */
			lReturn = ( FF_ERR_IOMAN_OUT_OF_BOUNDS_WRITE | FF_ERRFLAG );
		}
		else if( ( pxDisk->ulNumberOfSectors - ulSectorNumber ) < ulSectorCount )
		{
			/* The end sector is not within the bounds of the disk. */
			lReturn = ( FF_ERR_IOMAN_OUT_OF_BOUNDS_WRITE | FF_ERRFLAG );
		}
		//else if(!pxDisk->xStatus.bIsInitialised || !pxDisk->xStatus.bIsMounted)
		//{
		//	lReturn = ( FF_ERR_IOMAN_DRIVER_NOMEDIUM | FF_ERRFLAG );
		//}
		else
		{
			status = USBH_MSC_Write(&HOST_HANDLE, 0, ulSectorNumber, pucSource, ulSectorCount);

			if(status == USBH_OK)
			{
				lReturn = FF_ERR_NONE;
			}
			else
			{
				USBH_MSC_GetLUNInfo(&HOST_HANDLE, 0, &info);

				switch (info.sense.asc)
				{
				case SCSI_ASC_LOGICAL_UNIT_NOT_READY:
				case SCSI_ASC_MEDIUM_NOT_PRESENT:
				case SCSI_ASC_NOT_READY_TO_READY_CHANGE:
				  USBH_ErrLog ("USB Disk is not ready!");
				  lReturn = FF_ERR_DEVICE_DRIVER_FAILED | FF_ERRFLAG;
				  break;

				default:
					lReturn = FF_ERR_DEVICE_DRIVER_FAILED | FF_ERRFLAG;
				  break;
				}
			}



		}


	}
	else{
		lReturn = FF_ERR_NULL_POINTER | FF_ERRFLAG;
	}

	return lReturn;
}

static FF_Disk_t *FF_USBHDiskInit( char *pcName)
{

	FF_Error_t xError;
	FF_Disk_t *pxDisk = NULL;
	FF_CreationParameters_t xParameters;
	MSC_LUNTypeDef info;
	USBH_StatusTypeDef ret_val;
	size_t xIOManagerCacheSize;

	ret_val = USBH_MSC_GetLUNInfo(&HOST_HANDLE, 0, &info);

	if(ret_val != USBH_OK) return NULL;

	if(info.capacity.block_size==0 || info.capacity.block_size==0)
		return NULL;

	xIOManagerCacheSize = info.capacity.block_size * mainUSBH_CACHE_SIZE_MULTIPLIER;


    /* Check the validity of the xIOManagerCacheSize parameter. */
    configASSERT( ( xIOManagerCacheSize % info.capacity.block_size ) == 0 );
    configASSERT( ( xIOManagerCacheSize >= ( 2 * info.capacity.block_size ) ) );

    /* Attempt to allocated the FF_Disk_t structure. */
    pxDisk = ( FF_Disk_t * ) pvPortMalloc( sizeof( FF_Disk_t ) );

    if( pxDisk != NULL )
    {
        /* The signature is used by the disk read and disk write functions to
        ensure the disk being accessed is a USB disk. */
        pxDisk->ulSignature = usbhSIGNATURE;

        /* The number of sectors is recorded for bounds checking in the read and
        write functions. */
        pxDisk->ulNumberOfSectors = info.capacity.block_nbr;

        /* Create the IO manager that will be used to control the RAM disk -
        the FF_CreationParameters_t structure completed with the required
        parameters, then passed into the FF_CreateIOManager() function. */
        memset (&xParameters, '\0', sizeof xParameters);
        xParameters.pucCacheMemory = NULL;
        xParameters.ulMemorySize = xIOManagerCacheSize;
        xParameters.ulSectorSize = info.capacity.block_size;
        xParameters.fnWriteBlocks = FFUSBHWrite;
        xParameters.fnReadBlocks = FFUSBHRead;
        xParameters.pxDisk = pxDisk;

        xParameters.pvSemaphore = ( void * ) xSemaphoreCreateRecursiveMutex();
        xParameters.xBlockDeviceIsReentrant = pdTRUE; // tego nie jestem pewien czy procedura moze byc przewrwana

        pxDisk->pxIOManager = FF_CreateIOManger( &xParameters, &xError );

        if( ( pxDisk->pxIOManager != NULL ) && ( FF_isERR( xError ) == pdFALSE ) )
        {
            /* Record that the RAM disk has been initialised. */
            pxDisk->xStatus.bIsInitialised = pdTRUE;


            /* Mount the partition. */
            xError = FF_Mount( pxDisk, 0 );

            if( FF_isERR( xError ) == pdFALSE )
            {
                /* The partition mounted successfully, add it to the virtual
                file system - where it will appear as a directory off the file
                system's root directory. */
                FF_FS_Add( pcName, pxDisk);
            }

            /* Create a partition on the RAM disk.  NOTE!  The disk is only
            being partitioned here because it is a new RAM disk.  It is
            known that the disk has not been used before, and cannot already
            contain any partitions.  Most media drivers will not perform
            this step because the media will already been partitioned and
            formatted. */
            //xError = prvPartitionAndFormatDisk( pxDisk );

            //if( FF_isERR( xError ) == pdFALSE )
           // {
                /* Record the partition number the FF_Disk_t structure is, then
                mount the partition. */
              //  pxDisk->xStatus.bPartitionNumber = usbhPARTITION_NUMBER;

                /* Mount the partition. */
              //  xError = FF_Mount( pxDisk, usbhPARTITION_NUMBER );
           // }

           // if( FF_isERR( xError ) == pdFALSE )
           // {
                /* The partition mounted successfully, add it to the virtual
                file system - where it will appear as a directory off the file
                system's root directory. */
              //  FF_FS_Add( pcName, pxDisk->pxIOManager );
           // }
        }
        else
        {
            /* The disk structure was allocated, but the disk's IO manager could
            not be allocated, so free the disk again. */
        	FF_USBHDiskDelete( pxDisk );
            pxDisk = NULL;
        }

    }

    return pxDisk;
}

void FF_USBH_d(FF_Disk_t *pxDisk )
{

	static uint8_t is_connected_t;

	if(usbhDisk == NULL)
		usbhDisk = FF_USBHDiskInit( mainUSBH_DISK_NAME);

	if(is_connected_t != HOST_HANDLE.device.is_connected)
	{
		is_connected_t = HOST_HANDLE.device.is_connected;
		if(HOST_HANDLE.device.is_connected == 0) // wyjeto pendrive
		{
			FF_USBHDiskDelete(pxDisk);
			memset(usbhDisk,0,sizeof(usbhDisk));
			usbhDisk = NULL;
		}
	}

}



