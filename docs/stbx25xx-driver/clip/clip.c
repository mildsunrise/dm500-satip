#include <os/drv_debug.h>
#include <os/os-generic.h>
#include "clip/clip.h"


CLIPDEV_T clipdev_create(ULONG uBufSegLen, 
                         UINT uBufSegNum, 
                         INT (*write_clip)(CLIPINFO* pClipInfo),
                         INT (*buf_ready)(void))
{
    struct tagClipDev* pClipDev;
    int i;
    CLIPINFO info;

    PDEBUG("clip device create\n");

    if(write_clip == NULL || buf_ready == NULL)
    {
        PDEBUG("no necessary callback function\n");
        return NULL;
    }

    if((pClipDev = (CLIPDEV_T)MALLOC(sizeof(struct tagClipDev))) == NULL)
    {
        PDEBUG("malloc clipdev error\n");
        return NULL;
    }

    if((pClipDev->sema = os_create_semaphore(uBufSegNum)) == NULL)
    {
        PDEBUG("create mutex error\n");
        goto err3;
    }

    PDEBUG("create mutex OK\n");

    if(os_create_queue(&(pClipDev->bufq), NULL, uBufSegNum + 1, sizeof(CLIPINFO)) != 0)
    {
        PDEBUG("create buffer queue error\n");
        goto err2;
    }

    if(os_create_queue(&(pClipDev->clipq), NULL, uBufSegNum + 1, sizeof(CLIPINFO)) != 0)
    {
        PDEBUG("create buffer queue error\n");
        goto err1;
    }

    PDEBUG("create queues OK\n");

    pClipDev->uQueueSize = uBufSegNum;
    pClipDev->uBufLen = uBufSegLen;
    pClipDev->uStatus = CLIP_STOPPED;          //stopped
    pClipDev->nOff = 0;
    pClipDev->nLeft = uBufSegNum;
    pClipDev->write_clip_to_dev = write_clip;
    pClipDev->is_buf_ready = buf_ready;

    PDEBUG("init queue\n");

    memset(&info, 0, sizeof(CLIPINFO));

    //initialize buffer queue to default value
    for(i = 0; i < uBufSegNum; i++)
    {
        info.ulBufAdrOff = i * uBufSegLen;
        info.ulBufLen = uBufSegLen;
        if( os_enqueue(&(pClipDev->bufq) , (void*)&info) != 0)
        {
            PDEBUG("enqueue error, num = %d\n", i);
            goto err0;
        }
    }
    PDEBUG("init buffer queue OK\n");
    return pClipDev;
    
err0: os_delete_queue(&(pClipDev->clipq));
      //delete buffer queue
err1: os_delete_queue(&(pClipDev->bufq));
      //delete semaphore
err2: if(pClipDev->sema)
          os_delete_semaphore(pClipDev->sema);
      //delete clip device itself
err3: if(pClipDev)
          FREE(pClipDev);
      return NULL;
}


INT clipdev_delete(CLIPDEV_T clipdev)
{
    os_delete_queue(&(clipdev->clipq));
      //delete buffer queue
    os_delete_queue(&(clipdev->bufq));
      //delete semaphore
    if(clipdev->sema)
          os_delete_semaphore(clipdev->sema);
      //delete clip device itself
    if(clipdev)
          FREE(clipdev);
    return 0;
}


INT clipdev_get_buf_wait(CLIPDEV_T clipdev, CLIPINFO* pClipInfo)
{
    PDEBUG("wait left = %d\n", clipdev->nLeft);
    if( os_wait_semaphore(clipdev->sema, -1) < 0)
    {
        PDEBUG("wait failed\n");
        return -1;
    }
    PDEBUG("Wait ok\n");
    //get buffer information from buffer queue
    if( os_dequeue(&(clipdev->bufq), (void*)pClipInfo) != 0)
    {
        os_post_semaphore(clipdev->sema);
        return -1;
    }
    pClipInfo->uClipAdrOff = pClipInfo->ulBufAdrOff;
    pClipInfo->uClipLen = 0;
    clipdev->nLeft--;
    return 0;
}


INT clipdev_get_buf_nowait(CLIPDEV_T clipdev, CLIPINFO* pClipInfo)
{
    ULONG   flags;
    PDEBUG("no wait left = %d\n", clipdev->nLeft);

    flags = os_enter_critical_section();
    if(clipdev->nLeft <= 0)
    {
        os_leave_critical_section(flags);
        PDEBUG("no more left\n");
        return -1;
    }
    //it should not be blocked
    if(clipdev_get_buf_wait(clipdev, pClipInfo) != 0)
    {
        os_leave_critical_section(flags);
        return -1;
    }
    os_leave_critical_section(flags);
    return 0;
}


INT clipdev_get_buf_status(CLIPDEV_T clip)
{
    return clip->nLeft;
}


UINT clipdev_get_clip_status(CLIPDEV_T clip)
{
    return clip->uStatus;
}

INT clipdev_write_ex(CLIPDEV_T clipdev, CLIPINFO *pClipInfo)
{
    if(clipdev_write(clipdev, pClipInfo) != 0)
    {
        PDEBUG("write info to queue error\n");
        return -1;
    }
    if(clipdev_start(clipdev) != 0)
    {
        PDEBUG("clip start error\n");
        return -1;
    }
    return 0;
}

INT clipdev_write(CLIPDEV_T clipdev, CLIPINFO *pClipInfo)
{
    if(pClipInfo->uClipLen > pClipInfo->ulBufLen)
    {
        PDEBUG("clip info error\n");
        return -1;
    }       
    if( os_enqueue(&(clipdev->clipq), (void*)pClipInfo) != 0)
    {
        PDEBUG("write clip info error\n");
        return -1;
    }
    return 0;
}


INT clipdev_hw_write(CLIPDEV_T clipdev)
{
    ULONG flags;

    CLIPINFO *pInfo;
    if(clipdev->uStatus != CLIP_PLAYING)
        return -1;

    if( os_get_queue_status(&(clipdev->clipq)) == 0 )
        return -1;                  //no command

    if(clipdev->is_buf_ready == NULL || clipdev->write_clip_to_dev == NULL)
        return -1;

    while(os_get_queue_status(&(clipdev->clipq)) != 0)
    {
        if(clipdev->is_buf_ready())
        {
            flags = os_enter_critical_section();
            //read next waiting CLIPINFO 
            if((pInfo = (CLIPINFO*) os_dequeue_multiple_start 
                        (&(clipdev->clipq), clipdev->nOff)) == NULL)
            {
                PDEBUG("no more clip info\n");
                os_leave_critical_section(flags);
                return 0;
            }
            clipdev->nOff++;
            os_leave_critical_section(flags);
            PDEBUG("clip info queue num = %d, offset = %d\n", os_get_queue_status(&(clipdev->clipq)), clipdev->nOff);   
            //write command to video decoder
            clipdev->write_clip_to_dev(pInfo);
        }
        else
            break;
    }
    return 0;
}


INT  clipdev_start(CLIPDEV_T clipdev)
{
    clipdev->uStatus = CLIP_PLAYING;
    if( clipdev_hw_write(clipdev) != 0)
    {
        clipdev->uStatus = CLIP_STOPPED;
        return -1;
    }
    return 0;
}


INT clipdev_stop(CLIPDEV_T clipdev)
{
    clipdev->uStatus = CLIP_STOPPED;
    while(os_get_queue_status(&(clipdev->clipq)) > 0)
    {
        clipdev_clipinfo_done(clipdev);
    }
    return 0;
}

INT clipdev_pause(CLIPDEV_T clipdev)
{
    clipdev->uStatus = CLIP_PAUSED;
    return 0;
}


void clipdev_clipinfo_done(CLIPDEV_T clipdev)
{
    CLIPINFO info;
    UINT32   uFlags;

    //one block of clip finished
    if ( os_get_queue_status(&(clipdev->clipq)) == 0)
        return;
    else
    {
        //remove the current CLIPINFO from the clipinfo queue
        if(os_dequeue(&(clipdev->clipq), (void*)&info) != 0)
            return;
        if(clipdev->nOff > 0)
            clipdev->nOff--;
         
        //return the empty buffer to buffer queue
        os_enqueue(&(clipdev->bufq), &info);
        
        PDEBUG("remove clipinfo and enqueue OK\n");
        PDEBUG("clip info queue num = %d, offset = %d\n", os_get_queue_status(&(clipdev->clipq)), clipdev->nOff);   
        uFlags = os_enter_critical_section();
        clipdev->nLeft++;
        if(clipdev->nLeft > clipdev->uQueueSize)
            clipdev->nLeft = clipdev->uQueueSize;
        os_post_semaphore(clipdev->sema);
        os_leave_critical_section(uFlags);
    }
    return ;
}        

void clipdev_dump(CLIPDEV_T clipdev)
{
    PDEBUG("queue size = %d\n", clipdev->uQueueSize);
    PDEBUG("buffer length = %ld\n", clipdev->uBufLen);
    PDEBUG("clip queue offset = %d\n", clipdev->nOff);
    PDEBUG("status = %d\n", clipdev->uStatus);
    PDEBUG("empty left = %d\n", clipdev->nLeft);
}



