/*
 * server_config.h
 *
 *  Created on: 27 lis 2015
 *      Author: Mateusz
 */

#ifndef SERVER_CONFIG_H_
#define SERVER_CONFIG_H_

/* Include HTTP. */
#define ipconfigUSE_FTP  0
#define ipconfigUSE_HTTP 1
#define ipconfigUSE_SNTP 1


/* Dimension the buffers and windows used by the FTP and HTTP servers. */
#define ipconfigFTP_TX_BUFSIZE				( 8 * 1024 )
#define ipconfigFTP_TX_WINSIZE				( 4 )
#define ipconfigFTP_RX_BUFSIZE				( 8 * ipconfigNETWORK_MTU )
#define ipconfigFTP_RX_WINSIZE				( 4 )
#define ipconfigHTTP_TX_BUFSIZE				( 2 * ipconfigNETWORK_MTU )
#define ipconfigHTTP_TX_WINSIZE				( 2 )
#define ipconfigHTTP_RX_BUFSIZE				( 2 * ipconfigNETWORK_MTU )
#define ipconfigHTTP_RX_WINSIZE				( 2 )

#define mainTCP_SERVER_TASK_PRIORITY		5
#define mainTCP_SERVER_STACK_SIZE			2048


#define	configHTTP_ROOT						"usb/websrc"





#endif /* SERVER_CONFIG_H_ */
