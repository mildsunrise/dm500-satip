/*----------------------------------------------------------------------------+
|   This source code has been made available to you by IBM on an AS-IS
|   basis.  Anyone receiving this source is licensed under IBM
|   copyrights to use it in any way he or she deems fit, including
|   copying it, modifying it, compiling it, and redistributing it either
|   with or without modifications.  No license under IBM patents or
|   patent applications is to be implied by the copyright license.
|
|   Any user of this software should understand that IBM cannot provide
|   technical support for this software and will not be responsible for
|   any consequences resulting from the use of this software.
|
|   Any person who transfers this source code or any derivative work
|   must include the IBM copyright notice, this paragraph, and the
|   preceding two paragraphs in the transferred software.
|
|       STB025XX VXWORKS EVALUATION KIT SOFTWARE
|       IBM CONFIDENTIAL
|       (C) COPYRIGHT IBM CORPORATION 2001
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| Author:    David Judkovics
| Component: teletext
| File:      mbuf.c
| Purpose:
| Changes:
| Date:     Comment:
| -----     --------
| 12-Apr-01 Created
---------------------------------------------------------------------------*/
#include <linux/config.h>
#include <linux/version.h>
#ifdef MODVERSIONS
#include <linux/modversions.h>
#endif
#define  __NO_VERSION__
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <stdio.h>
#include "os/os-generic.h"
#include "ttx_mbuf.h"

/*----------------------------------------------------------------------------+
|  ttx_m_deq
+----------------------------------------------------------------------------*/
Mbuf ttx_m_deq(Mbuf* queue)
{
    Mbuf ret_q;

    if (queue == NULL)
        return NULL;

    if ((*queue) == NULL)
        return NULL;

    ret_q = *queue;

    *queue = ret_q->m_qnext;

    ret_q->m_qnext = NULL;

    return ret_q;
}

/*----------------------------------------------------------------------------+
|  ttx_m_free_p
+----------------------------------------------------------------------------*/
Mbuf ttx_m_free_p(Mbuf mb)
{
    Mbuf next_q;
    Mbuf next_p;

    if (mb == NULL)
        return NULL;

    next_q = mb->m_qnext;

    next_p = mb;
    while (next_p != NULL) {
        Mbuf next;

        next = next_p->m_pnext;
        FREE(next_p);
        next_p = next;
    }

    return next_q;
}

/*----------------------------------------------------------------------------+
|  ttx_m_free_q
+----------------------------------------------------------------------------*/
void ttx_m_free_q(Mbuf* queue)
{
    Mbuf next_q;

    if (queue == NULL)
        return;

    if ((*queue) == NULL)
        return;

    next_q = (*queue);
    while (next_q != NULL) {
        next_q = ttx_m_free_p(next_q);
    }
    *queue = NULL;

    return;
}

/*----------------------------------------------------------------------------+
|  ttx_m_free
+----------------------------------------------------------------------------*/
Mbuf ttx_m_free(Mbuf mb)
{
    Mbuf next;

    if (mb == NULL)
        return NULL;

    next = mb->m_pnext;

    FREE(mb);

    return next;
}

/*----------------------------------------------------------------------------+
|  ttx_m_cat
+----------------------------------------------------------------------------*/
Mbuf ttx_m_cat(Mbuf mb, Mbuf nb)
{
    Mbuf next;

    if (mb == NULL)
        return nb;

    next = mb;
    while (next->m_pnext != NULL) {
        next = next->m_pnext;
    }
    next->m_pnext = nb;

    return mb;
}

/*----------------------------------------------------------------------------+
|  ttx_m_enq
+----------------------------------------------------------------------------*/
void ttx_m_enq(Mbuf* q, Mbuf mb)
{
    Mbuf next;

    if (q == NULL)
        return;

    if ((*q) == NULL) {
        *q = mb;
        return;
    }

    next = *q;
    while (next->m_qnext != NULL) {
        next = next->m_qnext;
    }
    next->m_qnext = mb;

    return;
}

/*----------------------------------------------------------------------------+
|  ttx_m_getn
+----------------------------------------------------------------------------*/
Mbuf ttx_m_getn(unsigned short size, int* status)
{
    Mbuf mb;

    mb = (Mbuf)MALLOC(sizeof(mbuf)+size);
    if (mb != NULL) {
        mb->m_pnext = NULL;
        mb->m_qnext = NULL;
        mb->m_alloc = sizeof(mbuf) + size;
        mb->m_size = size;
        mb->m_flags = 0x00;
        mb->m_type = MB_DATA;
        mb->m_offset = sizeof(mbuf);
        *status = 0;
    } else {
        *status = -1;
    }
    return mb;
}
