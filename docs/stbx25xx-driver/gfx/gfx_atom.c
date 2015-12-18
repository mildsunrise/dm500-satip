//vulcan/drv/gfx/gfx_atom.c
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

#include <os/os-types.h>
#include <os/os-io.h>
#include <os/os-interrupt.h>
#include <os/os-sync.h>
#include <os/drv_debug.h>
#include <gfx/gfx_inf_struct.h>

#include <asm/semaphore.h>
#include <asm/io.h>
#include <linux/iobuf.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include "osd_atom.h"
#include "osd_osi.h"
#include "gfx_atom.h"

#include "../vid/vid_atom_hw.h"

static GFX_SWAP_SURFACE_T  osdanim_swap; // gfx_inf_h_swap_surface()  <--> gfx_atom_osdanim_irq_handler()

extern unsigned int stb_vid_int_status; // exported from the av_core driver (vid_atom_int.c)

static GFX_DMABLT_SRCMEM_T dmablt_type; // tell interrupt handler which transfer method to use
static dmablt_contig_t dmablt_contig;
static dmablt_kiobuf_t dmablt_kiobuf;

// local data structure
typedef struct __GFX_HANDLE_T_STRUCTURE__
{
    GFX_SURFACE_T  surface;
    UINT    uLocks;
    GFX_SURFACE_PHYSICAL_PARM_T phy;
} GFX_HANDLE_T;
extern GFX_HANDLE_T   *gpHandles;  // declared in gfx_inf_helper.c

void gfx_atom_osdanim_irq_handler(int irq, void* dev_id, struct pt_regs *regs)
{
    unsigned long reg;

    //PDEBUG("\n[IRQ = %d\n", uIrq);
    //get irq status

    // The av_core driver reads the video decoder interrupt status register first,
    // so it has already been reset.  Use the exported value from av_core instead.
//    reg = MF_DCR(VID_HOST_INT);
    reg = stb_vid_int_status;

    /*------------------------------------------------------------------------+
    | OSD animation interrupt
    +------------------------------------------------------------------------*/
    // when this interrupt goes active, there is 1.2ms to swap surfaces
    if (reg & 0x1) {
        if(osdanim_swap.bReadyForSwap) {
            osdanim_swap.bReadyForSwap = 0;
            if(osdanim_swap.hOldSurface != 0) {
                osd_osi_detach_comp_gfx_surface(osdanim_swap.hGraphDev,
                                                &gpHandles[osdanim_swap.hOldSurface].surface);
            }
            osd_osi_attach_comp_gfx_surface(osdanim_swap.hGraphDev,
                                            &gpHandles[osdanim_swap.hNewSurface].surface);
        }
        osdanim_swap.missedInts++;
        up(&osdanim_swap.sem);
    }
    return;
}

void gfx_atom_dmablt_irq_handler(UINT uIrq, void *pData)
{
    if(dmablt_type == GFX_DMABLT_SRCMEM_USER) { // use kiobuf to DMA directly from user-space memory
        if(MF_DCR(0xe0) & 0x40000000) {
            MT_DCR(0xe0, 0x40000000);
            /* start next DMA */
            if(++dmablt_kiobuf.list.cur_entry < dmablt_kiobuf.list.num_entries) {
                auto_dma(    dmablt_kiobuf.list.entry[dmablt_kiobuf.list.cur_entry].dest_phys_addr,
                             dmablt_kiobuf.list.entry[dmablt_kiobuf.list.cur_entry].source_phys_addr,
                            dmablt_kiobuf.list.entry[dmablt_kiobuf.list.cur_entry].bytecount);
            } else {
                up(&dmablt_kiobuf.sem);
            }
        }

    } else if(dmablt_type == GFX_DMABLT_SRCMEM_PHYS) { // source memory is contiguous
        BYTE repeat_prev_line=0;

        if(MF_DCR(0xe0) & 0x40000000) {
            MT_DCR(0xe0, 0x40000000);

            switch(dmablt_contig.draw_state) {
                case DRAW1X:
                    if(--dmablt_contig.draw1x_lines_left == 0) {
                        dmablt_contig.draw1x_lines_left = dmablt_contig.p.draw1xLines;
						if(dmablt_contig.p.drawMxLines != 0) {
	                        dmablt_contig.draw_state = DRAWMX;
						}
                    }
                    repeat_prev_line = 0;
                    break;

                case DRAWMX:
                    if(--dmablt_contig.drawMx_count_left == 0) {
                        dmablt_contig.drawMx_count_left = dmablt_contig.p.drawMxCount;
                        if(--dmablt_contig.drawMx_lines_left == 0) {
                            dmablt_contig.drawMx_lines_left = dmablt_contig.p.drawMxLines;
                            if(dmablt_contig.p.draw1xLines != 0) {
                                dmablt_contig.draw_state = DRAW1X;
                            } else {
                                dmablt_contig.draw_state = DRAWMX;
                            }
                        }
                        repeat_prev_line = 0;
                    } else {
                        repeat_prev_line = 1;
                    }
                    break;
            }

            switch(dmablt_contig.component) {
                case 0:
                    dmablt_contig.p.dstY += dmablt_contig.p.dstStride;
                    if(repeat_prev_line) {
                        auto_dma((UINT)dmablt_contig.p.dstY, (UINT)dmablt_contig.p.srcY, dmablt_contig.p.dstStride);
                    } else if(++dmablt_contig.linecount == dmablt_contig.p.imageHeight) {
                        /* done with Y */
                        dmablt_contig.linecount = 0;
                        dmablt_contig.component = 1;
                        dmablt_contig.draw1x_lines_left = dmablt_contig.p.draw1xLines;
                        dmablt_contig.drawMx_lines_left = dmablt_contig.p.drawMxLines;
                        dmablt_contig.drawMx_count_left = dmablt_contig.p.drawMxCount;
                        if(dmablt_contig.p.draw1xLines != 0) {
                            dmablt_contig.draw_state = DRAW1X;
                        } else {
                            dmablt_contig.draw_state = DRAWMX;
                        }
                        auto_dma((UINT)dmablt_contig.p.dstUV, (UINT)dmablt_contig.p.srcUV, dmablt_contig.p.dstStride);
                    } else {
                        dmablt_contig.p.srcY += dmablt_contig.p.srcStride;
                        auto_dma((UINT)dmablt_contig.p.dstY, (UINT)dmablt_contig.p.srcY, dmablt_contig.p.dstStride);
                    }
                    break;

                case 1:
                    dmablt_contig.p.dstUV += dmablt_contig.p.dstStride;
                    if(repeat_prev_line) {
                        auto_dma((UINT)dmablt_contig.p.dstUV, (UINT)dmablt_contig.p.srcUV, dmablt_contig.p.dstStride);
                    } else if(++dmablt_contig.linecount == dmablt_contig.p.imageHeight/2) {
                        /* done with UV */
                        up(&dmablt_contig.sem);
                        dmablt_contig.component = 2;
                    } else {
                        dmablt_contig.p.srcUV += dmablt_contig.p.srcStride;
                        auto_dma((UINT)dmablt_contig.p.dstUV, (UINT)dmablt_contig.p.srcUV, dmablt_contig.p.dstStride);
                    }
                    break;

             
            }
        }
    } else {
        PDEBUG("invalid dmablt_type (%d)\n", dmablt_type);
    }
    return;
}


int gfx_atom_init(void)
{
//    UINT32 flags;
	
    sema_init(&osdanim_swap.sem, 0);
    osdanim_swap.bReadyForSwap = 0;

    //install interrupt handler
#if 0
    if(os_install_irq(IRQ_VID,
                      IRQ_LEVEL_TRIG | IRQ_POSITIVE_TRIG,
					  gfx_atom_osdanim_irq_handler,
					  (void*)0) != 0)
    {
printk(KERN_ALERT "IRQ_VID install failed\n");
        PDEBUG("install interrupt handler error\n");
        return -1;
    }
#else
    if(request_irq(IRQ_VID, gfx_atom_osdanim_irq_handler, SA_SHIRQ, "OSDanim", &gfx_atom_init) != 0) {
        PDEBUG("install interrupt handler error\n");
        return -1;
	}
#endif
    
/* moved setting the mask to osd_atom_set_display_cntl when the amination interrupt bit is set    
    os_disable_irq(IRQ_VID);

    // enable OSD animation interrupt from video decoder
    flags = os_enter_critical_section();
    MT_DCR(VID_MASK, MF_DCR(VID_MASK) | DECOD_HOST_INT_OSD_DATA);
    os_leave_critical_section(flags);

    os_enable_irq(IRQ_VID);
*/
    PDEBUG("install irq handler and task OK\n");

    //install DMA1 interrupt handler
    // initialize both transfer mechanisms so either can be used
    dmablt_contig_init();
    dmablt_kiobuf_init();    

    if(os_install_irq(IRQ_DMA1,
                      IRQ_LEVEL_TRIG | IRQ_POSITIVE_TRIG,
                      gfx_atom_dmablt_irq_handler,
                      (void*)0) != 0)
    {
        PDEBUG("dma blt install interrupt handler error\n");
        return -1;
    }

    // this interrupt should already be enabled
//    os_enable_irq(IRQ_DMA1);
    return 0;
}

void gfx_atom_deinit(void)
{
/*  moved setting the mask to osd_atom_set_display_cntl when the amination interrupt bit is reset   
    UINT32 flags;
 
    os_disable_irq(IRQ_VID);
    // disable OSD animation interrupt from video decoder
    flags = os_enter_critical_section();
    MT_DCR(VID_MASK, MF_DCR(VID_MASK) & ~DECOD_HOST_INT_OSD_DATA);
    os_leave_critical_section(flags);
*/    
#if 0
    os_uninstall_irq(IRQ_VID);      //remove irq handler
#else
    free_irq(IRQ_VID, &gfx_atom_init);
#endif

      //  leave interrupt enabled
//    os_disable_irq(IRQ_DMA1);
    os_uninstall_irq(IRQ_DMA1);      //remove irq handler
}



int iobuf_lock(struct kiobuf **iobuf, int access, void *userbuf, int size)
{
    int result;
    result = alloc_kiovec(1, iobuf);
    if(result) {
        printk(KERN_WARNING "failed to allocate kiobuf\n");
        return result;
    }
    result = map_user_kiobuf(access, *iobuf, (unsigned int)userbuf, size);
    if(result) {
        free_kiovec(1, iobuf);
        printk(KERN_WARNING "failed to map kiobuf\n");
    }
    return 0;
}

void iobuf_unlock(struct kiobuf **iobuf)
{
    unmap_kiobuf(*iobuf);
    free_kiovec(1, iobuf);    
}

inline void iobuf_pos_init(iobuf_pos_t *ip, struct kiobuf *iobuf)
{
    ip->iobuf = iobuf;
    ip->offset = iobuf->offset;
    ip->pageno = 0;
}

inline UINT iobuf_pos_get(iobuf_pos_t *ip, UINT count, BYTE* *vaddr)
{
    UINT valid_count;
    *vaddr = (BYTE*)page_address(ip->iobuf->maplist[ip->pageno])+ip->offset;
    valid_count = count;
    if(ip->offset + valid_count > PAGE_SIZE) {
        valid_count = PAGE_SIZE - ip->offset;
        ip->pageno++;
        ip->offset=0;
    } else {
        ip->offset += valid_count;
    }        
    return valid_count;
}

inline void iobuf_pos_skip(iobuf_pos_t *ip, UINT count)
{
    ip->offset += count;
    while(ip->offset > PAGE_SIZE) {
        ip->pageno++;
        ip->offset -= PAGE_SIZE;
    }
}

void dmablt_kiobuf_init()
{
    dmablt_kiobuf.iobuf_locked = 0;
    sema_init(&dmablt_kiobuf.sem, 1);
}

inline void dmablt_kiobuf_addtransfer(BYTE** dest, iobuf_pos_t* bufpos, UINT bytecount, UINT repeat)
{
    BYTE* vaddr;
    UINT first_entry;
    UINT last_entry;    
    UINT i;

    first_entry = dmablt_kiobuf.list.cur_entry;
    while(bytecount > 0) {
        if(dmablt_kiobuf.list.cur_entry == dmablt_kiobuf.list.allocated_entries) {
            printk(KERN_ALERT "ran out of entries at %d\n", dmablt_kiobuf.list.cur_entry);
            return;
        }
        dmablt_kiobuf.list.entry[dmablt_kiobuf.list.cur_entry].bytecount = iobuf_pos_get(bufpos, bytecount, &vaddr);
        dmablt_kiobuf.list.entry[dmablt_kiobuf.list.cur_entry].source_phys_addr = virt_to_bus(vaddr);
        dmablt_kiobuf.list.entry[dmablt_kiobuf.list.cur_entry].dest_phys_addr = virt_to_bus(*dest);
        *dest += dmablt_kiobuf.list.entry[dmablt_kiobuf.list.cur_entry].bytecount;
        bytecount -= dmablt_kiobuf.list.entry[dmablt_kiobuf.list.cur_entry].bytecount;                
        dmablt_kiobuf.list.cur_entry++;
    }
    while(repeat > 0) {
        last_entry = dmablt_kiobuf.list.cur_entry;
        for(i=first_entry; i < last_entry; i++) {
            if(dmablt_kiobuf.list.cur_entry == dmablt_kiobuf.list.allocated_entries) {
                printk(KERN_ALERT "ran out of entries at %d\n", dmablt_kiobuf.list.cur_entry);
                return;
            }
            dmablt_kiobuf.list.entry[dmablt_kiobuf.list.cur_entry].bytecount = dmablt_kiobuf.list.entry[i].bytecount;
            dmablt_kiobuf.list.entry[dmablt_kiobuf.list.cur_entry].source_phys_addr = dmablt_kiobuf.list.entry[i].source_phys_addr;
            dmablt_kiobuf.list.entry[dmablt_kiobuf.list.cur_entry].dest_phys_addr = virt_to_bus(*dest);
            *dest += dmablt_kiobuf.list.entry[dmablt_kiobuf.list.cur_entry].bytecount;
            dmablt_kiobuf.list.cur_entry++;        
        }
        repeat--;
    }
}

void dmablt_kiobuf_transfer(GFX_DMABLT_PARM_T *pParm)
{
    UINT i,j;
    UINT nent;
    iobuf_pos_t Ybufpos;
    iobuf_pos_t UVbufpos;

    if(pParm->synchMode == GFX_DMABLT_MODE_ASYNCH) {
        /* wait for prior DMA to finish */
//        waitfor_dma();
        down_interruptible(&dmablt_kiobuf.sem);
	}
    dmablt_kiobuf.synchMode = pParm->synchMode;

    /* release any previously pinned memory pages */
    if(dmablt_kiobuf.iobuf_locked) {
        iobuf_unlock(&dmablt_kiobuf.iobufYs);
        iobuf_unlock(&dmablt_kiobuf.iobufUVs);    
        kfree(dmablt_kiobuf.list.entry);
    }

    /* allocate memory for the list of DMA transfers */
    nent = pParm->imageHeight * 8;
    dmablt_kiobuf.list.entry = (dmalist_ent_t *)kmalloc(nent*sizeof(dmalist_ent_t), GFP_KERNEL);
    dmablt_kiobuf.list.allocated_entries = nent;

    /* page-in and pin buffers in memory for DMA transfer */
    iobuf_lock(&dmablt_kiobuf.iobufYs,  READ,  pParm->srcY,  pParm->srcStride*pParm->imageHeight);
    iobuf_lock(&dmablt_kiobuf.iobufUVs, READ,  pParm->srcUV, pParm->srcStride*pParm->imageHeight);    
    dmablt_kiobuf.iobuf_locked = 1;

    /* create list of DMA transfers */
    dmablt_kiobuf.list.cur_entry = 0;

    iobuf_pos_init(&Ybufpos, dmablt_kiobuf.iobufYs);
    iobuf_pos_skip(&Ybufpos, pParm->srcLineOffset * pParm->srcStride);    

    i=0;
    while(i < pParm->imageHeight) {
        for(j=0; j < pParm->draw1xLines; j++) {
            dmablt_kiobuf_addtransfer(&pParm->dstY, &Ybufpos, pParm->dstStride, 0);
            iobuf_pos_skip(&Ybufpos, pParm->srcStride-pParm->dstStride);
            i++;
        }
        for(j=0; j < pParm->drawMxLines; j++) {
            dmablt_kiobuf_addtransfer(&pParm->dstY, &Ybufpos, pParm->dstStride, pParm->drawMxCount-1);
            iobuf_pos_skip(&Ybufpos, pParm->srcStride-pParm->dstStride);
            i++;
        }
    }

    iobuf_pos_init(&UVbufpos, dmablt_kiobuf.iobufUVs);
    iobuf_pos_skip(&UVbufpos, (pParm->srcLineOffset<<1) * pParm->srcStride);
    i=0;
    while(i < pParm->imageHeight/2) {
        for(j=0; j < pParm->draw1xLines; j++) {
            dmablt_kiobuf_addtransfer(&pParm->dstUV, &UVbufpos, pParm->dstStride, 0);
            iobuf_pos_skip(&UVbufpos, pParm->srcStride-pParm->dstStride);
            i++;
        }
        for(j=0; j < pParm->drawMxLines; j++) {
            dmablt_kiobuf_addtransfer(&pParm->dstUV, &UVbufpos, pParm->dstStride, pParm->drawMxCount-1);
            iobuf_pos_skip(&UVbufpos, pParm->srcStride-pParm->dstStride);
            i++;
        }
    }


    /* initiate first DMA transfer */
    dmablt_kiobuf.list.num_entries = dmablt_kiobuf.list.cur_entry;    
    dmablt_kiobuf.list.cur_entry = 0;

    if(pParm->synchMode == GFX_DMABLT_MODE_ASYNCH) {
        auto_dma(    dmablt_kiobuf.list.entry[0].dest_phys_addr,
                    dmablt_kiobuf.list.entry[0].source_phys_addr,
                    dmablt_kiobuf.list.entry[0].bytecount);
        /* when DMA finishes, ISR does "up(&dmablt_kiobuf.sem);" */
    } else {
        /* wait until DMA finishes before returning */
        sema_init(&dmablt_kiobuf.sem, 0);
        auto_dma(    dmablt_kiobuf.list.entry[0].dest_phys_addr,
                    dmablt_kiobuf.list.entry[0].source_phys_addr,
                    dmablt_kiobuf.list.entry[0].bytecount);
        /* when DMA finishes, ISR does "up(&dmablt_kiobuf.sem);" */
        down_interruptible(&dmablt_kiobuf.sem);
    }
}

void dmablt_contig_init()
{
    sema_init(&dmablt_contig.sem, 1);
}

void dmablt_contig_transfer(GFX_DMABLT_PARM_T *pParm)
{
    if(pParm->synchMode == GFX_DMABLT_MODE_ASYNCH) {
        /* wait for prior DMA to finish */
        down_interruptible(&dmablt_contig.sem);
    }

    memcpy(&dmablt_contig.p, pParm, sizeof(GFX_DMABLT_PARM_T));

    dmablt_contig.p.srcY = (BYTE*)virt_to_bus(dmablt_contig.p.srcY+(dmablt_contig.p.srcLineOffset*dmablt_contig.p.srcStride));
    dmablt_contig.p.dstY = (BYTE*)virt_to_bus(dmablt_contig.p.dstY);    
    dmablt_contig.p.srcUV = (BYTE*)virt_to_bus(dmablt_contig.p.srcUV+((dmablt_contig.p.srcLineOffset>>1)*dmablt_contig.p.srcStride));
    dmablt_contig.p.dstUV = (BYTE*)virt_to_bus(dmablt_contig.p.dstUV);    

    dmablt_contig.draw1x_lines_left = dmablt_contig.p.draw1xLines;
    dmablt_contig.drawMx_lines_left = dmablt_contig.p.drawMxLines;
    dmablt_contig.drawMx_count_left = dmablt_contig.p.drawMxCount;
    dmablt_contig.component = 0;
    dmablt_contig.linecount = 0;
    if(dmablt_contig.p.draw1xLines != 0) {
        dmablt_contig.draw_state = DRAW1X;
    } else {
        dmablt_contig.draw_state = DRAWMX;
    }

    if(pParm->synchMode == GFX_DMABLT_MODE_ASYNCH) {
        auto_dma((UINT)dmablt_contig.p.dstY, (UINT)dmablt_contig.p.srcY, dmablt_contig.p.dstStride);
        /* when DMA finishes, ISR does "up(&dmablt_contig.sem);" */
	} else {
        /* wait until DMA finishes before returning */
        sema_init(&dmablt_contig.sem, 0);
        auto_dma((UINT)dmablt_contig.p.dstY, (UINT)dmablt_contig.p.srcY, dmablt_contig.p.dstStride);
        /* when DMA finishes, ISR does "up(&dmablt_contig.sem);" */
        down_interruptible(&dmablt_contig.sem);
    }
}


int linedma_count = 0;
int worddma_count = 0;
int bytedma_count = 0;
inline void auto_dma(UINT dst, UINT src, UINT bytecount)
{
    if((src & 0xF) == 0 && (dst & 0xF) == 0 && (bytecount & 0xF) == 0) {
        /* 16-byte line DMA */
        start_dma(dst, src, bytecount>>4, 3);
        linedma_count++;
    } else if((src & 0x3) == 0 && (dst & 0x3) == 0 && (bytecount & 0x3) == 0) {
        /* 4-byte word DMA */
        start_dma(dst, src, bytecount>>2, 2);
        worddma_count++;
    } else {
        /* single byte width DMA */
        start_dma(dst, src, bytecount, 1);    
        bytedma_count++;
    }
}

//#define PRIORITY_MASK 0x00000000 /* low priority */
#define PRIORITY_MASK 0x00800001 /* high priority */
inline void start_dma(UINT dst, UINT src, UINT count, BYTE width)
{
    MT_DCR(DMA0SA1, src);          /* set source address */
    MT_DCR(DMA0DA1, dst);          /* set destination address */
    MT_DCR(DMA0CT1, count);              /* set transfer count */
    MT_DCR(DMA0CR1, 0xC3400302 | PRIORITY_MASK | (width << 26));
}

void waitfor_dma()
{
    int i=0;
    while((MF_DCR(DMASR0) & 0x00000200)) i++;
    if(i != 0) {
        printk(KERN_ALERT "waited for DMA %d times\n", i);
    }
}

int i=0;
int gfx_atom_dmablt_handle_request(GFX_DMABLT_PARM_T *pParm)
{
    // tell interrupt handler which transfer method to use
    dmablt_type = pParm->srcMemType;
    
    // initiate transfer using the appropriate method
    if(pParm->srcMemType == GFX_DMABLT_SRCMEM_PHYS) {
        dmablt_contig_transfer(pParm);
	} else if(pParm->srcMemType == GFX_DMABLT_SRCMEM_USER) {
        dmablt_kiobuf_transfer(pParm);
	}
#if 0
    if(++i == 30) {
        i = 0;
        printk(KERN_ALERT "line %d, word %d, byte %d\n", linedma_count, worddma_count, bytedma_count);
    }
#endif
    return 0;
}

int gfx_atom_dmablt_wait_handle_request(void)
{
    if(dmablt_type == GFX_DMABLT_SRCMEM_PHYS) {
        if(dmablt_contig.p.synchMode == GFX_DMABLT_MODE_ASYNCH) {
            down_interruptible(&dmablt_contig.sem);
            up(&dmablt_contig.sem);
	    }
	} else if(dmablt_type == GFX_DMABLT_SRCMEM_USER) {
        if(dmablt_kiobuf.synchMode == GFX_DMABLT_MODE_ASYNCH) {
            down_interruptible(&dmablt_kiobuf.sem);
            up(&dmablt_kiobuf.sem);
        }
	}
    return 0;
}

int gfx_atom_pmalloc_handle_request(GFX_PMALLOC_PARM_T *pParm)
{
    struct kiobuf *iobuf;
    iobuf_pos_t iobuf_pos;
    BYTE *vaddr;
    
    switch(pParm->mode) {
        case GFX_PMALLOC_ALLOCATE:
	        /* occupy an integral number of pages */
		    pParm->uAllocSize += PAGE_SIZE - (pParm->uAllocSize & (PAGE_SIZE-1));

			//printk(KERN_ALERT "Allocate size = %d bytes\n", pParm->uAllocSize);

            pParm->hBuffer = os_alloc_physical_justify(pParm->uAllocSize, OSD_ADDRESS_ALIGNMENT);

            if(NULL == pParm->hBuffer)
            {
                printk(KERN_INFO "Failed to alloc %d buffer for new surface!\n", pParm->uAllocSize);
                pParm->hBuffer = NULL;
                return -1;
            }

            printk(KERN_INFO "Buffer allocated = %08x \n", (UINT)pParm->hBuffer);
        
            pParm->uPhysical = os_get_physical_address(pParm->hBuffer);
            pParm->pLogical = os_get_logical_address(pParm->hBuffer);
            pParm->uSize = os_get_actual_physical_size(pParm->hBuffer);
            break;
            
        case GFX_PMALLOC_FREE:
            os_free_physical(pParm->hBuffer);
            break;
            
        case GFX_PMALLOC_CHKALIGN:            
            iobuf_lock(&iobuf, READ, pParm->pLogical, pParm->uSize);
            iobuf_pos_init(&iobuf_pos, iobuf);
            iobuf_pos_get(&iobuf_pos, 1, &vaddr);
            pParm->uPhysical = virt_to_bus(vaddr);
            iobuf_unlock(&iobuf);
            break;

        default:
            printk(KERN_ALERT "invalid mode\n");
    }
    return 0;
}
