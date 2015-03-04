/*
 * Ró¿ne
 */
#include <string.h>
#include <stdio.h>
#include "console.h"
#include <stddef.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"




extern char arg[MAX_LEX_COUNT][MAX_LEX_LEN];



void  help_cmd (void *tsm)
{

	print_console("OK\r\n");


}

void change_cmd (void *tsm)
{

	char *ptr= NULL;

	char param_integer[4];

	uint8_t i=0;

	if(!(strcmp(&arg[1][0],"ip")))
	{

		ptr=strtok(&arg[2][0],".");

		for(i=0;i<4;i++)
		{
			param_integer[i]=strtol(ptr,NULL,10);

			if(!param_integer[i])
			{
				print_console("Error\r\n");
				return;
			}
			ptr=strtok(NULL,".");
		}

	FreeRTOS_SetIPAddress(FreeRTOS_inet_addr_quick(param_integer[0],param_integer[1],param_integer[2],param_integer[3]));



	}
	else if(!(strcmp(&arg[1][0],"gw")))
	{

		//todo: nie da sie zmienic adresu ip itd w locie (przynajmniej narazie nie wiem) wiec zapiszemy do flash te dane i resetujemy urzadzenie


	}
	else if(!(strcmp(&arg[1][0],"mask")))
	{



	}
	else
	{
		print_console("Bad parameter\r\n");
	}






}
