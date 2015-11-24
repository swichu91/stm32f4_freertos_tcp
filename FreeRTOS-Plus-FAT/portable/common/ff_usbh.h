/*
 * ff_usbh.h
 *
 *  Created on: 19 lis 2015
 *      Author: Mateusz
 */

#ifndef FF_USBH_H_
#define FF_USBH_H_

#include "stdint.h"


#define HOST_HANDLE 	hUsbHostFS

#define usbhSIGNATURE	0x59505152

#define usbhPARTITION_NUMBER			1 /* Only a single partition is used. */

/* Where the RAM disk is mounted. */
#define mainUSBH_DISK_NAME			"/usb"

#define mainUSBH_CACHE_SIZE_MULTIPLIER		8


FF_Disk_t* usbhDisk;




int32_t FFUSBHRead( uint8_t *pucDestination, /* Destination for data being read. */
                   uint32_t ulSectorNumber, /* Sector from which to start reading data. */
                   uint32_t ulSectorCount,  /* Number of sectors to read. */
                   FF_Disk_t *pxDisk );     /* Describes the disk being read from. */

int32_t FFUSBHWrite( uint8_t *pucSource,      /* Source of data to be written. */
                 uint32_t ulSectorNumber, /* The first sector being written to. */
                 uint32_t ulSectorCount,  /* The number of sectors to write. */
                 FF_Disk_t *pxDisk );     /* Describes the disk being written to. */

void FF_USBH_d(FF_Disk_t *pxDisk );





#endif /* FF_USBH_H_ */
