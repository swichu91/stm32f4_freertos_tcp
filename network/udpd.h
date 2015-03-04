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



void vUDPReceiveDeamon( void *pvParameters );





#endif /* UDPD_H_ */
