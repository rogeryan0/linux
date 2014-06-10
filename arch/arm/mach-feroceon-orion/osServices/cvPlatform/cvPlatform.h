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

#ifndef _MV_CV_PLATFORM_H_
#define _MV_CV_PLATFORM_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdarg.h>

#include "mvTypes.h"
#include "ev88Fxx81.h"
#include "cvCore.h"
#include "cvPlatformArm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Link os functionality */
#define INLINE             inline
#define mvOsPrintf         cvPrintf
#define mvOsSPrintf        sprintf
#define mvOsMemcpy         memcpy
#define mvOsMalloc(_size_) cvMalloc((size_t)_size_)
#define mvOsFree           cvFree
#define mvOsSleep(us)      mvOsDelay(us)
#define mvOsTaskLock()
#define mvOsTaskUnlock()
#define mvOsDelay(ms)      cvPlatformMiliSecDelay(ms,mvBoardTclkGet(),0)
#define mvOsUDelay(us)     cvPlatformMicroSecDelay(us,mvBoardTclkGet(),0)
#define printf			   cvPrintf
#define mvOsOutput         printf




/* The two following defines are according to PPC architecture. */
#define NONE_CACHEABLE(address)		    (address)
#ifdef MPU
#define CACHEABLE(address)		        ((address) )
#else 
#define CACHEABLE(address)		        ((address) | 0x80000000)
#endif 


/* The two following defines are according to PPC architecture. */
#define CV_NONE_CACHEABLE(address)		    (address)
#define CV_CACHEABLE(address)		 (((address)<0x10000000)? ((address)|0x80000000):(address))

#define CV_MALLOC_SIZE                     (8*_1M)
#define CV_MALLOC_BASE                     (CV_CACHEABLE(32*_1M))
#define CV_SHOW_DATE_AND_TIME()  printf("Last compilation occured on: %s, %s",\
                                                             __TIME__,__DATE__);

#ifdef DISABLE_VIRTUAL_TO_PHY
    #define CV_VIRTUAL_TO_PHY(address)     ((MV_U32)(address))
#else 
    MV_U32 CV_VIRTUAL_TO_PHY(MV_U32 address);           
#endif /* DISABLE_VIRTUAL_TO_PHY */

#define CV_PHY_TO_VIRTUAL(address)  (((MV_U32)(address)) | CV_NONE_CACHEABLE(0))


typedef double  ALIGN;
union header
{
    struct
    {
        union header   *ptr;
        unsigned int   size;
    } s;
    // Size of long long = 8, Alighnment to cache line size
    MV_U64 x[CV_CACHE_LINE_SIZE/8]; 
};
typedef union header HEADER; 


#ifndef LE
    #ifdef __USING_GNU_PPC__ /* using GNU ppc compiler */
        #define CV_SHORT_SWAP(value)                                           \
                (                                                              \
                   {                                                           \
                       MV_U16 result;                                  \
                       __asm__ ("rlwimi %0,%1,8,16,23":"=r"(result):"r"(value),\
                                                           "0" ((value) >> 8));\
                       result; /* Value is returned in this variable */        \
                   }                                                           \
                )
        #define CV_WORD_SWAP(value)                                            \
                (                                                              \
                   {                                                           \
                       MV_U32 result;                                    \
                       __asm__ ("rlwimi %0,%1,24,16,23\n\t"                    \
                                "rlwimi %0,%1,8,8,15\n\t""rlwimi %0,%1,24,0,7":\
                                   "=r"(result):"r"(value),"0"((value) >> 24));\
                       result; /* Value is returned in this variable */        \
                   }                                                           \
                )
    #endif /* __USING_GNU_PPC__ */
#endif /* LE */  
               

MV_VOID    cvPrintf(MV_8 *fmt, ...);
MV_VOID	   uboot_printf(MV_8 *fmt, ...);
MV_VOID    cvPlatformMiliSecDelay(MV_U32 miliSec,MV_U32 Tclock,MV_U32 countNum);
MV_VOID    cvPlatformMicroSecDelay(MV_U32 microSec,MV_U32 Tclock,MV_U32 countNum);
MV_U8      getCDirect();
MV_VOID    cvInitMalloc(MV_U32 baseAddress,MV_U32 size);
MV_VOID   *cvMalloc(size_t nbytes);
MV_VOID    cvFree(MV_VOID *ap);

/*****/

#define mvOsDivide(num, div)        \
({                                  \
    int i=0, rem=(num);             \
                                    \
    while(rem >= (div))             \
    {                               \
        rem -= (div);               \
        i++;                        \
    }                               \
    (i);                            \
})

#define mvOsReminder(num, div)      \
({                                  \
    int rem = (num);                \
                                    \
    while(rem >= (div))             \
        rem -= (div);               \
    (rem);                          \
})

/*****/
#ifdef __cplusplus
}
#endif

#if defined(MV_ARM)
#   include "cvPlatformArm.h"
#else
#   error "CPU type not selected"
#endif



#endif /*_MV_CV_PLATFORM_H_*/
