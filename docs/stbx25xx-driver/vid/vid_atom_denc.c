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
| File:   vid_atom_denc.c
| Purpose: denc driver atom layer PALLAS
| Changes:
| Date:         Comment:
| -----         --------
| 15-Oct-01		create                  									SL
+----------------------------------------------------------------------------*/
#include <os/os-io.h>
#include <os/drv_debug.h>

#include "vid_atom_denc.h"

INT denc_init(DENC_MODE mode)
{
   unsigned long        reg0, reg1, reg_mux;

   MT_DCR(DENC0_RR1,0);
   MT_DCR(DENC1_RR1,0);
   MT_DCR(DENC0_RR2,0);
   MT_DCR(DENC1_RR2,0);
   MT_DCR(DENC0_RR3,0);
   MT_DCR(DENC1_RR3,0);
   MT_DCR(DENC0_RR4,0);
   MT_DCR(DENC1_RR4,0);
   MT_DCR(DENC0_RR5,0);
   MT_DCR(DENC1_RR5,0);
   MT_DCR(SECAM2,0x02500000);

   reg0 = MF_DCR(DENC0_CR1);
   reg1 = MF_DCR(DENC1_CR1);
   reg_mux = MF_DCR(DENCMUX);

   /*Program the output format of the DENCs.
     For Entry 5 of the table, denc0 outputs RGB,CVBS; denc1 outputs Y/C
     select the ports of the dencs. denc0-VP/MP, denc1-MP.*/

   MT_DCR(DENC0_CR1, reg0 & 0xFFF3FFFF);
   MT_DCR(DENC1_CR1, reg1 & 0xFFF3FFFF);

   // MT_DCR(DENCMUX, reg_mux | 0x23000000);
   MT_DCR(DENCMUX, reg_mux | 0xA6000000);    //  In pallas2, this one changed

   switch(mode) {
      case DENC_MODE_BAR_ON:
         MT_DCR(DENC0_CR1, (reg0 & 0xFFFFFF0F)|0x00100020);
		 MT_DCR(DENC1_CR1, (reg1 & 0xFFFFFF0F)|0x00100020); 
	     break;
      case DENC_MODE_BAR_OFF:
         MT_DCR(DENC0_CR1, (reg0 & 0xFFFFFF0F)|0x00100040);
		 MT_DCR(DENC1_CR1, (reg1 & 0xFFFFFF0F)|0x00100040);
         break;
      case DENC_MODE_NTSC:
          MT_DCR(DENC0_CR1,0x90100040); 
          MT_DCR(DENC1_CR1, 0x90100040);
         break;
      case DENC_MODE_PAL:
          MT_DCR(DENC0_CR1,0x91100040); 
          MT_DCR(DENC1_CR1, 0x91100040);         
          break;
   }
   return 0;
}


INT    denc_atom_set_outfmt(DENC_ID id, DENC_OUTFMT fmt)
{
	UINT uReg, uRegMux;

	if( id == DENC_ID0)
	{
		uReg = MF_DCR(DENC0_CR1);
		uReg &= 0xFFF3FFFF;
		uRegMux = MF_DCR(DENCMUX);
		uRegMux &= 0x87FFFFFF;
		switch(fmt)
		{
			case DENC_OUTFMT_RGB:
				MT_DCR(DENC0_CR1, uReg);
				break;
			case DENC_OUTFMT_LBR:
				uReg |= 0x00040000;
				MT_DCR(DENC0_CR1, uReg);
				break;
			case DENC_OUTFMT_LC:
				uRegMux |= 0x40000000;
				MT_DCR(DENCMUX, uRegMux);
				break;
			case DENC_OUTFMT_CVBS:
				uRegMux |= 0x78000000;
				MT_DCR(DENCMUX, uRegMux);
				break;
			default:
				return -1;
		}		
		return 0;
	}			
	if( id == DENC_ID1)
	{
        /*uReg = MF_DCR(DENC0_CR1);
		uRegMux = MF_DCR(DENCMUX);
		switch(fmt)
		{
			case DENC_OUTFMT_LC:
				uReg &= ~DENC_OUTFMT_MASK;
				uReg |= DENC_OUTFMT_MASK_LC;
				uRegMux &= ~DENC_DENCMUX_LC1;
				break;
			case DENC_OUTFMT_CVBS:
				uRegMux |= DENC_DENCMUX_CVBS1;
				break;
			default:
				return -1;
		}
		MT_DCR(DENCMUX, uRegMux | DENC_DENCMUX_DENC1_VP);
        PDEBUG("mux denc 1= %x\n", uRegMux);*/
		return 0;
	}
	return -1;
}



void   denc_atom_on(DENC_ID id)
{
    UINT uReg;
    if(id == DENC_ID0)
    {
        uReg = MF_DCR(DENC0_CR1) & (~DENC_DAC_DISABLE);
        MT_DCR(DENC0_CR1, uReg);
        return;
    }

    if(id == DENC_ID1)
    {
        uReg = MF_DCR(DENC1_CR1) & (~DENC_DAC_DISABLE);
        MT_DCR(DENC1_CR1, uReg);
        return;
    }
}


void   denc_atom_off(DENC_ID id)
{
    UINT uReg;
    if(id == DENC_ID0)
    {
        uReg = MF_DCR(DENC0_CR1);
        MT_DCR(DENC0_CR1, uReg | DENC_DAC_DISABLE);
        return;
    }

    if(id == DENC_ID1)
    {
        uReg = MF_DCR(DENC1_CR1);
        MT_DCR(DENC1_CR1, uReg | DENC_DAC_DISABLE);
        return;
    }
}
    



INT    denc_atom_set_dispfmt(DENC_ID id, DENC_MODE fmt)
{
	UINT uReg = 0;
	if( id == DENC_ID0 )
	{
        uReg = MF_DCR(DENC0_CR1);
		if( fmt == DENC_MODE_NTSC)
		{
			MT_DCR(DENC0_CR1, uReg & (~0x07000000));
			return 0;
		}
		else if (fmt == DENC_MODE_PAL)
		{
			MT_DCR(DENC0_CR1, (uReg & (~0x07000000)) | 0x01000000 );
			return 0;
		}
		return -1;

	}
    else if( id == DENC_ID1)
	{
        uReg = MF_DCR(DENC1_CR1);
		if( fmt == DENC_MODE_NTSC)
		{
			MT_DCR(DENC1_CR1, uReg & (~0x07000000));
			return 0;
		}
		else if (fmt == DENC_MODE_PAL)
		{
			MT_DCR(DENC1_CR1, (uReg & (~0x07000000)) | 0x01000000 );
			return 0;
		}
		return -1;

	}
    else
	    return -1;
    // YYD, comment out these below
    //set color bar off
    // denc_atom_set_colorbar(id, 0);
    //turn denc on
    // denc_atom_on(id);
    return 0;
}

INT    denc_atom_set_colorbar(DENC_ID id, UINT uFlag)
{
	UINT uReg;
	if( id == DENC_ID0 )
	{
		uReg = MF_DCR(DENC0_CR1);
		uReg &= (~DENC_TSTPAT_MASK);
		if( uFlag == 1)
			uReg |= DENC_TSTPAT_CBON;
		else if( uFlag == 0)
			uReg |= DENC_TSTPAT_CBOFF;
		else
			return -1;
		MT_DCR(DENC0_CR1, uReg);
	}
    else if( id == DENC_ID1)
	{
		uReg = MF_DCR(DENC1_CR1);
		uReg &= (~DENC_TSTPAT_MASK);
		if( uFlag == 1)
			uReg |= DENC_TSTPAT_CBON;
		else if( uFlag == 0)
			uReg |= DENC_TSTPAT_CBOFF;
		else
			return -1;
		MT_DCR(DENC1_CR1, uReg);
	}
    else
	    return -1;
    denc_atom_on(id);
    return 0;
}
