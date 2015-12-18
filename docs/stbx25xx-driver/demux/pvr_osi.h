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
|       COPYRIGHT   I B M   CORPORATION 1998
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
|   Author    :  Ian Govett
|   Component :  xp
|   File      :  pvr_osi.h
|   Purpose   :  header file for PVR os independent function
|   Changes   :
|
|   Date       By   Comments
|   ---------  ---  --------------------------------------------------------
|   10-Oct-01  LGH  Created
+----------------------------------------------------------------------------*/
#ifndef PVR_OSI
#define PVR_OSI

#define PVR_DMA_COMPLETE 0x1

typedef enum pvr_playback_mode_t
{
    PLAYBACK_WORD = 0,
    PLAYBACK_LINE
} PVR_PLAYBACK_MODE;

typedef struct pvr_status_t
{
    int BufEmpty;
    int BufFull;
} PVR_STATUS;

int pvr_osi_playback_deconfig(void);
int pvr_osi_playback_config(void);
int pvr_osi_playback_start(PVR_PLAYBACK_MODE mode);
int pvr_osi_playback_stop(void);
int pvr_osi_set_dma(ULONG addr,USHORT uwCount);
int pvr_osi_get_status(PVR_STATUS *pPvrStatus);
void pvr_osi_flush();
void pvr_osi_unflush(void);

void pvr_osi_set_vid(ULONG reg1);

#endif
