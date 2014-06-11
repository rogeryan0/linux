#ifndef _MICONCNTL_H_
#define _MICONCNTL_H_

void miconCntl_Reboot(void);
void miconCntl_PowerOff(void);
#if defined(CONFIG_BUFFALO_SUPPORT_WOL)
void miconCntl_WolEnable(void);
#endif

#endif
