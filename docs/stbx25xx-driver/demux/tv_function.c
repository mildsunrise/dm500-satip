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



/****************************************************************************/

/*                                                                          */

/*    DESCRIPTION :  A demo to test demux, audio and video

/****************************************************************************/



/****************************************************************************/

/*                                                                          */

/*  Author    :  Lin Guo Hui                                            */

/*  File      :                                                            */

/*  Purpose   :  Test application                                       */

/*  Changes   :                                                             */

/*  Date         Comments                                                   */

/*  ----------------------------------------------------------------------  */

/*  25-Jun-2001    Created                                                  */

/*  12-Jul-2001    Add Network ID recognition, When Filter PMT, add the     */

/*         Program Number in the filter parameters.                         */

/*  25-Oct-2001    seperated from test_av                                   */

/*  23-Jan-2002     fix the bug of program_number                           */

/*                                                                          */

/****************************************************************************/

#include <sys/ioctl.h>



#include <stdio.h>

#include <stdint.h>

#include <sys/types.h>

#include <sys/stat.h>

#include <fcntl.h>

#include <time.h>

#include <sys/poll.h>



#include <xp/xp_osd_user.h>

#include <app/tv_function.h>


unsigned char smap_global[4096];


int get_program_count(int fd_pat)

{

    int len = 0;

    int i;



    int ProgramCount = 0;

    int length;



    int temp;

    int pp;

    int fd_pat1;



    unsigned char pat_sec[4096];

    filter_para FilterPara;



    PAT_PTR pat_filter;

    PAT_PTR pat_mask;

    PAT_PTR pat;

    PAT_MAP_PTR smap;


//set the PAT filter parameters

    pat_filter = (PAT_PTR) malloc(sizeof(PAT_TYPE));

    pat_mask = (PAT_PTR) malloc(sizeof(PAT_TYPE));



    memset(pat_filter, 0, sizeof(PAT_TYPE));

    memset(pat_mask, 0, sizeof(PAT_TYPE));



    pat_filter->table_id = 0;

    pat_mask->table_id = ~0;



    memset(&FilterPara,0,sizeof(FilterPara));


    memcpy(FilterPara.filter, (unsigned char *) pat_filter, sizeof(PAT_TYPE));

    memcpy(FilterPara.mask, (unsigned char *) pat_mask, sizeof(PAT_TYPE));



    FilterPara.pid = 0;

    FilterPara.filter_length = 6;



//set pat filter

    if (ioctl(fd_pat, DEMUX_FILTER_SET, &FilterPara) < 0)

        printf("Error set Sec Filter\n");



//start receving PAT section

    if (ioctl(fd_pat, DEMUX_START) < 0)

        printf("Error Start Filter\n");


    len = 0;

    ProgramCount = 0;

    while (!len && !ProgramCount)

    {

        len = read(fd_pat, pat_sec, 4096);

        pat = (PAT_PTR) pat_sec;



        //if PAT received is not completed, think it as illegal

        if (pat->sectionLength + LENGTH_TABLE_HEADER > len)

            len = 0;



        //length of the information not related to PAT-map

        length =

            sizeof(PAT_TYPE) + LENGTH_TABLE_CRC - LENGTH_TABLE_HEADER -

            sizeof(pat->map);



        if (length < pat->sectionLength)

        {

            //hoe many PMT is?

            ProgramCount =

                (pat->sectionLength - length) / sizeof(PAT_MAP_TYPE);

        }



        else

        {

            printf("Error receive PAT\n");

            len = 0;

        }

    }


    memcpy(smap_global,&pat->map[0],sizeof(pat->map[0])*ProgramCount);

    pat = (PAT_PTR) pat_sec;

    if (ProgramCount > 0)

    {

        pp =0;

        smap = &pat->map[0];



        if (pat->sectionNumber == pat->lastSectionNumber)

        {

            if (ioctl(fd_pat, DEMUX_STOP) < 0)

                printf("Error Start Filter\n");

        }



        if(smap[0].programNumber_hi==0 && smap[0].programNumber_lo==0)

        {

            printf("The Network_PID is %4x\n",smap[0].pid);

            ProgramCount--;

            pp++;

        }



        printf("There are %d programs in this Channel\n", ProgramCount);



        for (i = pp; i <= ProgramCount; i++)

        {

            printf("TS: PMET PID: %04x\n", smap[i].pid);

        }

    }


    return ProgramCount;

}



int start_program(int fd_pmt, int fd_a, int fd_v, int fd_pcr, int uProgNumber, int flag)

{

    unsigned short video_pid;

    unsigned short audio_pid;

    unsigned short pcr_pid;



    unsigned char pmt_sec[4096];



    PMT_PTR pmt;

    PMT_MAP_PTR map;



    unsigned char *s_addr;

    unsigned short size;

    unsigned long info_length;



    filter_para FilterPara;

    Pes_para PesPara;

    PAT_MAP_PTR smap = (PAT_MAP_PTR)smap_global;



    int length=0,len=0;



    memset(&FilterPara,0,sizeof(FilterPara));


       if(smap[0].programNumber_hi==0 && smap[0].programNumber_lo==0)

       {

    uProgNumber++;

        }





    FilterPara.pid = smap[uProgNumber].pid;

    FilterPara.filter[0] = 0x02;

    FilterPara.filter[3] = smap[uProgNumber].programNumber_hi;

    FilterPara.filter[4] = smap[uProgNumber].programNumber_lo;



    FilterPara.mask[0] = 0xff;

    FilterPara.mask[3] = 0xff;

    FilterPara.mask[4] = 0xff;



    FilterPara.filter_length = 6;



    if (ioctl(fd_pmt, DEMUX_FILTER_SET, &FilterPara) < 0)

        printf("Error set Sec Filter\n");



    if (ioctl(fd_pmt, DEMUX_START) < 0)

        printf("Error Start Filter\n");



    len = 0;

    audio_pid = -1;

    video_pid = -1;

    pcr_pid = -1;



    while (!len)

    {

        len = read(fd_pmt, pmt_sec, 4096);



        pmt = (PMT_PTR) pmt_sec;



        length =

            sizeof(PMT_TYPE) + LENGTH_TABLE_CRC +

            pmt->program_info_length - LENGTH_TABLE_HEADER;



        if (length > pmt->sectionLength)

        {

            len = 0;

        }

    }



    length = pmt->sectionLength - length;



    pcr_pid = pmt->pcr_pid;

    s_addr =

        ((unsigned char *) pmt_sec) + sizeof(PMT_TYPE) +

        pmt->program_info_length;



    while (length > 0)

    {

        map = (PMT_MAP_PTR) s_addr;

        switch (map->streamType)

        {

        case 1:

        case 2:

                        if(video_pid == 0xffff)
              video_pid = map->pid;

            break;

        case 3:

        case 4:

                        if(audio_pid == 0xffff)
                          audio_pid = map->pid;

            break;

        default:

            break;

        }



        info_length = (map->info_len1 << 8) + map->info_len2;

        size = (SIZEOF_PMT_MAP_TYPE + info_length);



        if (length < size)

            break;



        length -= size;

        s_addr += size;

    }



    if (ioctl(fd_pmt, DEMUX_STOP) < 0)

        printf("Error Start Filter\n");



    printf("In program: %d .......................\n", uProgNumber);

    printf("Video: %x\n", video_pid);

    printf("Audio: %x\n", audio_pid);

    printf("PCR  : %x\n", pcr_pid);



    PesPara.pid = video_pid;



    if(flag == PLAY_TO_DECODE)

        PesPara.output = OUT_DECODER;

    else if(flag == PLAY_TO_MEMORY)

    {

        PesPara.output = OUT_MEMORY;

        if (ioctl(fd_v, DEMUX_SET_BUFFER_SIZE, 150*256) < 0)

            printf("Error set video buffer size\n");

        PesPara.unloader.threshold = 128;

        PesPara.unloader.unloader_type = UNLOADER_TYPE_PAYLOAD;

    }



    PesPara.pesType = DMX_PES_VIDEO;



    if (ioctl(fd_v, DEMUX_FILTER_PES_SET, &PesPara) < 0)

        printf("Error set Sec Filter\n");



    if (ioctl(fd_v, DEMUX_START) < 0)

        printf("Error Start Filter\n");



    PesPara.pid = audio_pid;



    if(flag == PLAY_TO_DECODE)

        PesPara.output = OUT_DECODER;

    else if(flag == PLAY_TO_MEMORY)

    {

        PesPara.output = OUT_MEMORY;

        if (ioctl(fd_a, DEMUX_SET_BUFFER_SIZE, 40*256) < 0)

            printf("Error set audio buffer size\n");



        PesPara.unloader.threshold = 32;

        PesPara.unloader.unloader_type = UNLOADER_TYPE_PAYLOAD;



    }



    PesPara.pesType = DMX_PES_AUDIO;



    if (ioctl(fd_a, DEMUX_FILTER_PES_SET, &PesPara) < 0)

        printf("Error set Sec Filter\n");



    if (ioctl(fd_a, DEMUX_START) < 0)

        printf("Error Start Filter\n");



    PesPara.pid = pcr_pid;



    if(flag == PLAY_TO_DECODE)

        PesPara.output = OUT_DECODER;

    else if(flag == PLAY_TO_MEMORY)

        PesPara.output = OUT_MEMORY;



    PesPara.pesType = DMX_PES_PCR;



    if (ioctl(fd_pcr, DEMUX_FILTER_PES_SET, &PesPara) < 0)

        printf("Error set Sec Filter\n");



    if (ioctl(fd_pcr, DEMUX_START) < 0)

        printf("Error Start Filter\n");


    return(0);
}



