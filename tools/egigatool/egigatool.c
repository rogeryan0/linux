#include <stdio.h>
#include <stdlib.h>
#include "mv_e_proc.h"

extern char **environ; /* all environments */

static unsigned int port = 0, q = 0, weight = 0, status = 0, mac[6] = {0,};
static unsigned int policy =0, direct = 0,command = 0, packett = 0;


void show_usage(int badarg)
{
        fprintf(stderr,
		"Usage: 										\n"
		" mvEgigaTool -h									\n"
		"   Display this help.									\n"
		"											\n"
		" mvEgigaTool <option> <Port No>							\n"	
		"   OPTIONS:										\n"
		"   -srq <0..8> <bpdu|arp|tcp|udp>      Set Rx q number for different packet types. 	\n"
		"                                       q number 8 means no special treatment.		\n"
		"					q number 8 for Arp means drop.			\n"
		"   -sq  <0..8> <Rx|Tx> <%%2x:%%2x:%%2x:%%2x:%%2x:%%2x>  				\n"
		"                                       Set Rx q number for a group of Multicast MACs,	\n"
		"                                       Or set Tx q number for a unicast address. 	\n"
		"                                       q number 8 indicate delete entry.		\n"
                "   -srp                 <WRR|FIXED>    Set the Rx Policy to WRR or Fixed 		\n"
		"   -srqw <0..7> <quota>                Set quota(hex) for RXQ (packet resolution)  	\n"
		"   -stp  <0..7> <quota> <WRR|FIXED>    Set the Tx Policy to WRR or Fixed               \n"
		"                                       And set quota(hex) for TXQ (256 Bytes resolution)\n"
		"											\n"
		" mvEgigaTool -St <option> <Port No>							\n"
		"   Display different status information of the port through the kernel printk.		\n"
		"   OPTIONS:										\n"
		"   p                                   Display General port information.		\n"
		"   q   <0..7>                          Display specific q information.			\n"
		"   rxp                                 Display Rx Policy information.			\n"
		"   txp                                 Display Tx Policy information.			\n"
		"   cntrs                               Display the HW MIB counters			\n"
		"   regs                                Display a dump of the giga registers		\n"
		"   statis                              Display port statistics information. 		\n\n"	
        );
#ifdef CONFIG_MV_ETH_HEADER
	show_head_usage();
#endif
        exit(badarg);
}

static void parse_pt(char *src)
{
        if (!strcmp(src, "bpdu"))
        	packett = PT_BPDU;
	else if(!strcmp(src, "arp"))
        	packett = PT_ARP;
	else if(!strcmp(src, "tcp"))
        	packett = PT_TCP;
	else if(!strcmp(src, "udp"))
        	packett = PT_UDP;
	else {
		fprintf(stderr, "Illegall packet type, packet type should be bpdu/arp/tcp/udp. \n");	
                exit(-1);
        }
        return;
}

static void parse_port(char *src)
{
	int count;

        count = sscanf(src, "%x",&port);

        if ((port > MAX_PORT) ||(count != 1))  {
		fprintf(stderr, "Illegal port number, max port supported is %d.\n",MAX_PORT);	
                exit(-1);
        }
        return;
}


static void parse_q(char *src)
{
	int count;

        count = sscanf(src, "%x",&q);

        if ((q >  MAX_Q + 1) || (count != 1)) {
		fprintf(stderr, "Illegal q number, max q supported is %d.\n",MAX_Q);	
                exit(-1);
        }
        return;
}

static void parse_direction(char *src)
{
        if (!strcmp(src, "Rx"))
        	direct = RX;
	else if(!strcmp(src, "Tx"))
        	direct = TX;
	else {
		fprintf(stderr, "Illegall direction, direction can be Rx or Tx.\n");	
                exit(-1);
        }
        return;
}

static void parse_policy(char *src)
{

        if (!strcmp(src, "WRR"))
        	policy = WRR;
	else if (!strcmp(src, "FIXED"))
        	policy = FIXED;
	else {
		fprintf(stderr, "Illegall policy, policy can be WRR or Fixed.\n");	
                exit(-1);
        }
        return;
}

static void parse_status(char *src)
{
    	if (!strcmp(src, "p")) {
      		status = STS_PORT;
      	}
     	else if(!strcmp(src, "q")) {
       		status = STS_PORT_Q;
     	}
   	else if(!strcmp(src, "rxp")) {
          	status = STS_PORT_RXP;
      	}
     	else if(!strcmp(src, "txp")) {
           	status = STS_PORT_TXP;
      	}
        else if(!strcmp(src, "cntrs")) {
                status = STS_PORT_MIB;
        }
        else if(!strcmp(src, "regs")) {
                status = STS_PORT_REGS;
        }
      	else if(!strcmp(src, "statis")) {
             	status = STS_PORT_STATIS;
        }
                else {
                fprintf(stderr, "Illegall ststus %d.\n");
                exit(-1);
        }
        return;
}

static void parse_weight(char *src)
{
        int i, count;

       	count = sscanf(src, "%x",&weight);
	if(count != 1) {
		fprintf(stderr, "Illegall weight, weight should be hex.\n");
                exit(-1);
	}
        return;
}

static int parse_mac(char *src)
{
        int count;
        int i;
        int buf[6];

        count = sscanf(src, "%2x:%2x:%2x:%2x:%2x:%2x",
                &buf[0], &buf[1], &buf[2], &buf[3], &buf[4], &buf[5]);

        if (count != MAC_ADDR_LEN) {
		fprintf(stderr, "Illegall MAC address, MAC adrdess should be %%2x:%%2x:%%2x:%%2x:%%2x:%%2x.\n");
                exit(-1);
        }

        for (i = 0; i < count; i++) {
                mac[i] = buf[i];
        }
        return 0;
}

static void parse_cmdline(int argc, char **argp)
{
	unsigned int i = 1;

	if(argc < 2) {
		show_usage(1);
	}

	if (!strcmp(argp[i], "-h")) {
		show_usage(0);
	}

	else if (!strcmp(argp[i], "-srq")) {
		command = COM_SRQ;
		i++;	
		if(argc != 5)
			show_usage(1); 
		parse_q(argp[i++]);
		parse_pt(argp[i++]);
	}
	else if (!strcmp(argp[i], "-sq")) {
		command = COM_SQ;
		i++;
		if(argc != 6)
			show_usage(1); 
		parse_q(argp[i++]);
		parse_direction(argp[i++]);
		parse_mac(argp[i++]);
	}
	else if (!strcmp(argp[i], "-srp")) {
		command = COM_SRP;
		i++;
		if(argc != 4)
			show_usage(1); 
		parse_policy(argp[i++]);
	}
	else if (!strcmp(argp[i], "-srqw")) {
		command = COM_SRQW;
		i++;
		if(argc != 5)
			show_usage(1); 
		parse_q(argp[i++]);
		parse_weight(argp[i++]);
	}
        else if (!strcmp(argp[i], "-stp")) {
                command = COM_STP;
                i++;
                if(argc != 6)
                        show_usage(1);
                parse_q(argp[i++]);
		parse_weight(argp[i++]);
		parse_policy(argp[i++]);
        }
	else if (!strcmp(argp[i], "-St")) {
                command = COM_STS;
                i++;
		if(argc < 4)
			show_usage(1);
		parse_status(argp[i++]);
		if( status == STS_PORT_Q ) {
                        if(argc != 5)
                                show_usage(1);
                	parse_q(argp[i++]);
		}
                else if(argc != 4)
                        show_usage(1);
        }
#ifdef CONFIG_MV_ETH_HEADER
        else if (!strcmp(argp[i], "-head")) {
		handle_head_cmd(argc,argp);
		exit(0);
        }	
#endif
	else {
		show_usage(i++);
	}
	parse_port(argp[i++]);
}

static int procit(void)
{
  	FILE *mvethproc;
  	mvethproc = fopen(FILE_PATH FILE_NAME, "w");
  	if (!mvethproc) {
    		printf ("Eror opening file %s/%s\n",FILE_PATH,FILE_NAME);
    		exit(-1);
  	}
  	fprintf (mvethproc, PROC_STRING, PROC_PRINT_LIST);
  	fclose (mvethproc);	
	
	return 0;
}

int main(int argc, char **argp, char **envp)
{
        parse_cmdline(argc, argp);
        return procit();
}

