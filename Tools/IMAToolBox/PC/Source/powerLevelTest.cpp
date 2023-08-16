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
 * Description: PowerLevel test module
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
#include "timing.h"
#include "powerLevelTest.h"

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#define TIMEOUT_60_SEC 60
typedef struct _PowerLevelTest
{
    BYTE bMyNodeId;
    BYTE bSourceNode;
    BYTE bDestNode;
    BYTE bPowerLevel;
    int  bLocalPowerLevel;
    int  NbrTestFrames;
    int  bLocalNbrTestFrames;
    short int  ackCountSuccess;
    CSerialAPI* pSerApi;
    static void PowerLevelTestSetAck(BYTE command);
    BOOL printActive;
    BOOL actionCancel;
    void (*completedFunc)(POWLEV_STATUS Status);
    CTime* pTimer;
    int timerH;
} tPowerLevelTest;


/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/
tPowerLevelTest plt;

/****************************************************************************/
/*                              PRIVATE function                            */
/****************************************************************************/
void PowerLevelTest_PowerLevelTestSetAck(BYTE command, TX_STATUS_TYPE *psTxInfo);
void PowerLevelTest_TestFrameAck(BYTE bStatus);
void PowerLevelTest_Status(BYTE sourceNode, BYTE destNode, BYTE status, short int ackCountSuccess);
void PowerLevelTest_CbTimeOut(int H);


/*========================   CmdHandlerPowerLevel  ========================
** Function description:
    Include this function call under Z-Wave CommandHandler callbacks switch-case 
    "COMMAND_CLASS_POWERLEVEL" as:
        case COMMAND_CLASS_POWERLEVEL:
            PowerLevelTest_CmdHandlerPowerLevel(  rxStatus, sourceNode, pCmd, cmdLength);
        break;

**--------------------------------------------------------------------------*/
void 
CmdHandlerPowerLevel(
    BYTE rxStatus, 
    BYTE sourceNode, 
    BYTE *pCmd, 
    BYTE cmdLength)
{
    switch(*(pCmd+CMDBUF_CMD_OFFSET))
    {
        case POWERLEVEL_TEST_NODE_GET:
            break;

        case POWERLEVEL_TEST_NODE_REPORT:
            {
                ZW_APPLICATION_TX_BUFFER* pD = (ZW_APPLICATION_TX_BUFFER*)pCmd;
                short int testFrameCount = pD->ZW_PowerlevelTestNodeReportFrame.testFrameCount2 |
                                           ((short int)(pD->ZW_PowerlevelTestNodeReportFrame.testFrameCount1) << 8);
                PowerLevelTest_Status(sourceNode, 
                                      pD->ZW_PowerlevelTestNodeReportFrame.testNodeid, 
                                      pD->ZW_PowerlevelTestNodeReportFrame.statusOfOperation, 
                                      testFrameCount);
            }
            break;

        case POWERLEVEL_TEST_NODE_SET:
            break;

        case POWERLEVEL_GET:
            break;

        case POWERLEVEL_REPORT:
            break;

        case POWERLEVEL_SET:
            break;
    }
}


/*===========================   PowerLevelTestSet   ==========================
** Function description:
    Constructor. 
**--------------------------------------------------------------------------*/
void
PowerLevelTest_CPowerLevelTest(
    CSerialAPI* pApi,
    BOOL printActive)
{
    plt.pSerApi = pApi;
    plt.NbrTestFrames = 0;
    plt.bMyNodeId = 0;
    plt.bSourceNode = 0;
    plt.bDestNode = 0;
    plt.bPowerLevel = 0;
    plt.ackCountSuccess = 0;
    plt.printActive = printActive;
    plt.actionCancel = FALSE;
    plt.completedFunc = NULL;
    plt.pTimer = CTime::GetInstance();
}


/*========================   Init   ========================================
** Function description
    Init test setup 
**--------------------------------------------------------------------------*/
void
PowerLevelTest_Init(
    BYTE MyNodeId,
    BYTE bSourceNode,
    BYTE bDestNode,
    BYTE bPowerLevel,
    int NbrTestFrames, 
    bool fSwitchSource)
{
    plt.NbrTestFrames = NbrTestFrames;
    plt.bMyNodeId = MyNodeId;
    plt.bSourceNode = bSourceNode;
    plt.bDestNode = bDestNode;
    plt.bPowerLevel = bPowerLevel;
    plt.ackCountSuccess = 0;
    plt.actionCancel = FALSE;

    /* Switch sourceNode and destNode if (fSwitchSource == true) */
    /* we assume PowerLevelSet is not locally implemented,  */
    /* either HOST or SerialApi are missing needed functioanlity. */
    if (fSwitchSource && (plt.bMyNodeId == bSourceNode))
    {
        plt.bSourceNode = bDestNode;
        plt.bDestNode = bSourceNode;
    }
}


/*========================   Status  =======================================
** Function description
    Handle status call from POWERLEVEL_TEST_NODE_REPORT frame and write it 
    to cmd-window.
**--------------------------------------------------------------------------*/
void 
PowerLevelTest_Status(
    BYTE sourceNode, 
    BYTE destNode, 
    BYTE status, 
    short int ackCountSuccess)
{
    /* Check if Timer is running. If Timer is not running then a timeout has */
    /* occured or process has finished and situation have allready been handled. */
    if (0 != plt.timerH)
    {
        plt.pTimer->Kill(plt.timerH);
        plt.timerH = 0;
        if (NULL != plt.completedFunc)
        {
            POWLEV_STATUS powStatus;
            powStatus.Status = (ePowLevStatus)(status + 0x10);
            powStatus.sourceNode = sourceNode;
            powStatus.destNode = destNode;
            powStatus.PowerLevel = plt.bPowerLevel;
            powStatus.ackCountSuccess = ackCountSuccess;
            powStatus.NbrTestFrames = plt.NbrTestFrames;
        
            if (0 <= plt.bLocalPowerLevel)
            {
                /* Reset z-Wave module RF PowerLevel to initial PowerLevel */
                plt.pSerApi->ZW_RF_Power_Level_Set(plt.bLocalPowerLevel);
                plt.bLocalPowerLevel = -1;
            }
            plt.completedFunc(powStatus);
        }
    }
}


/*========================   Set   ==========================================
** Function description
    Start test by sending PowerLevelTestSet frame or if local sending the
    test frame.
**--------------------------------------------------------------------------*/
int 
PowerLevelTest_Set(
    void (*completedFunc)(POWLEV_STATUS Status))
{
    int retVal = 0;

    plt.ackCountSuccess = 0;
    plt.bLocalNbrTestFrames = 0;
    plt.bLocalPowerLevel = -1;
    plt.completedFunc = completedFunc;
    plt.timerH = plt.pTimer->Timeout(TIMEOUT_60_SEC/2, &PowerLevelTest_CbTimeOut);
    if (0 == plt.timerH)
    {
        printf("PowerLevelTest_Set  no timer! ERROR!");
    }

    if (NULL != plt.pSerApi)
    {
        if (plt.bMyNodeId == plt.bSourceNode)
        {
            /* Local Node is to execute the PowerLevel Test */
            /* Get current Powerlevel - so we can reset it when test done */
            plt.bLocalPowerLevel = plt.pSerApi->ZW_RF_Power_Level_Get();
            /* Set Test Powerlevel */
            plt.pSerApi->ZW_RF_Power_Level_Set(plt.bPowerLevel);
            /* Send the first Test Frame to destination node */
            retVal = plt.pSerApi->ZW_SendTestFrame(plt.bDestNode, plt.bPowerLevel, PowerLevelTest_TestFrameAck);
        }
        else
        {
            /* PowerLevel Test is to be done by another node */
            ZW_APPLICATION_TX_BUFFER command;
            command.ZW_PowerlevelTestNodeSetFrame.cmd = POWERLEVEL_TEST_NODE_SET;
            command.ZW_PowerlevelTestNodeSetFrame.cmdClass = COMMAND_CLASS_POWERLEVEL;
            command.ZW_PowerlevelTestNodeSetFrame.testNodeid = plt.bDestNode;
            command.ZW_PowerlevelTestNodeSetFrame.powerLevel = plt.bPowerLevel;
            command.ZW_PowerlevelTestNodeSetFrame.testFrameCount1 = (BYTE)((plt.NbrTestFrames >> 8) & 0xff);
            command.ZW_PowerlevelTestNodeSetFrame.testFrameCount2 = (BYTE)(plt.NbrTestFrames & 0xff);
            retVal = plt.pSerApi->ZW_SendData(plt.bSourceNode, (BYTE*)&command, sizeof(ZW_POWERLEVEL_TEST_NODE_SET_FRAME),
                                              TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK,
                                              PowerLevelTest_PowerLevelTestSetAck);
        }
    }
    return retVal;
}


/*========================   IncPowerLevel   ===============================
** Function description
    Increment PowerLevel parameter.
**--------------------------------------------------------------------------*/
BYTE 
PowerLevelTest_IncPowerLevel()
{
    if (0 < plt.bPowerLevel)
    {
        plt.bPowerLevel--;
    }
    return plt.bPowerLevel;
}


/*========================   GetPowerLevel   ===============================
** Function description
    Read PowerLevel parameter.
**--------------------------------------------------------------------------*/
BYTE 
PowerLevelTest_GetPowerLevel()
{
    return plt.bPowerLevel;
}

/*========================   GetTestResult   ===============================
** Function description
    Get number of test frames transmitted, which the Test NodeID has 
    acknowledged. 
**--------------------------------------------------------------------------*/
short int 
PowerLevelTest_GetTestResult()
{
    return plt.ackCountSuccess;
}


/*===================   PowerLevelTest_TestFrameAck   ========================
** Function description
    Get number of test frames transmitted, which the Test NodeID has 
    acknowledged. 
**--------------------------------------------------------------------------*/
void
PowerLevelTest_TestFrameAck(
    BYTE bStatus)
{
    /* Abort if actionCancel or no Timer is running */
    if ((FALSE == plt.actionCancel) && (0 != plt.timerH))
    {
        if (bStatus == TRANSMIT_COMPLETE_OK)
        {
            plt.ackCountSuccess++;
        }
        plt.bLocalNbrTestFrames++;
        if (plt.bLocalNbrTestFrames < plt.NbrTestFrames)
        {
            /* Send the next Test Frame to destination node */
            plt.pSerApi->ZW_SendTestFrame(plt.bDestNode, plt.bPowerLevel, PowerLevelTest_TestFrameAck);
        }
        else
        {
            plt.actionCancel = TRUE;
            if (0 <= plt.bLocalPowerLevel)
            {
                /* Reset z-Wave module RF PowerLevel */
                plt.pSerApi->ZW_RF_Power_Level_Set(plt.bLocalPowerLevel);
                plt.bLocalPowerLevel = -1;
            }
            PowerLevelTest_Status(plt.bSourceNode, plt.bDestNode,
                                  (bStatus == TRANSMIT_COMPLETE_OK) ? POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_SUCCES : POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_FAILED,
                                  plt.NbrTestFrames);
        }
    }
}


/*========================   PowerLevelTestSetAck   =========================
** Function description
Ack status on PowerLevelTest_Set() call. 
**--------------------------------------------------------------------------*/
void 
PowerLevelTest_PowerLevelTestSetAck(
    BYTE command, 
	TX_STATUS_TYPE *psTxInfo)
{
    if (command != TRANSMIT_COMPLETE_OK)
    {
        /* If no Timer running then Test is done */
        if (0 != plt.timerH)
        {
            plt.pTimer->Kill(plt.timerH);
            plt.timerH = 0;
            if (NULL != plt.completedFunc)
            {
                POWLEV_STATUS powStatus;
                powStatus.Status = (ePowLevStatus)(command);
                powStatus.sourceNode = plt.bSourceNode;
                powStatus.destNode = plt.bDestNode;
                powStatus.PowerLevel = plt.bPowerLevel;
                powStatus.ackCountSuccess = plt.ackCountSuccess;
                powStatus.NbrTestFrames = plt.NbrTestFrames;
                
                plt.completedFunc(powStatus);
            }
        }
    }
}


/*========================   PowerLevelTest_CbTimeOut   =========================
** Function description
PowerLevelTest_Set() CB-timeout function. 
**--------------------------------------------------------------------------*/
void
PowerLevelTest_CbTimeOut(
    int H)
{
    if (0 != plt.timerH)
    {
        /* Now Timer is */
        plt.timerH = 0;
        if (NULL != plt.completedFunc)
        {
            POWLEV_STATUS powStatus;
            powStatus.Status = POWERLEV_TEST_NODE_REPORT_ZW_TEST_TIMEOUT;
            powStatus.sourceNode = plt.bSourceNode;
            powStatus.destNode = plt.bDestNode;
            powStatus.PowerLevel = plt.bPowerLevel;
            powStatus.ackCountSuccess = plt.ackCountSuccess;
            powStatus.NbrTestFrames = plt.NbrTestFrames;
            plt.actionCancel = TRUE;
            if (0 <= plt.bLocalPowerLevel)
            {
                /* PowerLevel Test has been executed locally */
                /* Reset z-Wave module RF PowerLevel */
                plt.pSerApi->ZW_RF_Power_Level_Set(plt.bLocalPowerLevel);
                plt.bLocalPowerLevel = -1;
            }
            plt.completedFunc(powStatus);
        }
    }
}


/*========================   PowerLevelTest_GetSourceNode   ==================
** Function description
 
**--------------------------------------------------------------------------*/
BYTE
PowerLevelTest_GetSourceNode()
{ 
    return plt.bSourceNode;
}


/*========================   PowerLevelTest_GetSourceNode   ==================
** Function description
 
**--------------------------------------------------------------------------*/
BYTE 
PowerLevelTest_GetDestNode()
{ 
    return plt.bDestNode;
}


/*========================   PowerLevelTest_UserActionCancel   ===============
** Function description
 
**--------------------------------------------------------------------------*/
void 
PowerLevelTest_UserActionCancel()
{
    plt.actionCancel = TRUE;   
}
