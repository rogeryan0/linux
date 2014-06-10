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

#include "gpp/mvGpp.h"
#include "gpp/mvGppRegs.h"

#include "ctrlEnv/sys/mvCpuIfRegs.h"

#include "rtc/integ_rtc/mvRtc.h"
#include "rtc/integ_rtc/mvRtcReg.h"
#include "mvStorageDev.h"
#include "mv_sata/mvLinuxIalHt.h"
#include "mv_sata/mvIALCommonUtils.h"

#if defined CONFIG_BUFFALO_USE_GPIO_DRIVER
 #include "eth-phy/mvEthPhy.h"
#endif

#include "buffalo/BuffaloGpio.h"
#include "buffalo/kernevnt.h"

#define MagicKeyAPC	0x45
#define MagicKeyOMR	0x3a
#define MagicKeyUSB	0x01

#if defined CONFIG_BUFFALO_USE_MICON
//----------------------------------------------------------------------
// micon
//----------------------------------------------------------------------
void BuffaloGpio_MiconIntSetup(void)
{
	mvGppTypeSet(0, BIT(BIT_MC_IRQ) , BIT(BIT_MC_IRQ));		// disable output
	mvGppPolaritySet(MICON_POL, BIT(BIT_MC_IRQ) , BIT(BIT_MC_IRQ));
	// set GPIO-2 : edge IRQ
	//set_irq_handler(MICON_IRQ, do_edge_IRQ);
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
	MV_REG_WRITE(GPP_INT_CAUSE_REG(0), ~(cause & BIT(BIT_MC_IRQ)));
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
	if (use_slide_power)
		return;

	unsigned int cause;
	printk("MPP_CONTROL_REG 0x%04x 0x%04x 0x%04x 0x%04x\n",
		MV_REG_READ(mvCtrlMppRegGet(0)),
		MV_REG_READ(mvCtrlMppRegGet(1)),
		MV_REG_READ(mvCtrlMppRegGet(2)),
		MV_REG_READ(mvCtrlMppRegGet(3)));

	cause = MV_REG_READ(GPP_INT_CAUSE_REG(0));
	printk("cause:0x%04x\n", (unsigned int)cause);
	MV_REG_WRITE(GPP_INT_CAUSE_REG(0), ~(cause & BIT(BIT_PWR_SW)));
	//BuffaloGpio_EthLedOn();
}



void
BuffaloGpio_CPUInterruptsSetupInit(void)
{
//	mvGppTypeSet(0, BIT(BIT_INIT_SW), BIT(BIT_INIT_SW));
//	mvGppPolaritySet(0, BIT(BIT_INIT_SW), (MV_GPP_IN_INVERT & BIT(BIT_INIT_SW)));
}

//void
//BuffaloGpio_CPUInterruptsClearInit(void)
//{
//	unsigned int cause;
//	printk("MPP_CONTROL_REG 0x%04x 0x%04x 0x%04x 0x%04x\n",
//		MV_REG_READ(mvCtrlMppRegGet(0)),
//		MV_REG_READ(mvCtrlMppRegGet(1)),
//		MV_REG_READ(mvCtrlMppRegGet(2)),
//		MV_REG_READ(mvCtrlMppRegGet(3)));
//
//	cause = MV_REG_READ(GPP_INT_CAUSE_REG(0));
//	printk("cause:0x%04x\n", cause);
//	MV_REG_WRITE(GPP_INT_CAUSE_REG(0), ~(cause & BIT(BIT_INIT_SW)));
//}


void
BuffaloGpio_CPUInterruptsSetupFunc(void)
{
//	mvGppTypeSet(0, BIT(BIT_FUNC_SW), BIT(BIT_FUNC_SW));
//	mvGppPolaritySet(0, BIT(BIT_FUNC_SW), (MV_GPP_IN_INVERT & BIT(BIT_FUNC_SW)));
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
	MV_REG_WRITE(GPP_INT_CAUSE_REG(0), ~(cause & BIT(BIT_FUNC_SW)));
}
#endif //CONFIG_BUFFALO_INTERRUPT_DRIVER


void
BuffaloGpio_Init(void)
{
  //	BuffaloGpio_FanSlow();
}

void __inline__
BuffaloGpio_LedEnable(int led_bit)
{
	bfGppOutRegBitClr(led_bit);
}

void __inline__
BuffaloGpio_LedDisable(int led_bit)
{
	bfGppOutRegBitSet(led_bit);
}

void __inline__
BuffaloGpio_LedInvert(int led_bit)
{
	bfGppOutRegBitInv(led_bit);
}

void __inline__
BuffaloGpio_LedBlinkEnable(int led_bit)
{
	bfGppBlinkRegBitSet(led_bit);
}

void __inline__
BuffaloGpio_LedBlinkDisable(int led_bit)
{
	bfGppBlinkRegBitClr(led_bit);
}

void __inline__
BuffaloGpio_HddPowerOn(int hdd_bit)
{
	bfGppOutRegBitSet(hdd_bit);
}

void __inline__
BuffaloGpio_HddPowerOff(int hdd_bit)
{
	bfGppOutRegBitClr(hdd_bit);
}

void __inline__
BuffaloGpio_UsbPowerOn(int usb_bit)
{
	bfGppOutRegBitSet(usb_bit);
}

void __inline__
BuffaloGpio_UsbPowerOff(int usb_bit)
{
	bfGppOutRegBitClr(usb_bit);
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
	if (BIT_ALARM_LED >= 0) {
		BuffaloGpio_AlarmLedDisable();
		BuffaloGpio_AlarmLedBlinkDisable();
	}
	if (BIT_INFO_LED >= 0) {
		BuffaloGpio_InfoLedDisable();
		BuffaloGpio_InfoLedBlinkDisable();
	}
	if (BIT_PWR_LED >= 0) {
		BuffaloGpio_PowerLedDisable();
		BuffaloGpio_PowerLedBlinkDisable();
	}
//	if (BIT_LED_ETH >= 0) {
//		BuffaloGpio_EthLedOff();
//	}
	if (BIT_FUNC_LED >= 0) {
		BuffaloGpio_FuncLedDisable();
		BuffaloGpio_FuncLedBlinkDisable();
	}
}

void
BuffaloGpio_AllLedOn(void)
{
	if (BIT_ALARM_LED >= 0)
		BuffaloGpio_AlarmLedEnable();
	if (BIT_INFO_LED >= 0)
		BuffaloGpio_InfoLedEnable();
	if (BIT_PWR_LED >= 0)
		BuffaloGpio_PowerLedEnable();
//	if (BIT_LED_ETH >= 0)
//		BuffaloGpio_EthLedOn();
	if (BIT_FUNC_LED >= 0)
		BuffaloGpio_FuncLedEnable();
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

MV_BOOL bfGppInRegBitTest(MV_32 bit)
{
	if (bit < 0)
		return MV_FALSE;

	if (bit < 32)
		return mvGppValueGet(0, BIT(bit)) ? MV_TRUE : MV_FALSE;
	else
		return mvGppValueGet(1, BIT(bit - 32)) ? MV_TRUE : MV_FALSE;
}

MV_BOOL bfGppOutRegBitTest(MV_32 bit)
{
	if (bit < 0)
		return MV_FALSE;

	if (bit < 32)
		return MV_REG_READ(GPP_DATA_OUT_REG(0)) & BIT(bit) ? MV_TRUE : MV_FALSE;
	else
		return MV_REG_READ(GPP_DATA_OUT_REG(1)) & BIT(bit - 32) ? MV_TRUE : MV_FALSE;
}

MV_VOID bfGppOutRegBitSet(MV_32 bit)
{
	if (bit < 0)
		return;

	if (bit < 32)
		mvGppValueSet(0, BIT(bit), 0xffffffff);
	else
		mvGppValueSet(1, BIT(bit - 32), 0xffffffff);
}

MV_VOID bfGppOutRegBitClr(MV_32 bit)
{
	if (bit < 0)
		return;

	if (bit < 32)
		mvGppValueSet(0, BIT(bit), 0x0);
	else
		mvGppValueSet(1, BIT(bit - 32), 0x0);
}

MV_VOID bfGppOutRegBitInv(MV_32 bit)
{
	if (bit < 0)
		return;

	if (bit < 32)
		mvGppValueSet(0, BIT(bit), ~mvGppValueGet(0, 0xffffffff));
	else
		mvGppValueSet(1, BIT(bit - 32), ~mvGppValueGet(1, 0xffffffff));
}

MV_BOOL bfGppBlinkRegBitTest(MV_32 bit)
{
	if (bit < 0)
		return MV_FALSE;

	if (bit < 32)
		return MV_REG_READ(GPP_BLINK_EN_REG(0)) & BIT(bit) ? MV_TRUE : MV_FALSE;
	else
		return MV_REG_READ(GPP_BLINK_EN_REG(1)) & BIT(bit - 32) ? MV_TRUE : MV_FALSE;
}

MV_VOID bfGppBlinkRegBitSet(MV_32 bit)
{
	if (bit < 0)
		return;

	if (bit < 32)
		mvGppBlinkEn(0, BIT(bit), 0xffffffff);
	else
		mvGppBlinkEn(1, BIT(bit - 32), 0xffffffff);
}

MV_VOID bfGppBlinkRegBitClr(MV_32 bit)
{
	if (bit < 0)
		return;

	if (bit < 32)
		mvGppBlinkEn(0, BIT(bit), 0x0);
	else
		mvGppBlinkEn(1, BIT(bit - 32), 0x0);
}

MV_BOOL bfGppPolarityRegBitTest(MV_32 bit)
{
	if (bit < 0)
		return MV_FALSE;

	if (bit < 32)
		return MV_REG_READ(GPP_DATA_IN_POL_REG(0)) & BIT(bit) ? MV_TRUE : MV_FALSE;
	else
		return MV_REG_READ(GPP_DATA_IN_POL_REG(1)) & BIT(bit - 32) ? MV_TRUE : MV_FALSE;
}

MV_VOID bfGppPolarityRegBitInv(MV_32 bit)
{
	if (bit < 0)
		return MV_FALSE;

	if (bit < 32)
		return bfRegInv(GPP_DATA_IN_POL_REG(0), BIT(bit));
	else
		return bfRegInv(GPP_DATA_IN_POL_REG(1), BIT(bit - 32));
}

MV_U32 bfRegGet(MV_U32 regOffs, MV_U32 mask)
{
	return (MV_REG_READ(regOffs) & mask);
}

MV_VOID bfRegSet(MV_U32 regOffs, MV_U32 mask, MV_U32 value)
{
	MV_U32 regData;

	regData = MV_REG_READ(regOffs);
	regData &= ~mask;
	regData |= (value & mask);
	MV_REG_WRITE(regOffs, regData);
}

MV_VOID bfRegInv(MV_U32 regOffs, MV_U32 mask)
{
	MV_U32 regData;

	regData = MV_REG_READ(regOffs) ^ mask;
	MV_REG_WRITE(regOffs, regData);
}

int bfIsSerialConsoleEnable(void)
{
	return bfGppInRegBitTest(BIT_UART_EN);
}

EXPORT_SYMBOL(BuffaloGpio_HddPowerOff);
EXPORT_SYMBOL(BuffaloGpio_CpuReset);
