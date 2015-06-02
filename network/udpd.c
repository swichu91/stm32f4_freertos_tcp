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
#include "udpd.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"
#include "NetworkBufferManagement.h"
#include "FreeRTOS_Sockets.h"

#include "global_db.h"


extern QueueHandle_t CANFramesQueue;


void vUDPTransmitCANFramesTask( void *pvParameters)
{

	xSocket_t xSocket;
	uint8_t *pucBuffer;
	uint8_t *pucBuffer_temp;

	BaseType_t lReturned;

	char cRxedChar;

		//adres ip i port udp bedziemy przekazywac z globalnej struktury
	   //xDestinationAddress.sin_addr = udp_ptr->udpcan_remote_ip;
	  // xDestinationAddress.sin_port = udp_ptr->udpcan_remote_port;

	   /* Create the socket. */
	   xSocket = FreeRTOS_socket( FREERTOS_AF_INET,
	                              FREERTOS_SOCK_DGRAM,/*FREERTOS_SOCK_DGRAM for UDP.*/
	                              FREERTOS_IPPROTO_UDP );

	   /* Check the socket was created. */
	   configASSERT( xSocket != FREERTOS_INVALID_SOCKET );

	   /* NOTE: FreeRTOS_bind() is not called.  This will only work if
	   ipconfigALLOW_SOCKET_SEND_WITHOUT_BIND is set to 1 in FreeRTOSIPConfig.h. */



	   for( ;; )
	   {
	       /* This RTOS task is going to send using the zero copy interface.  The
	       data being sent is therefore written directly into a buffer that is
	       passed into, rather than copied into, the FreeRTOS_sendto()
	       function.*/

		   if((xQueueReceive( CANFramesQueue,  &cRxedChar, portMAX_DELAY) == pdTRUE))
		   {

			   /*First obtain a buffer of adequate length from the TCP/IP stack into which
			 			 	   the string will be written. */

			   pucBuffer = FreeRTOS_GetUDPPayloadBuffer( UDP_CAN_TRANSMIT_BUFFER_SIZE, portMAX_DELAY );
			   /* Check a buffer was obtained. */
			   configASSERT( pucBuffer );
			   pucBuffer_temp = pucBuffer;

			   memset(pucBuffer,0,UDP_CAN_TRANSMIT_BUFFER_SIZE);

			   memcpy(pucBuffer++,&cRxedChar,sizeof(cRxedChar));

			   //dociagnij reszte charow z kolejki
			   while((xQueueReceive( CANFramesQueue,  &cRxedChar, 0) == pdTRUE))
			   {
			   		memcpy(pucBuffer++,&cRxedChar,sizeof(cRxedChar));

			   }
			   memcpy(pucBuffer,'\0',1);

				/* Pass the buffer into the send function.  ulFlags has the
	       	   FREERTOS_ZERO_COPY bit set so the TCP/IP stack will take control of the
	       	   buffer rather than copy data out of the buffer. */
				lReturned = FreeRTOS_sendto( xSocket,
	                                   ( void * ) pucBuffer_temp,
	                                   strlen( ( const char * ) pucBuffer_temp ) + 1,
	                                   FREERTOS_ZERO_COPY,
	                                   &gdb.udp.xDestinationAddress,
	                                   sizeof( gdb.udp.xDestinationAddress ) );

				if( lReturned == 0 )
				{
					/* The send operation failed, so this RTOS task is still responsible
	           	   for the buffer obtained from the TCP/IP stack.  To ensure the buffer
	           	   is not lost it must either be used again, or, as in this case,
	           	   returned to the TCP/IP stack using FreeRTOS_ReleaseUDPPayloadBuffer().
	           	   pucBuffer can be safely re-used after this call. */
					FreeRTOS_ReleaseUDPPayloadBuffer( ( void * ) pucBuffer );
				}
				else
				{
					/* The send was successful so the TCP/IP stack is now managing the
	           	   buffer pointed to by pucBuffer, and the TCP/IP stack will
	           	   return the buffer once it has been sent.  pucBuffer can
	           	   be safely re-used. */
				}


		   }
	   }




}


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

#define BUFFER_SIZE 512
void prvServerConnectionInstance( void *pvParameters )
{

	xSocket_t xSocket;
	static char cRxedData[ BUFFER_SIZE ];
	BaseType_t lBytesReceived;

	    /* It is assumed the socket has already been created and connected before
	    being passed into this RTOS task using the RTOS task's parameter. */
	    xSocket = ( xSocket_t ) pvParameters;

	    for( ;; )
	    {
	        /* Receive another block of data into the cRxedData buffer. */
	        lBytesReceived = FreeRTOS_recv( xSocket, &cRxedData, BUFFER_SIZE, 0 );

	        if( lBytesReceived > 0 )
	        {
	            /* Data was received, process it here. */
	            char buffer[50];
	            sprintf(buffer,"Bytes received:%x\r\n",lBytesReceived);
	            print_console(buffer);
	        }
	        else if( lBytesReceived == 0 )
	        {
	            /* No data was received, but FreeRTOS_recv() did not return an error.
	            Timeout? */
	        }
	        else
	        {
	            /* Error (maybe the connected socket already shut down the socket?).
	            Attempt graceful shutdown. */
	            FreeRTOS_shutdown( xSocket, FREERTOS_SHUT_RDWR );
	            break;
	        }
	    }


	    /* The RTOS task will get here if an error is received on a read.  Ensure the
	        socket has shut down (indicated by FreeRTOS_recv() returning a FREERTOS_EINVAL
	        error before closing the socket). */

	        /* Shutdown is complete and the socket can be safely closed. */
	        FreeRTOS_closesocket( xSocket );

	        /* Must not drop off the end of the RTOS task - delete the RTOS task. */
	        vTaskDelete(NULL);





}


void vCreateTCPServerSocket( void *pvParameters )
{
struct freertos_sockaddr xClient, xBindAddress;
xSocket_t xListeningSocket, xConnectedSocket;
socklen_t xSize = sizeof( xClient );
static const TickType_t xReceiveTimeOut = portMAX_DELAY;
const BaseType_t xBacklog = 20;

    /* Attempt to open the socket. */
    xListeningSocket = FreeRTOS_socket( FREERTOS_AF_INET,
			 	 	 	 	 	 	 	 FREERTOS_SOCK_STREAM,
			 	 	 	 	 	 	 	 FREERTOS_IPPROTO_TCP );

    /* Check the socket was created. */
    configASSERT( xListeningSocket != FREERTOS_INVALID_SOCKET );

    /* If FREERTOS_SO_RCVBUF or FREERTOS_SO_SNDBUF are to be used with
    FreeRTOS_setsockopt() to change the buffer sizes from their default then do
    it here!.  (see the FreeRTOS_setsockopt() documentation. */

    /* If ipconfigUSE_TCP_WIN is set to 1 and FREERTOS_SO_WIN_PROPERTIES is to
    be used with FreeRTOS_setsockopt() to change the sliding window size from
    its default then do it here! (see the FreeRTOS_setsockopt()
    documentation. */

    /* Set a time out so accept() will just wait for a connection. */
    FreeRTOS_setsockopt( xListeningSocket,
                         0,
                         FREERTOS_SO_RCVTIMEO,
                         &xReceiveTimeOut,
                         sizeof( xReceiveTimeOut ) );

    /* Set the listening port to 10000. */
    xBindAddress.sin_port = ( uint16_t ) 10000;
    xBindAddress.sin_port = FreeRTOS_htons( xBindAddress.sin_port );

    /* Bind the socket to the port that the client RTOS task will send to. */
    FreeRTOS_bind( xListeningSocket, &xBindAddress, sizeof( xBindAddress ) );

    /* Set the socket into a listening state so it can accept connections.
    The maximum number of simultaneous connections is limited to 20. */
    FreeRTOS_listen( xListeningSocket, xBacklog );

    for( ;; )
    {
        /* Wait for incoming connections. */
        xConnectedSocket = FreeRTOS_accept( xListeningSocket, &xClient, &xSize );
        //configASSERT( xConnectedSocket != FREERTOS_INVALID_SOCKET );
        if(xConnectedSocket == FREERTOS_INVALID_SOCKET) print_console("Invalid socket\r\n");
        else
        {

        /* Spawn a RTOS task to handle the connection. */
                xTaskCreate( prvServerConnectionInstance,
                             "EchoServer",
                             1024,
                             ( void * ) xConnectedSocket,
                             tskIDLE_PRIORITY,
                             NULL );

        }

    }
}


