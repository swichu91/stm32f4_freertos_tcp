/*
 * FreeRTOS+FAT Labs Build 150825 (C) 2015 Real Time Engineers ltd.
 * Authors include James Walmsley, Hein Tibosch and Richard Barry
 *
 *******************************************************************************
 ***** NOTE ******* NOTE ******* NOTE ******* NOTE ******* NOTE ******* NOTE ***
 ***                                                                         ***
 ***                                                                         ***
 ***   FREERTOS+FAT IS STILL IN THE LAB:                                     ***
 ***                                                                         ***
 ***   This product is functional and is already being used in commercial    ***
 ***   products.  Be aware however that we are still refining its design,    ***
 ***   the source code does not yet fully conform to the strict coding and   ***
 ***   style standards mandated by Real Time Engineers ltd., and the         ***
 ***   documentation and testing is not necessarily complete.                ***
 ***                                                                         ***
 ***   PLEASE REPORT EXPERIENCES USING THE SUPPORT RESOURCES FOUND ON THE    ***
 ***   URL: http://www.FreeRTOS.org/contact  Active early adopters may, at   ***
 ***   the sole discretion of Real Time Engineers Ltd., be offered versions  ***
 ***   under a license other than that described below.                      ***
 ***                                                                         ***
 ***                                                                         ***
 ***** NOTE ******* NOTE ******* NOTE ******* NOTE ******* NOTE ******* NOTE ***
 *******************************************************************************
 *
 * - Open source licensing -
 * While FreeRTOS+FAT is in the lab it is provided only under version two of the
 * GNU General Public License (GPL) (which is different to the standard FreeRTOS
 * license).  FreeRTOS+FAT is free to download, use and distribute under the
 * terms of that license provided the copyright notice and this text are not
 * altered or removed from the source files.  The GPL V2 text is available on
 * the gnu.org web site, and on the following
 * URL: http://www.FreeRTOS.org/gpl-2.0.txt.  Active early adopters may, and
 * solely at the discretion of Real Time Engineers Ltd., be offered versions
 * under a license other then the GPL.
 *
 * FreeRTOS+FAT is distributed in the hope that it will be useful.  You cannot
 * use FreeRTOS+FAT unless you agree that you use the software 'as is'.
 * FreeRTOS+FAT is provided WITHOUT ANY WARRANTY; without even the implied
 * warranties of NON-INFRINGEMENT, MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. Real Time Engineers Ltd. disclaims all conditions and terms, be they
 * implied, expressed, or statutory.
 *
 * 1 tab == 4 spaces!
 *
 * http://www.FreeRTOS.org
 * http://www.FreeRTOS.org/plus
 * http://www.FreeRTOS.org/labs
 *
 */

#ifndef __SDDISK_H__

#define __SDDISK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ff_headers.h"

/* Create a RAM disk, supplying enough memory to hold N sectors of 512 bytes each */
FF_Disk_t *FF_SDDiskInit( char *pcName );

BaseType_t FF_SDDiskReinit( FF_Disk_t *pxDisk );

/* Unmount the volume */
BaseType_t FF_SDDiskUnmount( FF_Disk_t *pDisk );

/* Mount the volume */
BaseType_t FF_SDDiskMount( FF_Disk_t *pDisk );

/* Release all resources */
BaseType_t FF_SDDiskDelete( FF_Disk_t *pDisk );

/* Show some partition information */
BaseType_t FF_SDDiskShowPartition( FF_Disk_t *pDisk );

/* Flush changes from the driver's buf to disk */
void FF_SDDiskFlush( FF_Disk_t *pDisk );

/* Format a given partition on an SD-card. */
BaseType_t FF_SDDiskFormat( FF_Disk_t *pxDisk, BaseType_t aPart );

/* Return non-zero if an SD-card is detected in a given slot. */
BaseType_t FF_SDDiskInserted( BaseType_t xDriveNr );

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __SDDISK_H__ */
