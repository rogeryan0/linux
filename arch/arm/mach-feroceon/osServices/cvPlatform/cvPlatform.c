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
#include "mvTypes.h"
#include "cvPlatform.h"
#include "mvCntmr.h"
#include "mvUart.h"

MV_U32 mvOsIoVirtToPhy( MV_VOID* pDev, MV_VOID* pVirtAddr)
{
	return (MV_U32)CV_VIRTUAL_TO_PHY((MV_U32)pVirtAddr);
}

MV_VOID* mvOsIoUncachedMalloc( MV_VOID* pDev, int size, MV_U32* pPhyAddr )
{
	*pPhyAddr = (MV_U32)cvMalloc(size);
	return (MV_VOID *)(*pPhyAddr);
}

MV_VOID mvOsIoUncachedFree( MV_VOID* pDev, int size, MV_U32 phyAddr, MV_VOID* pVirtAddr )
{
	cvFree(pVirtAddr);
}

MV_VOID* mvOsIoCachedMalloc( MV_VOID* pDev, MV_U32 size, MV_U32* pPhyAddr )
{
	*pPhyAddr = (MV_U32)cvMalloc(size);
	return (MV_VOID *)(*pPhyAddr);
}

MV_VOID mvOsIoCachedFree( MV_VOID* pDev, int size, MV_U32 phyAddr, MV_VOID* pVirtAddr )
{
	cvFree(pVirtAddr);
}


MV_VOID cvPrintf(MV_8 *fmt, ...)
{
    va_list  ap;
    MV_8     buf[2048];
    
    if(strlen(fmt) > 2048)
        uboot_printf("String is to long...\n");
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    uboot_printf( buf);
    va_end(ap);
}



/*******************************************************************************
* cvPlatformMiliSecDelay - halts the system for a desired mili - seconds.
*
* DESCRIPTION:
*       This function halts the system for a desired  mili-seconds delivered
*       by the parameter ‘miliSec’.It uses one of the 8 counter/timers within 
*       the MV by loading it with the ‘miliSec’ parameter multiplied
*       with 1 mili-second ( 1 mili-second is calculated by dividing the ‘
*       Tclock’ parameter by 1000) and waiting in a loop for it to end the 
*       count.
* INPUT:
*       miliSec - The desirable time ( in mili -seconds) to halt the system.
*       Tclock -  The MV core clock .
*       countNum - The desired counter/timer (one out of possible 8 
*                  counter/timers within the MV). 
* OUTPUT:
*       None.
*
* RETURN:
*       None.
*
*******************************************************************************/
MV_VOID cvPlatformMiliSecDelay(MV_U32 miliSec,MV_U32 Tclock,
                            MV_U32 countNum)
{
    MV_U32 oneMiliSec;
    MV_CNTMR_CTRL   counterCnrl;

    counterCnrl.enable = MV_TRUE;
    counterCnrl.autoEnable = MV_FALSE;
    oneMiliSec = Tclock/1000;
    mvCntmrDisable(countNum);	            
    mvCntmrCtrlSet(countNum,&counterCnrl);
    mvCntmrLoad(countNum,miliSec * oneMiliSec);	
    mvCntmrEnable(countNum);	            
    while(mvCntmrRead(countNum));
}


/*******************************************************************************
* cvPlatformMicroSecDelay - halts the system for the desired micro-seconds.
*
* DESCRIPTION:
*       This function halts the system for a desired micro-seconds delivered by
*       the parameter ‘microSec’. It uses one of the 8 counter/timers within the
*       MV by loading it with the ‘microSec’ parameter multiplied with 1 
*       micro-second ( 1 micro-second is calculated by dividing the ‘Tclock’ 
*       parameter by 1000000) and waiting in a loop for it to end the count . 
* INPUT:
*       microSec - The desirable time ( in micro -seconds) to halt the system.
*       Tclock -  The MV core clock .
*       countNum - The desired counter/timer (one out of possible 8 
*                  counter/timers within the MV).  
* OUTPUT:
*       None.
*
* RETURN:
*       None.
*
*******************************************************************************/
MV_VOID cvPlatformMicroSecDelay(MV_U32 microSec,MV_U32 Tclock,
                             MV_U32 countNum)
{
    MV_U32 oneMicroSec;

    MV_CNTMR_CTRL   counterCnrl;

    counterCnrl.enable = MV_TRUE;
    counterCnrl.autoEnable = MV_FALSE;
    oneMicroSec = Tclock/1000000;
    mvCntmrDisable(countNum);	            
    mvCntmrCtrlSet(countNum,&counterCnrl);
    mvCntmrLoad(countNum,microSec * oneMicroSec);	
    mvCntmrEnable(countNum);	            
    while(mvCntmrRead(countNum));
}

/* Malloc/Free globals */

MV_U32        mallocBaseAddress = (MV_U32)CV_MALLOC_BASE;
MV_U32          mallocSize = CV_MALLOC_SIZE;
MV_U32          callFlag = 1,mallocInitFlag = 1;
static HEADER   base;
static HEADER   *freep = NULL;     /* K&R called freep, freep */


static HEADER * moreMem(MV_U32 nu)
{
    MV_VOID    *cp;                          
    HEADER  *up;                         
    MV_U32 rnu;
    MV_U32   address;                         
                                                 
    if(callFlag == 1)
    {
        if(mallocInitFlag != 2)
        {
            for(address = mallocBaseAddress; address < (mallocBaseAddress + mallocSize) ; address+=128)
            {
               //CV_ALLOCATE_CACHE_LINE_WITH_ZEROS(address);
               CV_FLUSH_AND_INVALIDATE(address);
            }
        }
        callFlag = 2;
    }
    else
    {
        printf("ERROR: cvMalloc - moreMem called twice...\n");
        return NULL;
    }
    
    rnu = mallocSize / sizeof (HEADER); 
    cp = (MV_VOID *)mallocBaseAddress;           
    up = (HEADER *) cp;                          
    up->s.size = rnu;                            
    cvFree ((MV_VOID *)(up + 1));                     
    return (freep);                               
}

/* Malloc/Free implementations */
MV_VOID  * cvMalloc(size_t nbytes)
{
    register HEADER *p, *prevp; /* K&R called prevp, prevp */
    register MV_U32 nunits;

    if(callFlag == 1)
    {
        freep = NULL;
    }
    nunits = (nbytes + sizeof (HEADER) - 1) / sizeof (HEADER) + 1;
    if((prevp = freep) == NULL)
    { /* no free list yet */
        base.s.ptr = freep = prevp = &base;
        base.s.size = 0;
    }
    for(p = prevp->s.ptr;; prevp = p, p = p->s.ptr)
    {
        if (p->s.size >= nunits)
        {  /* big enough */
            if (p->s.size == nunits)    /* exactly */
                prevp->s.ptr = p->s.ptr;
            else
            {      /* allocate tail end */
                p->s.size -= nunits;
                p += p->s.size;
                p->s.size = nunits;
            }
            freep = prevp;
            return((MV_VOID *)(p + 1));
        }
        if (p == freep)
            if ((p = moreMem(nunits)) == NULL)
            {
                printf("ERROR: cvMalloc - moreMem returned with NULL...\n");
                return NULL;
            }
    }
}
/* free: put block ap in free list */
MV_VOID cvFree(MV_VOID *ap)
{
    HEADER *bp, *p;
    bp = (HEADER *)ap - 1; /* point to block header */
    for (p = freep; !((bp > p) && (bp < p->s.ptr)); p = p->s.ptr)
        if ((p >= p->s.ptr) && (bp > p || bp < p->s.ptr))
            break; /* freed block at start or end of arena */
    if ((bp + bp->s.size) == p->s.ptr)
    { /* join to upper nbr */
        bp->s.size += p->s.ptr->s.size;
        bp->s.ptr = p->s.ptr->s.ptr;
    } else
        bp->s.ptr = p->s.ptr;
    if (p + p->s.size == bp)
    { /* join to lower nbr */
        p->s.size += bp->s.size;
        p->s.ptr = bp->s.ptr;
    } else
        p->s.ptr = bp;
    freep = p;
}


MV_U32 allocsize (char *ap)
{
    register HEADER *p;

    p = (HEADER *) ap - 1;
    return(p->s.size * sizeof (HEADER));
}

MV_VOID cvInitMalloc(MV_U32 baseAddress,MV_U32 size)
{
    freep           = NULL;
    MV_U32   address;
    
    mallocBaseAddress = baseAddress;
    mallocSize        = size;

    mallocInitFlag = 2;

    for(address = baseAddress; address < (baseAddress + size) ; address+=128)
    {
        //CV_ALLOCATE_CACHE_LINE_WITH_ZEROS(address);
        CV_FLUSH_AND_INVALIDATE(address);
    }
}


MV_U8 getCDirect()
{
	volatile MV_UART_PORT *pUartPort = mvUartBase(0);
	while ((pUartPort->lsr & LSR_DR) == 0) ;
	return (pUartPort->rbr);
}


#ifndef DISABLE_VIRTUAL_TO_PHY
/*******************************************************************************
* VIRTUAL_TO_PHY - Converts addresses from virtual to phisical according to the
*                  system memory map.
* DESCRIPTION:
*      This function converts addresses from virtual to phisical according to 
*      the system memory map.
*
* NOTE!!!!!
*      This function is system dependent and must be changed accorind to the 
*      system's memory mapping.
*
* INPUT:
*      address - The address to be converted from virtual to phisical.
*
* OUTPUT:
*      None.
*
* RETURN:
*      None.
*
*******************************************************************************/
MV_U32  CV_VIRTUAL_TO_PHY(MV_U32 address)           
{
    if(address >= 0x80000000 && address <= 0x8fffffff)
        return (address & 0x7fffffff);
    if(address >= 0xa0000000 && address <= 0xafffffff)
        return ((address & 0x0fffffff) | 0x90000000);
    if(address >= 0xe0000000 && address <= 0xefffffff)
        return ((address | 0x10000000));
    return address;
}



#endif /*DISABLE_VIRTUAL_TO_PHY*/










