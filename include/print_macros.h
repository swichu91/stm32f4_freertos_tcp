/*
 * print_macros.h
 *
 *  Created on: 26 paü 2015
 *      Author: Mateusz
 */

#ifndef PRINT_MACROS_H_
#define PRINT_MACROS_H_

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "console.h"
//#include "sntp.h"

char print_buffer[512];


#define PRINT_SYSTIME	do{	print_console("Current time:\n");	\
					sprintf(print_buffer,"%d.%02d.%02d\n%02d:%02d:%02d\n",sys_time.year,sys_time.month,sys_time.day,sys_time.hour,sys_time.min,sys_time.sec);	\
					print_console(print_buffer);	\
					print_console("\n");	\
}while(0)


//printf ogolnego przeznaczenia
#define print_gen(...) do{	\
	print_console("\n");	\
	sprintf(print_buffer,##__VA_ARGS__);	\
	print_console(print_buffer);\
	print_console("\n");\
}while(0)




//wewnetrzny printf dla systemu FAT
#define print_FAT(...) do{	\
	print_console("\n");	\
	sprintf(print_buffer,##__VA_ARGS__);	\
	print_console(print_buffer);\
}while(0)


//wewnetrzny printf m.in dla stosu TCP/IP, uzywac tylko z podwojnymi nawiasami np. FreeRTOS_printf(("%d",test))
#define FreeRTOS_printfx(...) do{	\
	sprintf(print_buffer,##__VA_ARGS__);	\
	print_console(print_buffer);\
}while(0)




#endif /* PRINT_MACROS_H_ */
