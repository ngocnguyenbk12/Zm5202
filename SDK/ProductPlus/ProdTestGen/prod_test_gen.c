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
 * Author:   Johann Sigfredsson
 *
 * Last Changed By:  $Author: psh $
 * Revision:         $Revision: 32972 $
 * Last Changed:     $Date: 2016-02-04 10:41:15 +0100 (to, 04 feb 2016) $
 *
 ****************************************************************************/


/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include "config_app.h"
#include "ZW_sysdefs.h"
#include "ZW_typedefs.h"
#include "ZW_pindefs.h"
#include "ZW_evaldefs.h"
#include "ZW_classcmd.h"
#include "ZW_basis_api.h"
#include "ZW_slave_api.h"
#include "ZW_uart_api.h"
#include "ZW_pindefs.h"
#include "ZW_strings_rf050x.h"

#include "prod_test_gen.h"

#include <ZW_rf050x.h>

#ifdef ZW_ISD51_DEBUG
#include "ISD51.h"
#endif

extern IBYTE RF_freqno;
extern void RF_calibration_set(BYTE crystalcal, BYTE txcal1, BYTE txcal2);
/*===============================   RFDriverInitHW   ==========================
**    Perfrom RF HW initalization
**
**--------------------------------------------------------------------------*/
extern void                /*RET Nothing           */
RFDriverInitHW( void ); /*IN  Nothing           */
/*==============================   RFDriverInitSW   =========================
**    Starts the sample interrupt and the generation of halfbit values,
**    and system time ticks.
**
**--------------------------------------------------------------------------*/
extern  void                /*RET Nothing           */
RFDriverInitSW( void ); /*IN  Nothing */

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

#define ON  TRUE
#define OFF FALSE
#define TEST_COUNT 10     /*Initial test count*/
#define TEST_INTERVAL 8   /* 80ms */

#define TESTHOMEID  0x00000000
#define TESTDESTID  0x00

#define TRANSMIT_FAIL               0x04  /* A single_multicast/route frame transmit failed */

extern  BYTE homeID[4];

BYTE keyWasDepressed = 0;
BYTE timerTestReady = 1;
BYTE commandBuf[4] = {0, 0, 0, 0};
WORD count = 0;
WORD success = 0;
WORD test_count = TEST_COUNT;
BYTE frequencySelectionBuf = 0;
BYTE bAutoTest = 0;

static BYTE destID;		 /* destination NodeID. */

/* Version string protocol   123456789012 (12 char max)*/
BYTE api_version[] = "Unknown    ";
BYTE code appl_version[]
 = {'\n','\r','V','e','r','s','i','o','n',' ',
    ((APP_VERSION/10) ? (APP_VERSION/10)+0x30 : ' '),
    (APP_VERSION%10)+0x30, '.',
    (APP_REVISION/10)+0x30,
    (APP_REVISION%10)+0x30, ' ',
    0x00
   }
;

BYTE code helpText[] = {
"\n\rValid commands:\
\n\rh This help text\
\n\rE Set EU\
\n\rU Set US\
\n\rH Set HK\
\n\rZ Set ANZ\
\n\rI Set IN\
\n\rM Set MY\
\n\rJ Set JP\
\n\rS Send test frames\
\n\rA Capture HomeID from NIF and Send test frames\
\n\rC Set count=1000\
\n\rN Set nodeID\
\n\rR Reset module\
\n\rF Set frequency by number\
\n\r"
};

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
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

void ToggleRED(void)
{

  LED_TOGGLE(1);

}


void SetGREEN(BYTE on)
{
  if (on)
  {
    LED_ON(2);

  }
  else
  {
    LED_OFF(2);
  }
}


void SetRED(BYTE on)
{
  if (on)
  {
    LED_ON(1);

  }
  else
  {
    LED_OFF(1);
  }
}


/*==========================   ApplictionSlaveUpdate   =======================
**   Inform a slave application that a node information is received.
**   Called from the slave command handler when a node information frame
**   is received and the Z-Wave protocol is not in a state where it is needed.
**
**--------------------------------------------------------------------------*/
void
ApplicationSlaveUpdate(
  BYTE bStatus,     /*IN  Status event */
  BYTE bNodeID,     /*IN  Node id of the node that send node info */
  BYTE* pCmd,       /*IN  Pointer to Application Node information */
  BYTE bLen)       /*IN  Node info length                        */
{
  UNUSED(bNodeID);
  UNUSED(pCmd);
  UNUSED(bLen);

  if (UPDATE_STATE_NODE_INFO_RECEIVED == bStatus)
  {
    ZW_UART0_SEND_STR("\r\nNew homeID ");
    ZW_UART0_SEND_NUM(homeID[0]);
    ZW_UART0_SEND_NUM(homeID[1]);
    ZW_UART0_SEND_NUM(homeID[2]);
    ZW_UART0_SEND_NUM(homeID[3]);
    ZW_UART0_SEND_STR("\n\r");
    if ((bAutoTest) && (timerTestReady))
    {
      keyWasDepressed = 1;
      timerTestReady = 0;
    }
  }
}


code const void (code * ZCB_SendCompleteHandler_p)(BYTE txStatus, TX_STATUS_TYPE *psTxResult) = &ZCB_SendCompleteHandler;
/*==========================   SendCompletedHandler   =======================
**    Handling of a completed send command.
**    This function is called when the Z-Wave modul has completed sending a frame.
**
**    This is an application function example
**
**--------------------------------------------------------------------------*/
void       /*RET  Nothing                  */
ZCB_SendCompleteHandler(
    BYTE txStatus,
    TX_STATUS_TYPE *psTxResult)       /*IN  Send completed code  */
{
  UNUSED(psTxResult);
  if (txStatus == TRANSMIT_COMPLETE_NO_ACK)
  {
    /*Not received*/
  }
  else
  {
    /*Received*/
#ifndef ZM5202
    LED_TOGGLE(3);
#endif
    success++;
  }
  if (++count >= test_count)
  {
    if (success < test_count)
    {
      SetGREEN(OFF);
      SetRED(ON);
      ZW_UART0_SEND_BYTE('-');
    }
    else
    {
      SetGREEN(ON);
      SetRED(OFF);
      ZW_UART0_SEND_BYTE('+');
    }
    ZW_UART0_SEND_NUM((BYTE)(success>>8));
    ZW_UART0_SEND_NUM((BYTE)(success & 0x00FF));
    count = success = 0;
    timerTestReady = 1;
  }
  else
  {
    if(!ZW_SEND_DATA(destID, commandBuf, sizeof(BYTE)*2, TRANSMIT_OPTION_ACK | 0x40, ZCB_SendCompleteHandler))
    {
      ZCB_SendCompleteHandler(TRANSMIT_COMPLETE_NO_ACK, NULL);
    }
  }
}


code const void (code * ZCB_TimerTestHandler_p)(void) = &ZCB_TimerTestHandler;
/*=======================   TimerTestHandler   ========================
**
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
void                   /*RET  Nothing                  */
ZCB_TimerTestHandler(void) /*IN  Nothing                   */
{
  ToggleRED();
  /* Send CMD_NOP frame */
  /* the transmit option 0x40 is TRANSMIT_OPTION_SINGLE, which isn't known */
  /* to the application-layer */
  if(!ZW_SEND_DATA(destID, commandBuf, sizeof(BYTE)*2, TRANSMIT_OPTION_ACK | 0x40, ZCB_SendCompleteHandler))
  {
    ZCB_SendCompleteHandler(TRANSMIT_COMPLETE_NO_ACK, NULL);
  }
}

/*=============================   ApplicationPoll   ========================
**    Application poll function
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void           /*RET  Nothing                  */
ApplicationPoll( void )  /* IN  Nothing                  */
{
  eRF_ID frequencyIndex;

#ifdef ZW_ISD51_DEBUG         /* init ISD51 only when the uVision2 Debugger tries to connect */
  ISDcheck();                 /* initialize uVision2 Debugger and continue program run */
#endif
  if (BUTTON_PRESSED())
  {
    if (timerTestReady)
    {
      keyWasDepressed = 1;
      timerTestReady = 0;
    }
  }
  else
  {
    /*No key is pressed*/
    if (keyWasDepressed)
    {
      keyWasDepressed = 0;
      SetGREEN(OFF);
      ToggleRED();
      ZW_TIMER_START(ZCB_TimerTestHandler, TEST_INTERVAL, 1);
    }
    if(ZW_UART0_REC_STATUS)
    {
      BYTE recData = ZW_UART0_REC_BYTE;
      ZW_UART0_SEND_BYTE(' ');
      switch (recData)
      {
        case 'h':
          {
            ZW_UART0_SEND_STR("\n\rCurrent Home ID: ");
            ZW_UART0_SEND_NUM(homeID[0]);
            ZW_UART0_SEND_NUM(homeID[1]);
            ZW_UART0_SEND_NUM(homeID[2]);
            ZW_UART0_SEND_NUM(homeID[3]);
            ZW_UART0_SEND_STR("\n\r");
            ZW_UART0_SEND_STR(helpText);
          }
          break;
        case 'A': /* Auto mode, get homeID from received node information frame and send test frames */
        case 'a':
          {
            if (bAutoTest)
            {
              bAutoTest = FALSE;
              ZW_SetRFReceiveMode(FALSE);
              ZW_UART0_SEND_STR("Disabling auto test mode\n\r");
            }
            else
            {
              bAutoTest = TRUE;
              ZW_UART0_SEND_STR("Enabling auto test mode\n\r");
              ZW_SetRFReceiveMode(TRUE);
            }
          }
          break;
        case 'F':
          {
            ZW_UART0_SEND_STR("Valid frequencies:");
            for (frequencyIndex = 0; frequencyIndex < RF_ID_MAX; frequencyIndex++)
            {
            if (aRF_3CH[frequencyIndex] == ZW_3CH_Build)
              {
                ZW_UART0_SEND_STR("\n\r");
                ZW_UART0_SEND_DEC(frequencyIndex);
                ZW_UART0_SEND_STR(": ");
                ZW_UART0_SEND_STR(RF_strings[frequencyIndex]);
              }
            }
            ZW_UART0_SEND_STR("\n\r");
          }
          break;
        case 'U':
          {
            /*Fix TO# 2701 */
            RF_calibration_set(0x80, 0xFF, 0xFF);
            RF_freqno = RF_US;
            RFDriverInitHW();
            RFDriverInitSW();
            ZW_SET_RX_MODE(TRUE);
            ZW_UART0_SEND_BYTE('U');
            ZW_UART0_SEND_BYTE('S');
          }
          break;
        case 'E':
          {
            /*Fix TO# 2701 */
            RF_freqno = RF_EU;
            RF_calibration_set(0x80, 0xFF, 0xFF);
            RFDriverInitHW();
            RFDriverInitSW();
            ZW_SET_RX_MODE(TRUE);
            ZW_UART0_SEND_BYTE('E');
            ZW_UART0_SEND_BYTE('U');
          }
          break;
        case 'H':
          {
            RF_freqno = RF_HK;
            RF_calibration_set(0x80, 0xFF, 0xFF);
            RFDriverInitHW();
            RFDriverInitSW();
            ZW_SET_RX_MODE(TRUE);
            ZW_UART0_SEND_BYTE('H');
            ZW_UART0_SEND_BYTE('K');

          }
          break;

        case 'Z':
          {
            RF_freqno = RF_ANZ;
            RF_calibration_set(0x80, 0xFF, 0xFF);
            RFDriverInitHW();
            RFDriverInitSW();
            ZW_SET_RX_MODE(TRUE);
            ZW_UART0_SEND_BYTE('A');
            ZW_UART0_SEND_BYTE('N');
            ZW_UART0_SEND_BYTE('Z');

          }
          break;

        case 'I':
          {
            RF_freqno = RF_IN;
            RF_calibration_set(0x80, 0xFF, 0xFF);
            RFDriverInitHW();
            RFDriverInitSW();
            ZW_SET_RX_MODE(TRUE);
            ZW_UART0_SEND_BYTE('I');
            ZW_UART0_SEND_BYTE('N');
          }
          break;
        case 'M':
          {
            RF_freqno = RF_MY;
            RF_calibration_set(0x80, 0xFF, 0xFF);
            RFDriverInitHW();
            RFDriverInitSW();
            ZW_SET_RX_MODE(TRUE);
            ZW_UART0_SEND_BYTE('M');
            ZW_UART0_SEND_BYTE('Y');

          }
          break;

        case 'J':
          {
            RF_freqno = RF_JP;
            RF_calibration_set(0x80, 0xFF, 0xFF);
            RFDriverInitHW();
            RFDriverInitSW();
            ZW_SET_RX_MODE(TRUE);
            ZW_UART0_SEND_BYTE('J');
            ZW_UART0_SEND_BYTE('P');

          }
          break;

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          {
            if (frequencySelectionBuf == 0)
            {
              ZW_UART0_SEND_BYTE(recData);
              frequencySelectionBuf = recData;
            }
            else
            {
              ZW_UART0_SEND_BYTE(recData);
              RF_freqno = ((frequencySelectionBuf - '0') * 10) + (recData - '0');;
              ZW_UART0_SEND_STR("\n\rFrequency selected: ");
              ZW_UART0_SEND_DEC(RF_freqno);
              ZW_UART0_SEND_BYTE('=');
              ZW_UART0_SEND_STR(RF_strings[RF_freqno]);
              frequencySelectionBuf = 0;
              RF_calibration_set(0x80, 0xFF, 0xFF);
              RFDriverInitHW();
              RFDriverInitSW();
              ZW_SET_RX_MODE(TRUE);
            }
          }
          break;

        case 'S':
          {
            if (timerTestReady)
            {
              keyWasDepressed = 1;
              timerTestReady = 0;
            }
            ZW_UART0_SEND_BYTE('S');
            ZW_UART0_SEND_BYTE('T');
          }
          break;
        case 'C':
          {
            test_count = 1000;
            ZW_UART0_SEND_BYTE('C');
            ZW_UART0_SEND_BYTE('O');
          }
          break;
        case 'N':
          {
           /*Fix  TO# 02710 now we can send to nodeID 1*/
            destID = ZW_UART0_REC_BYTE - '0';
            ZW_UART0_SEND_NUM(destID);
            ZW_UART0_SEND_BYTE('N');
            ZW_UART0_SEND_BYTE('I');
          }
        break;
        case 'R':
          {
            /*Reset the module*/
            ZW_UART0_SEND_BYTE('R');
            ZW_UART0_SEND_BYTE('S');
            while(ZW_UART0_SEND_STATUS);
            ZW_WATCHDOG_ENABLE; /*reset asic*/
            while(1);
          }
        break;
        default:
          {
            ZW_UART0_SEND_BYTE('!');
            ZW_UART0_SEND_BYTE(recData);
          }
        break;

      }
    }
  }
}


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/*==============================   ApplicationInitHW   ===========================
**    Non Z-Wave hardware initialization
**
**    This is an application function example
**
**--------------------------------------------------------------------------*/
BYTE
ApplicationInitHW(
  BYTE bWakeupReason       /* Reason for the powerup of the chip */
)
{
  UNUSED(bWakeupReason);
#ifdef ZW_ISD51_DEBUG
  ISD_UART_init();
#endif
  /* hardware initialization */
  PIN_IN(Button, 1);


  PIN_OUT(LED1);
  PIN_OUT(LED2);
#ifndef ZM5202
  PIN_OUT(LED3);
#endif
#ifdef JP
  RF_freqno = RF_JP;
#else
  RF_freqno = RF_US;
#endif

  return(TRUE);
}

/*===========================   ApplicationInitSW   =========================
**    Initialization of the Application Software
**
**    This is an application function example
**
**--------------------------------------------------------------------------*/
BYTE                   /*RET  TRUE       */
ApplicationInitSW(ZW_NVM_STATUS nvmStatus)
{
  *((DWORD *)&homeID[0]) = TESTHOMEID;

  UNUSED(nvmStatus);
/* Do not reinitialize the UART if already initialized for ISD51 in ApplicationInitHW() */
#ifndef ZW_ISD51_DEBUG
  ZW_DEBUG_INIT(1152);
  ZW_DEBUG_SEND_BYTE('@');
#endif
  SetRED(ON);
  SetGREEN(OFF);
#ifndef ZM5202
  LED_OFF(3);
#endif
  destID = TESTDESTID;
  ZW_UART0_init(1152, TRUE, TRUE);
#ifdef ZM5202
   ZW_UART0_zm4102_mode_enable(TRUE);
#endif
  ZW_UART0_SEND_STR(appl_version);
  ZW_Version(api_version);
  ZW_UART0_SEND_STR(api_version);
  ZW_UART0_SEND_STR("\n\rUse 'h' for help\n\r");
  return(TRUE);
}


/*============================   ApplicationTestPoll   ======================
**    Function description
**      This function is called when the slave enters test mode.
**    Side effects:
**       Code will not exit until it is reset
**--------------------------------------------------------------------------*/
void ApplicationTestPoll(void)
{
  ApplicationPoll();
}


/*========================   ApplicationCommandHandler   ====================
**    Handling of a received application commands and requests
**
**    This is an application function example
**
**--------------------------------------------------------------------------*/
void       /*RET  Nothing                  */
ApplicationCommandHandler(
  ZW_APPLICATION_TX_BUFFER *pCmd, /* IN Payload from the received frame, the union */
                                  /*    should be used to access the fields */
  BYTE cmdLength,                 /* IN  Number of command bytes including the command */
  RECEIVE_OPTIONS_TYPE *rxOpt)   /* IN  rxopt struct contains rxStatus, sourceNodeID, destNodeID and rssiVal */
{
  UNUSED(cmdLength);
  UNUSED(rxOpt);

  ZW_UART0_SEND_STR("Received ");
  ZW_UART0_SEND_NUM(*(BYTE *)pCmd);
  ZW_UART0_SEND_BYTE(' ');
  ZW_UART0_SEND_NUM(*((BYTE *)pCmd)+1);
  ZW_UART0_SEND_STR("\n\r");
}

/*==========================   GetNodeInformation   =========================
**    Request Node information and current status
**    Called by the the Z-Wave application layer before transmitting a
**    "Node Information" frame.
**
**    This is an application function example
**
**--------------------------------------------------------------------------*/
extern  void               /*RET Nothing */
ApplicationNodeInformation(
BYTE   *deviceOptionsMask,     /*OUT Bitmask with application options     */
#ifdef NEW_NODEINFO
  APPL_NODE_TYPE  *nodeType,  /*OUT  Device type Generic and Specific   */
#else
  BYTE       *nodeType,       /*OUT  Device type */
#endif
BYTE  **nodeParm,     /*OUT  Device parameter buffer pointer      */
BYTE  *parmLength )   /*OUT  Number of Device parameter bytes   */
{
  BYTE dimLevel;

  dimLevel = 0;
*deviceOptionsMask = APPLICATION_NODEINFO_LISTENING;             /* this is a powered node */
#ifdef NEW_NODEINFO
  nodeType->generic = GENERIC_TYPE_SWITCH_MULTILEVEL; /* Generic device type */
  nodeType->specific = 0;             /* We have no Specific class */
#else
  *nodeType = GENERIC_TYPE_SWITCH_MULTILEVEL; /* Device type */
#endif
  *nodeParm = (BYTE *)&dimLevel;
  *parmLength = sizeof(dimLevel);
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
* Unused, but mandatory function. This is not a secure product.
* Not used in ProdTestGen.
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

