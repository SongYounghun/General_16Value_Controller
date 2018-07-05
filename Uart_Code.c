#include <p30Fxxxx.h>		//Include gld
#include <stdio.h>
#include <math.h>
#include <uart.h>
#include <spi.h>
#include <adc10.h>
#include <pwm.h>
#include <qei.h>
#include <timer.h>
#include <stdlib.h>
#include <string.h>
#include "UART_Code.h"
#include "COMMON.h"

//extern unsigned char UartRxBuffer[RXBUF_LEN];
//extern unsigned char UartTxBuffer[TXBUF_LEN];
//
//extern unsigned int UartRxBufWrPt;
//extern unsigned int UartRxBufRdPt;
//extern unsigned int UartTxBufWrPt;
//extern unsigned int UartTxBufRdPt;

extern char CommandBuf[RXCMDBUF_LEN];
extern unsigned int CommandBufWrPt;
extern unsigned int CommandBufRdPt;


void Uart1Welcome(void)
{
    char prtbuf[100];    

    sprintf(prtbuf, "Test version: V0.0, Date: 18.04.16\r\n");    
    UART1_String(prtbuf);

    sprintf(prtbuf, "Macrogen 16Chnnel Valve Controller. \r\n");
    UART1_String(prtbuf);

    
}

void UART1_String(char *buf)
{
    putsUART1((unsigned int*)buf);
}


