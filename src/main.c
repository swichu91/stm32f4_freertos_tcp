//
// This file is part of the GNU ARM Eclipse distribution.
// Copyright (c) 2014 Liviu Ionescu.
//

// ----------------------------------------------------------------------------

#include <stdio.h>
#include "diag/Trace.h"
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "stm32f407xx.h"
#include "task.h"
#include "core_cm4.h"
#include "stm32f4xx_hal.h"
#include "console.h"
#include "FreeRTOS_IP.h"


// ----------------------------------------------------------------------------
//
// Standalone STM32F4 empty sample (trace via NONE).
//
// Trace support is enabled by adding the TRACE macro definition.
// By default the trace messages are forwarded to the NONE output,
// but can be rerouted to any device or completely suppressed, by
// changing the definitions required in system/src/diag/trace_impl.c
// (currently OS_USE_TRACE_ITM, OS_USE_TRACE_SEMIHOSTING_DEBUG/_STDOUT).
//

// ----- main() ---------------------------------------------------------------

// Sample pragmas to cope with warnings. Please note the related line at
// the end of this function, used to pop the compiler diagnostics status.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"

uint64_t u64IdleTicksCnt=0; // Counts when the OS has no task to execute.
uint64_t tickTime=0;        // Counts OS ticks (default = 1000Hz).

void vApplicationTickHook( void ) {
    ++tickTime;

}

// This FreeRTOS call-back function gets when no other task is ready to execute.
// On a completely unloaded system this is getting called at over 2.5MHz!
// ----------------------------------------------------------------------------
void vApplicationIdleHook( void ) {

    ++u64IdleTicksCnt;
   // GPIOD->ODR ^=GPIO_ODR_ODR_13;
}

// A required FreeRTOS function.
// ----------------------------------------------------------------------------
void vApplicationMallocFailedHook( void ) {
    configASSERT( 0 );  // Latch on any failure / error.

}

void vApplicationStackOverflowHook( TaskHandle_t pxTask, signed char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */
	taskDISABLE_INTERRUPTS();
	for( ;; );
}


void vBlinkLed (void * pvparameters){


	xNetworkBufferDescriptor_t pxdesc;

	for (;;)
	{

		vTaskDelay(500 / portTICK_RATE_MS);
		GPIOD->ODR ^=GPIO_ODR_ODR_12;

	}


}

void init_system_led(void)
{
	/* STM32F4 discovery LEDs */
	GPIO_InitTypeDef LED_Config;

	/* Always remember to turn on the peripheral clock...  If not, you may be up till 3am debugging... */
	 RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;

	LED_Config.Pin = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
	LED_Config.Mode = GPIO_MODE_OUTPUT_PP;
	LED_Config.Pull = GPIO_PULLUP;
	LED_Config.Speed = GPIO_SPEED_FAST;
	HAL_GPIO_Init(GPIOD, &LED_Config);

	return;
}


extern void prvEMACDeferredInterruptHandlerTask( void *pvParameters );


void Main_task (void * pvparameters)
{

	xTaskCreate(vBlinkLed, "Blink Led",configMINIMAL_STACK_SIZE,NULL,0,NULL);
	xTaskCreate(vConsoleRxTask,"ConRxTask",100,NULL,1,NULL);
	xTaskCreate(vConsoleTxTask,"ConTxTask",100,NULL,0,NULL);
    xTaskCreate(prvEMACDeferredInterruptHandlerTask,"EthRcvTask",2048,NULL,configMAX_PRIORITIES-1,NULL);


	/* The MAC address array is not declared const as the MAC address will
	normally be read from an EEPROM and not hard coded (in real deployed
	applications).*/
	static uint8_t ucMACAddress[ 6 ] = { MAC_ADDR0, MAC_ADDR1, MAC_ADDR2, MAC_ADDR3, MAC_ADDR4, MAC_ADDR5 };

	/* Define the network addressing.  These parameters will be used if either
	ipconfigUDE_DHCP is 0 or if ipconfigUSE_DHCP is 1 but DHCP auto configuration
	failed. */
	static const uint8_t ucIPAddress[ 4 ] = { 192, 168, 1, 200 };
	static const uint8_t ucNetMask[ 4 ] = { 255, 0, 0, 0 };
	static const uint8_t ucGatewayAddress[ 4 ] = { 192, 168, 1, 1 };

	/* The following is the address of an OpenDNS server. */
	static const uint8_t ucDNSServerAddress[ 4 ] = { 208, 67, 222, 222 };

	  /* Initialise the RTOS's TCP/IP stack.  The tasks that use the network
	    are created in the vApplicationIPNetworkEventHook() hook function
	    below.  The hook function is called when the network connects. */
	    FreeRTOS_IPInit( ucIPAddress,
	                     ucNetMask,
	                     ucGatewayAddress,
	                     ucDNSServerAddress,
	                     ucMACAddress );

	vTaskDelete(NULL);
}








int
main(int argc, char* argv[])
{
  // At this stage the system clock should have already been configured
  // at high speed.

	HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);


	init_system_led();
	usart_init(3000000); // usart na 3MHz to jest maks co ft232 moze wyciagnac


	xTaskCreate(Main_task,"Main", 8096, NULL,0, NULL);




	vTaskStartScheduler(); // This should never return.


  // Infinite loop
  while (1)
    {
       // Add your code here.
    }
}





void prvGetRegistersFromStack( uint32_t *pulFaultStackAddress )
{
/* These are volatile to try and prevent the compiler/linker optimising them
away as the variables never actually get used.  If the debugger won't show the
values of the variables, make them global my moving their declaration outside
of this function. */
volatile uint32_t r0;
volatile uint32_t r1;
volatile uint32_t r2;
volatile uint32_t r3;
volatile uint32_t r12;
volatile uint32_t lr; /* Link register. */
volatile uint32_t pc; /* Program counter. */
volatile uint32_t psr;/* Program status register. */

    r0 = pulFaultStackAddress[ 0 ];
    r1 = pulFaultStackAddress[ 1 ];
    r2 = pulFaultStackAddress[ 2 ];
    r3 = pulFaultStackAddress[ 3 ];

    r12 = pulFaultStackAddress[ 4 ];
    lr = pulFaultStackAddress[ 5 ];
    pc = pulFaultStackAddress[ 6 ];
    psr = pulFaultStackAddress[ 7 ];

    /* When the following line is hit, the variables contain the register values. */
    for( ;; );
}


/* The fault handler implementation calls a function called
prvGetRegistersFromStack(). */
 void HardFault_Handler(void)
{
    __asm volatile
    (
        " tst lr, #4                                                \n"
        " ite eq                                                    \n"
        " mrseq r0, msp                                             \n"
        " mrsne r0, psp                                             \n"
        " ldr r1, [r0, #24]                                         \n"
        " ldr r2, handler2_address_const                            \n"
        " bx r2                                                     \n"
        " handler2_address_const: .word prvGetRegistersFromStack    \n"
    );
}


// ----------------------------------------------------------------------------
