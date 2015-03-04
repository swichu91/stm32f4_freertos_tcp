/*
 * udpd.c

 *
 *  Created on: 2 mar 2015
 *      Author: Mateusz
 */

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdint-gcc.h>
#include <stdio.h>
#include "console.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"
#include "NetworkBufferManagement.h"


void vUDPReceiveDeamon( void *pvParameters )
{
long lBytes;
uint8_t *pucBuffer;
struct freertos_sockaddr xClient, xBindAddress;
uint32_t xClientLength = sizeof( xClient );
xSocket_t xListeningSocket;

BaseType_t lReturned;

   /* Attempt to open the socket. */
   xListeningSocket = FreeRTOS_socket( FREERTOS_AF_INET,
                                       FREERTOS_SOCK_DGRAM,/*FREERTOS_SOCK_DGRAM for UDP.*/
                                       FREERTOS_IPPROTO_UDP );

   /* Check the socket was created. */
   configASSERT( xListeningSocket != FREERTOS_INVALID_SOCKET );

   /* Bind to port 10000. */
   xBindAddress.sin_port = FreeRTOS_htons( 10000 );
   FreeRTOS_bind( xListeningSocket, &xBindAddress, sizeof( xBindAddress ) );

   for( ;; )
   {
       /* Receive data from the socket.  ulFlags is zero, so the standard
       interface is used.  By default the block time is portMAX_DELAY, but it
       can be changed using FreeRTOS_setsockopt(). */
       lBytes = FreeRTOS_recvfrom( xListeningSocket,
                                   &pucBuffer,
                                   0,
                                   FREERTOS_ZERO_COPY,
                                   &xClient,
                                   &xClientLength );

       if( lBytes > 0 )
       {
    	   lReturned= FreeRTOS_sendto( xListeningSocket,
    			   	   	   	    ( void * ) pucBuffer,
    			   	   	   	   	strlen(( const char * )  pucBuffer ) + 1 ,
    			   	   	   	   	FREERTOS_ZERO_COPY,
    	                        &xClient,
    	                        sizeof( xClient ) );

    	   if(lReturned == 0)
    	   {
    		   FreeRTOS_ReleaseUDPPayloadBuffer( ( void * ) pucBuffer );
    	   }

       }
   }
}

