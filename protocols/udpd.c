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

#include "ff_headers.h"
#include "ff_stdio.h"



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

const char* head1 = "HTTP/1.1 200 OK\n";
const char* head2 = "Content-length: ";
const char* head3 = "Content-Type: text/html\nKeep-Alive: timeout=15, max=100\nConnection: keep-alive\n\n";

char buffik[200];
char buff[50];

#define BUFFER_SIZE 10000
char cRxedData[ BUFFER_SIZE ];
void prvServerConnectionInstance( void *pvParameters )
{

	xSocket_t xSocket;

	BaseType_t lBytesReceived;
	static const TickType_t keep_alive_time = 15000;

	    /* It is assumed the socket has already been created and connected before
	    being passed into this RTOS task using the RTOS task's parameter. */
	    xSocket = ( xSocket_t ) pvParameters;


	    /* Set a time out so accept() will just wait for a connection. */
	    FreeRTOS_setsockopt( xSocket,
	                         0,
	                         FREERTOS_SO_RCVTIMEO,
	                         &keep_alive_time,
	                         sizeof( keep_alive_time ) );

	    for( ;; )
	    {
	        /* Receive another block of data into the cRxedData buffer. */
	        lBytesReceived = FreeRTOS_recv( xSocket, cRxedData, BUFFER_SIZE, 0 );

	        if( lBytesReceived > 0 )
	        {
	        	if(strncmp(cRxedData,"GET",3)) continue;

	            /* Data was received, process it here. */
	           // FreeRTOS_send(xSocket,cRxedData,lBytesReceived,0);

		        FF_FILE* fd;
		        long lSize, lRead = 0L;
		        uint32_t read_t=0,size_t=0;

	        	memset(cRxedData,0,sizeof(cRxedData));

	           FreeRTOS_send(xSocket,head1, strlen(head1),0);

	           strcat(buffik,head2);

	           /* Open the pxFile specified by the pcFileName parameter. */
	           fd = ff_fopen( "/usb/Picture 012.jpg", "r" );

	           if( fd != NULL )
	           {
	               /* Determine the size of the file. */
	               lSize = 10000000;//ff_filelength( fd );

	               sprintf(buff,"%lu\n",lSize);
	               strcat(buffik,buff);

	               FreeRTOS_send(xSocket,buffik, strlen(buffik),0);
	               FreeRTOS_send(xSocket,head3, strlen(head3),0);

	               while(lRead != lSize)
	               {

	            	   if( (lSize-lRead) > BUFFER_SIZE )
	            	   {
	            		   size_t = BUFFER_SIZE;
	            	   }

	               	   /* Read the entire file. */
	            	   read_t = 10000;//ff_fread( cRxedData, 1, size_t, fd );
	            	   lRead +=read_t;
	               	   FreeRTOS_send(xSocket,cRxedData, read_t,0);

	               }

	               /* Close the file again. */
	               ff_fclose( fd );
	           }


	          // FreeRTOS_send(xSocket,"Content-length: 59\n", 19,0);
	          // FreeRTOS_send(xSocket,"Content-Type: text/html\n", 24,0);
	           //FreeRTOS_send(xSocket,"Keep-Alive: timeout=15, max=100\n", 32,0);
	          // FreeRTOS_send(xSocket,"Connection: keep-alive\n\n", 25,0);
	          // FreeRTOS_send(xSocket,"<html><body><H1>Pozdrowienia dla misiora!</H1></body></html>",59,0);

	            //cRxedData[lBytesReceived] = '\0';

	            //print_console(cRxedData);

	            //char buffer[50];
	        	        //    sprintf(buffer,"Bytes received:%d\r\n",lBytesReceived);
	        	        //    print_console(buffer);


	        }
	        else if( lBytesReceived == 0 )
	        {
	            /* No data was received, but FreeRTOS_recv() did not return an error.
	            Timeout? */
	        	// chwilowo timeout i remote close potraktujemy tka samo
	        	print_console("Remote close\r\n");
	            FreeRTOS_shutdown( xSocket, FREERTOS_SHUT_RDWR );
	            break;
	        }
	        else
	        {
	            /* Error (maybe the connected socket already shut down the socket?).
	            Attempt graceful shutdown. */
	        	print_console("Remote close\r\n");
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


#if( ipconfigUSE_TCP_WIN == 1 )
	WinProperties_t xWinProps;

	/* Fill in the buffer and window sizes that will be used by the socket. */
	xWinProps.lTxBufSize = ipconfigTCP_TX_BUF_LEN;
	xWinProps.lTxWinSize = 1;
	xWinProps.lRxBufSize = ipconfigTCP_RX_BUF_LEN;
	xWinProps.lRxWinSize = 1;
#endif /* ipconfigUSE_TCP_WIN */



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

    /* Set the window and buffer sizes. */
    	#if( ipconfigUSE_TCP_WIN == 1 )
    	{
    		FreeRTOS_setsockopt( xListeningSocket, 0, FREERTOS_SO_WIN_PROPERTIES, ( void * ) &xWinProps, sizeof( xWinProps ) );
    	}
    	#endif /* ipconfigUSE_TCP_WIN */

    /* Set the listening port to 20000. */
    xBindAddress.sin_port = ( uint16_t ) 80;
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
        configASSERT( xConnectedSocket != FREERTOS_INVALID_SOCKET );
        if( xConnectedSocket && xConnectedSocket != FREERTOS_INVALID_SOCKET)
        {
			#if( ipconfigUSE_TCP_WIN == 1 )
        		WinProperties_t xWinProps;

        		/* Fill in the buffer and window sizes that will be used by the socket. */
        		xWinProps.lTxBufSize = ipconfigTCP_TX_BUF_LEN;
        		xWinProps.lTxWinSize = 4;
        		xWinProps.lRxBufSize = ipconfigTCP_RX_BUF_LEN;
        		xWinProps.lRxWinSize = 1;
			#endif /* ipconfigUSE_TCP_WIN */

        	/* Set the window and buffer sizes. */
			#if( ipconfigUSE_TCP_WIN == 1 )
        	{
        		FreeRTOS_setsockopt( xConnectedSocket, 0, FREERTOS_SO_WIN_PROPERTIES, ( void * ) &xWinProps, sizeof( xWinProps ) );
        	}
			#endif /* ipconfigUSE_TCP_WIN */


        	//print_console("connected\r\n");

        /* Spawn a RTOS task to handle the connection. */
                xTaskCreate( prvServerConnectionInstance,
                             "EchoServer",
                             4*configMINIMAL_STACK_SIZE,
                             ( void * ) xConnectedSocket,
                             0,
                             NULL );


        }

    }
}


