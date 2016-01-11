/*
 * can.c
 *
 *  Created on: 17 maj 2015
 *      Author: Mateusz
 */


#include "can.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>
#include "console.h"



CAN_HandleTypeDef can_conf;
CanRxMsgTypeDef Rx_can_struct;
CanTxMsgTypeDef Tx_can_struct;



SemaphoreHandle_t ReceiveCANFrameSemaphore;
QueueHandle_t CANFramesQueue;

//function protos:
static void putCANframe(CAN_HandleTypeDef* hcan);

void CAN1_RX0_IRQHandler()
{
	GPIOD->ODR ^=GPIO_ODR_ODR_13; // blink orange diode on receiving can frame

	HAL_CAN_IRQHandler(&can_conf);
}

void CAN1_SCE_IRQHandler()
{
	HAL_CAN_IRQHandler(&can_conf);
}


void HAL_CAN_RxCpltCallback(CAN_HandleTypeDef* hcan)
{
	HAL_CAN_Receive_IT(&can_conf,CAN_FIFO0);

	putCANframe(hcan);
}

void HAL_CAN_ErrorCallback(CAN_HandleTypeDef* hcan)
{
	char buffer[50];

	sprintf(buffer,"Error:%x\r\n",hcan->ErrorCode);
	print_console(buffer);
}


static void putCANframe(CAN_HandleTypeDef* hcan)
{

	long lHigherPriorityTaskWoken = pdFALSE;

	char buffer[200];
	char* bf_ptr = buffer;
	char databuffer[10];
	uint8_t i;

	sprintf(buffer,"Frame START: ID:%d TIDE:%d RTR:%d FLENGTH:%d DATA:0x",(int)hcan->pRxMsg->StdId,(int)hcan->pRxMsg->IDE,(int)hcan->pRxMsg->RTR,(int)hcan->pRxMsg->DLC);

	for(i=0;i<hcan->pRxMsg->DLC;i++)
	{
		sprintf(databuffer,"%x",(unsigned int)hcan->pRxMsg->Data[i]);
		strcat(buffer,databuffer);
	}
	strcat(buffer," Frame END");

    for(; *bf_ptr!= 0; bf_ptr++)
	{
    	xQueueSendFromISR( CANFramesQueue, ( void * ) bf_ptr, &lHigherPriorityTaskWoken );
	}

    if(lHigherPriorityTaskWoken == pdTRUE) portEND_SWITCHING_ISR( lHigherPriorityTaskWoken );

}




void HAL_CAN_MspInit(CAN_HandleTypeDef* hcan)
{

	RCC->APB1ENR |=RCC_APB1ENR_CAN1EN | RCC_APB1ENR_CAN2EN;
	RCC->AHB1ENR |=RCC_AHB1ENR_GPIODEN;

	GPIO_InitTypeDef RxCAN_pin,TxCAN_pin;

	RxCAN_pin.Pin = GPIO_PIN_0;
	RxCAN_pin.Pull = GPIO_NOPULL;
	RxCAN_pin.Mode = GPIO_MODE_AF_PP;
	RxCAN_pin.Alternate = GPIO_AF9_CAN1;
	RxCAN_pin.Speed = GPIO_SPEED_FREQ_LOW;

	TxCAN_pin.Pin = GPIO_PIN_1;
	TxCAN_pin.Speed = GPIO_SPEED_FREQ_LOW;
	TxCAN_pin.Mode = GPIO_MODE_AF_PP;
	TxCAN_pin.Pull = GPIO_NOPULL;
	TxCAN_pin.Alternate = GPIO_AF9_CAN1;

	HAL_GPIO_Init(GPIOD,&RxCAN_pin);
	HAL_GPIO_Init(GPIOD,&TxCAN_pin);



	//interrupts
	NVIC_SetPriority(CAN1_RX0_IRQn,CAN_RX_FIFO0_PRIO); // fifo 0 receive interrupt
	//NVIC_SetPriority(CAN1_RX1_IRQn,CAN_RX_FIFO1_PRIO); // fifo 1 receive intterupt
	//NVIC_SetPriority(CAN1_TX_IRQn,CAN_TX_PRIO);			// transmit interrupt
	NVIC_SetPriority(CAN1_SCE_IRQn,CAN_SCE_PRIO);		//error and status change interrupt

	NVIC_EnableIRQ(CAN1_RX0_IRQn);
	//NVIC_EnableIRQ(CAN1_RX1_IRQn);
	//NVIC_EnableIRQ(CAN1_TX_IRQn);
	NVIC_EnableIRQ(CAN1_SCE_IRQn);


}







void can_init(uint32_t queue_size)
{

	CAN_FilterConfTypeDef can_conf_filter;

	CANFramesQueue = xQueueCreate(queue_size, sizeof( portCHAR ) );


	can_conf.Init.Mode = CAN_MODE_NORMAL;
	can_conf.Instance = CAN1;
	can_conf.Init.Prescaler = 16; // 125kbps
	can_conf.Init.SJW = CAN_SJW_1TQ;
	can_conf.Init.BS1 = CAN_BS1_14TQ;
	can_conf.Init.BS2 = CAN_BS2_6TQ;

	can_conf.Init.TTCM = DISABLE;
	can_conf.Init.ABOM = DISABLE;
	can_conf.Init.AWUM = DISABLE;
	can_conf.Init.NART = DISABLE;
	can_conf.Init.RFLM = ENABLE;
	can_conf.Init.TXFP = DISABLE;

	can_conf.pRxMsg =&Rx_can_struct;
	can_conf.pTxMsg =&Tx_can_struct;

	HAL_CAN_Init(&can_conf);

	if(can_conf.State == HAL_CAN_STATE_READY) print_console("CAN init -> OK\r\n");

	else print_console("CAN init failed\r\n");

	can_conf_filter.FilterNumber=0;
	can_conf_filter.FilterMode=CAN_FILTERMODE_IDMASK;
	can_conf_filter.FilterScale=CAN_FILTERSCALE_16BIT;
	can_conf_filter.FilterIdHigh=0x0000;
	can_conf_filter.FilterIdLow=0x0000;
	can_conf_filter.FilterMaskIdHigh=0x0000;
	can_conf_filter.FilterMaskIdLow=0x0000;
	can_conf_filter.FilterFIFOAssignment=CAN_FIFO0;
	can_conf_filter.FilterActivation=ENABLE;

	HAL_CAN_ConfigFilter(&can_conf,&can_conf_filter);

	HAL_CAN_Receive_IT(&can_conf,CAN_FIFO0);


}






