/******************** (C) COPYRIGHT 2015 SONiX *******************************
* COMPANY:			SONiX
* DATE:					2015/08
* AUTHOR:				SA1
* IC:						SN32F700B
* DESCRIPTION:	UART1 related functions.
*____________________________________________________________________________
*	REVISION	Date				User	Description
*	1.0				2015/08/14	SA1		First release
*
*____________________________________________________________________________
* THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS TIME TO MARKET.
* SONiX SHALL NOT BE HELD LIABLE FOR ANY DIRECT, INDIRECT OR CONSEQUENTIAL 
* DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE CONTENT OF SUCH SOFTWARE
* AND/OR THE USE MADE BY CUSTOMERS OF THE CODING INFORMATION CONTAINED HEREIN 
* IN CONNECTION WITH THEIR PRODUCTS.
*****************************************************************************/

/*_____ I N C L U D E S ____________________________________________________*/
#include <SN32F700B.h>
#include "UART.h"
#include "..\..\Utility\Utility.h"


/*_____ D E C L A R A T I O N S ____________________________________________*/
volatile uint8_t bUART1_RecvNew;
uint32_t GulNum1;
uint8_t  bUART1_RecvFIFO[(RecvSize << 1)];
uint8_t  bUART1_TranFIFO[(RecvSize << 1)];


/*_____ D E F I N I T I O N S ______________________________________________*/


/*_____ M A C R O S ________________________________________________________*/


/*_____ F U N C T I O N S __________________________________________________*/

/*****************************************************************************
* Function		: UART1_IRQHandler
* Description	: UART1 interrupt service routine
* Input			: None
* Output		: None
* Return		: None
* Note			: None
*****************************************************************************/
__irq void UART1_IRQHandler (void)
{
	uint32_t II_Buf, LS_Buf;
	volatile uint32_t Null_Buf;

	II_Buf = SN_UART1->II;
	while ((II_Buf & mskUART_II_STATUS) == UART_II_STATUS)		//check interrupt status, the INT can be determined by UARTn_II[3:1]
	{
		switch ((II_Buf>>1) & mskUART_INTID_STATUS)
		{
			case UART_RLS:		//Receive Line Status
				LS_Buf = SN_UART1->LS;
				if((LS_Buf & mskUART_LS_OE) ==  UART_LS_OE) 	//Overrun Error
				{ }
				if((LS_Buf & mskUART_LS_RXFE) == UART_LS_RXFE)//RX FIFO Error
				{
					if((LS_Buf & mskUART_LS_PE) == UART_LS_PE)//Parity Error
						Null_Buf = SN_UART1->RB;	//Clear interrupt
					if((LS_Buf & mskUART_LS_FE) == UART_LS_FE)	//Framing Error
						Null_Buf = SN_UART1->RB;	//Clear interrupt
					if((LS_Buf & mskUART_LS_BI) == UART_LS_BI)	//Break Interrupt
						Null_Buf = SN_UART1->RB;	//Clear interrupt
				}
				break;

      case UART_RDA:		//Receive Data Available
				LS_Buf = SN_UART1->LS;
                                               
				if((LS_Buf & mskUART_LS_RDR) == UART_LS_RDR)//Receiver Data Ready
				{
					bUART1_RecvFIFO[GulNum1] = SN_UART1->RB;
					GulNum1++;
				}
				if(GulNum1 == 6)
				{
					GulNum1 = 0;
					bUART1_RecvNew = 1; 
				}
        break;
            
			case UART_THRE:		//THRE interrupt
				LS_Buf = SN_UART1->LS;
				if((LS_Buf & mskUART_LS_THRE) == UART_LS_THRE)//THRE empty
				{	//SN_UART1->TH = Null_Buf; 	//Clear interrupt
				}
				break;

			case UART_TEMT:		//TEMT interrupt
				LS_Buf = SN_UART1->LS;
				if((LS_Buf & mskUART_LS_TEMT) == UART_LS_TEMT)
				{	//SN_UART1->TH = Null_Buf; 	//Clear interrupt
				}
				break;
			
			default:
         break;
    } //end switch ((II_Buf>>1) & mskUART_INTID_STATUS)

		II_Buf = SN_UART1->II;
  }	//end  while ((II_Buf&0x01) == mskUART_II_STATUS)

	if ((II_Buf & mskUART_II_ABEOIF) ==  UART_II_ABEOIF) //Auto Baud interrupt
		SN_UART1->ABCTRL |= UART_ABEO_EN;
	else if((II_Buf & mskUART_II_ABTOIF) ==  UART_II_ABTOIF) //Auto Baud time-out interrupt
		SN_UART1->ABCTRL |= UART_ABTO_EN;
}


/*****************************************************************************
* Function		: UART1_Init
* Description	: Initialization of UART1
* Input			: None
* Output		: None
* Return		: None
* Note			: None
*****************************************************************************/
void UART1_Init (void)
{
		SN_SYS1->AHBCLKEN |= UART1_CLK_EN; // Enables clock for UART1

		SN_UART1->LC = (
									UART_CHARACTER_LEN8BIT // 8bit character length.
								|	UART_STOPBIT_1BIT // stop bit of 1 bit
								|	UART_PARITY_BIT_EN // parity bit is enable
								|	UART_PARITY_BIT_DISEN // parity bit is disable
								|	UART_PARITY_SELECTODD // parity bit is odd
								|	UART_BREAK_DISEN // Break Transmission control disable
								|	UART_DIVISOR_EN); // Divisor Latch Access enable

		// Baud Rate Calculation
		/*
			UART_OVER_SAMPLE = 16
			UART_MULVAL = 8 ( Fill 7 in SN_UARTn->FD )
			UART_DIVADDVAL = 0
			DLM = 0
			DLL=12M/(BR*16*1)
			if require BR=9600, then DLL = 78.125 => 78
		*/
		
		/*
			// UART PCLK = 12MHz, Baud rate = 115200
			SN_UART1->FD = (UART_OVER_SAMPLE_16|UART_MULVAL_7|UART_DIVADDVAL_5);
			SN_UART1->DLM = 0;
			SN_UART1->DLL  = 4;
		*/
		
		/*
			// UART PCLK = 12MHz, Baud rate = 57600
			SN_UART1->FD = (OVER_SAMPLE_16|UART_MULVAL_7|UART_DIVADDVAL_5);
			SN_UART1->DLM  = 0;
			SN_UART1->DLL  = 8;
		*/
		
		// UART PCLK = 12MHz, Baud rate = 9600
		SN_UART1->FD = (UART_OVER_SAMPLE_16|UART_MULVAL_7|UART_DIVADDVAL_0);
		SN_UART1->DLM  = 0;
		SN_UART1->DLL  = 78;
		SN_UART1->LC &= ~(UART_DIVISOR_EN); // Disable divisor latch
	
		// Auto Baud Rate
		// UART1_Autobaudrate_Init(); //Auto buad rate initial

		// FIFO Control
		SN_UART1->FIFOCTRL =(
												UART_FIFO_ENABLE // Enable UART FIFOs
											|	UART_RXFIFO_RESET // RX FIFO Reset
											|	UART_TXFIFO_RESET // TX FIFO Reset
											|	UART_RXTRIGGER_LEVEL1 // RX Trigger Level(1/4/8/14 characters)
							);

		// Scratch Pad
		// SN_UART1->SP = 0; // A readable, writable byte

		// Oversampling
		// SN_UART1->FD |= UART_OVER_SAMPLE_8; // OVER8(Oversampling Value), 1:Oversampling by 8. 0:Oversampling by 16

		// Half-duplex
		// SN_UART1->HDEN = 1; // Half-duplex mode enable

		// Interrupt Enable
		UART1_InterruptEnable();

		// UART Control
		SN_UART1->CTRL =(
										UART_EN // Enable UART0
									|	UART_RX_EN // Enable RX
									|	UART_TX_EN // Enable TX
						);
		// NVIC
		NVIC_EnableIRQ(UART1_IRQn); // Enable UART1 INT
}

/*****************************************************************************
* Function		: UART1_SendByte
* Description	: MCU sends Byte through UTXD1
* Input			: ucDat - data to be sent
* Output		: None
* Return		: None
* Note			: None
*****************************************************************************/
void UART1_SendByte (uint8_t ucDat)
{
	SN_UART1->TH = ucDat; 
	while ((SN_UART1->LS & 0x40) == 0);
}

/*****************************************************************************
* Function		: UART1_AutoBaudrateInit
* Description	: Initialization of UART1 Auto baud rate.
* Input			: None
* Output		: None
* Return		: None
* Note			: None
*****************************************************************************/
void UART1_AutoBaudrateInit(void)
{
	SN_UART1->ABCTRL =(UART_ABTO_EN										//Clear Auto Baud Time-out interrupt
										| UART_ABEO_EN										//Clear Auto Baud interrupt
										| UART_ABCCTRL_RESTART						//Restart in case of time-out
										| UART_ABCCTRL_MODE1							//Auto Baud mode, 0:mode 0, 1:mode 1
										|	UART_ABCCTRL_START);						//Auto Baud start, 0:stop(not running), 1:start(running)
}

/*****************************************************************************
* Function		: UART1_Enable
* Description	: Enable UART1
* Input			: None
* Output		: None
* Return		: None
* Note			: None
*****************************************************************************/
void UART1_Enable(void)
{
	//Enable HCLK for UART1
	SN_SYS1->AHBCLKEN |= UART1_CLK_EN;						//Enables clock for UART1
	SN_UART1->CTRL_b.UARTEN = UART_CTRL_EN;				//UART enable bit
}

/*****************************************************************************
* Function		: UART1_Disable
* Description	: Disable UART1
* Input			: None
* Output		: None
* Return		: None
* Note			: None
*****************************************************************************/
void UART1_Disable(void)
{
	SN_UART1->CTRL_b.UARTEN = UART_CTRL_DIS;		//UART disable
	//Disable HCLK for UART1
	SN_SYS1->AHBCLKEN &= ~(UART1_CLK_EN);				//Disable clock for UART1
}

/*****************************************************************************
* Function		: UART1_InterruptEnable
* Description	: Interrupt Enable 
* Input			: None
* Output		: None
* Return		: None
* Note			: None
*****************************************************************************/
void UART1_InterruptEnable(void)
{
	SN_UART1->IE =(UART_RDAIE_EN		//Enables the Receive Data Available(RDA) interrupt	
								|	UART_THREIE_EN		//Enable THRE interrupt					
								|	UART_RLSIE_EN		//Enable Receive Line Status(RLS) interrupt									
								|	UART_TEMTIE_EN		//Enable TEMT interrupt
								|	UART_ABEOIE_EN		//Enable Auto Baud interrupt
								|	UART_ABTOIE_EN);		//Enable Auto Baud time-out interrupt
}

