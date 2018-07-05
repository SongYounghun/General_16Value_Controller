/*
 * File:   COMMON.h
 * Author: Administrator
 *
 * Created on 2016? 6? 21? (?), ?? 2:55
 */

#ifndef COMMON_H
#define	COMMON_H

/*
 * __delay_ms() and __delay_us() are defined as macros. They depend
 * on a user-supplied definition of FCY. If FCY is defined, the argument
 * is converted and passed to __delay32(). Otherwise, the functions
 * are declared external.
 *
 * For example, to declare FCY for a 10 MHz instruction rate:
 *
 * #define FCY 10000000UL
 */
#define FCY 24000000UL

#include <p30fxxxx.h>		//Include gld
#include <libpic30.h>
#include <stdio.h>
#include <stdlib.h>
#include <uart.h>
#include <adc10.h>
#include <timer.h>
#include <string.h>
#include <pwm.h>
#include <ctype.h>
//#include "MY_LCD.h"

#define RXBUF_LEN       512
#define TXBUF_LEN       512

#define RXCMDBUF_LEN    1024



#endif	/* COMMON_H */

