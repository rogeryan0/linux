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
/********************************************************************************
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

/* includes */
#include "cvPlatformArm.h"
#include "mvCpu.h"

/*************** Definitions related to SATA files****************************/
/* includes */
#include "mvSata.h"
#include "cvAuxil.h"    


#ifdef __cplusplus
extern "C" {
#endif 

MV_U32                gDeviceDisconnected = MV_FALSE;
MV_U8                 gChannelDisconnected;
struct mvSataAdapter *gpSataAdapter;
MV_U32                gTclock,gError;

/* Delay function in micro seconds resolution */
MV_VOID mvMicroSecondsDelay(MV_VOID_PTR pVoid, MV_U32 microSec)
{
    cvPlatformMicroSecDelay(microSec, gTclock,0);
}

/* Event notify function */
MV_BOOLEAN mvSataEventNotify(struct mvSataAdapter *adapter ,
                             MV_EVENT_TYPE type,MV_U32 a, MV_U32 b) 
{
    switch(type)
    {
    case MV_EVENT_TYPE_ADAPTER_ERROR:
        printf("\nEvent notify: Adapter#%d - Adapter error, pciCauseReg = %x.\n",
                                                     adapter->adapterId,a);
        gError = 1;
        break;
    case MV_EVENT_TYPE_SATA_CABLE:
        if(a == MV_SATA_CABLE_EVENT_CONNECT)
        {
            printf("\nEvent notify: Adapter#%d - channel#%d - device was connected.\n",
                   adapter->adapterId,b);
        }
        else if(a == MV_SATA_CABLE_EVENT_DISCONNECT)
        {
            printf("\nEvent notify: Adapter#%d - channel#%d - device was disconected.\n",
                   adapter->adapterId,b);
            gDeviceDisconnected = MV_TRUE;
            gChannelDisconnected = b;
            gpSataAdapter = adapter;
        }
        break;

    case MV_EVENT_TYPE_SATA_ERROR:
        switch(a)
        {
        case MV_SATA_DEVICE_ERROR:
            printf("\nEvent notify: AdapterID#%d - ",adapter->adapterId);
            printf("'SATA device error' on channel#%d.\n",b);
            gError = 1;
            break;
        case MV_SATA_UNRECOVERABLE_COMMUNICATION_ERROR:
            printf("\nEvent notify: AdapterID#%d - ",adapter->adapterId);
            printf("'SATA unrecoverable communication error' on channel#%d.\n",b);
            gError = 1;
            break;
        case MV_SATA_RECOVERABLE_COMMUNICATION_ERROR:
            printf("\nEvent notify: AdapterID#%d - ",adapter->adapterId);
            printf("'SATA recoverable communication error' on channel#%d.\n",b);
            break;
        default:
            printf("\nEvent notify: AdapterID#%d - ",adapter->adapterId);
            printf("'Unknown error' on channel#%d.\n",b);
            break;
        }
        break;


    default:
        printf("\nEvent notify: adapterID #%d - Unknown type of event.\n",
                                                            adapter->adapterId);
        break;
    }
    return MV_TRUE;
}


MV_U16 mvByteSwap16(MV_U16  data)
{
#if defined (MV_88F5182)  
    return data;
#endif
    return CV_SHORT_SWAP(data);

}
MV_U32 mvByteSwap32(MV_U32  data)
{
#if defined (MV_88F5182)  
    return data;
#endif
    return CV_WORD_SWAP(data);
}

#ifdef __cplusplus
extern "C" {
#endif 

/********************* End of SATA related to SATA files *********************/

static MV_U32 read_p15_c0 (void);

/* defines  */
#define ARM_ID_REVISION_OFFS	0
#define ARM_ID_REVISION_MASK	(0xf << ARM_ID_REVISION_OFFS)

#define ARM_ID_PART_NUM_OFFS	4
#define ARM_ID_PART_NUM_MASK	(0xfff << ARM_ID_PART_NUM_OFFS)

#define ARM_ID_ARCH_OFFS		16
#define ARM_ID_ARCH_MASK	(0xf << ARM_ID_ARCH_OFFS)

#define ARM_ID_VAR_OFFS			20
#define ARM_ID_VAR_MASK			(0xf << ARM_ID_VAR_OFFS)

#define ARM_ID_ASCII_OFFS		24
#define ARM_ID_ASCII_MASK		(0xff << ARM_ID_ASCII_OFFS)



/*******************************************************************************
* mvOsCpuVerGet() - 
*
* DESCRIPTION:
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       32bit CPU Revision
*
*******************************************************************************/
MV_U32 mvOsCpuRevGet( MV_VOID )
{
	return ((read_p15_c0() & ARM_ID_REVISION_MASK ) >> ARM_ID_REVISION_OFFS);
}
/*******************************************************************************
* mvOsCpuPartGet() - 
*
* DESCRIPTION:
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       32bit CPU Part number
*
*******************************************************************************/
MV_U32 mvOsCpuPartGet( MV_VOID )
{
	return ((read_p15_c0() & ARM_ID_PART_NUM_MASK ) >> ARM_ID_PART_NUM_OFFS);
}
/*******************************************************************************
* mvOsCpuArchGet() - 
*
* DESCRIPTION:
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       32bit CPU Architicture number
*
*******************************************************************************/
MV_U32 mvOsCpuArchGet( MV_VOID )
{
    return ((read_p15_c0() & ARM_ID_ARCH_MASK ) >> ARM_ID_ARCH_OFFS);
}
/*******************************************************************************
* mvOsCpuVarGet() - 
*
* DESCRIPTION:
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       32bit CPU Variant number
*
*******************************************************************************/
MV_U32 mvOsCpuVarGet( MV_VOID )
{
    return ((read_p15_c0() & ARM_ID_VAR_MASK ) >> ARM_ID_VAR_OFFS);
}
/*******************************************************************************
* mvOsCpuAsciiGet() - 
*
* DESCRIPTION:
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       32bit CPU Variant number
*
*******************************************************************************/
MV_U32 mvOsCpuAsciiGet( MV_VOID )
{
    return ((read_p15_c0() & ARM_ID_ASCII_MASK ) >> ARM_ID_ASCII_OFFS);
}



/*
static unsigned long read_p15_c0 (void)
*/
/* read co-processor 15, register #0 (ID register) */
static MV_U32 read_p15_c0 (void)
{
	MV_U32 value;

	__asm__ __volatile__(
		"mrc	p15, 0, %0, c0, c0, 0   @ read control reg\n"
		: "=r" (value)
		:
		: "memory");

	return value;
}

