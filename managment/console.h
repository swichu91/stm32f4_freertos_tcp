#ifndef CONSOLE_H_
#define CONSOLE_H_


#include <stdint.h>


#define CMD_MAX_LEN 	50

#define EOT (0x04)
#define BS	(0x7f)
#define LF	(0x0a)
#define CR_CM	(0x0d)
#define ETB	 (0x17)
#define ESC	(0x1b)
#define CMD_PRMT	(0x3E)

#define RX_QUEUE_LENGTH	1000
#define MAX_LEX_LEN		20 // 16 + 1 na na znak '\0' np. adres ip: 100.100.100.100=15znakow (zmiana na 32)
#define MAX_LEX_COUNT	10

struct history
{
    char* cmd;
    unsigned char  no;
};

typedef struct __action_command
{
	const signed char command[32];
	void (*action)(void *tsm);
	char idx_in_help_file[4];
}_action_command;


static const char cmd_not_found[]="\r\nCommand not found\r\n";

void vConsoleRxTask (void *pvparameters);
void vConsoleTxTask (void *pvparameters);
void print_console (char const * string);
void interpreter(char *cmd_buf, char len, void *tsm,char *cmd);
void usart_init(uint32_t baudrate);


#endif
