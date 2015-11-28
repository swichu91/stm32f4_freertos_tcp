#ifndef COMMANDS_H_
#define COMMANDS_H_


void  help_cmd (void *tsm);
void change_cmd (void *tsm);
void reset_cmd(void *tsm);
void commlist_cmd(void *tsm);
void show_cmd(void *tsm);
void udp_cmd(void *tsm);
void tasks_list_cmd(void *tsm);
void time_cmd(void *tsm);

void netstat_cmd( void *tsm);
void arp_cmd( void *tsm);

void DIR_cmd( void *tsm);
void CWD_cmd( void *tsm);









#endif
