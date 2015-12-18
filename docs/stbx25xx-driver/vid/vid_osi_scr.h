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
| File:   vid_osi_scr.h
| Purpose:  scrman osi layer PALLAS
| Changes:
| Date:         Comment:
| -----         --------
| 15-Oct-01		create                  									SL
+----------------------------------------------------------------------------*/
#ifndef PALLAS_SCRMAN_OSI_H
#define PALLAS_SCRMAN_OSI_H
#include <os/os-types.h>
#include <hw/physical-mem.h>

#include "vid_atom_denc.h"

#define DENC_INTERNAL   0
#define DENC_EXTERNAL   1

#define SCRMAN_FMT_NTSC   DENC_MODE_NTSC     // YYD
#define SCRMAN_FMT_PAL    DENC_MODE_PAL      // YYD

#define DEFAULT_INTER_DENC0_OUTFMT      DENC_OUTFMT_LC
#define DEFAULT_INTER_DENC1_OUTFMT      DENC_OUTFMT_CVBS

INT scrman_osi_init();
INT scrman_osi_set_fmt(DENC_MODE mode);
INT scrman_osi_set_output(DENC_OUTFMT fmt);
INT scrman_osi_colorbar_on();
INT scrman_osi_colorbar_off();
UINT scrman_osi_get_denc_id();
UINT scrman_osi_get_fmt();

#endif
