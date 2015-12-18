/*-----------------------------------------------------------------------------+
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
+-------------------------------------------------------------------------------
|
|  File Name:   cstuner.c
|
|  Function:    Sony NIM control functions.
|               Code for the Dual Sony Tuner board connected to H1/H2 
|
|  Author:      K. Sugita
|
|  Change Activity-
|
|  Date        Description of Change                                       BY
|  ---------   ---------------------                                       ---
|  01-Jul-99   Created                                                     K.S
|  02-Jul-99   invert TS CLOCK (rising edge in mid of data)                sn
+-----------------------------------------------------------------------------*/
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <asm/io.h>
#include "os/os-sync.h"
#include "tuner.h"
#include "cstuner.h"

static int READ_I2C(
           unsigned long               tunerid,
           unsigned char               dev_addr,
           unsigned char               dev_subaddr,
           unsigned char               count,
           unsigned char               *data,
           unsigned char               flags );

static int WRITE_I2C(
           unsigned long               tunerid,
           unsigned char               dev_addr,
           unsigned char               dev_subaddr,
           unsigned char               count,
           unsigned char               *data,
           unsigned char               flags );

static int write_reg(unsigned char index, unsigned char data);
static int read_reg(unsigned char index, unsigned char *data);

unsigned char lnb_status[2] = {FE_LNB_NO_POWER,FE_LNB_NO_POWER};

/****************************************************************************
 *                                                                          *
 *  Name:   tuner_tune_to_freq                                              *
 *  Function:                                                               *
 *  Args:                                                                   *
 *  Return:                                                                 *
 *                                                                          *
 *                                                                          *
 ****************************************************************************/
int tuner_tune_to_freq(IOCTL_TUNE_PARAMETER *p)
{
  int rc;
  int i;
  unsigned char tslock;
  unsigned char viterbi;

#ifdef TUNER_DEBUG
  printk("tuner_tune_to_freq:\n");
  printk("     lnb = %d\n",p->lnb_power);
  printk("     disecq = %d\n",p->disecq_ctrl);
  printk("     SymbolRateBand = %ld\n",p->symbolrateband);
  printk("     symbolrateparm = %ld\n",p->symbolrateparm);
  printk("     rf_val[0] = %d\n",p->rf_val[0]);
  printk("     rf_val[1] = %d\n",p->rf_val[1]);
  printk("     rf_val[2] = %d\n",p->rf_val[2]);
  printk("     rf_val[3] = %d\n",p->rf_val[3]);
#endif
  
  rc = tuner_reset(0,FE_STATE_ENABLE);
  if(rc != 0)
  {
    printk("     tuner_reset failed\n");
    return(-1);
  }

  tuner_set_lnb(0,p->lnb_power);
  tuner_set_disecq_ctrl(0,p->disecq_ctrl);
  tuner_set_freq_and_symbolrate(0,&(p->rf_val[0]),p->symbolrateparm,p->symbolrateband);
  for(i = 0; i < 50; i++)
  {
    SLEEP(100);
    tuner_read_status(0,&tslock,&viterbi);
    if(tslock)
      break;
  }

#ifdef TUNER_DEBUG
  printk("\ntslock = %d\n",tslock);
  printk("code rate = ");
  switch(viterbi)
  {
    case 0:
            printk("1/2\n");
            break;
    case 1:
            printk("2/3\n");
            break;
    case 2:
            printk("3/4\n");
            break;
    case 3:
            printk("5/6\n");
            break;
    case 4:
            printk("7/8\n");
            break;
    default:
            printk("INVALID\n");
            break;
  }
#endif
  return(0);
}


/****************************************************************************
 *                                                                          *
 *  Name:   tuner_reset                                                     *
 *  Function:                                                               *
 *  Args:                                                                   *
 *  Return:                                                                 *
 *                                                                          *
 *                                                                          *
 ****************************************************************************/

int tuner_reset(unsigned long tunerid, unsigned char fe_state)
{
  unsigned short *fpga = (void*)0xF2040000;
  unsigned short val;
  
  fpga = ioremap(0xF2040000, 32);
  if (fe_state == FE_STATE_ENABLE)
  {
    /* Release the reset Line of the Tuner card **/
    fpga[5] |= 0x2000; 

    /* Enable Tuner data path **/ 
    val = fpga[4] & 0xFFCF;
    fpga[4] = val | 0x0020; 

    /* Clock the Xilinx Registers */
    fpga[7] = 0xFFFF;
  }
  else
  {
    /* Release the reset Line of the Tuner card **/
    val = fpga[5] & 0xDFFF;
    fpga[5] = val; 

    /* Enable Streamer data path **/ 
    val = fpga[4] & 0xFFCF;
    fpga[4] = val | 0x0010; 

    /* Clock the Xilinx Registers */
    fpga[7] = 0xFFFF;
  }
  SLEEP(100);
  iounmap(fpga);
  return(0);
}

/****************************************************************************
 *                                                                          *
 *  Name:   tuner_set_freq                                                        *
 *  Function: this function tune to specified frequency                     *
 *  Args:                                                           *
 *  Return: 0   success                                                     *
 *          >0  error from nim specific initializer                         *
 *                                                                          *
 ****************************************************************************/
int tuner_set_freq_and_symbolrate(unsigned long tunerid,unsigned char *rf_val,long symbolrateparm, long SymbolRateBand)
{

  long tmp ;
  unsigned char STOFS0, STOFS1 ;



  write_reg (CXM3002_FREQ_SET_1_REG_ADD, rf_val[0]);
  write_reg (CXM3002_FREQ_SET_2_REG_ADD, rf_val[1]);
  write_reg (CXM3002_FREQ_SET_3_REG_ADD, rf_val[2]);
  write_reg (CXM3002_FREQ_SET_4_REG_ADD, rf_val[3]);
	
  /*
  ** Program the RF synthsister with the data values loaded above
  */
  write_reg (CXM3002_FREQ_SW_REG_ADD, CXM3002_WSAT_BIT_MSK);


  tmp=symbolrateparm;
  STOFS0 = tmp & CXM3002_STOFFS0_BIT_MSK ;
  STOFS1 = (tmp >> CXM3002_STOFFS1_BIT_SHIFT ) & CXM3002_STOFFS1_BIT_MSK  ;

  write_reg(CXM3002_SYMBOL_RATE_PRIMARY_REG_ADD, SymbolRateBand);
  write_reg(CXM3002_STOFFS0_REG_ADD, STOFS0);
  write_reg(CXM3002_STOFFS1_REG_ADD, STOFS1);

  write_reg (CXM3002_IQAT_REG_ADD, 0x5F);
  write_reg (CXM3002_FEC_REG_ADD, 0x47);
  write_reg (0x32, 0x0D);
  write_reg (CXM3002_RESET_REG_ADD, CXM3002_QPSK_RESET_BIT);
  write_reg (CXM3002_IQAT_REG_ADD, 0xDF);

  return (0);
}


/****************************************************************************
 *                                                                          *
 *  Name:   tuner_set_lnb                                                   *
 *  Function: this function tune to specified frequency                     *
 *  Args:  DVB_LINEAR_VERTICAL / DVB_LINEAR_HORIZONTAL                      *
 *  Return: 0   success                                                     *
 *          >0  error from nim specific initializer                         *
 *                                                                          *
 ****************************************************************************/

int tuner_set_lnb(unsigned long tunerid,unsigned char lnb)
{

  if (lnb == FE_LNB_POWER_VERTICAL)		/* H/V polarity */
  {
    lnb = 0;	   /* vertical */
  }
  else if(lnb == FE_LNB_POWER_HORIZONTAL)
  {
    lnb = 4;	   /* horizontal */
  }
  else
    lnb = 1;

  if  ( write_reg (0x27, 0x30 | lnb) != 0 ) 
  {
    /* I2C Access error */
    return (-1);	
  }

  return(0);
}

/****************************************************************************
 *                                                                          *
 *  Name:   tuner_set_disecq_ctrl                                           *
 *  Function: this function set the symbol rate                             *
 *  Args:   22K_ON : 22K tone burst ON                                      *
 *  Args:   22K_OFF : 22K tone burst OFF                                    *
 *  Return: 0   success                                                     *
 *          >0  error from nim specific initializer                         *
 *                                                                          *
 ****************************************************************************/
int tuner_set_disecq_ctrl(unsigned long tunerid, unsigned char ctrl )
{

  if (ctrl == FE_DISECQ_22K_ON)
  {
    ctrl = 0;  /** High Band ==> Enable 22Khz **/
  }
  else
  {
    ctrl = 0xC;  /** Low Band ===> Disable 22 KHz **/
  }

  /*** Write To the Tone Setting Register ***/

  if (write_reg(0x20, ctrl)) 
  {
    /* I2C Access error */
    return (-1);	
  }
     
  return (0);
}

void tuner_read_status(unsigned long tunerid, unsigned char *tslock, char *viterbi)
{
  unsigned char reg;

  read_reg(CXM3002_STATUS_REG_ADD,&reg);
#ifdef TUNER_DEBUG
  printk("Status reg = %02X\n",reg);
#endif

  *tslock = reg & 0x01;

  reg = ( reg & 0x1C ) >> 2;
  if ((*tslock == 1) && (reg != 0))
  {
    *viterbi = reg - 1;
  }
  else
  {
    *viterbi = -1;
  }
}



static int read_reg(unsigned char index, unsigned char *data)
{

  int rc;

  rc = WRITE_I2C(0,SONY_TUNER_ADDR, 0, 1, &index, IIC_FLAGS_WAIT );
  if (rc != 0) {
    printk("error in WRITE_I2C, in read routine \n");
    return rc;
  }

  rc = READ_I2C(0,SONY_TUNER_ADDR, 0, 1, data, IIC_FLAGS_WAIT);
  if (rc != 0) {
    printk("error in READ_I2C \n");
    return rc;
  }
  return 0;
}


static int write_reg(unsigned char index, unsigned char data)
{

  int rc;
  unsigned char reg[2];
    
  reg[0]= index;
  reg[1] = data;
  rc = WRITE_I2C(0,SONY_TUNER_ADDR, 0, 2, &reg[0], IIC_FLAGS_WAIT);
  if (rc != 0) {
    printk("error in WRITE_I2C\n");
    return rc;
  }
  return 0;
}

/****************************************************************************
 *                                                                          *
 *  Name:   READ_I2C                                                        *
 *  Function:                                                               *
 *  Args:                                                                   *
 *  Return:                                                                 *
 *                                                                          *
 *                                                                          *
 ****************************************************************************/

static int READ_I2C(
  unsigned long               tunerid,
  unsigned char               dev_addr,
  unsigned char               dev_subaddr,
  unsigned char               count,
  unsigned char               *data,
  unsigned char               flags )
{
  int rc;

  rc = do_read_i2c(dev_addr,dev_subaddr,count,data,flags);
  return(rc);
}

/****************************************************************************
 *                                                                          *
 *  Name:   WRITE_I2C                                                       *
 *  Function:                                                               *
 *  Args:                                                                   *
 *  Return:                                                                 *
 *                                                                          *
 *                                                                          *
 ****************************************************************************/

static int WRITE_I2C(
           unsigned long               tunerid,
           unsigned char               dev_addr,
           unsigned char               dev_subaddr,
           unsigned char               count,
           unsigned char               *data,
           unsigned char               flags )
{
  int rc;

  rc = do_write_i2c(dev_addr,dev_subaddr,count,data,flags);
  return(rc);
}

