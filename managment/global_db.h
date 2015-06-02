/*
 * global_db.h
 *
 *  Created on: 24 maj 2015
 *      Author: Mateusz
 */

#ifndef GLOBAL_DB_H_
#define GLOBAL_DB_H_

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "FreeRTOS_Sockets.h"



void gdb_init();

typedef struct __sys
{
	uint32_t ipaddress;
	uint32_t maskaddress;
	uint32_t gwaddress;
	uint32_t dnsaddress;

}_sys;

typedef struct __monit
{







}_monit;


typedef struct __udp_task_param
{

	uint32_t udpcan_remote_ip;
	uint16_t udpcan_remote_port;


}_udp_task_param;

typedef struct __udp
{

	struct freertos_sockaddr xDestinationAddress;

}_udp;


typedef struct __gdb
{

	_sys sys; // info o konfiguracji urzadzenia
	_monit monit; // monitorujaca
	_udp udp;	// roznorakie do udp


}_gdb;


//inicjalizacja globalnej struktury z danymi
_gdb gdb;











#endif /* GLOBAL_DB_H_ */
