
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
|       COPYRIGHT   I B M   CORPORATION 1999
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+-------------------------------------------------------------------------------
|
|  File Name:   cstuner.h
|
|  Function:    SHARP/ALPS NIM definitions
|
|  Author:      K. Sugita
|
|  Change Activity-
|
|  Date        Description of Change                                       BY
|  ---------   ---------------------                                       ---
|  01-Jul-99   Created                                                     K.S
|
|  Nov-2001    Changed for Lamar board                              E. Khorasani
+-----------------------------------------------------------------------------*/

#ifndef _CSTUNER_H_
#define _CSTUNER_H_

//#define TUNER_DEBUG

#define IIC_FLAGS_WAIT    1
#define IIC_FLAGS_SUB_ADDR 2

#define SLEEP(msec) {os_sleep((msec*HZ)/1000);}

#define FE_STATE_ENABLE 0
#define FE_STATE_DISABLE 1

/* TS_OUTPUT Bits	 */

#define CXM3002_TS_OUTPUT_REG_ADD	  0x0

#define CXM3002_MODE_BIT_MSK  0x08
#define CXM3002_ORDER_MSK  0x04
#define CXM3002_ERROR_BIT_MSK	 0x02
#define CXM3002_EDGE_BIT_MSK  0x01

/* FEC bits */

#define CXM3002_FEC_REG_ADD	  0x01

#define CXM3002_FSYNC_BIT_MSK  0x20
#define CXM3002_PCKB8_BIT_MSK  0x10
#define CXM3002_ER_EN_BIT_MSK  	0x4
#define CXM3002_RS_EN_BIT_MSK  	0x2
#define CXM3002_DI_EN_BIT_MSK  	0x1

/* Received Rate Setting (Symbol rate) bits */

#define CXM3002_SYMBOL_RATE_PRIMARY_REG_ADD	  0x03
#define CXM3002_DECI_BIT_MSK	  0x1C
#define CXM3002_DECI_BIT_SHIFT 		2
#define CXM3002_CSEL_BIT_MSK	  0x02
#define CXM3002_CSEL_BIT_SHIFT 		1
#define CXM3002_RSEL_BIT_MSK	  0x01
#define CXM3002_RSEL_BIT_SHIFT 		0

#define CXM3002_STOFFS0_REG_ADD	  0x04
#define CXM3002_STOFFS0_BIT_MSK	  0x0FF
#define CXM3002_STOFFS0_BIT_SHIFT 0

#define CXM3002_STOFFS1_REG_ADD	  0x05
#define CXM3002_STOFFS1_BIT_MSK	  0x00F
#define CXM3002_STOFFS1_BIT_SHIFT 8

/* Viterbi bits */

#define CXM3002_IQAT_REG_ADD	  0x06

#define CXM3002_IQSTA_BIT_MSK	0x80
#define CXM3002_IQAT_BIT_MSK	0x40
#define CXM3002_IQINV_BIT_MSK	0x20

#define CXM3002_VIR_ALL_BIT_MSK 0x1F

/* Frame Sync bits */

#define CXM3002_FRAME_SYNC_REG_ADD	  0x07

#define CXM3002_SYA_BIT_MSK  	0xF
#define CXM3002_SYA_LSB 		0x1

#define CXM3002_SYB_BIT_MSK  	0xF0
#define CXM3002_SYB_LSB 		0x10


/* CR Loop Filter bits */

#define CXM3002_CR_LOOP_COEFF_1_REG_ADD	  0x08

#define CXM3002_CRAS_BIT_MSK  	0xF
#define CXM3002_CRAS_LSB 		0x1			   

#define CXM3002_CRBS_BIT_MSK  	0xF0
#define CXM3002_CRBS_LSB 		0x10


#define CXM3002_CR_LOOP_COEFF_2_REG_ADD	  0x09

#define CXM3002_CRA_BIT_MSK  	0xF
#define CXM3002_CRA_LSB 		0x1

#define CXM3002_CRB_BIT_MSK  	0x70
#define CXM3002_CRB_LSB 		0x10


/* STR Loop Filter bits */

#define CXM3002_STR_LOOP_COEFF_1_REG_ADD	  0x0A

#define CXM3002_STRAS_BIT_MSK  	0x7
#define CXM3002_STRAS_LSB 		0x1

#define CXM3002_STRBS_BIT_MSK  	0x38
#define CXM3002_STRBS_LSB 		0x08

#define CXM3002_STR_LOOP_COEFF_2_REG_ADD	  0x0B

#define CXM3002_STRA_BIT_MSK  	0x7
#define CXM3002_STRA_LSB 		0x1

#define CXM3002_STRB_BIT_MSK  	0x38
#define CXM3002_STRB_LSB 		0x08


/* Reset register bits */

#define CXM3002_RESET_REG_ADD	  0x0C

#define CXM3002_RESET_TYPE_BITS  0x03
#define CXM3002_QPSK_RESET_BIT  0x2
#define CXM3002_LSI_RESET_BIT  0x1

#define CXM3002_RESET_BLOCK_BITS  0x03
#define CXM3002_WARM_RESET_BIT  0x2
#define CXM3002_COLD_RESET_BIT  0x1

/* Status register bits */

#define CXM3002_STATUS_REG_ADD 0x0D	

#define CXM3002_VIRM_BIT_MSK 			0x1c
#define CXM3002_VIRM_LSB 				0x04
#define CXM3002_VIM_BIT_MSK 			0x02
#define CXM3002_VITERBI_SYNC_BIT_MSK 	0x01
#define CXM3002_TS_LOCK_BIT_MSK 		0x01

/* AFC Monitor register */

#define CXM3002_AFCM_REG_ADD 0x0E
#define CXM3002_AFCM_BIT_MASK 0xFF
#define CXM3002_AFCM_LSB 0x01

/* C/N Monitor register bits */

#define CXM3002_C_N_MONITOR_REG_ADD	  0x0F

#define CXM3002_C_N_MONITOR_BIT_MSK	  0xFF
#define CXM3002_C_N_MONITOR_LSB 	0x1

/* BER registers */

#define CXM3002_BER_MONITOR_REG_ADD	0x10

#define CXM3002_BERRST_BIT_MSK		0x18
#define CXM3002_BERRST_LSB			0x08
#define CXM3002_BER_EN_BIT_MSK		0x04
#define CXM3002_BERTIM_BIT_MSK		0x02
#define CXM3002_BERSEL_BIT_MSK		0x01

#define CXM3002_BER_TABLE_REG_ADD	0x11
#define CXM3002_BERT_BIT_MSK		0x1F
#define CXM3002_BERT_LSB			0x01

#define CXM3002_BER_LSB_REG_ADD	  	0x12
#define CXM3002_BER_LSB_BIT_SHIFT	0

#define CXM3002_BER_MID_REG_ADD	  	0x13
#define CXM3002_BER_MID_BIT_SHIFT	8

#define CXM3002_BER_MSB_REG_ADD	  	0x14
#define CXM3002_BER_MSB_BIT_SHIFT	16


/* AGC Monitor register */

#define CXM3002_AGCM_REG_ADD 0x0E
#define CXM3002_AGCM_BIT_MASK 0xFF
#define CXM3002_AGCM_LSB 0x01

/* Tuner Registers */

#define CXM3002_FREQ_SET_1_REG_ADD 0x21

#define CXM3002_C_BIT_MSK		0x38
#define CXM3002_C_BIT_SHIFT		3

#define CXM3002_F_BIT_MSK		0x06
#define CXM3002_F_BIT_SHIFT		2

#define CXM3002_PE_BIT_MSK		0x01
#define CXM3002_PE_BIT_SHIFT	0

#define CXM3002_FREQ_SET_2_REG_ADD 0x22

#define CXM3002_R_BIT_MSK		0xE0
#define CXM3002_R_BIT_SHIFT		5
#define CXM3002_M_HIGH_BIT_MSK	0x1F000
#define CXM3002_M12_BIT_SHIFT	12

#define CXM3002_FREQ_SET_3_REG_ADD 0x23

#define CXM3002_M_MID_BIT_MSK	0x00FF0
#define CXM3002_M4_BIT_SHIFT	4

#define CXM3002_FREQ_SET_4_REG_ADD 0x24

#define CXM3002_M_LOW_BIT_MSK	0x0000F
#define CXM3002_M0_BIT_SHIFT	4

#define CXM3002_FREQ_SW_REG_ADD 0x25

#define CXM3002_WSAT_BIT_MSK	0x01

extern unsigned char lnb_status[2];
extern unsigned long active_tuner;

/************************/
/* Function prototypes. */
/************************/

int do_read_i2c(unsigned char devaddr, unsigned char subaddr, unsigned char count,
                 unsigned char *data, unsigned char flags);
int do_write_i2c(unsigned char devaddr, unsigned char subaddr, unsigned char count,
                 unsigned char *data, unsigned char flags);

extern int tuner_reset(unsigned long tunerid, unsigned char);
extern int tuner_set_freq_and_symbolrate(unsigned long tunerid,unsigned char *rf_val,long symbolrateparm, long SymbolRateBand);
extern int tuner_set_lnb(unsigned long tunerid,unsigned char lnb);
extern int tuner_set_disecq_ctrl(unsigned long tunerid,unsigned char ctrl );
extern void tuner_read_status(unsigned long tuner_id,unsigned char*,char *);
int tuner_tune_to_freq(IOCTL_TUNE_PARAMETER *p);
#endif  /* end of CS_TUNER_H_ */
