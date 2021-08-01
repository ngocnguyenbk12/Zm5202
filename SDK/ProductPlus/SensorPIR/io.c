/***************************************************************************
*
* Copyright (c) 2001-2014
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Some nice descriptive description.
*
* Author: Thomas Roll
*
* Last Changed By: $Author: tro $
* Revision: $Revision: 0.00 $
* Last Changed: $Date: 2014/12/15 20:09:03 $
*
****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include "io_zdp03a.h"

#include <ZW050x.h>
#include <ZW_uart_api.h>
#include <ZW_nvr_api.h>
#include <appl_timer_api.h>
#include <key_driver.h>
#include <gpio_driver.h>
#include <misc.h>
#include <ota_util.h>
#include <CommandClassVersion.h>
#include <ZW_string.h>
#include <ZW_security_api.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#ifdef ZW_DEBUG_IO
#define ZW_DEBUG_IO_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_IO_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_IO_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_IO_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_IO_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_IO_SEND_BYTE(data)
#define ZW_DEBUG_IO_SEND_STR(STR)
#define ZW_DEBUG_IO_SEND_NUM(data)
#define ZW_DEBUG_IO_SEND_WORD_NUM(data)
#define ZW_DEBUG_IO_SEND_NL()
#endif

//#define IO_HELP
//#define IO_EXTRAS


typedef struct _IO_ZDP03A_
{
  VOID_CALLBACKFUNC(pFlasTimeout)(void);
  BYTE bTimerHandle;
} IO_ZDP03A;


typedef struct _KEYMAN_
{
  VOID_CALLBACKFUNC(pEventKeyQueue)(BYTE);
} t_KEYMAN;


/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

IO_ZDP03A myIo = { NULL, 0};
t_KEYMAN mykeyMan = {NULL};
/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/
void ZCB_KeyAction(KEY_NAME_T keyName, KEY_EVENT_T keyEvent, BYTE holdCount);


/**
 * List here the pins which are used as GPIO and have been pin swapped.
 * The pins listed here are the two swapped pins on the Z-Wave ZM5202
 * Development Module which is used as LED and button at the Z-Wave ZDP03A
 * Development Platform.
 */
static PIN_T_ARRAY xdata pins = {
        {0x07, 0x10},
        {0x22, 0x32}
};

void
gpio_GetPinSwapList(PIN_T_ARRAY xdata ** pPinList, BYTE * const pPinListSize)
{
  *pPinList = (PIN_T_ARRAY xdata *)&pins;
  *pPinListSize = sizeof(pins) / sizeof(PIN_T);
}



/*============================ InitZDP03A ===============================
**-------------------------------------------------------------------------*/
void ZDP03A_InitHW(
  VOID_CALLBACKFUNC(pEventKeyQueue)(BYTE))             /*function-pointer to application event-queue or state machine*/
{
  ZW_DEBUG_IO_SEND_STR("IO_Init");

  gpio_DriverInit(TRUE);
  mykeyMan.pEventKeyQueue = pEventKeyQueue;

  /*
   * Initialize keys.
   */
  if (!KeyDriverInit())
  {
    ZW_DEBUG_IO_SEND_NL();
    ZW_DEBUG_IO_SEND_STR("Couldn't initialize Key Driver :(");
  }

  /**
   * Setup interrupt for EXT1 which is used by the key driver.
   */
  gpio_SetPinIn(ZDP03A_KEY_INTP, TRUE);
  ZW_SetExtIntLevel(ZW_INT1, FALSE);
  EX1 = 1; // Enable external interrupt 1
}

void SetPinIn( ZDP03A_KEY key, BOOL pullUp)
{
    ZW_DEBUG_IO_SEND_NL();
    ZW_DEBUG_IO_SEND_STR("SetPinIn key");
    ZW_DEBUG_IO_SEND_NUM(key);
    ZW_DEBUG_IO_SEND_NUM(pullUp);
  switch(key)
  {
    case ZDP03A_KEY_1:
      KeyDriverRegisterCallback(KEY01, key, BITFIELD_KEY_DOWN , ZCB_KeyAction);
      break;
    case ZDP03A_KEY_2:
      KeyDriverRegisterCallback(KEY02, key, BITFIELD_KEY_DOWN , ZCB_KeyAction);
      break;
    case ZDP03A_KEY_3:
      // ZDP03A S3 is used for SPI/NVM.
      break;
    case ZDP03A_KEY_4:
      KeyDriverRegisterCallback(KEY04, key, BITFIELD_KEY_DOWN , ZCB_KeyAction);
      break;
    case ZDP03A_KEY_5:
      KeyDriverRegisterCallback(KEY05, key, BITFIELD_KEY_DOWN , ZCB_KeyAction);
      break;
    case ZDP03A_KEY_6:
      KeyDriverRegisterCallback(KEY06, key, BITFIELD_KEY_DOWN , ZCB_KeyAction);
      break;
  }
  gpio_SetPinIn(key, pullUp);
}

void SetPinOut( LED_OUT led)
{
  gpio_SetPinOut(led);
}


/*============================ Led ===============================
**-------------------------------------------------------------------------*/
void Led( LED_OUT led,  LED_ACTION action)
{
  gpio_SetPin(led, action);
}


/*============================ ButtonGet ===============================
**-------------------------------------------------------------------------*/
BYTE KeyGet(ZDP03A_KEY key)
{
  return gpio_GetPin( key);
}

/**
 * Uswed to calc key-BX group start
 */
EVENT_KEY keyGrpStart[NUMBER_OF_KEYS] =
{
  EVENT_KEY_B1_DOWN,
  EVENT_KEY_B2_DOWN,
  EVENT_KEY_B3_DOWN,
  EVENT_KEY_B4_DOWN,
  EVENT_KEY_B5_DOWN,
  EVENT_KEY_B6_DOWN
};

/**
 * @brief Call specific Key0XAction function dependent on keyName.
 */
PCB(ZCB_KeyAction)(KEY_NAME_T keyName, KEY_EVENT_T keyEvent, BYTE holdCount)
{
  UNUSED(holdCount);

  ZW_DEBUG_IO_SEND_STR("ZCB_KeyAction ");
  ZW_DEBUG_IO_SEND_NUM(keyName);
  ZW_DEBUG_IO_SEND_NUM(keyEvent);
  ZW_DEBUG_IO_SEND_NUM(holdCount);
  ZW_DEBUG_IO_SEND_NL();

  if(IS_NULL(mykeyMan.pEventKeyQueue) || (KEY_DOWN != keyEvent))
  {
    return;
  }

  mykeyMan.pEventKeyQueue(keyGrpStart[keyName] + keyEvent);
}


