
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
#include <stdio.h>
#include "stm32f4xx_hal.h"





/*
 * Variables
 */


static char was_hist;

//tablica ze stringami komend
static char tab_cmd[3][CMD_MAX_LEN];

//tablica przetrzymujaca tablice struktur kolejno wpisanych komend
static struct history com_cmd_hist[3] =
{
		{tab_cmd[0], 1},
		{tab_cmd[1], 2},
		{tab_cmd[2], 3},
};

//bufory dla leksemow polecenia konsoli
char arg[MAX_LEX_COUNT][MAX_LEX_LEN];
//dlugosc tablicy komend we flaszu
short len_comm_act_tab;



static xQueueHandle TxconsoleQueue;
static xQueueHandle RxQueue;
static xSemaphoreHandle Txconsole;



/*
 * Declarations
 */

void przewinwtyl(struct history* cmd_hist, char* ptr_cmd_buf);

void clean_term(char *ptr_cmd_buf);
void add_to_history_cmd(struct history* cmd_hist, char* cmd);
static void usart_put_string(char* string);
static void usart_put_char(char byte);





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
		 {"tasklist",		 tasks_list_cmd}



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





//Task wysylania znakow na konsole

void vConsoleTxTask (void *pvparameters)
{

	//utworz kolejke znakow
	TxconsoleQueue = xQueueCreate(RX_QUEUE_LENGTH, sizeof( portCHAR ) );

    //utworz semafor binarny blokujacy czytanie z kolejki
	Txconsole=xSemaphoreCreateMutex();



	portCHAR cRxedChar;


    while(1)
	    {
    	//vTaskDelay(10 / portTICK_RATE_MS);
	        //czekaj az kos wywola funkcje print_c...(...) i wywola ten semafor
	        if(xSemaphoreTake(Txconsole,portMAX_DELAY ))
	       {
	            while((xQueueReceive( TxconsoleQueue,  &cRxedChar, 0) == pdTRUE))
	            {
	            	  usart_put_char(cRxedChar);

	           }



	       }
	    }
}
void print_console (char const * string)
{


		if(TxconsoleQueue && Txconsole )
		{
			// wpis do kolejki

			for(; *string!= 0; string++)
			{
				xQueueSend( TxconsoleQueue, ( void * ) string, 10 );
			}

			//ustaw semafor, watek ConsoleTaskTx bedzie mogl wyslac dane
			xSemaphoreGive(Txconsole);
		}
		else
		{
			usart_put_string(string);
		}
}




// Task odbioru i analizy znakow z konsoli
void vConsoleRxTask (void *pvparameters){


	char special=0;
	char i = 0;

	char command_buffer[CMD_MAX_LEN];
	char *ptr_cmd_buf = command_buffer;
	char onechar;

	//utworz kolejke znakow
	RxQueue = xQueueCreate(100, sizeof( portCHAR ) );


	for (;;)
	{

		//vTaskDelay(10 / portTICK_RATE_MS);
		while(xQueueReceive(RxQueue,&onechar,portMAX_DELAY))
		{

			  // sprawdzamy wstepnie czy to znak specjalny (wszelkie strzalki, delete)
			                if (onechar == 0x5b ||onechar == 0x1b) {
			                	special = 1;
			                }
			                //sprawdz czy to byla strzalka w gore
			                else if (special && onechar == 0x41 )
			                {
			                    //skasuj wyswietlona z historii komende i przygotuj dla poprzedniej
			                    clean_term(ptr_cmd_buf);
			                    //wczytaj poprzednia komende
			                    przewinwtyl(com_cmd_hist, ptr_cmd_buf);

			                    //wyswietla te komnde na rs
			                    print_console(ptr_cmd_buf);

						        //print_char(0, user.prom(&user));
			                    i= (char)strlen(ptr_cmd_buf);

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
			                	usart_put_char(onechar);

			                    //zabezpieczenie przed przekroczeniem tablicy
			                    if(i < CMD_MAX_LEN)
			                    {
			                        *(ptr_cmd_buf + i) = onechar;
			                        i++;
			                    }
			                }

			                //sprawdz czy to byl backspace
			                else if (onechar == BS) //backspace
			                {
			                    //sprawdz czy index jest jeszcze w dopuszczlnym obszarze,
			                    //z lewej i z prawej strony
			                    if(i > 0 && i < CMD_MAX_LEN)
			                    {
			                    	usart_put_char(BS);

			                        //wyczysc ostatni znak
			                        *(ptr_cmd_buf + i) = 0x0;
			                        //przesun o 1 do tylu aktualny indeks bufora
			                        i--;

			                    }
			                }
			                //jesli jakikolwiek inny klawisz ze znakow specjalnych
			                else if(special)	special = 0;

			                //sprawdz czy to byl enter lub koniec transmisji
			                else if (onechar == EOT || onechar ==LF ||
			                		onechar ==CR_CM || onechar==ETB)
			                {
			                	//usart_put_string("\r\n");
			                	//usart_put_char((char*)LF);
			                	//usart_put_char((char*)CMD_PRMT);
			                    //zakoncz c-stringa znakiem konca= '\0'
			                    *(ptr_cmd_buf + i) = '\0';

			                    //skopiuj te komende do kolejki przyjetych komend
			                    //add_to_history_cmd(com_cmd_hist, ptr_cmd_buf);
			                    //interpretacja komend
			                    interpreter(ptr_cmd_buf, i, NULL,ptr_cmd_buf);


			                    //przygotuj indeks pod nastepna komende na poczatek bufora
			                    i= 0;

			                    //byl ENTER, wiec ustaw ze nie kasuje nic STRZALKA W GORE wypisujaca z historii
			                    was_hist = 0;
			                }
			            }
			        }


}

//dodaje konende do historii
void add_to_history_cmd(struct history* cmd_hist , char* cmd)
{



	int i=0, j=0;

	//przesun wszystkie komendy o 1 i zapisz pod 0 aktualna, zmien z 0=>3
	for ( ; i< 3; i++)
	{
		cmd_hist[i].no--;
	}

	for ( ; j< 3; j++)
	{
		if(cmd_hist[j].no == 0)
		{
			strncpy(cmd_hist[j].cmd, cmd, CMD_MAX_LEN);
			cmd_hist[j].no = 3;
			break;
		}
	}

}


//odnajduje ostatnio wpisana komende
void przewinwtyl(struct history* cmd_hist, char* ptr_cmd_buf)
{




	int j=0;
	//przetrzymuje indeks cofnietej komendy
	static char last_id = 3;

	//znajdz najwiekszy indeks
	for ( ; j< 3; j++)
	{
		if(cmd_hist[j].no == last_id)
		{

			//skopiuj do bufora wywolywania komend
			strncpy(ptr_cmd_buf, cmd_hist[j].cmd, CMD_MAX_LEN);
			//dekrementuj indeks dla wczesniejszej komendy
			last_id--;
			//jesli dejdziemy do zera => zawroc
			if(!last_id)
				last_id = 3;
			break;
		}
	}


}


//czysc terminal jesli bylo kolejne wywolanie STRZALKI W GORE
void clean_term(char *ptr_cmd_buf)
{
	//czysc terminal teraz
	if (was_hist)
	{
		short len_back=0;
		len_back = (short)strlen(ptr_cmd_buf);

		for(; len_back >0; len_back--)
		{
			//wyczysc terminal
			//przesow na konsoli
			usart_put_char(BS);
		}

		return;
	}

	//ustaw na przyszlosc
	was_hist = 1;
}

//czysci bufory leksemow dla nowej komendy
void clean_arg_buffers()
{
	memset(&arg[0], 0x0, MAX_LEX_LEN);
	memset(&arg[1], 0x0, MAX_LEX_LEN);
	memset(&arg[2], 0x0, MAX_LEX_LEN);
	memset(&arg[3], 0x0, MAX_LEX_LEN);
	memset(&arg[4], 0x0, MAX_LEX_LEN);

}


void cut_cmd_str(char *cmd_buf, char len)
{
	short i;
	char* tok_ptr;

	//wyczysc bufory arg dla nowej komendy
	clean_arg_buffers();

	tok_ptr = strtok(cmd_buf, " ");

	for(i=0; i< MAX_LEX_COUNT && tok_ptr!=NULL; i++)
	{
		strncpy(&arg[i][0], tok_ptr, MAX_LEX_LEN-1);

		//zwraca NULL jesli nie znalazl znaku rozdzielajacego: ' '
		tok_ptr = strtok(NULL, " ");

	}
}

void interpreter(char *cmd_buf, char len, void *tsm, char* cmd)
{
    int i=0;

    int cmd_len =  sizeof(comm_act)/sizeof(_action_command);

    //potnij stringi wzgledem spacji
    cut_cmd_str(cmd_buf, len);

    //nic nie wpisane, tylko ENTER poszedl ???[lp]
    if(!arg[0][0]){
       // char str[3] = {' ', 0x08, "\0"}; //spacja, backspace, koniec stringa
        //wyslij spacje i skasuj ja
        print_console("\r\n");
        //po tym wyslij znak zachety
        goto PROMPT;
    }

    // skopiuj komende tylko jesli bylo cos wpisane, sam enter pomija
    add_to_history_cmd(com_cmd_hist, cmd);

    //przeszukaj cala tablice komend
    for (; i< cmd_len; i++)
    {

        if(strncmp((char*)comm_act[i].command, arg[0], MAX_LEX_LEN) == 0)    break;
    }

    //wykonaj komende, jesli wyskoczyl z petli przez: break;
    if(i<cmd_len)
    {
    	print_console("\r\n");
        comm_act[i].action(tsm);
    }
    else
    {
        //nie podejmuj akcji, komenda nierozpoznana
    	print_console(cmd_not_found);

    }

PROMPT:
    print_console(">");

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



	 USART6->BRR = APB2_FREQ/baudrate;

	 USART6->CR1 |= USART_CR1_RXNEIE | USART_CR1_TE | USART_CR1_RE | USART_CR1_UE ;

	 NVIC_SetPriority(USART6_IRQn,10);
	 NVIC_EnableIRQ(USART6_IRQn);
}

static void usart_put_char(char byte)
{

	USART6->DR = (uint32_t)byte;

	while(!(USART6->SR & USART_SR_TC));

}

static void usart_put_string(char* string)
{


	while(*string)
	{
		usart_put_char(*string);
		string++;

	}
}


// RX USART interrupt
void USART6_IRQHandler()
{

	char onechar;
	long lHigherPriorityTaskWoken = pdFALSE;

	GPIOD->ODR ^=GPIO_ODR_ODR_14;
	onechar=(char) USART6->DR;

	xQueueSendFromISR(RxQueue,&onechar,&lHigherPriorityTaskWoken);

	portEND_SWITCHING_ISR( lHigherPriorityTaskWoken );



}



