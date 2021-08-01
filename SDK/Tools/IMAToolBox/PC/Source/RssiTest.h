/****************************************************************************
 *
 * Z-Wave, the wireless language.
 * 
 * Copyright (c) 2014
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 * This source file is subject to the terms and conditions of the
 * Sigma Designs Software License Agreement which restricts the manner
 * in which it may be used.
 *
 *---------------------------------------------------------------------------
 *
 * Description:      RSSI test module for Z-Wave Network Health Toolbox
 *
 * Last Changed By:  $Author$: 
 * Revision:         $Rev$: 
 * Last Changed:     $Date$: 
 *
 ****************************************************************************/
#ifndef RSSITEST_H
#define RSSITEST_H

/*=========================== PerformRSSITest ===============================
** Function description
**      Perform the RSSI test step for the node currently undergoing
**  Network Health Test.
**		
** Side effects:
**
**--------------------------------------------------------------------------*/
void
PerformRSSITest(void);

/*=========================== ShowBackgroundRSSI ===============================
** Function description
**      Obtain and print background rssi for all channels
**		
** Side effects:
**
**--------------------------------------------------------------------------*/
void ShowBackgroundRSSI(void);

/*======================= CollectBackgroundRssi ============================
** Function description
**      Sample and average background RSSI samples.
**      Samples rssi 10 times 50 ms apart. Discard the 5 highest samples
**      for each channel. Average the remaining samples for each channel.
**		
** Side effects:
**   Detect if the Z-Wave network is 2- or 3-channel and return the result
**   in numChannels paramter.
**
**--------------------------------------------------------------------------*/
void
CollectBackgroundRssi(signed char abBackgrRssi[], BYTE *numChannels);

/*========================   rssi2dBm   =========================
** Function description
**   Callbak for the RSSI transmission.
** 
**--------------------------------------------------------------------------*/
signed char rssi2dBm(signed char val);
#endif