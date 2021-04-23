#ifndef __TTY_UART_H
#define __TTY_UART_H
 
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>  
#include <termios.h>  
#include <errno.h>   
#include <string.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <getopt.h>
#include <linux/serial.h>

void sig_handler(int signo);
int libtty_setopt(int fd, int speed, int databits, int stopbits, char parity, char hardflow);
int libtty_open(const char *devname);
int libtty_close(int fd);
int libtty_tiocmset(int fd, char bDTR, char bRTS);
int libtty_rs485set(int fd, char enable);
int libtty_rs485get(int fd, struct serial_rs485 *rs485conf);
void libtty_write(int fd,unsigned char *buf);
void libtty_read(int fd);
void sig_handler(int signo);
int libtty_sendbreak(int fd);

#endif