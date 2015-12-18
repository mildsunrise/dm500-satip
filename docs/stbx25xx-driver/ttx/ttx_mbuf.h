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
|       IBM CONFIDENTIAL
|       STB025XX VXWORKS EVALUATION KIT SOFTWARE
|       (C) COPYRIGHT IBM CORPORATION 2003
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| Author:    David Judkovics
| Component: teletext
| File:      ttx_mbuf.h
| Purpose:
| Changes:
| Date:     Comment:
| -----     --------
| 12-Apr-01 Created
---------------------------------------------------------------------------*/
#ifndef _MBUF_H
#define _MBUF_H

/* Maximum mbuf allocation */
#define MBMAX 32768

/* Mbuf header structure */
typedef struct mbuf {
    struct mbuf     *m_pnext;           /* mbuf packet data chain */
    struct mbuf     *m_qnext;           /* mbuf queue chain */
    unsigned short         m_alloc;            /* size of mbuf allocation */
    unsigned short         m_size;             /* size of data */
    unsigned char          m_flags;            /* mbuf flags */
        /* Additional flags for normal mbufs */
        #define SPF_NOFREE      0x80    /* Don't free mbuf after xmitting   */
        #define SPF_DONE        0x40    /* Mbuf has been xmitted            */
        #define SPF_RXERR       0x08    /* Receive mbuf marked as errored   */
        /*BSD 4.4 m_flag definitions */
        #define M_EXT           0x01  /* has associated external storage */
        #define M_PKTHDR        0x02  /* start of record */
        #define M_EOR           0x04  /* end of record */
        #define M_BCAST         0x10  /* send/received as link-level broadcast */
        #define M_MCAST         0x20  /* send/received as link-level multicast */
    unsigned char          m_type;             /* mbuf type */
        #define MB_NONE         0
        #define MB_DATA         1
        #define MB_ADDR         2
        #define MB_HEADER       3
        #define MB_RSV          0x80    /* Reserved type field */
        #define MB_NEWTYPE      0xff    /* Flag for checking if the mbuf is
                                           a new structure type <future impl> */
    /*BSD 4.4 m_type definitions */
        #define MBT_FREE        0       /* should be on free list */
        #define MBT_DATA        1       /* dynamic (data) allocation */
        #define MBT_SONAME      2       /* socket name */
        #define MBT_HEADER      3       /* packet header */
        #define MBT_PCB         4       /* protocol control block */
        #define MBT_RTABLE      5       /* routing tables */
        #define MBT_HTABLE      6       /* IMP host tables */
        #define MBT_ATABLE      7       /* address resolution tables */
        #define MBT_SOCKET      8       /* socket structure */
        #define MBT_SOOPTS      10      /* socket options */
        #define MBT_FTABLE      11      /* fragment reassembly header */
        #define MBT_RIGHTS      12      /* access rights */
        #define MBT_IFADDR      13      /* interface address */
        #define MBT_CONTROL     14      /* extra-data protocol message */
        #define MBT_OOBDATA     15      /* expedited data  */
        #define MBT_SENDTO    0x7F      /* sendto  */
    unsigned short         m_offset;           /* offset to data from start of mbuf*/
} mbuf, *Mbuf;
#define MBHDR_SZ                sizeof(struct mbuf)
#define DATA_PTR(mb)            ((unsigned char*)((char*)(mb)+(mb)->m_offset))

extern unsigned int      ttx_m_adj(Mbuf mb, int count);
extern Mbuf         ttx_m_cat(Mbuf mb, Mbuf nb);
extern Mbuf         ttx_m_copy(Mbuf mb, unsigned int offset, unsigned short count, int* status);
extern Mbuf         ttx_m_deq(Mbuf* q);
extern void         ttx_m_enq(Mbuf* q, Mbuf mb);
extern Mbuf         ttx_m_flush(Mbuf mb);
extern Mbuf         ttx_m_free(Mbuf mb);
extern int          ttx_m_free_m(Mbuf mb);
extern Mbuf         ttx_m_free_p(Mbuf mb);
extern void         ttx_m_free_q(Mbuf* queue);
extern Mbuf         ttx_m_get(unsigned short size, int* status);
extern Mbuf         ttx_m_getn(unsigned short size, int* status);
extern unsigned int      ttx_m_len_p(Mbuf mb);
extern unsigned int      ttx_m_move(Mbuf mb, unsigned int offset, unsigned int count, char* buffer);
extern unsigned int      ttx_m_msize(Mbuf mb);
extern unsigned int      M_PAD(Mbuf mb, unsigned short count);
extern unsigned int      ttx_m_pad(Mbuf mb, unsigned short count);
extern void         M_PREPEND(Mbuf mbp, unsigned short count, unsigned short how);
extern char*        ttx_m_ptod(Mbuf mb);
extern int        ttx_m_pullup(Mbuf* mb, unsigned short count);
extern unsigned int      M_UNPAD(Mbuf mb, unsigned short count);
extern unsigned int      ttx_m_unpad(Mbuf mb, unsigned short count);
extern void         *mtod(Mbuf mb, void *type);

/* Macro routines to manipulate mbufs */
#define M_PAD(mb,count)     ((unsigned int)((mb)->m_offset += count))
#define M_UNPAD(mb,count)   ((unsigned int)((mb)->m_offset -= count))
#define mtod(mb,type)       ((type)((mb)->m_offset + (char *)(mb)))

/*
 * Compute the amount of space available
 * before the current start of data in an mbuf.
 */
#define M_LEADINGSPACE(mbp) ((unsigned int)((mbp)->m_offset - MBHDR_SZ))

/*
 * Arrange to prepend space of size count to mbuf mbp.
 * If there is not enough space, return the NULL mbuf pointer.
 */
#define M_PREPEND(mbp, count, how) { \
    if (M_LEADINGSPACE(mbp) >= (count)) { \
        (mbp)->m_offset -= (count); \
        (mbp)->m_size += (count); \
    } \
}

#endif  /* _MBUF_H */
