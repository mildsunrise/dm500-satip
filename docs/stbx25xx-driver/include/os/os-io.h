//pallas/drv/include/os/os-io.h
/*----------------------------------------------------------------------------+
|
|       This source code has been made available to you by IBM on an AS-IS
|       basis.  Anyone receiving this source is licensed under IBM
|       copyrights to use it in any way he or she deems fit, including
|       copying it, modifying it, compiling it, and redistributing it either
|       with or without modifications.  No license under IBM patents or
|       patent applications is to be implied by the copyright license.
|
|       Any user of this software should understand that IBM cannot provide
|       technical support for this software and will not be responsible for
|       any consequences resulting from the use of this software.
|
|       Any person who transfers this source code or any derivative work
|       must include the IBM copyright notice, this paragraph, and the
|       preceding two paragraphs in the transferred software.
|
|       COPYRIGHT   I B M   CORPORATION 1998
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
//
//Comment: 
//  A common PPC IO access interface for Linux  
//Revision Log:   
//  Aug/31/2001			                  Created by YYD
//  Oct/22/2001		Renamed, Add SPR and mmap I/O by YYD

#ifndef  _DRV_INCLUDE_OS_OS_IO_H_INC_
#define  _DRV_INCLUDE_OS_OS_IO_H_INC_

#define __to_asm_token(s)   #s

#define MF_DCR(rn) __mfdcr_or_dflt(rn, 0)

#define __mfdcr_or_dflt(rn,default_rval) \
        ({unsigned int rval;                                            \
        if (rn == 0)                                                    \
                rval = default_rval;                                    \
        else                                                            \
                asm volatile("mfdcr %0," __to_asm_token(rn) : "=r" (rval));  \
        rval;})

#define MT_DCR(rn, v)  \
        {if (rn != 0) \
                asm volatile("mtdcr " __to_asm_token(rn) ",%0" : : "r" (v));}


#define MF_SPR(rn) __mfspr_or_dflt(rn, 0)

#define __mfspr_or_dflt(rn,default_rval) \
        ({unsigned int rval;                                            \
        if (rn == 0)                                                    \
                rval = default_rval;                                    \
        else                                                            \
                asm volatile("mfspr %0," __to_asm_token(rn) : "=r" (rval));  \
        rval;})

#define MT_SPR(rn, v)  \
        {if (rn != 0) \
                asm volatile("mtspr " __to_asm_token(rn) ",%0" : : "r" (v));}


#include <asm/io.h>     // for OS defined io routines

// BYTE
#define _OS_INB(uPort)           in_8((volatile BYTE *)(uPort))
#define _OS_OUTB(uPort, data)    out_8((volatile BYTE *)(uPort), (int)(data))

// big endian I/O routines
// USHORT (16)
#define _OS_INW(uPort)           in_be16((volatile USHORT *)(uPort))
#define _OS_OUTW(uPort, data)    out_be16((volatile USHORT *)(uPort), (int)(data))

// UINT (32)
#define _OS_INL(uPort)           in_be32((volatile UINT32 *)(uPort))
#define _OS_OUTL(uPort, data)    out_be32((volatile UINT32 *)(uPort), (int)(data))

// little endian I/O routines
// USHORT (16)
#define _OS_INWX(uPort)          in_le16((volatile USHORT *)(uPort))
#define _OS_OUTWX(uPort, data)   out_le16((volatile USHORT *)(uPort), (int)(data))

// UINT (32)
#define _OS_INLX(uPort)          in_le32((volatile UINT32 *)(uPort))
#define _OS_OUTLX(uPort, data)   out_1e32((volatile UINT32 *)(uPort), (int)(data))


#endif  //  _DRV_INCLUDE_OS_OS_IO_H_INC_



