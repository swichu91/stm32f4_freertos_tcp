/*
 * sntp.c
 *
 *  Created on: 22 lis 2015
 *      Author: Mateusz
 */

#include "sntp.h"
#include "stdint.h"
#include "string.h"
#include "print_macros.h"
#include "task.h"


const char* s_task_create_ok ="Creating SNTP task --> OK\n";
const char* s_task_create_fail = "Creating SNTP task -- > failed\n";
const char* s_KOD = "Kiss-of-death response";


static void SNTP_thread(void* pvp);
static void GetGregorianDate(long JD, struct stime* time);

// dodaje lub odejmuje sekundy do pobranego czasu z serwera w zaleznosci od strefy czasowej
static inline void parseTimeZone(unsigned long* sec,struct stime* time)
{
	*sec += time->timezone*3600;
}


static uint getDayofYear(struct stime* time)
{
              //s,l,  m, k, m, cz,li,s, w, p, l, g
  uint tab[13]={0,
                31,
                31+28,
                31+28+31,
                31+28+31+30,
                31+28+31+30+31,
                31+28+31+30+31+30,
                31+28+31+30+31+30+31,
                31+28+31+30+31+30+31+31,
                31+28+31+30+31+30+31+31+30,
                31+28+31+30+31+30+31+31+30+31,
                31+28+31+30+31+30+31+31+30+31+30,
                31+28+31+30+31+30+31+31+30+31+30+31
                };

  return ( tab[time->month-1] + time->day);
}


//Sprawdzanie czasu letniego i zimowego
static uint32_t retPeriodofYearTime(struct stime* time)
{

  uint32_t ret_val;
  uint mdayofyear = getDayofYear(time);

  if ( (mdayofyear >= SNTP_SUMMER_MIN_DAY) && (mdayofyear < SNTP_SUMMER_MAX_DAY) )
  {
     ret_val = SNTP_LOCAL_TIME_OFFSET_L;
  }
  else
  {
     ret_val = SNTP_LOCAL_TIME_OFFSET_Z;
  }

  return (ret_val);
}


static void getDate(uint32_t sec,struct stime* time)
{
	long JD;

	  parseTimeZone(&sec,time);

	  sec +=retPeriodofYearTime(time);

	  time->sec = (uint32_t)(sec % 60);
	  sec /= 60;
	  time->min = (uint32_t)(sec % 60);
	  sec /= 60;
	  time->hour = ((uint32_t)(sec % 24));
	  sec /= 24;

	  JD = sec + JAN_1ST_1900;

	  GetGregorianDate(JD, time);
}

static void GetGregorianDate(long JD, struct stime* time)
{
	long j,y,d,m;

	j = JD - 1721119;
	y = (4 * j - 1) / 146097;
	j = 4 * j - 1 - 146097 * y;
	d = j / 4;
	j = (4 * d + 3) / 1461;
	d = 4 * d + 3 - 1461 * j;
	d = (d + 4) / 4;
	m = (5 * d - 3) / 153;
	d = 5 * d - 3 - 153 * m;
	d = (d + 5) / 5;
	y = 100 * y + j;
	if (m < 10)
	m = m + 3;
	else
	{
		m = m - 9;
		y = y + 1;
	}

	time->year = (uint32_t) y;
	time->month = (uint32_t) m;
	time->day = (uint32_t) d;
}


static void SNTP_thread(void* pvp)
{
	struct sntp_c 		params;
	const TickType_t 	xReceiveTimeOut = 500;
	uint32_t			ntp_addr=0;
	struct sntp_msg		sntpmsg,sntpresponse;
	uint16_t			usBytes;
	uint32_t			addr_len = sizeof(params.xClient);



	sys_time.timezone = 1;

    /* Attempt to open the socket. */
    params.socket = FreeRTOS_socket( FREERTOS_AF_INET,
    									 FREERTOS_SOCK_DGRAM,
			 	 	 	 	 	 	 	 FREERTOS_IPPROTO_UDP );

    /* Set a time out so connect() will try connect over and over */
    FreeRTOS_setsockopt( params.socket,
                         0,
                         FREERTOS_SO_RCVTIMEO,
                         &xReceiveTimeOut,
                         sizeof( xReceiveTimeOut ) );

    params.xClient.sin_port = FreeRTOS_htons(SNTP_PORT_NR);

    memset(&sntpmsg,0,sizeof(sntpmsg));
    sntpmsg.li_vn_mode = SNTP_LI_NO_WARNING | SNTP_VERSION | SNTP_MODE_CLIENT;


    for(;;)
    {
        while(ntp_addr==0)
        {
        	ntp_addr=FreeRTOS_gethostbyname(SNTP_SERVER_ADDRESS_NAME); // try to connect 5 times after that returns 0
        	params.xClient.sin_addr = ntp_addr;
        }

        FreeRTOS_sendto(params.socket,&sntpmsg,sizeof(sntpmsg),0,&params.xClient,sizeof(params.xClient)); // send request

        usBytes=FreeRTOS_recvfrom(params.socket,&sntpresponse,sizeof(sntpresponse),0,&params.xClient,&addr_len); // wait for NTP's server response,blocking

        if(usBytes == SNTP_MSG_LEN)
        {
        	/* Kiss-of-death packet. Use another server or increase SNTP_POLL_INTERVAL. */
        	if(sntpresponse.stratum == SNTP_STRATUM_KOD)
        	{
        		print_gen(s_KOD);

        	}
        	else if (((sntpresponse.li_vn_mode & SNTP_MODE_MASK) == SNTP_MODE_SERVER) ||
                    ((sntpresponse.li_vn_mode & SNTP_MODE_MASK) == SNTP_MODE_BROADCAST))
        	{
        		getDate(FreeRTOS_ntohl(sntpresponse.receive_timestamp[0]),&sys_time);
        		PRINT_SYSTIME;
        	}
        }
        else //error
        {

        }

        vTaskDelay(SNTP_POLL_INTERVAL);
    }
}


void SNTP_init(void)
{
	#if USE_SNTP

	 if(xTaskCreate(SNTP_thread,SNTP_THREAD_NAME,SNTP_THREAD_STACK_SIZE,NULL,SNTP_THREAD_PRIO,NULL)==pdPASS)
		 print_console(s_task_create_ok);
	 else
		 print_console(s_task_create_fail);



	#endif



}
