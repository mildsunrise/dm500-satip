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
| File:   aud_atom.c
| Purpose: audio driver atom layer PALLAS
| Changes:
| Date:         Comment:
| -----         --------
| 30-Jan-02		create                  									SL
+----------------------------------------------------------------------------*/
#include <os/os-io.h>
#include <os/drv_debug.h>
#include <os/os-sync.h>
#include <linux/delay.h>
#include "aud_atom.h"
#include "aud_atom_hw.h"

void aud_atom_sync_master(UINT uAudMaster)
{
    unsigned long reg;
    //enable audio synchronize
   /*-------------------------------------------------------------------------+
   | Disable SYNC
   +-------------------------------------------------------------------------*/
   MT_DCR(AUD_CTRL0, MF_DCR(AUD_CTRL0)& (~DECOD_AUD_CTRL0_ENABLE_SYNC));

   reg = MF_DCR(AUD_DSR) & (~DECOD_AUD_DSR_SYNC_MASTER);

   if(uAudMaster)     //audio master
   {
       MT_DCR(AUD_DSR, reg | DECOD_AUD_DSR_SYNC_MASTER);
   }
   else
   {
       MT_DCR(AUD_DSR, reg);
   }

    MT_DCR(AUD_CTRL0, MF_DCR(AUD_CTRL0) | DECOD_AUD_CTRL0_ENABLE_SYNC);
}


void aud_atom_sync_on()
{
    unsigned long reg;
    reg = MF_DCR(AUD_CTRL0);
    MT_DCR(AUD_CTRL0, reg | DECOD_AUD_CTRL0_ENABLE_SYNC);
}

INT aud_atom_sync_off()
{

    unsigned long reg;
    int           sync;

    reg = MF_DCR(AUD_CTRL0);
    sync = reg & DECOD_AUD_CTRL0_ENABLE_SYNC;
    MT_DCR(AUD_CTRL0, reg & (~DECOD_AUD_CTRL0_ENABLE_SYNC));
    return sync;
}

inline int aud_atom_get_sync()
{
    return MF_DCR(AUD_CTRL0) & DECOD_AUD_CTRL0_ENABLE_SYNC ?1:0;
}


inline void aud_atom_do_set_pts(STC_T *pData)
{
    MT_DCR(AUD_PTS, pData->bit0 | ((pData->bit32_1 <<1)&0x0000FFFE));
    MT_DCR(AUD_PTS, (pData->bit32_1>>15)&0x0000FFFF);
    MT_DCR(AUD_PTS, (pData->bit32_1>>31)&0x00000001);
}

void  aud_atom_do_get_pts(STC_T *pData)
{
    unsigned long tmp;
    tmp = MF_DCR(AUD_PTS)&0x0000FFFF;
    pData->bit0 = tmp&0x00000001;
    pData->bit32_1  = (tmp>>1);
    tmp = MF_DCR(AUD_PTS)&0x0000FFFF;
    pData->bit32_1 |= (tmp<<15);
    tmp = MF_DCR(AUD_PTS)&0x00000001;
    pData->bit32_1 |= (tmp<<31);
}

inline void  aud_atom_do_set_stc(STC_T *pData)
{
    MT_DCR(AUD_STC, pData->bit0 | ((pData->bit32_1 <<1)&0x0000FFFE));
    MT_DCR(AUD_STC, (pData->bit32_1>>15)&0x0000FFFF);
    MT_DCR(AUD_STC, (pData->bit32_1>>31)&0x00000001);
}

void  aud_atom_do_get_stc(STC_T *pData)
{
    unsigned long tmp;
    tmp = MF_DCR(AUD_STC)&0x0000FFFF;
    pData->bit0 = tmp&0x00000001;
    pData->bit32_1  = (tmp>>1);
    tmp = MF_DCR(AUD_STC)&0x0000FFFF;
    pData->bit32_1 |= (tmp<<15);
    tmp = MF_DCR(AUD_STC)&0x00000001;
    pData->bit32_1 |= (tmp<<31);
}


INT  aud_atom_set_pts(STC_T *pData)
{
    int num_retry;
    int sync;
    //STC_T dummy;

    //disable sync
    sync = aud_atom_sync_off();

    /*-------------------------------------------------------------------------+
    | Wait for audio code ready.
    +-------------------------------------------------------------------------*/
    num_retry=0;
    while(1) 
    {
        if(aud_atom_dsp_ready())
        {
            aud_atom_do_set_pts(pData);
            break;
        }
        //dummy read
        MF_DCR(AUD_PTS);
      
        num_retry++;
        if (num_retry >= AUD_SYNC_RW_TIMEOUT) 
        {
            if(sync)
                aud_atom_sync_on();
            return -1;
        }
        udelay(20);
        //sleep_on_time(20);
    }
    if(sync)
        aud_atom_sync_on();
    return(0);
}

INT aud_atom_get_pts(STC_T *pData)
{
    int num_retry;
    //STC_T dummy;
    int sync;

    //disable sync
    sync = aud_atom_sync_off();

    /*-------------------------------------------------------------------------+
    | Wait for audio code ready.
    +-------------------------------------------------------------------------*/
    num_retry=0;
    while(1) 
    {
        if(aud_atom_dsp_ready())
        {
            //dummy read
            MF_DCR(AUD_PTS);
            break;
        }
        //dummy read
        MF_DCR(AUD_PTS);
        
        num_retry++;
        if (num_retry >= AUD_SYNC_RW_TIMEOUT) 
        {
            if(sync)
                aud_atom_sync_on();
            return -1;
        }
        //sleep_on_time(20);
        udelay(20);
    }
    num_retry = 0;
    while(1)
    {
        if(!aud_atom_dsp_ready())
        {
            aud_atom_do_get_pts(pData);
            break;
        }
      
        num_retry++;
        if (num_retry >= AUD_SYNC_RW_TIMEOUT) 
        {
            if(sync)
                aud_atom_sync_on();
            return -1;
        }
        //sleep_on_time(20);
        udelay(20);
    }

    if(sync)
        aud_atom_sync_on();
    return(0);
}


INT aud_atom_set_stc(STC_T *pData)
{
    int num_retry;
    int sync;
    //STC_T dummy;

    //disable sync
    sync = aud_atom_sync_off();

    /*-------------------------------------------------------------------------+
    | Wait for audio code ready.
    +-------------------------------------------------------------------------*/
    num_retry=0;
    while(1) 
    {
        if(aud_atom_dsp_ready())
        {
            aud_atom_do_set_stc(pData);
            break;
        }
        //dummy read
        MF_DCR(AUD_STC);
      
        num_retry++;
        if (num_retry >= AUD_SYNC_RW_TIMEOUT) 
        {
            if(sync)
                aud_atom_sync_on();
            return -1;
        }
        //sleep_on_time(20);
        udelay(20);
    }
    if(sync)
        aud_atom_sync_on();
    return(0);

}

INT aud_atom_get_stc(STC_T *pData)
{
    int num_retry;
    int sync;
    //STC_T dummy;

    //disable sync
    sync = aud_atom_sync_off();

    /*-------------------------------------------------------------------------+
    | Wait for audio code ready.
    +-------------------------------------------------------------------------*/
    num_retry=0;
    while(1) 
    {
        if(aud_atom_dsp_ready())
        {
            //dummy read
            MF_DCR(AUD_STC);
            break;
        }
        //dummy read
        MF_DCR(AUD_STC);
      
        num_retry++;
        if (num_retry >= AUD_SYNC_RW_TIMEOUT) 
        {
            if(sync)
                aud_atom_sync_on();
            return -1;
        }
        //sleep_on_time(20);
        udelay(20);
    }
    num_retry = 0;
    while(1)
    {
        if(!aud_atom_dsp_ready())
        {
            aud_atom_do_get_stc(pData);
            break;
        }
      
        num_retry++;
        if (num_retry >= AUD_SYNC_RW_TIMEOUT) 
        {
            if(sync)
                aud_atom_sync_on();
            return -1;
        }
        //sleep_on_time(20);
        udelay(20);
    }

    if(sync)
        aud_atom_sync_on();
    return(0);
}
