/*
 * can.h
 *
 *  Created on: 17 maj 2015
 *      Author: Mateusz
 */

#ifndef CAN_H_
#define CAN_H_

#include <stdint.h>



// interrupts priorities
#define CAN_RX_FIFO0_PRIO		7
#define CAN_RX_FIFO1_PRIO		7
#define CAN_TX_PRIO				8
#define	CAN_SCE_PRIO			7












void can_init(uint32_t queue_size);
void CANRxTask(void *pvParameters);


#endif /* CAN_H_ */
