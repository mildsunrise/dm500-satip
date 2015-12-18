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
|       COPYRIGHT   I B M   CORPORATION 1999
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------+
| Author:    Maciej P. Tyrlik
| Component: Include file.
| File:      xp.h
| Purpose:   Transport general definitions.
| Changes:
|
| Date:      Author  Comment:
| ---------  ------  --------
| 05-Jan-00  TJC     Created.
| 26-May-01  TJC     Added new "XP_CONFIG_VALUES" structure and two new APIs
|                    xp_get_config_values and xp_set_config_values.
| 30-Sep-01  LGH     Ported to Linux
+----------------------------------------------------------------------------*/

#ifndef _xp_h_

#define _xp_h_



#include <os/os-generic.h>

#include <os/os-types.h>

#include <os/pm-alloc.h>

#include <os/os-interrupt.h>





/*----------------------------------------------------------------------------+

|  General Defines

+----------------------------------------------------------------------------*/

#define XP0                             0   /* Transport 0                   */

#define XP1                             1   /* Transport 1                   */

#define XP2                             2   /* Transport 2                   */



/*----------------------------------------------------------------------------+

|  Driver Error Return Codes

+----------------------------------------------------------------------------*/

#define XP_ERROR_MSG_COUNT              36  /* max error messages            */



/*----------------------------------------------------------------------------+

|  General Error Messages

+----------------------------------------------------------------------------*/

#define XP_ERROR_INVALID_ERROR_RC      -1   /* invalid error message number  */

#define XP_ERROR_INVALID_STC           -2   /* stc value is not valid        */

#define XP_ERROR_INTERNAL              -3   /* driver error                  */

#define XP_ERROR_OUT_OF_SPACE          -4   /* insufficient space available  */



/*----------------------------------------------------------------------------+

|  Channel Error Messages

+----------------------------------------------------------------------------*/

#define XP_ERROR_CHANNEL_INUSE         -5   /* channel is already in use     */

#define XP_ERROR_CHANNEL_ENABLED       -6   /* channel is already in use     */

#define XP_ERROR_CHANNEL_UNUSED        -7   /* channel was not allocated     */

#define XP_ERROR_CHANNEL_INVALID       -8   /* channel id. is invalid        */

#define XP_ERROR_CHANNEL_DISABLED      -9   /* channel id. is disabled       */

#define XP_ERROR_CHANNEL_UNDEFINED_UT  -10  /* channel unload type undefined */

#define XP_ERROR_CHANNEL_MAX           -11  /* no more channels available    */



/*----------------------------------------------------------------------------+

|  Queue Error Messages

+----------------------------------------------------------------------------*/

#define XP_ERROR_QUEUE_DEFINED         -12  /* queue already allocated       */

#define XP_ERROR_QUEUE_UNDEFINED       -13  /* queue not allocated to channel*/

#define XP_ERROR_QUEUE_ADDRESS         -14  /* queue address invalid         */

#define XP_ERROR_QUEUE_ENABLED         -15  /* queue is currently active     */

#define XP_ERROR_QUEUE_DISABLED        -16  /* queue is currently active     */

#define XP_ERROR_QUEUE_CONFIGURED      -17  /* queue is already configured   */

#define XP_ERROR_QUEUE_ALIGNMENT       -18  /* queue is not aligned          */

#define XP_ERROR_QUEUE_REGION_SIZE     -19  /* size of region to unlock is 0 */



/*----------------------------------------------------------------------------+

|  Filter Error Messages

+----------------------------------------------------------------------------*/

#define XP_ERROR_FILTER_UNAVAILABLE    -20  /* no more filters available     */

#define XP_ERROR_FILTER_INVALID        -21  /* filter id is invalid          */

#define XP_ERROR_FILTER_FREE           -22  /* filter id is not allocated    */

#define XP_ERROR_FILTER_DATA           -23  /* filter content is invalid     */

#define XP_ERROR_FILTER_INUSE          -24  /* filter already in use         */

#define XP_ERROR_FILTER_UNDEFINED      -25  /* filter not defined            */

#define XP_ERROR_FILTER_NOT_ASSIGN     -26  /* filter not assigned to channel*/

#define XP_ERROR_FILTER_BAD_LENGTH     -27  /* invalid filter length         */

#define XP_ERROR_FILTER_PENDING        -28  /* pending filter operation      */

#define XP_ERROR_FILTER_MAX            -29  /* maximum filters for channel   */



/*----------------------------------------------------------------------------+

|  Interrupt Error Messages

+----------------------------------------------------------------------------*/

#define XP_ERROR_INTER_MASK            -30  /* interrupt mask can't be found */

#define XP_ERROR_INTER_NOTIFY_CMD      -31  /* notify command invalid        */

#define XP_ERROR_INTER_NOTIFY_DUP      -32  /* duplicate notification funct  */

#define XP_ERROR_INTER_NOTIFY_INVALID  -33  /* notification function invalid */



/*----------------------------------------------------------------------------+

|  Descrambler Error Messages

+----------------------------------------------------------------------------*/

#define XP_ERROR_KEY_INDEX             -34  /* descrambler key index invalid */

#define XP_ERROR_KEY_TYPE              -35  /* descrambler key type invalid  */



/*----------------------------------------------------------------------------+

|  PSIT Error Messages

+----------------------------------------------------------------------------*/

#define XP_ERROR_PSIT_NOT_FOUND        -36  /* PSIT not found                */





/*----------------------------------------------------------------------------+

|  XP0 CONFIGURATION STRUCTURE

+----------------------------------------------------------------------------*/

typedef struct xp_config_type {

    /*------------------------------------------------------------------------+

    |  For additional information on the following bit definitions, refer to

    |  the Digital Set-Top Box Integrated Controller User's Guide Transport 0

    |  Configuration 1 Register (XPT0_CONFIG1)

    +------------------------------------------------------------------------*/

    unsigned denbl       : 1;               /* 1=descram enabled, 0=disabled */

    unsigned vpu         : 1;               /* 1=vid pckts also to Q31/BQ    */

    unsigned apu         : 1;               /* 1=aud packets also to Q30/BQ  */

    unsigned tstoe       : 1;               /* 1=TS timeout enable, 0=disab  */

    unsigned tsclkp      : 1;               /* 1=data latched on clk FE,0=RE */

    unsigned tsdp        : 1;               /* 1=CI_DATA inverted, 0=not inv */

    unsigned tssp        : 1;               /* 1=CI_PACKET_START low act,0=hi*/

    unsigned tsep        : 1;               /* 1=CI_DATA_ERROR low act,0=hi  */

    unsigned tsvp        : 1;               /* 1=CI_DATA_ENABLE low, 0=high  */

    unsigned tssm        : 1;               /* 1=CI_PACKET_START used, 0=not */

    unsigned syncd       : 2;               /* no. of syncbytes +1 dropped   */

    unsigned bbmode      : 1;               /* 1=bit mode, 0=byte mode       */

    unsigned syncl       : 3;               /* no. sync bytes+1 before sync  */



    /*------------------------------------------------------------------------+

    |  For additional information on the following bit definitions, refer to

    |  the Digital Set-Top Box Integrated Controller User's Guide Transport 0

    |  Configuration 2 Register (XPT0_CONFIG2)

    +------------------------------------------------------------------------*/

    unsigned ved         : 1;               /* 1=vid unloader clr errs,0=snd */

    unsigned acpm        : 1;               /* 1=normal, 0=ignore request    */

    unsigned vcpm        : 1;               /* 1=normal, 0=ignore request    */

    unsigned mwe         : 1;               /* 1=match word enabled,  0=disa */

    unsigned salign      : 1;               /* 1=table sect wrd align,0=byte */

    unsigned atsed       : 1;               /* 1=aud TS errs disabled, 0=ena */

    unsigned atbd        : 1;               /* 1=aud time base disable,0=ena */

    unsigned accd        : 1;               /* 1=aud chan chg disabled,0=ena */

    unsigned vtsed       : 1;               /* 1=vid TS errs disabled, 0=ena */

    unsigned vtbd        : 1;               /* 1=vid time base disable,0=ena */

    unsigned vccd        : 1;               /* 1=vid chan chg disabled,0=ena */



    /*------------------------------------------------------------------------+

    |  For additional information on the following bit definitions, refer to

    |  the Digital Set-Top Box Integrated Controller User's Guide Transport 0

    |  Configuration 3 Register (XPT0_CONFIG3)

    +------------------------------------------------------------------------*/

    unsigned insel       : 2;               /* Input Selet                   */

                                            /* 00=CI0, 01=CI1, 10=1394       */



    /*------------------------------------------------------------------------+

    |  For additional information on the following bit definitions, refer to

    |  the Digital Set-Top Box Integrated Controller User's Guide Transport 0

    |  Control 1 Register (XPT0_CONTROL1).

    +------------------------------------------------------------------------*/

    unsigned sbe         : 1;               /* 1=bypass, 0=scan for 0x47     */

    unsigned pbe         : 1;               /* 1=parser bypass enab, 0=disab */

    unsigned senbl       : 1;               /* 1=bit mode, 0=byte mode       */

    unsigned sdop        : 1;               /* 1=disable packet sync,0=enab  */



    /*------------------------------------------------------------------------+

    |  For additional information on the following bit definitions, refer to

    |  the Digital Set-Top Box Integrated Controller User's Guide Transport 0

    |  Packet Buffer Level Register (XPT0_PBUFLVL)

    +------------------------------------------------------------------------*/

    unsigned qpthres     : 4;               /* queue packet threshold        */

    unsigned apthres     : 4;               /* audio packet threshold        */

    unsigned vpthres     : 4;               /* video packet threshold        */

} XP_CONFIG_VALUES;



/*---------------------------------------------------------------------------+

| Prototype Definitions

+----------------------------------------------------------------------------*/

void xp_osi_get_config_values (GLOBAL_RESOURCES *pGlobal,XP_CONFIG_VALUES *configv_parm);

short xp_osi_init(GLOBAL_RESOURCES *pGlobal);

void xp_osi_set_config_values (GLOBAL_RESOURCES *pGlobal, XP_CONFIG_VALUES *configv_parm);

void xp_osi_start_parse_bypass(GLOBAL_RESOURCES *pGlobal);

void xp_osi_stop_parse_bypass(GLOBAL_RESOURCES *pGlobal);

void xp_osi_set_parse_bypass_mode(GLOBAL_RESOURCES *pGlobal);

int xp_osi_set_vcpm(GLOBAL_RESOURCES *pGlobal, int mode);

int xp_osi_set_acpm(GLOBAL_RESOURCES *pGlobal, int mode);

#endif

