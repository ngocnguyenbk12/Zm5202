/*******************************  ZW_PORTDEFS.H  *******************************
 *           #######
 *           ##  ##
 *           #  ##    ####   #####    #####  ##  ##   #####
 *             ##    ##  ##  ##  ##  ##      ##  ##  ##
 *            ##  #  ######  ##  ##   ####   ##  ##   ####
 *           ##  ##  ##      ##  ##      ##   #####      ##
 *          #######   ####   ##  ##  #####       ##  #####
 *                                           #####
 *          Z-Wave, the wireless lauguage.
 *
 *              Copyright (c) 2001
 *              Zensys A/S
 *              Denmark
 *
 *              All Rights Reserved
 *
 *    This source file is subject to the terms and conditions of the
 *    Zensys Software License Agreement which restricts the manner
 *    in which it may be used.
 *
 *---------------------------------------------------------------------------
 *
 * Description: IO definitions for the ZW0102 based Z-Wave Standard Module
 *
 * Author:   Ivar Jeppesen
 *
 * Last Changed By:  $Author: efh $
 * Revision:         $Revision: 23353 $
 * Last Changed:     $Date: 2012-09-14 12:56:15 +0200 (fr, 14 sep 2012) $
 *
 ****************************************************************************/
#ifndef _ZW_PORTDEFS_H_
#define _ZW_PORTDEFS_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/


/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/
/* TODO_ZW040x */
#if defined(ZW020x) || defined(ZW030x)

/* I/O Port initialization values */

/* Port 0 *******/
/* 0  I ZeroX   Triac ZeroX input                   */
/* 1  I Fire    Triac Pulse output      */
#define P0_INIT     0x00
#define P0DIR_INIT  0xFF
#define P0PULLUP_INIT 0x00


/* Port 1 **********************/
/* 0  I Tx Uart Output line  */
/* 1  I Rx UART input line   */
/* 2  I MISO                 */
/* 3  I MOSI                 */
/* 4  I SCK                  */
/* 5  I SS_N                 */
/* 6  I Int0                 */
/* 7  I Int1                 */
#define P1_INIT     0x00
#define P1DIR_INIT  0xFF
#define P1PULLUP_INIT 0x00

#endif

#ifdef ZW040x
/* I/O Port initialization values */

/* Port 0 *******/
/* 0  GPIO  */
/* 1  GPIO  */
/* 2  GPIO  */
/* 3  GPIO  */
/* 4  GPIO  */
/* 5  GPIO  */
/* 6  GPIO  */
/* 7  GPIO  */
#define P0_INIT     0x00
#define P0DIR_INIT  0xFF
#define P0PULLUP_INIT 0x00


/* Port 1 **********************/
/* 0  GPIO  */
/* 1  GPIO  */
/* 2  GPIO  */
/* 3  GPIO  */
/* 4  GPIO  */
/* 5  GPIO  */
/* 6  GPIO  */
/* 7  GPIO  */
#define P1_INIT     0x00
#define P1DIR_INIT  0xFF
#define P1PULLUP_INIT 0x00


/* Port 2 **********************/
/* 0  GPIO, UART0 Rx  */
/* 1  GPIO, UART0 Tx  */
/* 2  GPIO, SPI1 MOSI */
/* 3  GPIO, SPI1 MISO */
/* 4  GPIO  SPI1 SCK  */
/* 5  GPIO  SPI0 MOSI */
/* 6  GPIO  SPI0 MISO */
/* 7  GPIO  SPI0 SCK  */

#define P2_INIT     0x00
#define P2DIR_INIT  0xFF
#define P2PULLUP_INIT 0x00

/* 0  GPIO, UART1 Rx, SPI0 SS_N  */
/* 1  GPIO, UART1 Tx, IR Tx      */
/* 4  GPIO, IR Tx, ADC0          */
/* 5  GPIO, IR Tx, ADC0, PWM(IR) */
/* 6  GPIO  IR Rx, ADC2, TRIAC   */
/* 7  GPIO  PWM, ADC3, ZEROX     */

#define P3_INIT     0x00
#define P3DIR_INIT  0xFF
#define P3PULLUP_INIT 0x00

#endif


#endif /* _ZW_PORTDEFS_H_ */
