// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023, Alex Taradov <alex@taradov.com>. All rights reserved.

#ifndef _FX2_REGS_H_
#define _FX2_REGS_H_

/*- Includes ----------------------------------------------------------------*/
#include <stdint.h>

/*- Definitions -------------------------------------------------------------*/
#define REG(addr, name)			__xdata __at(addr) volatile uint8_t name
#define SFR_REG(addr, name)		__sfr   __at(addr) name
#define SFR_BIT(addr, name)		__sbit  __at(addr) name

#define SYNCDELAY			__asm nop; nop; nop; nop; nop; nop; __endasm
#define NOP				__asm nop; __endasm

#define MSB(x)				(((uint16_t)(x)) >> 8)
#define LSB(x)				(((uint16_t)(x)) & 0xff)

// Registers that Require a Synchronization Delay:
// FIFORESET FIFOPINPOLAR ECCCFG INPKTEND EPxBCH:L ECCRESET
// EPxFIFOPFH:L EPxAUTOINLENH:L ECC1B0 EPxFIFOCFG EPxGPIFFLGSEL ECC1B1
// PINFLAGSAB PINFLAGSCD ECC1B2 EPxFIFOIE EPxFIFOIRQ ECC2B0
// GPIFIE GPIFIRQ ECC2B1 UDMACRCH:L GPIFADRH:L ECC2B2
// GPIFTRIG EPxGPIFTRIG OUTPKTEND REVCTL GPIFTCB3 GPIFTCB2
// GPIFTCB1 GPIFTCB0

//-----------------------------------------------------------------------------
SFR_REG(0x80, IOA);
  SFR_BIT(0x80+0, IOA_0_b);
  SFR_BIT(0x80+1, IOA_1_b);
  SFR_BIT(0x80+2, IOA_2_b);
  SFR_BIT(0x80+3, IOA_3_b);
  SFR_BIT(0x80+4, IOA_4_b);
  SFR_BIT(0x80+5, IOA_5_b);
  SFR_BIT(0x80+6, IOA_6_b);
  SFR_BIT(0x80+7, IOA_7_b);
SFR_REG(0x81, SP);
SFR_REG(0x82, DPL0);
SFR_REG(0x83, DPH0);
SFR_REG(0x84, DPL1);
SFR_REG(0x85, DPH1);
SFR_REG(0x86, DPS);
  SFR_BIT(0x86+0, DPS_SEL_b);
SFR_REG(0x87, PCON);
  SFR_BIT(0x87+0, PCON_IDLE_b);
  SFR_BIT(0x87+7, PCON_SMOD0_b);
SFR_REG(0x88, TCON);
  SFR_BIT(0x88+0, TCON_IT0_b);
  SFR_BIT(0x88+1, TCON_IE0_b);
  SFR_BIT(0x88+2, TCON_IT1_b);
  SFR_BIT(0x88+3, TCON_IE1_b);
  SFR_BIT(0x88+4, TCON_TR0_b);
  SFR_BIT(0x88+5, TCON_TF0_b);
  SFR_BIT(0x88+6, TCON_TR1_b);
  SFR_BIT(0x88+7, TCON_TF1_b);
SFR_REG(0x89, TMOD);
  SFR_BIT(0x89+0, TMOD_M00_b);
  SFR_BIT(0x89+1, TMOD_M10_b);
  SFR_BIT(0x89+2, TMOD_CT0_b);
  SFR_BIT(0x89+3, TMOD_GATE0_b);
  SFR_BIT(0x89+4, TMOD_M01_b);
  SFR_BIT(0x89+5, TMOD_M11_b);
  SFR_BIT(0x89+6, TMOD_CT1_b);
  SFR_BIT(0x89+7, TMOD_GATE1_b);
SFR_REG(0x8a, TL0);
SFR_REG(0x8b, TL1);
SFR_REG(0x8c, TH0);
SFR_REG(0x8d, TH1);
SFR_REG(0x8e, CKCON);
  SFR_BIT(0x8e+0, CKCON_MD0_b);
  SFR_BIT(0x8e+1, CKCON_MD1_b);
  SFR_BIT(0x8e+2, CKCON_MD2_b);
  SFR_BIT(0x8e+3, CKCON_T0M_b);
  SFR_BIT(0x8e+4, CKCON_T1M_b);
  SFR_BIT(0x8e+5, CKCON_T2M_b);
SFR_REG(0x90, IOB);
  SFR_BIT(0x90+0, IOB_0_b);
  SFR_BIT(0x90+1, IOB_1_b);
  SFR_BIT(0x90+2, IOB_2_b);
  SFR_BIT(0x90+3, IOB_3_b);
  SFR_BIT(0x90+4, IOB_4_b);
  SFR_BIT(0x90+5, IOB_5_b);
  SFR_BIT(0x90+6, IOB_6_b);
  SFR_BIT(0x90+7, IOB_7_b);
SFR_REG(0x91, EXIF);
  SFR_BIT(0x91+4, EXIF_USBINT_b);
  SFR_BIT(0x91+5, EXIF_I2CINT_b);
  SFR_BIT(0x91+6, EXIF_IE4_b);
  SFR_BIT(0x91+7, EXIF_IE5_b);
SFR_REG(0x92, MPAGE);
SFR_REG(0x98, SCON0);
  SFR_BIT(0x98+0, SCON0_RI_b);
  SFR_BIT(0x98+1, SCON0_TI_b);
  SFR_BIT(0x98+2, SCON0_RB8_b);
  SFR_BIT(0x98+3, SCON0_TB8_b);
  SFR_BIT(0x98+4, SCON0_REN_b);
  SFR_BIT(0x98+5, SCON0_SM2_b);
  SFR_BIT(0x98+6, SCON0_SM1_b);
  SFR_BIT(0x98+7, SCON0_SM0_b);
SFR_REG(0x99, SBUF0);
SFR_REG(0x9a, AUTOPTRH1);
SFR_REG(0x9b, AUTOPTRL1);
SFR_REG(0x9d, AUTOPTRH2);
SFR_REG(0x9e, AUTOPTRL2);
SFR_REG(0xa0, IOC);
  SFR_BIT(0xa0+0, IOC_0_b);
  SFR_BIT(0xa0+1, IOC_1_b);
  SFR_BIT(0xa0+2, IOC_2_b);
  SFR_BIT(0xa0+3, IOC_3_b);
  SFR_BIT(0xa0+4, IOC_4_b);
  SFR_BIT(0xa0+5, IOC_5_b);
  SFR_BIT(0xa0+6, IOC_6_b);
  SFR_BIT(0xa0+7, IOC_7_b);
SFR_REG(0xa1, INT2CLR);
SFR_REG(0xa2, INT4CLR);
SFR_REG(0xa8, IE);
  SFR_BIT(0xa8+0, IE_EX0_b);
  SFR_BIT(0xa8+1, IE_ET0_b);
  SFR_BIT(0xa8+2, IE_EX1_b);
  SFR_BIT(0xa8+3, IE_ET1_b);
  SFR_BIT(0xa8+4, IE_ES0_b);
  SFR_BIT(0xa8+5, IE_ET2_b);
  SFR_BIT(0xa8+6, IE_ES1_b);
  SFR_BIT(0xa8+7, IE_EA_b);
SFR_REG(0xaa, EP2468STAT);
  SFR_BIT(0xaa+0, EP2468STAT_EP2E_b);
  SFR_BIT(0xaa+1, EP2468STAT_EP2F_b);
  SFR_BIT(0xaa+2, EP2468STAT_EP4E_b);
  SFR_BIT(0xaa+3, EP2468STAT_EP4F_b);
  SFR_BIT(0xaa+4, EP2468STAT_EP6E_b);
  SFR_BIT(0xaa+5, EP2468STAT_EP6F_b);
  SFR_BIT(0xaa+6, EP2468STAT_EP8E_b);
  SFR_BIT(0xaa+7, EP2468STAT_EP8F_b);
SFR_REG(0xab, EP24FIFOFLGS);
  SFR_BIT(0xab+0, EP24FIFOFLGS_EP2FF_b);
  SFR_BIT(0xab+1, EP24FIFOFLGS_EP2EF_b);
  SFR_BIT(0xab+2, EP24FIFOFLGS_EP2PF_b);
  SFR_BIT(0xab+3, EP24FIFOFLGS_EP4FF_b);
  SFR_BIT(0xab+4, EP24FIFOFLGS_EP4EF_b);
  SFR_BIT(0xab+5, EP24FIFOFLGS_EP4PF_b);
SFR_REG(0xac, EP68FIFOFLGS);
  SFR_BIT(0xac+0, EP68FIFOFLGS_EP6FF_b);
  SFR_BIT(0xac+1, EP68FIFOFLGS_EP6EF_b);
  SFR_BIT(0xac+2, EP68FIFOFLGS_EP6PF_b);
  SFR_BIT(0xac+3, EP68FIFOFLGS_EP8FF_b);
  SFR_BIT(0xac+4, EP68FIFOFLGS_EP8EF_b);
  SFR_BIT(0xac+5, EP68FIFOFLGS_EP8PF_b);
SFR_REG(0xaf, AUTOPTRSETUP);
  SFR_BIT(0xaf+0, AUTOPTRSETUP_APTREN_b);
  SFR_BIT(0xaf+1, AUTOPTRSETUP_APTR1INC_b);
  SFR_BIT(0xaf+2, AUTOPTRSETUP_APTR2INC_b);
SFR_REG(0xb0, IOD);
  SFR_BIT(0xb0+0, IOD_0_b);
  SFR_BIT(0xb0+1, IOD_1_b);
  SFR_BIT(0xb0+2, IOD_2_b);
  SFR_BIT(0xb0+3, IOD_3_b);
  SFR_BIT(0xb0+4, IOD_4_b);
  SFR_BIT(0xb0+5, IOD_5_b);
  SFR_BIT(0xb0+6, IOD_6_b);
  SFR_BIT(0xb0+7, IOD_7_b);
SFR_REG(0xb1, IOE);
  SFR_BIT(0xb1+0, IOE_0_b);
  SFR_BIT(0xb1+1, IOE_1_b);
  SFR_BIT(0xb1+2, IOE_2_b);
  SFR_BIT(0xb1+3, IOE_3_b);
  SFR_BIT(0xb1+4, IOE_4_b);
  SFR_BIT(0xb1+5, IOE_5_b);
  SFR_BIT(0xb1+6, IOE_6_b);
  SFR_BIT(0xb1+7, IOE_7_b);
SFR_REG(0xb2, OEA);
  SFR_BIT(0xb2+0, OEA_0_b);
  SFR_BIT(0xb2+1, OEA_1_b);
  SFR_BIT(0xb2+2, OEA_2_b);
  SFR_BIT(0xb2+3, OEA_3_b);
  SFR_BIT(0xb2+4, OEA_4_b);
  SFR_BIT(0xb2+5, OEA_5_b);
  SFR_BIT(0xb2+6, OEA_6_b);
  SFR_BIT(0xb2+7, OEA_7_b);
SFR_REG(0xb3, OEB);
  SFR_BIT(0xb3+0, OEB_0_b);
  SFR_BIT(0xb3+1, OEB_1_b);
  SFR_BIT(0xb3+2, OEB_2_b);
  SFR_BIT(0xb3+3, OEB_3_b);
  SFR_BIT(0xb3+4, OEB_4_b);
  SFR_BIT(0xb3+5, OEB_5_b);
  SFR_BIT(0xb3+6, OEB_6_b);
  SFR_BIT(0xb3+7, OEB_7_b);
SFR_REG(0xb4, OEC);
  SFR_BIT(0xb4+0, OEC_0_b);
  SFR_BIT(0xb4+1, OEC_1_b);
  SFR_BIT(0xb4+2, OEC_2_b);
  SFR_BIT(0xb4+3, OEC_3_b);
  SFR_BIT(0xb4+4, OEC_4_b);
  SFR_BIT(0xb4+5, OEC_5_b);
  SFR_BIT(0xb4+6, OEC_6_b);
  SFR_BIT(0xb4+7, OEC_7_b);
SFR_REG(0xb5, OED);
  SFR_BIT(0xb5+0, OED_0_b);
  SFR_BIT(0xb5+1, OED_1_b);
  SFR_BIT(0xb5+2, OED_2_b);
  SFR_BIT(0xb5+3, OED_3_b);
  SFR_BIT(0xb5+4, OED_4_b);
  SFR_BIT(0xb5+5, OED_5_b);
  SFR_BIT(0xb5+6, OED_6_b);
  SFR_BIT(0xb5+7, OED_7_b);
SFR_REG(0xb6, OEE);
  SFR_BIT(0xb6+0, OEE_0_b);
  SFR_BIT(0xb6+1, OEE_1_b);
  SFR_BIT(0xb6+2, OEE_2_b);
  SFR_BIT(0xb6+3, OEE_3_b);
  SFR_BIT(0xb6+4, OEE_4_b);
  SFR_BIT(0xb6+5, OEE_5_b);
  SFR_BIT(0xb6+6, OEE_6_b);
  SFR_BIT(0xb6+7, OEE_7_b);
SFR_REG(0xb8, IP);
  SFR_BIT(0xb8+0, IP_PX0);
  SFR_BIT(0xb8+1, IP_PT0);
  SFR_BIT(0xb8+2, IP_PX1);
  SFR_BIT(0xb8+3, IP_PT1);
  SFR_BIT(0xb8+4, IP_PS0);
  SFR_BIT(0xb8+5, IP_PT2);
  SFR_BIT(0xb8+6, IP_PS1);
SFR_REG(0xba, EP01STAT);
  SFR_BIT(0xba+0, EP01STAT_EP0BSY_b);
  SFR_BIT(0xba+1, EP01STAT_EP1OUTBSY_b);
  SFR_BIT(0xba+2, EP01STAT_EP1INBSY_b);
SFR_REG(0xbb, GPIFTRIG);
  SFR_BIT(0xbb+0, GPIFTRIG_EP0_b);
  SFR_BIT(0xbb+1, GPIFTRIG_EP1_b);
  SFR_BIT(0xbb+2, GPIFTRIG_RW_b);
  SFR_BIT(0xbb+7, GPIFTRIG_DONE_b);
SFR_REG(0xbd, GPIFSGLDATH);
SFR_REG(0xbe, GPIFSGLDATLX);
SFR_REG(0xbf, GPIFSGLDATLNOX);
SFR_REG(0xc0, SCON1);
  SFR_BIT(0xc0+0, SCON1_RI_b);
  SFR_BIT(0xc0+1, SCON1_TI_b);
  SFR_BIT(0xc0+2, SCON1_RB8_b);
  SFR_BIT(0xc0+3, SCON1_TB8_b);
  SFR_BIT(0xc0+4, SCON1_REN_b);
  SFR_BIT(0xc0+5, SCON1_SM2_b);
  SFR_BIT(0xc0+6, SCON1_SM1_b);
  SFR_BIT(0xc0+7, SCON1_SM0_b);
SFR_REG(0xc1, SBUF1);
SFR_REG(0xc8, T2CON);
  SFR_BIT(0xc8+0, T2CON_CPRL2_b);
  SFR_BIT(0xc8+1, T2CON_CT2_b);
  SFR_BIT(0xc8+2, T2CON_TR2_b);
  SFR_BIT(0xc8+3, T2CON_EXEN2_b);
  SFR_BIT(0xc8+4, T2CON_TCLK_b);
  SFR_BIT(0xc8+5, T2CON_RCLK_b);
  SFR_BIT(0xc8+6, T2CON_EXF2_b);
  SFR_BIT(0xc8+7, T2CON_TF2_b);
SFR_REG(0xca, RCAP2L);
SFR_REG(0xcb, RCAP2H);
SFR_REG(0xcc, TL2);
SFR_REG(0xcd, TH2);
SFR_REG(0xd0, PSW);
  SFR_BIT(0xd0+0, PSW_P_b);
  SFR_BIT(0xd0+1, PSW_F1_b);
  SFR_BIT(0xd0+2, PSW_OV_b);
  SFR_BIT(0xd0+3, PSW_RS0_b);
  SFR_BIT(0xd0+4, PSW_RS1_b);
  SFR_BIT(0xd0+5, PSW_F0_b);
  SFR_BIT(0xd0+6, PSW_AC_b);
  SFR_BIT(0xd0+7, PSW_CY_b);
SFR_REG(0xd8, EICON);
  SFR_BIT(0xd8+3, EICON_INT6_b);
  SFR_BIT(0xd8+4, EICON_RESI_b);
  SFR_BIT(0xd8+5, EICON_ERESI_b);
  SFR_BIT(0xd8+7, EICON_SMOD1_b);
SFR_REG(0xe0, ACC);
SFR_REG(0xe8, EIE);
  SFR_BIT(0xe8+0, EIE_EUSB_b);
  SFR_BIT(0xe8+1, EIE_EI2C_b);
  SFR_BIT(0xe8+2, EIE_EX4_b);
  SFR_BIT(0xe8+3, EIE_EX5_b);
  SFR_BIT(0xe8+4, EIE_EX6_b);
SFR_REG(0xf0, B);
  SFR_BIT(0xf0+0, B_0_b);
  SFR_BIT(0xf0+1, B_1_b);
  SFR_BIT(0xf0+2, B_2_b);
  SFR_BIT(0xf0+3, B_3_b);
  SFR_BIT(0xf0+4, B_4_b);
  SFR_BIT(0xf0+5, B_5_b);
  SFR_BIT(0xf0+6, B_6_b);
  SFR_BIT(0xf0+7, B_7_b);
SFR_REG(0xf8, EIP);
  SFR_BIT(0xf8+0, EIP_PUSB_b);
  SFR_BIT(0xf8+1, EIP_PI2C_b);
  SFR_BIT(0xf8+2, EIP_PX4_b);
  SFR_BIT(0xf8+3, EIP_PX5_b);
  SFR_BIT(0xf8+4, EIP_PX6_b);

//-----------------------------------------------------------------------------
#define DPS_SEL				(1ul << 0)

#define PCON_IDLE			(1ul << 0)
#define PCON_SMOD0			(1ul << 7)

#define TCON_IT0			(1ul << 0)
#define TCON_IE0			(1ul << 1)
#define TCON_IT1			(1ul << 2)
#define TCON_IE1			(1ul << 3)
#define TCON_TR0			(1ul << 4)
#define TCON_TF0			(1ul << 5)
#define TCON_TR1			(1ul << 6)
#define TCON_TF1			(1ul << 7)

#define TMOD_M00			(1ul << 0)
#define TMOD_M10			(1ul << 1)
#define TMOD_CT0			(1ul << 2)
#define TMOD_GATE0			(1ul << 3)
#define TMOD_M01			(1ul << 4)
#define TMOD_M11			(1ul << 5)
#define TMOD_CT1			(1ul << 6)
#define TMOD_GATE1			(1ul << 7)

#define CKCON_MD0			(1ul << 0)
#define CKCON_MD1			(1ul << 1)
#define CKCON_MD2			(1ul << 2)
#define CKCON_T0M			(1ul << 3)
#define CKCON_T1M			(1ul << 4)
#define CKCON_T2M			(1ul << 5)

#define EXIF_USBINT			(1ul << 4)
#define EXIF_I2CINT			(1ul << 5)
#define EXIF_IE4			(1ul << 6)
#define EXIF_IE5			(1ul << 7)

#define SCON0_RI			(1ul << 0)
#define SCON0_TI			(1ul << 1)
#define SCON0_RB8			(1ul << 2)
#define SCON0_TB8			(1ul << 3)
#define SCON0_REN			(1ul << 4)
#define SCON0_SM2			(1ul << 5)
#define SCON0_SM1			(1ul << 6)
#define SCON0_SM0			(1ul << 7)

#define IE_EX0				(1ul << 0)
#define IE_ET0				(1ul << 1)
#define IE_EX1				(1ul << 2)
#define IE_ET1				(1ul << 3)
#define IE_ES0				(1ul << 4)
#define IE_ET2				(1ul << 5)
#define IE_ES1				(1ul << 6)
#define IE_EA				(1ul << 7)

#define EP2468STAT_EP2E			(1ul << 0)
#define EP2468STAT_EP2F			(1ul << 1)
#define EP2468STAT_EP4E			(1ul << 2)
#define EP2468STAT_EP4F			(1ul << 3)
#define EP2468STAT_EP6E			(1ul << 4)
#define EP2468STAT_EP6F			(1ul << 5)
#define EP2468STAT_EP8E			(1ul << 6)
#define EP2468STAT_EP8F			(1ul << 7)

#define EP24FIFOFLGS_EP2FF		(1ul << 0)
#define EP24FIFOFLGS_EP2EF		(1ul << 1)
#define EP24FIFOFLGS_EP2PF		(1ul << 2)
#define EP24FIFOFLGS_EP4FF		(1ul << 3)
#define EP24FIFOFLGS_EP4EF		(1ul << 4)
#define EP24FIFOFLGS_EP4PF		(1ul << 5)

#define EP68FIFOFLGS_EP6FF		(1ul << 0)
#define EP68FIFOFLGS_EP6EF		(1ul << 1)
#define EP68FIFOFLGS_EP6PF		(1ul << 2)
#define EP68FIFOFLGS_EP8FF		(1ul << 3)
#define EP68FIFOFLGS_EP8EF		(1ul << 4)
#define EP68FIFOFLGS_EP8PF		(1ul << 5)

#define AUTOPTRSETUP_APTREN		(1ul << 0)
#define AUTOPTRSETUP_APTR1INC		(1ul << 1)
#define AUTOPTRSETUP_APTR2INC		(1ul << 2)

#define IP_PX0				(1ul << 0)
#define IP_PT0				(1ul << 1)
#define IP_PX1				(1ul << 2)
#define IP_PT1				(1ul << 3)
#define IP_PS0				(1ul << 4)
#define IP_PT2				(1ul << 5)
#define IP_PS1				(1ul << 6)

#define EP01STAT_EP0BSY			(1ul << 0)
#define EP01STAT_EP1OUTBSY		(1ul << 1)
#define EP01STAT_EP1INBSY		(1ul << 2)

#define GPIFTRIG_EP0			(1ul << 0)
#define GPIFTRIG_EP1			(1ul << 1)
#define GPIFTRIG_RW			(1ul << 2)
#define GPIFTRIG_DONE			(1ul << 7)

#define SCON1_RI			(1ul << 0)
#define SCON1_TI			(1ul << 1)
#define SCON1_RB8			(1ul << 2)
#define SCON1_TB8			(1ul << 3)
#define SCON1_REN			(1ul << 4)
#define SCON1_SM2			(1ul << 5)
#define SCON1_SM1			(1ul << 6)
#define SCON1_SM0			(1ul << 7)

#define T2CON_CPRL2			(1ul << 0)
#define T2CON_CT2			(1ul << 1)
#define T2CON_TR2			(1ul << 2)
#define T2CON_EXEN2			(1ul << 3)
#define T2CON_TCLK			(1ul << 4)
#define T2CON_RCLK			(1ul << 5)
#define T2CON_EXF2			(1ul << 6)
#define T2CON_TF2			(1ul << 7)

#define PSW_P				(1ul << 0)
#define PSW_F1				(1ul << 1)
#define PSW_OV				(1ul << 2)
#define PSW_RS0				(1ul << 3)
#define PSW_RS1				(1ul << 4)
#define PSW_F0				(1ul << 5)
#define PSW_AC				(1ul << 6)
#define PSW_CY				(1ul << 7)

#define EICON_INT6			(1ul << 3)
#define EICON_RESI			(1ul << 4)
#define EICON_ERESI			(1ul << 5)
#define EICON_SMOD1			(1ul << 7)

#define EIE_EUSB			(1ul << 0)
#define EIE_EI2C			(1ul << 1)
#define EIE_EX4				(1ul << 2)
#define EIE_EX5				(1ul << 3)
#define EIE_EX6				(1ul << 4)

#define EIP_PUSB			(1ul << 0)
#define EIP_PI2C			(1ul << 1)
#define EIP_PX4				(1ul << 2)
#define EIP_PX5				(1ul << 3)
#define EIP_PX6				(1ul << 4)

//-----------------------------------------------------------------------------
// GPIF Waveform Memories
REG(0xE400, GPIF_WAVE_DATA[128]);

// General Configuration
REG(0xE600, CPUCS);
REG(0xE601, IFCONFIG);
REG(0xE602, PINFLAGSAB);
REG(0xE603, PINFLAGSCD);
REG(0xE604, FIFORESET);
REG(0xE605, BREAKPT);
REG(0xE606, BPADDRH);
REG(0xE607, BPADDRL);
REG(0xE608, UART230);
REG(0xE609, FIFOPINPOLAR);
REG(0xE60A, REVID);
REG(0xE60B, REVCTL);

#define CPUCS_8051RES			(1ul << 0)
#define CPUCS_CLKOE			(1ul << 1)
#define CPUCS_CLKINV			(1ul << 2)
#define CPUCS_CLKSPD0			(1ul << 3)
#define CPUCS_CLKSPD1			(1ul << 4)
#define   CPUCS_CLKSPD_12_MHZ			(0ul << 3)
#define   CPUCS_CLKSPD_24_MHZ			(1ul << 3)
#define   CPUCS_CLKSPD_48_MHZ			(2ul << 3)
#define CPUCS_PRTCSTB			(1ul << 5)

#define IFCONFIG_IFCFG0			(1ul << 0)
#define IFCONFIG_IFCFG1			(1ul << 1)
#define   IFCONFIG_IFCFG_PORTS			(0ul << 0)
#define   IFCONFIG_IFCFG_GPIF			(2ul << 0)
#define   IFCONFIG_IFCFG_FIFO			(3ul << 0)
#define IFCONFIG_GSTATE			(1ul << 2)
#define IFCONFIG_ASYNC			(1ul << 3)
#define IFCONFIG_IFCLKPOL		(1ul << 4)
#define IFCONFIG_IFCLKOE		(1ul << 5)
#define IFCONFIG_3048MHZ		(1ul << 6)
#define IFCONFIG_IFCLKSRC		(1ul << 7)

#define PINFLAGSAB_FLAGA_FIFOADR_PF	(0ul << 0)
#define PINFLAGSAB_FLAGA_EP2PF		(4ul << 0)
#define PINFLAGSAB_FLAGA_EP4PF		(5ul << 0)
#define PINFLAGSAB_FLAGA_EP6PF		(6ul << 0)
#define PINFLAGSAB_FLAGA_EP8PF		(7ul << 0)
#define PINFLAGSAB_FLAGA_EP2EF		(8ul << 0)
#define PINFLAGSAB_FLAGA_EP4EF		(9ul << 0)
#define PINFLAGSAB_FLAGA_EP6EF		(10ul << 0)
#define PINFLAGSAB_FLAGA_EP8EF		(11ul << 0)
#define PINFLAGSAB_FLAGA_EP2FF		(12ul << 0)
#define PINFLAGSAB_FLAGA_EP4FF		(13ul << 0)
#define PINFLAGSAB_FLAGA_EP6FF		(14ul << 0)
#define PINFLAGSAB_FLAGA_EP8FF		(15ul << 0)

#define PINFLAGSAB_FLAGB_FIFOADR_PF	(0ul << 4)
#define PINFLAGSAB_FLAGB_EP2PF		(4ul << 4)
#define PINFLAGSAB_FLAGB_EP4PF		(5ul << 4)
#define PINFLAGSAB_FLAGB_EP6PF		(6ul << 4)
#define PINFLAGSAB_FLAGB_EP8PF		(7ul << 4)
#define PINFLAGSAB_FLAGB_EP2EF		(8ul << 4)
#define PINFLAGSAB_FLAGB_EP4EF		(9ul << 4)
#define PINFLAGSAB_FLAGB_EP6EF		(10ul << 4)
#define PINFLAGSAB_FLAGB_EP8EF		(11ul << 4)
#define PINFLAGSAB_FLAGB_EP2FF		(12ul << 4)
#define PINFLAGSAB_FLAGB_EP4FF		(13ul << 4)
#define PINFLAGSAB_FLAGB_EP6FF		(14ul << 4)
#define PINFLAGSAB_FLAGB_EP8FF		(15ul << 4)

#define PINFLAGSCD_FLAGC_FIFOADR_PF	(0ul << 0)
#define PINFLAGSCD_FLAGC_EP2PF		(4ul << 0)
#define PINFLAGSCD_FLAGC_EP4PF		(5ul << 0)
#define PINFLAGSCD_FLAGC_EP6PF		(6ul << 0)
#define PINFLAGSCD_FLAGC_EP8PF		(7ul << 0)
#define PINFLAGSCD_FLAGC_EP2EF		(8ul << 0)
#define PINFLAGSCD_FLAGC_EP4EF		(9ul << 0)
#define PINFLAGSCD_FLAGC_EP6EF		(10ul << 0)
#define PINFLAGSCD_FLAGC_EP8EF		(11ul << 0)
#define PINFLAGSCD_FLAGC_EP2FF		(12ul << 0)
#define PINFLAGSCD_FLAGC_EP4FF		(13ul << 0)
#define PINFLAGSCD_FLAGC_EP6FF		(14ul << 0)
#define PINFLAGSCD_FLAGC_EP8FF		(15ul << 0)

#define PINFLAGSCD_FLAGD_FIFOADR_PF	(0ul << 4)
#define PINFLAGSCD_FLAGD_EP2PF		(4ul << 4)
#define PINFLAGSCD_FLAGD_EP4PF		(5ul << 4)
#define PINFLAGSCD_FLAGD_EP6PF		(6ul << 4)
#define PINFLAGSCD_FLAGD_EP8PF		(7ul << 4)
#define PINFLAGSCD_FLAGD_EP2EF		(8ul << 4)
#define PINFLAGSCD_FLAGD_EP4EF		(9ul << 4)
#define PINFLAGSCD_FLAGD_EP6EF		(10ul << 4)
#define PINFLAGSCD_FLAGD_EP8EF		(11ul << 4)
#define PINFLAGSCD_FLAGD_EP2FF		(12ul << 4)
#define PINFLAGSCD_FLAGD_EP4FF		(13ul << 4)
#define PINFLAGSCD_FLAGD_EP6FF		(14ul << 4)
#define PINFLAGSCD_FLAGD_EP8FF		(15ul << 4)

#define FIFORESET_EP(x)			((x) << 0)
#define FIFORESET_NAKALL		(1ul << 7)

#define BREAKPT_BPEN			(1ul << 1)
#define BREAKPT_BPPULSE			(1ul << 2)
#define BREAKPT_BREAK			(1ul << 3)

#define FIFOPINPOLAR_FF			(1ul << 0)
#define FIFOPINPOLAR_EF			(1ul << 1)
#define FIFOPINPOLAR_SLWR		(1ul << 2)
#define FIFOPINPOLAR_SLRD		(1ul << 3)
#define FIFOPINPOLAR_SLOE		(1ul << 4)
#define FIFOPINPOLAR_PKTEND		(1ul << 5)

#define REVCTL_ENH_PKT			(1ul << 0)
#define REVCTL_DYN_OUT			(1ul << 1)

// Endpoint Configuration
REG(0xe610, EP1OUTCFG);
REG(0xe611, EP1INCFG);
REG(0xe612, EP2CFG);
REG(0xe613, EP4CFG);
REG(0xe614, EP6CFG);
REG(0xe615, EP8CFG);
REG(0xe618, EP2FIFOCFG);
REG(0xe619, EP4FIFOCFG);
REG(0xe61a, EP6FIFOCFG);
REG(0xe61b, EP8FIFOCFG);
REG(0xe620, EP2AUTOINLENH);
REG(0xe621, EP2AUTOINLENL);
REG(0xe622, EP4AUTOINLENH);
REG(0xe623, EP4AUTOINLENL);
REG(0xe624, EP6AUTOINLENH);
REG(0xe625, EP6AUTOINLENL);
REG(0xe626, EP8AUTOINLENH);
REG(0xe627, EP8AUTOINLENL);
REG(0xe630, EP2FIFOPFH);
REG(0xe631, EP2FIFOPFL);
REG(0xe632, EP4FIFOPFH);
REG(0xe633, EP4FIFOPFL);
REG(0xe634, EP6FIFOPFH);
REG(0xe635, EP6FIFOPFL);
REG(0xe636, EP8FIFOPFH);
REG(0xe637, EP8FIFOPFL);
REG(0xe640, EP2ISOINPKTS);
REG(0xe641, EP4ISOINPKTS);
REG(0xe642, EP6ISOINPKTS);
REG(0xe643, EP8ISOINPKTS);
REG(0xe648, INPKTEND);
REG(0xe649, OUTPKTEND);

#define EPCFG_BUF0			(1ul << 0)
#define EPCFG_BUF1			(1ul << 1)
#define   EPCFG_BUF_QUAD			(0ul << 0)
#define   EPCFG_BUF_DOUBLE			(2ul << 0)
#define   EPCFG_BUF_TRIPLE			(3ul << 0)
#define EPCFG_SIZE			(1ul << 3)
#define   EPCFG_SIZE_512			(0ul << 3)
#define   EPCFG_SIZE_1024			(1ul << 3)
#define EPCFG_TYPE0			(1ul << 4)
#define EPCFG_TYPE1			(1ul << 5)
#define   EPCFG_TYPE_ISOCHRONOUS		(1ul << 4)
#define   EPCFG_TYPE_BULK			(2ul << 4)
#define   EPCFG_TYPE_INTERRUPT			(3ul << 4)
#define EPCFG_DIR			(1ul << 6)
#define   EPCFG_DIR_OUT				(0ul << 6)
#define   EPCFG_DIR_IN				(1ul << 6)
#define EPCFG_VALID			(1ul << 7)

#define EPFIFOCFG_WORDWIDE		(1ul << 0)
#define EPFIFOCFG_ZEROLENIN		(1ul << 2)
#define EPFIFOCFG_AUTOIN		(1ul << 3)
#define EPFIFOCFG_AUTOOUT		(1ul << 4)
#define EPFIFOCFG_OEP			(1ul << 5)
#define EPFIFOCFG_INFM			(1ul << 6)

#define EPISOINPKTS_INPPF0		(1ul << 0)
#define EPISOINPKTS_INPPF1		(1ul << 1)
#define   EPISOINPKTS_INPPF_1_PER_FRAME		(1ul << 0)
#define   EPISOINPKTS_INPPF_2_PER_FRAME		(2ul << 0)
#define   EPISOINPKTS_INPPF_3_PER_FRAME		(3ul << 0)
#define EPISOINPKTS_AADJ		(1ul << 7)

#define INPKTEND_EP(x)			((x) << 0)
#define INPKTEND_SKIP			(1ul << 7)

#define OUTPKTEND_EP(x)			((x) << 0)
#define OUTPKTEND_SKIP			(1ul << 7)

// Endpoints
REG(0xe68a, EP0BCH);
REG(0xe68b, EP0BCL);
REG(0xe68d, EP1OUTBC);
REG(0xe68f, EP1INBC);
REG(0xe690, EP2BCH);
REG(0xe691, EP2BCL);
REG(0xe694, EP4BCH);
REG(0xe695, EP4BCL);
REG(0xe698, EP6BCH);
REG(0xe699, EP6BCL);
REG(0xe69c, EP8BCH);
REG(0xe69d, EP8BCL);
REG(0xe6a0, EP0CS);
REG(0xe6a1, EP1OUTCS);
REG(0xe6a2, EP1INCS);
REG(0xe6a3, EP2CS);
REG(0xe6a4, EP4CS);
REG(0xe6a5, EP6CS);
REG(0xe6a6, EP8CS);
REG(0xe6a7, EP2FIFOFLGS);
REG(0xe6a8, EP4FIFOFLGS);
REG(0xe6a9, EP6FIFOFLGS);
REG(0xe6aa, EP8FIFOFLGS);
REG(0xe6ab, EP2FIFOBCH);
REG(0xe6ac, EP2FIFOBCL);
REG(0xe6ad, EP4FIFOBCH);
REG(0xe6ae, EP4FIFOBCL);
REG(0xe6af, EP6FIFOBCH);
REG(0xe6b0, EP6FIFOBCL);
REG(0xe6b1, EP8FIFOBCH);
REG(0xe6b2, EP8FIFOBCL);
REG(0xe6b3, SUDPTRH);
REG(0xe6b4, SUDPTRL);
REG(0xe6b5, SUDPTRCTL);
REG(0xe6b8, SETUPDAT[8]);

#define EPCS_STALL			(1ul << 0)
#define EPCS_BUSY			(1ul << 1)
#define EPCS_EMPTY			(1ul << 2)
#define EPCS_FULL			(1ul << 3)
#define EPCS_NPAK0			(1ul << 4)
#define EPCS_NPAK1			(1ul << 5)
#define EPCS_NPAK2			(1ul << 6)
#define EPCS_HSNAK			(1ul << 7)

#define EPFIFOFLGS_FF			(1ul << 0)
#define EPFIFOFLGS_EF			(1ul << 1)
#define EPFIFOFLGS_PF			(1ul << 2)

#define SUDPTRCTL_SDPAUTO		(1ul << 0)

// Interrupts
REG(0xe650, EP2FIFOIE);
REG(0xe651, EP2FIFOIRQ);
REG(0xe652, EP4FIFOIE);
REG(0xe653, EP4FIFOIRQ);
REG(0xe654, EP6FIFOIE);
REG(0xe655, EP6FIFOIRQ);
REG(0xe656, EP8FIFOIE);
REG(0xe657, EP8FIFOIRQ);
REG(0xe658, IBNIE);
REG(0xe659, IBNIRQ);
REG(0xe65a, NAKIE);
REG(0xe65b, NAKIRQ);
REG(0xe65c, USBIE);
REG(0xe65d, USBIRQ);
REG(0xe65e, EPIE);
REG(0xe65f, EPIRQ);
REG(0xe660, GPIFIE);
REG(0xe661, GPIFIRQ);
REG(0xe662, USBERRIE);
REG(0xe663, USBERRIRQ);
REG(0xe664, ERRCNTLIM);
REG(0xe665, CLRERRCNT);
REG(0xe666, INT2IVEC);
REG(0xe667, INT4IVEC);
REG(0xe668, INTSETUP);

#define EPFIFOIE_FF			(1ul << 0)
#define EPFIFOIE_EF			(1ul << 1)
#define EPFIFOIE_PF			(1ul << 2)
#define EPFIFOIE_EDGEPF			(1ul << 3)

#define EPFIFOIRQ_FF			(1ul << 0)
#define EPFIFOIRQ_EF			(1ul << 1)
#define EPFIFOIRQ_PF			(1ul << 2)

#define IBNIE_EP0			(1ul << 0)
#define IBNIE_EP1			(1ul << 1)
#define IBNIE_EP2			(1ul << 2)
#define IBNIE_EP4			(1ul << 3)
#define IBNIE_EP6			(1ul << 4)
#define IBNIE_EP8			(1ul << 5)

#define IBNIRQ_EP0			(1ul << 0)
#define IBNIRQ_EP1			(1ul << 1)
#define IBNIRQ_EP2			(1ul << 2)
#define IBNIRQ_EP4			(1ul << 3)
#define IBNIRQ_EP6			(1ul << 4)
#define IBNIRQ_EP8			(1ul << 5)

#define NAKIE_IBN			(1ul << 0)
#define NAKIE_EP0			(1ul << 2)
#define NAKIE_EP1			(1ul << 3)
#define NAKIE_EP2			(1ul << 4)
#define NAKIE_EP4			(1ul << 5)
#define NAKIE_EP6			(1ul << 6)
#define NAKIE_EP8			(1ul << 7)

#define NAKIRQ_IBN			(1ul << 0)
#define NAKIRQ_EP0			(1ul << 2)
#define NAKIRQ_EP1			(1ul << 3)
#define NAKIRQ_EP2			(1ul << 4)
#define NAKIRQ_EP4			(1ul << 5)
#define NAKIRQ_EP6			(1ul << 6)
#define NAKIRQ_EP8			(1ul << 7)

#define USBIE_SUDAV			(1ul << 0)
#define USBIE_SOF			(1ul << 1)
#define USBIE_SUTOK			(1ul << 2)
#define USBIE_SUSP			(1ul << 3)
#define USBIE_URES			(1ul << 4)
#define USBIE_HSGRANT			(1ul << 5)
#define USBIE_EP0ACK			(1ul << 6)

#define USBIRQ_SUDAV			(1ul << 0)
#define USBIRQ_SOF			(1ul << 1)
#define USBIRQ_SUTOK			(1ul << 2)
#define USBIRQ_SUSP			(1ul << 3)
#define USBIRQ_URES			(1ul << 4)
#define USBIRQ_HSGRANT			(1ul << 5)
#define USBIRQ_EP0ACK			(1ul << 6)

#define EPIE_EP0IN			(1ul << 0)
#define EPIE_EP0OUT			(1ul << 1)
#define EPIE_EP1IN			(1ul << 2)
#define EPIE_EP1OUT			(1ul << 3)
#define EPIE_EP2			(1ul << 4)
#define EPIE_EP4			(1ul << 5)
#define EPIE_EP6			(1ul << 6)
#define EPIE_EP8			(1ul << 7)

#define EPIRQ_EP0IN			(1ul << 0)
#define EPIRQ_EP0OUT			(1ul << 1)
#define EPIRQ_EP1IN			(1ul << 2)
#define EPIRQ_EP1OUT			(1ul << 3)
#define EPIRQ_EP2			(1ul << 4)
#define EPIRQ_EP4			(1ul << 5)
#define EPIRQ_EP6			(1ul << 6)
#define EPIRQ_EP8			(1ul << 7)

#define GPIFIE_GPIFDONE			(1ul << 0)
#define GPIFIE_GPIFWF			(1ul << 1)

#define GPIFIRQ_GPIFDONE		(1ul << 0)
#define GPIFIRQ_GPIFWF			(1ul << 1)

#define USBERRIE_ERRLIMIT		(1ul << 0)
#define USBERRIE_ISOEP2			(1ul << 4)
#define USBERRIE_ISOEP4			(1ul << 5)
#define USBERRIE_ISOEP6			(1ul << 6)
#define USBERRIE_ISOEP8			(1ul << 7)

#define USBERRIRQ_ERRLIMIT		(1ul << 0)
#define USBERRIRQ_ISOEP2		(1ul << 4)
#define USBERRIRQ_ISOEP4		(1ul << 5)
#define USBERRIRQ_ISOEP6		(1ul << 6)
#define USBERRIRQ_ISOEP8		(1ul << 7)

#define INT2IVEC_IV0			(1ul << 2)
#define INT2IVEC_IV1			(1ul << 3)
#define INT2IVEC_IV2			(1ul << 4)
#define INT2IVEC_IV3			(1ul << 5)
#define INT2IVEC_IV4			(1ul << 6)

#define INT4IVEC_IV0			(1ul << 2)
#define INT4IVEC_IV1			(1ul << 3)
#define INT4IVEC_IV2			(1ul << 4)
#define INT4IVEC_IV3			(1ul << 5)
#define INT4IVEC_IV4			(1ul << 6)

#define INTSETUP_AV4EN			(1ul << 0)
#define INTSETUP_INT4IN			(1ul << 1)
#define INTSETUP_AV2EN			(1ul << 3)

// Input/Output
REG(0xe670, PORTACFG);
REG(0xe671, PORTCCFG);
REG(0xe672, PORTECFG);
REG(0xe678, I2CS);
REG(0xe679, I2DAT);
REG(0xe67a, I2CTL);
REG(0xe67b, XAUTODAT1);
REG(0xe67c, XAUTODAT2);

#define PORTACFG_INT0			(1ul << 0)
#define PORTACFG_INT1			(1ul << 1)
#define PORTACFG_SLCS			(1ul << 6)
#define PORTACFG_FLAGD			(1ul << 7)

#define PORTCCFG_GPIFA0			(1ul << 0)
#define PORTCCFG_GPIFA1			(1ul << 1)
#define PORTCCFG_GPIFA2			(1ul << 2)
#define PORTCCFG_GPIFA3			(1ul << 3)
#define PORTCCFG_GPIFA4			(1ul << 4)
#define PORTCCFG_GPIFA5			(1ul << 5)
#define PORTCCFG_GPIFA6			(1ul << 6)
#define PORTCCFG_GPIFA7			(1ul << 7)

#define PORTECFG_T0OUT			(1ul << 0)
#define PORTECFG_T1OUT			(1ul << 1)
#define PORTECFG_T2OUT			(1ul << 2)
#define PORTECFG_RXD0OUT		(1ul << 3)
#define PORTECFG_RXD1OUT		(1ul << 4)
#define PORTECFG_INT6			(1ul << 5)
#define PORTECFG_T2EX			(1ul << 6)
#define PORTECFG_GPIFA8			(1ul << 7)

#define I2CS_DONE			(1ul << 0)
#define I2CS_ACK			(1ul << 1)
#define I2CS_BERR			(1ul << 2)
#define I2CS_ID0			(1ul << 3)
#define I2CS_ID1			(1ul << 4)
#define I2CS_LASTRD			(1ul << 5)
#define I2CS_STOP			(1ul << 6)
#define I2CS_START			(1ul << 7)

#define I2CTL_400KHZ			(1ul << 0)
#define I2CTL_STOPIE			(1ul << 1)

// USB Control
REG(0xe680, USBCS);
REG(0xe681, SUSPEND);
REG(0xe682, WAKEUPCS);
REG(0xe683, TOGCTL);
REG(0xe684, USBFRAMEH);
REG(0xe685, USBFRAMEL);
REG(0xe686, MICROFRAME);
REG(0xe687, FNADDR);

#define USBCS_SIGRESUME			(1ul << 0)
#define USBCS_RENUM			(1ul << 1)
#define USBCS_NOSYNSOF			(1ul << 2)
#define USBCS_DISCON			(1ul << 3)
#define USBCS_HSM			(1ul << 7)

#define WAKEUPCS_WUEN			(1ul << 0)
#define WAKEUPCS_WU2EN			(1ul << 1)
#define WAKEUPCS_DPEN			(1ul << 2)
#define WAKEUPCS_WUPOL			(1ul << 4)
#define WAKEUPCS_WU2POL			(1ul << 5)
#define WAKEUPCS_WU			(1ul << 6)
#define WAKEUPCS_WU2			(1ul << 7)

#define TOGCTL_EP(x)			((x) << 0)
#define TOGCTL_IO			(1ul << 4)
#define TOGCTL_R			(1ul << 5)
#define TOGCTL_S			(1ul << 6)
#define TOGCTL_Q			(1ul << 7)

// GPIF
REG(0xe6c0, GPIFWFSELECT);
REG(0xe6c1, GPIFIDLECS);
REG(0xe6c2, GPIFIDLECTL);
REG(0xe6c3, GPIFCTLCFG);
REG(0xe6c4, GPIFADRH);
REG(0xe6c5, GPIFADRL);

REG(0xe6ce, GPIFTCB3);
REG(0xe6cf, GPIFTCB2);
REG(0xe6d0, GPIFTCB1);
REG(0xe6d1, GPIFTCB0);

REG(0xe6d2, EP2GPIFFLGSEL);
REG(0xe6d3, EP2GPIFPFSTOP);
REG(0xe6d4, EP2GPIFTRIG);
REG(0xe6da, EP4GPIFFLGSEL);
REG(0xe6db, EP4GPIFPFSTOP);
REG(0xe6dc, EP4GPIFTRIG);
REG(0xe6e2, EP6GPIFFLGSEL);
REG(0xe6e3, EP6GPIFPFSTOP);
REG(0xe6e4, EP6GPIFTRIG);
REG(0xe6ea, EP8GPIFFLGSEL);
REG(0xe6eb, EP8GPIFPFSTOP);
REG(0xe6ec, EP8GPIFTRIG);

REG(0xe6f0, XGPIFSGLDATH);
REG(0xe6f1, XGPIFSGLDATLX);
REG(0xe6f2, XGPIFSGLDATLNOX);
REG(0xe6f3, GPIFREADYCFG);
REG(0xe6f4, GPIFREADYSTAT);
REG(0xe6f5, GPIFABORT);

// UDMA
REG(0xe6c6, FLOWSTATE);
REG(0xe6c7, FLOWLOGIC);
REG(0xe6c8, FLOWEQ0CTL);
REG(0xe6c9, FLOWEQ1CTL);
REG(0xe6ca, FLOWHOLDOFF);
REG(0xe6cb, FLOWSTB);
REG(0xe6cc, FLOWSTBEDGE);
REG(0xe6cd, FLOWSTBHPERIOD);
REG(0xe60c, GPIFHOLDAMOUNT);
REG(0xe67d, UDMACRCH);
REG(0xe67e, UDMACRCL);
REG(0xe67f, UDMACRCQUAL);

// Endpoint Buffers
REG(0xe740, EP0BUF[64]);
REG(0xe780, EP1OUTBUF[64]);
REG(0xe7c0, EP1INBUF[64]);
REG(0xf000, EP2FIFOBUF[1024]);
REG(0xf400, EP4FIFOBUF[1024]);
REG(0xf800, EP6FIFOBUF[1024]);
REG(0xfc00, EP8FIFOBUF[1024]);

#endif // _FX2_REGS_H_

