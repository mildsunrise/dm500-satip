//vulcan/drv/gfx/gfx_atom.h
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
|       COPYRIGHT   I B M   CORPORATION 2002
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
//
//Comment: 
//  Linux device driver interrupt and DMA handling functions for graphics
//Revision Log:   
//  Oct/21/2002                          Created by BJC


#ifndef _GFX_DMA_H
#define _GFX_DMA_H

#include <os/os-types.h>
#include <os/os-io.h>
#include <gfx/gfx_inf_struct.h>

#include <asm/semaphore.h>
#include <asm/io.h>
#include <linux/iobuf.h>
#include <linux/mm.h>

/*---------*/
/* public: */
/*---------*/

/*
 * local data structure used to relay surface swapping parameters between gfx_inf_h_swap_surface()
 * and the OSD animation interrupt handler
 */
typedef struct __GFX_SWAP_SURFACE_T_STRUCTURE__
{
    struct semaphore sem;
    BYTE bReadyForSwap;
    int hOldSurface;
    int hNewSurface;
    GFX_VISUAL_DEVICE_ID_T hGraphDev;
    UINT missedInts;
} GFX_SWAP_SURFACE_T;


/* interrupt handlers */
void gfx_atom_osdanim_irq_handler(int irq, void* dev_id, struct pt_regs *regs);
void gfx_atom_dmablt_irq_handler(UINT uIrq, void *pData);

/* initialization */
int gfx_atom_init(void);
void gfx_atom_deinit(void);

/* helper routines */
int gfx_atom_dmablt_handle_request(GFX_DMABLT_PARM_T *pParm);
int gfx_atom_dmablt_wait_handle_request(void);
int gfx_atom_pmalloc_handle_request(GFX_PMALLOC_PARM_T *pParm);


/*---------*/
/* private:*/
/*---------*/

/* time measurement */
struct timeval t1, t2;
#define timeval_sub(end, begin) (long)(((end.tv_sec-begin.tv_sec-1)*1000000) \
        + (1000000-begin.tv_usec) + end.tv_usec)
#define mark_start() do_gettimeofday(&t1)
#define mark_end()   do_gettimeofday(&t2); \
                    printk(KERN_ALERT "%ldus\n", timeval_sub(t2, t1))


/* virtual --> physical address mapping */
typedef struct _iobuf_pos_t {
    struct kiobuf *iobuf;
    UINT offset;
    UINT pageno;    
} iobuf_pos_t;


inline void iobuf_pos_init(iobuf_pos_t *ip, struct kiobuf *iobuf);
inline UINT iobuf_pos_get(iobuf_pos_t *ip, UINT count, BYTE* *vaddr);
inline void iobuf_pos_skip(iobuf_pos_t *ip, UINT count);

int iobuf_lock(struct kiobuf **iobuf, int access, void *userbuf, int size);
void iobuf_unlock(struct kiobuf **iobuf);


/* DMA controller DCRs */
#define DMA0SA1 0xCB
#define DMA0DA1 0xCA
#define DMA0CT1 0xC9
#define DMA0CR1 0xC8

#define DMA0SA3 0xDB
#define DMA0DA3 0xDA
#define DMA0CT3 0xD9
#define DMA0CR3 0xD8

#define DMASR0  0xE0

inline void start_dma(UINT dst, UINT src, UINT count, BYTE width);
inline void auto_dma(UINT dst, UINT src, UINT bytecount);


/* DMA transfer list */
typedef struct _dmalist_ent_t {
    unsigned int source_phys_addr;
    unsigned int dest_phys_addr;
    unsigned int bytecount;
} dmalist_ent_t;

typedef struct _dmalist_t {
    dmalist_ent_t *entry;
    unsigned int num_entries;
    unsigned int allocated_entries;    
    unsigned int cur_entry;
} dmalist_t;


/* kiobuf DMA */
typedef struct _dmablt_kiobuf_t {
    struct semaphore sem;
    struct kiobuf *iobufYs;
    struct kiobuf *iobufUVs;    
    BYTE iobuf_locked;
    dmalist_t list;
    GFX_DMABLT_MODE_T synchMode;    
} dmablt_kiobuf_t;

void dmablt_kiobuf_init();
inline void dmablt_kiobuf_addtransfer(BYTE** dest, iobuf_pos_t* bufpos, UINT bytecount, UINT repeat);
void dmablt_kiobuf_transfer(GFX_DMABLT_PARM_T *pParm);


/* contig DMA */
typedef enum
{
  DRAW1X,
  DRAWMX
} DRAWSTATE_T;
typedef struct _dmablt_contig_t {
    struct semaphore sem;
    GFX_DMABLT_PARM_T p;
    BYTE component;
    UINT linecount;
    INT draw1x_lines_left;
    INT drawMx_lines_left;
    INT drawMx_count_left;
    DRAWSTATE_T draw_state;
} dmablt_contig_t;

void dmablt_contig_init();
void dmablt_contig_transfer(GFX_DMABLT_PARM_T *pParm);



#endif /* _GFX_DMA_H */
