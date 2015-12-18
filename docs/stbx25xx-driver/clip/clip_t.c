#include <linux/kernel.h>       /* We're doing kernel work */
#include <linux/module.h>       /* Specifically, a module */
#include <linux/version.h>
#include "clip.h"


#ifdef MODULE
MODULE_PARM(wait, "i");
MODULE_PARM_DESC(wait,
                 "wait: 0-nowait, 1-wait");

MODULE_PARM(num, "i");
#endif

int wait = 0;
int num = 32;

CLIPINFO  info[32];
CLIPDEV_T clipdev;

int clipt_ready()
{
    printk("test ready\n");
    return 1;
}

int clipt_write(CLIPINFO *info)
{
    printk("test write to hw\n");
    printk("start address = %ld\n", info->ulBufAdrOff);
    printk("len = %ld\n", info->ulBufLen);
    return 0;
}

int init_module()
{
    int i;

    clipdev = clipdev_create(200, num, 
                            clipt_write,clipt_ready);
    if(clipdev == NULL)
    {
        printk("clip device init error\n");
	return -1;
    }

    for( i =0; i < num; i++)
    {
        if(wait == 0)
        {
            if(clipdev_get_buf_nowait(clipdev, &info[i]) != 0)
            {
                printk("dequeue error\n");
            }
        }
        else if(wait == 1)
        {
            if(clipdev_get_buf_wait(clipdev, &info[i]) != 0)
            {
                printk("dequeue error\n");
            }
        }
        else
            return -1;

        printk("[%d], start = %ld, len = %ld\n", 
            i, 
            info[i].ulBufAdrOff,
            info[i].ulBufLen);
    }
    clipdev_dump(clipdev);
    return 0;
}

/* Cleanup - unregister the appropriate file from /proc */
void cleanup_module()
{
    int i;
    printk("remove bufman\n");
    for(i = 0; i < num; i++)
    {        
        clipdev_clipinfo_done(clipdev);
    }
    if(clipdev)
        clipdev_delete(clipdev);
}

