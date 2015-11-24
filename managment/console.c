
#include "console.h"
#include "commands.h"


/*
 * FreeRTOS
 */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"


/*
 * Ró¿ne
 */
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <strings.h>
#include <stdlib.h>
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"


#define TX_BLOCKING_TIME	200


/*
 * Variables
 */

static xQueueHandle 	console_mngt_tx_queue;
static xQueueHandle		console_mngt_rx_queue;
static xSemaphoreHandle	console_mngt_txrdy_semph;

static console_mngt_t mngt_s;

const char EOT 		= 		0x04;
const char BS		=		0x7f;
const char LF		=		0x0a;
const char CR_CM	=		0x0d;
const char ETB		= 		0x17;
const char ESC		=		0x1b;
const char CMD_PRMT	=		0x3E;


/*
 * Declarations
 */
static void usart_put_string(const char* string);
static void usart_put_char(char byte);
static void console_mngt_clear_line(console_mngt_t* ptr);
static void console_mngt_interpreter(console_mngt_t* ptr,char len, void *tsm);
static void console_mngt_clean_arg_buffers(void);
static void console_mngt_cut_cmd_str(char *cmd_buf, char len);

static void console_mngt_TxTask (void *pvparameters);
static void console_mngt_RxTask (void *pvparameters);

static void print_console_b(const char* b);
void print_console(const char* string);

void history_db_write(history_db_t* ptr,const char* data_ptr);
void history_db_init(history_db_t* ptr );
void history_db_deinit(history_db_t* ptr);
char* history_db_read_back(history_db_t* ptr);
char* history_db_read(history_db_t* ptr);
char* history_db_read_not_mod(history_db_t* ptr);

static inline void SET_TXE_INTERRUPT(void){
	(USART6->CR1 |= USART_CR1_TXEIE);
}
static inline void CLR_TXE_INTERRUPT(void){
	(USART6->CR1 &= ~USART_CR1_TXEIE);
}

static inline void SET_RXE_INTERRUPT(void){
	(USART6->CR1 |= USART_CR1_RXNEIE);
}

static inline void CLR_RXE_INTERRUPT(void){
	(USART6->CR1 &= ~USART_CR1_RXNEIE);
}

/* Determine whether we are in thread mode or handler mode. */
static int inHandlerMode (void)
{
  return __get_IPSR() != 0;
}


/*
 * Tablica zawierajaca komendy obslugiwane przez interpreter
 *
 *
 *
 */

const _action_command comm_act[] =
{
		 {"help",            help_cmd},
		 {"change",			 change_cmd},
		 {"resetcpu",		 reset_cmd},
		 {"cmdlist",		 commlist_cmd},
		 {"show",			 show_cmd},
		 {"udp",			 udp_cmd},
		 {"tasklist",		 tasks_list_cmd},
		 {"netstat",		 netstat_cmd},
		 {"arp",		 	 arp_cmd},
		 {"dir",		 	 DIR_cmd},
		 {"cwd",		 	 CWD_cmd}



};



void commlist_cmd(void *tsm)
{

	uint32_t max_nr = sizeof(comm_act)/sizeof(_action_command);
	uint32_t i;

	char bufor[34];

	print_console("Avaible commands list:\r\n");

	for(i=0;i<max_nr;i++)
	{

		if(strcmp(comm_act[i].command,"cmdlist"))
		{
			strcpy(bufor,comm_act[i].command);
			strcat(bufor,"\r\n");
			print_console(bufor);
		}

	}

}

inline static void print_prompt(console_mngt_t* ptr){
	print_console_b("#");
}


void console_mngt_init(void)
{
	history_db_init(&mngt_s.history_db);
												//przy masakrowaniu pakietami danych przez Putty watermark zostaje na 14 wiec tyle jest optymalnie
	xTaskCreate(console_mngt_RxTask,"ConRxTask",configMINIMAL_STACK_SIZE*3,&mngt_s,0,NULL);
	xTaskCreate(console_mngt_TxTask,"ConTxTask",configMINIMAL_STACK_SIZE,&mngt_s,1,NULL);

}


//Task wysylania znakow na konsole

static void console_mngt_TxTask (void *pvparameters)
{
	console_mngt_t* ptr = pvparameters;

	//utworz kolejke znakow
	ptr->uart_tx_queue = xQueueCreate(TX_QUEUE_LENGTH, sizeof( portCHAR ) );
	ptr->uart_tx_rdy_smph = xSemaphoreCreateBinary();
	//xSemaphoreGive(ptr->uart_tx_rdy_smph);

	console_mngt_tx_queue = ptr->uart_tx_queue;
	portCHAR cRxedChar;

	//SET_TXE_INTERRUPT();

    while(1)
	{
	  while((xQueueReceive( ptr->uart_tx_queue,  &cRxedChar, 200) == pdTRUE)){
	     usart_put_char(cRxedChar);
	  }

	}
}

void print_console(const char* string)
{
	portBASE_TYPE taskWoken = pdFALSE;

	if(console_mngt_tx_queue)
	{
		// wpis do kolejki
		for(; *string!= 0; string++){

			if(inHandlerMode())
				xQueueSendFromISR(console_mngt_tx_queue, ( void * ) string, &taskWoken);
			else
				xQueueSend( console_mngt_tx_queue, ( void * ) string, TX_BLOCKING_TIME ); // silently ignore error

		}
		if(inHandlerMode())
			portEND_SWITCHING_ISR(taskWoken);
	}
	else{
		usart_put_string(string);
	}
}

static void print_console_b(const char* b){

	if(console_mngt_tx_queue){
		xQueueSend( console_mngt_tx_queue, b, TX_BLOCKING_TIME ); // silently ignore error
	}
	else{
		usart_put_char(*b);
	}

}




// Task odbioru i analizy znakow z konsoli
static void console_mngt_RxTask (void *pvparameters){

	console_mngt_t* mngt_ptr = (console_mngt_t*)pvparameters;

	char special=0;
	uint8_t i = 0;

	char onechar;

	//utworz kolejke znakow
	mngt_ptr->uart_rx_queue = xQueueCreate(RX_QUEUE_LENGTH, sizeof( portCHAR ) );
	console_mngt_rx_queue = mngt_ptr->uart_rx_queue;

	SET_RXE_INTERRUPT();

	for (;;)
	{

		while(xQueueReceive(mngt_ptr->uart_rx_queue,&onechar,portMAX_DELAY)==pdTRUE)
		{

			  // sprawdzamy wstepnie czy to znak specjalny (wszelkie strzalki, delete)
			                if (onechar == 0x5b ||onechar == 0x1b) {
			                	special = 1;
			                }
			                //sprawdz czy to byla strzalka w gore
			                else if (special && onechar == 0x41 )
			                {
			                    //skasuj wyswietlona z historii komende i przygotuj dla poprzedniej
			                    console_mngt_clear_line(mngt_ptr);

			                    strcpy(mngt_ptr->cmd_buf,history_db_read(&mngt_ptr->history_db));

			                    //wyswietla te komnde na rs
			                    print_console(mngt_ptr->cmd_buf);

						        //print_char(0, user.prom(&user));
			                    i= strlen(mngt_ptr->cmd_buf);

			                    mngt_ptr->current_cmd_len = i;

			                    // deaktywacja sekwencji sterujacej
			                    special = 0;

			                }
			                // to jest magiczna strzalka w dol
			                else if (special && onechar == 0x42 )
			                {
			                    //skasuj wyswietlona z historii komende i przygotuj dla poprzedniej
			                    console_mngt_clear_line(mngt_ptr);

			                    strcpy(mngt_ptr->cmd_buf,history_db_read_back(&mngt_ptr->history_db));

			                    //wyswietla te komnde na rs
			                    print_console(mngt_ptr->cmd_buf);

						        //print_char(0, user.prom(&user));
			                    i= strlen(mngt_ptr->cmd_buf);

			                    mngt_ptr->current_cmd_len = i;

			                    // deaktywacja sekwencji sterujacej
			                    special = 0;

			                }


			                //przyjmuj tylko znaki z przedzialu a-z 0-9 spacja, enter, backspace,itd.
			                //uwaga na strzalki, wysylaja po 2 znaki: A+costam oraz B+costam
			                else if(( (onechar >= 'a' && onechar <= 'z') ||
			                        (onechar >= 'A' && onechar <= 'Z') ||
			                        (onechar >= '0' && onechar <= '9') ||
			                        onechar == ' ' || onechar == '.'   ||
			                        onechar == '-' || onechar == '_'   ||
			                        onechar == '+' || onechar == '?' ||
			                        onechar == '@' )&&(!special)  )
			                {
			                    //echo dla wpisanego znaku
			                	print_console_b(&onechar);

			                    //zabezpieczenie przed przekroczeniem tablicy
			                    if(i < DB_CMD_MAX_LEN){
			                        mngt_ptr->cmd_buf[i++] = onechar;
			                        mngt_ptr->current_cmd_len++;
			                    }
			                }

			                //sprawdz czy to byl backspace
			                else if (onechar == BS) //backspace
			                {
			                    //sprawdz czy index jest jeszcze w dopuszczlnym obszarze,
			                    //z lewej i z prawej strony
			                    if(i > 0 && i < DB_CMD_MAX_LEN)
			                    {
			                    	print_console_b(&BS);

			                        //wyczysc ostatni znak i przesun znak
			                    	mngt_ptr->cmd_buf[i--] = 0x0;
			                    	mngt_ptr->current_cmd_len--;
			                    }
			                }
			                //jesli jakikolwiek inny klawisz ze znakow specjalnych
			                else if(special)	special = 0;

			                //sprawdz czy to byl enter lub koniec transmisji
			                else if (onechar == EOT || onechar ==LF ||
			                		onechar ==CR_CM || onechar==ETB)
			                {
			                	mngt_ptr->cmd_buf[i] = '\0';

			                	//mngt_ptr->current_cmd_len = strlen(mngt_ptr->cmd_buf);

			                    //history_db_write(&mngt_ptr->history_db_ptr,mngt_ptr->cmd_buf);
			                    //interpretacja komend
			                    console_mngt_interpreter(mngt_ptr, i, NULL);

			                    //przygotuj indeks pod nastepna komende na poczatek bufora
			                    i= 0;


			                }
			            }
			        }


}

//czysci linie terminalu
static void console_mngt_clear_line(console_mngt_t* ptr)
{
		uint8_t cnt;
		for(cnt=0;  cnt < ptr->current_cmd_len;cnt++){
			print_console_b(&BS);

		}
		//print_prompt(ptr);
}

//czysci bufory leksemow dla nowej komendy
static void console_mngt_clean_arg_buffers(void)
{
	uint8_t cnt;

	for(cnt=0;cnt<DB_COUNT;cnt++){

		memset(&arg[cnt], 0x0, DB_CMD_MAX_LEN);
	}

}


static void console_mngt_cut_cmd_str(char *cmd_buf, char len)
{
	uint8_t i;
	char* tok_ptr;

	//wyczysc bufory arg dla nowej komendy
	console_mngt_clean_arg_buffers();

	tok_ptr = strtok(cmd_buf, " ");

	for(i=0; i< DB_CMD_MAX_LEN && tok_ptr!=NULL; i++)
	{
		strncpy(&arg[i][0], tok_ptr, DB_CMD_MAX_LEN-1);

		//zwraca NULL jesli nie znalazl znaku rozdzielajacego: ' '
		tok_ptr = strtok(NULL, " ");

	}
}

static void console_mngt_interpreter(console_mngt_t* ptr,char len, void *tsm)
{
    uint32_t i=0;
    char cmd_buff_tmp[DB_CMD_MAX_LEN];

    strcpy(cmd_buff_tmp,ptr->cmd_buf);

    //potnij stringi wzgledem spacji
    console_mngt_cut_cmd_str(ptr->cmd_buf, len);

    //tylko enter
    if(!*arg[0]){

        print_console("\r\n");
        goto PROMPT;
    }

    if(strcmp(cmd_buff_tmp,history_db_read_not_mod(&ptr->history_db))){
    	history_db_write(&ptr->history_db,cmd_buff_tmp);
    }



    //TODO: wrzucic tutaj jakis wydajniejszy algorytm

    //przeszukaj cala tablice komend
    for (; i< sizeof(comm_act)/sizeof(_action_command); i++)
    {
        if(strncmp((char*)comm_act[i].command, arg[0], DB_CMD_MAX_LEN) == 0){

        	print_console("\r\n");
        	comm_act[i].action(tsm);
        	goto PROMPT;
        }
    }
    //nie podejmuj akcji, komenda nierozpoznana
    print_console(cmd_not_found);

	PROMPT:
	print_prompt(ptr);

}


void usart_init(uint32_t baudrate)
{

	 RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

	 RCC->APB2ENR |=RCC_APB2ENR_USART6EN;

	 GPIO_InitTypeDef Rx_pin,TX_pin;

	 Rx_pin.Pin = GPIO_PIN_7;
	 Rx_pin.Mode = GPIO_MODE_AF_OD;
	 Rx_pin.Pull = GPIO_NOPULL;
	 Rx_pin.Alternate = GPIO_AF8_USART6;

	 TX_pin.Pin = GPIO_PIN_6;
	 TX_pin.Mode = GPIO_MODE_AF_PP;
	 TX_pin.Speed = GPIO_SPEED_FAST;
	 TX_pin.Alternate = GPIO_AF8_USART6;

	 HAL_GPIO_Init(GPIOC,&Rx_pin);
	 HAL_GPIO_Init(GPIOC,&TX_pin);

	 CLR_RXE_INTERRUPT();
	 CLR_TXE_INTERRUPT();

	 USART6->BRR = APB2_FREQ/baudrate;

	 USART6->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;

	 NVIC_SetPriority(USART6_IRQn,10);
	 NVIC_EnableIRQ(USART6_IRQn);
}

static void usart_put_char(char byte)
{
/*
	if(console_mngt_txrdy_semph && xSemaphoreTake(console_mngt_txrdy_semph,20)==pdPASS)
	{
		SET_TXE_INTERRUPT();
		taskENTER_CRITICAL();
		USART6->DR = (uint32_t)byte;
		taskEXIT_CRITICAL();
	}
	else*/{

		USART6->DR = (uint32_t)byte;
		while(!(USART6->SR & USART_SR_TXE));

	}


}

static void usart_put_string(const char* string)
{


	while(*string)
	{
		usart_put_char(*string);
		string++;

	}
}


// RX USART interrupt
void USART6_IRQHandler(void)
{
	int8_t onechar;

	long lHigherPriorityTaskWoken = pdFALSE;

	if(USART6->SR & USART_SR_RXNE) // RX interrupt
	{
		GPIOD->ODR ^=GPIO_ODR_ODR_14;
		onechar=(char) USART6->DR;

		xQueueSendFromISR(console_mngt_rx_queue,&onechar,&lHigherPriorityTaskWoken);


	}

	if(USART6->SR & USART_SR_ORE) // overrun interrupt
	{
		onechar=(char) USART6->DR;
	}

	//if(USART6->SR & USART_SR_TXE) // data register empty
	//{
		//xSemaphoreGiveFromISR(console_mngt_txrdy_semph,&lHigherPriorityTaskWoken);
	//	CLR_TXE_INTERRUPT();

	//}
	/*
	if(USART6->SR & USART_SR_TC) // trasmit complete interrupt
	{

		dummy = USART6->SR;
		//USART6->DR = (uint32_t)dummy;



	}*/


	portEND_SWITCHING_ISR( lHigherPriorityTaskWoken );

}



/*
 * Struktury obslugujace historie komend i przewijanie
 */

extern inline void history_db_set_rd_to_beg(history_db_t* ptr);

//static allocation of history db
void history_db_init(history_db_t* ptr )
{
	uint8_t cnt;
	for(cnt=0;cnt<DB_COUNT;cnt++){
		ptr->data_ptr[cnt]=NULL;
	}
	//ptr->data_ptr = history_db;
	ptr->RD_p = 0;
	ptr->WR_p = 0;
	ptr->is_empty = true;
	ptr->is_full = false;
	ptr->size = 0;
	ptr->count = DB_COUNT;

}

void history_db_deinit(history_db_t* ptr)
{
	uint8_t cnt;
	for(cnt=0;cnt<DB_COUNT;cnt++){
			free(ptr->data_ptr[cnt]);
		}
	free(ptr);
}

void history_db_write(history_db_t* ptr,const char* data_ptr)
{
	if(ptr->data_ptr[ptr->WR_p]){
		ptr->size -=(strlen(ptr->data_ptr[ptr->WR_p])+1);
		free(ptr->data_ptr[ptr->WR_p]);
		ptr->data_ptr[ptr->WR_p] = calloc(strlen(data_ptr)+1,sizeof(char));
		ptr->size +=(strlen(data_ptr)+1);
	}
	else{
		ptr->data_ptr[ptr->WR_p] = calloc(strlen(data_ptr)+1,sizeof(char));
		ptr->size +=(strlen(data_ptr)+1);
	}

	memcpy(ptr->data_ptr[ptr->WR_p],data_ptr,strlen(data_ptr)+1);

	ptr->RD_p = ptr->WR_p;

	ptr->WR_p++;

	if(ptr->WR_p == ptr->count){ //todo: write pointer przekracza tablice
			ptr->is_full = true;
			ptr->WR_p = 0;
	}

}
char* history_db_read_not_mod(history_db_t* ptr)
{

	//if(!ptr->RD_p)
		return ptr->data_ptr[ptr->RD_p];
	//else
		//return ptr->data_ptr[ptr->RD_p-1];
}

char* history_db_read(history_db_t* ptr)
{


	/*if((!ptr->RD_p) || !(*ptr->data_ptr[ptr->RD_p])){

				if(!ptr->WR_p && ptr->data_ptr[DB_COUNT-1])
					ptr->RD_p = DB_COUNT-1;
				else if (!ptr->WR_p)
					ptr->RD_p = ptr->WR_p;
				else
					ptr->RD_p = ptr->WR_p-1;
	}*/

	if(ptr->RD_p && (ptr->RD_p < DB_COUNT)){

		ptr->RD_p--;
		return ptr->data_ptr[ptr->RD_p];
	}
	else if(ptr->RD_p == 0) return ptr->data_ptr[ptr->RD_p];


}

char* history_db_read_back(history_db_t* ptr)
{
	if((ptr->RD_p < DB_COUNT) && (*ptr->data_ptr[ptr->RD_p]) ){
		ptr->RD_p++;
		return ptr->data_ptr[ptr->RD_p];
	}
	else return ptr->data_ptr[ptr->RD_p-1];

/*
	if((ptr->RD_p==ptr->WR_p) || !(*ptr->data_ptr[ptr->RD_p])){
				ptr->RD_p = 0;
		}
*/


}

inline void history_db_set_rd_to_beg(history_db_t* ptr)
{
	ptr->RD_p = ptr->WR_p;
}



