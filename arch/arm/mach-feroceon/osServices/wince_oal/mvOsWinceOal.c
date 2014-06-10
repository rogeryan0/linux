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

#include "mvOsWinceOal.h"
#include "mvTypes.h"




unsigned long mvOsGetCurrentTime(void)
{
	/* maen later */
	return 0;
}

void mvOsSleep(unsigned long mils)
{
	/* maen later */
}

int	mvOsRand(void)
{
	/* maen later */
	return 0;
}

/********************/
/*   Semaphors      */
/********************/

MV_STATUS   mvOsSemCreate(char *name,unsigned long init,unsigned long count,
					 unsigned long *smid)
{
	/* maen later */
	return MV_OK;
}
MV_STATUS 	mvOsSemDelete(unsigned long smid)
{
	/* maen later */
	return MV_OK;
}

MV_STATUS	mvOsSemWait(unsigned long smid, unsigned long time_out)
{
	/* maen later */
	return MV_OK;
}
MV_STATUS	mvOsSemSignal(unsigned long smid)
{
	/* maen later */
	return MV_OK;
}



/*********************/
/*    Memory         */
/*********************/


void*   mvOsMalloc(MV_U32 size)
{
	/* maen later */
	return NULL;
}

void*   mvOsRealloc(void *pVirtAddr, MV_U32 size)
{
	/* maen later */
	return NULL;
}

void mvOsFree(void *pVirtAddr)
{
	/* maen later */
}


void* mvOsIoUncachedMalloc( void* pDev, MV_U32 size, MV_U32* pPhyAddr )
{
    *pPhyAddr = (MV_U32)OALVAtoPA(mvOsMalloc(size));
    return (void *)OALPAtoUA(*pPhyAddr);
}

void mvOsIoUncachedFree( void* pDev, MV_U32 size, MV_U32 phyAddr, void* pVirtAddr )
{
    mvOsFree(pVirtAddr);
}

void* mvOsIoCachedMalloc( void* pDev, MV_U32 size, MV_U32* pPhyAddr )
{
    *pPhyAddr = (MV_U32)OALVAtoPA(mvOsMalloc(size));
    return (void *)OALPAtoCA(*pPhyAddr);
}

void mvOsIoCachedFree( void* pDev, MV_U32 size, MV_U32 phyAddr, void* pVirtAddr )
{
    mvOsFree(pVirtAddr);
}

MV_U32 mvOsCacheFlush( void* pDev, void* p, int size )
{
	OALFlushDCacheLines(p,size);
    return (MV_U32)p;
}

MV_U32 mvOsCacheInvalidate( void* pDev, void* p, int size )
{
	OALFlushDCacheLines(p,size);
    return (MV_U32)p;
}



void mvOsPrintf(const char *sz, ...)
{
	unsigned char	c;
	static WCHAR 	szPrintfBuf[384];
    va_list         arglist;
    LPCWSTR			lpszFmt=&szPrintfBuf[0];
    unsigned int	j=0;
	int				skip_options;
	

	va_start(arglist, sz);

    while (*sz) {
        c = *sz++;
        switch (c) {
		case '%':
			szPrintfBuf[j++]=(WCHAR)c;
			do
			{	
				skip_options=0;
				c = *sz++;
                switch (c) { 
				case 's':
					szPrintfBuf[j++]=(WCHAR)'S';
					break;
				case 'S':
					szPrintfBuf[j++]=(WCHAR)'s';
					break;
				case 'x':
				case 'X':
				case 'B':
				case 'H':
				case 'd':
				case 'i':
				case 'u':
				case '%':
				case 'c':
					szPrintfBuf[j++]=(WCHAR)c;
					break;
				default:
					szPrintfBuf[j++]=(WCHAR)c;
					skip_options=1;
                break;
				}

			}while(skip_options);
            break;
        case '\n':
			szPrintfBuf[j++]=(WCHAR)'\r';
			szPrintfBuf[j++]=(WCHAR)'\n';
			break;
        default:
			szPrintfBuf[j++]=(WCHAR)c;
			break;
		}
    }
	szPrintfBuf[j++]=(WCHAR)'\0';

	NKvDbgPrintfW(lpszFmt, arglist);
    va_end(arglist);

}



unsigned int mvOsSPrintf (unsigned char *pBuf, const unsigned char *sz, ...)
{
	unsigned char	c;
	static WCHAR 	szPrintfBuf[384];
	static WCHAR 	rgchBuf[384];
    va_list         arglist;
    LPCWSTR			lpszFmt=&szPrintfBuf[0];
    unsigned int	j=0;
	int				skip_options;
	

	va_start(arglist, sz);

    while (*sz) {
        c = *sz++;
        switch (c) {
		case '%':
			szPrintfBuf[j++]=(WCHAR)c;
			do
			{	
				skip_options=0;
				c = *sz++;
            switch (c) { 
				case 's':
					szPrintfBuf[j++]=(WCHAR)'S';
					break;
				case 'S':
					szPrintfBuf[j++]=(WCHAR)'s';
					break;
				case 'x':
				case 'X':
				case 'B':
				case 'H':
				case 'd':
				case 'i':
				case 'u':
				case '%':
				case 'c':
					szPrintfBuf[j++]=(WCHAR)c;
					break;
				default:
					szPrintfBuf[j++]=(WCHAR)c;
					skip_options=1;
                break;
				}

			}while(skip_options);
                break;
            default:
			szPrintfBuf[j++]=(WCHAR)c;
			break;
		}
    }
	szPrintfBuf[j++]=(WCHAR)'\0';

   	NKwvsprintfW(rgchBuf, lpszFmt, arglist, sizeof(rgchBuf)/sizeof(WCHAR));

	// Convert to char
	lpszFmt = &rgchBuf[0];
	j=0;
	do
	{
		*pBuf++ = (unsigned char)*lpszFmt++;
		j++;
	}while (*lpszFmt);

	*pBuf = '\0';
    va_end(arglist);

	return j;

}

