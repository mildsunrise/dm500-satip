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
|       IBM CONFIDENTIAL
|       STB025XX VXWORKS EVALUATION KIT SOFTWARE
|       (C) COPYRIGHT IBM CORPORATION 2003
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| Author:    David Judkovics
| Component: teletext
| File:      ttx_dbg.h
| Purpose:
| Changes:
| Date:         Comment:
| -----         --------
| 25-Apr-01     Created
---------------------------------------------------------------------------*/
#ifndef _TTX_DBG_H
#define _TTX_DBG_H

/*----------------------------------------------------------------------------+
| Data Declarations
+----------------------------------------------------------------------------*/
typedef struct dbg_stat {
    unsigned char *base,
                  *ptr;
    unsigned int  size,
                  wrap;
} dbg_stat, *Dbg_stat;

/*----------------------------------------------------------------------------+
| Prototype Definitions
+----------------------------------------------------------------------------*/
int  ttx_debug_init(unsigned int size, Dbg_stat *dbg_ptr);
void ttx_debug_data(Dbg_stat debug_stat, char *sptr, unsigned int data);
void ttx_debug_2data(Dbg_stat debug_stat, char *sptr, unsigned int a, unsigned int b);
void ttx_debug_3data(Dbg_stat debug_stat, char *sptr,
                            unsigned int a, unsigned int b, unsigned int c);
void ttx_debug_string(Dbg_stat debug_stat, unsigned char *dptr, unsigned int size);
void ttx_debug_timestamp(Dbg_stat debug_stat, unsigned int x, unsigned int y);
void ttx_debug_4data(Dbg_stat debug_stat,
                 unsigned int a, unsigned int b, unsigned int c, unsigned int d);
void ttx_debug_dump(Dbg_stat debug_stat, unsigned int start, int line_count);

#endif /* ifndef _TTX_DBG_H */
