/*
 * sntp.h
 *
 *  Created on: 22 lis 2015
 *      Author: Mateusz
 */

#ifndef SNTP_H_
#define SNTP_H_

#include "stdint.h"
/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

#define SNTP_PRINT_LOG				0



#define SNTP_THREAD_STACK_SIZE		(2*configMINIMAL_STACK_SIZE)
#define SNTP_THREAD_NAME			"sntp_client"
#define SNTP_THREAD_PRIO			0
#define SNTP_POLL_INTERVAL			5000
#define SNTP_PORT_NR				123
#define SNTP_SERVER_ADDRESS_NAME	"0.pl.pool.ntp.org"


#define JAN_1ST_1900  				2415021
#define SNTP_LOCAL_TIME_OFFSET_L  	3600
#define SNTP_LOCAL_TIME_OFFSET_Z 	0

#define SNTP_SUMMER_MIN_DAY       	(31+28+25)                        //25.03
#define SNTP_SUMMER_MAX_DAY       	(31+28+31+30+31+30+31+31+30+25)   //25.10


#define SNTP_MSG_LEN				48
#define SNTP_MODE_MASK              0x07
#define SNTP_MODE_CLIENT            0x03
#define SNTP_MODE_SERVER            0x04
#define SNTP_MODE_BROADCAST         0x05
#define SNTP_STRATUM_KOD            0x00
#define SNTP_VERSION                (4/* NTP Version 4*/<<3)
#define SNTP_LI_NO_WARNING          0x00

/* number of seconds between 1900 and 1970 */
#define DIFF_SEC_1900_1970         (2208988800UL)


struct stime
{
	uint32_t  sec;
	uint8_t  min;
	uint8_t  hour;
	uint8_t  day;
	uint8_t  month;
	uint16_t year;

	int8_t  timezone;
	uint64_t sec_1990;

}sys_time;

struct sntp_c
{
	xSocket_t					socket;
	uint16_t 					port;
	struct freertos_sockaddr 	xClient;
	uint16_t					poll_interval;

};

__attribute__( (packed) )
struct sntp_msg {
 uint8_t           li_vn_mode;
 uint8_t           stratum;
 uint8_t           poll;
 uint8_t           precision;
 uint32_t          root_delay;
 uint32_t          root_dispersion;
 uint32_t          reference_identifier;
 uint32_t          reference_timestamp[2];
 uint32_t          originate_timestamp[2];
 uint32_t          receive_timestamp[2];
 uint32_t          transmit_timestamp[2];
};

void SNTP_init(void);
time_t FreeRTOS_time( time_t *pxTime );

#endif /* SNTP_H_ */
