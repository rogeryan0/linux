/*
 *  Driver routines for BUFFALO Platform
 *
 *  Copyright (C)  BUFFALO INC.
 *
 *  This software may be used and distributed according to the terms of
 *  the GNU General Public License (GPL), incorporated herein by reference.
 *  Drivers based on or derived from this code fall under the GPL and must
 *  retain the authorship, copyright and license notice.  This file is not
 *  a complete program and may only be used when the entire operating
 *  system is licensed under the GPL.
 *
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/init.h>
#include <linux/device.h>
#include <asm/uaccess.h>

#include "kernevntProc.h"

/* Globals */
// same as Buffalo Kernel Ver.
#define BUFCORE_VERSION "0.16"

/* Module parameters */
MODULE_AUTHOR("BUFFALO");
MODULE_DESCRIPTION("Buffalo Platform Linux Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(BUFCORE_VERSION);

/* Definitions */
//#define DEBUG

#define USE_PROC_BUFFALO

// ----------------------------------------------------

/* Definitions for DEBUG */
#ifdef DEBUG
 #define FUNCTRACE(x)  x

#else
 #define FUNCTRACE(x) 

#endif

/* Function prototypes */

/* variables */
int buffalo_booting = 1;
u32 env_addr;
u32 env_size;
u32 env_offset;
static struct class *buffalo_cls;

#define DEBUG_PROC
#ifdef DEBUG_PROC
//----------------------------------------------------------------------
static int debug_read_proc(char *page, char **start, off_t offset, int length)
{
	int              len  = 0;
	char*            buf  = page;
	off_t begin = 0;
#if 0
	volatile struct tagReg {
		unsigned MainIntCause;
		unsigned MainIrqIntMask;
		unsigned MainFiqIntMask;
	} *intreg;

	
	intreg = (struct tagReg *)(base + 0x20200);
	
	len += sprintf(buf+len,"cause=%lx irqmask=%lx fiqmask=%lx\n "
		,intreg->MainIntCause, intreg->MainIrqIntMask,intreg->MainFiqIntMask );
	
	len += sprintf(buf+len,"GPIO:OutReg=0x%04x\nDOutEn=0x%04x\nDBlink=0x%04x\nDatainActLow=0x%04x\nDIn=0x%04x\nInt=0x%04x\nMask=0x%03x\nLMask=0x%04x\n "
		,gpio[0],gpio[1],gpio[2],gpio[3],gpio[4],gpio[5],gpio[6],gpio[7] );
#endif

	unsigned base=0xF1000000;
	volatile unsigned *gpio=(unsigned *)(base+0x10100);

	len += sprintf(buf + len, "GPP_DATA_OUT_REG(0)   = 0x%08x\n", gpio[0]);
	len += sprintf(buf + len, "GPP_DATA_OUT_EN_REG(0)= 0x%08x\n", gpio[1]);
	len += sprintf(buf + len, "GPP_BLINK_EN_REG(0)   = 0x%08x\n", gpio[2]);
	len += sprintf(buf + len, "GPP_DATA_IN_POL_REG(0)= 0x%08x\n", gpio[3]);
	len += sprintf(buf + len, "GPP_DATA_IN_REG(0)    = 0x%08x\n", gpio[4]);
	len += sprintf(buf + len, "GPP_INT_CAUSE_REG(0)  = 0x%08x\n", gpio[5]);
	len += sprintf(buf + len, "GPP_INT_MASK_REG(0)   = 0x%08x\n", gpio[6]);
	len += sprintf(buf + len, "GPP_INT_LVL_REG(0)    = 0x%08x\n", gpio[7]);
	len += sprintf(buf + len, "MPP Control 0         = 0x%08x\n", *((unsigned *)(base+0x10000)));
	len += sprintf(buf + len, "MPP Control 1         = 0x%08x\n", *((unsigned *)(base+0x10004)));
	len += sprintf(buf + len, "MPP Control 2         = 0x%08x\n", *((unsigned *)(base+0x10050)));
	len += sprintf(buf + len, "Sample at Reset       = 0x%08x\n", *((unsigned *)(base+0x10010)));
	len += sprintf(buf + len, "Device Multiplex Control Register = 0x%08x\n", *((unsigned *)(base+0x10008)));
	
	*start = page + (offset - begin);	/* Start of wanted data */
	len -= (offset - begin);	/* Start slop */
	if (len > length)
		len = length;	/* Ending slop */
	return (len);
}

#endif

/* Functions */
//----------------------------------------------------------------------
char * getdate(void)
{
	static char newstr[32];
	char sMonth[8];
	unsigned char i;
	int iDay,iYear,iMonth=0;
	const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug",
                            "Sep", "Oct", "Nov", "Dec"};

	
	sscanf(__DATE__,"%s %d %d",sMonth,&iDay,&iYear);
	
	for (i = 0; i < 12; i++){
		if (!strcmp(sMonth, months[i])){
			iMonth = i + 1;
			break;
		}
	}
	sprintf(newstr,"%d/%02d/%02d",iYear,iMonth,iDay);
	return newstr;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,21)
extern char saved_command_line[];
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,21) */

extern int buffaloBoardInfoInit(void);
extern u32 buffalo_product_id;
extern char buffalo_series_name[];
extern char buffalo_product_name[];
//----------------------------------------------------------------------
static int kernelfw_read_proc(char *page, char **start, off_t offset, int length)
{
	//int              i;
	int              len  = 0;
	char*            buf  = page;
	off_t begin = 0;
	char *p,bootver[16];
	
	//printk("cmd:%s\n",saved_command_line);
	p=strstr(saved_command_line,"BOOTVER=");
	if (p){
		strncpy(bootver,p,sizeof(bootver));
		p=strstr(bootver," ");
		if (p)
			*p=0;
	}

	len += sprintf(buf+len,"SERIES=%s\n", buffalo_series_name);
	len += sprintf(buf+len,"PRODUCTNAME=%s\n", buffalo_product_name);

	len += sprintf(buf+len,"VERSION=%s\n",BUFCORE_VERSION);
	len += sprintf(buf+len,"SUBVERSION=FLASH 0.00\n");

	len += sprintf(buf+len,"PRODUCTID=0x%08X\n", buffalo_product_id);
	
	len += sprintf(buf+len,"BUILDDATE=%s %s\n",getdate(),__TIME__);
	len += sprintf(buf+len,"%s\n",bootver);
	
	*start = page + (offset - begin);	/* Start of wanted data */
	len -= (offset - begin);	/* Start slop */
	if (len > length)
		len = length;	/* Ending slop */
	return (len);
}
//----------------------------------------------------------------------
static int enetinfo_read_proc(char *page, char **start, off_t offset, int length)
{
	//int              i;
	int              len  = 0;
	char*            buf  = page;
	off_t begin = 0;
	int egiga_buffalo_proc_string(char *buf); // arch/arm/mach-mv88fxx81/egiga/mv_e_main.c

	len = egiga_buffalo_proc_string(buf);
	*start = page + (offset - begin);	/* Start of wanted data */
	len -= (offset - begin);	/* Start slop */
	if (len > length)
		len = length;	/* Ending slop */
	return (len);
}

static int enetinfo_write_proc(struct file *filep, const char *buffer, unsigned long count, void *data)
{
	//printk("buffer=%s\n", buffer);
	if(strncmp(buffer, "auto", 4) == 0){
		//printk("setting to auto...\n");
		egiga_buffalo_change_configuration((unsigned short)0);
	}else if(strncmp(buffer, "master", 6) == 0){
		//printk("setting to master...\n");
		egiga_buffalo_change_configuration((unsigned short)1);
	}else if(strncmp(buffer, "slave", 5) == 0){
		//printk("setting to slave...\n");
		egiga_buffalo_change_configuration((unsigned short)2);
	}else{
		//printk("no effect\n");
	}

	return count;
}
//----------------------------------------------------------------------
static int micon_read_proc(char *page, char **start, off_t offset, int length)
{
	//int              i;
	int              len  = 0;
	char*            buf  = page;
	off_t begin = 0;
	
	len = sprintf(buf,"on\n");
	*start = page + (offset - begin);	/* Start of wanted data */
	len -= (offset - begin);	/* Start slop */
	if (len > length)
		len = length;	/* Ending slop */
	return (len);
}

//----------------------------------------------------------------------
#if defined(CONFIG_BUFFALO_TERASTATION_TSHTGL)
static int lcd_read_proc(char *page, char **start, off_t offset, int length)
{
	//int              i;
	int              len  = 0;
	char*            buf  = page;
	off_t begin = 0;
	
	len = sprintf(buf,"on\n");
	*start = page + (offset - begin);	/* Start of wanted data */
	len -= (offset - begin);	/* Start slop */
	if (len > length)
		len = length;	/* Ending slop */
	return (len);
}
#endif

//----------------------------------------------------------------------
#if defined CONFIG_BUFFALO_SUPPORT_BOARD_INFO
static int board_info_read_proc(char *page, char **start, off_t offset, int length)
{
	int len = 0;
	char *buf = page;
	off_t begin = 0;
	char szBoardName[30];

	mvBoardNameGet(szBoardName);
	len += sprintf(buf + len, "BoardId=%02x\n", mvBoardIdGet());
	len += sprintf(buf + len, "BoardName=%s\n", szBoardName);
	//	len += sprintf(buf + len, "BoardStrap=%02x\n", BuffaloBoardGetStrapStatus());

	*start = page + (offset - begin);
	len -= (offset - begin);
	if (len > length)
		len = length;
	return (len);
}
#endif

//----------------------------------------------------------------------
static int booting_read_proc(char *page, char **start, off_t offset, int length, int *eof, void *data)
{
	int len = sprintf(page, "%d\n", buffalo_booting);
	*eof = 1;
	return len;
}

//----------------------------------------------------------------------
static int booting_write_proc(struct file *filep, const char *buffer, unsigned long count, void *data)
{
	if (strncmp(buffer, "0\n", count) == 0)
	{
		buffalo_booting = 0;
	}
	else if (strncmp(buffer, "1\n", count) == 0)
	{
		buffalo_booting = 1;
	}

	return count;
}

static int
BuffaloEnvInfoReadProc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int len = 0;

	len += sprintf(page + len, "0x%08x\n", (u32*)data);

	if (off + count >= len)
		*eof = 1;

	if (len < off)
		return 0;

	*start = page + off;

	return ((count < len - off) ? count : len - off);
}

struct class *BuffaloClassGet(void)
{
	struct class *tmp_cls;

	if (buffalo_cls)
		return buffalo_cls;

	tmp_cls = class_create(THIS_MODULE, "buffalo");

	if (IS_ERR(tmp_cls)) {
		printk(">%s: class_create failed.", __FUNCTION__);
		return NULL;
	}

	buffalo_cls = tmp_cls;
	return buffalo_cls;
}

static ssize_t show_env_addr(struct class_device *cls, char *buf)
{
  return sprintf(buf, "0x%08x\n", env_addr);
}

static ssize_t show_env_size(struct class_device *cls, char *buf)
{
  return sprintf(buf, "0x%08x\n", env_size);
}

static ssize_t show_env_offset(struct class_device *cls, char *buf)
{
  return sprintf(buf, "0x%08x\n", env_offset);
}

CLASS_DEVICE_ATTR(env_addr, S_IRUSR, show_env_addr, NULL);
CLASS_DEVICE_ATTR(env_size, S_IRUSR, show_env_size, NULL);
CLASS_DEVICE_ATTR(env_offset, S_IRUSR, show_env_offset, NULL);

//----------------------------------------------------------------------
int __init buffaloDriver_init (void)
{
	struct proc_dir_entry *generic;
	struct proc_dir_entry *enet;
	struct class *tmp_cls;
	FUNCTRACE(printk(">%s\n",__FUNCTION__));

	generic = create_proc_info_entry ("buffalo/firmware", 0, 0, kernelfw_read_proc);
	if (!generic) {
		proc_mkdir("buffalo", 0);
		generic = create_proc_info_entry ("buffalo/firmware", 0, 0, kernelfw_read_proc);
	}
#ifdef DEBUG_PROC
	generic = create_proc_info_entry ("buffalo/debug", 0, 0, debug_read_proc);
#endif
	enet = create_proc_entry ("buffalo/enet", 0, 0);
	enet->read_proc = &enetinfo_read_proc;
	enet->write_proc= &enetinfo_write_proc;

#ifdef CONFIG_BUFFALO_USE_MICON
	generic = create_proc_info_entry ("buffalo/micon", 0, 0, micon_read_proc);
#endif
#if defined CONFIG_BUFFALO_SUPPORT_BOARD_INFO
	generic = create_proc_info_entry ("buffalo/board_info", 0, 0, board_info_read_proc);
#endif

	generic = create_proc_entry("buffalo/booting", 0, 0);
	generic->read_proc = &booting_read_proc;
	generic->write_proc = &booting_write_proc;

	BuffaloKernevnt_init();
#if defined(CONFIG_BUFFALO_TERASTATION_TSHTGL)
	generic = create_proc_info_entry ("buffalo/lcd", 0, 0, lcd_read_proc);
#endif

	struct class *cls = BuffaloClassGet();
	if (cls) {
		struct class_device *cd = class_device_create(cls, NULL, 0, NULL, "u-boot");
		class_device_create_file(cd, &class_device_attr_env_addr);
		class_device_create_file(cd, &class_device_attr_env_size);
		class_device_create_file(cd, &class_device_attr_env_offset);
	}

	return 0;
}


//----------------------------------------------------------------------
void buffaloDriver_exit(void)
{
	FUNCTRACE(printk(">%s\n",__FUNCTION__));
	
	BuffaloKernevnt_exit();
#if defined CONFIG_BUFFALO_SUPPORT_BOARD_INFO
	remove_proc_entry("buffalo/board_info", 0);
#endif
	remove_proc_entry("buffalo/booting", 0);
#if defined CONFIG_BUFFALO_USE_MICON
	remove_proc_entry("buffalo/micon", 0);
#endif
	remove_proc_entry("buffalo/enet", 0);
#ifdef DEBUG_PROC
	remove_proc_entry("buffalo/debug", 0);
#endif
	remove_proc_entry("buffalo/firmware", 0);
	remove_proc_entry ("buffalo", 0);
}

module_init(buffaloDriver_init);
module_exit(buffaloDriver_exit);
