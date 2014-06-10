#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/irq.h>

#include "boardEnv/mvBoardEnvLib.h"
#include "boardEnv/mvBoardEnvSpec.h"
#include "buffalo/BuffaloSataHotplug.h"
#include "buffalo/BuffaloGpio.h"
#include "buffalo/kernevnt.h"
#include "gpp/mvGpp.h"
#include "gpp/mvGppRegs.h"

#define AUTHOR			"(C) BUFFALO INC."
#define DRIVER_NAME		"Buffalo GPIO SATA Hotplug Event Driver"
#define BUFFALO_DRIVER_VER	"1.00"

//#define DEBUG_SATA_HOTPLUG
#ifdef DEBUG_SATA_HOTPLUG
	#define TRACE(x)	x
#else
	#define TRACE(x)
#endif

#define PLUGGED_EVENT_MSG	"SATA %d plugged"
#define UNPLUGGED_EVENT_MSG	"SATA %d unplugged"

// for polling timer
#define SATA_POL_INTERVAL	HZ/100
#define SATA_POL_LOOPS		10

static MV_U32 initial_polarity_val;
static MV_U32 initial_polarity_val_high;

static int
BuffaloSataHotplugReadProc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int len = 0;
	struct sata_hotplug_data *sdata = (struct sata_hotplug_data *)data;

	if (sdata->prevplugstat == SATA_STAT_PLUGGED)
		len += sprintf(page + len, "plugged\n");
	else if (sdata->prevplugstat == SATA_STAT_UNPLUGGED)
		len += sprintf(page + len, "unplugged\n");
	else
		len += sprintf(page + len, "unknown\n");

	if (off + count >= len)
		*eof = 1;

	if (len < off)
		return 0;

	*start = page + off;

	return ((count < len - off) ? count : len - off);
}

static void
BuffaloSataHotplugProcEntryInit(struct sata_hotplug_data *data)
{
	char buf[64];
	struct proc_dir_entry *ent;
	struct sata_hotplug_data *sdata = (struct sata_hotplug_data *)data;

	sprintf(buf, "buffalo/gpio/hotplug/sata%d", (int)sdata->dev_id);
	ent = create_proc_entry(buf, 0, 0);
	if (!ent) {
		ent = proc_mkdir("buffalo/gpio/hotplug", 0);
		ent = create_proc_entry(buf, 0, 0);
	}

	if (ent) {
		ent->read_proc = BuffaloSataHotplugReadProc;
		ent->data = (void *)data;
	}
}

static void PollingSATAHotplug(unsigned long arg)
{
	struct sata_hotplug_data *data = (struct sata_hotplug_data *)arg;
	MV_BOOL pinstat = bfGppInRegBitTest(data->pin);
	SATA_PLUG_STATE plugstat;

	char buf[32];

	if (data->prevpinstat == pinstat)
		--data->loops;
	else
		data->loops = SATA_POL_LOOPS;

	if (data->loops) {
		data->timer.expires += SATA_POL_INTERVAL;
		data->prevpinstat = pinstat;
		add_timer(&data->timer);
	}
	else {
		if (pinstat & bfGppPolarityRegBitTest(data->pin))
			plugstat = SATA_STAT_PLUGGED;
		else if (!pinstat & bfGppPolarityRegBitTest(data->pin))
			plugstat = SATA_STAT_UNPLUGGED;
		else if (pinstat & !bfGppPolarityRegBitTest(data->pin))
			plugstat = SATA_STAT_UNPLUGGED;
		else
			plugstat = SATA_STAT_PLUGGED;

		if (plugstat == SATA_STAT_PLUGGED)
			sprintf(buf, PLUGGED_EVENT_MSG, (int)data->dev_id);
		else
			sprintf(buf, UNPLUGGED_EVENT_MSG, (int)data->dev_id);
		
		if (plugstat == SATA_STAT_UNPLUGGED) {
			if(data->pin < 32)
				mvGppPolaritySet(0, BIT(data->pin), initial_polarity_val);
			else
				mvGppPolaritySet(1, BIT(data->pin - 32), initial_polarity_val_high);
		}
		else {
			if(data->pin < 32)
				mvGppPolaritySet(0, BIT(data->pin), ~initial_polarity_val);
			else
				mvGppPolaritySet(1, BIT(data->pin - 32), ~initial_polarity_val_high);
		}

		data->prevplugstat = plugstat;
		buffalo_kernevnt_queuein(buf);
		enable_irq(IRQ_GPP_START + data->pin);
	}
}

static irqreturn_t
SataHotplugInterrupts(int irq, void *dev_id)
{
	struct sata_hotplug_data *data = (struct sata_hotplug_data *)dev_id;

	if(data->pin < 32)
		bfRegSet(GPP_INT_CAUSE_REG(0), BIT(data->pin), 0x0);
	else
		bfRegSet(GPP_INT_CAUSE_REG(1), BIT(data->pin - 32), 0x0);
		
	disable_irq_nosync(irq);

	init_timer(&data->timer);
	data->timer.expires = jiffies + SATA_POL_INTERVAL;
	data->timer.function = PollingSATAHotplug;
	data->timer.data = (unsigned long)data;
	data->loops = SATA_POL_LOOPS;
	data->prevpinstat = bfGppInRegBitTest(data->pin);
	add_timer(&data->timer);

	return IRQ_HANDLED;
}

static int __init BuffaloSataHotplug_init(void)
{
	int i;
	int retval;
	MV_32 pinNum;
	struct sata_hotplug_data *data;

	initial_polarity_val = MV_REG_READ(GPP_DATA_IN_POL_REG(0));
	initial_polarity_val_high = MV_REG_READ(GPP_DATA_IN_POL_REG(1));

	printk("initial_polarity_val = 0x%08x\n", initial_polarity_val);
	printk("initial_polarity_val_high = 0x%08x\n", initial_polarity_val_high);
	for (i = 0; (pinNum = mvBoardGpioPinNumGet(BOARD_GPP_SATA_HOT, i)) != MV_ERROR; i++) {
		if (!bfGppInRegBitTest(pinNum)) {
			bfGppPolarityRegBitInv(pinNum);
		}

		data = kmalloc(sizeof(struct sata_hotplug_data), 0);
		if (!data) {
			printk("%s> kmalloc() failed. SATA%d Hotplug Interrupt disabled.\n",
			       __FUNCTION__, i);
			continue;
		}

		data->pin = pinNum;
		data->prevplugstat = SATA_STAT_UNKNOWN;
		data->loops = SATA_POL_LOOPS;
		data->dev_id = (void*)i;
		
		retval = request_irq(IRQ_GPP_START + pinNum,
				     SataHotplugInterrupts,
				     0,
				     "SataHotplug",
				     (void*)data);

		if (retval) {
			printk("%s> request_irq() failed. SATA%d Hotplug Interrupt disabled.\n",
			       __FUNCTION__, i);
			kfree(data);
		}
		
		BuffaloSataHotplugProcEntryInit(data);
	}

	printk("%s %s Ver.%s installed.\n", DRIVER_NAME, AUTHOR, BUFFALO_DRIVER_VER);
	return 0;
}

void __exit BuffaloSataHotplug_exit(void)
{
	printk("%s %s uninstalled.\n", DRIVER_NAME, BUFFALO_DRIVER_VER);
}

module_init(BuffaloSataHotplug_init);
module_exit(BuffaloSataHotplug_exit);
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DRIVER_NAME);
MODULE_VERSION(BUFFALO_DRIVER_VER);
MODULE_LICENSE("GPL");
