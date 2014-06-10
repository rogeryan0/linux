/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/sysdev.h>
#include <asm/mach/time.h>
#if defined(CONFIG_MTD_PHYSMAP) 
#include <linux/mtd/physmap.h>
#endif
#include <linux/clocksource.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/setup.h>
#include <asm/mach-types.h>

#include <asm/mach/arch.h>
#include <asm/mach/flash.h>
#include <asm/mach/irq.h>
#include <asm/mach/map.h>
#include <asm/arch/system.h>
#include <asm/arch/orion_ver.h>

#include <asm/vfp.h>

#include <linux/tty.h>
#include <linux/platform_device.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/serial_8250.h>
#include <linux/serial_reg.h>
#include <asm/serial.h>

#include <asm/arch/serial.h>

#include "mvCtrlEnvLib.h"
#include "mvCpuIf.h"
#include "mvDevice.h"
#include "mvBoardEnvLib.h"
#include "mvDebug.h"
#include "mvSysHwConfig.h"
#include "mvPexRegs.h"
#include "mvCntmr.h"

#ifdef CONFIG_MV_INCLUDE_IDMA
#   include "mvIdma.h"
#endif
#ifdef CONFIG_MV_INCLUDE_USB
#   include "mvUsb.h"
#endif

#ifdef CONFIG_MV_INCLUDE_CESA
#include "mvCesa.h"
#include "mvMD5.h"
#include "mvSHA1.h"
#endif

#if defined (CONFIG_MV_INCLUDE_INTEG_MFLASH) || defined (CONFIG_MV_INCLUDE_SPI)
#include <mvCtrlEnvLib.h>
#endif

#ifdef CONFIG_MV_INCLUDE_SPI
#include <mvSFlash.h>
#include <mvSFlashSpec.h>
#endif

#ifdef CONFIG_MV_INCLUDE_INTEG_MFLASH
#include <mvMFlash.h>
#include <mvMFlashSpec.h>
#endif

#ifdef CONFIG_MV_INCLUDE_INTEG_SATA
#include <mvSataAddrDec.h>
#endif
extern u32 mvTclk;
extern u32 mvSysclk;
extern u32 mvIsUsbHost;
extern int mv_idma_usage_get(int* free_map);

EXPORT_SYMBOL(mvCtrlPwrClckGet);
EXPORT_SYMBOL(mvTclk);
EXPORT_SYMBOL(mvSysclk);
EXPORT_SYMBOL(mvCtrlModelGet);
EXPORT_SYMBOL(mvOsIoUncachedMalloc);
EXPORT_SYMBOL(mvOsIoUncachedFree);
EXPORT_SYMBOL(mvOsIoCachedMalloc);
EXPORT_SYMBOL(mvOsIoCachedFree);
EXPORT_SYMBOL(mvDebugMemDump);
EXPORT_SYMBOL(mvHexToBin);
EXPORT_SYMBOL(mvBinToHex);
#ifdef CONFIG_MV_INCLUDE_USB
EXPORT_SYMBOL(mvIsUsbHost);
EXPORT_SYMBOL(mvCtrlUsbMaxGet);
#endif
EXPORT_SYMBOL(mvCtrlModelRevGet);

#ifdef CONFIG_MV_INCLUDE_IDMA
EXPORT_SYMBOL(mv_idma_usage_get);
EXPORT_SYMBOL(mvDmaTransfer);
EXPORT_SYMBOL(mvDmaStateGet);
#endif
#ifdef CONFIG_MV_INCLUDE_USB
EXPORT_SYMBOL(mvUsbGetCapRegAddr);
EXPORT_SYMBOL(mvUsbGppInit);
EXPORT_SYMBOL(mvUsbBackVoltageUpdate);
#endif /* CONFIG_MV_INCLUDE_USB */

#ifdef CONFIG_MV_INCLUDE_CESA
EXPORT_SYMBOL(mvCesaInit);
EXPORT_SYMBOL(mvCesaSessionOpen);
EXPORT_SYMBOL(mvCesaSessionClose);
EXPORT_SYMBOL(mvCesaAction);
EXPORT_SYMBOL(mvCesaReadyGet);
EXPORT_SYMBOL(mvCesaCopyFromMbuf);
EXPORT_SYMBOL(mvCesaCopyToMbuf);
EXPORT_SYMBOL(mvCesaMbufCopy);
EXPORT_SYMBOL(mvCesaCryptoIvSet);
EXPORT_SYMBOL(mvMD5);
EXPORT_SYMBOL(mvSHA1);

EXPORT_SYMBOL(mvCesaDebugQueue);
EXPORT_SYMBOL(mvCesaDebugSram);
EXPORT_SYMBOL(mvCesaDebugSAD);
EXPORT_SYMBOL(mvCesaDebugStatus);
EXPORT_SYMBOL(mvCesaDebugMbuf);
EXPORT_SYMBOL(mvCesaDebugSA);
#endif /* CONFIG_MV_CESA */
#ifdef CONFIG_MV_INCLUDE_CESA
extern unsigned char*  mv_sram_usage_get(int* sram_size_ptr);
EXPORT_SYMBOL(mv_sram_usage_get);
#endif

#if defined (CONFIG_MV_INCLUDE_SPI)
EXPORT_SYMBOL(mvCtrlSpiBusModeDetect);
#endif

#ifdef CONFIG_MV_INCLUDE_INTEG_MFLASH
EXPORT_SYMBOL(mvMFlashInit);
EXPORT_SYMBOL(mvMFlashChipErase);
EXPORT_SYMBOL(mvMFlashMainErase);
EXPORT_SYMBOL(mvMFlashInfErase);
EXPORT_SYMBOL(mvMFlashSecErase);
EXPORT_SYMBOL(mvMFlashBlockWr);
EXPORT_SYMBOL(mvMFlashInfBlockWr);
EXPORT_SYMBOL(mvMFlashBlockRd);
EXPORT_SYMBOL(mvMFlashBlockInfRd);
EXPORT_SYMBOL(mvMFlashReadConfig);
EXPORT_SYMBOL(mvMFlashSetConfig);
EXPORT_SYMBOL(mvMFlashSetSlewRate);
EXPORT_SYMBOL(mvMFlashWriteProtectSet);
EXPORT_SYMBOL(mvMFlashWriteProtectGet);
EXPORT_SYMBOL(mvMFlashSectorSizeSet);
EXPORT_SYMBOL(mvMFlashPrefetchSet);
EXPORT_SYMBOL(mvMFlashShutdownSet);
EXPORT_SYMBOL(mvMFlashIdGet);
EXPORT_SYMBOL(mvMFlashReset);
#endif

#ifdef CONFIG_MV_INCLUDE_SPI
EXPORT_SYMBOL(mvSFlashInit);
EXPORT_SYMBOL(mvSFlashSectorErase);
EXPORT_SYMBOL(mvSFlashChipErase);
EXPORT_SYMBOL(mvSFlashBlockRd);
EXPORT_SYMBOL(mvSFlashBlockWr);
EXPORT_SYMBOL(mvSFlashIdGet);
EXPORT_SYMBOL(mvSFlashWpRegionSet);
EXPORT_SYMBOL(mvSFlashWpRegionGet);
EXPORT_SYMBOL(mvSFlashStatRegLock);
EXPORT_SYMBOL(mvSFlashSizeGet);
EXPORT_SYMBOL(mvSFlashPowerSaveEnter);
EXPORT_SYMBOL(mvSFlashPowerSaveExit);
EXPORT_SYMBOL(mvSFlashModelGet);
#endif

#ifdef CONFIG_MV_INCLUDE_INTEG_SATA
EXPORT_SYMBOL(mvSataWinInit);
#endif
#if defined (CONFIG_MV_XORMEMCOPY) || defined (CONFIG_MV_IDMA_MEMCOPY)
EXPORT_SYMBOL(asm_memcpy);
EXPORT_SYMBOL(asm_memmove);
#endif
#if defined (CONFIG_MV_XORMEMZERO) || defined (CONFIG_MV_IDMA_MEMZERO)
EXPORT_SYMBOL(asm_memzero);
#endif
