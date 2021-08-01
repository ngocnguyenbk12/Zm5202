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
 * Description: PowerLevelTest Manager module
 *
 * Last Changed By:  $Author$: 
 * Revision:         $Rev$: 
 * Last Changed:     $Date$: 
 *
 ****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include "stdafx.h"

#include <stdlib.h>
#include <stdio.h>
#include "SerialAPI.h"
#include "powLevTestMan.h"


/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

typedef struct _PowerLevelTestMan
{
    BYTE bGatwayNodeId;
    BYTE bRepeater;
    BYTE bSourceId;
    BYTE bDestId;
    BYTE measLevelSource;
    BYTE measLevelDest;
    BYTE bPowerLevel;
    int  nbrTestFrames;
    BOOL actionCancel;
    BYTE job;
    POWLEVMAN_STATUS status[2];
    //POWLEV_STATUS Status;
    void (*pF)(POWLEVMAN_STATUS* pSource,POWLEVMAN_STATUS* pDest);
} PowerLevelTestMan;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/
PowerLevelTestMan pltm;

/****************************************************************************/
/*                              PRIVATE function                            */
/****************************************************************************/
BYTE FindPowerLevel(void);

/*========================   CPowerLevelTestMan  ============================
** Function description
    Constructor
**--------------------------------------------------------------------------*/
void 
PowerLevelTestMan_CPowerLevelTestMan(
	CSerialAPI* pApi,
	BYTE bGatwayNodeId, 
	BYTE bSourceId, 
	BYTE bRepeater, 
	BYTE bDestId, 
	void (*pF)(POWLEVMAN_STATUS* pSource,
	POWLEVMAN_STATUS* pDestination))
{
    pltm.bGatwayNodeId = bGatwayNodeId;
    pltm.bRepeater = bRepeater;
    pltm.bSourceId = bSourceId;
    pltm.bDestId = bDestId;
    pltm.bPowerLevel = 9;
    pltm.nbrTestFrames = POWERLEVEL_FRAMECOUNT;
    pltm.actionCancel = FALSE;
    pltm.pF = pF;
    PowerLevelTest_CPowerLevelTest(pApi, FALSE);
    PowerLevelTestMan_ReRun();
}


/*========================   ReRun  =========================================
** Function description
    ReRun test.
**--------------------------------------------------------------------------*/
void 
PowerLevelTestMan_ReRun()
{
    if(0 == pltm.bRepeater || 0 == pltm.bSourceId || 0 == pltm.bDestId)
    {
        if(0 ==pltm.bRepeater)
            printf("\nError! repeater node ID is 0!!\n");
        if(0 ==pltm.bSourceId)
            printf("\nError! destination 1 node ID is 0!!\n");
        if(0 ==pltm.bDestId)
            printf("\nError! destination 2 node ID is 0!!\n");
        return;
    }
    PowerLevelTest_Init(pltm.bGatwayNodeId, pltm.bRepeater, pltm.bSourceId, pltm.bPowerLevel, pltm.nbrTestFrames, true);
    pltm.job = 0;
    pltm.measLevelSource = FindPowerLevel( );
    
    //If first test fail stop second test!
    if( 0xff != pltm.measLevelSource)
    {
        PowerLevelTest_Init(pltm.bGatwayNodeId, pltm.bDestId, pltm.bRepeater, pltm.bPowerLevel, pltm.nbrTestFrames, true);
        pltm.job = 1;
        pltm.measLevelDest = FindPowerLevel( );
    }    
    if( NULL != pltm.pF)
    {
        pltm.pF( &pltm.status[0], &pltm.status[1]);
    }
}

/*========================   completedFunc  ================================
** Function description
**--------------------------------------------------------------------------*/
void 
completedFunc(
	POWLEV_STATUS powStatus)
{
    memcpy( &(pltm.status[pltm.job].PLTStatus), &powStatus, sizeof(powStatus));

    if(  TRANSMIT_COMPLETE_NO_ACK == powStatus.Status ||
         TRANSMIT_COMPLETE_FAIL== powStatus.Status ||
         POWERLEV_TEST_NODE_REPORT_ZW_TEST_TIMEOUT == powStatus.Status)
    {
        pltm.status[pltm.job].PLMstatus = POWLEV_MAN_FAILED;
    }
    else
    {
        if( 6 > PowerLevelTest_GetPowerLevel())
        {
            pltm.status[pltm.job].PLMstatus = POWLEV_MAN_WARNING_OVER_INSTALLER_LEVEL;
        }
        else
        {
            pltm.status[pltm.job].PLMstatus = POWLEV_MAN_SUCCESS;
        }
    }
}

/*========================   FindPowerLevel  ================================
** Function description
    Step up in power level to Test Frame acknowledged count is 100 %. Return
    power level for the test. Return 0xFF if test failed.
**--------------------------------------------------------------------------*/
BYTE 
FindPowerLevel()
{
    BOOL endTest = FALSE;

    printf("\n\nTest Power Level between source %u to dest. %u.", 
        PowerLevelTest_GetSourceNode(), PowerLevelTest_GetDestNode());
    do{
        printf("\nTry levels: %u",  PowerLevelTest_GetPowerLevel());
        printf("\n Busy!");
        
        //Start PowerLevelTestSet
        pltm.status[pltm.job].PLMstatus = POWLEV_MAN_WAITING;
        PowerLevelTest_Set(&completedFunc);
        

        //Wait to result is delivered
        while( POWLEV_MAN_WAITING == pltm.status[pltm.job].PLMstatus)
        {
            Sleep(500);
            printf(".");
        }
        
        if(POWLEV_MAN_FAILED == pltm.status[pltm.job].PLMstatus)
        {
            /*Test failed!*/
            return 0xff;

        }
        //If not all test frames are received, increase power level.
        if(pltm.nbrTestFrames > PowerLevelTest_GetTestResult())
        {

            if(0 == PowerLevelTest_GetPowerLevel())
            {
                // max power level and not 100% frames. Fail the test!!
                return 0xff;
            }
            else
            {
                PowerLevelTest_IncPowerLevel();
            }
        }
        else
        {
           endTest = TRUE;
        }
    }while( FALSE == endTest );
    printf("\n");
    
    return PowerLevelTest_GetPowerLevel();
}


BYTE 
PowerLevelTestMan_GetRepeaterId()
{
    return pltm.bRepeater;
}


BYTE 
PowerLevelTestMan_GetSourceId()
{
    return pltm.bSourceId;
}

BYTE 
PowerLevelTestMan_GetDestinationId()
{
    return pltm.bDestId;
}

void 
PowerLevelTestMan_UserActionCancel()
{
    pltm.actionCancel = TRUE; 
    PowerLevelTest_UserActionCancel();
}
