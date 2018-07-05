#include "COMMON.h"
#include "UART_Code.h"

_FOSC( CSW_FSCM_OFF & XT_PLL4 );  
_FWDT( WDT_OFF );
_FBORPOR( PBOR_OFF & MCLR_EN );

#define Frc          			24000000         	//On-board Crystal frequency
#define PLLMODE                 4               	//On-chip PLL setting
#define MY_FCY                  Frc*PLLMODE/4       //
#define MICROSEC                1       // 1.5 us
#define MILLISEC                MICROSEC*4808       // 1.001 ms
#define BAUDRATE_19200          77                  //BAUDRATE==UxBRG (TEST BOARD)

//////////////////////////////////////////////
//        Fcy/(16(UxBRG+1))               //
//        ((Fcy/BAUDRATE)/16)-1         //
//////////////////////////////////////////////

//unsigned char UartRxBuffer[RXBUF_LEN];
//unsigned char UartTxBuffer[TXBUF_LEN];
//unsigned int UartRxBufWrPt = 0;
//unsigned int UartRxBufRdPt = 0;
//unsigned int UartTxBufWrPt = 0;
//unsigned int UartTxBufRdPt = 0;
//char g_cmd_buf[MAX_CMD_BUF];
//unsigned long g_cmd_pos = 0;
//unsigned int Err_Count=0;

#define CMD_NONE        0
#define CMD_RST         1
#define CMD_SET         2
#define CMD_RUN         3
#define CMD_SHOW        4
#define CMD_CHMOD       5
#define CMD_HDELAY      6
#define CMD_ALLON       7

#define CMD_COUNT       20

#define UNMOD_MS        0
#define UNMOD_US        1

char CommandBuf[RXCMDBUF_LEN];
unsigned int CommandBufWrPt = 0;
unsigned int CommandBufRdPt = 0;

unsigned int m_xOut[20] = {0,};
unsigned int m_timeWait[20] = {0, };
unsigned int m_setedList[20] = {0, };
unsigned int m_xOut0 = 0;
unsigned int m_xOut0_Calc = 0;

unsigned int m_nUnitMode = UNMOD_MS;
unsigned int m_nHighDelay = 25;

unsigned int CommandIndex = CMD_NONE;


void Init_INT(void)
{       // 7 max priority
    IPC2bits.U1TXIP = 5;	//UART1  Priority
    IPC2bits.U1RXIP = 7;	//UART1  Priority
    IPC0bits.T1IP=6;          //Timer 1  Priority
}

void InitUART(void)
{
    CloseUART1();
    ConfigIntUART1(UART_RX_INT_EN &			// EN, DIS
				UART_RX_INT_PR4 &			// 0 ~ 7
				UART_TX_INT_DIS &			// EN, DIS
				UART_TX_INT_PR4);			// 0 ~ 7

    OpenUART1(	UART_EN &					// EN, DIS
				UART_IDLE_STOP &			// CON, STOP
				UART_EN_WAKE &				// EN_WAKE, DIS_WAKE
				UART_DIS_LOOPBACK &			// EN_LOOPBACK, DIS_LOOPBACK
				UART_DIS_ABAUD &			// EN_ABAUD, DIS_ABAUD
				UART_NO_PAR_8BIT &			// NO_PAR_9BIT, NO_PAR_8BIT, ODD_PAR_8BIT, EVEN_PAR_8BIT
				UART_1STOPBIT,				// 2STOPBITS, 1STOPBIT

				UART_INT_TX &				// INT_BUF_EMPTY, INT_TX
				UART_TX_PIN_NORMAL &		// NORMAIL, LOW
				UART_TX_ENABLE &			// ENABLE, DISABLE
				UART_INT_RX_CHAR &			// RX_BUF_FUL, RX_3_4_FUL, RX_CHAR
				UART_ADR_DETECT_DIS &		// EN, DIS
				UART_RX_OVERRUN_CLEAR,		// CLEAR
				BAUDRATE_19200);			// 16=115200BPS, 34=57600BPS, 51=38400, 103=19200, Fcy/(16(UxBRG+1))
											// ((Fcy/BAUDRATE)/16)-1
     U1MODEbits.ALTIO=1;
}

void IO_Init(void)
{
    ADPCFG = 0xFFFF;
    TRISB=0x00;
    PORTB=0x00;
    TRISD=0x00;
    PORTD=0x00;
    TRISE=0x00;
    PORTE=0x00;
    TRISFbits.TRISF0 = 0;
    TRISFbits.TRISF1 = 0;    
}

int main(void)
{    
    __delay_ms(100);
    InitUART();    
    __delay_ms(100);
    IO_Init();    
    __delay_ms(100);    
    Init_INT();    
    __delay_ms(100);    
    
    Uart1Welcome();
    __delay_ms(200);
    
//    _wait_eedata();
//    _memcpy_p2d16(&m_nHighDelay, 0x0D, sizeof(m_nHighDelay));
    
    while(1)
    {
        __delay_ms(1);
        switch (CommandIndex)
        {
            case CMD_RUN:
            {
                Run();
                CommandIndex = CMD_NONE;
            }
            break;
            
            case CMD_SHOW:
            {
                ShowList();
                CommandIndex = CMD_NONE;
            }
            break;
        }
    }
}

void __attribute__((interrupt, auto_psv)) _U1RXInterrupt(void)
{
    IFS0bits.U1RXIF = 0;
    IEC0bits.U1RXIE = 0;
    
    if (CommandIndex != CMD_RUN || CommandIndex != CMD_SHOW)
    {
        char c = getcUART1();
        CommandBuf[CommandBufWrPt++] = c;

        if (c == 0x0d)
        {
            char *retSET = strstr(CommandBuf, "SET");
            char *retRST = strstr(CommandBuf, "RST");
            char *retRUN = strstr(CommandBuf, "RUN");
            char *strSHOW = strstr(CommandBuf, "SHOW");
            char *strCHMOD = strstr(CommandBuf, "CHMOD");
            char *strHDELAY = strstr(CommandBuf, "HDELAY");
            char *strTEST12 = strstr(CommandBuf, "TEST12");
            char *strTEST24 = strstr(CommandBuf, "TEST24");
            char *strTESTALLON = strstr(CommandBuf, "ALLON");            

            if (retSET != NULL)
            {
                CommandIndex = CMD_SET;
                if (SetList(retSET) == -1)
                {
                    UART1_String("ERROR\n\r");
                }
                else
                {
                    UART1_String(retSET);
                }
                CommandIndex = CMD_NONE;
            }
            else if (retRST != NULL)
            {
                CommandIndex = CMD_RST;
                ResetList();
                UART1_String(retRST);
                CommandIndex = CMD_NONE;
            }
            else if (retRUN != NULL)
            {
                CommandIndex = CMD_RUN;            
                UART1_String(retRUN);
            }
            else if (strSHOW != NULL)
            {
                CommandIndex = CMD_SHOW;
            }
            else if (strCHMOD != NULL)
            {
                CommandIndex = CMD_CHMOD;
                SetChangeMode(strCHMOD);
                UART1_String(strCHMOD);
                CommandIndex = CMD_NONE;
            }
            else if (strHDELAY != NULL)
            {
                CommandIndex = CMD_HDELAY;
                SetHighDelayTime(strHDELAY);
                UART1_String(retRST);
                CommandIndex = CMD_NONE;
            }
            else if (strTEST24 != NULL)
            {
                Xout_H_(65535);
                Xout_L_(65535);
            }
            else if (strTEST12 != NULL)
            {
                Xout_H_(0);
                Xout_L_(65535);
            }
            else if (strTESTALLON)
            {
                Xout_H_(65535);
                TimeDelay(m_nHighDelay);
                Xout_L_(65535);
            }            
            else
            {
                CommandIndex = CMD_NONE;
                UART1_String(CommandBuf);
            }

            CommandBufWrPt = 0;
            memset(CommandBuf, 0, sizeof(CommandBuf));
        }
        else if (CommandBufWrPt >= RXCMDBUF_LEN)
        {
            UART1_String("Over length!\n\r");
            CommandBufWrPt = 0;
            memset(CommandBuf, 0, sizeof(CommandBuf));
        }
    }
    else
    {
        UART1_String("RUNNING\n\r");
    }
    
    IEC0bits.U1RXIE = 1;
}

int SetChangeMode(char *recv)
{    
    if (recv[6] == 'm')
    {
        m_nUnitMode = UNMOD_MS;
        m_nHighDelay = 25;
        ResetList();
    }
    else if (recv[6] == 'u')
    {
        m_nUnitMode = UNMOD_US;
        m_nHighDelay = 200;
        ResetList();
    }
}

int SetHighDelayTime(char *recv)
{           
    char prtbuf[100];
    
    char temp[20] = {0, };
    int dataRdCnt = 7;
    int tempCnt = 0;
    while (recv[dataRdCnt] != 0x0d)
    {
        temp[tempCnt++] = recv[dataRdCnt++];
    }
    
    if (tempCnt > 0)
    {
        m_nHighDelay = atoi(temp);
        sprintf(prtbuf, "High Delay : %d\n\r", m_nHighDelay);
        UART1_String(prtbuf);
        
//        _write_eedata_word(0x0D, m_nHighDelay);
//        _wait_eedata();
        
        UART1_String("Save eeprom\n\r");
    }
    
    return 0;
}

int SetList(char *recv)
{    
    char temp[20] = {0, };
    int dataRdCnt = 4;
    int tempCnt = 0;
    int setDataIndex = 0;
    int listInd = -1;
    int checkSize = 0, check_i = 0, isErrorDigit = 0;
    while (recv[dataRdCnt] != 0x0d)
    {        
        if (recv[dataRdCnt] == ' ')
        {
            switch (setDataIndex++)
            {
                case 0:
                    checkSize = strlen(temp);
                    if (checkSize > 0)
                    {
                        for (check_i = 0; check_i < checkSize; check_i++)
                        {
                            if (isdigit(temp[check_i]) == 0)
                            {
                                UART1_String("Error digit\n\r");
                                isErrorDigit = 1;
                                break;
                            }
                        }
                        if (isErrorDigit == 0)
                        {
                            listInd = atoi(temp);
                            if (listInd < 0)        listInd = 0;
                            else if (listInd > 19)  listInd = 19;
                            m_setedList[listInd] = 1;
                        }
                    }
                    break;
                    
                case 1:
                    if (listInd != -1 && isErrorDigit == 0)
                    {
                        m_xOut[listInd] = strtol(temp, NULL, 0);
                    }                    
                    break;
            }
            tempCnt = 0;
            memset(temp, 0, sizeof(temp));
        }
        else
        {
            temp[tempCnt++] = recv[dataRdCnt];            
        }
        dataRdCnt++;
    }
    
    if (tempCnt != 0 && setDataIndex == 2)
    {
        if (listInd != -1)
        {
            m_timeWait[listInd] = atoi(temp);
        }
    }
    else
    {
        return -1;
    }
    
    if (RunCalc() == -1)
    {
        if (listInd != -1)
        {
            m_setedList[listInd] = 0;
            m_xOut[listInd] = 0;
            m_timeWait[listInd] = 0;
            
            m_xOut0_Calc = 0;
            
            return -1;
        }
    }
    
    m_xOut0_Calc = 0;
    
    return 0;
}

int ResetList(void)
{
    memset(m_xOut, 0, sizeof(m_xOut));
    memset(m_timeWait, 0, sizeof(m_timeWait));
    memset(m_setedList, 0, sizeof(m_setedList));  
    
    Xout_H_(0);
    Xout_L_(0);
}

int ShowList(void)
{
    char prtbuf[100];
    int i = 0;
    
    UART1_String("\n\r");
    for (i = 0; i < CMD_COUNT; i++)
    {
        sprintf(prtbuf, "%01d\t: %u\t%X\t%u\n\r", i, m_setedList[i], m_xOut[i], m_timeWait[i]);
        UART1_String(prtbuf);
    }
    
    sprintf(prtbuf, "HighDelay : %d\n\rUnitMode : %d\n\r", m_nHighDelay, m_nUnitMode);
    UART1_String(prtbuf);
}

int RunCalc(void)
{
    int n;
    for (n = 0; n < CMD_COUNT; n++)
    {
        if (m_setedList[n] == 1)
        {
            if (CalcXOut(m_xOut[n], m_timeWait[n]) == -1)
                return -1;
        }
    }    
    return 0;
}

int CalcXOut(unsigned int xOut, unsigned int timeWait)
{
    unsigned int outHigh = xOut & (m_xOut0_Calc ^ 0xffff);
    if (outHigh > 0)
    {
        if (timeWait <= m_nHighDelay)
        {
            return -1;
        }
    }
    
    m_xOut0_Calc = xOut;
    
    return 0;
}

int Run(void)
{
    int n;
    for (n = 0; n < CMD_COUNT; n++)
    {   
        if (m_setedList[n] == 1)
        {
            RunXOut(m_xOut[n], m_timeWait[n]);            
        }
    }
    
    Xout_H_(0);
    Xout_L_(0);
    m_xOut0 = 0;
    UART1_String("DONE\n\r");
    return 0;
}

int RunXOut(unsigned int xOut, unsigned int timeWait)
{
    unsigned int outHigh = xOut & (m_xOut0 ^ 0xffff);
    
    Xout_H_(outHigh);
    Xout_L_(xOut);
    if (outHigh > 0)
    {        
        TimeDelay(m_nHighDelay);
        Xout_H_(0);
        if (timeWait >= m_nHighDelay) 
            TimeDelay(timeWait - m_nHighDelay);        
    }
    else
    {
        TimeDelay(timeWait);
    }
    
    m_xOut0 = xOut;
    
    return 0;
}

int TimeDelay(unsigned int timeWait)
{
    if (m_nUnitMode == UNMOD_MS)
    {
        __delay_ms(timeWait);
    }        
    else                            
    {
        __delay_us(timeWait);
    }
        
    return 0;
}

int Xout_H_(unsigned int xOut)
{   
    unsigned int xBit = xOut;    
    LATDbits.LATD3 = (xBit & 0x0001); // 1ch
    LATDbits.LATD5 = ((xBit >> 1) & 0x0001); // 2ch
    LATDbits.LATD7 = ((xBit >> 2) & 0x0001); // 3ch
    LATFbits.LATF1 = ((xBit >> 3) & 0x0001); // 4ch
    LATEbits.LATE1 = ((xBit >> 4) & 0x0001); // 5ch
    LATEbits.LATE3 = ((xBit >> 5) & 0x0001); // 6ch
    LATEbits.LATE5 = ((xBit >> 6) & 0x0001); // 7ch
    LATEbits.LATE7 = ((xBit >> 7) & 0x0001); // 8ch
    LATBbits.LATB2 = ((xBit >> 8) & 0x0001); // 9ch
    LATBbits.LATB0 = ((xBit >> 9) & 0x0001); // 10ch
    LATBbits.LATB8 = ((xBit >> 10) & 0x0001); // 11ch
    LATBbits.LATB10 = ((xBit >> 11) & 0x0001); // 12ch
    LATBbits.LATB12 = ((xBit >> 12) & 0x0001); // 13ch
    LATBbits.LATB14 = ((xBit >> 13) & 0x0001); // 14ch
    LATDbits.LATD8 = ((xBit >> 14) & 0x0001); // 15ch
    LATDbits.LATD10 = ((xBit >> 15) & 0x0001); // 16ch
    
    return 0;
}

int Xout_L_(unsigned int xOut)
{   
    unsigned int xBit = xOut; 
    LATDbits.LATD2 = (xBit & 0x0001); // 1ch
    LATDbits.LATD4 = ((xBit >> 1) & 0x0001); // 2ch
    LATDbits.LATD6 = ((xBit >> 2) & 0x0001); // 3ch
    LATFbits.LATF0 = ((xBit >> 3) & 0x0001); // 4ch
    LATEbits.LATE0 = ((xBit >> 4) & 0x0001); // 5ch
    LATEbits.LATE2 = ((xBit >> 5) & 0x0001); // 6ch 
    LATEbits.LATE4 = ((xBit >> 6) & 0x0001); // 7ch
    LATEbits.LATE6 = ((xBit >> 7) & 0x0001); // 8ch
    LATBbits.LATB3 = ((xBit >> 8) & 0x0001); // 9ch
    LATBbits.LATB1 = ((xBit >> 9) & 0x0001); // 10ch
    LATBbits.LATB9 = ((xBit >> 10) & 0x0001); // 11ch
    LATBbits.LATB11 = ((xBit >> 11) & 0x0001); // 12ch
    LATBbits.LATB13 = ((xBit >> 12) & 0x0001); // 13ch
    LATBbits.LATB15 = ((xBit >> 13) & 0x0001); // 14ch
    LATDbits.LATD9 = ((xBit >> 14) & 0x0001); // 15ch    
    LATDbits.LATD11 = ((xBit >> 15) & 0x0001); // 16ch
    
    return 0;
}
