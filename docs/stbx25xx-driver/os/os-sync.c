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
#include <asm/semaphore.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <asm/signal.h>

#include <os/os-generic.h>
#include <os/os-sync.h>
#include <os/drv_debug.h>


#define SIG_SEMPHORE_TOUT    60

SEMAPHORE_T os_create_semaphore(UINT uInitialValue)
{
    SEMAPHORE_T sema;

    if((sema = (SEMAPHORE_T)MALLOC(sizeof(struct semaphore))) == NULL)
    {
        PDEBUG("SYNC: semaphore alloc mem error\n");
        return NULL;
    }
    //initialize mutex
    sema_init((struct semaphore*)sema, uInitialValue);
    return sema;
}

void os_delete_semaphore(SEMAPHORE_T semaphore)
{
    if(semaphore)
        FREE(semaphore);
}

static void os_sync_timer(ULONG data)
{
    PDEBUG("Timer get\n");
    send_sig(SIG_SEMPHORE_TOUT, (struct task_struct *)data, 1);     
}

INT os_wait_semaphore(SEMAPHORE_T semaphore, INT nTimeout)
{
    if(nTimeout < 0)
    {    
        //Timeout does not implemented here
        return down_interruptible((struct semaphore*)semaphore);
    }

    if (nTimeout == 0)
    {
        return down_trylock(semaphore);
    }
    else    // timeout > 0
    {
        int ret;
        struct timer_list timer;
        init_timer (&timer);
        timer.expires = jiffies + (nTimeout*HZ + 999)/1000;
        timer.function = os_sync_timer;
        timer.data = (ULONG)current;
    
        sigdelset(&current->blocked, SIG_SEMPHORE_TOUT);
        add_timer (&timer);
        ret = down_interruptible ((struct semaphore*)semaphore);
        PDEBUG("DOWN return %d\n", ret);
        if (!del_timer (&timer))
        {
             sigset_t sset;
             siginfo_t info;
             int sig;
             memset(&sset, 0xff, sizeof(sset));
             sigdelset(&sset, SIG_SEMPHORE_TOUT);
             spin_lock_irq(&current->sigmask_lock);
             sig = dequeue_signal(&sset, &info);
             spin_unlock_irq(&current->sigmask_lock);
             if(SIG_SEMPHORE_TOUT != sig)    // ? we don't get that !
                PDEBUG("Timeout unget\n");
        }
        sigaddset(&current->blocked, SIG_SEMPHORE_TOUT);
        return ret;
    }
}

INT os_try_wait_semaphore(SEMAPHORE_T semaphore, INT nTimeout)
{
    //Timeout does not implemented here
    return down_trylock(semaphore);
}

void os_post_semaphore(SEMAPHORE_T semaphore)
{
    up((struct semaphore*)semaphore);
}


MUTEX_T os_create_mutex (void)
{
    MUTEX_T mutex;

    if((mutex = (MUTEX_T)MALLOC(sizeof(struct semaphore))) == NULL)
    {
        PDEBUG("SYNC: mutex alloc mem error\n");
        return NULL;
    }
    //initialize mutex
    sema_init((struct semaphore*)mutex, 1);
    return mutex;
}

void os_delete_mutex(MUTEX_T mutex)
{
    if(mutex)
        FREE(mutex);
}

INT os_get_mutex(MUTEX_T mutex)
{
    return down_interruptible((struct semaphore*)mutex);
}

INT os_try_get_mutex(MUTEX_T mutex)
{
    return down_trylock((struct semaphore*)mutex);
}

void os_release_mutex(MUTEX_T mutex)
{
    up((struct semaphore*)mutex);
}

UINT32 os_enter_critical_section()
{
    unsigned long cpu_flags;
    save_flags(cpu_flags);
    cli();
    return cpu_flags;
}

void os_leave_critical_section(UINT32 flags)
{
    restore_flags(flags);
}


void os_sleep(ULONG ticks)
{
    set_current_state(TASK_INTERRUPTIBLE);
    schedule_timeout(ticks);
}

void os_delay(ULONG usecs)
{
    udelay(usecs);
}

void os_cpu_clock_delay(ULONG clocks)
{
    volatile ULONG i;
    for(i = 0; i < clocks; i++)
    {
        ;
    }
}


