#ifndef _BUFFALO_UART_H_
#define _BUFFALO_UART_H_

void BuffaloInitUart(void);
void BuffaloConsoleOutput(const unsigned char *buff);
void BuffaloMiconOutput(const unsigned char *buff, int len);
int BuffaloMiconInput(unsigned char *ch, unsigned tmout);
void BuffaloConsoleOutput(const unsigned char *buff);
int BuffaloConsoleInput(unsigned char *ch, unsigned tmout);
void BuffaloMiconOutput(const unsigned char *buff, int len);
int BuffaloMiconInput(unsigned char *ch, unsigned tmout);

#if defined(CONFIG_ARCH_FEROCEON_MV78XX0) || defined(CONFIG_ARCH_ARMADA_XP)
#define BUFFALO_MODEM_CTRL_MODE_GPIO  1
#define BUFFALO_MODEM_CTRL_MODE_MPP   0

extern unsigned int BuffaloMctrlMode;
#endif // of CONFIG_ARCH_FEROCEON_MV78XX0

/*UART IP core*/
#define UART0 0
#define UART1 1
#define UART2 2
#define UART3 3

/*buffaloUART define*/
#define CONSOLEPORT 0
#define MICONPORT 1
#define UPSPORT 2

#endif
