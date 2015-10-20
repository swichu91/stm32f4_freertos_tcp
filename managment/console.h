#ifndef CONSOLE_H_
#define CONSOLE_H_


#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"


#define RX_QUEUE_LENGTH	512
#define TX_QUEUE_LENGTH	512

#define DB_CMD_MAX_LEN 50
#define DB_COUNT	10


typedef struct __action_command
{
	const char command[32];
	void (*action)(void *tsm);
	uint32_t idx_in_help_file;
}_action_command;

typedef struct _history_db
{
	uint32_t	RD_p;
	uint32_t	WR_p;
	uint32_t 	size;
	uint32_t 	count;

	char    *data_ptr[DB_COUNT];
	bool	is_full;
	bool	is_empty;

}history_db_t;

typedef struct _console_mngt
{
	xQueueHandle uart_tx_queue;
	xQueueHandle uart_rx_queue;
	xSemaphoreHandle uart_tx_rdy_smph;

	uint32_t b_rx_cnt;
	uint32_t b_tx_cnt;

	history_db_t history_db;

	int8_t	current_cmd_len;
	char	cmd_buf[DB_CMD_MAX_LEN];


}console_mngt_t;

//bufory dla leksemow polecenia konsoli
char arg[DB_COUNT][DB_CMD_MAX_LEN];


static const char cmd_not_found[]="\r\nCommand not found\r\n";

void print_console (const char * string);
void console_mngt_init(void);

void usart_init(uint32_t baudrate);
void usart_set_interrupts(void);


#endif
