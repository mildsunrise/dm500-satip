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
|       COPYRIGHT   I B M   CORPORATION 1997, 1999, 2003
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| File:   aud_osi.c
| Purpose: audio driver osi layer PALLAS
| Changes:
| Date:     Comment:
| -----     --------
| 15-Oct-01 create
| 17-Jun-03 Added ioremap/iounmap for audio status area in temp region
| 17-Jun-03 Added call to aud_atom to get current audio status
| 20-Jun-03 Remove mute side effects from stop/start
+----------------------------------------------------------------------------*/
#include <os/os-interrupt.h>
#include <os/drv_debug.h>
#include "aud_osi.h"

ADEC    _adec;

ADEC_CON _aud_conf = {
    MPEG_A_SEGMENT1,
    MPEG_A_SEGMENT2,
    MPEG_A_SEGMENT3,
    MPEG_A_SEGMENT3_SIZE,
    MPEG_A_RB_OFFSET
};

INT  aud_osi_init(audMode_t mode)
{   
    if(_adec.uOpenFlag == 1)
        return 0;

    _aud_conf.mode = mode;
    PDEBUG("\nstart config audio decoder\n");
    //configurate audio decoder
    if(aud_atom_init(&_aud_conf) != 0)
        return -1;

    //install interrupt handler
    if(os_install_irq(IRQ_AUD, 
                      IRQ_LEVEL_TRIG | IRQ_POSITIVE_TRIG,
					  aud_atom_irq_handler,
					  (void*)&_adec) != 0)
    {
        PDEBUG("install interrupt handler error\n");
        return -1;
    }
    os_disable_irq(IRQ_AUD);
    //add interrupt task
    if(os_add_irq_task(IRQ_AUD,
                       aud_osi_task,
                       sizeof(TASK_MSG),
                       32) != 0)
    {
        PDEBUG("install task error\n");
        return -1;
    }

    aud_atom_init_irq_mask();
    os_enable_irq(IRQ_AUD);

    PDEBUG("install irq handler and task OK\n");

    _adec.src = AUD_SOURCE_NO;
    _adec.state = AUD_STOPPED;
    _adec.mode = mode;
    _adec.stream = AUD_STREAM_TYPE_UNKNOWN;
    _adec.channel = AUD_CHANNEL_STEREO;
    _adec.uSync = 0;        //NO SYNC
    _adec.uOpenFlag = 1;
    _adec.ulStatusArea = ioremap(MPEG_A_STATUS_START, MPEG_A_STATUS_SIZE);
    return 0;
}

void aud_osi_close()
{
    if (_adec.uOpenFlag == 0)
        return;

    PDEBUG("aud osi close\n");

    /*aud_osi_stop();
    aud_atom_close();

    //delete clip device
    if(_adec.clipdev)
    {
        clipdev_delete(_adec.clipdev);
        _adec.clipdev = NULL;
    }*/
    if(_adec.src == AUD_SOURCE_DEMUX)
        aud_osi_close_tv();
    else if(_adec.src == AUD_SOURCE_MEMORY)
        aud_osi_close_clip();

    os_disable_irq(IRQ_AUD);
    os_delete_irq_task(IRQ_AUD);    //remove irq task
    os_uninstall_irq(IRQ_AUD);      //remove irq handler

    iounmap(_adec.ulStatusArea);
    _adec.uOpenFlag = 0;
}

//INT     aud_osi_set_source(audSource_t src);

INT     aud_osi_get_status(audioStatus *pStatus)
{
  pStatus->stream_decode_type = _adec.mode;
  return aud_atom_get_status(_adec.ulStatusArea, pStatus);
}

INT aud_osi_get_openflag()
{
    return _adec.uOpenFlag;
}

INT  aud_osi_play()
{
    if(_adec.src == AUD_SOURCE_NO)
    {
        PDEBUG("WARNING: set source first\n");
        return -1;
    }
    if(_adec.stream == AUD_STREAM_TYPE_UNKNOWN)
    {
        PDEBUG("WARNING: set stream type first\n");
        return -1;
    }
    if(_adec.state == AUD_PLAYING)
        return 0;
    //start playing
    aud_atom_play();
    //enable sync
    //aud_osi_enable_sync();
    //    aud_osi_unmute();

    _adec.state = AUD_PLAYING;
    return 0;
}

INT aud_osi_stop()
{
    if(_adec.state == AUD_STOPPED)
        return 0;
    //disable sync
    //    aud_osi_disable_sync();
    //mute
    //    aud_osi_mute();
    //stop
    aud_atom_stop();

    _adec.state = AUD_STOPPED;
    return 0;
}

INT aud_osi_mute()
{
    aud_atom_mute();
    return 0;
}

INT aud_osi_unmute()
{
    aud_atom_unmute();
    return 0;
}

INT  aud_osi_set_vol(AUDVOL *vol)
{
    aud_atom_set_vol(vol);
    return 0;
}
INT  aud_osi_get_vol(AUDVOL *vol)
{
    aud_atom_get_vol(vol);
    return 0;
}

INT  aud_osi_set_stream_type(audStream_t stream)
{
    if(_adec.stream == stream)
        return 0;
    aud_atom_set_stream_type(stream);
    _adec.stream = stream;
    return 0;
}

INT  aud_osi_set_channel(audChannel_t channel)
{
    if(_adec.channel == channel)
        return 0;
    aud_atom_set_channel(channel);
    _adec.channel = channel;
    return 0;
}

//INT     aud_osi_get_microcode_ver(ULONG *pVer);

//TV group
INT     aud_osi_init_tv()
{
    if(_adec.uOpenFlag == 0)
    {
        PDEBUG("WARNING: audio decoder not initialized\n");
        return -1;
    }
    if(_adec.src != AUD_SOURCE_NO)
    {
        PDEBUG("WARNING: audio decoder is in use, src = %d\n", _adec.src);
        return -1;
    }
    
    aud_atom_init_tv();
    _adec.src = AUD_SOURCE_DEMUX;
    return 0;
}

void  aud_osi_close_tv()
{
    if(_adec.state != AUD_STOPPED)
        aud_osi_stop();
    _adec.src = AUD_SOURCE_NO;
}




/*INT aud_atom_start_clip()
{
    if(_adec.src != AUD_SOURCE_MEMORY)
    {
        PDEBUG("init memory mode first\n");
        return -1;
    }
    
    if( aud_osi_write_clip(&(_adec.queue)) == 0)
    {
        _adec.uClipStart = 1;
        return 0;
    }
    else
        return -1;
}


INT aud_atom_stop_clip()
{
    _adec.uClipStart = 0;
    return 0;
}

INT aud_osi_write_clip(QUEUE_T *pQueue)
{
    PDEBUG("not finished yet\n");
    return 0;
}*/


//Task
void aud_osi_task(QUEUE_T *pQueue)
{
    TASK_MSG msg;
    CLIPDEV_T cdev;

    PDEBUG("aud task\n");
    while( os_get_queue_status(pQueue) > 0)
    {
        //get first message
        if( os_dequeue(pQueue, &msg) != 0 )
        {
            PDEBUG("no message\n");
            return;
        }
        if( msg.uMsgType & AUD_MSG_BLOCK_READ)
        {
            PDEBUG("main block read msg\n");
            cdev = aud_osi_get_clipdev();
            clipdev_clipinfo_done(cdev);
            //write next clip
            clipdev_hw_write(cdev);
        }
        if( msg.uMsgType & AUD_MSG_BLOCK_READ2)
        {
            PDEBUG("mixer block read msg\n");
            cdev = aud_osi_get_mixerdev();
            clipdev_clipinfo_done(cdev);
            clipdev_start(cdev);
        }
    }
}

INT  aud_osi_set_pts(STC_T *pData)
{
    return aud_atom_set_pts(pData);
}

INT aud_osi_get_pts(STC_T *pData)
{
    return aud_atom_get_pts(pData);
}


INT aud_osi_set_stc(STC_T *pData)
{
    return aud_atom_set_stc(pData);
}

INT aud_osi_get_stc(STC_T *pData)
{
    return aud_atom_get_stc(pData);
}

INT aud_osi_enable_sync(UINT uAudMaster)
{
    aud_atom_sync_master(uAudMaster);
    _adec.uSync = 1;
    return 0;
}

INT aud_osi_disable_sync()
{
    if(_adec.uSync == 0)
        return 0;
    else
        aud_atom_sync_off();
    _adec.uSync = 0;
    return 0;
}

/*CLIPDEV_T aud_osi_get_clipdev()
{
    if(_adec.src != AUD_SOURCE_MEMORY)
    {
        PDEBUG("init memory mode first\n");
       return NULL;
    }
    return _adec.clipdev;
}*/


    
