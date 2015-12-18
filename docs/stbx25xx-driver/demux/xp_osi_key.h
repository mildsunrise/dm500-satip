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
|       COPYRIGHT   I B M   CORPORATION 1998
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------+
| Author:    Ian Govett
| Component: xp
| File:      xp_osi_key.h
| Purpose:   Transport key defines and prototypes.
| Changes:
|
| Date:      Author  Comment:
| ---------  ------  --------
| 04-May-01  TJC     Updates for Pallas.
| 30-Sep-01  LGH     Ported to Linux
+-----------------------------------------------------------------------------*/
#ifndef XP_KEY_H
#define XP_KEY_H

/*----------------------------------------------------------------------------+
|  Local Defines
+----------------------------------------------------------------------------*/
#define XP_KEY_INDEX_COUNT       6
#define XP_KEYSET_COUNT          8       /* number of descrambler key sets   */

/*----------------------------------------------------------------------------+
|  Type Declarations
+----------------------------------------------------------------------------*/
typedef enum xp_key_index {
    XP_KEY_INDEX_INIT_A  = 0,            /* Key Set array indices            */
    XP_KEY_INDEX_INIT_B  = 1,
    XP_KEY_INDEX_ODD_A   = 2,
    XP_KEY_INDEX_ODD_B   = 3,
    XP_KEY_INDEX_EVEN_A  = 4,
    XP_KEY_INDEX_EVEN_B  = 5,
} XP_KEY_INDEX;

typedef enum xp_key_type {               /* Types of descrambler keys        */
    XP_KEY_TYPE_INIT = 0,
    XP_KEY_TYPE_EVEN = 1,
    XP_KEY_TYPE_ODD  = 2,
} XP_KEY_TYPE;

typedef struct xp_key_table {            /* Key Set Table Elements           */
    unsigned long init_a;
    unsigned long init_b;
    unsigned long even_a;
    unsigned long even_b;
    unsigned long odd_a;
    unsigned long odd_b;
} XP_KEY_TABLE;

/*----------------------------------------------------------------------------+
|  Prototype Definitions
+----------------------------------------------------------------------------*/
SHORT xp_osi_key_get(GLOBAL_RESOURCES *pGlobal,SHORT wId,
                     XP_KEY_TYPE keytype,ULONG *pKey);

SHORT xp_osi_key_getall(GLOBAL_RESOURCES *pGlobal,SHORT wId, 
                     XP_KEY_TABLE *pKey);

SHORT xp_osi_key_set(GLOBAL_RESOURCES *pGlobal, SHORT wId,
                     XP_KEY_TYPE keytype, ULONG *pKey);

SHORT xp_osi_key_setall(GLOBAL_RESOURCES *pGlobal, SHORT wId, 
                     XP_KEY_TABLE *pKey);

#endif
