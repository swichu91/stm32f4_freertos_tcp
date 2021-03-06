/*
 * NetworkInterface.c
 *
 *  Created on: 6 lut 2015
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

/* Thread-safe circular buffers are being used to pass data to and from the PCAP
 access functions. */
#include "FreeRTOS_Stream_Buffer.h"

//#include "stm32f4xx_hal_eth.h"
#include "stm32f4xx_hal.h"

#include "udpd.h"
#include "global_db.h"
#include "sntp.h"
#include "FreeRTOS_server_private.h"

/* Declarations */
static void prvEMACDeferredInterruptHandlerTask(void *pvParameters);
static void probePHY(ETH_HandleTypeDef * heth);

/* Global Ethernet handle*/
ETH_HandleTypeDef heth_global;

uint32_t ulPHYLinkStatus;

extern uint8_t ucMACAddress[6];

#ifndef	BMSR_LINK_STATUS
#define BMSR_LINK_STATUS            0x0004UL
#endif

/* Private variables ---------------------------------------------------------*/

__ALIGN_BEGIN ETH_DMADescTypeDef DMARxDscrTab[ETH_RXBUFNB] __ALIGN_END;/* Ethernet Rx MA Descriptor */

__ALIGN_BEGIN ETH_DMADescTypeDef DMATxDscrTab[ETH_TXBUFNB] __ALIGN_END;/* Ethernet Tx DMA Descriptor */

__ALIGN_BEGIN uint8_t Rx_Buff[ETH_RXBUFNB][ETH_RX_BUF_SIZE] __ALIGN_END; /* Ethernet Receive Buffer */

__ALIGN_BEGIN uint8_t Tx_Buff[ETH_TXBUFNB][ETH_TX_BUF_SIZE] __ALIGN_END; /* Ethernet Transmit Buffer */

static SemaphoreHandle_t xEMACRxEventSemaphore;

static void InitializeZeroCopyBuffersRx(uint8_t buff_nr, uint8_t **buff_tab) {
	uint8_t i;

	size_t size = ETH_RX_BUF_SIZE;

	for (i = 0; i < buff_nr; i++) {
		*buff_tab = pucGetNetworkBuffer(&size);
		buff_tab++;
	}

}

static void InitializeZeroCopyBuffersTx(uint8_t buff_nr, ETH_DMADescTypeDef *DMATxDescTab) {
	uint8_t i;

	for (i = 0; i < buff_nr; i++) {
		DMATxDescTab[i].Buffer1Addr = (uint32_t) NULL;
	}

}

static void FillZeroCopyBuffersRx(uint8_t buff_nr, ETH_DMADescTypeDef * DMARxDesc,
		uint8_t** zerocopy_tab) {

	uint8_t i;

	/* Fill each DMARxDesc descriptor buffer1 with the right values */
	for (i = 0; i < buff_nr; i++) {
		DMARxDesc[i].Buffer1Addr = (uint32_t) zerocopy_tab[i];
		DMARxDesc[i].ControlBufferSize = ETH_DMARXDESC_RCH | (ETH_RX_BUF_SIZE);
	}

}

/*-----------------------------------------------------------*/

void ETH_IRQHandler(void) {

	GPIOD->ODR ^= GPIO_ODR_ODR_15; // toggle blue diode

	HAL_ETH_IRQHandler(&heth_global);

}

/*-----------------------------------------------------------*/

void HAL_ETH_RxCpltCallback(ETH_HandleTypeDef *heth) {
	long lHigherPriorityTaskWoken = pdFALSE;

	//TODO: sprawdzaj juz tutaj czy odebrana ramka nadaje sie do przetwarzania

	xSemaphoreGiveFromISR(xEMACRxEventSemaphore, &lHigherPriorityTaskWoken);

	if (lHigherPriorityTaskWoken == pdTRUE)
		portEND_SWITCHING_ISR(lHigherPriorityTaskWoken);

}

/*-----------------------------------------------------------*/

void HAL_ETH_ErrorCallback(ETH_HandleTypeDef *heth) {
	print_gen("MAC error!");
}

/*-----------------------------------------------------------*/

void HAL_ETH_TxCpltCallback(ETH_HandleTypeDef *heth) {
	/* This call-back would only be useful in case packets are being sent
	 zero-copy.  Once they're sent, the buffers must be released. */
}

void HAL_ETH_MspInit(ETH_HandleTypeDef* heth) {
	GPIO_InitTypeDef GPIO_InitStruct;
	if (heth->Instance == ETH) {
		/* USER CODE BEGIN ETH_MspInit 0 */

		/* USER CODE END ETH_MspInit 0 */
		/* Peripheral clock enable */
		__ETH_CLK_ENABLE();

		//enable gpio clocks
		RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIOBEN;

		/**ETH GPIO Configuration
		 PC1     ------> ETH_MDC
		 PA1     ------> ETH_REF_CLK
		 PA2     ------> ETH_MDIO
		 PA7     ------> ETH_CRS_DV
		 PC4     ------> ETH_RXD0
		 PC5     ------> ETH_RXD1
		 PB11     ------> ETH_TX_EN
		 PB12     ------> ETH_TXD0
		 PB13     ------> ETH_TXD1
		 */
		GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_7;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

		HAL_NVIC_SetPriority(ETH_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
		HAL_NVIC_EnableIRQ(ETH_IRQn);

	}
}

/*-----------------------------------------------------------*/

static void probePHY(ETH_HandleTypeDef * heth) {

	uint32_t ulRegValue = 0;

	/* Read the result of the auto-negotiation. */
	HAL_ETH_ReadPHYRegister(heth, PHY_SR, &ulRegValue);

	if (heth->Init.AutoNegotiation != ETH_AUTONEGOTIATION_DISABLE) {

		/* Read the result of the auto-negotiation. */
		HAL_ETH_ReadPHYRegister(heth, PHY_SR, &ulRegValue);
		if ((ulRegValue & PHY_LINK_STATUS) != 0) {
			ulPHYLinkStatus |= BMSR_LINK_STATUS;
		}
		else {
			ulPHYLinkStatus &= ~( BMSR_LINK_STATUS);
		}

		FreeRTOS_printf( ( ">> Autonego ready: %08x: %s duplex %u mbit %s status\n",
			(unsigned int)ulRegValue,
			(ulRegValue & PHY_DUPLEX_STATUS) ? "full" : "half",
			(ulRegValue & PHY_SPEED_STATUS) ? 10 : 100,
			(ulRegValue & PHY_LINK_STATUS) ? "high" : "low" ) );
	}

}

/*-----------------------------------------------------------*/

// configure rj45 leds
static void setPHYleds(ETH_HandleTypeDef * heth) {
	uint32_t regvalue;

	HAL_ETH_ReadPHYRegister(heth, PHY_CR, &regvalue);

	regvalue &= ~((1 << 5) | (1 << 6)); // mode 3 led -> orange->LINK , green -> FD/HD
	regvalue |= (1 << 6);

	HAL_ETH_WritePHYRegister(heth, PHY_CR, regvalue);

}

/*-----------------------------------------------------------*/

BaseType_t xNetworkInterfaceInitialise(void) {

	HAL_StatusTypeDef hal_eth_init_status;
	uint8_t status = 0;

	/* Init ETH */

	heth_global.Instance = ETH;
	heth_global.Init.AutoNegotiation = ETH_AUTONEGOTIATION_ENABLE;
	heth_global.Init.Speed = ETH_SPEED_100M;
	heth_global.Init.DuplexMode = ETH_MODE_FULLDUPLEX;
	heth_global.Init.PhyAddress = DP83848_PHY_ADDRESS;

	heth_global.Init.MACAddr = &ucMACAddress[0];
	heth_global.Init.RxMode = ETH_RXINTERRUPT_MODE;
#if (ipconfigDRIVER_INCLUDED_RX_IP_CHECKSUM == 1) && (ipconfigDRIVER_INCLUDED_TX_IP_CHECKSUM == 1)
	heth_global.Init.ChecksumMode = ETH_CHECKSUM_BY_HARDWARE;
#else
	heth_global.Init.ChecksumMode = ETH_CHECKSUM_BY_SOFTWARE;
#endif

	heth_global.Init.MediaInterface = ETH_MEDIA_INTERFACE_RMII;

	hal_eth_init_status = HAL_ETH_Init(&heth_global);

	if (hal_eth_init_status == HAL_OK) {
		print_console(">> MAC init OK\r\n");
		status = pdPASS;
	}
	else {
		return pdFAIL;
		print_console(">> MAC init failed\r\n");
	}

	probePHY(&heth_global);

	/* Initialize Tx Descriptors list: Chain Mode */
	HAL_ETH_DMATxDescListInit(&heth_global, DMATxDscrTab, &Tx_Buff[0][0],
	ETH_TXBUFNB);
	//HAL_ETH_DMATxDescListInit(&heth_global, DMATxDscrTab, NULL, ETH_TXBUFNB);

	//InitializeZeroCopyBuffersTx(ETH_TXBUFNB,DMATxDscrTab);

	/* Initialize zero copy buffers */
	//uint8_t* zerocopy_bufor_tab[ETH_RXBUFNB]; //  tab to hold pointers to zero copy bufors
	/* Initialize Rx Descriptors list: Chain Mode  */
	HAL_ETH_DMARxDescListInit(&heth_global, DMARxDscrTab, &Rx_Buff[0][0],
	ETH_RXBUFNB);

	/* Enable MAC and DMA transmission and reception */
	HAL_ETH_Start(&heth_global);

	setPHYleds(&heth_global);

#if PHY_LINK_CHANGE_INT

	uint32_t regvalue = 0;

	/**** Configure PHY to generate an interrupt when Eth Link state changes ****/
	/* Read Register Configuration */
	HAL_ETH_ReadPHYRegister(&heth_global, PHY_MICR, &regvalue);

	regvalue |= (PHY_MICR_INT_EN | PHY_MICR_INT_OE);

	/* Enable Interrupts */
	HAL_ETH_WritePHYRegister(&heth_global, PHY_MICR, regvalue );

	/* Read Register Configuration */
	HAL_ETH_ReadPHYRegister(&heth_global, PHY_MISR, &regvalue);

	regvalue |= PHY_MISR_LINK_INT_EN;

	/* Enable Interrupt on change of link status */
	HAL_ETH_WritePHYRegister(&heth_global, PHY_MISR, regvalue);

#endif

	xTaskCreate(prvEMACDeferredInterruptHandlerTask, "EthRcvTask", configMINIMAL_STACK_SIZE, NULL,
			configMAX_PRIORITIES-1, NULL);

	return status;

}

/*-----------------------------------------------------------*/

BaseType_t xNetworkInterfaceOutput(
xNetworkBufferDescriptor_t * const pxDescriptor, BaseType_t xReleaseAfterSend) {

	uint8_t *buffer = (uint8_t *) (heth_global.TxDesc->Buffer1Addr);
	__IO ETH_DMADescTypeDef *DmaTxDesc;
	uint32_t framelength = 0;
	uint32_t byteslefttocopy = 0;
	uint32_t payloadoffset = 0;
	DmaTxDesc = heth_global.TxDesc;

	uint8_t errval;

	/* copy frame from pbufs to driver buffers */

	/* Is this buffer available? If not, goto error */
	if ((DmaTxDesc->Status & ETH_DMATXDESC_OWN) != (uint32_t) RESET) {
		errval = pdFAIL;
		goto error;
	}

#if( ipconfigDRIVER_INCLUDED_TX_IP_CHECKSUM != 0 )
	{
		ProtocolPacket_t *pxPacket;

		/* If the peripheral must calculate the checksum, it wants
		 the protocol checksum to have a value of zero. */
		pxPacket = (ProtocolPacket_t *) (pxDescriptor->pucEthernetBuffer);

		if (pxPacket->xICMPPacket.xIPHeader.ucProtocol == ipPROTOCOL_ICMP) {
			pxPacket->xICMPPacket.xICMPHeader.usChecksum = (uint16_t) 0u;
		}
	}
#endif

	/* Get bytes in current lwIP buffer */
	byteslefttocopy = pxDescriptor->xDataLength;
	framelength = byteslefttocopy; // copy of frame length
	payloadoffset = 0;

	/* Check if the length of data to copy is bigger than Tx buffer size*/
	while ((byteslefttocopy) > ETH_TX_BUF_SIZE) {
		/* Copy data to Tx buffer*/
		memcpy((uint8_t*) ((uint8_t*) buffer),
				(uint8_t*) ((uint8_t*) pxDescriptor->pucEthernetBuffer + payloadoffset),
				(ETH_TX_BUF_SIZE));

		/* Point to next descriptor */
		DmaTxDesc = (ETH_DMADescTypeDef *) (DmaTxDesc->Buffer2NextDescAddr);

		/* Check if the buffer is available */
		if ((DmaTxDesc->Status & ETH_DMATXDESC_OWN) != (uint32_t) RESET) {
			errval = pdFAIL;
			goto error;
		}

		// memcpy( (uint8_t*)((uint8_t*)buffer), (uint8_t*)((uint8_t*)pxDescriptor->pucEthernetBuffer + payloadoffset), byteslefttocopy );
		buffer = (uint8_t *) (DmaTxDesc->Buffer1Addr);

		byteslefttocopy -= (ETH_TX_BUF_SIZE);
		payloadoffset += (ETH_TX_BUF_SIZE);

	}

	/* Copy the remaining bytes */
	memcpy((uint8_t*) ((uint8_t*) buffer),
			(uint8_t*) ((uint8_t*) pxDescriptor->pucEthernetBuffer + payloadoffset),
			byteslefttocopy);

	/* Prepare transmit descriptors to give to DMA */
	HAL_ETH_TransmitFrame(&heth_global, framelength);

	gdb.monit.tx_eth_frames++;

	/* Call the standard trace macro to log the send event. */
	iptraceNETWORK_INTERFACE_TRANSMIT();

	errval = pdPASS;

	error:

	if (xReleaseAfterSend != pdFALSE) {
		/* It is assumed SendData() copies the data out of the FreeRTOS+TCP Ethernet
		 buffer.  The Ethernet buffer is therefore no longer needed, and must be
		 freed for re-use. */
		vReleaseNetworkBufferAndDescriptor(pxDescriptor);
	}

	/* When Transmit Underflow flag is set, clear it and issue a Transmit Poll Demand to resume transmission */
	if ((heth_global.Instance->DMASR & ETH_DMASR_TUS) != (uint32_t) RESET) {
		/* Clear TUS ETHERNET DMA flag */
		heth_global.Instance->DMASR = ETH_DMASR_TUS;

		/* Resume DMA transmission*/
		heth_global.Instance->DMATPDR = 0;
	}

	return errval;

}

/*-----------------------------------------------------------*/

/* The deferred interrupt handler is a standard RTOS task.  FreeRTOS's centralised
 deferred interrupt handling capabilities can also be used. */
static void prvEMACDeferredInterruptHandlerTask(void *pvParameters) {
	xNetworkBufferDescriptor_t *pxBufferDescriptor;
	size_t xBytesReceived;
	/* Used to indicate that xSendEventStructToIPTask() is being called because
	 of an Ethernet receive event. */
	xIPStackEvent_t xRxEvent;

	uint8_t *buffer;
	__IO ETH_DMADescTypeDef *dmarxdesc;
	uint32_t payloadoffset = 0;
	uint32_t byteslefttocopy = 0;
	uint32_t i = 0;

	xEMACRxEventSemaphore = xSemaphoreCreateCounting(10, 0);

	for (;;) {
		/* Wait for the Ethernet MAC interrupt to indicate that another packet
		 has been received.  It is assumed xEMACRxEventSemaphore is a counting
		 semaphore (to count the Rx events) and that the semaphore has already
		 been created (remember this is an example of a simple rather than
		 optimised port layer). */
		if (xSemaphoreTake( xEMACRxEventSemaphore, portMAX_DELAY ) == pdTRUE) {

			/* get received frame */
			if (HAL_ETH_GetReceivedFrame_IT(&heth_global) != HAL_OK)
				return;

			/* Obtain the size of the packet */
			xBytesReceived = (uint16_t) heth_global.RxFrameInfos.length;
			buffer = (uint8_t *) heth_global.RxFrameInfos.buffer;

			if (xBytesReceived > 0) {
				/* Allocate a network buffer descriptor that points to a buffer
				 large enough to hold the received frame.  As this is the simple
				 rather than efficient example the received data will just be copied
				 into this buffer. */
				pxBufferDescriptor = pxGetNetworkBufferWithDescriptor(xBytesReceived, 0);

				if (pxBufferDescriptor != NULL) {
					/* pxBufferDescriptor->pucEthernetBuffer now points to an Ethernet
					 buffer large enough to hold the received data.  Copy the
					 received data into pcNetworkBuffer->pucEthernetBuffer.  Here it
					 is assumed ReceiveData() is a peripheral driver function that
					 copies the received data into a buffer passed in as the function's
					 parameter.  Remember! While is is a simple robust technique -
					 it is not efficient.  An example that uses a zero copy technique
					 is provided further down this page. */

					dmarxdesc = heth_global.RxFrameInfos.FSRxDesc;

					pxBufferDescriptor->xDataLength = xBytesReceived;

					byteslefttocopy = xBytesReceived;
					payloadoffset = 0;

					/* Check if the length of bytes to copy in current pbuf is bigger than Rx buffer size*/
					while (byteslefttocopy > ETH_RX_BUF_SIZE) {
						/* Copy data to pbuf */
						memcpy(
								(uint8_t*) ((uint8_t*) pxBufferDescriptor->pucEthernetBuffer
										+ payloadoffset), (uint8_t*) ((uint8_t*) buffer),
								ETH_RX_BUF_SIZE);

						/* Point to next descriptor */
						dmarxdesc = (ETH_DMADescTypeDef *) (dmarxdesc->Buffer2NextDescAddr);
						buffer = (uint8_t *) (dmarxdesc->Buffer1Addr);

						byteslefttocopy -= ETH_RX_BUF_SIZE;
						payloadoffset += ETH_RX_BUF_SIZE;
					}
					/* Copy remaining data in pbuf */
					memcpy(
							(uint8_t*) ((uint8_t*) pxBufferDescriptor->pucEthernetBuffer
									+ payloadoffset), (uint8_t*) ((uint8_t*) buffer),
							byteslefttocopy);
				}

				/* See if the data contained in the received Ethernet frame needs
				 to be processed.  NOTE! It is preferable to do this in
				 the interrupt service routine itself, which would remove the need
				 to unblock this task for packets that don't need processing. */
				if (eConsiderFrameForProcessing(pxBufferDescriptor->pucEthernetBuffer)
						== eProcessBuffer) {
					/* The event about to be sent to the TCP/IP is an Rx event. */
					xRxEvent.eEventType = eNetworkRxEvent;

					/* pvData is used to point to the network buffer descriptor that
					 now references the received data. */
					xRxEvent.pvData = (void *) pxBufferDescriptor;

					/* Send the data to the TCP/IP stack. */
					if (xSendEventStructToIPTask(&xRxEvent, 0) == pdFALSE) {
						/* The buffer could not be sent to the IP task so the buffer
						 must be released. */
						vReleaseNetworkBufferAndDescriptor(pxBufferDescriptor);

						/* Make a call to the standard trace macro to log the
						 occurrence. */
						iptraceETHERNET_RX_EVENT_LOST();
					}
					else {
						/* The message was successfully sent to the TCP/IP stack.
						 Call the standard trace macro to log the occurrence. */
						iptraceNETWORK_INTERFACE_RECEIVE();
						gdb.monit.rx_eth_frames++;
					}
				}
				else {
					/* The Ethernet frame can be dropped, but the Ethernet buffer
					 must be released. */
					vReleaseNetworkBufferAndDescriptor(pxBufferDescriptor);
				}

				/* Release descriptors to DMA */
				/* Point to first descriptor */
				dmarxdesc = heth_global.RxFrameInfos.FSRxDesc;
				/* Set Own bit in Rx descriptors: gives the buffers back to DMA */
				for (i = 0; i < heth_global.RxFrameInfos.SegCount; i++) {
					dmarxdesc->Status |= ETH_DMARXDESC_OWN;
					dmarxdesc = (ETH_DMADescTypeDef *) (dmarxdesc->Buffer2NextDescAddr);
				}

				/* Clear Segment_Count */
				heth_global.RxFrameInfos.SegCount = 0;

			}
			else {
				/* The event was lost because a network buffer was not available.
				 Call the standard trace macro to log the occurrence. */
				iptraceETHERNET_RX_EVENT_LOST();
			}
		}

		/* When Rx Buffer unavailable flag is set: clear it and resume reception */
		if ((heth_global.Instance->DMASR & ETH_DMASR_RBUS) != (uint32_t) RESET) {
			/* Clear RBUS ETHERNET DMA flag */
			heth_global.Instance->DMASR = ETH_DMASR_RBUS;
			/* Resume DMA reception */
			heth_global.Instance->DMARPDR = 0;
		}
	}

}

extern TaskHandle_t xServerWorkTaskHandle;

/* Defined by the application code, but called by FreeRTOS+UDP when the network
 connects/disconnects (if ipconfigUSE_NETWORK_EVENT_HOOK is set to 1 in
 FreeRTOSIPConfig.h). */
void vApplicationIPNetworkEventHook(eIPCallbackEvent_t eNetworkEvent) {
	uint32_t ulIPAddress, ulNetMask, ulGatewayAddress, ulDNSServerAddress;
	static BaseType_t xTasksAlreadyCreated = pdFALSE;
	int8_t cBuffer[16];
	char bufor[100];

	/* Check this was a network up event, as opposed to a network down event. */
	if (eNetworkEvent == eNetworkUp) {
		/* Create the tasks that use the IP stack if they have not already been
		 created. */
		if (xTasksAlreadyCreated == pdFALSE) {
			//if(xTaskCreate(vUDPTransmitCANFramesTask,"Tx CAN/UDP",UDPD_CAN_TX_SIZE,NULL,UDP_CAN_TX_PRIO,NULL)==pdPASS) print_console("Creating UDP/CAN Tx task -- > OK\r\n");
			// else print_console("Creating UDP/CAN Tx -- > failed\r\n");

			SNTP_init();

			//daj znac taskowi do obslugi http/ftp,ze moze utworzyc sockety
#if ipconfigUSE_HTTP==1 || ipconfigUSE_FTP==1
			xTaskNotifyGive(xServerWorkTaskHandle);
#endif

			xTasksAlreadyCreated = pdTRUE;
		}

		/* The network is up and configured.  Print out the configuration,
		 which may have been obtained from a DHCP server. */
		FreeRTOS_GetAddressConfiguration(&ulIPAddress, &ulNetMask, &ulGatewayAddress,
				&ulDNSServerAddress);

		print_console("\r\n-----------------------\r\n\r\n");

		/* Convert the IP address to a string then print it out. */

		FreeRTOS_inet_ntoa(ulIPAddress, cBuffer);
		sprintf(bufor, "IP Address: %s\r\n", cBuffer);
		print_console(bufor);
		gdb.sys.ipaddress = ulIPAddress;

		/* Convert the net mask to a string then print it out. */
		FreeRTOS_inet_ntoa(ulNetMask, cBuffer);
		sprintf(bufor, "Subnet Mask: %s\r\n", cBuffer);
		print_console(bufor);
		gdb.sys.maskaddress = ulNetMask;

		/* Convert the IP address of the gateway to a string then print it out. */
		FreeRTOS_inet_ntoa(ulGatewayAddress, cBuffer);
		sprintf(bufor, "Gateway IP Address: %s\r\n", cBuffer);
		print_console(bufor);
		gdb.sys.gwaddress = ulGatewayAddress;

		/* Convert the IP address of the DNS server to a string then print it out. */
		FreeRTOS_inet_ntoa(ulDNSServerAddress, cBuffer);
		sprintf(bufor, "DNS server IP Address: %s\r\n", cBuffer);
		print_console(bufor);
		gdb.sys.dnsaddress = ulDNSServerAddress;
	}
}




