/*******************************************************************************
*                Copyright 2004, MARVELL SEMICONDUCTOR, LTD.                   *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL.                      *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
*                                                                              *
* MARVELL COMPRISES MARVELL TECHNOLOGY GROUP LTD. (MTGL) AND ITS SUBSIDIARIES, *
* MARVELL INTERNATIONAL LTD. (MIL), MARVELL TECHNOLOGY, INC. (MTI), MARVELL    *
* SEMICONDUCTOR, INC. (MSI), MARVELL ASIA PTE LTD. (MAPL), MARVELL JAPAN K.K.  *
* (MJKK), MARVELL ISRAEL LTD. (MSIL).                                          *
*******************************************************************************/
 
#ifndef _CVPLATFORMASM_H_
#define _CVPLATFORMASM_H_


#include "mvCommon.h"


#ifdef __cplusplus
extern "C" {
#endif
 
#if defined (MV_PPC)
 #include <config.h>
 #include <ppc_asm.tmpl>
 #include <asm/cache.h>
 #include <asm/mmu.h>
 #include <ppc_defs.h>
 #include <74xx_7xx.h>

#elif defined(MV_PPC64)
 #include <config.h>
 #include <ppc_asm.tmpl>
 #include <asm/cache.h>
 #include <asm/mmu.h>
 #include <ppc_defs.h>
 #include <ppc970.h>
#else
 #error "MV Arch type not selected"
#endif


#ifdef __cplusplus
}
#endif
 
#endif /* _MVOSUBOOTASM_H_ */

