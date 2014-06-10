#include <linux/list.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/sysdev.h>
#include <linux/completion.h>

#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/setup.h>
#include <asm/mach-types.h>

#include <asm/mach/arch.h>
#include <asm/mach/irq.h>

#include "BuffaloGpio.h"
#include "mvGpp.h"
#include "mvGppRegs.h"

#include "mvCpuIfRegs.h"

#include "mvDS1339.h"
#include "mvDS1339Reg.h"
#include "mvStorageDev.h"
#include "mvSata/mvLinuxIalHt.h"
#include "mvSata/mvIALCommonUtils.h"

#if defined CONFIG_BUFFALO_USE_GPIO_DRIVER
 #include "mvEthPhy.h"
#endif

#define MagicKeyAPC	0x45
#define MagicKeyOMR	0x3a
#define MagicKeyUSB	0x16

/*
	GPIO-setup
	LS-GL	BIT2 : micon-int
			BIT3 : RTC-int
	TS-HTGL
			BIT8 : micon-int
			BIT9 : RTC-int
			BIT13: UPS-SerailPort
			BIT14: OMRON_UPS Battery Low detection
*/


#if defined CONFIG_BUFFALO_USE_MICON
//----------------------------------------------------------------------
// micon
//----------------------------------------------------------------------
void BuffaloGpio_MiconIntSetup(void)
{
	mvGppTypeSet(0, BIT(BIT_MICON) , BIT(BIT_MICON));		// disable output
	mvGppPolaritySet(MICON_POL, BIT(BIT_MICON) , BIT(BIT_MICON));
	// set GPIO-2 : edge IRQ
	///set_irq_handler(32+BIT_MICON, do_edge_IRQ);
}

//----------------------------------------------------------------------
void BuffaloGpio_ClearMiconInt(void)
{
	unsigned cause;
	//printk(">%s\n",__FUNCTION__);
	/*
	printk("MPP_CONTROL_REG  %x %x %x %x\n"
		,MV_REG_READ(mvCtrlMppRegGet(0))
		,MV_REG_READ(mvCtrlMppRegGet(1))
		,MV_REG_READ(mvCtrlMppRegGet(2))
		,MV_REG_READ(mvCtrlMppRegGet(3))
		);
	*/
	
	cause = MV_REG_READ(GPP_INT_CAUSE_REG(0));
	//printk("cause:%x\n",cause);
	MV_REG_WRITE(GPP_INT_CAUSE_REG(0), ~(cause & BIT(BIT_MICON)));
}
#endif // of CONFIG_BUFFALO_USE_MICON

#if defined CONFIG_BUFFALO_USE_MICON
//----------------------------------------------------------------------
// RTC
//----------------------------------------------------------------------
void BuffaloGpio_RtcIntSetup(void)
{
	mvGppTypeSet(0, BIT(BIT_RTC) , BIT(BIT_RTC));		// disable output
	mvGppPolaritySet(0, BIT(BIT_RTC) , 0);
	MV_REG_WRITE(GPP_INT_LVL_REG(0), 0);
}

//----------------------------------------------------------------------
void BuffaloGpio_ClearRtcInt(void)
{
	unsigned cause;
	
	cause = MV_REG_READ(GPP_INT_CAUSE_REG(0));
	MV_REG_WRITE(GPP_INT_CAUSE_REG(0), ~(cause & BIT(BIT_RTC)));
}
#endif // of CONFIG_BUFFALO_USE_MICON


#if defined CONFIG_BUFFALO_USE_INTERRUPT_DRIVER
void
BuffaloGpio_CPUInterruptsSetup(void)
{
//	mvGppTypeSet(0, BIT(BIT_POWER), BIT(BIT_POWER));
//	mvGppPolaritySet(0, BIT(BIT_POWER), (MV_GPP_IN_INVERT & BIT(BIT_POWER)));
}

void
BuffaloGpio_CPUInterruptsClear(void)
{
	if (use_slide_power == 1)
		return;

	unsigned int cause;
	printk("MPP_CONTROL_REG 0x%04x 0x%04x 0x%04x 0x%04x\n",
		MV_REG_READ(mvCtrlMppRegGet(0)),
		MV_REG_READ(mvCtrlMppRegGet(1)),
		MV_REG_READ(mvCtrlMppRegGet(2)),
		MV_REG_READ(mvCtrlMppRegGet(3)));

	cause = MV_REG_READ(GPP_INT_CAUSE_REG(0));
	printk("cause:0x%04x\n", (unsigned int)cause);
	MV_REG_WRITE(GPP_INT_CAUSE_REG(0), ~(cause & BIT(BIT_POWER)));
	//BuffaloGpio_EthLedOn();
}



void
BuffaloGpio_CPUInterruptsSetupInit(void)
{
//	mvGppTypeSet(0, BIT(BIT_INIT), BIT(BIT_INIT));
//	mvGppPolaritySet(0, BIT(BIT_INIT), (MV_GPP_IN_INVERT & BIT(BIT_INIT)));
}

void
BuffaloGpio_CPUInterruptsClearInit(void)
{
	unsigned int cause;
	printk("MPP_CONTROL_REG 0x%04x 0x%04x 0x%04x 0x%04x\n",
		MV_REG_READ(mvCtrlMppRegGet(0)),
		MV_REG_READ(mvCtrlMppRegGet(1)),
		MV_REG_READ(mvCtrlMppRegGet(2)),
		MV_REG_READ(mvCtrlMppRegGet(3)));

	cause = MV_REG_READ(GPP_INT_CAUSE_REG(0));
	printk("cause:0x%04x\n", cause);
	MV_REG_WRITE(GPP_INT_CAUSE_REG(0), ~(cause & BIT(BIT_INIT)));
}


void
BuffaloGpio_CPUInterruptsSetupFunc(void)
{
//	mvGppTypeSet(0, BIT(BIT_FUNC), BIT(BIT_FUNC));
//	mvGppPolaritySet(0, BIT(BIT_FUNC), (MV_GPP_IN_INVERT & BIT(BIT_FUNC)));
}

void
BuffaloGpio_CPUInterruptsClearFunc(void)
{
	unsigned int cause;
	printk("MPP_CONTROL_REG 0x%04x 0x%04x 0x%04x 0x%04x\n",
		MV_REG_READ(mvCtrlMppRegGet(0)),
		MV_REG_READ(mvCtrlMppRegGet(1)),
		MV_REG_READ(mvCtrlMppRegGet(2)),
		MV_REG_READ(mvCtrlMppRegGet(3)));

	cause = MV_REG_READ(GPP_INT_CAUSE_REG(0));
	printk("cause:0x%04x\n", cause);
	MV_REG_WRITE(GPP_INT_CAUSE_REG(0), ~(cause & BIT(BIT_FUNC)));
}
#endif //CONFIG_BUFFALO_INTERRUPT_DRIVER

unsigned int
buffalo_gpio_read(void)
{
	return *((volatile unsigned int*)(INTER_REGS_BASE + GPP_DATA_IN_REG(0)));
}

unsigned int
buffalo_gpio_blink_reg_read(void)
{
	return *((volatile unsigned int*)(INTER_REGS_BASE + GPP_BLINK_EN_REG(0)));
}

void
BuffaloGpio_Init(void)
{
	BuffaloGpio_FanSlow();
}

void
BuffaloGpio_LedEnable(int led_bit)
{
	if (led_bit < 0)
		return;

	//	printk("GPIO_OUT_REG: 0x%08X => ", GPIO_OUT_REG);
	GPIO_OUT_REG &= ~BIT(led_bit);
	//	printk("0x%08X\n", GPIO_OUT_REG);
}

void
BuffaloGpio_LedDisable(int led_bit)
{
	if (led_bit < 0)
		return;

	//	printk("GPIO_OUT_REG: 0x%08X => ", GPIO_OUT_REG);
	GPIO_OUT_REG |= BIT(led_bit);
	//	printk("0x%08X\n", GPIO_OUT_REG);
}

void
BuffaloGpio_LedInvert(int led_bit)
{
	if (led_bit < 0)
		return;

	//	printk("GPIO_OUT_REG: 0x%08X => ", GPIO_OUT_REG);
	GPIO_OUT_REG ^= BIT(led_bit);
	//	printk("0x%08X\n", GPIO_OUT_REG);
}

void
BuffaloGpio_LedBlinkEnable(int led_bit)
{
	if (led_bit < 0)
		return;

	MV_REG_BIT_SET(GPP_BLINK_EN_REG(0), (BIT(led_bit)));
}

void
BuffaloGpio_LedBlinkDisable(int led_bit)
{
	if (led_bit < 0)
		return;

	MV_REG_BIT_RESET(GPP_BLINK_EN_REG(0), (BIT(led_bit)));
}


void
BuffaloGpio_HddPowerOff(int hdd_bit)
{
	if (hdd_bit < 0)
		return;

	MV_REG_BIT_RESET(GPP_DATA_OUT_REG(0), BIT(hdd_bit));
	//	MV_REG_BIT_RESET(GPP_DATA_OUT_EN_REG(0), BIT(hdd_bit));
}

void
BuffaloGpio_HddPowerOn(int hdd_bit)
{
	if (hdd_bit < 0)
		return;

	MV_REG_BIT_SET(GPP_DATA_OUT_REG(0), BIT(hdd_bit));
	//	MV_REG_BIT_SET(GPP_DATA_OUT_EN_REG(0), BIT(hdd_bit));
}



void
BuffaloGpio_UsbPowerOff(int usb_bit)
{
	if (usb_bit < 0)
		return;

	MV_REG_BIT_RESET(GPP_DATA_OUT_REG(0), BIT(usb_bit));
}

void
BuffaloGpio_UsbPowerOn(int usb_bit)
{
	if (usb_bit < 0)
		return;

	MV_REG_BIT_SET(GPP_DATA_OUT_REG(0), BIT(usb_bit));
}



void
BuffaloGpio_CpuReset(void)
{
	MV_REG_BIT_SET(CPU_RSTOUTN_MASK_REG, BIT(2));
	MV_REG_BIT_SET(CPU_SYS_SOFT_RST_REG, BIT(0));
}



void
BuffaloGpio_EthLedOn(void)
{
	buffalo_link_led_on(0);
}

void
BuffaloGpio_EthLedOff(void)
{
	buffalo_link_led_off(0);
}



void
BuffaloGpio_AllLedOff(void)
{
	if (BIT_LED_ALARM >= 0) {
		BuffaloGpio_AlarmLedDisable();
		BuffaloGpio_AlarmLedBlinkDisable();
	}
	if (BIT_LED_INFO >= 0) {
		BuffaloGpio_InfoLedDisable();
		BuffaloGpio_InfoLedBlinkDisable();
	}
	if (BIT_LED_PWR >= 0) {
		BuffaloGpio_PowerLedDisable();
		BuffaloGpio_PowerLedBlinkDisable();
	}
	if (BIT_LED_ETH >= 0) {
		BuffaloGpio_EthLedOff();
	}
	if (BIT_LED_FUNC >= 0) {
		BuffaloGpio_FuncLedDisable();
		BuffaloGpio_FuncLedBlinkDisable();
	}
}

void
BuffaloGpio_AllLedOn(void)
{
	if (BIT_LED_ALARM >= 0)
		BuffaloGpio_AlarmLedEnable();
	if (BIT_LED_INFO >= 0)
		BuffaloGpio_InfoLedEnable();
	if (BIT_LED_PWR >= 0)
		BuffaloGpio_PowerLedEnable();
	if (BIT_LED_ETH >= 0)
		BuffaloGpio_EthLedOn();
	if (BIT_LED_FUNC >= 0)
		BuffaloGpio_FuncLedEnable();
}


void
BuffaloGpio_FanControlWrite(unsigned int value)
{
	if (BIT_FAN_LOW < 0 || BIT_FAN_HIGH < 0)
		return;

	unsigned int outreg = GPIO_OUT_REG;

	switch(value){
	case BUFFALO_GPIO_FAN_STOP:
		//printk("%s > changing to stop\n", __FUNCTION__);
//		MV_REG_BIT_SET(GPP_DATA_OUT_REG(0), (BIT(BIT_FAN_LOW)));
//		MV_REG_BIT_SET(GPP_DATA_OUT_REG(0), (BIT(BIT_FAN_HIGH)));
		outreg |= BIT(BIT_FAN_LOW) | BIT(BIT_FAN_HIGH);
		break;
	case BUFFALO_GPIO_FAN_SLOW:
		//printk("%s > changing to slow\n", __FUNCTION__);
//		MV_REG_BIT_SET(GPP_DATA_OUT_REG(0), (BIT(BIT_FAN_LOW)));
//		MV_REG_BIT_RESET(GPP_DATA_OUT_REG(0), (BIT(BIT_FAN_HIGH)));
		outreg |= BIT(BIT_FAN_LOW);
		outreg &= ~BIT(BIT_FAN_HIGH);
		break;
	case BUFFALO_GPIO_FAN_FAST:
		//printk("%s > changing to fast\n", __FUNCTION__);
//		MV_REG_BIT_RESET(GPP_DATA_OUT_REG(0), (BIT(BIT_FAN_LOW)));
//		MV_REG_BIT_SET(GPP_DATA_OUT_REG(0), (BIT(BIT_FAN_HIGH)));
		outreg &= ~BIT(BIT_FAN_LOW);
		outreg |= BIT(BIT_FAN_HIGH);
		break;
	case BUFFALO_GPIO_FAN_FULL:
		//printk("%s > changing to full\n", __FUNCTION__);
//		MV_REG_BIT_RESET(GPP_DATA_OUT_REG(0), (BIT(BIT_FAN_LOW)));
//		MV_REG_BIT_RESET(GPP_DATA_OUT_REG(0), (BIT(BIT_FAN_HIGH)));
		outreg &=  ~(BIT(BIT_FAN_LOW) | BIT(BIT_FAN_HIGH));
		break;
	default:
		break;
	} // end of switch
	GPIO_OUT_REG = outreg;
}

unsigned int
BuffaloGpio_FanControlRead(void)
{
	if (BIT_FAN_LOW < 0 || BIT_FAN_HIGH < 0)
		return 0;

	unsigned long fan_low = GPIO_OUT_REG & BIT(BIT_FAN_LOW);
	unsigned long fan_high= GPIO_OUT_REG & BIT(BIT_FAN_HIGH);

	if(! fan_low && ! fan_high)
	{
		return BUFFALO_GPIO_FAN_FULL;
	}
	else if(!fan_low && fan_high)
	{
		return BUFFALO_GPIO_FAN_FAST;
	}
	else if(fan_low && !fan_high)
	{
		return BUFFALO_GPIO_FAN_SLOW;
	}
	else if(fan_low && fan_high)
	{
		return BUFFALO_GPIO_FAN_STOP;
	}
	return 0;
}


uint8_t
BuffaloGpio_ChangePowerStatus(uint8_t ChangeType){

        uint8_t MagicKey=0;

        switch(ChangeType){
        case POWER_STATUS_REBOOTING:
                MagicKey = MagicKeyReboot;
                break;
        case POWER_STATUS_REBOOTING_UBOOT_PASSED:
                MagicKey = MagicKeyRebootUbootPassed;
                break;
        case POWER_STATUS_NORMAL_STATE:
                MagicKey = MagicKeyNormalState;
                break;
        case POWER_STATUS_HW_POWER_OFF:
                MagicKey = MagicKeyHwPoff;
                break;
        case POWER_STATUS_SW_POWER_OFF:
                MagicKey = MagicKeySwPoff;
                break;
        case POWER_STATUS_SW_POFF_UBOOT_PASSED:
                MagicKey = MagicKeySWPoffUbootPassed;
                break;
        case POWER_STATUS_FWUPDATING:
                MagicKey = MagicKeyFWUpdating;
                break;
        case POWER_STATUS_REBOOT_REACHED_HALT:
                MagicKey = MagicKeyRebootReachedHalt;
                break;
        case POWER_STATUS_SW_POFF_REACHED_HALT:
                MagicKey = MagicKeySWPoffReachedHalt;
                break;
	case POWER_STATUS_UPS_SHUTDOWN:
		MagicKey = MagicKeyUpsShutdown;
		break;
	case POWER_STATUS_UPS_SHUTDOWN_REACHED_HALT:
		MagicKey = MagicKeyUpsShutdownReachedHalt;
		break;
        }

        if(MagicKey){
		//printk("%s > Writing 0x%02x\n", __FUNCTION__, MagicKey);
                BufRtcDS1339AlarmBSet(MagicKey);
        }else{
                BufRtcDS1339AlarmBGet(&MagicKey);
        }

	return MagicKey;
}



#ifdef CONFIG_BUFFALO_USE_UPS
// for ups codes
void BuffaloGpio_UPSPortEnable(void){

	mvGppTypeSet(0, BIT(BIT_UPS), (MV_GPP_OUT_EN & BIT(BIT_UPS)));
	mvGppValueSet(0, BIT(BIT_UPS), BIT(BIT_UPS));

}

void BuffaloGpio_UPSPortDisable(void){

	mvGppTypeSet(0, BIT(BIT_UPS), (MV_GPP_OUT_DIS & BIT(BIT_UPS)));
	mvGppValueSet(0, BIT(BIT_UPS), (0 & BIT(BIT_UPS)));

}

unsigned int BuffaloGpio_UPSPortScan(void){

	MV_U32 ret = mvGppValueGet(0, BIT(BIT_UPS));
	return (unsigned int)( ret & BIT(BIT_UPS));
	
}
// end of basically ups codes

////////////////////////////////////////////////
void BuffaloGpio_UPSOmronBLEnable(void){
	
	mvGppTypeSet(0, BIT(BIT_OMR_BL), (MV_GPP_OUT_DIS & BIT(BIT_OMR_BL)));
	
}

void BuffaloGpio_UPSOmronBLDisable(void){
	
	mvGppTypeSet(0, BIT(BIT_OMR_BL), (MV_GPP_OUT_EN & BIT(BIT_OMR_BL)));
	mvGppValueSet(0, BIT(BIT_OMR_BL), (0 & BIT(BIT_OMR_BL)));
	
}

unsigned int BuffaloGpio_UPSOmronBLGetStatus(void){
	
	MV_U32 ret = mvGppValueGet(0, BIT(BIT_OMR_BL));
	return (unsigned int)( ret & BIT(BIT_OMR_BL));
	
}

unsigned int BuffaloGpio_UPSOmronBLUseStatus(void){

	unsigned base=0xF1000000;								////caution base addr directry using!!!!///////
	volatile unsigned *gpioStat=(unsigned *)(base+GPP_DATA_OUT_EN_REG(0));
//	printk("0x%x\n", gpio[1]);
	return (gpioStat[0] & BIT(BIT_OMR_BL));
	
}
///////////////////////////////////////////////////////


// for ups recover function

void BuffaloRtc_UPSRecoverInit(void){
	mvRtcDS1339Init();
	BufRtcDS1339AlarmBSet(0);
}

void BuffaloRtc_UPSRecoverEnable(int8_t TargetType){

	int8_t MagicKey=0;

	switch(TargetType){
	case RECOVER_TARGET_UPS_APC:
		MagicKey = MagicKeyAPC;
		break;
	case RECOVER_TARGET_UPS_OMR:
		MagicKey = MagicKeyOMR;
		break;
	case RECOVER_TARGET_UPS_USB: 
		MagicKey = MagicKeyUSB;
		break;
	default:
		break;
	}
	
	if(MagicKey){
		BufRtcDS1339AlarmBSet(MagicKey);
	}
	
}

void BuffaloRtc_UPSRecoverDisable(void){
	
	BufRtcDS1339AlarmBSet(0);
}

int BuffaloRtc_UPSRecoverReadStatus(void){
	
	int8_t data;
	BufRtcDS1339AlarmBGet(&data);

	switch(data){
	case MagicKeyAPC:
		return RECOVER_TARGET_UPS_APC;
		break;
	case MagicKeyOMR:
		return RECOVER_TARGET_UPS_OMR;
		break;
	case MagicKeyUSB:
		return RECOVER_TARGET_UPS_USB;
		break;
	default:
		break;
	}
	return -1;
}
// end of recover function 


////
// for debug
////
/*
unsigned int BuffaloGpio_PortScan(void){
	
	MV_U32 ret = mvGppValueGet(0, 0xffff);
	
	return ret;
}*/

#endif  //CONFIG_BUFFALO_USE_UPS

//----------------------------------------------------------------------
// for Debug
//----------------------------------------------------------------------
/*
void BuffaloPrintGpio(void)
{
	printk("GPIO:Din=%x Int=%x mask=%x lmask=%x\n "
		,MV_REG_READ(GPP_DATA_IN_REG(0))
		,MV_REG_READ(GPP_INT_CAUSE_REG(0))
		,MV_REG_READ(GPP_INT_MASK_REG(0))
		,MV_REG_READ(GPP_INT_LVL_REG(0))
		);
}
*/

static uint32_t pm_gpio_value;
DECLARE_COMPLETION(pm_comp);

static MV_BOOLEAN
PMCommandCompletionCB(MV_SATA_ADAPTER *pSataAdapter,
		      MV_U8 channelIndex,
		      MV_COMPLETION_TYPE comp_type,
		      MV_VOID_PTR commandId,
		      MV_U16 responseFlags,
		      MV_U32 timeStamp,
		      MV_STORAGE_DEVICE_REGISTERS *registerStruct)
{
	MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt =
	  (MV_IAL_COMMON_ADAPTER_EXTENSION *)commandId;
	ialExt->IALChannelExt[channelIndex].pmRegAccessInProgress = MV_FALSE;
	switch (comp_type) {
	case MV_COMPLETION_TYPE_NORMAL:
		if (ialExt->IALChannelExt[channelIndex].pmAccessType
		    == MV_ATA_COMMAND_PM_READ_REG) {
			pm_gpio_value = registerStruct->sectorCountRegister;
			pm_gpio_value |= (registerStruct->lbaLowRegister << 8);
			pm_gpio_value |= (registerStruct->lbaMidRegister << 16);
			pm_gpio_value |= (registerStruct->lbaHighRegister << 24);
//			printk("registerStruct->sectorCountRegister = %04x\n", registerStruct->sectorCountRegister);
//			printk("registerStruct->lbaLowRegister = %04x\n", registerStruct->lbaLowRegister);
//			printk("registerStruct->lbaMidRegister = %04x\n", registerStruct->lbaMidRegister);
//			printk("registerStruct->lbaHighRegister = %04x\n", registerStruct->lbaHighRegister);
//			printk("PM GPIO Control register = 0x%08x\n", pm_gpio_value);
		}
		break;
	case MV_COMPLETION_TYPE_ABORT:
		printk("[%d %d]: read PM register aborted!\n", pSataAdapter->adapterId, channelIndex);
		break;
	case MV_COMPLETION_TYPE_ERROR:
		printk("[%d %d]: read PM register error!\n", pSataAdapter->adapterId, channelIndex);
		break;
	default:
		printk("[%d %d]: Unknown completion type (%d)\n", pSataAdapter->adapterId, channelIndex, comp_type);
		return MV_FALSE;
	}

	complete(&pm_comp);
	return MV_TRUE;
}

extern IAL_ADAPTER_T *pSocAdapter;
static int
BuffaloAccessPmGpio(uint32_t *value, int isRead)
{
	MV_QUEUE_COMMAND_INFO   qCommandInfo;
	MV_QUEUE_COMMAND_RESULT result;
	MV_SATA_ADAPTER *pSataAdapter;
	MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt;

	memset(&qCommandInfo, 0, sizeof(MV_QUEUE_COMMAND_INFO));
	ialExt = &(pSocAdapter->ialCommonExt);
	pSataAdapter = &(pSocAdapter->mvSataAdapter);

	qCommandInfo.type = MV_QUEUED_COMMAND_TYPE_NONE_UDMA;
	qCommandInfo.PMPort = MV_SATA_PM_CONTROL_PORT;
	qCommandInfo.commandParams.NoneUdmaCommand.protocolType = MV_NON_UDMA_PROTOCOL_NON_DATA;
	qCommandInfo.commandParams.NoneUdmaCommand.isEXT = MV_TRUE;
	qCommandInfo.commandParams.NoneUdmaCommand.bufPtr = NULL;
	qCommandInfo.commandParams.NoneUdmaCommand.count = 0;
	qCommandInfo.commandParams.NoneUdmaCommand.features = MV_SATA_GSCR_GPIO_CONTROL_REG_NUM;
	qCommandInfo.commandParams.NoneUdmaCommand.device = MV_SATA_PM_CONTROL_PORT;
	qCommandInfo.commandParams.NoneUdmaCommand.callBack = PMCommandCompletionCB;
	qCommandInfo.commandParams.NoneUdmaCommand.commandId = (MV_VOID_PTR)ialExt;
	ialExt->IALChannelExt[0].pmReg = MV_SATA_GSCR_GPIO_CONTROL_REG_NUM;

	if (isRead) {
		ialExt->IALChannelExt[0].pmAccessType = MV_ATA_COMMAND_PM_READ_REG;
		qCommandInfo.commandParams.NoneUdmaCommand.command = MV_ATA_COMMAND_PM_READ_REG;
		qCommandInfo.commandParams.NoneUdmaCommand.sectorCount = 0;
		qCommandInfo.commandParams.NoneUdmaCommand.lbaLow = 0;
		qCommandInfo.commandParams.NoneUdmaCommand.lbaMid = 0;
		qCommandInfo.commandParams.NoneUdmaCommand.lbaHigh = 0;
	}
	else {
		ialExt->IALChannelExt[0].pmAccessType = MV_ATA_COMMAND_PM_WRITE_REG;
		qCommandInfo.commandParams.NoneUdmaCommand.command = MV_ATA_COMMAND_PM_WRITE_REG;
		qCommandInfo.commandParams.NoneUdmaCommand.sectorCount = (MV_U16)((*value) & 0xff);
		qCommandInfo.commandParams.NoneUdmaCommand.lbaLow = (MV_U16)(((*value) & 0xff00) >> 8);
		qCommandInfo.commandParams.NoneUdmaCommand.lbaMid = (MV_U16)(((*value) & 0xff0000) >> 16);
		qCommandInfo.commandParams.NoneUdmaCommand.lbaHigh = (MV_U16)(((*value) & 0xff000000) >> 24);
	}

	result = mvSataQueueCommand(pSataAdapter, 0, &qCommandInfo);
	if (result != MV_QUEUE_COMMAND_RESULT_OK) {
		printk("mvSataQueueCommand failed.(error code:%d)\n", result);
		return result;
	}

	wait_for_completion(&pm_comp);

	if (isRead) {
		*value = pm_gpio_value;
	}

	return result;
}

int
BuffaloReadPmGpio(uint32_t *value)
{
	return BuffaloAccessPmGpio(value, 1);
}

int
BuffaloWritePmGpio(uint32_t *value)
{
	return BuffaloAccessPmGpio(value, 0);
}

int
Buffalo_has_PM(void)
{
	MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt = &(pSocAdapter->ialCommonExt);
	if (ialExt->IALChannelExt[0].PMnumberOfPorts != 0) {
		return 1;
	}

	return 0;
}

EXPORT_SYMBOL(BuffaloGpio_HddPowerOff);
EXPORT_SYMBOL(BuffaloGpio_CpuReset);
