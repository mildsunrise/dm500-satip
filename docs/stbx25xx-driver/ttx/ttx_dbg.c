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
| File:      ttx_dbg.c
| Purpose:
| Changes:
| Date:         Comment:
| -----         --------
| 25-Apr-01     Created
---------------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "os/os-generic.h"
#include "ttx_dbg.h"

/*----------------------------------------------------------------------------+
| Static Variables
+----------------------------------------------------------------------------*/
static char landmark[16] = ">>>>>>>>>>>>>>>>";

/*----------------------------------------------------------------------------+
| ttx_debug_init
+----------------------------------------------------------------------------*/
int  ttx_debug_init(unsigned int size, Dbg_stat *dbg_ptr)
{

  if ((size % 16) != 0) {
      size = ((size + 16)/16)*16;
  }

  if ((*dbg_ptr = (Dbg_stat)MALLOC(sizeof(dbg_stat))) == NULL) {
      return -1;
  }


  if (((*dbg_ptr)->base = (unsigned char *)MALLOC(size)) == NULL) {
      FREE(*dbg_ptr);
      return -1;
  }

  (*dbg_ptr)->ptr = (*dbg_ptr)->base;
  (*dbg_ptr)->size = size;
  (*dbg_ptr)->wrap = 0;

  strncpy((*dbg_ptr)->ptr, landmark, 16);

  return 0;
}

/*----------------------------------------------------------------------------+
| ttx_debug_data
+----------------------------------------------------------------------------*/
void ttx_debug_data(Dbg_stat debug_stat, char *sptr, unsigned int data)
{
  strncpy(debug_stat->ptr, sptr, 12);
  *(unsigned int *)(debug_stat->ptr + 12) = data;

  debug_stat->ptr += 16;
  if (debug_stat->ptr - debug_stat->base >= debug_stat->size) {
      debug_stat->ptr = debug_stat->base;
      debug_stat->wrap++;
  }

  strncpy(debug_stat->ptr, landmark, 16);

  return;
}

/*----------------------------------------------------------------------------+
| ttx_debug_2data
+----------------------------------------------------------------------------*/
void ttx_debug_2data(Dbg_stat debug_stat, char *sptr,
                     unsigned int a, unsigned int b)
{
  strncpy(debug_stat->ptr, sptr, 8);
  *(unsigned int *)(debug_stat->ptr + 8) = a;
  *(unsigned int *)(debug_stat->ptr + 12) = b;

  debug_stat->ptr += 16;
  if (debug_stat->ptr - debug_stat->base >= debug_stat->size) {
      debug_stat->ptr = debug_stat->base;
      debug_stat->wrap++;
  }

  strncpy(debug_stat->ptr, landmark, 16);

  return;
}

/*----------------------------------------------------------------------------+
| ttx_debug_3data
+----------------------------------------------------------------------------*/
void ttx_debug_3data(Dbg_stat debug_stat, char *sptr,
                     unsigned int a, unsigned int b, unsigned int c)
{
  strncpy(debug_stat->ptr, sptr, 4);
  *(unsigned int *)(debug_stat->ptr + 4) = a;
  *(unsigned int *)(debug_stat->ptr + 8) = b;
  *(unsigned int *)(debug_stat->ptr + 12) = c;

  debug_stat->ptr += 16;
  if (debug_stat->ptr - debug_stat->base >= debug_stat->size) {
      debug_stat->ptr = debug_stat->base;
      debug_stat->wrap++;
  }

  strncpy(debug_stat->ptr, landmark, 16);

  return;
}

/*----------------------------------------------------------------------------+
| ttx_debug_4data
+----------------------------------------------------------------------------*/
void ttx_debug_4data(Dbg_stat debug_stat,
     unsigned int a, unsigned int b, unsigned int c, unsigned int d)
{

  *(unsigned int *)(debug_stat->ptr     ) = a;
  *(unsigned int *)(debug_stat->ptr + 4 ) = b;
  *(unsigned int *)(debug_stat->ptr + 8 ) = c;
  *(unsigned int *)(debug_stat->ptr + 12) = d;

  debug_stat->ptr += 16;
  if (debug_stat->ptr - debug_stat->base >= debug_stat->size) {
      debug_stat->ptr = debug_stat->base;
      debug_stat->wrap++;
  }

  strncpy(debug_stat->ptr, landmark, 16);

  return;
}

/*----------------------------------------------------------------------------+
| ttx_debug_string
+----------------------------------------------------------------------------*/
void ttx_debug_string(Dbg_stat debug_stat,unsigned char *dptr,unsigned int size)
{
  return;
}

/*----------------------------------------------------------------------------+
| ttx_debug_timestamp
+----------------------------------------------------------------------------*/
void ttx_debug_timestamp(Dbg_stat debug_stat, unsigned int x, unsigned int y)
{
  return;
}

/*----------------------------------------------------------------------------+
| ttx_debug_dump
+----------------------------------------------------------------------------*/
void ttx_debug_dump(Dbg_stat debug_stat, unsigned int start, int line_count)
{
  int i, j;
  char str[17];
  unsigned char *ptr = debug_stat->base + start*16;

  for (i = 0; i < line_count; i++) {

      for (j = 0; j < 16; j++) {
          str[j] = isprint(ptr[j]) ? ptr[j] : '.';
      }
      str[j] = '\0';

      printf("%08x: %08x %08x %08x %08x %s\n",
          (unsigned int)ptr,
          *(unsigned int *)ptr,
          *(unsigned int *)(ptr + 4),
          *(unsigned int *)(ptr + 8),
          *(unsigned int *)(ptr + 12),
          str);

      ptr += 16;
      if (ptr - debug_stat->base >= debug_stat->size) {
          ptr = debug_stat->base;
      }
  }
}
