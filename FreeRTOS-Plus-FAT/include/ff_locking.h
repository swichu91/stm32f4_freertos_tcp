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

/**
 *	@file		ff_locking.h
 *	@ingroup	LOCKING
 **/

#ifndef _FF_LOCKING_H_
#define	_FF_LOCKING_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

/*---------- PROTOTYPES (in order of appearance). */

/* PUBLIC: */


/* PRIVATE: */
void		FF_PendSemaphore		(void *pSemaphore);
BaseType_t	FF_TrySemaphore         (void *pSemaphore, uint32_t TimeMs);
void		FF_ReleaseSemaphore		(void *pSemaphore);
void		FF_Sleep				(uint32_t TimeMs);

/* Create an event group and bind it to an I/O manager. */
BaseType_t	FF_CreateEvents( FF_IOManager_t *pxIOManager );

/* Get a lock on all DIR operations for a given I/O manager. */
void FF_LockDirectory( FF_IOManager_t *pxIOManager );

/* Release the lock on all DIR operations. */
void FF_UnlockDirectory( FF_IOManager_t *pxIOManager );

/* Get a lock on all FAT operations for a given I/O manager. */
void FF_LockFAT( FF_IOManager_t *pxIOManager );

/* Release the lock on all FAT operations. */
void FF_UnlockFAT( FF_IOManager_t *pxIOManager );

/* Called from FF_GetBuffer() as long as no buffer is available. */
BaseType_t FF_BufferWait( FF_IOManager_t *pxIOManager, uint32_t xWaitMS );

/* Called from FF_ReleaseBuffer(). */
void FF_BufferProceed( FF_IOManager_t *pxIOManager );

/* Check if the current task already has locked the FAT. */
int FF_Has_Lock( FF_IOManager_t *pxIOManager, uint32_t aBits );

/*
 * Throw a configASSERT() in case the FAT has not been locked
 * by this task.
 */
/* _HT_ This function is only necessary while testing. */
void FF_Assert_Lock( FF_IOManager_t *pxIOManager, uint32_t aBits );

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif	/* _FF_LOCKING_H_ */

