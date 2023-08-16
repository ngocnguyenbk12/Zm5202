/****************************************************************************
 *
 * Z-Wave, the wireless language.
 *
 * Copyright (c) 2001-2011
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description: Header file for PowerLevelTest Manager module
 *
 * Last Changed By:  $Author$: 
 * Revision:         $Rev$: 
 * Last Changed:     $Date$: 
 *
 ****************************************************************************/

#ifndef _POWLEVTEST_MAN_H_
#define _POWLEVTEST_MAN_H_

#include "powerleveltest.h"

typedef enum
{
    POWLEV_MAN_SUCCESS,
    POWLEV_MAN_WARNING_OVER_INSTALLER_LEVEL,
    POWLEV_MAN_FAILED,
    POWLEV_MAN_WAITING
} ePowLevManStatus;

typedef struct _POWLEVMAN_STATUS
{
    ePowLevManStatus PLMstatus; 
    POWLEV_STATUS PLTStatus;
} POWLEVMAN_STATUS;

  void PowerLevelTestMan_ReRun();
  void PowerLevelTestMan_CPowerLevelTestMan(CSerialAPI* pApi, BYTE bGatwayNodeId, 
                                            BYTE bSourceId, BYTE bRepeater, BYTE bDestId, 
                                            void (*pF)(POWLEVMAN_STATUS* pSource,POWLEVMAN_STATUS* pDestination)
                                            );

  BYTE PowerLevelTestMan_GetRepeaterId();
  BYTE PowerLevelTestMan_GetSourceId();
  BYTE PowerLevelTestMan_GetDestinationId();
  void PowerLevelTestMan_UserActionCancel();
#endif /*_POWLEVTEST_MAN_H_*/





