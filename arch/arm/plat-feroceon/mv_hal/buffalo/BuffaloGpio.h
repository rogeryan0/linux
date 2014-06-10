#ifndef _BUFFALO_GPIO_H_
#define _BUFFALO_GPIO_H_

#define BIT(x)	(1<<(x))

#if defined(CONFIG_BUFFALO_PLATFORM)
extern s8 bitHddPower[];
extern s8 bitUsbPower[];
extern s8 bitInput[];
extern s8 bitOutput[];
extern s8 bitLed[];
extern u8 use_slide_power;

#define BIT_LED_INFO		bitLed[BUF_NV_OFFS_BIT_LED_INFO]
#define BIT_LED_PWR		bitLed[BUF_NV_OFFS_BIT_LED_POWER]
#define BIT_LED_ALARM		bitLed[BUF_NV_OFFS_BIT_LED_ALARM]
#define BIT_LED_FUNC		bitLed[BUF_NV_OFFS_BIT_LED_FUNC]
#define BIT_LED_ETH		bitLed[BUF_NV_OFFS_BIT_LED_ETH]

#define BIT_FAN_LOW		bitOutput[BUF_NV_OFFS_BIT_FAN_LOW]
#define BIT_FAN_HIGH		bitOutput[BUF_NV_OFFS_BIT_FAN_HIGH]
#define BIT_FAN_LCK		bitInput[BUF_NV_OFFS_BIT_FAN_LOCK]
#define BIT_POWER		bitInput[BUF_NV_OFFS_SW_POWER]
#define BIT_FUNC		bitInput[BUF_NV_OFFS_SW_FUNC]
#define BIT_AUTO_POWER		bitInput[BUF_NV_OFFS_SW_AUTO_POWER]
#define BIT_INIT		bitInput[BUF_NV_OFFS_SW_INIT]

#define BUFFALO_GPIO_FAN_STOP	0
#define BUFFALO_GPIO_FAN_SLOW	1
#define BUFFALO_GPIO_FAN_FAST	2
#define BUFFALO_GPIO_FAN_FULL	3

	// prototypes.
void BuffaloGpio_CPUInterruptsSetup(void);
void BuffaloGpio_CPUInterruptsClear(void);

void BuffaloGpio_CPUInterruptsSetupAutopower(void);
void BuffaloGpio_CPUInterruptsClearAutopower(void);


void BuffaloGpio_CPUInterruptsSetupInit(void);
void BuffaloGpio_CPUInterruptsClearInit(void);


void BuffaloGpio_CPUInterruptsSetupFunc(void);
void BuffaloGpio_CPUInterruptsClearFunc(void);

unsigned int buffalo_gpio_read(void);
unsigned int buffalo_gpio_blink_reg_read(void);
void BuffaloGpio_Init(void);
void BuffaloGpio_LedEnable(int);
void BuffaloGpio_LedDisable(int);
void BuffaloGpio_LedInvert(int);
void BuffaloGpio_LedBlinkEnable(int);
void BuffaloGpio_LedBlinkDisable(int);

void BuffaloGpio_HddPowerOff(int);
void BuffaloGpio_HddPowerOn(int);


void BuffaloGpio_UsbPowerOff(int);
void BuffaloGpio_UsbPowerOn(int);

unsigned int BuffaloGpio_GetAutoPowerStatus(void);
void BuffaloGpio_CpuReset(void);
void BuffaloGpio_EthLedOn(void);
void BuffaloGpio_EthLedOff(void);
void BuffaloGpio_AllLedOff(void);
void BuffaloGpio_AllLedOn(void);
uint8_t BuffaloGpio_ChangePowerStatus(uint8_t);


#define BUFFALO_GPIO_FAN_STOP	0
#define BUFFALO_GPIO_FAN_SLOW	1
#define BUFFALO_GPIO_FAN_FAST	2
#define BUFFALO_GPIO_FAN_FULL	3

void BuffaloGpio_FanControlWrite(unsigned int);
unsigned int BuffaloGpio_FanControlRead(void);

#define BuffaloGpio_FanStop()			BuffaloGpio_FanControlWrite(BUFFALO_GPIO_FAN_STOP)
#define BuffaloGpio_FanSlow()			BuffaloGpio_FanControlWrite(BUFFALO_GPIO_FAN_SLOW)
#define BuffaloGpio_FanFast()			BuffaloGpio_FanControlWrite(BUFFALO_GPIO_FAN_FAST)
#define BuffaloGpio_FanFull()			BuffaloGpio_FanControlWrite(BUFFALO_GPIO_FAN_FULL)


// convenient macros.

#define BuffaloGpio_AlarmLedEnable()	 	BuffaloGpio_LedEnable(BIT_LED_ALARM)
#define BuffaloGpio_AlarmLedDisable()		BuffaloGpio_LedDisable(BIT_LED_ALARM)
#define BuffaloGpio_AlarmLedBlinkEnable()	BuffaloGpio_LedBlinkEnable(BIT_LED_ALARM)
#define BuffaloGpio_AlarmLedBlinkDisable()	BuffaloGpio_LedBlinkDisable(BIT_LED_ALARM)



#define BuffaloGpio_InfoLedEnable()		BuffaloGpio_LedEnable(BIT_LED_INFO)
#define BuffaloGpio_InfoLedDisable()		BuffaloGpio_LedDisable(BIT_LED_INFO)
#define BuffaloGpio_InfoLedBlinkEnable()	BuffaloGpio_LedBlinkEnable(BIT_LED_INFO)
#define BuffaloGpio_InfoLedBlinkDisable()	BuffaloGpio_LedBlinkDisable(BIT_LED_INFO)



#define BuffaloGpio_PowerLedEnable()		BuffaloGpio_LedEnable(BIT_LED_PWR)
#define BuffaloGpio_PowerLedDisable()		BuffaloGpio_LedDisable(BIT_LED_PWR)
#define BuffaloGpio_PowerLedBlinkEnable()	BuffaloGpio_LedBlinkEnable(BIT_LED_PWR)
#define BuffaloGpio_PowerLedBlinkDisable()	BuffaloGpio_LedBlinkDisable(BIT_LED_PWR)



#define BuffaloGpio_FuncLedEnable()		BuffaloGpio_LedEnable(BIT_LED_FUNC)
#define BuffaloGpio_FuncLedDisable()		BuffaloGpio_LedDisable(BIT_LED_FUNC)
#define BuffaloGpio_FuncLedBlinkEnable()	BuffaloGpio_LedBlinkEnable(BIT_LED_FUNC)
#define BuffaloGpio_FuncLedBlinkDisable()	BuffaloGpio_LedBlinkDisable(BIT_LED_FUNC)


// variable definition.
#define POWER_STATUS_REBOOTING                  1
#define POWER_STATUS_REBOOTING_UBOOT_PASSED     2
#define POWER_STATUS_NORMAL_STATE               3
#define POWER_STATUS_HW_POWER_OFF               4
#define POWER_STATUS_SW_POWER_OFF               5
#define POWER_STATUS_SW_POFF_UBOOT_PASSED       6
#define POWER_STATUS_FWUPDATING                 7
#define POWER_STATUS_REBOOT_REACHED_HALT        8
#define	POWER_STATUS_SW_POFF_REACHED_HALT       9
#define POWER_STATUS_UPS_SHUTDOWN		10
#define POWER_STATUS_UPS_SHUTDOWN_REACHED_HALT	11

#define MagicKeyReboot                  0x18
#define MagicKeyRebootUbootPassed       0x3a
#define MagicKeyNormalState             0x71
#define MagicKeyHwPoff                  0x43
#define MagicKeySwPoff                  0x02
#define MagicKeySWPoffUbootPassed       0x5c
#define MagicKeyFWUpdating              0x6f
#define MagicKeyRebootReachedHalt       0x2b
#define MagicKeySWPoffReachedHalt       0x7a
#define MagicKeyUpsShutdown		0x21
#define MagicKeyUpsShutdownReachedHalt	0x32


void BuffaloGpio_MiconIntSetup(void);
void BuffaloGpio_ClearMiconInt(void);

void BuffaloGpio_RtcIntSetup(void);
void BuffaloPrintGpio(void);


void BuffaloGpio_UPSPortEnable(void);
void BuffaloGpio_UPSPortDisable(void);
unsigned int BuffaloGpio_UPSPortScan(void); // for debug

void BuffaloGpio_UPSOmronBLEnable(void);
void BuffaloGpio_UPSOmronBLDisable(void);
unsigned int BuffaloGpio_UPSOmronBLGetStatus(void);
unsigned int BuffaloGpio_UPSOmronBLUseStatus(void);

void BuffaloRtc_UPSRecoverInit(void);
void BuffaloRtc_UPSRecoverEnable(int8_t);
void BuffaloRtc_UPSRecoverDisable(void);
int BuffaloRtc_UPSRecoverReadStatus(void);

int BuffaloReadPmGpio(uint32_t *value);
int BuffaloWritePmGpio(uint32_t *value);
int Buffalo_has_PM(void);

#define RECOVER_TARGET_UPS_APC	1
#define RECOVER_TARGET_UPS_OMR	2
#define RECOVER_TARGET_UPS_USB	3

#define GPIO_OUT_REG		*((volatile unsigned int*)(INTER_REGS_BASE + GPP_DATA_OUT_REG(0)))
#define GPIO_OUT_EN_REG		*((volatile unsigned int*)(INTER_REGS_BASE + GPP_DATA_OUT_EN_REG(0)))
#define GPIO_IN_REG		*((volatile unsigned int*)(INTER_REGS_BASE + GPP_DATA_IN_REG(0)))
#define GPIO_IN_POL_REG		*((volatile unsigned int*)(INTER_REGS_BASE + GPP_DATA_IN_POL_REG(0)))
#define GPIO_BLINK_EN_REG	*((volatile unsigned int*)(INTER_REGS_BASE + GPP_DATA_BLINK_EN_REG(0)))


#endif // end of CONFIG_BUFFALO_PLATFORM
#endif

