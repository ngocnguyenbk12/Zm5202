/*
 * config_pin.c
 *
 *  Created on: 08/05/2015
 *      Author: COlsen
 */
#include <gpio_driver.h>

/**
 * List here the pins which are used as GPIO and have been pin swapped.
 * The pins listed here are the two swapped pins on the Z-Wave ZM5202
 * Development Module which is used as LED and button at the Z-Wave ZDP03A
 * Development Platform.
 */
static const PIN_T pins[] = {
        {07, 10},
        {22, 32}
};

void
gpio_getPinSwapList(const PIN_T * pPinList, BYTE * const pPinListSize)
{
  pPinList = pins;
  *pPinListSize = sizeof(pins);
}
