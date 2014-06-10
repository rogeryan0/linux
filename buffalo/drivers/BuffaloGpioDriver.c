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


#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/init.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include "BuffaloGpio.h"
#include "kernevnt.h"
#include "buffalo5182BoardEnv.h"
#include "mvGppRegs.h"

/* Globals */
#define BUFFALO_DRIVER_VER "0.02"
#define DRIVER_NAME	"Buffalo Gpio Control Driver"
#define AUTHOR	"(C) BUFFALO INC."

/* Module parameters */
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DRIVER_NAME);
MODULE_LICENSE("GPL");
MODULE_VERSION(BUFFALO_DRIVER_VER);

/* Definitions */
//#define DEBUG
#define USE_PROC_BUFFALO
#define BUFFALO_LED_CONTROL_INFO_BLOCK_ENABLE
	
#define MAX_ALARM_NUM	100
#define MAX_INFO_NUM	100
#define MAX_POWER_NUM	9
#define MAX_FUNC_NUM	9

#define LED_ON	1
#define LED_OFF	0

#define LED_PHASE_10	1
#define LED_PHASE_1		2
#define LED_PHASE_SHORT	3
#define LED_PHASE_LONG	4

#define BUFFALO_LED_WAIT_10		(HZ * 10 / 10)	// 1 sec
#define BUFFALO_LED_WAIT_1		(HZ * 5 / 10)	// 0.5 sec
#define BUFFALO_LED_WAIT_SHORT		(HZ * 3 / 10)	// 0.3 sec
#define BUFFALO_LED_WAIT_LONG		(HZ * 10 / 10)	// 1 sec


#define SW_POWER_NS	-1
#define SW_POWER_OFF	0
#define SW_POWER_ON	1
#define SW_POWER_AUTO	2

#define SW_POLLING_INTERVAL	1000	// 1sec

#define SW_PIN_STAT_OFF		0
#define SW_PIN_STAT_ON		(BIT(BIT_POWER))
#define SW_PIN_STAT_AUTO	(BIT(BIT_AUTO_POWER))

static struct timer_list BuffaloSwPollingTimer;

static unsigned int g_buffalo_sw_status = SW_POWER_NS;
static unsigned int g_buffalo_sw_polling_control = 0;

struct led_t{
	int mppNumber;
	int ErrorNumber;
	int Brighted;
	int Brighting;
	struct timer_list LedTimer;
} alarmLed, infoLed, powerLed, funcLed;


// ----------------------------------------------------

/* Definitions for DEBUG */
#ifdef DEBUG
 #define FUNCTRACE(x)  x
#else
 #define FUNCTRACE(x) 

#endif

#if defined LED_DEBUG
  #define LED_TRACE(x)	x
#else
  #define LED_TRACE(x)
#endif

#if defined PST_DEBUG
  #define PST_TRACE(x)	x
#else
  #define PST_TRACE(x)
#endif


static int
gpio_power_read_proc(char *page, char **start, off_t offset, int length)
{
	if (BIT_POWER < 0)
		return 0;

	int len=0;
	off_t begin=0;
	unsigned int PowerStat=buffalo_gpio_read();
	int i;

	len += sprintf(page + len, "GpioStat=0x%04x\n", PowerStat);
	len += sprintf(page + len, "BIT(%d)=0x%04x\n", BIT_POWER, BIT(BIT_POWER));
	len += sprintf(page + len, "PowerStat=%d\n", (PowerStat & BIT(BIT_POWER))? 1:0);

	if (BIT_LED_INFO >= 0)
		len += sprintf(page + len, "LedInfo=%d\n", (PowerStat & BIT(BIT_LED_INFO))? 0:1);

	if (BIT_LED_ALARM >= 0)
		len += sprintf(page + len, "LedAlarm=%d\n", (PowerStat & BIT(BIT_LED_ALARM))? 0:1);

	if (BIT_LED_FUNC >= 0)
		len += sprintf(page + len, "LedFunc=%d\n", (PowerStat & BIT(BIT_LED_FUNC))? 0:1);

	for (i=0; i<BUF_NV_SIZE_BIT_HDD_POWER; i++) {
		if (bitHddPower[i] >= 0)
		  len += sprintf(page + len, "HddPower%d=%d\n", i, (PowerStat & BIT(bitHddPower[i]))? 0:1);
	}
	
	for (i=0; i<BUF_NV_SIZE_BIT_USB_POWER; i++) {
		if (bitUsbPower[i] >= 0)
		  len += sprintf(page + len, "UsbPower%d=%d\n", i, (PowerStat & BIT(bitUsbPower[i]))? 0:1);
	}
	
	*start = page + (offset - begin);
	len -= (offset - begin);
	if (len > length)
		len = length;
	return (len);
}


static int
BuffaloKatoi(const char *buffer, unsigned int count){
	#define MAX_LENGTH	16
	int i;
	int TmpNum=0;
	
	LED_TRACE(printk("%s > Entered\n", __FUNCTION__));
	
	if(count > MAX_LENGTH){
		LED_TRACE(printk("%s > Leaving\n", __FUNCTION__));
		return -1;
	}
	
	LED_TRACE(printk("%s > buffer=0x%08x\n", __FUNCTION__, buffer));
	LED_TRACE(printk("%s > *buffer=0x%08x\n", __FUNCTION__, *buffer));
	for(i=0; i<count; i++, buffer++){
		LED_TRACE(printk("%s > *buffer=0x%08x, TmpNum=%d\n", __FUNCTION__, *buffer, TmpNum));
		if(*buffer>='0' && *buffer<='9')
			TmpNum=(TmpNum * 10) + (*buffer - '0');
	}

	LED_TRACE(printk("%s > Leaving\n", __FUNCTION__));
	return TmpNum;
}


static int
BuffaloLedGetNextPhase(struct led_t *led_stat){
	LED_TRACE(printk("%s > Entered\n", __FUNCTION__));

	switch(led_stat->Brighting){
	case LED_ON:
		if((led_stat->ErrorNumber - led_stat->Brighted) > 0)
			return LED_PHASE_SHORT;
		else
			return LED_PHASE_LONG;
		break;
	case LED_OFF:
		LED_TRACE(printk("%s > led_stat->ErrorNumer=%d, led_stat->Brighted=%d\n",
				 __FUNCTION__, led_stat->ErrorNumber, led_stat->Brighted));
		if((led_stat->ErrorNumber - led_stat->Brighted) >= 10)
			return LED_PHASE_10;
		else
			return LED_PHASE_1;
		break;
	default:
		return -1;
	}
	LED_TRACE(printk("%s > Leaving\n", __FUNCTION__));
	return -1;
}


static void
BuffaloLed_StatChange(unsigned long data)
{
	struct led_t *led = (struct led_t *)data;
	int NextPhase = BuffaloLedGetNextPhase(led);
	LED_TRACE(printk("%s > Entered\n", __FUNCTION__));
	
	switch(NextPhase){
	case LED_PHASE_10:
		led->LedTimer.expires = (jiffies + BUFFALO_LED_WAIT_10);
#if defined BUFFALO_LED_CONTROL_INFO_BLOCK_ENABLE
		if (led == &infoLed && alarmLed.ErrorNumber != 0)
			BuffaloGpio_LedDisable(led->mppNumber);
		else
			BuffaloGpio_LedEnable(led->mppNumber);
#else
		BuffaloGpio_LedEnable(led->mppNumber);
#endif
		led->Brighted += 10;
		led->Brighting = LED_ON;
		LED_TRACE(printk("%s > change to LED_PHASE_10\n", __FUNCTION__));
		break;
	case LED_PHASE_1:
		led->LedTimer.expires = (jiffies + BUFFALO_LED_WAIT_1);
#if defined BUFFALO_LED_CONTROL_INFO_BLOCK_ENABLE
		if (led == &infoLed && alarmLed.ErrorNumber != 0)
			BuffaloGpio_LedDisable(led->mppNumber);
		else
			BuffaloGpio_LedEnable(led->mppNumber);
#else
		BuffaloGpio_LedEnable(led->mppNumber);
#endif
		led->Brighted += 1;
		led->Brighting = LED_ON;
		LED_TRACE(printk("%s > change to LED_PHASE_1\n", __FUNCTION__));
		break;
	case LED_PHASE_SHORT:
		led->LedTimer.expires = (jiffies + BUFFALO_LED_WAIT_SHORT);
		BuffaloGpio_LedDisable(led->mppNumber);
		led->Brighting = LED_OFF;
		LED_TRACE(printk("%s > change to LED_PHASE_SHORT\n", __FUNCTION__));
		break;
	case LED_PHASE_LONG:
		led->LedTimer.expires = (jiffies + BUFFALO_LED_WAIT_LONG);
		BuffaloGpio_LedDisable(led->mppNumber);
		led->Brighting = LED_OFF;
		led->Brighted = 0;
		LED_TRACE(printk("%s > change to LED_PHASE_LONG\n", __FUNCTION__));
		break;
	}
	add_timer(&led->LedTimer);
	LED_TRACE(printk("%s > Leaving\n", __FUNCTION__));
}


static void
BuffaloLed_BlinkStart(struct led_t *led){
	LED_TRACE(printk("%s > Entered\n", __FUNCTION__));
	led->Brighted = 0;
	led->Brighting = LED_OFF;

	init_timer(&led->LedTimer);
	led->LedTimer.expires = (jiffies + BUFFALO_LED_WAIT_SHORT);
	led->LedTimer.function = BuffaloLed_StatChange;
	led->LedTimer.data = (unsigned long)led;
	add_timer(&led->LedTimer);
	LED_TRACE(printk("%s > Leaving\n", __FUNCTION__));
}


static void
BuffaloLed_BlinkStop(struct led_t *led)
{
	LED_TRACE(printk("%s > Entered\n", __FUNCTION__));
	if (led->ErrorNumber)
		del_timer(&led->LedTimer);
	led->ErrorNumber = 0;
	led->Brighted = 0;
	led->Brighting = LED_OFF;
	LED_TRACE(printk("%s > Leaving\n", __FUNCTION__));
}


static int
BuffaloLedWriteProc(struct file *filep, const char *buffer, unsigned long count, void *data)
{
	int TmpNum=0;
	struct led_t *led = data;
	LED_TRACE(printk("%s > Entered\n", __FUNCTION__));
	TmpNum = BuffaloKatoi(buffer, count);
	LED_TRACE(printk("%s > TmpNum=%d\n", __FUNCTION__, TmpNum));
	
	// if the status buffalo_led_status.alarm.ErrorNumber is 0, force on is enable.
	if (TmpNum == 0) {
		if (strncmp(buffer, "on", 2) == 0 ) {
			LED_TRACE(printk("%s > Calling BuffaloGpio_AlarmLedEnable\n", __FUNCTION__));
			BuffaloGpio_LedEnable(led->mppNumber);
		}
		else if (strncmp(buffer, "off", 3) == 0) {
			LED_TRACE(printk("%s > Calling BuffaloGpio_AlarmLedDisable\n", __FUNCTION__));
			BuffaloLed_BlinkStop(led);
			BuffaloGpio_LedDisable(led->mppNumber);
		}
		else if (strncmp(buffer, "inv", 3) == 0) {
			BuffaloGpio_LedInvert(led->mppNumber);
		}
	}
	else {
		if (led->ErrorNumber ==0) {
			if ((TmpNum > 0) && (TmpNum <= MAX_ALARM_NUM)) {
				led->ErrorNumber = TmpNum;
				BuffaloLed_BlinkStart(led);
			}
			else if (TmpNum == 0) {
				BuffaloLed_BlinkStop(led);
			}
			else {
				// Nothing to do...
			}
		}
	}
	LED_TRACE(printk("%s > Leaving\n", __FUNCTION__));
	return count;
}


static int
BuffaloLedBlinkReadProc(char *page, char **start, off_t offset, int length, int *eof, void *data)
{
	int len = 0;
	struct led_t *led = data;

	unsigned int blinkStat = buffalo_gpio_blink_reg_read();
	if (blinkStat & BIT(led->mppNumber))
		len = sprintf(page, "on\n");
	else
		len = sprintf(page, "off\n");

	return (len);
}


static int
BuffaloLedBlinkWriteProc(struct file *filep, const char *buffer, unsigned long count, void *data)
{
	struct led_t *led = data;

	if(strncmp(buffer, "on", 2) == 0 ){
		BuffaloGpio_LedBlinkEnable(led->mppNumber);
	}else if(strncmp(buffer, "off", 3) == 0){
		BuffaloGpio_LedBlinkDisable(led->mppNumber);
	}
	LED_TRACE(printk("%s > Leaving\n", __FUNCTION__));

	return count;
}



static int
BuffaloAllLedReadProc(char *page, char **start, off_t offset, int length, int *eof, void *data)
{
	int len = 0;
	off_t begin = 0;
	unsigned int PinStat = buffalo_gpio_read();

	if (BIT_LED_PWR >= 0) {
		if(~PinStat & BIT(BIT_LED_PWR))
			len += sprintf(page+len, "power led :on\n");
		else
			len += sprintf(page+len, "power led :off\n");
	}

	if (BIT_LED_INFO >=0) {
		if(~PinStat & BIT(BIT_LED_INFO))
			len += sprintf(page+len, "info  led :on\n");
		else
			len += sprintf(page+len, "info  led :off\n");
	}

	if (BIT_LED_ALARM >= 0) {
		if(~PinStat & BIT(BIT_LED_ALARM))
			len += sprintf(page+len, "alarm led :on\n");
		else
			len += sprintf(page+len, "alarm led :off\n");
	}

	if (BIT_LED_FUNC >= 0) {
		if(PinStat & BIT(BIT_LED_FUNC))
			len += sprintf(page+len, "func led :on\n");
		else
			len += sprintf(page+len, "func led :off\n");
	}

	if(len == 0)
		len += sprintf(page + len, "available led not found.\n");

	*start = page + (offset - begin);
	len -= (offset - begin);
	if (len > length)
		len = length;
	return (len);

}


static int
BuffaloLedReadProc(char *page, char **start, off_t offset, int length, int *eof, void *data)
{
	LED_TRACE(printk("%s > Entered\n", __FUNCTION__));

	int len = 0;
	off_t begin = 0;
	struct led_t *led = data;

	if (led->mppNumber < 0)
		return 0;
  
	if (led->ErrorNumber == 0) {
		if ((~GPIO_OUT_REG) & BIT(led->mppNumber))
			len = sprintf(page, "on\n");
		else
			len = sprintf(page, "off\n");
	}
	else {
#if defined BUFFALO_LED_CONTROL_INFO_BLOCK_ENABLE
		if (led == &infoLed && alarmLed.ErrorNumber != 0)
			len = sprintf(page, "%d(Blocked)\n", led->ErrorNumber);
		else
			len = sprintf(page, "%d\n", led->ErrorNumber);
#else
		len = sprintf(page, "%d\n", led->ErrorNumber);
#endif
	}

	*start = page + (offset - begin);
	len -= (offset - begin);
	if (len > length)
		len = length;

	LED_TRACE(printk("%s > Leaving\n", __FUNCTION__));
	return (len);
}

static int
BuffaloAllLedWriteProc(struct file *filep, const char *buffer, unsigned long count, void *data)
{
	if(strncmp(buffer, "on", 2) == 0 ){
		BuffaloGpio_AllLedOn();
	}else if(strncmp(buffer, "off", 3) == 0){
		BuffaloGpio_AllLedOff();
	}
	return count;
}



static int
BuffaloHddReadProc(char *page, char **start, off_t offset, int length, int *eof, void *data)
{
	int len = 0;
	off_t begin = 0;
	unsigned int PinStat = buffalo_gpio_read();

	if(PinStat & BIT(bitHddPower[(int)data]))
		len = sprintf(page, "on\n");
	else
		len = sprintf(page, "off\n");
	*start = page + (offset - begin);
	len -= (offset - begin);
	if (len > length)
		len = length;

	return (len);
}


static int
BuffaloHddWriteProc(struct file *filep, const char *buffer, unsigned long count, void *data)
{
	if(strncmp(buffer, "on", 2) == 0 ){
		BuffaloGpio_HddPowerOn(bitHddPower[(int)data]);
	}else if(strncmp(buffer, "off", 3) == 0){
		BuffaloGpio_HddPowerOff(bitHddPower[(int)data]);
	}
	return count;
}




static int
BuffaloUsbReadProc(char *page, char **start, off_t offset, int length, int *eof, void *data)
{
	int len = 0;
	off_t begin = 0;
	unsigned int PinStat = buffalo_gpio_read();

	if(PinStat & BIT(bitUsbPower[(int)data]))
		len = sprintf(page, "on\n");
	else
		len = sprintf(page, "off\n");
	*start = page + (offset - begin);
	len -= (offset - begin);
	if (len > length)
		len = length;

	return (len);
}


static int
BuffaloUsbWriteProc(struct file *filep, const char *buffer, unsigned long count, void *data)
{
	if(strncmp(buffer, "on", 2) == 0 ){
		BuffaloGpio_UsbPowerOn(bitUsbPower[(int)data]);
	}else if(strncmp(buffer, "off", 3) == 0){
		BuffaloGpio_UsbPowerOff(bitUsbPower[(int)data]);
	}
	return count;
}


static int
BuffaloPowerReadProc(char *page, char **start, off_t offset, int length, int *eof, void *data)
{
	if (BIT_POWER < 0)
		return 0;

	int len = 0;
	off_t begin = 0;
	unsigned int PinStat = buffalo_gpio_read();

	if(PinStat & BIT(BIT_POWER))
		len = sprintf(page, "on\n");
	else
		len = sprintf(page, "off\n");
	*start = page + (offset - begin);
	len -= (offset - begin);
	if (len > length)
		len = length;

	return (len);
}

static int
BuffaloSlideSwitchReadProc(char *page, char **start, off_t offset, int length)
{
	int len = 0;
	off_t begin = 0;

	if (use_slide_power == 1)
		len += sprintf(page, "1\n");
	else
		len += sprintf(page, "0\n");

	*start = page + (offset - begin);
	len -= (offset - begin);
	if (len > length)
		len = length;
	return (len);
}


static int
BuffaloAutoPowerReadProc(char *page, char **start, off_t offset, int length, int *eof, void *data)
{
	int len = 0;
	off_t begin = 0;
	unsigned int PinStat = buffalo_gpio_read();

	if(PinStat & BIT(BIT_AUTO_POWER))
		len = sprintf(page, "on\n");
	else
		len = sprintf(page, "off\n");
	*start = page + (offset - begin);
	len -= (offset - begin);
	if (len > length)
		len = length;
	return (len);
}

//#define SW_POLL_DEBUG
#ifdef SW_POLL_DEBUG
 #define SW_POLL_TRACE(x)	(x)
#else
 #define SW_POLL_TRACE(x)	/* */
#endif
static int
BuffaloSwPollingCheck(void)
{
	unsigned int PinStat = GPIO_IN_REG;
	unsigned int present_sw_status = SW_POWER_NS;
	static char msg[32];

	SW_POLL_TRACE(printk("PinStat = 0x%08x\n", PinStat));
	SW_POLL_TRACE(printk("PinStat & (BIT(BIT_POWER) | BIT(BIT_AUTO_POWER)) = 0x%08x\n", (PinStat & (BIT(BIT_POWER) | BIT(BIT_AUTO_POWER)))));
	SW_POLL_TRACE(printk("SW_PIN_STAT_OFF=0x%08x\n", SW_PIN_STAT_OFF));
	SW_POLL_TRACE(printk("SW_PIN_STAT_ON=0x%08x\n", SW_PIN_STAT_ON));
	SW_POLL_TRACE(printk("SW_PIN_STAT_AUTO=0x%08x\n", SW_PIN_STAT_AUTO));

	if (use_slide_power) {
		if(!(PinStat & BIT(BIT_POWER)) && !(PinStat & BIT(BIT_AUTO_POWER)))
			present_sw_status = SW_POWER_OFF;
		else if((PinStat & BIT(BIT_POWER)) && !(PinStat & BIT(BIT_AUTO_POWER)))
			present_sw_status = SW_POWER_ON;
		else if(!(PinStat & BIT(BIT_POWER)) && (PinStat & BIT(BIT_AUTO_POWER)))
			present_sw_status = SW_POWER_AUTO;
		else
			present_sw_status = SW_POWER_NS;
	}
	else {
		if(PinStat & BIT(BIT_AUTO_POWER))
			present_sw_status = SW_POWER_AUTO;
		else
			present_sw_status = SW_POWER_ON;
	}

	SW_POLL_TRACE(printk("present_sw_status = 0x%08x\n", present_sw_status));
	SW_POLL_TRACE(printk("g_buffalo_sw_status = 0x%08x\n", g_buffalo_sw_status));
	
	if(g_buffalo_sw_status != present_sw_status)
	{
		g_buffalo_sw_status = present_sw_status;
		memset(msg, 0, sizeof(msg));
		switch(g_buffalo_sw_status)
		{
			case SW_POWER_OFF:
				SW_POLL_TRACE(printk("%s> SW_POWER_OFF\n", __FUNCTION__));
				buffalo_kernevnt_queuein("PSW_off");;
				break;
			case SW_POWER_ON:
				SW_POLL_TRACE(printk("%s> SW_POWER_ON\n", __FUNCTION__));
				buffalo_kernevnt_queuein("PSW_on");;
				break;
			case SW_POWER_AUTO:
				SW_POLL_TRACE(printk("%s> SW_POWER_AUTO\n", __FUNCTION__));
				buffalo_kernevnt_queuein("PSW_auto");;
				break;
			case SW_POWER_NS:
				SW_POLL_TRACE(printk("%s> SW_POWER_NS\n", __FUNCTION__));
				buffalo_kernevnt_queuein("PSW_unknown");;
				break;
			default:
				break;
		}
		SW_POLL_TRACE(printk("%s\n", msg));
	}

	BuffaloSwPollingTimer.expires = (jiffies + SW_POLLING_INTERVAL);
	add_timer(&BuffaloSwPollingTimer);
	return 0;
}

static int
BuffaloSwPollingStop(void)
{
	del_timer(&BuffaloSwPollingTimer);
	return 0;
}

static int
BuffaloSwPollingStart(void)
{
	init_timer(&BuffaloSwPollingTimer);
	BuffaloSwPollingTimer.expires = (jiffies + SW_POLLING_INTERVAL);
	BuffaloSwPollingTimer.function = &BuffaloSwPollingCheck;
	BuffaloSwPollingTimer.data = 0;
	add_timer(&BuffaloSwPollingTimer);
	return 0;
}


static int
BuffaloSwPollingWriteProc(struct file *filep, const char *buffer, unsigned long count, void *data)
{
	FUNCTRACE(printk("%s> Entered\n", __FUNCTION__));
	if(strncmp(buffer, "on", 2) == 0 )
	{
		if(g_buffalo_sw_polling_control == 0)
		{
			g_buffalo_sw_polling_control = 1;
			g_buffalo_sw_status = SW_POWER_NS;
			BuffaloSwPollingStart();
		}
	}
	else if(strncmp(buffer, "off", 3) == 0)
	{
		if(g_buffalo_sw_polling_control == 1)
		{
			g_buffalo_sw_polling_control = 0;
			BuffaloSwPollingStop();
			g_buffalo_sw_status = SW_POWER_NS;
		}
	}
	return count;
}

static int
BuffaloSwPollingReadProc(char *page, char **start, off_t offset, int length, int *eof, void *data)
{
	int len = 0;
	off_t begin = 0;

	FUNCTRACE(printk("%s> Entered\n", __FUNCTION__));

	if(g_buffalo_sw_polling_control == 1)
	{
		len = sprintf(page, "on\n");
	}
	else
	{
		len = sprintf(page, "off\n");
	}

	*start = page + (offset - begin);
	len -= (offset - begin);
	if (len > length)
		len = length;
	return (len);
}



static int
BuffaloFuncReadProc(char *page, char **start, off_t offset, int length, int *eof, void *data)
{
	int len = 0;
	off_t begin = 0;
	unsigned int PinStat = buffalo_gpio_read();

	if(PinStat & BIT(BIT_FUNC))
		len = sprintf(page, "on\n");
	else
		len = sprintf(page, "off\n");
	*start = page + (offset - begin);
	len -= (offset - begin);
	if (len > length)
		len = length;
	return (len);

}



static int
BuffaloEthLedReadProc(char *page, char **start, off_t offset, int length, int *eof, void *data)
{
	if (BIT_LED_ETH < 0)
		return 0;

	int len = 0;
	off_t begin = 0;
	unsigned int PinStat = buffalo_gpio_read();

	if(PinStat & BIT(BIT_LED_ETH))
		len = sprintf(page, "on\n");
	else
		len = sprintf(page, "off\n");
	*start = page + (offset - begin);
	len -= (offset - begin);
	if(len > length)
		len = length;
	return (len);
}


static int
BuffaloEthLedWriteProc(struct file *filep, const char *buffer, unsigned long count, void *data){
	if(strncmp(buffer, "on", 2) == 0){
		BuffaloGpio_EthLedOn();
	}else if(strncmp(buffer, "off", 3) == 0){
		BuffaloGpio_EthLedOff();
	}
	return count;
}



static int
BuffaloFanStatusReadProc(char *page, char **start, off_t offset, int length, int *eof, void *data)
{
	if (BIT_FAN_LCK < 0)
		return 0;

	int len = 0;
	off_t begin = 0;
	
	unsigned int PinStat = buffalo_gpio_read();
	FUNCTRACE(printk(" %s > ~PinStat & BIT(BIT_FAN_LCK) = %d\n", __FUNCTION__, (~PinStat & BIT(BIT_FAN_LCK))));
	if(~PinStat & BIT(BIT_FAN_LCK))
		len = sprintf(page, "Fine\n");
	else
		len = sprintf(page, "Stop\n");
	*start = page + (offset - begin);
	len -= (offset - begin);
	if(len > length)
		len = length;
	return (len);
}



static int
BuffaloFanStatusWriteProc(struct file *filep, const char *buffer, unsigned long count, void *data){
	if(strncmp(buffer, "on", 2) == 0){
		//Nothing to do ...
	}else if(strncmp(buffer, "off", 3) == 0){
		//Nothing to do ...
	}
	return count;
}



static int
BuffaloFanControlWriteProc(struct file *filep, const char *buffer, unsigned long count, void *data)
{
	if(strncmp(buffer, "stop", strlen("stop")) == 0)
	{
		BuffaloGpio_FanStop();
	}
	else if(strncmp(buffer, "slow", strlen("slow")) == 0)
	{
		BuffaloGpio_FanSlow();
	}
	else if(strncmp(buffer, "fast", strlen("fast")) == 0)
	{
		BuffaloGpio_FanFast();
	}
	else if(strncmp(buffer, "full", strlen("full")) == 0)
	{
		BuffaloGpio_FanFull();
	}
	return count;
}

static int
BuffaloFanControlReadProc(char *page, char **start, off_t offset, int length, int *eof, void *data)
{
	int len = 0;
	off_t begin = 0;

	unsigned int fan_status = BuffaloGpio_FanControlRead();

	switch(fan_status){
	case BUFFALO_GPIO_FAN_STOP:
		len += sprintf(page, "stop\n");
		break;
	case BUFFALO_GPIO_FAN_SLOW:
		len += sprintf(page, "slow\n");
		break;
	case BUFFALO_GPIO_FAN_FAST:
		len += sprintf(page, "fast\n");
		break;
	case BUFFALO_GPIO_FAN_FULL:
		len += sprintf(page, "full\n");
		break;
	default:
		break;
	}// end of switch

	*start = page + (offset - begin);
	len -= (offset - begin);
	if(len > length)
		len = length;
	return (len);
}



static int BuffaloCpuStatusReadProc(char *page, char **start, off_t offset, int length, int *eof, void *data)
{
	int len = 0;
	off_t begin = 0;
	unsigned int CpuStatus = BuffaloGpio_ChangePowerStatus(0);

	switch(CpuStatus){
	case MagicKeyReboot:
		len = sprintf(page, "reboot\n");
		break;
	case MagicKeyRebootUbootPassed:
		len = sprintf(page, "reboot_uboot_passed\n");
		break;
	case MagicKeyNormalState:
		len = sprintf(page, "normal_state\n");
		break;
	case MagicKeyHwPoff:
		len = sprintf(page, "hwpoff\n");
		break;
	case MagicKeySwPoff:
		len = sprintf(page, "swpoff\n");
		break;
	case MagicKeySWPoffUbootPassed:
		len = sprintf(page, "swpoff_uboot_passed\n");
		break;
	case MagicKeyFWUpdating:
		len = sprintf(page, "fwup\n");
		break;
	case MagicKeyUpsShutdown:
		len = sprintf(page, "ups_shutdown\n");
		break;
	default:
		len = sprintf(page, "Unknown(CpuStatus=%d)\n", CpuStatus);
		break;
	}

	*start = page + (offset - begin);
	len -= (offset - begin);
	if (len > length)
		len = length;
	return (len);

}

static int BuffaloCpuStatusWriteProc(struct file *filep, const char *buffer, unsigned long count, void *data){
	
	if(strncmp(buffer, "reboot", 6) == 0){
		PST_TRACE(printk("%s > setting reboot\n", __FUNCTION__));
		BuffaloGpio_ChangePowerStatus(POWER_STATUS_REBOOTING);
	}else if(strncmp(buffer, "reboot_uboot_passed", 19) == 0){
		PST_TRACE(printk("%s > setting Reboot_uboot_passed\n", __FUNCTION__));
		BuffaloGpio_ChangePowerStatus(POWER_STATUS_REBOOTING_UBOOT_PASSED);
	}else if(strncmp(buffer, "normal_state", 12) == 0){
		PST_TRACE(printk("%s > setting normal_state\n", __FUNCTION__));
		BuffaloGpio_ChangePowerStatus(POWER_STATUS_NORMAL_STATE);
	}else if(strncmp(buffer, "hwpoff", 6) == 0){
		PST_TRACE(printk("%s > setting hwpoff\n", __FUNCTION__));
		BuffaloGpio_ChangePowerStatus(POWER_STATUS_HW_POWER_OFF);
	}else if(strncmp(buffer, "swpoff", 6) == 0){
		PST_TRACE(printk("%s > setting swpoff\n", __FUNCTION__));
		BuffaloGpio_ChangePowerStatus(POWER_STATUS_SW_POWER_OFF);
	}else if(strncmp(buffer, "swpoff_uboot_passed", 19) == 0){
		PST_TRACE(printk("%s > setting swpoff_uboot_passed\n", __FUNCTION__));
		BuffaloGpio_ChangePowerStatus(POWER_STATUS_SW_POFF_UBOOT_PASSED);
	}else if(strncmp(buffer, "fwup", 4) == 0){
		PST_TRACE(printk("%s > setting fwup\n", __FUNCTION__));
		BuffaloGpio_ChangePowerStatus(POWER_STATUS_FWUPDATING);
	}else if(strncmp(buffer, "ups_shutdown", 12) == 0){
		PST_TRACE(printk("%s > setting ups_shutdown\n", __FUNCTION__));
		BuffaloGpio_ChangePowerStatus(POWER_STATUS_UPS_SHUTDOWN);
	}else{
		PST_TRACE(printk("%s > no meaning...(%s)\n", __FUNCTION__, buffer));
	}

	return count;
}

static int
BuffaloGpioRegisterReadProc(char *page, char **start, off_t offset, int count, int *eof, void *data)
{
	return sprintf(page, "0x%08X\n", *((unsigned long*)(0xF1000000 + (unsigned long)data)));
}

static int
BuffaloGpioRegisterWriteProc(struct file *filep, const char *buffer, unsigned long count, void *data)
{
	*((unsigned long*)(0xF1000000 + (unsigned long)data)) = simple_strtoul(buffer, NULL, 16);
	return count;
}


static int
BuffaloPMHddPowerReadProc(char *page, char **start, off_t offset, int length, int *eof, void *data)
{
	uint32_t pm_hdd_power;
	if (BuffaloReadPmGpio(&pm_hdd_power) != 0)
		return 0;

	pm_hdd_power = (pm_hdd_power & 0xff00) >> 8;

	if (pm_hdd_power & BIT((int)data))
		return sprintf(page, "on\n");
	else
		return sprintf(page, "off\n");
}


static int
BuffaloPMHddPowerWriteProc(struct file *filep, const char *buffer, unsigned long count, void *data)
{
	uint32_t pm_hdd_power;
	
	if (BuffaloReadPmGpio(&pm_hdd_power) != 0)
		return 0;

	if (strncmp(buffer, "on", 2) == 0 ){
		pm_hdd_power |= BIT((int)data + 8);
	}
	else if(strncmp(buffer, "off", 3) == 0){
		pm_hdd_power &= ~BIT((int)data + 8);
	}

	if (BuffaloWritePmGpio(&pm_hdd_power) != 0)
		return 0;

	return count;
}


static int
BuffaloPMHddDiagLedReadProc(char *page, char **start, off_t offset, int length, int *eof, void *data)
{
	uint32_t pm_diag_led;
	
	if (BuffaloReadPmGpio(&pm_diag_led) != 0)
		return 0;

	pm_diag_led = (pm_diag_led & 0xff00) >> 12;

	if (pm_diag_led & BIT((int)data))
		return sprintf(page, "off\n");
	else
		return sprintf(page, "on\n");
}


static int
BuffaloPMHddDiagLedWriteProc(struct file *filep, const char *buffer, unsigned long count, void *data)
{
	uint32_t pm_diag_led;

	if (BuffaloReadPmGpio(&pm_diag_led) != 0)
		return 0;

	if (strncmp(buffer, "on", 2) == 0 ){
		pm_diag_led &= ~BIT((int)data + 12);
	}
	else if(strncmp(buffer, "off", 3) == 0){
		pm_diag_led |= BIT((int)data + 12);
	}

	if (BuffaloWritePmGpio(&pm_diag_led) != 0)
		return 0;

	return count;
}


//----------------------------------------------------------------------
int __init buffaloLedDriver_init (void)
{
	int i;
	char buf[1024];
	struct proc_dir_entry *pde;

	FUNCTRACE(printk("%s > Entered.\n",__FUNCTION__));

	BuffaloGpio_Init();
	pde = proc_mkdir("buffalo/gpio", 0);
	pde = proc_mkdir("buffalo/gpio/led", 0);
	pde = proc_mkdir("buffalo/gpio/switch", 0);
	pde = proc_mkdir("buffalo/gpio/fan", 0);
	pde = proc_mkdir("buffalo/gpio/power_control", 0);
	pde = create_proc_info_entry ("buffalo/power_sw", 0, 0, gpio_power_read_proc);

	pde = create_proc_entry("buffalo/gpio/led/all", 0, 0);
	pde->read_proc = BuffaloAllLedReadProc;
	pde->write_proc= BuffaloAllLedWriteProc;

	pde = create_proc_entry("buffalo/cpu_status", 0, 0);
	pde->read_proc = BuffaloCpuStatusReadProc;
	pde->write_proc = BuffaloCpuStatusWriteProc;


	if (BIT_LED_ALARM >= 0) {
		BuffaloGpio_AlarmLedDisable();

		pde = create_proc_entry("buffalo/gpio/led/alarm", 0, 0);
		pde->read_proc = BuffaloLedReadProc;
		pde->write_proc = BuffaloLedWriteProc;
		alarmLed.mppNumber = BIT_LED_ALARM;
		pde->data = &alarmLed;

		pde = create_proc_entry("buffalo/gpio/led/alarm_blink", 0, 0);
		pde->read_proc = BuffaloLedBlinkReadProc;
		pde->write_proc = BuffaloLedBlinkWriteProc;
		pde->data = &alarmLed;
	}

	if (BIT_LED_INFO >= 0) {
		BuffaloGpio_InfoLedDisable();

		pde = create_proc_entry("buffalo/gpio/led/info", 0, 0);
		pde->read_proc = BuffaloLedReadProc;
		pde->write_proc = BuffaloLedWriteProc;
		infoLed.mppNumber = BIT_LED_INFO;
		pde->data = &infoLed;

		pde = create_proc_entry("buffalo/gpio/led/info_blink", 0, 0);
		pde->read_proc = BuffaloLedBlinkReadProc;
		pde->write_proc = BuffaloLedBlinkWriteProc;
		pde->data = &infoLed;
	}

	if (BIT_LED_PWR >= 0) {
		BuffaloGpio_PowerLedEnable();

		pde = create_proc_entry("buffalo/gpio/led/power", 0, 0);
		pde->read_proc = BuffaloLedReadProc;
		pde->write_proc = BuffaloLedWriteProc;
		powerLed.mppNumber = BIT_LED_PWR;
		pde->data = &powerLed;

		pde = create_proc_entry("buffalo/gpio/led/power_blink", 0, 0);
		pde->read_proc = BuffaloLedBlinkReadProc;
		pde->write_proc = BuffaloLedBlinkWriteProc;
		pde->data = &powerLed;
	}

	if (BIT_LED_FUNC >= 0) {
		BuffaloGpio_FuncLedDisable();

		pde = create_proc_entry("buffalo/gpio/led/func", 0, 0);
		pde->read_proc = BuffaloLedReadProc;
		pde->write_proc = BuffaloLedWriteProc;
		funcLed.mppNumber = BIT_LED_FUNC;
		pde->data = &funcLed;

		pde = create_proc_entry("buffalo/gpio/led/func_blink", 0, 0);
		pde->read_proc = BuffaloLedBlinkReadProc;
		pde->write_proc = BuffaloLedBlinkWriteProc;
		pde->data = &funcLed;
	}

	if (BIT_LED_ETH >= 0) {
		pde = create_proc_entry("buffalo/gpio/led/eth", 0, 0);
		pde->read_proc = BuffaloEthLedReadProc;
		pde->write_proc = BuffaloEthLedWriteProc;
	}


	for (i=0; i<BUF_NV_SIZE_BIT_HDD_POWER; i++) {
		if (bitHddPower[i] < 0)
			continue;

		sprintf(buf, "buffalo/gpio/power_control/hdd%d", i);
		pde = create_proc_entry(buf, 0, 0);
		if (pde != NULL) {
			pde->read_proc = BuffaloHddReadProc;
			pde->write_proc= BuffaloHddWriteProc;
			pde->owner = THIS_MODULE;
			pde->data = (void *)i;
		}
	}

	for (i=0; i<BUF_NV_SIZE_BIT_USB_POWER; i++) {
		if (bitUsbPower[i] < 0)
			continue;

		sprintf(buf, "buffalo/gpio/power_control/usb%d", i);
		pde = create_proc_entry(buf, 0, NULL);
		if (pde != NULL) {
			pde->read_proc = BuffaloUsbReadProc;
			pde->write_proc = BuffaloUsbWriteProc;
			pde->owner = THIS_MODULE;
			pde->data = (void *)i;
		}
	}

	if (use_slide_power == 1) {
		pde = create_proc_entry("buffalo/gpio/switch/power", 0, 0);
		pde->read_proc = BuffaloPowerReadProc;
//		pde->write_proc = BuffaloPowerReadProc;

		pde = create_proc_info_entry("buffalo/gpio/switch/slide_switch", 0, 0, BuffaloSlideSwitchReadProc);
	}


	if (BIT_AUTO_POWER >= 0) {
		pde = create_proc_entry("buffalo/gpio/switch/auto_power", 0, 0);
		pde->read_proc = BuffaloAutoPowerReadProc;
//		pde->write_proc = BuffaloAutoPowerReadProc;
	}

	if (use_slide_power == 1 || BIT_AUTO_POWER >= 0) {
		pde = create_proc_entry("buffalo/gpio/switch/sw_control", 0, 0);
		pde->read_proc = BuffaloSwPollingReadProc;
		pde->write_proc = BuffaloSwPollingWriteProc;
	}

	if (BIT_FUNC >= 0) {
		pde = create_proc_entry("buffalo/gpio/switch/func", 0, 0);
		pde->read_proc = BuffaloFuncReadProc;
		//		pde->write_proc= BuffaloFuncReadProc;
	}

	if (BIT_FAN_LOW >= 0 && BIT_FAN_HIGH) {
		pde = create_proc_entry("buffalo/gpio/fan/control", 0, 0);
		pde->read_proc = BuffaloFanControlReadProc;
		pde->write_proc= BuffaloFanControlWriteProc;
	}

	if (BIT_FAN_LCK >= 0) {
		pde = create_proc_entry("buffalo/gpio/fan/lock", 0, 0);
		pde->read_proc = BuffaloFanStatusReadProc;
		pde->write_proc= BuffaloFanStatusWriteProc;
	}

	pde = create_proc_entry("buffalo/gpio/data_out_register", 0, 0);
	pde->read_proc = BuffaloGpioRegisterReadProc;
	pde->write_proc= BuffaloGpioRegisterWriteProc;
	pde->data = (void *)GPP_DATA_OUT_REG(0);
	
	pde = create_proc_entry("buffalo/gpio/data_out_enable_register", 0, 0);
	pde->read_proc = BuffaloGpioRegisterReadProc;
	pde->write_proc= BuffaloGpioRegisterWriteProc;
	pde->data = (void *)GPP_DATA_OUT_EN_REG(0);
	
	pde = create_proc_entry("buffalo/gpio/blink_enable_register", 0, 0);
	pde->read_proc = BuffaloGpioRegisterReadProc;
	pde->write_proc= BuffaloGpioRegisterWriteProc;
	pde->data = (void *)GPP_BLINK_EN_REG(0);
	
	pde = create_proc_entry("buffalo/gpio/data_in_polarity_register", 0, 0);
	pde->read_proc = BuffaloGpioRegisterReadProc;
	pde->write_proc= BuffaloGpioRegisterWriteProc;
	pde->data = (void *)GPP_DATA_IN_POL_REG(0);
	
	pde = create_proc_entry("buffalo/gpio/data_in_register", 0, 0);
	pde->read_proc = BuffaloGpioRegisterReadProc;
	pde->write_proc= BuffaloGpioRegisterWriteProc;
	pde->data = (void *)GPP_DATA_IN_REG(0);
	
	if (Buffalo_has_PM()) {
		for (i=0; i<4; i++) {
			sprintf(buf, "buffalo/gpio/power_control/hdd%d", i);
			pde = create_proc_entry(buf, 0, 0);
			if (pde != NULL) {
				pde->read_proc = BuffaloPMHddPowerReadProc;
				pde->write_proc= BuffaloPMHddPowerWriteProc;
				pde->owner = THIS_MODULE;
				pde->data = (void *)i;
			}

			sprintf(buf, "buffalo/gpio/led/pm_diag_led%d", i);
			pde = create_proc_entry(buf, 0, 0);
			if (pde != NULL) {
				pde->read_proc = BuffaloPMHddDiagLedReadProc;
				pde->write_proc= BuffaloPMHddDiagLedWriteProc;
				pde->owner = THIS_MODULE;
				pde->data = (void *)i;
			}
		}
	}

	printk("%s %s Ver.%s installed.\n", DRIVER_NAME, AUTHOR, BUFFALO_DRIVER_VER);
	return 0;
}

//----------------------------------------------------------------------
void buffaloLedDriver_exit(void)
{
	int i;
	char buf[1024];
	
	FUNCTRACE(printk(">%s\n",__FUNCTION__));
	if (BIT_LED_INFO >= 0) {
		BuffaloLed_BlinkStop(&infoLed);
		remove_proc_entry("buffalo/gpio/led/info", 0);
		remove_proc_entry("buffalo/gpio/led/info_blink", 0);
	}

	if (BIT_LED_PWR >= 0) {
		BuffaloLed_BlinkStop(&powerLed);
		remove_proc_entry("buffalo/gpio/led/power", 0);
		remove_proc_entry("buffalo/gpio/led/power_blink", 0);
	}

	if (BIT_LED_ALARM >= 0) {
		BuffaloLed_BlinkStop(&alarmLed);
		remove_proc_entry("buffalo/gpio/led/alarm", 0);
		remove_proc_entry("buffalo/gpio/led/alarm_blink", 0);
	}

	if (BIT_FAN_LOW >= 0 && BIT_FAN_HIGH >= 0) {
		remove_proc_entry("buffalo/gpio/fan/control", 0);
	}

	if (BIT_FAN_LCK >= 0) {
		remove_proc_entry("buffalo/gpio/fan/lock", 0);
	}

	remove_proc_entry("buffalo/gpio/fan", 0);

	if (BIT_LED_FUNC >= 0) {
		remove_proc_entry("buffalo/gpio/led/func", 0);
		remove_proc_entry("buffalo/gpio/led/func_blink", 0);
	}

	if (BIT_LED_ETH >= 0) {
		remove_proc_entry("buffalo/gpio/led/eth", 0);
	}

	remove_proc_entry("buffalo/gpio/led/all", 0);
	remove_proc_entry("buffalo/gpio/led", 0);

	if (BIT_FUNC >= 0) {
		remove_proc_entry("buffalo/gpio/switch/func", 0);
	}

	if (BIT_AUTO_POWER >= 0) {
		remove_proc_entry("buffalo/gpio/switch/auto_power", 0);
	}

	if (use_slide_power == 1) {
		remove_proc_entry("buffalo/gpio/switch/power", 0);
		remove_proc_entry("buffalo/gpio/switch/sw_control", 0);
	}

	remove_proc_entry("buffalo/gpio/switch", 0);


	for (i=0; i<BUF_NV_SIZE_BIT_HDD_POWER; i++) {
		if (bitHddPower[i] >= 0) {
			sprintf(buf, "buffalo/gpio/power_control/hdd%d", i);
			remove_proc_entry(buf, 0);
		}
	}

	for (i=0; i<BUF_NV_SIZE_BIT_USB_POWER; i++) {
		if (bitUsbPower[i] >= 0) {
			sprintf(buf, "buffalo/gpio/power_control/usb%d", i);
			remove_proc_entry(buf, 0);
		}
	}

	remove_proc_entry("buffalo/gpio/power_control", 0);
	remove_proc_entry("buffalo/gpio", 0);

	remove_proc_entry ("buffalo", 0);
}

module_init(buffaloLedDriver_init);
module_exit(buffaloLedDriver_exit);
