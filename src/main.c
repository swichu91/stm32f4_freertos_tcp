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

#include "stm32f4xx_hal.h"
#include "console.h"
#include "FreeRTOS_IP.h"
#include "udpd.h"
#include "can.h"
#include "global_db.h"
#include "ff_headers.h"
#include "ff_ramdisk.h"
#include "usb_host.h"
#include "ff_usbh.h"
#include "usbh_msc.h"
#include "ff_stdio.h"
#include "server_config.h"
#include "FreeRTOS_TCP_server.h"
#include "FreeRTOS_server_private.h"


// Prêdkosci poszczegolnych szyn:
// AHB -> 168MHz
// APB2 -> 84MHz
// APB1 -> 42Mhz




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

const char* welcome_logo =	"\r\nWelcome!\r\n" \
		"Software Version : 1.00 alpha\r\n" \
		"Created by: Mateusz Piesta\r\n\r\n\r\n"; \

void vApplicationTickHook( void ) {
    ++tickTime;

}

// This FreeRTOS call-back function gets when no other task is ready to execute.
// On a completely unloaded system this is getting called at over 2.5MHz!
// ----------------------------------------------------------------------------
void vApplicationIdleHook( void ) {

    ++u64IdleTicksCnt;
   // MX_USB_HOST_Process();
   // GPIOD->ODR ^=GPIO_ODR_ODR_13;
}

// A required FreeRTOS function.
// ----------------------------------------------------------------------------
void vApplicationMallocFailedHook( void ) {
    //configASSERT( 0 );  // Latch on any failure / error.
	print_console("Malloc failed!\n");

}

void vApplicationStackOverflowHook( TaskHandle_t pxTask, signed char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */
	//taskDISABLE_INTERRUPTS();
	//for( ;; );
	print_console("Holy FUCK! STACK OVERFLOW !!\n");

}


void vBlinkLed (void * pvparameters){

	for (;;)
	{

		vTaskDelay(500 / portTICK_RATE_MS);
		GPIOD->ODR ^=GPIO_ODR_ODR_12;
		FF_USBH_d(usbhDisk);
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
	LED_Config.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOD, &LED_Config);

	return;
}


extern void prvEMACDeferredInterruptHandlerTask( void *pvParameters );



void Main_task (void * pvparameters)
{
	//print welcome msg with actual software version
	print_console(welcome_logo);
	console_mngt_init();
	MX_USB_HOST_Init();
	xTaskCreate(vBlinkLed, "Blink Led",3*configMINIMAL_STACK_SIZE,NULL,0,NULL);
    xTaskCreate(prvEMACDeferredInterruptHandlerTask,"EthRcvTask",configMINIMAL_STACK_SIZE,NULL,configMAX_PRIORITIES-3,NULL);

	//prvCreateDiskAndExampleFiles(); //TODO: nie postawie fata na ramie w procku bo mam go za malo... minimalnie to 2Mb
    //sprvCreateDiskAndExampleFiles();

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

void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

  __PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1
                              |RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);

  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}


/* Handle of the task that runs the FTP and HTTP servers. */
TaskHandle_t xServerWorkTaskHandle = NULL;


/*-----------------------------------------------------------*/

#if( ( ipconfigUSE_FTP == 1 ) || ( ipconfigUSE_HTTP == 1 ) )

	static void prvServerWorkTask( void *pvParameters )
	{
	TCPServer_t *pxTCPServer = NULL;
	const TickType_t xInitialBlockTime = pdMS_TO_TICKS( 200UL );

	/* A structure that defines the servers to be created.  Which servers are
	included in the structure depends on the mainCREATE_HTTP_SERVER and
	mainCREATE_FTP_SERVER settings at the top of this file. */
	static const struct xSERVER_CONFIG xServerConfiguration[] =
	{
		#if( ipconfigUSE_HTTP == 1 )
				/* Server type,		port number,	backlog, 	root dir. */
				{ eSERVER_HTTP, 	80, 			12, 		configHTTP_ROOT },
		#endif

		#if( ipconfigUSE_FTP == 1 )
				/* Server type,		port number,	backlog, 	root dir. */
				{ eSERVER_FTP,  	21, 			12, 		"" }
		#endif
	};

		/* Remove compiler warning about unused parameter. */
		( void ) pvParameters;

		/* The priority of this task can be raised now the disk has been
		initialised. */
		vTaskPrioritySet( NULL, mainTCP_SERVER_TASK_PRIORITY );

		/* If the CLI is included in the build then register commands that allow
		the file system to be accessed. */
		#if( mainCREATE_UDP_CLI_TASKS == 1 )
		{
			vRegisterFileSystemCLICommands();
		}
		#endif /* mainCREATE_UDP_CLI_TASKS */


		/* Wait until the network is up before creating the servers.  The
		notification is given from the network event hook. */
		ulTaskNotifyTake( pdTRUE, portMAX_DELAY );

		/* Create the servers defined by the xServerConfiguration array above. */
		pxTCPServer = FreeRTOS_CreateTCPServer( xServerConfiguration, sizeof( xServerConfiguration ) / sizeof( xServerConfiguration[ 0 ] ) );
		configASSERT( pxTCPServer );

		for( ;; )
		{
			FreeRTOS_TCPServerWork( pxTCPServer, xInitialBlockTime );
		}
	}

#endif /* ( ( mainCREATE_FTP_SERVER == 1 ) || ( mainCREATE_HTTP_SERVER == 1 ) ) */
/*-----------------------------------------------------------*/

int
main(int argc, char* argv[])
{
  // At this stage the system clock should have already been configured
  // at high speed.

	HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

	SystemClock_Config();

	gdb_init();

	init_system_led();

	usart_init(3000000); // usart na 3MHz to jest smaks co ft232 moze wyciagnac

	//can_init(1024);

	#if( ( ipconfigUSE_FTP == 1 ) || ( ipconfigUSE_HTTP == 1 ) )
	{
		/* Create the task that handles the FTP and HTTP servers.The task is created at the idle
		priority, and sets itself to mainTCP_SERVER_TASK_PRIORITY after the file
		system has initialised. */
		xTaskCreate( prvServerWorkTask, "SvrWork", mainTCP_SERVER_STACK_SIZE, NULL, tskIDLE_PRIORITY, &xServerWorkTaskHandle );
	}
	#endif
	xTaskCreate(Main_task,"Main", 8096, NULL,7, NULL);

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
