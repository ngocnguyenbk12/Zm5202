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
 * Description: Header file for PowerLevel test module
 *
 * Last Changed By:  $Author$: 
 * Revision:         $Rev$: 
 * Last Changed:     $Date$: 
 *
 ****************************************************************************/

#ifndef _POWERLEVEL_TEST_H_
#define _POWERLEVEL_TEST_H_

#define POWERLEVEL_FRAMECOUNT 10

typedef enum _ePowLevStatus
{
    POWERLEV_TEST_TRANSMIT_COMPLETE_OK           = 0x00,  
    POWERLEV_TEST_TRANSMIT_COMPLETE_NO_ACK       = 0x01,  /* retransmission error */
    POWERLEV_TEST_TRANSMIT_COMPLETE_FAIL         = 0x02,  /* transmit error */ 
    POWERLEV_TEST_NODE_REPORT_ZW_TEST_FAILED     = 0x10,
    POWERLEV_TEST_NODE_REPORT_ZW_TEST_SUCCES     = 0x11,
    POWERLEV_TEST_NODE_REPORT_ZW_TEST_INPROGRESS = 0x12,
    POWERLEV_TEST_NODE_REPORT_ZW_TEST_TIMEOUT    = 0x13
} ePowLevStatus;


typedef struct _POWLEV_STATUS
{
    ePowLevStatus Status; 
    BYTE sourceNode; 
    BYTE destNode; 
    BYTE PowerLevel;
    int ackCountSuccess;
    int NbrTestFrames;
} POWLEV_STATUS;


void PowerLevelTest_CPowerLevelTest(CSerialAPI* pApi, BOOL printActive);
void PowerLevelTest_Init(BYTE MyNodeId, BYTE bSourceNode, BYTE bDestNode, BYTE bPowerLevel, int NbrTestFrames, bool fSwitchSource);
int PowerLevelTest_Set(void (*completedFunc)(POWLEV_STATUS Status));
BYTE PowerLevelTest_IncPowerLevel();
BYTE PowerLevelTest_GetPowerLevel();
short int PowerLevelTest_GetTestResult();
ePowLevStatus PowerLevelTest_Busy();
BYTE PowerLevelTest_GetSourceNode();
BYTE PowerLevelTest_GetDestNode();
void CmdHandlerPowerLevel(  BYTE rxStatus, BYTE sourceNode, BYTE *pCmd, BYTE cmdLength);
void PowerLevelTest_UserActionCancel();

#endif /*_POWERLEVEL_TEST_H_*/

