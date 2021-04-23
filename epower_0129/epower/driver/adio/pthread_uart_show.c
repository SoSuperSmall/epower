#include "tty_uart.h"
#include "pthread_uart_show.h"

int uart_show_pthread(unsigned char *msg)
{
	int fd;
	int ret;
	char c;
	const char *device = "/dev/ttyS1";
	int speed = 115200;
	int hardflow = 0;
	int verbose = 1;
	int rs485 = 0;
	int savefile = 0;
	FILE *fp;
	//parse_opts(argc, argv);
	
	signal(SIGINT, sig_handler); 
	
	fd = libtty_open(device);
	if (fd < 0) {
		printf("libtty_open error.\n");
		exit(0);
	}
	ret = libtty_setopt(fd, speed, 8, 1, 'n', hardflow);
	if (ret != 0) {
		printf("libtty_setopt error.\n");
		exit(0);
	}
	while (1) {
			libtty_write(fd,msg);
	}	
	ret = libtty_close(fd);
	if (ret != 0) {
		printf("libtty_close error.\n");
		exit(0);
	}
	return 0;
}

