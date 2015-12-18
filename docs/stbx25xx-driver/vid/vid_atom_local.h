/*----------------------------------------------------------------------------+
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
|       COPYRIGHT   I B M   CORPORATION 1997, 1999, 2001
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| Author: Ling shao
| File:   vid_atom_local.h
| Purpose: video decoder local definition for PALLAS
| Changes:
| Date:         Comment:
| -----         --------
| 15-Oct-01		create                  									SL
+----------------------------------------------------------------------------*/
#ifndef PALLAS_VID_ATOM_LOCAL_H
#define PALLAS_VID_ATOM_LOCAL_H

#include <os/os-types.h>
#include "vid_atom.h"




/*---------------------------------------------------------
| MPEG video decoder command group
+---------------------------------------------------------*/
/*list of video decoder command*/
#define DECOD_COM_PLAY                  (0x0000<<1)
#define DECOD_COM_PAUSE                 (0x0001<<1)
#define DECOD_COM_SFRAME                (0x0002<<1)
#define DECOD_COM_FF                    (0x0003<<1)
#define DECOD_COM_SLOWMO                (0x0004<<1)
#define DECOD_COM_MOV_FRAME_BUF         (0x0005<<1)
#define DECOD_COM_NO_PAN_SCAN           (0x0006<<1)
#define DECOD_COM_FFRAME                (0x0007<<1)
#define DECOD_COM_RES_VID_BUF           (0x0008<<1)
#define DECOD_COM_CONF                  (0x0009<<1)
#define DECOD_COM_INIT_SP               (0x000A<<1)
#define DECOD_COM_DISP_SP               (0x000B<<1)
#define DECOD_COM_DEL_SP                (0x000C<<1)
#define DECOD_COM_FRAME_SW              (0x000D<<1)
#define DECOD_COM_STILL_P               (0x000E<<1)
#define DECOD_COM_SKIP_PICT             (0x000F<<1)
#define DECOD_COM_PAL_4M                0x0000  /* PAL 4 Meg */
#define DECOD_COM_PAL                   0x8000
#define DECOD_COMD_CHAIN                0x00000001
#define DECOD_COMD_STAT_PENDING         0x00000001
#define DECOD_TIMEOUT                   10
#define VID_WAIT_CMD_TIME		10


typedef struct tagVideoCmd
{
    UINT32	uCmd;		    //command
    INT		uNum;		    //parameters number
    UINT32 	uPara[4];	    //at most four parameters
    INT		uRetry;	            //retry number
    UINT	uChained;	    //chain flag
}VIDEOCMD;

void   vid_atom_set_cmd(VIDEOCMD *pCmd);
INT    vid_atom_exec_cmd(VIDEOCMD *pCmd);
INT    vid_atom_wait_cmd_done(UINT uRetry);

/*-----------------------------------------------------------
| MPEG video decoder reset
+------------------------------------------------------------*/
//INT     vid_atom_reset_ratebuf();      //reset rate buffer
void    vid_atom_reset_vdec();	   //soft reset video decoder
void    vid_atom_set_fb_adr(int mode);

/*-----------------------------------------------------------
| Microcode load and verify
+------------------------------------------------------------*/
INT    vid_atom_load_microcode(USHORT *pCode, INT nCount);
INT    vid_atom_verify_microcode(USHORT *pCode, INT nCount);


INT    vid_atom_config_modes(VDEC_CON *pVdec);
void   vid_atom_copy_at_mem(AT_MEM *pDes, AT_MEM *pSrc);
INT    vid_atom_ff_wait();

ULONG  vid_atom_make_memseg(MEM_SEG *pHigh, MEM_SEG *pLow);
#endif
