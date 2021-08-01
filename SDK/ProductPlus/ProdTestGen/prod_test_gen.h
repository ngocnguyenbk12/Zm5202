/****************************************************************************
 *
 * Copyright (c) 2001-2013
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description: This program starts by setting pin 0 on PORTA (now LED0) HIGH
 *  and if the button is pressed it sends 10 NOP frames while clearing LED0
 *  and setting pin 1 on PORTA (now LED1) HIGH, if ACK is received for all
 *  frames, LED1 continues to be HIGH, if not then LED1 is set LOW and LED0
 *  is set HIGH. By pressing button the sequence is started again...
 *
 * Author:   Erik Friis Harck
 *
 * Last Changed By:  $Author: efh $
 * Revision:         $Revision: 11456 $
 * Last Changed:     $Date: 2008-09-25 16:29:18 +0200 (Thu, 25 Sep 2008) $
 *
 ****************************************************************************/


/****************************************************************************/
/*                              PRIVATE FUNCTIONS                           */
/****************************************************************************/
extern void SetGREEN(BYTE on);

extern void SetRED(BYTE on);

extern void ToggleRED(void);

extern void ZCB_SendCompleteHandler(BYTE  commandCode, TX_STATUS_TYPE *psTxResult);

extern void ZCB_TimerTestHandler(void);


