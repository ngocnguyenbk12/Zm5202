/****************************************************************************
 *
 * Copyright (c) 2001-2011
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description: IO definitions for the Z-Wave Evaluation board
 *
 * Author:   Ivar Jeppesen
 *
 * Last Changed By:  $Author: tro $
 * Revision:         $Revision: 25909 $
 * Last Changed:     $Date: 2013-05-29 14:49:43 +0200 (on, 29 maj 2013) $
 *
 ****************************************************************************/
#ifndef _ZW_EVALDEFS_H_
#define _ZW_EVALDEFS_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/* Evaluation board LEDs */

/* Turn LED on/off
 *  led - LED number
 */


#define LED_ON(led)   PIN_OFF(LED##led)
#define LED_OFF(led)  PIN_ON(LED##led)

#define LED_TOGGLE(led) PIN_TOGGLE(LED##led)


/* LED number       Z-Wave Device pin */

/* old stuff.. need to be removed*/
#define LEDZM4102_1Port       P1
#define LEDZM4102_1SHADOW     P1Shadow
#define LEDZM4102_1SHADOWDIR  P1ShadowDIR
#define LEDZM4102_1DIR        P1DIR
#define LEDZM4102_1DIR_PAGE   P1DIR_PAGE
#define LEDZM4102_1           0

#ifdef ZM5202
/*LED1 P1_0*/
#define LED1Port        P1
#define LED1SHADOW      P1Shadow
#define LED1SHADOWDIR   P1ShadowDIR
#define LED1DIR         P1DIR
#define LED1DIR_PAGE    P1DIR_PAGE
#define LED1            0

/*LED2 P3_7*/
#define LED2Port       P3
#define LED2SHADOW     P3Shadow
#define LED2SHADOWDIR  P3ShadowDIR
#define LED2DIR        P3DIR
#define LED2DIR_PAGE   P3DIR_PAGE
#define LED2           7

#else
/*LED1 P0_7*/
#define LED1Port        P0
#define LED1SHADOW      P0Shadow
#define LED1SHADOWDIR   P0ShadowDIR
#define LED1DIR         P0DIR
#define LED1DIR_PAGE    P0DIR_PAGE
#define LED1            7

/*LED2 P3_7*/
#define LED2Port       P3
#define LED2SHADOW     P3Shadow
#define LED2SHADOWDIR  P3ShadowDIR
#define LED2DIR        P3DIR
#define LED2DIR_PAGE   P3DIR_PAGE
#define LED2           7

/*LED3 P1_0*/
#define LED3Port       P1
#define LED3SHADOW     P1Shadow
#define LED3SHADOWDIR  P1ShadowDIR
#define LED3DIR        P1DIR
#define LED3DIR_PAGE   P1DIR_PAGE
#define LED3           0

/*LED4 P1_2*/
#define LED4Port        P1
#define LED4SHADOW      P1Shadow
#define LED4SHADOWDIR   P1ShadowDIR
#define LED4DIR         P1DIR
#define LED4DIR_PAGE    P1DIR_PAGE
#define LED4            2
#endif


#endif /* _ZW_EVALDEFS_H_ */
