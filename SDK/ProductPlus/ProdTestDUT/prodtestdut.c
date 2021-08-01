/********************************  prodtestdut.c  *****************************
 *           #######
 *           ##  ##
 *           #  ##    ####   #####    #####  ##  ##   #####
 *             ##    ##  ##  ##  ##  ##      ##  ##  ##
 *            ##  #  ######  ##  ##   ####   ##  ##   ####
 *           ##  ##  ##      ##  ##      ##   #####      ##
 *          #######   ####   ##  ##  #####       ##  #####
 *                                           #####
 *          Z-Wave, the wireless language.
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
 * Copyright Zensys A/S, 2001
 *
 * Description: Application used in productiontest
 *
 * Author:   Henrik Holm
 *
 * Last Changed By:  $Author: efh $
 * Revision:         $Revision: 29510 $
 * Last Changed:     $Date: 2014-09-16 12:18:23 +0200 (ti, 16 sep 2014) $
 *
 ****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include "config_app.h"
#include <ZW_basis_api.h>
#include <ZW_slave_api.h>
#include <ZW_pindefs.h>
#ifdef ENABLE_DEBUG_LEDS
#include <ZW_evaldefs.h>
#endif
#include <prodtestdut.h>


/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/


/* ADC3/ZEROX/GP_PWM/KP_OUT8 P3_7 pin now input */
#define SET_PRODUCTIONTEST_PIN  PIN_IN(P04, 0)
#define IN_PRODUCTIONTEST       (!PIN_GET(P04))


/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/
enum TEST_STATES
     {
      testIdle = 0,
      testIoStart,
      testNopStart,
      testDone
     };

enum TEST_STATES  testMode = testIdle;
/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/*===========================   ApplicationRfNotify   ===========================
**    Notify the application when the radio switch state
**    Called from the Z-Wave PROTOCOL When radio switch from Rx to Tx or from Tx to Rx
**    or when the modulator PA (Power Amplifier) turn on/off
**---------------------------------------------------------------------------------*/
void          /*RET Nothing */
ApplicationRfNotify(
  BYTE rfState          /* IN state of the RF, the available values is as follow:
                               ZW_RF_TX_MODE: The RF switch from the Rx to Tx mode, the modualtor is started and PA is on
                               ZW_RF_PA_ON: The RF in the Tx mode, the modualtor PA is turned on
                               ZW_RF_PA_OFF: the Rf in the Tx mode, the modulator PA is turned off
                               ZW_RF_RX_MODE: The RF switch from Tx to Rx mode, the demodulator is started.*/
)
{
  UNUSED(rfState);
}

/*============================   ApplicationInitHW   ========================
**    Initialization of non Z-Wave module hardware
**
**    Side effects:
**       Returning FALSE from this function will force the API into
**       production test mode.
**--------------------------------------------------------------------------*/
BYTE
ApplicationInitHW(
  BYTE bWakeupReason       /* Reason for the powerup of the chip */
)
{
  BYTE i;
  UNUSED(bWakeupReason);
  SET_PRODUCTIONTEST_PIN;
  for (i =0 ; i < 250; i++);
  return(TRUE);
}

/*============================   ApplicationTestPoll   ======================
**    Function description
**      This function is called when the slave enters test mode.
**
**    Side effects:
**       Code will not exit until it is reset
**--------------------------------------------------------------------------*/
void ApplicationPoll(void)
{
  if (testMode == testIdle)
  {
    if (IN_PRODUCTIONTEST) /* Anyone forcing it into productiontest mode ? */
    {
      testMode = testIoStart;
    }
    else
    {
      testMode = testNopStart;
    }
  }
  if(testMode == testIoStart)
  {
    testMode = testDone;
    ZW_SEND_CONST();
  }
  else if (testMode == testDone)
  {
  }
  else if (testMode == testNopStart)
  {
    ZW_IOS_set(0, 0xFF, 0xFF);
    ZW_IOS_set(1, 0xFF, 0xFF);
    ZW_IOS_set(2, 0xFF, 0xFF);
    ZW_IOS_set(3, 0xFF, 0xFF);
    testMode = testDone;
  }

}

/*
 * @brief Called when the protocol needs user input for client side authentication (CSA)
 * @details If the app does not need/want to support CSA this function can be left empty.
 *
 *          If CSA is needed, the app must do the following when this is called:
 *             1. Obtain user input with first 4 bytes of the S2 including node DSK
 *             2. Store the user input in a response variable of type s_SecurityS2InclusionCSAPublicDSK_t.
 *             3. Call ZW_SetSecurityS2InclusionPublicDSK_CSA(response)
 *
 */
void
ApplicationSecurityS2InclusionRequestPublicDSK_CSA(void)
{
}

/**
* Set up security keys to request when joining a network.
* These are taken from the config_app.h header file.
* Not used in ProdTestDUT.
*/
BYTE ApplicationSecureKeysRequested(void)
{
  return 0;
}

/**
* Set up security S2 inclusion authentication to request when joining a network.
* Unused, but mandatory function. This is not a secure product.
*/
BYTE ApplicationSecureAuthenticationRequested(void)
{
  return 0;
}
