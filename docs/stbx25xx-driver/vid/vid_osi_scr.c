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
| File:   vid_osi_scr.c
| Purpose: screen mananger osi layer PALLAS
| Changes:
| Date:         Comment:
| -----         --------
| 15-Oct-01             create                                                                                          SL
+----------------------------------------------------------------------------*/
#include "vid_atom.h"
#include "vid_atom_local.h"
#include "vid_osi_scr.h"



extern UINT    _denc_id;        //setup parameter
extern UINT    _fmt;            //setup parameter


/*scrman_osi_init should be called when the whole module initialized*/
INT scrman_osi_init()
{
  if( _denc_id == DENC_INTERNAL )
  {
    if(_fmt == SCRMAN_FMT_NTSC)
      denc_init(DENC_MODE_NTSC);//ntsc
    else
      denc_init(DENC_MODE_PAL);
    //set internal denc display delay
  if(vid_atom_init(DECOD_DLY_DENC_INTERNAL) !=0 )
      return -1;
     //set MEM_SEGx in video decoder
    //vid_atom_set_memseg(&mem_segs[0]);
  }
  else
  {
    //not supported yet
  }

  //set display format
  if(scrman_osi_set_fmt(_fmt) != 0)
    return -1;
  //scrman_osi_colorbar_off();
  return 0;
}

INT scrman_osi_set_fmt(DENC_MODE mode)
{

  if(_denc_id == DENC_INTERNAL)
  {
    //set display format
    //fisrtly set denc display format
    if( mode  == SCRMAN_FMT_NTSC  )
    {
      if(denc_atom_set_dispfmt(DENC_ID0, DENC_MODE_NTSC) != 0)
        return -1;
      if(denc_atom_set_dispfmt(DENC_ID1, DENC_MODE_NTSC) != 0)
        return -1;
      //secondly set video decoder format
      vid_atom_set_dispfmt(0);
      _fmt = mode;
    }
    else if( mode == SCRMAN_FMT_PAL )
    {
      //firstly set denc display format
      if(denc_atom_set_dispfmt(DENC_ID0, DENC_MODE_PAL) != 0) // YYD, changed from NTSC
        return -1;
      if(denc_atom_set_dispfmt(DENC_ID1, DENC_MODE_PAL) != 0) // YYD, changed from NTSC
        return -1;
      //secondly set video decoder format
      vid_atom_set_dispfmt(1);
      _fmt = mode;
    }
  }
  else
  {
    //not supported yet
    return -1;
  }

  return 0;
}

INT scrman_osi_colorbar_on()
{
        if( _denc_id == DENC_INTERNAL )
        {
                if( denc_atom_set_colorbar(DENC_ID0, 1) != 0)
                        return -1;
                if( denc_atom_set_colorbar(DENC_ID1, 1) != 0)
                        return -1;
                return 0;
        }
        else
                return -1;
}

INT scrman_osi_colorbar_off()
{
        if( _denc_id == DENC_INTERNAL )
        {
                if( denc_atom_set_colorbar(DENC_ID0, 0) != 0)
                        return -1;
                if( denc_atom_set_colorbar(DENC_ID1, 0) != 0)
                        return -1;
                return 0;
        }
        else
                return -1;
}

UINT scrman_osi_get_denc_id()
{
        return _denc_id;
}

UINT scrman_osi_get_fmt()
{
        return _fmt;
}

INT scrman_osi_set_output(DENC_OUTFMT fmt)
{
        return denc_atom_set_outfmt(_denc_id, fmt);
}
