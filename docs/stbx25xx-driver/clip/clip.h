#ifndef PALLAS_CLIP_H
#define PALLAS_CLIP_H

#include <os/os-types.h>
#include <os/helper-queue.h>
#include <os/os-sync.h>
#include "clipinfo.h"

#define CLIP_STOPPED    0
#define CLIP_PLAYING    1
#define CLIP_PAUSED     2



struct tagClipDev
{
    QUEUE_T	        bufq;       //buffer queue
    QUEUE_T         clipq;      //clip info queue
    UINT            uQueueSize; //queue size
    ULONG           uBufLen;    //the length of each segment of the buffer
    INT             nOff;       //offset between read pointer and remove pointer
    UINT            uStatus;    
    SEMAPHORE_T     sema;
    INT             nLeft;      //empty slot left
    INT             (*write_clip_to_dev)(CLIPINFO* pClipInfo);
    INT             (*is_buf_ready)(void);
};

typedef struct tagClipDev *CLIPDEV_T; 

CLIPDEV_T clipdev_create(ULONG uBufSegLen, 
                         UINT uBufSegNum, 
                         INT (*write_clip)(CLIPINFO* pClipInfo),
                         INT (*buf_ready)(void));
INT       clipdev_delete(CLIPDEV_T );
INT       clipdev_get_buf_wait(CLIPDEV_T , CLIPINFO* pClipInfo);
INT       clipdev_get_buf_nowait(CLIPDEV_T , CLIPINFO* pClipInfo);
INT       clipdev_get_buf_status(CLIPDEV_T );
UINT      clipdev_get_clip_status(CLIPDEV_T );
INT       clipdev_write(CLIPDEV_T , CLIPINFO *pClipInfo);
INT       clipdev_write_ex(CLIPDEV_T , CLIPINFO *pClipInfo);
INT       clipdev_start(CLIPDEV_T );
INT       clipdev_stop(CLIPDEV_T );
INT       clipdev_pause(CLIPDEV_T );
void      clipdev_clipinfo_done(CLIPDEV_T );
void      clipdev_dump(CLIPDEV_T );
#endif