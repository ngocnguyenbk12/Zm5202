/****************************************************************************
 *
 * Copyright (c) 2001-2011
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description: In/Out definitions Z-Wave Single Chips
 *
 * Author:   Ivar Jeppesen
 *
 * Last Changed By:  $Author: efh $
 * Revision:         $Revision: 23305 $
 * Last Changed:     $Date: 2012-09-04 08:36:50 +0200 (ti, 04 sep 2012) $
 *
 ****************************************************************************/
#ifndef _ZW_PINDEFS_H_
#define _ZW_PINDEFS_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#ifdef __C51__
#include <ZW_typedefs.h>
#endif /* __C51__ */

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/
/*!!!WARNING DO NOT MODIFY THESE VARIABLES ONLY USED BY THE Z-WAVE STACK !!!*/
#ifdef __C51__
extern IBYTE P0Shadow;
extern IBYTE P1Shadow;
extern IBYTE P2Shadow;
extern IBYTE P3Shadow;

extern IBYTE P0ShadowDIR;
extern IBYTE P1ShadowDIR;
extern IBYTE P2ShadowDIR;
extern IBYTE P3ShadowDIR;
#endif /* __C51__ */


#ifdef __C51__
/* Macros for I/O Port controlling */

/* Set I/O pin as input:
 *    pin     - Z-Wave pin name
 *    pullup  - if not zero activate the internal pullup resistor
 */
#if defined (ZW040x) || defined(ZW050x)
/*Fix TO#02731*/
#define PIN_IN(pin,pullup)  {pin##DIR_PAGE;(pin##SHADOWDIR |= (1<<pin)); (pullup)?(pin##SHADOW &=~(1<<pin)):(pin##SHADOW |= (1<<pin));pin##Port = pin##SHADOW;pin##DIR = pin##SHADOWDIR;}
#endif /* #if defined (ZW040x) || defined(ZW050x) */


/* Set I/O pin as output:
 *    pin     - Z-Wave pin name
 */
#if defined (ZW040x) || defined(ZW050x)
#define PIN_OUT(pin)  {pin##DIR_PAGE;(pin##SHADOWDIR &=~(1<<pin)) ;(pin##DIR = pin##SHADOWDIR);}
#endif /* #if defined (ZW040x) || defined(ZW050x) */


/* Read pin value:
 *    pin     - Z-Wave pin name
 */
#define PIN_GET(pin)  (pin##Port & (1<<pin))

/* Set output pin to 1:
 *    pin     - Z-Wave pin name
 */
 /*Fix TO#02731*/
#define PIN_ON(pin)  {pin##SHADOW |= (1<<pin);pin##Port = pin##SHADOW;}
#define PIN_HIGH(pin) {pin##SHADOW |= (1<<pin);pin##Port = pin##SHADOW;}

/* Set output pin to 0:
 *    pin     - Z-Wave pin name
 */
 /*Fix TO#02731*/
#define PIN_OFF(pin) {pin##SHADOW &= ~(1<<pin);pin##Port = pin##SHADOW;}
#define PIN_LOW(pin) {pin##SHADOW &= ~(1<<pin);pin##Port = pin##SHADOW;}

/* Toggle output pin:
 *    pin     - Z-Wave pin name
 */
 /*Fix TO#02731*/
#define PIN_TOGGLE(pin) {pin##SHADOW ^= (1<<pin);pin##Port = pin##SHADOW;}

/* Button pressed */
#define BUTTON_PRESSED() ((PIN_GET(Button))?0:1)

#endif


#if defined (ZW040x) || defined(ZW050x)

/* Z-Wave Button - INT1 */
#define	ButtonPort          P1
#define	ButtonSHADOW        P1Shadow
#define	ButtonDIR           P1DIR
#define	ButtonSHADOWDIR     P1ShadowDIR
#define	ButtonDIR_PAGE      P1DIR_PAGE;
#define	Button              1


/*ZW-Wave  EEPROM CS */
#define EECSPort          P2
#define EECSSHADOW        P2Shadow
#define EECSDIR           P2DIR
#define EECSSHADOWDIR     P2ShadowDIR
#define	EECSDIR_PAGE      P2DIR_PAGE
#define EECS              5

#define P00Port          P0
#define P00SHADOW        P0Shadow
#define P00DIR           P0DIR
#define P00SHADOWDIR     P0ShadowDIR
#define	P00DIR_PAGE      P0DIR_PAGE
#define P00              0

#define P01Port          P0
#define P01SHADOW        P0Shadow
#define P01DIR           P0DIR
#define P01SHADOWDIR     P0ShadowDIR
#define	P01DIR_PAGE      P0DIR_PAGE
#define P01              1

#define P02Port          P0
#define P02SHADOW        P0Shadow
#define P02DIR           P0DIR
#define P02SHADOWDIR     P0ShadowDIR
#define	P02DIR_PAGE      P0DIR_PAGE
#define P02              2

#define P03Port          P0
#define P03SHADOW        P0Shadow
#define P03DIR           P0DIR
#define P03SHADOWDIR     P0ShadowDIR
#define	P03DIR_PAGE      P0DIR_PAGE
#define P03              3

#define P04Port          P0
#define P04SHADOW        P0Shadow
#define P04DIR           P0DIR
#define P04SHADOWDIR     P0ShadowDIR
#define	P04DIR_PAGE      P0DIR_PAGE
#define P04              4

#define P05Port          P0
#define P05SHADOW        P0Shadow
#define P05DIR           P0DIR
#define P05SHADOWDIR     P0ShadowDIR
#define	P05DIR_PAGE      P0DIR_PAGE
#define P05              5

#define P06Port          P0
#define P06SHADOW        P0Shadow
#define P06DIR           P0DIR
#define P06SHADOWDIR     P0ShadowDIR
#define	P06DIR_PAGE      P0DIR_PAGE
#define P06              6

#define P07Port          P0
#define P07SHADOW        P0Shadow
#define P07DIR           P0DIR
#define P07SHADOWDIR     P0ShadowDIR
#define	P07DIR_PAGE      P0DIR_PAGE
#define P07              7

#define P10Port          P1
#define P10SHADOW        P1Shadow
#define P10DIR           P1DIR
#define P10SHADOWDIR     P1ShadowDIR
#define	P10DIR_PAGE      P1DIR_PAGE
#define P10              0

#define P11Port          P1
#define P11SHADOW        P1Shadow
#define P11DIR           P1DIR
#define P11SHADOWDIR     P1ShadowDIR
#define	P11DIR_PAGE      P1DIR_PAGE
#define P11              1

#define P12Port          P1
#define P12SHADOW        P1Shadow
#define P12DIR           P1DIR
#define P12SHADOWDIR     P1ShadowDIR
#define	P12DIR_PAGE      P1DIR_PAGE
#define P12              2

#define P13Port          P1
#define P13SHADOW        P1Shadow
#define P13DIR           P1DIR
#define P13SHADOWDIR     P1ShadowDIR
#define	P13DIR_PAGE      P1DIR_PAGE
#define P13              3

#define P14Port          P1
#define P14SHADOW        P1Shadow
#define P14DIR           P1DIR
#define P14SHADOWDIR     P1ShadowDIR
#define	P14DIR_PAGE      P1DIR_PAGE
#define P14              4

#define P15Port          P1
#define P15SHADOW        P1Shadow
#define P15DIR           P1DIR
#define P15SHADOWDIR     P1ShadowDIR
#define	P15DIR_PAGE      P1DIR_PAGE
#define P15              5

#define P16Port          P1
#define P16SHADOW        P1Shadow
#define P16DIR           P1DIR
#define P16SHADOWDIR     P1ShadowDIR
#define	P16DIR_PAGE      P1DIR_PAGE
#define P16              6

#define P17Port          P1
#define P17SHADOW        P1Shadow
#define P17DIR           P1DIR
#define P17SHADOWDIR     P1ShadowDIR
#define	P17DIR_PAGE      P1DIR_PAGE
#define P17              7


#define P20Port          P2
#define P20SHADOW        P2Shadow
#define P20DIR           P2DIR
#define P20SHADOWDIR     P2ShadowDIR
#define	P20DIR_PAGE      P2DIR_PAGE
#define P20              0

#define P21Port          P2
#define P21SHADOW        P2Shadow
#define P21DIR           P2DIR
#define P21SHADOWDIR     P2ShadowDIR
#define	P21DIR_PAGE      P2DIR_PAGE
#define P21              1

#define P22Port          P2
#define P22SHADOW        P2Shadow
#define P22DIR           P2DIR
#define P22SHADOWDIR     P2ShadowDIR
#define	P22DIR_PAGE      P2DIR_PAGE
#define P22              2

#define P23Port          P2
#define P23SHADOW        P2Shadow
#define P23DIR           P2DIR
#define P23SHADOWDIR     P2ShadowDIR
#define	P23DIR_PAGE      P2DIR_PAGE
#define P23              3

#define P24Port          P2
#define P24SHADOW        P2Shadow
#define P24DIR           P2DIR
#define P24SHADOWDIR     P2ShadowDIR
#define	P24DIR_PAGE      P2DIR_PAGE
#define P24              4

#define P25Port          P2
#define P25SHADOW        P2Shadow
#define P25DIR           P2DIR
#define P25SHADOWDIR     P2ShadowDIR
#define	P25DIR_PAGE      P2DIR_PAGE
#define P25              5

#define P26Port          P2
#define P26SHADOW        P2Shadow
#define P26DIR           P2DIR
#define P26SHADOWDIR     P2ShadowDIR
#define	P26DIR_PAGE      P2DIR_PAGE
#define P26              6

#define P27Port          P2
#define P27SHADOW        P2Shadow
#define P27DIR           P2DIR
#define P27SHADOWDIR     P2ShadowDIR
#define	P27DIR_PAGE      P2DIR_PAGE
#define P27              7

#define P30Port          P3
#define P30SHADOW        P3Shadow
#define P30DIR           P3DIR
#define P30SHADOWDIR     P3ShadowDIR
#define	P30DIR_PAGE      P3DIR_PAGE
#define P30              0

#define P31Port          P3
#define P31SHADOW        P3Shadow
#define P31DIR           P3DIR
#define P31SHADOWDIR     P3ShadowDIR
#define	P31DIR_PAGE      P3DIR_PAGE
#define P31              1

#define P34Port          P3
#define P34SHADOW        P3Shadow
#define P34DIR           P3DIR
#define P34SHADOWDIR     P3ShadowDIR
#define	P34DIR_PAGE      P3DIR_PAGE
#define P34              4

#define P35Port          P3
#define P35SHADOW        P3Shadow
#define P35DIR           P3DIR
#define P35SHADOWDIR     P3ShadowDIR
#define	P35DIR_PAGE      P3DIR_PAGE
#define P35              5

#define P36Port          P3
#define P36SHADOW        P3Shadow
#define P36DIR           P3DIR
#define P36SHADOWDIR     P3ShadowDIR
#define	P36DIR_PAGE      P3DIR_PAGE
#define P36              6

#define P37Port          P3
#define P37SHADOW        P3Shadow
#define P37DIR           P3DIR
#define P37SHADOWDIR     P3ShadowDIR
#define	P37DIR_PAGE      P3DIR_PAGE
#define P37              7

/* IO function definitions
^(#define ^(*Port^)^( +P[0-9]^)^)
^1 #define ^2SHADOW     ^3
*/

/* Production Test Pin for Prod_Test_Dut, Pin 5 */
#define SSNPort          P0
#define SSNSHADOW        P0Shadow
#define SSNDIR           P0DIR
#define SSNSHADOWDIR     P0ShadowDIR
#define	SSNDIR_PAGE      P0DIR_PAGE
#define SSN              4

#define LED_OUT0Port          P0
#define LED_OUT0SHADOW        P0Shadow
#define LED_OUT0DIR           P0DIR
#define LED_OUT0SHADOWDIR     P0ShadowDIR
#define	LED_OUT0DIR_PAGE      P0DIR_PAGE
#define LED_OUT0              4

#define LED_OUT1Port          P0
#define LED_OUT1SHADOW        P0Shadow
#define LED_OUT1DIR           P0DIR
#define LED_OUT1SHADOWDIR     P0ShadowDIR
#define	LED_OUT1DIR_PAGE      P0DIR_PAGE
#define LED_OUT1              5

#define LED_OUT2Port          P0
#define LED_OUT2SHADOW        P0Shadow
#define LED_OUT2DIR           P0DIR
#define LED_OUT2SHADOWDIR     P0ShadowDIR
#define	LED_OUT2DIR_PAGE      P0DIR_PAGE
#define LED_OUT2              6

#define LED_OUT3Port          P0
#define LED_OUT3SHADOW        P0Shadow
#define LED_OUT3DIR           P0DIR
#define LED_OUT3SHADOWDIR     P0ShadowDIR
#define	LED_OUT3DIR_PAGE      P0DIR_PAGE
#define LED_OUT3              7

#define KP_OUT0Port          P0
#define KP_OUT0SHADOW        P0Shadow
#define KP_OUT0DIR           P0DIR
#define KP_OUT0SHADOWDIR     P0ShadowDIR
#define	KP_OUT0DIR_PAGE      P0DIR_PAGE
#define KP_OUT0              0

#define KP_OUT1Port          P0
#define KP_OUT1SHADOW        P0Shadow
#define KP_OUT1DIR           P0DIR
#define KP_OUT1SHADOWDIR     P0ShadowDIR
#define	KP_OUT1DIR_PAGE      P0DIR_PAGE
#define KP_OUT1              1

#define KP_OUT2Port          P0
#define KP_OUT2SHADOW        P0Shadow
#define KP_OUT2DIR           P0DIR
#define KP_OUT2SHADOWDIR     P0ShadowDIR
#define	KP_OUT2DIR_PAGE      P0DIR_PAGE
#define KP_OUT2              2

#define KP_OUT3Port          P0
#define KP_OUT3SHADOW        P0Shadow
#define KP_OUT3DIR           P0DIR
#define KP_OUT3SHADOWDIR     P0ShadowDIR
#define	KP_OUT3DIR_PAGE      P0DIR_PAGE
#define KP_OUT3              3

#define KP_OUT4Port          P0
#define KP_OUT4SHADOW        P0Shadow
#define KP_OUT4DIR           P0DIR
#define KP_OUT4SHADOWDIR     P0ShadowDIR
#define	KP_OUT4DIR_PAGE      P0DIR_PAGE
#define KP_OUT4              4

#define KP_OUT5Port          P0
#define KP_OUT5SHADOW        P0Shadow
#define KP_OUT5DIR           P0DIR
#define KP_OUT5SHADOWDIR     P0ShadowDIR
#define	KP_OUT5DIR_PAGE      P0DIR_PAGE
#define KP_OUT5              5

#define KP_OUT6Port          P0
#define KP_OUT6SHADOW        P0Shadow
#define KP_OUT6DIR           P0DIR
#define KP_OUT6SHADOWDIR     P0ShadowDIR
#define	KP_OUT6DIR_PAGE      P0DIR_PAGE
#define KP_OUT6              6

#define KP_OUT7Port          P0
#define KP_OUT7SHADOW        P0Shadow
#define KP_OUT7DIR           P0DIR
#define KP_OUT7SHADOWDIR     P0ShadowDIR
#define	KP_OUT7DIR_PAGE      P0DIR_PAGE
#define KP_OUT7              7

#define KP_OUT8Port          P3
#define KP_OUT8SHADOW        P3Shadow
#define KP_OUT8DIR           P3DIR
#define KP_OUT8SHADOWDIR     P3ShadowDIR
#define	KP_OUT8DIR_PAGE      P3DIR_PAGE
#define KP_OUT8              7

#define KP_OUT9Port          P3
#define KP_OUT9SHADOW        P3Shadow
#define KP_OUT9DIR           P3DIR
#define KP_OUT9SHADOWDIR     P3ShadowDIR
#define	KP_OUT9DIR_PAGE      P3DIR_PAGE
#define KP_OUT9              6

#define KP_OUT10Port          P3
#define KP_OUT10SHADOW        P3Shadow
#define KP_OUT10DIR           P3DIR
#define KP_OUT10SHADOWDIR     P3ShadowDIR
#define	KP_OUT10DIR_PAGE      P3DIR_PAGE
#define KP_OUT10              5

#define KP_OUT11Port          P3
#define KP_OUT11SHADOW        P3Shadow
#define KP_OUT11DIR           P3DIR
#define KP_OUT11SHADOWDIR     P3ShadowDIR
#define	KP_OUT11DIR_PAGE      P3DIR_PAGE
#define KP_OUT11              4

#define KP_OUT12Port          P3
#define KP_OUT12SHADOW        P3Shadow
#define KP_OUT12DIR           P3DIR
#define KP_OUT12SHADOWDIR     P3ShadowDIR
#define	KP_OUT12DIR_PAGE      P3DIR_PAGE
#define KP_OUT12              1

#define KP_OUT13Port          P3
#define KP_OUT13SHADOW        P3Shadow
#define KP_OUT13DIR           P3DIR
#define KP_OUT13SHADOWDIR     P3ShadowDIR
#define	KP_OUT13DIR_PAGE      P3DIR_PAGE
#define KP_OUT13              0

#define KP_OUT14Port          P2
#define KP_OUT14SHADOW        P2Shadow
#define KP_OUT14DIR           P2DIR
#define KP_OUT14SHADOWDIR     P2ShadowDIR
#define	KP_OUT14DIR_PAGE      P2DIR_PAGE
#define KP_OUT14              1

#define KP_OUT15Port          P2
#define KP_OUT15SHADOW        P2Shadow
#define KP_OUT15DIR           P2DIR
#define KP_OUT15SHADOWDIR     P2ShadowDIR
#define	KP_OUT15DIR_PAGE      P2DIR_PAGE
#define KP_OUT15              0

#define KP_IN0Port          P1
#define KP_IN0SHADOW        P1Shadow
#define KP_IN0DIR           P1DIR
#define KP_IN0SHADOWDIR     P1ShadowDIR
#define	KP_IN0DIR_PAGE      P1DIR_PAGE
#define KP_IN0              0

#define KP_IN1Port          P1
#define KP_IN1SHADOW        P1Shadow
#define KP_IN1DIR           P1DIR
#define KP_IN1SHADOWDIR     P1ShadowDIR
#define	KP_IN1DIR_PAGE      P1DIR_PAGE
#define KP_IN1              1

#define KP_IN2Port          P1
#define KP_IN2SHADOW        P1Shadow
#define KP_IN2DIR           P1DIR
#define KP_IN2SHADOWDIR     P1ShadowDIR
#define	KP_IN2DIR_PAGE      P1DIR_PAGE
#define KP_IN2              2

#define KP_IN3Port          P1
#define KP_IN3SHADOW        P1Shadow
#define KP_IN3DIR           P1DIR
#define KP_IN3SHADOWDIR     P1ShadowDIR
#define	KP_IN3DIR_PAGE      P1DIR_PAGE
#define KP_IN3              3

#define KP_IN4Port          P1
#define KP_IN4SHADOW        P1Shadow
#define KP_IN4DIR           P1DIR
#define KP_IN4SHADOWDIR     P1ShadowDIR
#define	KP_IN4DIR_PAGE      P1DIR_PAGE
#define KP_IN4              4

#define KP_IN5Port          P1
#define KP_IN5SHADOW        P1Shadow
#define KP_IN5DIR           P1DIR
#define KP_IN5SHADOWDIR     P1ShadowDIR
#define	KP_IN5DIR_PAGE      P1DIR_PAGE
#define KP_IN5              5

#define KP_IN6Port          P1
#define KP_IN6SHADOW        P1Shadow
#define KP_IN6DIR           P1DIR
#define KP_IN6SHADOWDIR     P1ShadowDIR
#define	KP_IN6DIR_PAGE      P1DIR_PAGE
#define KP_IN6              6

#define KP_IN7Port          P1
#define KP_IN7SHADOW        P1Shadow
#define KP_IN7DIR           P1DIR
#define KP_IN7SHADOWDIR     P1ShadowDIR
#define	KP_IN7DIR_PAGE      P1DIR_PAGE
#define KP_IN7              7

#define INT0Port          P1
#define INT0SHADOW        P1Shadow
#define INT0DIR           P1DIR
#define INT0SHADOWDIR     P1ShadowDIR
#define	INT0DIR_PAGE      P1DIR_PAGE
#define INT0              0

#define INT1Port          P1
#define INT1SHADOW        P1Shadow
#define INT1DIR           P1DIR
#define INT1SHADOWDIR     P1ShadowDIR
#define	INT1DIR_PAGE      P1DIR_PAGE
#define INT1              1

#define INT2Port          P1
#define INT2SHADOW        P1Shadow
#define INT2DIR           P1DIR
#define INT2SHADOWDIR     P1ShadowDIR
#define	INT2DIR_PAGE      P1DIR_PAGE
#define INT2              2

#define INT3Port          P1
#define INT3SHADOW        P1Shadow
#define INT3DIR           P1DIR
#define INT3SHADOWDIR     P1ShadowDIR
#define	INT3DIR_PAGE      P1DIR_PAGE
#define INT3              3

#define INT4Port          P1
#define INT4SHADOW        P1Shadow
#define INT4DIR           P1DIR
#define INT4SHADOWDIR     P1ShadowDIR
#define	INT4DIR_PAGE      P1DIR_PAGE
#define INT4              4

#define INT5Port          P1
#define INT5SHADOW        P1Shadow
#define INT5DIR           P1DIR
#define INT5SHADOWDIR     P1ShadowDIR
#define	INT5DIR_PAGE      P1DIR_PAGE
#define INT5              5

#define INT6Port          P1
#define INT6SHADOW        P1Shadow
#define INT6DIR           P1DIR
#define INT6SHADOWDIR     P1ShadowDIR
#define	INT6DIR_PAGE      P1DIR_PAGE
#define INT6              6

#define INT7Port          P1
#define INT7SHADOW        P1Shadow
#define INT7DIR           P1DIR
#define INT7SHADOWDIR     P1ShadowDIR
#define	INT7DIR_PAGE      P1DIR_PAGE
#define INT7              7

#define UART0_RXDPort          P2
#define UART0_RXDSHADOW        P2Shadow
#define UART0_RXDDIR           P2DIR
#define UART0_RXDSHADOWDIR     P2ShadowDIR
#define	UART0_RXDDIR_PAGE      P2DIR_PAGE
#define UART0_RXD              0

#define UART0_TXDPort          P2
#define UART0_TXDSHADOW        P2Shadow
#define UART0_TXDDIR           P2DIR
#define UART0_TXDSHADOWDIR     P2ShadowDIR
#define	UART0_TXDDIR_PAGE      P2DIR_PAGE
#define UART0_TXD              1

#define SPI1_MOSIPort          P2
#define SPI1_MOSISHADOW        P2Shadow
#define SPI1_MOSIDIR           P2DIR
#define SPI1_MOSISHADOWDIR     P2ShadowDIR
#define	SPI1_MOSIDIR_PAGE      P2DIR_PAGE
#define SPI1_MOSI              2

#define SPI1_MISOPort          P2
#define SPI1_MISOSHADOW        P2Shadow
#define SPI1_MISODIR           P2DIR
#define SPI1_MISOSHADOWDIR     P2ShadowDIR
#define	SPI1_MISODIR_PAGE      P2DIR_PAGE
#define SPI1_MISO              3

#define SPI1_SCKPort          P2
#define SPI1_SCKSHADOW        P2Shadow
#define SPI1_SCKDIR           P2DIR
#define SPI1_SCKSHADOWDIR     P2ShadowDIR
#define	SPI1_SCKDIR_PAGE      P2DIR_PAGE
#define SPI1_SCK              4

#define ISP_MISOPort          P2
#define ISP_MISOSHADOW        P2Shadow
#define ISP_MISODIR           P2DIR
#define ISP_MISOSHADOWDIR     P2ShadowDIR
#define	ISP_MISODIR_PAGE      P2DIR_PAGE
#define ISP_MISO              2

#define ISP_MOSIPort          P2
#define ISP_MOSISHADOW        P2Shadow
#define ISP_MOSIDIR           P2DIR
#define ISP_MOSISHADOWDIR     P2ShadowDIR
#define	ISP_MOSIDIR_PAGE      P2DIR_PAGE
#define ISP_MOSI              3

#define ISP_SCKPort          P2
#define ISP_SCKSHADOW        P2Shadow
#define ISP_SCKDIR           P2DIR
#define ISP_SCKSHADOWDIR     P2ShadowDIR
#define	ISP_SCKDIR_PAGE      P2DIR_PAGE
#define ISP_SCK              4

#define IR_TX0Port          P3
#define IR_TX0SHADOW        P3Shadow
#define IR_TX0DIR           P3DIR
#define IR_TX0SHADOWDIR     P3ShadowDIR
#define	IR_TX0DIR_PAGE      P3DIR_PAGE
#define IR_TX0              4

#define IR_TX1Port          P3
#define IR_TX1SHADOW        P3Shadow
#define IR_TX1DIR           P3DIR
#define IR_TX1SHADOWDIR     P3ShadowDIR
#define	IR_TX1DIR_PAGE      P3DIR_PAGE
#define IR_TX1              5

#define IR_TX2Port          P3
#define IR_TX2SHADOW        P3Shadow
#define IR_TX2DIR           P3DIR
#define IR_TX2SHADOWDIR     P3ShadowDIR
#define	IR_TX2DIR_PAGE      P3DIR_PAGE
#define IR_TX2              6

#define ADC0Port          P3
#define ADC0SHADOW        P3Shadow
#define ADC0DIR           P3DIR
#define ADC0SHADOWDIR     P3ShadowDIR
#define	ADC0DIR_PAGE      P3DIR_PAGE
#define ADC0              4

#define ADC1Port          P3
#define ADC1SHADOW        P3Shadow
#define ADC1DIR           P3DIR
#define ADC1SHADOWDIR     P3ShadowDIR
#define ADC1DIR_PAGE      P3DIR_PAGE
#define ADC1              5

#define ADC2Port          P3
#define ADC2SHADOW        P3Shadow
#define ADC2DIR           P3DIR
#define ADC2SHADOWDIR     P3ShadowDIR
#define ADC2DIR_PAGE      P3DIR_PAGE
#define ADC2              6

#define ADC3Port          P3
#define ADC3SHADOW        P3Shadow
#define ADC3DIR           P3DIR
#define ADC3SHADOWDIR     P3ShadowDIR
#define ADC3DIR_PAGE      P3DIR_PAGE
#define ADC3              7

#define UART1_TXDPort          P3
#define UART1_TXDSHADOW        P3Shadow
#define UART1_TXDDIR           P3DIR
#define UART1_TXDSHADOWDIR     P3ShadowDIR
#define UART1_TXDDIR_PAGE      P3DIR_PAGE
#define UART1_TXD              1

#define UART1_RXDPort          P3
#define UART1_RXDSHADOW        P3Shadow
#define UART1_RXDDIR           P3DIR
#define UART1_RXDSHADOWDIR     P3ShadowDIR
#define UART1_RXDDIR_PAGE      P3DIR_PAGE
#define UART1_RXD              0

#define SPI0_SSNPort          P3
#define SPI0_SSNSHADOW        P3Shadow
#define SPI0_SSNDIR           P3DIR
#define SPI0_SSNSHADOWDIR     P3ShadowDIR
#define SPI0_SSNDIR_PAGE      P3DIR_PAGE
#define SPI0_SSN              0

#define SPI0_MOSIPort          P2
#define SPI0_MOSISHADOW        P2Shadow
#define SPI0_MOSIDIR           P2DIR
#define SPI0_MOSISHADOWDIR     P2ShadowDIR
#define SPI0_MOSIDIR_PAGE      P2DIR_PAGE
#define SPI0_MOSI              5

#define SPI0_MISOPort          P2
#define SPI0_MISOSHADOW        P2Shadow
#define SPI0_MISODIR           P2DIR
#define SPI0_MISOSHADOWDIR     P2ShadowDIR
#define SPI0_MISODIR_PAGE      P2DIR_PAGE
#define SPI0_MISO              6

#define SPI0_SCKPort          P2
#define SPI0_SCKSHADOW        P2Shadow
#define SPI0_SCKDIR           P2DIR
#define SPI0_SCKSHADOWDIR     P2ShadowDIR
#define SPI0_SCKDIR_PAGE      P2DIR_PAGE
#define SPI0_SCK              7

#define IR_RXPort          P3
#define IR_RXSHADOW        P3Shadow
#define IR_RXDIR           P3DIR
#define IR_RXSHADOWDIR     P3ShadowDIR
#define IR_RXDIR_PAGE      P3DIR_PAGE
#define IR_RX              1


#define FIREPort          P3
#define FIRESHADOW        P3Shadow
#define FIREDIR           P3DIR
#define FIRESHADOWDIR     P3ShadowDIR
#define FIREDIR_PAGE      P3DIR_PAGE
#define FIRE              6

#define TRIACPort          P3
#define TRIACSHADOW        P3Shadow
#define TRIACDIR           P3DIR
#define TRIACSHADOWDIR     P3ShadowDIR
#define TRIACDIR_PAGE      P3DIR_PAGE
#define TRIAC              6

#define GP_PWMPort          P3
#define GP_PWMSHADOW        P3Shadow
#define GP_PWMDIR           P3DIR
#define GP_PWMSHADOWDIR     P3ShadowDIR
#define GP_PWMDIR_PAGE      P3DIR_PAGE
#define GP_PWM              7

#define ZEROXPort          P3
#define ZEROXSHADOW        P3Shadow
#define ZEROXDIR           P3DIR
#define ZEROXSHADOWDIR     P3ShadowDIR
#define ZEROXDIR_PAGE      P3DIR_PAGE
#define ZEROX              7

#define IR_PWMPort          P3
#define IR_PWMSHADOW        P3Shadow
#define IR_PWMDIR           P3DIR
#define IR_PWMSHADOWDIR     P3ShadowDIR
#define IR_PWMDIR_PAGE      P3DIR_PAGE
#define IR_PWM              7

#endif /* #if defined (ZW040x) || defined(ZW050x) */

#endif /* _ZW_PINDEFS_H_ */

