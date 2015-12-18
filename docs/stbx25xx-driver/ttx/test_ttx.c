/*----------------------------------------------------------------------------+
|     This source code has been made available to you by IBM on an AS-IS
|     basis.  Anyone receiving this source is licensed under IBM
|     copyrights to use it in any way he or she deems fit, including
|     copying it, modifying it, compiling it, and redistributing it either
|     with or without modifications.  No license under IBM patents or
|     patent applications is to be implied by the copyright license.
|
|     Any user of this software should understand that IBM cannot provide
|     technical support for this software and will not be responsible for
|     any consequences resulting from the use of this software.
|
|     Any person who transfers this source code or any derivative work
|     must include the IBM copyright notice, this paragraph, and the
|     preceding two paragraphs in the transferred software.
|
|       IBM CONFIDENTIAL
|       STB025XX VXWORKS EVALUATION KIT SOFTWARE
|       (C) COPYRIGHT IBM CORPORATION 2003
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| Author    :  Tony J. Cerreto
| Component :  ttx
| File      :  ttx_drv.c
| Purpose   :  Teletext Driver
| Changes   :
|
| Date:      By   Comment:
| ---------  ---  --------
| 30-Sep-03  TJC  Modified
+----------------------------------------------------------------------------*/
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <pthread.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/poll.h>
#include <ttx/ttx_osd_user.h>
#include <vid/vid_inf.h>
#include <xp/xp_osd_user.h>

/*----------------------------------------------------------------------------+
| Defines
+----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------+
| Static Declarations
+----------------------------------------------------------------------------*/
static int fd_pcr0     = 0;
static int fd_xp0      = 0;
static int fd_ttx0     = 0;
static int ttx_inited  = 0;
static int ttx_started = 0;
static int ttx_loop    = 1;
static int ttx_nullp   = 0;

static unsigned char *mu_ttx_help[] = {
         "t h[elp]            - TTX help menu      ",
         "t init              - TTX initialize     ",
         "t term              - TTX terminate      ",
         "t start <ppid tpid> - TTX start          ",
         "t stat  <get|clear> - TTX statistics     ",
         "t stop              - TTX stop           ",
         "t q[uit]            - Quit application   ",
         0,
};

/*----------------------------------------------------------------------------+
|  XXXXXX    XX     XXXXX   XX   XX   XXXXX
|  X XX X   XXXX   XX   XX  XX  XX   XX   XX
|    XX    XX  XX   XX      XX XX     XX
|    XX    XX  XX     XX    XXXX        XX
|    XX    XXXXXX      XX   XX XX        XX
|    XX    XX  XX  XX   XX  XX  XX   XX   XX
|   XXXX   XX  XX   XXXXX   XX   XX   XXXXX
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| ttx_rw_task
+----------------------------------------------------------------------------*/
static int ttx_rw_task(void *dummy)

{
  int           rc;
  int           ttx_len;
  unsigned char ttx_buffer[188*12];


  ttx_len    = 0;

  while(ttx_started) {
    ttx_len = read(fd_xp0, ttx_buffer, 188*12);

    if (ttx_len > 0) {
       if ((rc = write(fd_ttx0, ttx_buffer, ttx_len)) != 0) {
          printf("Error writing TTX packets to Teletext driver\n");
       }
    } else if (ttx_len < 0) {
       ttx_nullp = ttx_nullp  + 1;
    }

  }

}

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
| ttx_get_stats
+----------------------------------------------------------------------------*/
static int ttx_get_stats()

{
  TTX_STATISTICS    tstats;

  /*--------------------------------------------------------------------------+
  | Verify TTX is in proper state
  +--------------------------------------------------------------------------*/
  if (!ttx_inited) {
     printf("TTX Driver must be initialized to display statistics\n");
     return(-1);
  }

  /*--------------------------------------------------------------------------+
  | Display TTX Statistics
  +--------------------------------------------------------------------------*/
  ioctl(fd_ttx0, TTX_STATS_GET, &tstats);

  printf("T T X  S T A T I S T I C S\n");
  printf("-----  -------------------\n");
  printf("Total incoming packets received:            %d\n", tstats.in_pkt);
  printf("TTX Packets processed with PUSI:            %d\n", tstats.in_pusi);
  printf("TTX Packets processed without PUSI:         %d\n", tstats.no_pusi);
  printf("TTX Buffers queued for transmission:        %d\n", tstats.out_pusi);
  printf("TTX Buffers transmitted:                    %d\n", tstats.inbound_pes);
  printf("TTX Packet read attempts with no data:      %d\n", ttx_nullp);
  printf("TTX Packets discarded due to STC mismatch:  %d\n", tstats.outbound_pes);
  printf("TTX Packets discarded due to stream errors: %d\n", tstats.trash_pkt);
  printf("\n");

  return(0);
}

/*----------------------------------------------------------------------------+
| ttx_clr_stats
+----------------------------------------------------------------------------*/
static int ttx_clr_stats()

{

  /*--------------------------------------------------------------------------+
  | Verify TTX is in proper state
  +--------------------------------------------------------------------------*/
  if (!ttx_inited) {
     printf("TTX Driver must be initialized to clear statistics\n");
     return(-1);
  }

  /*--------------------------------------------------------------------------+
  | Clear TTX Statistics
  +--------------------------------------------------------------------------*/
  ioctl(fd_ttx0, TTX_STATS_CLEAR);
  ttx_nullp = 0;

  return(0);
}

/*----------------------------------------------------------------------------+
| ttx_stop
+----------------------------------------------------------------------------*/
static int ttx_stop()
{

  char          key;
  int           ttx_file;
  int           ttx_len, tot_len;
  int           tflag;
  unsigned char ttx_buffer[1024];
  Pes_para      PesPara;


  /*--------------------------------------------------------------------------+
  | Verify TTX is in proper state
  +--------------------------------------------------------------------------*/
  if (!ttx_started) {
     return(0);
  }

  /*--------------------------------------------------------------------------+
  | Stop Demux 0 Filter to Acquire TTX Data
  +--------------------------------------------------------------------------*/
  if (ioctl(fd_xp0, DEMUX_STOP) < 0) {
    printf("Error stopping Demux Teletext Filter\n");
    return(-1);
  }

  /*--------------------------------------------------------------------------+
  | Stop the PCR PID
  +--------------------------------------------------------------------------*/
    if (ioctl(fd_pcr0, DEMUX_STOP) < 0) {
     printf("Error stopping Demux PCR Filter\n");
     return(-1);
  }

  /*--------------------------------------------------------------------------+
  | Stop TTX
  +--------------------------------------------------------------------------*/
  if (ioctl(fd_ttx0, TTX_STOP) < 0) {
    printf("Error stopping Teletext Driver\n");
    return(-1);
  }

  ttx_started = 0;
  return(0);
}

/*----------------------------------------------------------------------------+
| ttx_start
+----------------------------------------------------------------------------*/
static int ttx_start(int pcrpid, int txtpid)
{

  int                rc;
  Pes_para           PesPara;
  pthread_attr_t     attr;
  pthread_t          thread;
  struct sched_param sched;


  /*--------------------------------------------------------------------------+
  | Verify TTX is in proper state
  +--------------------------------------------------------------------------*/
  if (!ttx_inited) {
     printf("TTX Driver must be initialized before re-started\n");
     return(-1);
  }

  if (ttx_started) {
     printf("TTX Driver must be stopped before started\n");
     return(-1);
  }

  /*--------------------------------------------------------------------------+
  | Start TTX
  +--------------------------------------------------------------------------*/
  if (ioctl(fd_ttx0, TTX_START, txtpid) < 0) {
    printf("Error starting Teletext Driver\n");
    return(-1);
  }

  /*--------------------------------------------------------------------------+
  | Set PCR PID
  +--------------------------------------------------------------------------*/
  PesPara.pid     = pcrpid;
  PesPara.output  = OUT_DECODER;
  PesPara.pesType = DMX_PES_PCR;

  if (ioctl(fd_pcr0, DEMUX_FILTER_PES_SET, &PesPara) < 0) {
     printf("Error setting Demux PCR Filter\n");
     return(-1);
  }

  if (ioctl(fd_pcr0, DEMUX_START) < 0) {
     printf("Error starting Demux PCR Filter\n");
     return(-1);
  }

  /*--------------------------------------------------------------------------+
  | Start Demux 0 Filter to Acquire TTX Data
  +--------------------------------------------------------------------------*/
  PesPara.pid                    = txtpid;
  PesPara.output                 = OUT_MEMORY;
  PesPara.pesType                = DMX_PES_TELETEXT;
  PesPara.unloader.threshold     = 2;
  PesPara.unloader.unloader_type = UNLOADER_TYPE_TRANSPORT;

  if (ioctl(fd_xp0, DEMUX_FILTER_PES_SET, &PesPara) < 0) {
    printf("Error setting Demux Teletext Filter\n");
    return(-1);
  }

  if (ioctl(fd_xp0, DEMUX_START) < 0) {
    printf("Error starting Demux Teletext Filter\n");
    return(-1);
  }

  /*--------------------------------------------------------------------------+
  | Start TTX Receive and Transmit Task
  +--------------------------------------------------------------------------*/
  ttx_started = 1;
  if ((rc = pthread_attr_init(&attr)) != 0) {
    printf("Error initializing Teletext Receive Task\n");
    return(rc);
  }

  if ((rc = pthread_attr_setschedpolicy(&attr, SCHED_RR)) != 0) {
    printf("Error setting scheduler policy for Teletext Receive Task\n");
    return(rc);
  }

  if ((rc = pthread_create(&thread,(void *)&attr,
     (void *)ttx_rw_task,(void *)NULL)) != 0) {
    printf("Error starting Teletext Receive Task\n");
    return(rc);
  }

  return(0);
}

/*----------------------------------------------------------------------------+
| ttx_close
+----------------------------------------------------------------------------*/
static int ttx_close()
{
  int   rc;


  /*--------------------------------------------------------------------------+
  | Stop Teletext processing if active
  +--------------------------------------------------------------------------*/
  if (ttx_started) {
     if ((rc = ttx_stop()) != 0) {
        printf("ttx_stop() failed to stop Teletext driver\n");
        return(rc);
     }
  }

  /*--------------------------------------------------------------------------+
  | Close all Devices
  +--------------------------------------------------------------------------*/
  if (fd_ttx0 != 0) {
     if ((rc = close(fd_ttx0)) < 0) {
        printf("Error closing Teletext driver (/dev/ttx0)\n");
        return(rc);
     }
     fd_ttx0 = 0;
  }

  if (fd_xp0 != 0) {
     if ((rc = close(fd_xp0)) < 0) {
        printf ("Error closing Demux TTX Filter (/dev/demuxapi0/xp0)\n");
        return(rc);
     }
     fd_xp0 = 0;
  }

  if (fd_pcr0 != 0) {
     if ((rc = close(fd_pcr0)) < 0) {
        printf ("Error closing Demux PCR Filter (/dev/demuxapi0/pcr0)\n");
        return(rc);
     }
     fd_pcr0 = 0;
  }

  ttx_inited = 0;
  return(0);
}

/*----------------------------------------------------------------------------+
| ttx_open
+----------------------------------------------------------------------------*/
static int ttx_open()
{
  int  rc;
  int  fd_vid;

  /*--------------------------------------------------------------------------+
  | Close any devices still open
  +--------------------------------------------------------------------------*/
  if ((fd_pcr0 != 0)  ||
      (fd_xp0  != 0)  ||
      (fd_ttx0 != 0)) {
     ttx_close();
  }

  /*--------------------------------------------------------------------------+
  | Open Demux 0 PCR Filter
  +--------------------------------------------------------------------------*/
  if ((fd_pcr0 = open("/dev/demuxapi0", O_RDONLY)) < 0) {
     printf ("Error opening Demux PCR Filter (/dev/demuxapi0/pcr0)\n");
     return(fd_pcr0);
  }

  /*--------------------------------------------------------------------------+
  | Open Demux 0 Channel Filter
  +--------------------------------------------------------------------------*/
  if ((fd_xp0 = open("/dev/demuxapi0", O_RDONLY)) < 0) {
     printf ("Error opening Demux TTX Filter (/dev/demuxapi0/xp0)\n");
     return(fd_xp0);
  }

  /*--------------------------------------------------------------------------+
  | Open Video Driver and Set Video Decoder to PAL
  +--------------------------------------------------------------------------*/
  if ((fd_vid = open("/dev/vdec_dev", O_RDWR)) < 0) {
     printf("Unable to open video driver (/dev/vdec_dev)\n");
     printf("Assuming video driver is already opened and set to PAL Mode\n");
  } else {
     if ((rc = ioctl(fd_vid,MPEG_VID_SET_DISPFMT,VID_DISPFMT_PAL)) < 0) {
        printf("Error setting video display format to PAL\n");
        return(rc);
     }
     if ((rc = close(fd_vid)) < 0) {
        printf("Error closing video driver (/dev/vdec_dev)\n");
        return(rc);
     }
  }

  /*--------------------------------------------------------------------------+
  | Open TTX Driver
  +--------------------------------------------------------------------------*/
  if ((fd_ttx0 = open("/dev/ttx0", O_RDWR)) < 0) {
     printf ("Error opening Teletext Driver (/dev/ttx0)\n");
     return(fd_ttx0);
  }

  ttx_inited = 1;
  ttx_clr_stats();

  return(0);
}

/*----------------------------------------------------------------------------+
| ttx_help_menu
+----------------------------------------------------------------------------*/
static void ttx_help_menu(void)

{
   int n;


   printf("-Teletext Menu-------------------------");
   printf("---------------------------------------\n\r");

   for (n=0; mu_ttx_help[n]!=0; n++) {
     if (n % 2)
        printf("%-39s\n",mu_ttx_help[n]);
     else
        printf("%-39s|", mu_ttx_help[n]);
   }

   if (n%2) {
      printf("\n");
   }

   printf("---------------------------------------");
   printf("---------------------------------------\n\r");

   return;
}

/*----------------------------------------------------------------------------+
| ttx_test
+----------------------------------------------------------------------------*/
static int ttx_test(char *cmd)
{

  char parm[80];
  int  pcrpid, txtpid;
  int  err;


  /*--------------------------------------------------------------------------+
  | t help
  +--------------------------------------------------------------------------*/
  if (strncmp(cmd, "h", 1)    ==0  ||
      strncmp(cmd, "help", 4) ==0) {
     ttx_help_menu();
  }

  /*--------------------------------------------------------------------------+
  | t init
  +--------------------------------------------------------------------------*/
  else if (strncmp(cmd, "init", 4)==0) {
     if ((err = ttx_open()) != 0) {
        printf("ttx_open() failed.  rc=%d\n\r", err);
     } else {
        printf("Teletext Driver initialized sucessfully\n");
     }
  }

  /*--------------------------------------------------------------------------+
  | t term
  +--------------------------------------------------------------------------*/
  else if (strncmp(cmd, "term", 4)==0) {
     if ((err = ttx_close()) != 0) {
        printf("ttx_close() failed.  rc=%d\n\r", err);
     } else {
        printf("Teletext Driver terminated sucessfully\n");
     }
  }

  /*--------------------------------------------------------------------------+
  | t start
  +--------------------------------------------------------------------------*/
  else if (strncmp(cmd, "start", 5)==0) {
     if (sscanf(&cmd[5], "%x %x", &pcrpid, &txtpid) != 2) {
         printf("Incorrect number of arguments was specified\n\r");
         printf("Usage: t start <pcrpid txtpid>  ie. t start 0x1ffe 0x240\n");
     } else {
         if ((err = ttx_start(pcrpid,txtpid)) != 0) {
            printf("ttx_start() failed. rc=%d\n\r", err);
         } else {
            printf("Teletext Driver started, PCR PID=0x%04x, TTX PID=0x%04x\n",
                   pcrpid,txtpid);
         }
     }
  }

  /*--------------------------------------------------------------------------+
  | t stop
  +--------------------------------------------------------------------------*/
  else if (strncmp(cmd, "stop", 4)==0) {
     if ((err = ttx_stop()) != 0) {
        printf("ttx_stop() failed. rc=%d\n\r", err);
     } else {
        printf("Teletext Driver stopped\n");
     }
  }

  /*--------------------------------------------------------------------------+
  | t stat
  +--------------------------------------------------------------------------*/
  else if (strncmp(cmd, "stat", 4)==0) {
     if (sscanf(&cmd[5], "%s", parm) != 1) {
         printf("Incorrect number of arguments was specified\n\r");
         printf("Usage: t stat <get | clear>  ie. t stat clear\n");
     } else {
        if (strcmp(parm, "clear") == 0) {
           if ((err = ttx_clr_stats()) != 0) {
              printf("ttx_clr_stats() failed. rc=%d\n\r", err);
           } else {
              printf("Teletext statistics cleared\n");
           }
        } else if (strcmp(parm, "get") == 0) {
           if ((err = ttx_get_stats()) != 0) {
              printf("ttx_get_stats() failed. rc=%d\n\r", err);
           }
        } else {
           printf("Usage: t stat <get | clear>  ie. t stat clear\n");
        }
     }
  }

  /*--------------------------------------------------------------------------+
  | t quit
  +--------------------------------------------------------------------------*/
  else if (strncmp(cmd, "q",    1) ==0  ||
           strncmp(cmd, "quit", 4) ==0) {
          ttx_loop = 0;
  }

  /*--------------------------------------------------------------------------+
  | Unknown Command
  +--------------------------------------------------------------------------*/
  else {
     printf("Unknown TTX Command\n\r");
     return(-1);
  }

  return(0);
}

/*----------------------------------------------------------------------------+
|  XX      XX    XX    XXXX  XX   XX
|  XXX    XXX   XXXX    XX   XXX  XX
|  XXXX  XXXX  XX  XX   XX   XXXX XX
|  XX  XX  XX  XX  XX   XX   XX XXXX
|  XX  XX  XX  XXXXXX   XX   XX  XXX
|  XX      XX  XX  XX   XX   XX   XX
|  XX      XX  XX  XX  XXXX  XX   XX
+----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------+
| main
+----------------------------------------------------------------------------*/
int main()
{
  char inp[40];


 /*---------------------------------------------------------------------------+
 | Process all Teletext commands
 +---------------------------------------------------------------------------*/
  while(ttx_loop) {
    printf("TTX Input Command (h=help):");
    fflush(0);

    memset(inp,' ',sizeof(inp));
    read(0,inp,sizeof(inp));
    printf("\n\r");

    /*------------------------------------------------------------------------+
    | Teletext Menu
    +------------------------------------------------------------------------*/
    if (inp[0]=='t') {
       ttx_test(&inp[strspn(&inp[1], " ")]+ 1);
    }
  }

  /*---------------------------------------------------------------------------+
  | Close Teletext Device
  +---------------------------------------------------------------------------*/
  ttx_close();
}
