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

void gdb_init(void);

struct _sys
{
	uint32_t ipaddress;
	uint32_t maskaddress;
	uint32_t gwaddress;
	uint32_t dnsaddress;

};

struct _monit
{
	uint64_t rx_eth_frames;
	uint64_t tx_eth_frames;

};


struct _udp_task_param
{

	uint32_t udpcan_remote_ip;
	uint16_t udpcan_remote_port;


};

struct _udp
{

	struct freertos_sockaddr xDestinationAddress;

};


struct _gdb
{

	struct _sys sys; // info o konfiguracji urzadzenia
	struct _monit monit; // monitorujaca
	struct _udp udp;	// roznorakie do udp


};


//inicjalizacja globalnej struktury z danymi
struct _gdb gdb;




#endif /* GLOBAL_DB_H_ */
