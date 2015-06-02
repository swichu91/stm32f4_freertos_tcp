/*
 * global_db.c
 *
 *  Created on: 24 maj 2015
 *      Author: Mateusz
 */

#include "global_db.h"
#include "udpd.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"




void gdb_init()
{

	//UDP
	gdb.udp.xDestinationAddress.sin_port =FreeRTOS_htons( UDP_CAN_TRANSMIT_DEFAULT_PORT );
	gdb.udp.xDestinationAddress.sin_addr = FreeRTOS_inet_addr( "192.168.1.252" );


}

