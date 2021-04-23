#ifndef __PTHREAD_UART_SHOW_H
#define __PTHREAD_UART_SHOW_H

#include "adio.h"
pthread_t pthread_uart_show;

int uart_show_pthread(unsigned char *msg);
#endif