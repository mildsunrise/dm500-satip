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
|   COPYRIGHT   I B M   CORPORATION 1998
|   LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
|
|   Author    :  Ian Govett
|   Component :  xp
|   File      :  xp_osi_key.c
|   Purpose   :  Descrambler Management
|
|   Changes   :
|   Date        By   Description
|   ----------  ---  ---------------------------------------------------
|   15-June-98  IG   Created
|   30-Sep-01   LGH  Ported to Linux, combined codes of 3 devices
+----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------+

|                      Demux Descrambler Management

+-----------------------------------------------------------------------------+
|
|   The descrambler functions in the transport demux driver provide access
|   to the descrambler keys, and functions to define the level of
|   descrambling (PES or tspacket) for each channel.
|
|   Eight descrambler key sets are available, with each keyset consisting
|   of an initial, odd, and even keys.  Each key is a 64-bit value.  The
|   key set number (0..7) and level of descrambling (PES or tspacket) is
|   defined to the channel using the xp_channel_set_key() function.  The
|   values provided to the xp_channel_set_key() function will not be in
|   effect until AFTER the xp_channel_set_pid() function is called.  This
|   allows channel changes to switch both the pid value, and keyset value
|   simultaneously.

|   The xp_key_set() and xp_key_get() functions provide access to write and
|   retrieve the value of a specific key (init,even, or odd) for a key set
|   number.
|
|   The xp_key_setall() and xp_key_getall() functions provide access to
|   write and retrieve the values of the keyset.  The xp_key_setall() 
|   function writes the init, even, and odd key values, and the 
|   xp_key_getall() retrieves those values into the structure provided.
|
+----------------------------------------------------------------------------*/
/* The necessary header files */
#include <linux/config.h>
#include <linux/version.h>
#ifdef MODVERSIONS
#include <linux/modversions.h>
#endif

#define  __NO_VERSION__
#include <linux/module.h>
#include <linux/kernel.h>
#include "xp_osi_global.h"
#include "xp_atom_reg.h"

/*----------------------------------------------------------------------------+
| XXXX   XX   XX   XXXXXX  XXXXXXX  XXXXXX   XX   XX     XX    XXXX
|  XX    XXX  XX   X XX X   XX   X   XX  XX  XXX  XX    XXXX    XX
|  XX    XXXX XX     XX     XX X     XX  XX  XXXX XX   XX  XX   XX
|  XX    XX XXXX     XX     XXXX     XXXXX   XX XXXX   XX  XX   XX
|  XX    XX  XXX     XX     XX X     XX XX   XX  XXX   XXXXXX   XX
|  XX    XX   XX     XX     XX   X   XX  XX  XX   XX   XX  XX   XX  XX
| XXXX   XX   XX    XXXX   XXXXXXX  XXX  XX  XX   XX   XX  XX  XXXXXXX
+----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------+
| XXXXXXX  XXX XXX   XXXXXX  XXXXXXX  XXXXXX   XX   XX     XX    XXXX
|  XX   X   XX XX    X XX X   XX   X   XX  XX  XXX  XX    XXXX    XX
|  XX X      XXX       XX     XX X     XX  XX  XXXX XX   XX  XX   XX
|  XXXX       X        XX     XXXX     XXXXX   XX XXXX   XX  XX   XX
|  XX X      XXX       XX     XX X     XX XX   XX  XXX   XXXXXX   XX
|  XX   X   XX XX      XX     XX   X   XX  XX  XX   XX   XX  XX   XX  XX
| XXXXXXX  XXX XXX    XXXX   XXXXXXX  XXX  XX  XX   XX   XX  XX  XXXXXXX
+----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------+
|   XX     XXXXXX    XXXXXX    XXXXX
|  XXXX    XX   XX     XX     XX   XX
| XX  XX   XX   XX     XX      XX
| XX  XX   XXXXX       XX        XX
| XXXXXX   XX          XX         XX
| XX  XX   XX          XX     XX   XX
| XX  XX   XX        XXXXXX    XXXXX
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
|   xp0_key_get
+-----------------------------------------------------------------------------+
|
|   DESCRIPTION:  retrieves the key values for a descrambler key set
|
|   PROTOTYPE  :  xp0_key_get(
|                 short id,
|                 XP_KEY_TYPE keytype,
|                 unsigned long key[])
|
|   ARGUMENTS  :  id            -  key set number
|                 keytype       -  type of key.  This must be one of the
|                                  following values:
|                                       XP_KEY_TYPE_INIT
|                                       XP_KEY_TYPE_ODD
|                                       XP_KEY_TYPE_EVEN
|                 key           -  descrambler key value
|
|   RETURNS    :  0 if successful, or non-zero if an error occurs
|
|   ERRORS     :  XP_ERROR_KEY_INDEX
|                 XP_ERROR_KEY_TYPE
|
|   COMMENTS   :  The key set value for the keyset number and keytype are
|                 returned to the caller.  A two element 'key' array must
|                 be provided by the caller.
|
+---------------------------------------------------------------------------*/
SHORT xp_osi_key_get(
     GLOBAL_RESOURCES *pGlobal,
     SHORT             wId,
     XP_KEY_TYPE       keytype,
     ULONG             *pKey)

{
    short rc=0;
    unsigned long addr;
    UINT32  flag;


    if((wId < 0) || (wId >= XP_KEYSET_COUNT))
    {
        rc = XP_ERROR_KEY_INDEX;
    }
    else if (( (int)keytype < XP_KEY_TYPE_INIT) ||
                  (keytype > XP_KEY_TYPE_EVEN))
    {
        rc = XP_ERROR_KEY_TYPE;
    }
    else
    {
        addr = XP_DCR_ADDR_BASE_DEKEY + (wId * 8) + (keytype * 2);

        flag = os_enter_critical_section();
        pKey[0] = xp_atom_dcr_read(pGlobal->uDeviceIndex,addr);
        pKey[1] = xp_atom_dcr_read(pGlobal->uDeviceIndex,++addr);
        os_leave_critical_section(flag);
   }

    return(rc);
}

/*----------------------------------------------------------------------------+
|   xp0_key_getall
+-----------------------------------------------------------------------------+
|
|   DESCRIPTION:  defines the key value for a descrambler key set
|
|   PROTOTYPE  :  xp0_key_getall(
|                 short id,
|                 XP_KEY_TABLE *key)
|
|   ARGUMENTS  :  id    -  key set number
|                 key   -  descrambler key value
|
|   RETURNS    :  0 if successful, or non-zero if an error occurs
|
|   ERRORS     :  XP_ERROR_KEY_INDEX
|                 XP_ERROR_KEY_TYPE
|
|   COMMENTS   :  The caller provides a pointer to a structure which will
|                 contain all the descrambler keys for the key id specified.
|                 The following keys will be retrieved.
|                       INIT_A
|                       INIT_B
|                       EVEN_A
|                       EVEN_B
|                       ODD_A
|                       ODD_B
|
+----------------------------------------------------------------------------*/
SHORT xp_osi_key_getall(
    GLOBAL_RESOURCES *pGlobal,
    SHORT             wId,
    XP_KEY_TABLE     *pKey)

{
    short rc=0;
    unsigned long addr;
    UINT32  flag;


    if((wId < 0) || (wId >= XP_KEYSET_COUNT))
    {
        rc = XP_ERROR_KEY_INDEX;
    }
    else
    {
        addr = XP_DCR_ADDR_BASE_DEKEY + (wId * 8);

        flag = os_enter_critical_section();

        pKey->init_a = xp_atom_dcr_read(pGlobal->uDeviceIndex,addr+XP_KEY_INDEX_INIT_A);
        pKey->init_b = xp_atom_dcr_read(pGlobal->uDeviceIndex,addr+XP_KEY_INDEX_INIT_B);
        pKey->even_a = xp_atom_dcr_read(pGlobal->uDeviceIndex,addr+XP_KEY_INDEX_EVEN_A);
        pKey->even_b = xp_atom_dcr_read(pGlobal->uDeviceIndex,addr+XP_KEY_INDEX_EVEN_B);
        pKey->odd_a  = xp_atom_dcr_read(pGlobal->uDeviceIndex,addr+XP_KEY_INDEX_ODD_A);
        pKey->odd_b  = xp_atom_dcr_read(pGlobal->uDeviceIndex,addr+XP_KEY_INDEX_ODD_B);

        os_leave_critical_section(flag);
    }

    return(rc);
}

/*----------------------------------------------------------------------------+
|   xp0_key_set
+-----------------------------------------------------------------------------+
|
|   DESCRIPTION:  defines the key value for a descrambler key set
|
|   PROTOTYPE  :  xp0_key_set(
|                 short id,
|                 XP_KEY_TYPE keytype,
|                 unsigned long key[])
|
|   ARGUMENTS  :  id       -  key set number
|                 keytype  -  type of key.  This must be one of the
|                             following values:
|                                  XP_KEY_TYPE_INIT
|                                  XP_KEY_TYPE_ODD
|                                  XP_KEY_TYPE_EVEN
|                 key      -  descrambler key value
|
|   RETURNS    :  0 if successful, or non-zero if an error occurs
|
|   ERRORS     :  XP_ERROR_KEY_INDEX
|                 XP_ERROR_KEY_TYPE
|
|   COMMENTS   :  The caller provides the descrambler keys in a two element
|                 array.  These values are written for the specific key set
|                 and keytype specified.
|
+----------------------------------------------------------------------------*/
SHORT xp_osi_key_set(
    GLOBAL_RESOURCES *pGlobal,
    SHORT             wId,
    XP_KEY_TYPE       keytype,
    ULONG            *pKey)

{
    short rc=0;
    unsigned long addr;
    UINT32  flag;


    if ((wId < 0) || (wId >= XP_KEYSET_COUNT))
    {
        rc = XP_ERROR_KEY_INDEX;
    }
    else if (( (int)keytype < XP_KEY_TYPE_INIT) ||
                   (keytype > XP_KEY_TYPE_ODD))
    {
        rc = XP_ERROR_KEY_TYPE;
    }
    else
    {
        addr = XP_DCR_ADDR_BASE_DEKEY + (wId * 8) + (keytype * 2);

        flag = os_enter_critical_section();
        xp_atom_dcr_write(pGlobal->uDeviceIndex,  addr, pKey[0]);
        xp_atom_dcr_write(pGlobal->uDeviceIndex,++addr, pKey[1]);
        os_leave_critical_section(flag);
    }

    return(rc);
}

/*----------------------------------------------------------------------------+
|   xp0_key_setall
+-----------------------------------------------------------------------------+
|
|   DESCRIPTION:  defines the key value for a descrambler key set
|
|   PROTOTYPE  :  xp0_key_setall(
|                 short id,
|                 XP_KEY_TABLE *key)
|
|   ARGUMENTS  :  id    -  key set number
|                 key   -  descrambler key value
|
|   RETURNS    :  0 if successful, or non-zero if an error occurs
|
|   ERRORS     :  XP_ERROR_KEY_INDEX
|                 XP_ERROR_KEY_TYPE
|
|   COMMENTS   :  The caller provides a pointer to structure with the
|                 descrambler keys that are to be written.  The following
|                 keys will be set.
|                       INIT_A
|                       INIT_B
|                       EVEN_A
|                       EVEN_B
|                       ODD_A
|                       ODD_B
|
+----------------------------------------------------------------------------*/
SHORT xp_osi_key_setall(
    GLOBAL_RESOURCES *pGlobal,
    SHORT             wId,
    XP_KEY_TABLE     *pKey)
{
    short rc=0;
    unsigned long addr;
    UINT32  flag;


    if((wId < 0) || (wId >= XP_KEYSET_COUNT))
    {
        rc = XP_ERROR_KEY_INDEX;
    }
    else
    {
        addr = XP_DCR_ADDR_BASE_DEKEY + (wId * 8);

        flag = os_enter_critical_section();
        xp_atom_dcr_write(pGlobal->uDeviceIndex, addr+XP_KEY_INDEX_INIT_A, pKey->init_a);
        xp_atom_dcr_write(pGlobal->uDeviceIndex, addr+XP_KEY_INDEX_INIT_B, pKey->init_b);
        xp_atom_dcr_write(pGlobal->uDeviceIndex, addr+XP_KEY_INDEX_EVEN_A, pKey->even_a);
        xp_atom_dcr_write(pGlobal->uDeviceIndex, addr+XP_KEY_INDEX_EVEN_B, pKey->even_b);
        xp_atom_dcr_write(pGlobal->uDeviceIndex, addr+XP_KEY_INDEX_ODD_A,  pKey->odd_a);
        xp_atom_dcr_write(pGlobal->uDeviceIndex, addr+XP_KEY_INDEX_ODD_B,  pKey->odd_b);
        os_leave_critical_section(flag);
    }

    return(rc);
}
