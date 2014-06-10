#ifndef _KERNEVNT_H_
#define _KERNEVNT_H_

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

/* routines are in kernel/arch/ppc/platforms/buffalo/kernevnt.c */

void kernevnt_LanAct(void *data);
#ifdef CONFIG_MD
void kernevnt_RaidRecovery(int devno,int on, int isRecovery, int major, int minor);
void kernevnt_RaidScan(int devno, int on);
void kernevnt_RaidDegraded(int devno, int major, int minor);
#endif

// drivers/block/ll_rw_blk.c
void kernevnt_IOErr(const char *kdevname, const char *dir, unsigned long sector, unsigned int errcnt);  /* 2006.9.5 :add errcnt */
void kernevnt_FlashUpdate(int on);
void kernevnt_DriveDead(const char *drvname);
void kernevnt_I2cErr(void);
void kernevnt_MiconInt(void);
void kernevnt_EnetOverload(const char *name);
void buffalo_kernevnt_queuein(const char *cmd);

#endif
