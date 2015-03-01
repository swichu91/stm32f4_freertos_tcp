/*
 *	FreeRTOS_Stream_Buffer.h
 *
 *	A cicular character buffer
 *	An implementation of a circular buffer without a length field
 *	If LENGTH defines the size of the buffer, a maximum of (LENGT-1) bytes can be stored
 *	In order to add or read data from the buffer, memcpy() will be called at most 2 times
 */

#ifndef FREERTOS_STREAM_BUFFER_H
#define	FREERTOS_STREAM_BUFFER_H

#if	defined( __cplusplus )
extern "C" {
#endif

typedef struct xSTREAM_BUFFER {
	volatile int32_t lTail;		/* next item to read */
	volatile int32_t lMid;		/* iterator within the valid items */
	volatile int32_t lHead;		/* next position store a new item */
	volatile int32_t lFront;	/* iterator within the free space */
	int32_t LENGTH;				/* const value: number of reserved elements */
	uint8_t ucArray[ sizeof( int32_t ) ];
} xStreamBuffer;

static portINLINE void vStreamBufferClear( xStreamBuffer *pxBuffer )
{
	/* Make the circular buffer empty */
	pxBuffer->lHead = 0;
	pxBuffer->lTail = 0;
	pxBuffer->lFront = 0;
	pxBuffer->lMid = 0;
}
/*-----------------------------------------------------------*/

static portINLINE int32_t lStreamBufferSpace( const xStreamBuffer *pxBuffer, int32_t lLower, int32_t lUpper )
{
/* Returns the space between lLower and lUpper, which equals to the distance minus 1 */
int32_t lCount;

	lCount = pxBuffer->LENGTH + lUpper - lLower - 1;
	if( lCount >= pxBuffer->LENGTH )
	{
		lCount -= pxBuffer->LENGTH;
	}

	return lCount;
}
/*-----------------------------------------------------------*/

static portINLINE int32_t lStreamBufferDistance( const xStreamBuffer *pxBuffer, int32_t lLower, int32_t lUpper )
{
/* Returns the distance between lLower and lUpper */
int32_t lCount;

	lCount = pxBuffer->LENGTH + lUpper - lLower;
	if ( lCount >= pxBuffer->LENGTH )
	{
		lCount -= pxBuffer->LENGTH;
	}

	return lCount;
}
/*-----------------------------------------------------------*/

static portINLINE int32_t lStreamBufferGetSpace( const xStreamBuffer *pxBuffer )
{
	/* Returns the number of items which can still be added to lHead
	before hitting on lTail */
	return lStreamBufferSpace( pxBuffer, pxBuffer->lHead, pxBuffer->lTail );
}
/*-----------------------------------------------------------*/

static portINLINE int32_t lStreamBufferFrontSpace( const xStreamBuffer *pxBuffer )
{
	/* Distance between lFront and lTail
	or the number of items which can still be added to lFront,
	before hitting on lTail */
	return lStreamBufferSpace( pxBuffer, pxBuffer->lFront, pxBuffer->lTail );
}
/*-----------------------------------------------------------*/

static portINLINE int32_t lStreamBufferGetSize( const xStreamBuffer *pxBuffer )
{
	/* Returns the number of items which can be read from lTail
	before reaching lHead */
	return lStreamBufferDistance( pxBuffer, pxBuffer->lTail, pxBuffer->lHead );
}
/*-----------------------------------------------------------*/

static portINLINE int32_t lStreamBufferMidSpace( const xStreamBuffer *pxBuffer )
{
	/* Returns the distance between lHead and lMid */
	return lStreamBufferDistance( pxBuffer, pxBuffer->lMid, pxBuffer->lHead );
}
/*-----------------------------------------------------------*/

static portINLINE void vStreamBufferMoveMid( xStreamBuffer *pxBuffer, int32_t lCount )
{
	/* Increment lMid, but no further than lHead */
	int32_t lSize = lStreamBufferMidSpace( pxBuffer );
	if( lCount > lSize )
	{
		lCount = lSize;
	}
	pxBuffer->lMid += lCount;
	if( pxBuffer->lMid >= pxBuffer->LENGTH )
	{
		pxBuffer->lMid -= pxBuffer->LENGTH;
	}
}
/*-----------------------------------------------------------*/

static portINLINE BaseType_t xStreamBufferIsEmpty( const xStreamBuffer *pxBuffer )
{
BaseType_t xReturn;

	/* True if no item is available */
	if( pxBuffer->lHead == pxBuffer->lTail )
	{
		xReturn = pdTRUE;
	}
	else
	{
		xReturn = pdFALSE;
	}
	return xReturn;
}
/*-----------------------------------------------------------*/

static portINLINE BaseType_t xStreamBufferIsFull( const xStreamBuffer *pxBuffer )
{
	/* True if the available space equals zero. */
	return lStreamBufferGetSpace( pxBuffer ) == 0;
}
/*-----------------------------------------------------------*/

static portINLINE BaseType_t xStreamBufferLessThenEqual( const xStreamBuffer *pxBuffer, int32_t ulLeft, int32_t ulRight )
{
BaseType_t xReturn;
int32_t lTail = pxBuffer->lTail;

	/* Returns true if ( ulLeft < ulRight ) */
	if( ( ulLeft < lTail ) ^ ( ulRight < lTail ) )
	{
		if( ulRight < lTail )
		{
			xReturn = pdTRUE;
		}
		else
		{
			xReturn = pdFALSE;
		}
	}
	else
	{
		if( ulLeft <= ulRight )
		{
			xReturn = pdTRUE;
		}
		else
		{
			xReturn = pdFALSE;
		}
	}
	return xReturn;
}
/*-----------------------------------------------------------*/

static portINLINE int32_t lStreamBufferGetPtr( xStreamBuffer *pxBuffer, uint8_t **ppucData )
{
int32_t lNextTail = pxBuffer->lTail;
int32_t lSize = lStreamBufferGetSize( pxBuffer );

	*ppucData = pxBuffer->ucArray + lNextTail;

	return FreeRTOS_min_int32( lSize, pxBuffer->LENGTH - lNextTail );
}

/*
 * Add bytes to a stream buffer.
 *
 * pxBuffer -	The buffer to which the bytes will be added.
 * lOffset -	If lOffset > 0, data will be written at an offset from lHead
 *				while lHead will not be moved yet.
 * pucData -	A pointer to the data to be added.
 * lCount -		The number of bytes to add.
 */
int32_t lStreamBufferAdd( xStreamBuffer *pxBuffer, int32_t lOffset, const uint8_t *pucData, int32_t lCount );

/*
 * Read bytes from a stream buffer.
 *
 * pxBuffer -	The buffer from which the bytes will be read.
 * lOffset -	Can be used to read data located at a certain offset from 'lTail'.
 * pucData -	A pointer to the buffer into which data will be read.
 * lMaxCount -	The number of bytes to read.
 * xPeek -		If set to pdTRUE the data will remain in the buffer.
 */
int32_t lStreamBufferGet( xStreamBuffer *pxBuffer, int32_t lOffset, uint8_t *pucData, int32_t lMaxCount, BaseType_t xPeek );

#if	defined( __cplusplus )
} /* extern "C" */
#endif

#endif	/* !defined( FREERTOS_STREAM_BUFFER_H ) */
