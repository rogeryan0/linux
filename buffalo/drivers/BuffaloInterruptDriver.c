
#include <linux/config.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/timer.h>

#include "kernevnt.h"
#include "BuffaloGpio.h"

#define AUTHOR	"(C) BUFFALO INC."
#define DRIVER_NAME	"Buffalo CPU Inerupts Driver"
#define BUFFALO_DRIVER_VER	"0.01 alpha1"

MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DRIVER_NAME);
MODULE_LICENSE("GPL");
MODULE_VERSION(BUFFALO_DRIVER_VER);

#define CONFIG_BUFFALO_USE_KERNEL_EVENT_DRIVER

//#define CONFIG_BUFFALO_DEBUG_GPIO_INTERRUPT_DRIVER
#ifdef CONFIG_BUFFALO_DEBUG_GPIO_INTERRUPT_DRIVER
	#define TRACE(x)	x
#else
	#define TRACE(x)
#endif

#define POLLING_COUNTS_PER_1SEC	10

#define SW_PUSHED		(1 << 0)	// 0x01
#define SW_LONG_PUSHED		(1 << 1)	// 0x02

#if !defined(CONFIG_BUFFALO_USE_SWITCH_SLIDE_POWER)
static int g_irq=0;
static unsigned int power_pushed_status;

#define PSW_IRQ	(32 + BIT_POWER)
#define PSW_POL_MSEC	(HZ / POLLING_COUNTS_PER_1SEC) 	// 50 mili second
#define PSW_WAIT_SECOND	(1 * HZ)			// 3 seccond
#define PSW_LONG_WAIT_COUNT	(PSW_WAIT_SECOND / PSW_POL_MSEC)	// PSW_WAIT_SECOND / PSW_POL_MSEC

#define PSW_WAIT_COUNT	(0 * PSW_POL_MSEC) // 0.3(6 * 0.05) second wait

static struct timer_list PSWPollingTimer;
static void PollingTimerGoOn(void);
static void PollingTimerStop(void);
#endif

// for init sw
#if defined(CONFIG_BUFFALO_USE_SWITCH_INIT)
 static int g_irq_init=0;
 static unsigned int init_pushed_status;

 #define INIT_IRQ (32 + BIT_INIT)

 #define INIT_POL_MSEC	(HZ / POLLING_COUNTS_PER_1SEC)
 #define INIT_WAIT_SECOND	(5 * HZ)
 #define INIT_WAIT_COUNT	(INIT_WAIT_SECOND / INIT_POL_MSEC)

 static struct timer_list INITPollingTimer;
 static void PollingTimerGoOnInit(void);
 static void PollingTimerStopInit(void);
#endif // CONFIG_BUFFALO_USE_SWITCH_INIT

// for Func sw
#if defined(CONFIG_BUFFALO_USE_SWITCH_FUNC)
 static int irq_func=0;
 static unsigned int func_pushed_status;
 #define FUNC_IRQ (32 + BIT_FUNC)

 #define FUNC_POL_MSEC		(HZ / POLLING_COUNTS_PER_1SEC)
 #define FUNC_WAIT_SECOND	(1 * HZ)
 #define FUNC_WAIT_COUNT	(0)
 #define FUNC_LONG_WAIT_COUNT	(FUNC_WAIT_SECOND / FUNC_POL_MSEC)

 static struct timer_list FuncPollingTimer;
 static void PollingTimerGoOnFunc(void);
 static void PollingTimerStopFunc(void);
#endif // CONFIG_BUFFALO_USE_SWITCH_FUNC

#if !defined(CONFIG_BUFFALO_USE_SWITCH_SLIDE_POWER)
static int PollingPowerSWStatus(unsigned long data){
	
	TRACE(printk(">%s, data=%u, PSW_LONG_WAIT_COUNT=%d\n", __FUNCTION__, data, PSW_LONG_WAIT_COUNT));
	unsigned int PowerStat = buffalo_gpio_read();

	if(PowerStat & BIT(BIT_POWER)){
		if((data >= PSW_LONG_WAIT_COUNT) && !(power_pushed_status & SW_LONG_PUSHED)){
			power_pushed_status |= SW_LONG_PUSHED;
#if defined CONFIG_BUFFALO_USE_KERNEL_EVENT_DRIVER
			buffalo_kernevnt_queuein("PSW_pushed");
#else

#endif
		}else if((data >= PSW_WAIT_COUNT) && !(power_pushed_status & SW_PUSHED)){
			power_pushed_status |= SW_PUSHED;
#if defined CONFIG_BUFFALO_USE_KERNEL_EVENT_DRIVER
			buffalo_kernevnt_queuein("PSW_short_pushed");
#else

#endif
		}
		PollingTimerGoOn();
	}else{
		PollingTimerStop();
		if(g_irq)
			enable_irq(g_irq);

		power_pushed_status = 0;
	}

	return 0;
}

static void PollingTimerStop(void){
	del_timer(&PSWPollingTimer);
}

static void PollingTimerGoOn(void){
	PSWPollingTimer.expires=(jiffies + PSW_POL_MSEC);
	PSWPollingTimer.data+=1;
	add_timer(&PSWPollingTimer);
}

static void PollingTimerStart(void){
	init_timer(&PSWPollingTimer);
	PSWPollingTimer.expires=(jiffies + PSW_POL_MSEC);
	PSWPollingTimer.function=&PollingPowerSWStatus;
	PSWPollingTimer.data=0;
	add_timer(&PSWPollingTimer);
}

static int CpuIntActivate_read_proc(char *page, char **start, off_t offset, int length)
{
	const char *msg="PowerIntAct\n";
	
	if(offset>0)
		return 0;

	*start=page+offset;
	strcpy(page, msg);

	if(g_irq){
		disable_irq(g_irq);
		enable_irq(g_irq);
	}

	return (strlen(msg));
}


static int PowSwInterrupts(int irq, void *dev_id, struct pt_regs *reg)
{
	TRACE(printk(">%s\n", __FUNCTION__));
	//Process to checking power sw pushed or not.

	disable_irq(irq);
	g_irq=irq;

	// Setup PollingPowerSWStatus;
	PollingTimerStart();
	TRACE(printk(">%s\n", __FUNCTION__));

	return IRQ_HANDLED;
}
#endif


#if defined(CONFIG_BUFFALO_USE_SWITCH_INIT)
static int
PollingINITSWStatus(unsigned long data){
	
	TRACE(printk(">%s, data=%u, INIT_WAIT_COUNT=%d\n", __FUNCTION__, data, INIT_WAIT_COUNT));
	unsigned int InitStat=buffalo_gpio_read();

	if(InitStat & BIT(BIT_INIT)){
		if((data > INIT_WAIT_COUNT) && !(init_pushed_status & SW_PUSHED)){
			init_pushed_status |= SW_PUSHED;
#if defined CONFIG_BUFFALO_USE_KERNEL_EVENT_DRIVER
			buffalo_kernevnt_queuein("INITSW_pushed");
#else

#endif
			// keep Polling untile button would be released.
			TRACE(printk("initialize start\n"));
		}
			PollingTimerGoOnInit();
	}else{
		PollingTimerStopInit();
		if(g_irq_init)
			enable_irq(g_irq_init);
		init_pushed_status = 0;
	}

	return 0;
}


static void
PollingTimerStopInit(void){
	del_timer(&INITPollingTimer);
}


static void
PollingTimerGoOnInit(void){
	INITPollingTimer.expires=(jiffies + INIT_POL_MSEC);
	INITPollingTimer.data+=1;
	add_timer(&INITPollingTimer);
}


static void
PollingTimerStartInit(void){
	init_timer(&INITPollingTimer);
	INITPollingTimer.expires=(jiffies + INIT_POL_MSEC);
	INITPollingTimer.function=&PollingINITSWStatus;
	INITPollingTimer.data=0;
	add_timer(&INITPollingTimer);
}

static int
InitSwInterrupts(int irq, void *dev_id, struct pt_regs *reg)
{
	TRACE(printk(">%s\n", __FUNCTION__));

	disable_irq(irq);
	g_irq_init=irq;

	PollingTimerStartInit();
	TRACE(printk(">%s\n", __FUNCTION__));

	return IRQ_HANDLED;
}
#endif //CONFIG_BUFFALO_USE_SWITCH_INIT

#if defined(CONFIG_BUFFALO_USE_SWITCH_FUNC)
static int
PollingFuncSWStatus(unsigned long data)
{
	TRACE(printk(">%s, data=%u, FUNC_WAIT_COUNT=%d\n", __FUNCTION__, data, FUNC_WAIT_COUNT));
	unsigned int FuncStat = buffalo_gpio_read();

	if (FuncStat & BIT(BIT_FUNC)){
		func_pushed_status |= SW_PUSHED;
		if ((data > FUNC_LONG_WAIT_COUNT) && !(func_pushed_status & SW_LONG_PUSHED)){
			func_pushed_status |= SW_LONG_PUSHED;
#if defined CONFIG_BUFFALO_USE_KERNEL_EVENT_DRIVER
			buffalo_kernevnt_queuein("FUNCSW_long_pushed");
#endif
		}
		PollingTimerGoOnFunc();
	}else{
		PollingTimerStopFunc();
		if (irq_func)
			enable_irq(irq_func);
		
		if ((data > FUNC_WAIT_COUNT) && !(func_pushed_status ^ SW_PUSHED)) {
#if defined CONFIG_BUFFALO_USE_KERNEL_EVENT_DRIVER
			buffalo_kernevnt_queuein("FUNCSW_pushed");
#endif
		}
		func_pushed_status = 0;
	}

	return 0;
}

static void
PollingTimerStopFunc(void)
{
	del_timer(&FuncPollingTimer);
}

static void
PollingTimerGoOnFunc(void)
{
	FuncPollingTimer.expires=(jiffies + FUNC_POL_MSEC);
	FuncPollingTimer.data+=1;
	add_timer(&FuncPollingTimer);
}

static void
PollingTimerStartFunc(void)
{
	init_timer(&FuncPollingTimer);
	FuncPollingTimer.expires=(jiffies + FUNC_POL_MSEC);
	FuncPollingTimer.function=&PollingFuncSWStatus;
	FuncPollingTimer.data=0;
	add_timer(&FuncPollingTimer);
}

static int
FuncSwInterrupts(int irq, void *dev_id, struct pt_regs *reg)
{
	TRACE(printk(">%s\n", __FUNCTION__));

	disable_irq(irq);
	irq_func=irq;

	PollingTimerStartFunc();
	TRACE(printk(">%s\n", __FUNCTION__));

	return IRQ_HANDLED;
}
#endif /* CONFIG_BUFFALO_USE_SWITCH_FUNC */

/*
 * Initialize driver.
 */
int __init BuffaloCpuInterrupts_init(void)
{
	TRACE(printk(">%s\n", __FUNCTION__));

#if !defined(CONFIG_BUFFALO_USE_SWITCH_SLIDE_POWER)
	struct proc_dir_entry *buffalo_cpu_int_proc;

	if((buffalo_cpu_int_proc = create_proc_info_entry("buffalo/PowerSWInt_en", 0, 0, CpuIntActivate_read_proc))==0){
		proc_mkdir("buffalo", 0);
		buffalo_cpu_int_proc = create_proc_info_entry("buffalo/PowerSWInt_en", 0, 0, CpuIntActivate_read_proc);
	}

	BuffaloGpio_CPUInterruptsSetup();
	request_irq(PSW_IRQ, PowSwInterrupts, 0, "PowSw", NULL);
#endif

#if defined(CONFIG_BUFFALO_USE_SWITCH_INIT)
	BuffaloGpio_CPUInterruptsSetupInit();
	request_irq(INIT_IRQ, InitSwInterrupts, 0, "InitSw", NULL);
#endif

#if defined(CONFIG_BUFFALO_USE_SWITCH_FUNC)
	BuffaloGpio_CPUInterruptsSetupFunc();
	request_irq(FUNC_IRQ, FuncSwInterrupts, 0, "FuncSw", NULL);
#endif

	printk("%s %s Ver.%s installed.\n", DRIVER_NAME, AUTHOR, BUFFALO_DRIVER_VER);
	return 0;
}

void BuffaloCpuInterrupts_exit(void)
{
	TRACE(printk(">%s\n", __FUNCTION__));
#if !defined(CONFIG_BUFFALO_USE_SWITCH_SLIDE_POWER)
	free_irq(PSW_IRQ, NULL);
	PollingTimerStop();
#endif
	remove_proc_entry("buffalo/PowerSWInt_en", 0);
	printk("%s %s uninstalled.\n", DRIVER_NAME, BUFFALO_DRIVER_VER);
}

module_init(BuffaloCpuInterrupts_init);
module_exit(BuffaloCpuInterrupts_exit);

