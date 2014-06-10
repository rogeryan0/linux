#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <paths.h>
#include <stdlib.h>

#include <sys/sysctl.h>
#include <sys/time.h>

unsigned int
mv_reg_read(unsigned int reg)
{
    unsigned int value;
    FILE *resource_dump;

    resource_dump = fopen ("/proc/resource_dump" , "w");
    if (!resource_dump) {
      printf ("Eror opening file /proc/resource_dump\n");
      exit(-1);
    }
    fprintf (resource_dump,"register  r %08x",reg);
    fclose (resource_dump);
    resource_dump = fopen ("/proc/resource_dump" , "r");
    if (!resource_dump) {
      printf ("Eror opening file /proc/resource_dump\n");
      exit(-1);
    }
    fscanf (resource_dump , "%x" , &value);
    fclose (resource_dump);

    return value;
}

void
mv_reg_write(unsigned int reg, unsigned int value)
{
    FILE *resource_dump;

    resource_dump = fopen ("/proc/resource_dump" , "w");
    if (!resource_dump) {
      printf ("Eror opening file /proc/resource_dump\n");
      exit(-1);
    }
    fprintf (resource_dump,"register  w %08x %08x",reg,value);
    fclose (resource_dump);
    return;
}

int
main(int argc, char **argv)
{
        unsigned int regaddr,val,temp;
	int bit;

        if (argc == 4) {
                sscanf(argv[1], "%x", &regaddr);
                sscanf(argv[2], "%d", &bit);
		sscanf(argv[3], "%x", &val);
        }else{ printf ("Usage: mv_reg_bit_set <reg addr> <bit number 0-31> <value 0/1>\n");
                return 0;
        }

	val = val&1;
	bit = bit%32;
	temp = mv_reg_read(regaddr);
//	printf("old val %x\n",temp);
	temp &= ~(1<<bit);
	temp |= val<<bit;
	mv_reg_write(regaddr, temp);
        temp = mv_reg_read(regaddr);
//        printf("new val %x\n",temp);

        return 1;
}

