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
|       COPYRIGHT   I B M   CORPORATION 2003
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/i2c.h>

#include "os/os-types.h"
#include "tuner.h"

int fd;

static void prtstat();
static int gen_freq_parm(IOCTL_TUNE_PARAMETER *p);

int main(int argc, char **argv)
{
  int rc;
  unsigned int arg;
  unsigned int arg2;
  IOCTL_TUNE_PARAMETER p;
  
  if(argc == 1)  // return tuner status
  {
    fd = open("/dev/fe_sonysat", O_RDWR);
    if(fd == 0)
    {
      printf("error opening fe_sonysat\n");
      return(-1);
    }
    prtstat();
    close(fd);
    return(0);
  }

  if(argc == 2)
  {
    if(argv[1][0] == '-' && argv[1][1] == 'h')
    {
      printf("tuner_test lnb disecq symbolrate freq\n");
      printf("     lnb        = 0 - no power\n");
      printf("                  1 - power vertical\n");
      printf("                  2 - power horizontal\n\n");
      printf("     disecq     = 0 - 22KHZ off\n");
      printf("                  1 - 22KHZ on\n\n");
      printf("     symbolrate = KBaud\n\n");
      printf("     freq       = MHz\n\n"); 
    }
  }
  if(argc != 5)
  {
    printf("Invalid parameter. Enter -h for help\n");
    return(-1);
  }
  if( 1 != sscanf(argv[1],"%d",&arg) || arg > 2)
  {
    printf("Invalid lnb parameter. Enter -h for help\n");
    return(-1);
  }
  p.lnb_power = arg;
  if( 1 != sscanf(argv[2],"%d",&arg) || arg > 1)
  {
    printf("Invalid disecq parameter. Enter -h for help\n");
    return(-1);
  }
  p.disecq_ctrl = arg;
  if( 1 != sscanf(argv[3],"%d",&arg))
  {
    printf("Invalid symbolrate parameter. Enter -h for help\n");
    return(-1);
  }
  p.symbolrate = arg;
  if( 1 != sscanf(argv[4],"%d",&arg))
  {
    printf("Invalid frequency parameter. Enter -h for help\n");
    return(-1);
  }
  p.freq = arg;
  if(0 == gen_freq_parm(&p))
  {
    fd = open("/dev/fe_sonysat", O_RDWR);
    if(fd == 0)
    {
      printf("error opening fe_sonysat\n");
      return(-1);
    }
    if( ioctl( fd, TUNER_IOCTL_TUNE, &p) < 0)
      printf("Error attempting to set tuner frequency\n");
    else
      prtstat();
   
    close(fd);
  }
  return (0);
}

/*
    Since I have not figured out how to do floating point arithmetic in kernel mode, do the 
    arithmetic here.
*/
static int gen_freq_parm(IOCTL_TUNE_PARAMETER *p)
{
  unsigned char C; 	/* Charge pump */
  unsigned char B;	/* Ref divider */ 
  unsigned char R;	/* Rx rate */ 
  unsigned char F;	/* Band Switch */
  double 	Md ;
  unsigned long	M ;
  double b_rate ;
  long SymbolRateBand ;
  long tmp ;
  double tmpD ;
  double m ;
  double clkmst ;
  unsigned char DECI, CSEL ;
  unsigned char STOFS0, STOFS1 ;
  unsigned long symbol_rate;


  symbol_rate = p->symbolrate * 1000; /* convert in Bauds */
  
  if ((p->freq >= 950) && (p->freq < 1045))
  {
    B = 0;
  }
  else if (p->freq < 1145)
  {
    B = 1;
  }
  else if (p->freq < 1260)
  {
    B = 2;
  }
  else if (p->freq < 1390)
  {
    B = 3;
  }
  else if (p->freq < 1545)
  {
    B = 4;
  }
  else if (p->freq < 1670)
  {
    B = 5;
  }
  else if (p->freq < 1815)
  {
    B = 6;
  }
  else if (p->freq < 1970)
  {
    B = 7;
  }
  else if (p->freq < 2085)
  {
    B = 8;
  }
  else if (p->freq < 2150)
  {
    B = 9;
  }
  else 
  {
    printf("Invalid frequency specified\n");
    return(-1);
  }

  /*
  ** Set the Charge Pump, Rx Rate and Reference Divider
  ** according to the symbol rate
  */
  if ((symbol_rate <= 45000000) && (symbol_rate > 40000000))
  {
    C = 5; F = 3; R = 0; Md = 0.5;
  }
  else if (symbol_rate > 33000000)
  {
    C = 4; F = 3; R = 0; Md = 0.5;
  }
  else if (symbol_rate > 22750000)
  {
    C = 5; F = 1; R = 0; Md = 0.5;
  }
  else if (symbol_rate > 11000000)
  {
    C = 5; F = 1; R = 1; Md = 1;
  }
  else if (symbol_rate > 9375000)
  {
    C = 6; F = 0; R = 1; Md = 1;
  }
  else if (symbol_rate > 1000000)
  {
    C = 6; F = 0; R = 2; Md = 2;
  }
  else
  {
    printf("Invalid symbol rate specified\n");
    return(-1);
  }

  /*
  ** Set the main divider
  */
  M = (unsigned long)((p->freq * Md) + 0.51); /* round up to nearest 1 */
  
  /*
  ** Load RF Freq data into registers
  */
  p->rf_val[0] = 0x01 | (C<<3) | (F<<1) ;
  p->rf_val[1] = (R<<5) | ((M & 0x1f000)>>12) ;
  p->rf_val[2] = (M&0x00ff0)>>4 ;
  p->rf_val[3] = ((M&0x0000f)<<4) | B ;

  b_rate = ((double)symbol_rate * 2.0) / 1.0E6 ;
 
  if(      b_rate>75.50 ) SymbolRateBand =  0 ;
  else if( b_rate>61.00 ) SymbolRateBand =  1 ;
  else if( b_rate>53.25 ) SymbolRateBand =  2 ;
  else if( b_rate>45.50 ) SymbolRateBand =  3 ;
  else if( b_rate>37.75 ) SymbolRateBand =  4 ;
  else if( b_rate>30.00 ) SymbolRateBand =  5 ;
  else if( b_rate>26.25 ) SymbolRateBand =  6 ;
  else if( b_rate>22.50 ) SymbolRateBand =  7 ;
  else if( b_rate>18.75 ) SymbolRateBand =  8 ;
  else if( b_rate>15.00 ) SymbolRateBand =  9 ;
  else if( b_rate>13.00 ) SymbolRateBand = 10 ;
  else if( b_rate>11.00 ) SymbolRateBand = 11 ;
  else if( b_rate> 9.25 ) SymbolRateBand = 12 ;
  else if( b_rate> 7.50 ) SymbolRateBand = 13 ;
  else if( b_rate> 6.50 ) SymbolRateBand = 14 ;
  else if( b_rate> 5.50 ) SymbolRateBand = 15 ;
  else if( b_rate> 4.50 ) SymbolRateBand = 16 ;
  else if( b_rate> 3.50 ) SymbolRateBand = 17 ;
  else if( b_rate> 3.00 ) SymbolRateBand = 18 ;
  else if( b_rate> 2.50 ) SymbolRateBand = 19 ;
  else if( b_rate> 2.25 ) SymbolRateBand = 20 ;
  else                    SymbolRateBand = 21 ;

  DECI = SymbolRateBand>>2 ;
  CSEL = (SymbolRateBand&0x02)>>1 ;

  if( CSEL==0 ) clkmst = 92.00 ;
  else          clkmst = 61.3333 ;

  m = (double)(1<<DECI) ;

  tmpD = ( 1.0 - m*b_rate/clkmst ) * 8192.0 ;
  tmp = (unsigned long)(tmpD + 0.50) ;
  p->symbolrateparm = tmp;
  p->symbolrateband = SymbolRateBand;

  return (0);

}

static void prtstat()
{
  IOCTL_STATUS_PARAMETER ps;

  if( ioctl( fd, TUNER_IOCTL_STATUS, &ps) < 0)
  {
    printf("Error attempting to read tuner status\n");
  }
  else
  {
    printf("\nlnb power = ");
    if(ps.lnb_power == FE_LNB_POWER_VERTICAL)
      printf("VERICAL\n");
    else if(ps.lnb_power == FE_LNB_POWER_HORIZONTAL)
      printf("HORIZONTAL\n");
    else
      printf("NO POWER\n");

    printf("disecq control = ");
    if(ps.disecq_ctrl == FE_DISECQ_22K_ON)
      printf("22KHZ ON\n");
    else if(ps.disecq_ctrl == FE_DISECQ_22K_OFF)
      printf("22KHZ OFF\n");

    printf("symbolrate = %d Kbaud\n",ps.symbolrate);
    printf("frequency = %d Mhz\n",ps.freq);
    printf("tslock = %d\n",ps.tslock);
    printf("code rate = %d/%d\n",ps.code_rate_n,ps.code_rate_d);
     
  }
  return;
}
