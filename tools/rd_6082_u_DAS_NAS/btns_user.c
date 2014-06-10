#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/poll.h>

#include "btns_dev.h"

int main(int argc, char *argv[])
{
        char *name = "/dev/btns";
	int fd, fdflags;
        int rc = 0;

	BTNS_STS btns_sts;

	/* open the device */
        fd = open(name, O_RDWR);
        if (fd <= 0) {
                printf("## Cannot open %s device.##\n",name);
                exit(2);
        }

	/* set some flags */
        fdflags = fcntl(fd,F_GETFL,0);
        fdflags |= O_NONBLOCK;
        fcntl(fd,F_SETFL,fdflags);

	rc = ioctl(fd, CIOCWAIT_P, &btns_sts);
        if(rc < 0) 
		printf("btns: failed to perform action!\n");

	/* do what ever you want */
	if(btns_sts.mask & RESET_BTN)
		printf("Reset ");

	if(btns_sts.mask & STNBY_BTN)
		printf("Standby ");

	printf("button was pressed.\n");

        close(fd);

        return 0;
}


