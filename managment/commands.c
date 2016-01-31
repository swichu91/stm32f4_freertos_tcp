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
#include "stm32f4xx_hal.h"
#include "global_db.h"
#include "udpd.h"
#include "ff_headers.h"
#include "ff_stdio.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_ARP.h"

extern char arg[DB_COUNT][DB_CMD_MAX_LEN];
extern ETH_HandleTypeDef heth_global;

void show_cmd(void *tsm) {
	uint32_t ret_val;

	HAL_ETH_ReadPHYRegister(&heth_global, PHY_SR, &ret_val);

	char buffer[200];

	if (!strcmp(arg[1], "link")) {

		char link[4], speed[8], duplex[3];

		if (ret_val & 0x01)
			strcpy(link, "ON");
		else
			strcpy(link, "OFF");

		if (ret_val & 0x02)
			strcpy(speed, "10Mbps");
		else
			strcpy(speed, "100Mbps");

		if (ret_val & 0x04)
			strcpy(duplex, "FD");
		else
			strcpy(duplex, "HD");

		sprintf(buffer, "LINK: %s SPEED: %s DUPLEX: %s \r\n", link, speed, duplex);

		print_console(buffer);

	}
	else if (!strcmp(arg[1], "ip")) {

		sprintf(buffer,
				"IP: %d.%d.%d.%d\r\nMASK: %d.%d.%d.%d\r\nGATEWAY: %d.%d.%d.%d\r\nDNS: %d.%d.%d.%d\r\n",
				gdb.sys.ipaddress & 0xFF, (gdb.sys.ipaddress & 0xFF00) >> 8,
				(gdb.sys.ipaddress & 0xFF0000) >> 16, gdb.sys.ipaddress >> 24,
				gdb.sys.maskaddress & 0xFF, (gdb.sys.maskaddress & 0xFF00) >> 8,
				(gdb.sys.maskaddress & 0xFF0000) >> 16, gdb.sys.maskaddress >> 24,
				gdb.sys.gwaddress & 0xFF, (gdb.sys.gwaddress & 0xFF00) >> 8,
				(gdb.sys.gwaddress & 0xFF0000) >> 16, gdb.sys.gwaddress >> 24,
				gdb.sys.dnsaddress & 0xFF, (gdb.sys.dnsaddress & 0xFF00) >> 8,
				(gdb.sys.dnsaddress & 0xFF0000) >> 16, gdb.sys.dnsaddress >> 24);

		print_console(buffer);

	}
	else
		print_console("Bad param\r\n");

}

void reset_cmd(void *tsm) {

	vPortEnterCritical();
	HAL_NVIC_SystemReset();
	vPortExitCritical();

}

void help_cmd(void *tsm) {

	print_console("OK\r\n");

}

void change_cmd(void *tsm) {

	char *ptr = NULL;

	uint8_t param_integer[4];

	uint8_t i = 0;

	if (!(strcmp(&arg[1][0], "ip"))) {

		ptr = strtok(&arg[2][0], ".");

		for (i = 0; i < 4; i++) {
			param_integer[i] = strtol(ptr, NULL, 10);

			if (!param_integer[i]) {
				print_console("Error\r\n");
				return;
			}
			ptr = strtok(NULL, ".");
		}

		FreeRTOS_SetIPAddress(
				FreeRTOS_inet_addr_quick(param_integer[0], param_integer[1], param_integer[2],
						param_integer[3]));
		FreeRTOS_ClearARP();
		gdb.sys.ipaddress = FreeRTOS_inet_addr_quick(param_integer[0], param_integer[1],
				param_integer[2], param_integer[3]);

	}
	else if (!(strcmp(&arg[1][0], "gw"))) {
		ptr = strtok(&arg[2][0], ".");

		for (i = 0; i < 4; i++) {
			param_integer[i] = strtol(ptr, NULL, 10);

			if (!param_integer[i]) {
				print_console("Error\r\n");
				return;
			}
			ptr = strtok(NULL, ".");
		}

		FreeRTOS_SetGatewayAddress(
				FreeRTOS_inet_addr_quick(param_integer[0], param_integer[1], param_integer[2],
						param_integer[3]));
		gdb.sys.gwaddress = FreeRTOS_inet_addr_quick(param_integer[0], param_integer[1],
				param_integer[2], param_integer[3]);

	}
	else if (!(strcmp(&arg[1][0], "mask"))) {
		ptr = strtok(&arg[2][0], ".");

		for (i = 0; i < 4; i++) {
			param_integer[i] = strtol(ptr, NULL, 10);

			if (!param_integer[i]) {
				print_console("Error\r\n");
				return;
			}
			ptr = strtok(NULL, ".");
		}

		FreeRTOS_SetNetmask(
				FreeRTOS_inet_addr_quick(param_integer[0], param_integer[1], param_integer[2],
						param_integer[3]));
		gdb.sys.maskaddress = FreeRTOS_inet_addr_quick(param_integer[0], param_integer[1],
				param_integer[2], param_integer[3]);
	}
	else {
		print_console("Bad param\r\n");
	}

}

void udp_cmd(void *tsm) {
	char buffer[100];

	char *ptr = NULL;

	uint8_t param_integer[4];
	uint8_t i = 0;

	if (!(strcmp(&arg[1][0], "show"))) {

		sprintf(buffer, "UDP/CAN Transmit Address:%d.%d.%d.%d\r\nPort:%d\r\n",
				gdb.udp.xDestinationAddress.sin_addr & 0xFF,
				(gdb.udp.xDestinationAddress.sin_addr & 0xFF00) >> 8,
				(gdb.udp.xDestinationAddress.sin_addr & 0xFF0000) >> 16,
				gdb.udp.xDestinationAddress.sin_addr >> 24,
				FreeRTOS_htons(gdb.udp.xDestinationAddress.sin_port));

		print_console(buffer);

	}
	else if (!(strcmp(&arg[1][0], "ip"))) {
		ptr = strtok(&arg[2][0], ".");

		for (i = 0; i < 4; i++) {
			param_integer[i] = strtol(ptr, NULL, 10);

			if (!param_integer[i]) {
				print_console("Error\r\n");
				return;
			}
			ptr = strtok(NULL, ".");
		}

		gdb.udp.xDestinationAddress.sin_addr = FreeRTOS_inet_addr_quick(param_integer[0],
				param_integer[1], param_integer[2], param_integer[3]);
	}
	else if (!(strcmp(&arg[1][0], "port"))) {
		uint16_t port;

		sscanf(arg[2], "%d", (int*) &port);

		gdb.udp.xDestinationAddress.sin_port = FreeRTOS_htons(port);

	}

	else
		print_console("Bad param\r\n");

}

void tasks_list_cmd(void *tsm) {

	char buffer[500];

	vTaskList(buffer);
	print_console(buffer);

}

void time_cmd(void *tsm) {
	PRINT_SYSTIME
	;
}

void arp_cmd(void *tsm) {
	if (!*arg[1])
		FreeRTOS_PrintARPCache();
	else if (!strcmp(arg[1], "-d"))
		FreeRTOS_ClearARP();
	print_console("\n");
}

void netstat_cmd(void *tsm) {
	vTCPNetStat();
}

void DIR_cmd(void *tsm) {
	FF_FindData_t *pxFindStruct;
	const char *pcAttrib;
	const char *pcWritableFile = "writable file";
	const char *pcReadOnlyFile = "read only file";
	const char *pcDirectory = "directory";

	char* dir_ptr = "/usb";

	/* FF_FindData_t can be large, so it is best to allocate the structure
	 dynamically, rather than declare it as a stack variable. */
	pxFindStruct = (FF_FindData_t *) pvPortMalloc(sizeof(FF_FindData_t));

	/* FF_FindData_t must be cleared to 0. */
	memset(pxFindStruct, 0x00, sizeof(FF_FindData_t));

	print_console(dir_ptr);
	print_console("\r\n");

	/* The first parameter to ff_findfist() is the directory being searched.  Do
	 not add wildcards to the end of the directory name. */
	if (ff_findfirst(dir_ptr, pxFindStruct) == 0) {
		do {
			/* Point pcAttrib to a string that describes the file. */
			if ((pxFindStruct->ucAttributes & FF_FAT_ATTR_DIR) != 0) {
				pcAttrib = pcDirectory;
			}
			else if (pxFindStruct->ucAttributes & FF_FAT_ATTR_READONLY) {
				pcAttrib = pcReadOnlyFile;
			}
			else {
				pcAttrib = pcWritableFile;
			}

			/* Print the files name, size, and attribute string. */
			FreeRTOS_printf(
					( "%s [%s] [size=%d]\r\n", pxFindStruct->pcFileName, pcAttrib, (int)pxFindStruct->ulFileSize ));

		}
		while (ff_findnext(pxFindStruct) == 0);
	}

	/* Free the allocated FF_FindData_t structure. */
	vPortFree(pxFindStruct);
}

/*
 * Pobiera current working directory w kontekscie tasku obslugujacego konsole,
 * to znaczy,ze kazdy task ma swoja kopie current directory. Kazdy z taskow
 * moze pracowac na calkiem innych sciezkach relatywnych
 */

void CWD_cmd(void *tsm) {

	char pcBuffer[50];

	ff_getcwd(pcBuffer, sizeof(pcBuffer));
	FreeRTOS_printf(("%s\n",pcBuffer));

}

