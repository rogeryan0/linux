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
/*******************************************************************************
* mvOsCpuArchLib.c - Marvell CPU architecture library
*
* DESCRIPTION:
*       This library introduce Marvell API for OS dependent CPU architecture 
*       APIs. This library introduce single CPU architecture services APKI 
*       cross OS.
*
* DEPENDENCIES:
*       None.
*
*******************************************************************************/


#ifndef __INCcvPlatformArmh
#define __INCcvPlatformArmh

//#include "mvTypes.h"

#include "mvCommon.h"


#define CPU_PHY_MEM(x)			    (MV_U32)x
#define CPU_MEMIO_CACHED_ADDR(x)    (void*)x
#define CPU_MEMIO_UNCACHED_ADDR(x)	(void*)x

/* Flash APIs */
#define MV_FL_8_READ            MV_MEMIO8_READ
#define MV_FL_16_READ           MV_MEMIO_LE16_READ
#define MV_FL_32_READ           MV_MEMIO_LE32_READ
#define MV_FL_8_DATA_READ       MV_MEMIO8_READ
#define MV_FL_16_DATA_READ      MV_MEMIO16_READ
#define MV_FL_32_DATA_READ      MV_MEMIO32_READ
#define MV_FL_8_WRITE           MV_MEMIO8_WRITE
#define MV_FL_16_WRITE          MV_MEMIO_LE16_WRITE
#define MV_FL_32_WRITE          MV_MEMIO_LE32_WRITE
#define MV_FL_8_DATA_WRITE      MV_MEMIO8_WRITE
#define MV_FL_16_DATA_WRITE     MV_MEMIO16_WRITE
#define MV_FL_32_DATA_WRITE     MV_MEMIO32_WRITE


/* CPU architecture dependent 32, 16, 8 bit read/write IO addresses */
#define MV_MEMIO32_WRITE(addr, data)	\
    ((*((volatile unsigned int*)(addr))) = ((unsigned int)(data)))

#define MV_MEMIO32_READ(addr)       	\
    ((*((volatile unsigned int*)(addr))))

#define MV_MEMIO16_WRITE(addr, data)	\
    ((*((volatile unsigned short*)(addr))) = ((unsigned short)(data)))

#define MV_MEMIO16_READ(addr)       	\
    ((*((volatile unsigned short*)(addr))))

#define MV_MEMIO8_WRITE(addr, data) 	\
    ((*((volatile unsigned char*)(addr))) = ((unsigned char)(data)))

#define MV_MEMIO8_READ(addr)        	\
    ((*((volatile unsigned char*)(addr))))


#define MV_BYTE_SWAP_32BIT_FAST(value)  MV_BYTE_SWAP_32BIT(value)
#define MV_BYTE_SWAP_16BIT_FAST(value)  MV_BYTE_SWAP_16BIT(value)


/* 32bit read in little endian mode */
#if defined(MV_CPU_LE)
#   define MV_32BIT_LE_FAST(val)            (val)
#   define MV_16BIT_LE_FAST(val)            (val)
#elif defined(MV_CPU_BE)
#   define MV_32BIT_LE_FAST(val)            MV_BYTE_SWAP_32BIT_FAST(val)
#   define MV_16BIT_LE_FAST(val)            MV_BYTE_SWAP_16BIT_FAST(val)
#else
    #error "CPU endianess isn't defined!!\n"
#endif
    
/* 16bit write in little endian mode */
#define MV_MEMIO_LE16_WRITE(addr, data) \
        MV_MEMIO16_WRITE(addr, MV_16BIT_LE_FAST(data))

/* 16bit read in little endian mode */
static __inline MV_U16 MV_MEMIO_LE16_READ(MV_U32 addr)
{
	MV_U16 data;

	data = (MV_U16)MV_MEMIO16_READ(addr);

	return (MV_U16)MV_16BIT_LE_FAST(data);
}

/* 32bit write in little endian mode */
#define MV_MEMIO_LE32_WRITE(addr, data) \
        MV_MEMIO32_WRITE(addr, MV_32BIT_LE_FAST(data))

/* 32bit read in little endian mode */
static __inline MV_U32 MV_MEMIO_LE32_READ(MV_U32 addr)
{
	MV_U32 data;

	data= (MV_U32)MV_MEMIO32_READ(addr);

	return (MV_U32)MV_32BIT_LE_FAST(data);
}

/* CPU cache information */
#define CPU_I_CACHE_LINE_SIZE   32    /* 2do: replace 32 with linux core macro */
#define CPU_D_CACHE_LINE_SIZE   32    /* 2do: replace 32 with linux core macro */



/* Typedefs for CPU related functions and MACROS */
                      
#define stringify(s)	tostring(s)

#define tostring(s)	#s

#define CV_CACHE_LINE_SIZE	    CPU_D_CACHE_LINE_SIZE    

#define CV_INVALIDATE_CACHE_LINE(Address)\
    __asm__ __volatile__("mcr    p15, 0, %0, c7, c6, 1" : : "r" (Address));

#define CV_FLUSH_AND_INVALIDATE(Address)\
    __asm__ __volatile__("mcr    p15, 0, %0, c7, c14, 1" : : "r" (Address));

#define CV_CLEAN_CACHE_LINE(Address)\
    __asm__ __volatile__("mcr    p15, 0, %0, c7, c10, 1" : : "r" (Address));
    
#define CV_DRAIN_WRITE_BUFFER(Address)\
    __asm__ __volatile__("mcr    p15, 0, %0, c7, c10, 4" : : "r" (Address));


#define CV_FLUSH_CACHE(from,to)\
    {\
        unsigned int addr;\
        for(addr = from ; addr < to ; addr += 32)\
        {\
            CV_FLUSH_AND_INVALIDATE(addr);\
        }\
    }
    
#define CV_CLEAN_CACHE(from,to)\
    {\
        MV_ULONG addr;\
        for(addr = from ; addr < to ; addr += CV_CACHE_LINE_SIZE)\
        {\
            CV_CLEAN_CACHE_LINE(addr);\
        }\
        if(to % CV_CACHE_LINE_SIZE) CV_CLEAN_CACHE_LINE(to);\
    }

    
#define CV_INVALIDATE_CACHE(from,to)\
    {\
        unsigned int addr;\
        for(addr = from ; addr < to ; addr += 32)\
        {\
            CV_INVALIDATE_CACHE_LINE(addr);\
        }\
    }
    
#define CV_READ64BIT(Address)\
	(\
        {\
        unsigned long long rval;\
        __asm__ volatile("ldrd %0,[%1,#0]" : "=r" (rval) : "r" (Address));\
        rval;\
        }\
    )

/* Data cache flush one line */
#define mvOsCacheLineFlushInv(addr)
#define mvOsCacheLineInv(addr)
/* Flush CPU pipe */
#define CPU_PIPE_FLUSH

/* ARM architecture APIs */

MV_U32  mvOsCpuRevGet (MV_VOID);
MV_U32  mvOsCpuPartGet (MV_VOID);
MV_U32  mvOsCpuArchGet (MV_VOID);
MV_U32  mvOsCpuVarGet (MV_VOID);
MV_U32  mvOsCpuAsciiGet (MV_VOID);

#endif /* __INCcvPlatformArmh */
