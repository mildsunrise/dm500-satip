//vulcan/drv/gfx/osd_atom.h
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
|       COPYRIGHT   I B M   CORPORATION 1998
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
//
//Comment: 
//  Atom function of OSD controls 
//Revision Log:   
//  Sept/18/2001                          Created by YYD
//  Oct/21/2002                           Added OSD_CNTL_CHFSR to STB_OSD_DISPLAY_CONTROL_T
//                                        for setting custom horizontal scaling ratio.  BJC

#ifndef  _DRV_INCLUDE_GFX_OSD_ATOM_H_INC_
#define  _DRV_INCLUDE_GFX_OSD_ATOM_H_INC_

#include "os/os-types.h"
#include "gfx_surface.h"
#include "osd_dcr.h"

#define OSD_ADDRESS_ALIGNMENT   (128)      // address on 128 byte boundary
#define OSD_LINK_ALIGNMENT      (32)       // link address on 32 byte boundary
#define OSD_BUFFER_ALIGNMENT    (4096)     // Buffer address on PAGESIZE byte boundary
#define OSD_BMLA_ALIGNMENT      (4)        // BMLA address on 4 byte boundary

// to distinguish field applicables
// z/Z  : not used
// c/C  : cursor plane
// g/G  : graphics plane
// s/S  : still plane
// b/B  : back ground plane
// a/A  : all plane

//  -------------------  PACKED -----------------------
#ifdef __GNUC__
#pragma pack(1)
#endif

// the following is defination of osd control block of stbx25xx 
typedef struct __STB_OSD_GI_CONTROL_BLOCK_T__ {     // use with BMLA = 1
	// basic osd hdr, 64 bits
	unsigned	color_table_update  : 1;    // 
	unsigned	region_hsize        : 8;    // we always use 8 bit color mode, so is high_color mode
	unsigned	shade_level	        : 4;
	unsigned	high_color          : 1;    // always 1 for 256 color mode
	unsigned	start_row           : 9;   
	unsigned	start_column        : 9;
	unsigned	link_addr           : 16;
	unsigned	color_resolution    : 1;    // 0 for 256 color mode and 1 for yuv direct color mode
	unsigned	region_vsize        : 9;
	unsigned	pixel_resolution    : 1;    // set to 0 for 256 and yuv422, 1 for yuv420
	unsigned	blend_level_g       : 4;    // blend level for graphics only
	unsigned	force_transparency  : 1;

	// extended hdr 1 
	unsigned	extlink_addr        :19;
	unsigned	extlink_addr_lsb    :4;
	unsigned	hsb_ext             :2;
	unsigned	h_ext_2             :1;  // set to 1 
	unsigned	dcus                :1;
	unsigned	shade_ext           :2;
	unsigned	dcub                :1;
	unsigned	blend_ext           :2;
	
	// extended hdr 2
	unsigned	h_fir_scaling       :4;
	unsigned	titling_ctrl        :2;
	unsigned	anti_flicker        :2;
	unsigned	zres1               :1;
	unsigned	color_s_blend       :1;
	unsigned	h_ext_3             :1;     // set to 0
	unsigned	zres2               :1;
	unsigned	chroma_lnk_en       :1;
	unsigned	chroma_lnk          :19;

    // palette section   512 unsigned chars
	// unsigned short       palette[256];
    // bitmap data should follow here
}  STB_OSD_GI_CONTROL_BLOCK_T;

typedef struct __STB_OSD_GI_PALETTE_T__
{
        unsigned    y   : 6;        // 6 bit y
        unsigned    cb  : 4;        // 4 bit cb
        unsigned    cr  : 4;        // 4 bit cr
        unsigned    bf1 : 1;        // 4 level blend factor
        unsigned    bf0 : 1;        // 4 level blend factor
} STB_OSD_GI_PALETTE_T;



typedef struct __STB_OSD_CURSOR_CONTROL_BLOCK_STRUCTURE__   // use with BMLA = 1
{
	// basic osd hdr, 64 bits
	unsigned	zres1               : 4;    // 
	unsigned	region_hsize        : 4;    // 
	unsigned	zres2	            : 6;
	unsigned	start_row           : 9;   
	unsigned	start_column        : 9;
	unsigned	start_row_odd       : 1;
	unsigned	start_column_odd    : 1;  
	unsigned	region_vsize_odd    : 1;
    unsigned    zres3               : 13;
	unsigned	color_resolution    : 1;    // 0 for 16 color mode and 1 for 4 color mode
    unsigned    zres4               : 2;
	unsigned	region_vsize        : 7;
	unsigned	pixel_resolution    : 1;    // set to 0 for 256 and yuv422, 1 for yuv420
    unsigned    zres5               : 1;
    unsigned    anti_flicker        : 2;    
    unsigned    zres6               : 1;
	unsigned	force_transparency  : 1;

	// extended hdr 1 
	unsigned	extlink_addr        :19;
	unsigned	zres7               :13;

} STB_OSD_CURSOR_CONTROL_BLOCK_T;

typedef  struct __STB_OSD_C_PALETTE_T__
{
    unsigned    y   : 6;        // 6 bit y
    unsigned    cb  : 4;        // 4 bit cb
    unsigned    cr  : 4;        // 4 bit cr
    unsigned    steady : 1;     
    unsigned    peep   : 1;     
} STB_OSD_C_PALETTE_T;


#ifdef __GNUC__
#pragma pack()
#endif
//  -------------------  UNPACKED ---------------------


typedef enum
{
    OSD_CNTL_AFVP,      // Anti-flicker video plane, 0 disable, 1 enable
    OSD_CNTL_EDAF,      // Enable display anti-flicker, 0 disable, 1 enable
    OSD_CNTL_AFDT,      // Anti-flicker detection threshold, 2 bits attr
    OSD_CNTL_VPAFC,     // Video plane anti-flicker correction, 2 bits attr
    OSD_CNTL_E32BCO,    // Enable 32bit color option, 0 no, 1 yes
    OSD_CNTL_ANIM,      // Animation mode, 0 no, 1 yes
    OSD_CNTL_ANIMR,     // Animation rate, 3 bits attr
    OSD_CNTL_BACKCOLOR, // backgound layer color
    OSD_CNTL_CHFSR      // Custom horizontal finite scaling ratio, 9 bits attr -- BJC 102102
} STB_OSD_DISPLAY_CONTROL_T;


int osd_atom_set_display_control(STB_OSD_DISPLAY_CONTROL_T cntl, UINT uAttr);
UINT osd_atom_get_display_control(STB_OSD_DISPLAY_CONTROL_T cntl);

typedef enum
{
    OSD_PLANE_A_ENABLE,                 // R/W enable plane, OSD_PLANE_GRAPHICS | OSD_PLANE_CURSOR | OSD_PLANE_IMAGE
    OSD_PLANE_A_DISABLE,                // W   disable plane, OSD_PLANE_GRAPHICS | OSD_PLANE_CURSOR | OSD_PLANE_IMAGE
    OSD_PLANE_A_BMLA,                   // R/W enable/disable BMLA mode, OSD_PLANE_GRAPHICS | OSD_PLANE_CURSOR | OSD_PLANE_IMAGE
    OSD_PLANE_A_422_AVERAGING,          // R/W enable/disable 444 to 422 conversion chroma averaging of CG plane, 0 disable/ 1 enable
    OSD_PLANE_G_PER_COLOR_BLEND_MODE,   // R/W enable/disable per color blend mode, OSD_PLANE_GRAPHICS
} STB_OSD_PLANE_CONTROL_T;

int osd_atom_set_plane_control(STB_OSD_PLANE_CONTROL_T cntl, UINT uAttr);
UINT osd_atom_get_plane_control(STB_OSD_PLANE_CONTROL_T cntl, UINT uPlane);

typedef enum
{
    OSD_ADDR_CURSOR_BASE    = DCR_VID0_CPBASE,
    OSD_ADDR_CURSOR_LINK    = DCR_VID0_CSLA,
    OSD_ADDR_GRAPHICS_BASE  = DCR_VID0_GPBASE,
    OSD_ADDR_GRAPHICS_LINK  = DCR_VID0_GSLA,
    OSD_ADDR_IMAGE_BASE     = DCR_VID0_IPBASE,
    OSD_ADDR_IMAGE_LINK     = DCR_VID0_ISLA,
} STB_OSD_BASE_ADDR_T;

int osd_atom_set_base_address(STB_OSD_BASE_ADDR_T index, UINT uAddr);


#endif // _DRV_INCLUDE_GFX_OSD_ATOM_H_INC_
