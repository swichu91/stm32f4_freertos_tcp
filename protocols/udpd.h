/*
 * udpd.h
 *
 *  Created on: 2 mar 2015
 *      Author: Mateusz
 */

#ifndef UDPD_H_
#define UDPD_H_


#define UDPD_PRIO		2
#define UDPD_STACK_SIZE	 (configMINIMAL_STACK_SIZE * 2)

#define UDP_CAN_TX_PRIO	2
#define UDPD_CAN_TX_SIZE	1024


#define UDP_CAN_TRANSMIT_BUFFER_SIZE		1024

#define UDP_CAN_TRANSMIT_DEFAULT_PORT	10000



void vUDPReceiveDeamon( void *pvParameters );
void vCreateTCPServerSocket( void *pvParameters );
void vUDPTransmitCANFramesTask( void *pvParameters);




#endif /* UDPD_H_ */
