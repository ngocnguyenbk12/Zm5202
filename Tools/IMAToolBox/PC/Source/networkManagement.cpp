/****************************************************************************
 *
 * Z-Wave, the wireless language.
 * 
 * Copyright (c) 2012
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description: Z-Wave Network Health module
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
#include <conio.h>
#include <cmath>

#include <sys/timeb.h>
#include <time.h>

#include "IMAtoolbox.h"
#include "timing.h"
#include "networkManagement.h"
#include "powerLevelTest.h"
#include "ZW_SerialAPI.h"
#include "logging.h"
#include "RssiTest.h"

sNetworkManagement_Stat *nodeStats;
sNetworkManagement *spNetworkManagement;
sSample *spCurrentSample;

FILE *fp_statLog;

char networkHealth_logfilenameStr[255];

sLOG *networkHealthlog = NULL;

CTime *pSecTimer;
int timerSecH = 0;

long lNetworkManagementTimeSecondsTicker = 0;
long lNetworkManagementCurrentSecondsTickerSample;
long iSecondsBetweenEveryMaintenanceSample;

sNetworkManagement_Stat *singleRediscoveryStats;

sTiming sTxTiming;
BYTE *pabLWRRoute;
int bLWRRouteIndex;
bool pafLWRNodesSupportPowerLevel[6];
int iLWRNumberOfNodesInLink;

BYTE testConnectionPayload[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
BYTE testConnectionTxOptions = TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_EXPLORE;
BYTE bDutNodeID;
BOOL requestUpdateOn = false;

/* Internal callback function used for sampling transmit time for Maintenance ZW_SendData usage */
/* The resulting sample are placed into the samples (fifo) and the updated samples are then used */
/* to calculate the Network Health for the specified node. */
void (__cdecl *cbNetworkManagement_ZW_SendData)(BYTE, TX_STATUS_TYPE *psTxInfo);
/* Internal callback funtion used for sampling transmit time for NH/Maintenance ZW_SendData usage. */
void (__cdecl *cbNetworkManagement_Timed_ZW_SendData)(BYTE, TX_STATUS_TYPE *);
/* */
void (__cdecl *cbNetworkManagement_DoRediscoverSingle)(BYTE);

/* Main logging structure for adding/writing logs */
extern CLogging mlog;

/* HOST main Log file */
extern void mainlog_wr(const char * fmt, ...);

/* HOST SerialAPI interface to Z-Wave SerialAPI Module */
extern CSerialAPI api;

bool StartNextNetworkHealthTest();

void NextNetworkHealthTestRound();

bool StartNextSingleNodeRediscovery();


/*============================= AreNodeANeighbor =============================
** Function description
**      Checks if bNodeID is a neighbor according to the neighbor nodemask
** specified abNodeMask
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
bool
AreNodeANeighbor(
    BYTE bNodeID,
    BYTE *abNodeMask)
{
    bool retVal = false;

    if ((bNodeID > 0) && (bNodeID <= ZW_MAX_NODES))
    {
		retVal = api.BitMask_isBitSet(abNodeMask, bNodeID);
    }
    return retVal;
}


/*===================== NetworkManagement_CheckRepeater =====================
** Function description
**      Check if a repeater is neighbor to the destination and if it has 
**      a Green neighbor or is neighbor to the controller
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
bool
NetworkManagement_CheckRepeater(
    BYTE bRepNodeID, 
    BYTE bDestNodeID,
    BYTE bMyNodeId,
    sNetworkManagement *spNetworkMan)
{
    int i,t;
    BYTE abRoutingInfoNodeMask[ZW_MAX_NODEMASK_LENGTH];

    printf("CheckRepeater(%i, %i, %i)\n",bRepNodeID, bDestNodeID, bMyNodeId);
    printf("Repeater list for %i - ", bRepNodeID);

    /* Get all neighbors for repeater */
    memset(abRoutingInfoNodeMask, 0, sizeof(abRoutingInfoNodeMask));
    api.ZW_GetRoutingInfo(bRepNodeID, abRoutingInfoNodeMask, FALSE, FALSE);

    /* Debug output */
	for (t = 1; t <= ZW_MAX_NODES; t++)
    {
		if (api.BitMask_isBitSet(abRoutingInfoNodeMask, t))
        {
            printf("%i ", t);
        }
    }
    printf("\n");

    /* Check if repeater is neighbor to destination */
	if (0 == api.BitMask_isBitSet(abRoutingInfoNodeMask, bDestNodeID))
    {
        printf("Not neighbor to destination\n");
        return false;
    }

    /* Check if repeater is neighbor to this controller */
	if (1 == api.BitMask_isBitSet(abRoutingInfoNodeMask, bMyNodeId))
    {
        printf("Neighbor to Controller\n");
        return true;
    }

    /* The controller is not a neighbor, check if it has another repeater as neighbor */
    /* Get all repeater neighbors for the repeater */
    memset(abRoutingInfoNodeMask, 0, sizeof(abRoutingInfoNodeMask));
    api.ZW_GetRoutingInfo(bRepNodeID, abRoutingInfoNodeMask, FALSE, TRUE);

    /* Check if one of the repeaters has a Green NHS (NH >=8 ) in the last test */
	for (t = 1; t <= ZW_MAX_NODES; t++)
    {
		if (1 == api.BitMask_isBitSet(abRoutingInfoNodeMask, t))
        {
            for (i = 0; i < spNetworkMan->bNodesUnderTestSize; i++)
            {
                if (spNetworkMan->abNodesUnderTest[i] == t)
                {
                    if (spNetworkMan->nodeUT[spNetworkMan->abNodesUnderTest[i] - 1].bDestNodeID == spNetworkMan->abNodesUnderTest[i])
                    {
                        if (spNetworkMan->nodeUT[spNetworkMan->abNodesUnderTest[i] - 1].NetworkHealthNumber > 7)
                        {
                            printf("Neighbor to Green Repeater %i\n", spNetworkMan->abNodesUnderTest[i]);
                            return true;
                        }
                    }
                    else
                    {
                        printf("Found Neighbor but no Network Health Test has been done on Repeater %i\n", spNetworkMan->abNodesUnderTest[i]);
                    }
                }
            }
        }
    }
    return false;
}


/*=================== NetworkManagement_UpdateNeighborInfo ===================
** Function description
**      Update neighbor info in structure pointed to by spNodeStat.
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
NetworkManagement_UpdateNodeNeighbor(
    BYTE bControllerNodeID,
    sNetworkManagement_Stat *spNodeStat)
{
    BYTE bNeighborCount = 0;

    memset(spNodeStat->abRoutingInfoNodeMask, 0, sizeof(spNodeStat->abRoutingInfoNodeMask));
    api.ZW_GetRoutingInfo(spNodeStat->bDestNodeID, spNodeStat->abRoutingInfoNodeMask, FALSE, FALSE);
    if ((bControllerNodeID != spNodeStat->bDestNodeID) && 
        AreNodeANeighbor(bControllerNodeID, spNodeStat->abRoutingInfoNodeMask))
    {
        /* bControllerNodeID/GW are a Neighbor */
        spNodeStat->abNeighbors[bNeighborCount] = bControllerNodeID;
		spNodeStat->bControllerIsANeighborAndNotARepeater = true;
        bNeighborCount++;
    }
	else
	{
		spNodeStat->bControllerIsANeighborAndNotARepeater = false;
	}
    api.ZW_GetRoutingInfo(spNodeStat->bDestNodeID, spNodeStat->abRoutingInfoNodeMask, FALSE, TRUE);
    for (int bCurrentNodeID = 1; bCurrentNodeID <= ZW_MAX_NODES; bCurrentNodeID++)
    {
		if (AreNodeANeighbor(bCurrentNodeID, spNodeStat->abRoutingInfoNodeMask))
		{
			/* We must not count bControllerNodeID/GW twice */
			if (bCurrentNodeID != bControllerNodeID)
			{
				/* Another neighbor found */
				spNodeStat->abNeighbors[bNeighborCount] = bCurrentNodeID;
				bNeighborCount++;
			}
			else
			{
				/* Controller is a neighbor but also a repeater */
				spNodeStat->bControllerIsANeighborAndNotARepeater = false;
			}
		}
    }
    spNodeStat->neighborCount = bNeighborCount;
}


/*===================== NetworkManagement_DumpNodeNeighbors ======================
** Function description
**      Dump the registered neighbors for the node specified with an index (i) 
** into the abNodesUnderTest list
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
NetworkManagement_DumpNodeNeighbors(
	BYTE i)
{
	BYTE abRoutingInfoNodeMask[232/8];
	sNetworkManagement_Stat *spNodeStat;
	BYTE bCurrentNode;
	char pCharBuf[1024];
    int j;
    char cch = ',';

    bCurrentNode = spNetworkManagement->abNodesUnderTest[i];
    spNodeStat = &spNetworkManagement->nodeUT[bCurrentNode - 1];
    /* First dump all neighbors */
    api.ZW_GetRoutingInfo(bCurrentNode, abRoutingInfoNodeMask, FALSE, FALSE);
    j = 0;
    memset(pCharBuf, 0, sizeof(pCharBuf));
    sprintf_s(pCharBuf, sizeof(pCharBuf), "[");
    for (int n = 1; n <= ZW_MAX_NODES; n++)
    {
        if (AreNodeANeighbor(n, abRoutingInfoNodeMask))
        {
            if (j++ != 0)
            {
                sprintf_s(&pCharBuf[strlen(pCharBuf)], sizeof(pCharBuf)-strlen(pCharBuf), "%c", cch);
            }
            sprintf_s(&pCharBuf[strlen(pCharBuf)], sizeof(pCharBuf)-strlen(pCharBuf), "%u", n);
        }
    }
    sprintf_s(&pCharBuf[strlen(pCharBuf)], sizeof(pCharBuf) - strlen(pCharBuf), "]");
    mainlog_wr("Node %03u - 'All' %u NBs %s", bCurrentNode, j, pCharBuf);
    /* Now dump repeaters only */
    api.ZW_GetRoutingInfo(bCurrentNode, abRoutingInfoNodeMask, FALSE, TRUE);
    j = 0;
    memset(pCharBuf, 0, sizeof(pCharBuf));
    sprintf_s(pCharBuf, sizeof(pCharBuf), "[");
    for (int n = 1; n <= ZW_MAX_NODES; n++)
    {
        if (AreNodeANeighbor(n, abRoutingInfoNodeMask))
        {
            if (j++ != 0)
            {
                sprintf_s(&pCharBuf[strlen(pCharBuf)], sizeof(pCharBuf)-strlen(pCharBuf), "%c", cch);
            }
            sprintf_s(&pCharBuf[strlen(pCharBuf)], sizeof(pCharBuf)-strlen(pCharBuf), "%u", n);
        }
    }
    sprintf_s(&pCharBuf[strlen(pCharBuf)], sizeof(pCharBuf) - strlen(pCharBuf), "]");
    mainlog_wr("Node %03u - 'Repeaters' %u NBs %s", bCurrentNode, j, pCharBuf);
    /* Now dump the registered GW centric "repeater" neighbors for node */
    j = 0;
    memset(pCharBuf, 0, sizeof(pCharBuf));
    sprintf_s(pCharBuf, sizeof(pCharBuf), "[");
    for (int n = 0; n < spNodeStat->neighborCount; n++)
    {
        if (j++ != 0)
        {
            sprintf_s(&pCharBuf[strlen(pCharBuf)], sizeof(pCharBuf)-strlen(pCharBuf), "%c", cch);
        }
        sprintf_s(&pCharBuf[strlen(pCharBuf)], sizeof(pCharBuf)-strlen(pCharBuf), "%u", 
                  spNodeStat->abNeighbors[n]);
    }
    sprintf_s(&pCharBuf[strlen(pCharBuf)], sizeof(pCharBuf) - strlen(pCharBuf), "]");
    mainlog_wr("Node %03u - 'GW centric' %u NBs %s", bCurrentNode, j, pCharBuf);
}


/*===================== NetworkManagement_DumpNeighbors ======================
** Function description
**      Dump the registered neighbors for the nodes in the network
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
NetworkManagement_DumpNeighbors()
{
    if (NULL != spNetworkManagement)
    {
        if (0 < spNetworkManagement->bNodesUnderTestSize)
        {
            for (int i = 0; i < spNetworkManagement->bNodesUnderTestSize; i++)
            {
				NetworkManagement_DumpNodeNeighbors(i);
            }
        }
        else
        {
            mainlog_wr("No nodes to dump neighbors on");
        }
    }
    else
    {
        mainlog_wr("NetworkManagement_Init has not been called");
    }
}


/*============== NetworkManagement_Timed_ZW_SendData_Completed ===============
** Function description
**      Callback function for NetworkManagement_Timed_ZW_SendData which is an
** Internal NetworkManagement module wrapper function for Z-Wave API 
** ZW_SendData function call 
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
NetworkManagement_Timed_ZW_SendData_Completed(
	BYTE bTxStatus,
	TX_STATUS_TYPE *psTxInfo)
{
	/* HOST transmit metrics sampling specifics - Only needed if hw do not support transmit latency sampling */
	/* Sample transmit Stop time */
	TimingStop(&sTxTiming);
    if (NULL != spCurrentSample)
    {
        /* Sample time and LWR - Callback function do the following NH calculations according to testMode */
		if (psTxInfo->wTime > 0)
		{
			/* Only use wTime if bigger than ZERO - if ZERO then we assume not supported */
			spCurrentSample->sampleTime = psTxInfo->wTime * 10;
		}
		else
		{
			spCurrentSample->sampleTime = (long)TimingGetElapsedMSec(sTxTiming.elapsedTime);
			psTxInfo->wTime = spCurrentSample->sampleTime / 10;
		}
        spCurrentSample->failed = (TRANSMIT_COMPLETE_OK != bTxStatus);
        spCurrentSample->routeFound = ((NULL != nodeStats) && (api.ZW_GetLastWorkingRoute(nodeStats->bDestNodeID, spCurrentSample->pRoute) == TRUE));
    }
	else
	{
		if (psTxInfo->wTime == 0)
		{
			psTxInfo->wTime = (long)TimingGetElapsedMSec(sTxTiming.elapsedTime) / 10;
		}
	}
    /* Call the registered callback function with transmission result */
    if (NULL != cbNetworkManagement_Timed_ZW_SendData)
    {
        cbNetworkManagement_Timed_ZW_SendData(bTxStatus, psTxInfo);
    }
}


/*==================== NetworkManagement_Timed_ZW_SendData ===================
** Function description
**      Internal NetworkManagement module wrapper function for Z-Wave API 
** ZW_SendData function call, which does the actual time metric gathering 
** (if bNodeID eligible for transmit metric gathering)
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
BYTE
NetworkManagement_Timed_ZW_SendData(
    BYTE bNodeID,
    BYTE *pData,
    BYTE bDataLength,
    void (__cdecl *completedFunc)(BYTE, TX_STATUS_TYPE *psTxInfo))
{
    BYTE retVal;
    cbNetworkManagement_Timed_ZW_SendData = completedFunc;
    retVal = api.ZW_SendData(bNodeID, pData, bDataLength, testConnectionTxOptions, NetworkManagement_Timed_ZW_SendData_Completed);
    if (0 != retVal)
    {
		/* HOST transmit metrics sampling specifics - Only needed if hw do not support transmit latency sampling */
        TimingStart(&sTxTiming);
    }
    return retVal;
}


/*================= NetworkManagement_ZW_SendData_Completed ==================
** Function description
**      Callback function for NetworkManagement API Z-Wave API ZW_SendData 
** wrapper function. If destination was eligible for transmit metric gathering
** then the destination is marked as needing a NH recalculation.
** A potential Application specified callback function is called with 
** transmit status.
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
NetworkManagement_ZW_SendData_Completed(
    BYTE bTxStatus,
	TX_STATUS_TYPE *psTxInfo)
{
    if (NULL != spCurrentSample)
    {
        /* Now a recalculation of NH is needed */
        nodeStats->fRecalculateNetworkHealth = true;
        /* Release nodeStats and spCurrentSample for next sample */
        spCurrentSample = NULL;
        nodeStats = NULL;
    }
    /* Call the registered callback function with transmission result */
    if (NULL != cbNetworkManagement_ZW_SendData)
    {
        cbNetworkManagement_ZW_SendData(bTxStatus, psTxInfo);
    }
}


/*======================= NetworkManagement_ZW_SendData ======================
** Function description
**      NetworkManagement API wrapper function for Z-Wave API ZW_SendData 
** function. Besides calling ZW_SendData and transmitting to the specified 
** bNodeID it also uses the transmission for transmit metric gathering 
** (if applicable), which then are used to determine the NH for destination 
** bNodeID
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
BYTE
NetworkManagement_ZW_SendData(
    BYTE bNodeID,
    BYTE *pData,
    BYTE bDataLength,
    void (__cdecl *completedFunc)(BYTE, TX_STATUS_TYPE *psTxInfo))
{
    bool retVal;
    bool nodeMetricFound = false;

    if (bNodeID > 0)
    {
        /* Make sure Network Management Network Health has been initialized and test is not allready inprogress */
        if ((NULL != spNetworkManagement) && (0 < spNetworkManagement->bNodesUnderTestSize) && (NULL == nodeStats))
        {
            for (int i = 0; i < spNetworkManagement->bNodesUnderTestSize; i++)
            {
                if (bNodeID == spNetworkManagement->abNodesUnderTest[i])
                {
                    /* Set nodeStat to point to node under test transmit metric structure */
                    nodeStats = &spNetworkManagement->nodeUT[bNodeID - 1];
                    nodeStats->bDestNodeID = bNodeID;
                    /* Set spCurrentSample to point to sample slot to place new sample in - always last sample slot */
                    spCurrentSample = &spNetworkManagement->nodeUT[bNodeID - 1].samples[TESTCOUNT - 1];
                    nodeMetricFound = true;
                    break;
                }
            }
        }
        cbNetworkManagement_ZW_SendData = completedFunc;
        retVal = (0 != NetworkManagement_Timed_ZW_SendData(bNodeID, pData, bDataLength, NetworkManagement_ZW_SendData_Completed));
        /* Only update transmit metric if transmit could be initiated and node metric to update has been found */
        if (retVal && nodeMetricFound)
        {
            if (!nodeStats->samples[0].failed)
            {
                nodeStats->routeCount--;
                if (nodeStats->samples[1].routeUniq)
                {
                    nodeStats->routeCount--;
                }
            }
            /* Make room for new sample - move/copy last (TESTCOUNT - 1) samples one sample slot in sample array */
            memcpy(&nodeStats->samples[0], &nodeStats->samples[1], 
                   sizeof(nodeStats->samples) - sizeof(sSample));
            /* First LWR always uniq if not failed */
            if (!nodeStats->samples[0].failed)
            {
                nodeStats->samples[0].routeUniq = true;
                nodeStats->routeCount++;
            }
        }
    }
    return retVal;
}


/*===================== NetworkManagement_NH_TestConnection =====================
** Function description
**      Function used to do the actual communication test
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
BYTE
NetworkManagement_NH_TestConnection(
    BYTE bNodeID,
    void (*CompletedFunc)(BYTE, TX_STATUS_TYPE *))
{
    return NetworkManagement_Timed_ZW_SendData(bNodeID, testConnectionPayload, sizeof(testConnectionPayload), CompletedFunc);
}


/*=================== NetworkManagement_UpdateTrafficLight ===================
** Function description
**      Update Traffic Light indicator in structure pointed to by spNodeStat
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
NetworkManagement_UpdateTrafficLight(
    sNetworkManagement_Stat *spNodeStats)
{
    if (NULL != spNodeStats)
    {
        /* Set the color of the traffic light */
        if (spNodeStats->NetworkHealthNumber > 7)
        {
            spNodeStats->NetworkHealthSymbol = "GREEN";
        }
        else if (spNodeStats->NetworkHealthNumber > 3)
        {
            spNodeStats->NetworkHealthSymbol = "YELLOW";
        }
        else if (spNodeStats->NetworkHealthNumber > 0)
        {
            spNodeStats->NetworkHealthSymbol = "RED";
        }
        else
        {
            spNodeStats->NetworkHealthSymbol = "FAILED";
        }
    }
}


/*============= NetworkManagement_UpdateNetworkHealthIndicators ==============
** Function description
**      Update Network Health Indicators in structure pointed to by spNodeStat
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
NetworkManagement_UpdateNetworkHealthIndicators(
    sNetworkManagement_Stat *spNodeStats)
{
    if (NULL != spNodeStats)
    {
        if (spNodeStats->testTXErrors == spNodeStats->testCount) // Failed
        {
            spNodeStats->NetworkHealthNumber = 0;
        }
        else if (spNodeStats->testTXErrors > 0) // Red
        {
            spNodeStats->NetworkHealthNumber = 1;
            if (spNodeStats->testTXErrors == 2)
            {
                spNodeStats->NetworkHealthNumber = 2;
            }
            if (spNodeStats->testTXErrors == 1)
            {
                spNodeStats->NetworkHealthNumber = 3;
            }
        }
        else if (spNodeStats->testTXErrors == 0) // Green or Yellow
        {
            if (spNodeStats->routeChange > 1) // Yellow
            {
                if (spNodeStats->routeChange <= 4)
                {
                    spNodeStats->NetworkHealthNumber = 5;
                }
                else if (spNodeStats->routeChange > 4)
                {
                    spNodeStats->NetworkHealthNumber = 4;
                }
            }
            else	// Green
            {
                if ((spNodeStats->routeChange == 0) && 
                    (spNodeStats->neighborCount > 2))
                {
                    spNodeStats->NetworkHealthNumber = 10;
                }
                else if ((spNodeStats->routeChange == 1) &&
                         (spNodeStats->neighborCount > 2))
                {
                    spNodeStats->NetworkHealthNumber = 9;
                }
                else
                {
                    spNodeStats->NetworkHealthNumber = 8;
                }
            }
        }
        NetworkManagement_UpdateTrafficLight(spNodeStats);
    }
}


/*============================ powerStatus2String ============================
** Function description
**		Convert Powerlevel status received from PowerLevelTest module to string
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
char *powerStatus2String(
    ePowLevStatus eStatus)
{
    char *pStatus = "";

    switch(eStatus)
    {
        case POWERLEV_TEST_TRANSMIT_COMPLETE_OK:
            pStatus = "Transmit OK";
            break;

        case POWERLEV_TEST_TRANSMIT_COMPLETE_NO_ACK:
            pStatus = "Transmit NO ACK";
            break;

        case POWERLEV_TEST_TRANSMIT_COMPLETE_FAIL:
            pStatus = "Transmit FAIL";
            break;

        case POWERLEV_TEST_NODE_REPORT_ZW_TEST_FAILED:
            pStatus = "Report Failed";
            break;

        case POWERLEV_TEST_NODE_REPORT_ZW_TEST_SUCCES:
            pStatus = "Report Success";
            break;

        case POWERLEV_TEST_NODE_REPORT_ZW_TEST_INPROGRESS:
            pStatus = "Report Inprogress";
            break;

        case POWERLEV_TEST_NODE_REPORT_ZW_TEST_TIMEOUT:
            pStatus = "Report TIMEOUT";
            break;

        default:
            pStatus = "UNKNOWN CMD";
            break;
    }
    return pStatus;
}


/*================== NetworkManagement_DumpNetworkHealth =====================
** Function description
**      Dump Network Health result in new Network Health filelog
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
NetworkManagement_DumpNetworkHealth()
{
    char pFileName[256];
    const char* timestamp = "Z-Wave Network Health_%u%02u%02u-%02u%02u%02u.txt";
    struct _timeb timebuffer;
    struct tm timeinfo;
    errno_t err;
    err = _ftime64_s(&timebuffer);
    err = _localtime64_s(&timeinfo, &timebuffer.time);

    if (NULL != spNetworkManagement)
    {
        sprintf_s(pFileName, sizeof(pFileName), timestamp,
                  timeinfo.tm_year+1900, timeinfo.tm_mon+1, timeinfo.tm_mday,
                  timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        fopen_s(&fp_statLog, pFileName, "w");
        for (int m = 0; m < spNetworkManagement->bNodesUnderTestSize; m++)
        {
			if ((FULL == spNetworkManagement->bTestMode) || (spNetworkManagement->bNHSCurrentIndex == m))
			{
				BYTE i = spNetworkManagement->abNodesUnderTest[m] - 1;

				fprintf_s(fp_statLog, "\nZ-Wave Network Health for Node %03u\n", spNetworkManagement->nodeUT[i].bDestNodeID);
				/* TO#3934 fix */
				fprintf_s(fp_statLog, "Neighbor count %03u (%3.1f procent) out of %03u\n", 
						  spNetworkManagement->nodeUT[i].neighborCount, 
						  (spNetworkManagement->bNetworkRepeaterCount != 0) ? 
						   ((double) spNetworkManagement->nodeUT[i].neighborCount / (double) (spNetworkManagement->bNetworkRepeaterCount + (spNetworkManagement->nodeUT[i].bControllerIsANeighborAndNotARepeater ? 1 : 0))) * 100 : 
						   (double) 0, spNetworkManagement->bNetworkRepeaterCount + (spNetworkManagement->nodeUT[i].bControllerIsANeighborAndNotARepeater ? 1 : 0));
				char pCharBuf[1024];
				memset(pCharBuf, 0, sizeof(pCharBuf));
				if (spNetworkManagement->nodeUT[i].neighborCount > 0)
				{
					sprintf_s(pCharBuf, sizeof(pCharBuf), "Neighbors ");
					sprintf_s(&pCharBuf[strlen(pCharBuf)], sizeof(pCharBuf)-strlen(pCharBuf), "[");
					char cch = ',';
					for (int n = 0; n < spNetworkManagement->nodeUT[i].neighborCount; n++)
					{
						if (n != 0)
						{
							sprintf_s(&pCharBuf[strlen(pCharBuf)], sizeof(pCharBuf)-strlen(pCharBuf), "%c", cch);
						}
						sprintf_s(&pCharBuf[strlen(pCharBuf)], sizeof(pCharBuf)-strlen(pCharBuf), "%u",
								  spNetworkManagement->nodeUT[i].abNeighbors[n]);
					}
					sprintf_s(&pCharBuf[strlen(pCharBuf)], sizeof(pCharBuf) - strlen(pCharBuf), "]");
					fprintf_s(fp_statLog, "%s\n", pCharBuf);
				}
				else
				{
					fprintf_s(fp_statLog, "NO NEIGHBORS\n");
				}
				for (int j = 0; j < spNetworkManagement->nodeUT[i].testCount; j++)
				{
					fprintf_s(fp_statLog, "%li %s ", spNetworkManagement->nodeUT[i].samples[j].sampleTime,
							  spNetworkManagement->nodeUT[i].samples[j].failed ? "#failed" : "");
					fprintf_s(fp_statLog, "LWR %s [%02X, %02X, %02X, %02X at %s] ", 
							  (spNetworkManagement->nodeUT[i].samples[j].routeFound == 0) ? "NONE" : "FOUND", 
							  spNetworkManagement->nodeUT[i].samples[j].pRoute[ROUTECACHE_LINE_REPEATER_1_INDEX], 
							  spNetworkManagement->nodeUT[i].samples[j].pRoute[ROUTECACHE_LINE_REPEATER_2_INDEX], 
							  spNetworkManagement->nodeUT[i].samples[j].pRoute[ROUTECACHE_LINE_REPEATER_4_INDEX], 
							  spNetworkManagement->nodeUT[i].samples[j].pRoute[ROUTECACHE_LINE_REPEATER_4_INDEX],
							  (spNetworkManagement->nodeUT[i].samples[j].pRoute[ROUTECACHE_LINE_CONF_INDEX] == ZW_LAST_WORKING_ROUTE_SPEED_100K) ? "100k" :
							   (spNetworkManagement->nodeUT[i].samples[j].pRoute[ROUTECACHE_LINE_CONF_INDEX] == ZW_LAST_WORKING_ROUTE_SPEED_40K) ? "40k" :
							    (spNetworkManagement->nodeUT[i].samples[j].pRoute[ROUTECACHE_LINE_CONF_INDEX] == ZW_LAST_WORKING_ROUTE_SPEED_9600) ? "9.6k" : "Auto");
					fprintf_s(fp_statLog, "Route quality - %s, uniq LWR %s\n",
										  spNetworkManagement->nodeUT[i].samples[j].routeChanged ? "Change" : "Stable",
										  spNetworkManagement->nodeUT[i].samples[j].routeUniq ? "TRUE" : "FALSE");
				}
				for (int j = 0; j < 5; j++)
				{
					if (spNetworkManagement->nodeUT[i].powStatus[j].sourceNode != 0)
					{
						char* pStatus = "";
						POWLEV_STATUS powStatus = spNetworkManagement->nodeUT[i].powStatus[j];

						pStatus = powerStatus2String(powStatus.Status);
						fprintf_s(fp_statLog, "PowerLevel %03d-%03d, POW %u, tx %02u[%02u], %s\n", 
								  powStatus.sourceNode,
								  powStatus.destNode,
								  powStatus.PowerLevel,
								  powStatus.ackCountSuccess, 
								  powStatus.NbrTestFrames,
								  pStatus);
					}
				}
				fprintf_s(fp_statLog, "%u tries to reach DUT %u times [PER %u] [RC %u] Uniq LWR %u\n", 
						  spNetworkManagement->nodeUT[i].testCount,
						  spNetworkManagement->nodeUT[i].testCount - spNetworkManagement->nodeUT[i].testTXErrors, 
						  spNetworkManagement->nodeUT[i].testTXErrors,
						  spNetworkManagement->nodeUT[i].routeChange, spNetworkManagement->nodeUT[i].routeCount);
				fprintf_s(fp_statLog, "Average %.0f ms\n", spNetworkManagement->nodeUT[i].average);
				fprintf_s(fp_statLog, "Minimum transmit time           %li ms\n", spNetworkManagement->nodeUT[i].min);
				fprintf_s(fp_statLog, "Maximum transmit time           %li ms\n", spNetworkManagement->nodeUT[i].max);
				fprintf_s(fp_statLog, "NH  = %02i [%02i(%02i),%02i(%02i),%02i(%02i),%02i(%02i),%02i(%02i),%02i(%02i)]\n", 
						  spNetworkManagement->nodeUT[i].NetworkHealthNumber,
						  spNetworkManagement->nodeUT[i].aNetworkHealthNumber[0], spNetworkManagement->nodeUT[i].aNetworkHealthNumberPriorToPowerLevelTest[0],
						  spNetworkManagement->nodeUT[i].aNetworkHealthNumber[1], spNetworkManagement->nodeUT[i].aNetworkHealthNumberPriorToPowerLevelTest[1],
						  spNetworkManagement->nodeUT[i].aNetworkHealthNumber[2], spNetworkManagement->nodeUT[i].aNetworkHealthNumberPriorToPowerLevelTest[2],
						  spNetworkManagement->nodeUT[i].aNetworkHealthNumber[3], spNetworkManagement->nodeUT[i].aNetworkHealthNumberPriorToPowerLevelTest[3],
						  spNetworkManagement->nodeUT[i].aNetworkHealthNumber[4], spNetworkManagement->nodeUT[i].aNetworkHealthNumberPriorToPowerLevelTest[4],
						  spNetworkManagement->nodeUT[i].aNetworkHealthNumber[5], spNetworkManagement->nodeUT[i].aNetworkHealthNumberPriorToPowerLevelTest[5]);
				fprintf_s(fp_statLog, "NHS = %s\n",
						  spNetworkManagement->nodeUT[i].NetworkHealthSymbol);
			}
        }
        fclose(fp_statLog);
    }
}


/*==================== NetworkManagement_NH_TestComplete =====================
** Function description
**      Callback function called when a NH test has been executed for a node.
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
NetworkManagement_NH_TestComplete(
    BYTE bStatus)
{
    if (NULL != spNetworkManagement)
    {
        if (spNetworkManagement->fTestStarted && (FULL == spNetworkManagement->bTestMode) &&
            (spNetworkManagement->bNHSCurrentIndex < spNetworkManagement->bNodesUnderTestSize))
        {
            StartNextNetworkHealthTest();
        }
        else
        {
            NetworkManagement_DumpNetworkHealth();
            spNetworkManagement->bNHSCurrentIndex = 0;
            spNetworkManagement->fTestStarted = false;
            nodeStats = NULL;
            mainlog_wr("Done with Z-Wave Network Health");
            if (spNetworkManagement->CurrentTestCompleted)
            {
                spNetworkManagement->CurrentTestCompleted(1);
            }
        }
    }
}


/*============================ LogNodeNetworkHealth ==========================
** Function description
**      Logs the Network Health result for the specified node
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
LogNodeNetworkHealth(
    sNetworkManagement_Stat *spNodeStats,
	bool fLogAllNHs)
{
    if ((NULL != spNetworkManagement) && (NULL != spNodeStats))
    {
        printf("\n");
        mainlog_wr("Z-Wave Network Health for nodeID %03u completed", spNodeStats->bDestNodeID); 
        mainlog_wr("%u tries to reach DUT %u times, PER %u. RC %u, Uniq LWR %u", 
                   spNodeStats->testCount,
                   spNodeStats->testCount - spNodeStats->testTXErrors, spNodeStats->testTXErrors,
                   spNodeStats->routeChange, spNodeStats->routeCount);
		/* TO#3934 fix */
        mainlog_wr("Neighbor count %03u (%3.1f procent) out of %03u repeaters",
			       spNodeStats->neighborCount, 
				   (spNetworkManagement->bNetworkRepeaterCount != 0) ? ((double) spNodeStats->neighborCount / (double) (spNetworkManagement->bNetworkRepeaterCount + (spNodeStats->bControllerIsANeighborAndNotARepeater ? 1 : 0))) * 100 : (double) 0,
				   spNetworkManagement->bNetworkRepeaterCount + (spNodeStats->bControllerIsANeighborAndNotARepeater ? 1 : 0));
        /* Debug - Dump to screen and log Repeater Neighbors */
        char pCharBuf[1024];
        if (spNodeStats->neighborCount > 0)
        {
            sprintf_s(pCharBuf, sizeof(pCharBuf), "Neighbors ");
            sprintf_s(&pCharBuf[strlen(pCharBuf)], sizeof(pCharBuf)-strlen(pCharBuf), "[");
            char cch = ',';
            for (int i = 0; i < spNodeStats->neighborCount; i++)
            {
                if (i != 0)
                {
                    sprintf_s(&pCharBuf[strlen(pCharBuf)], sizeof(pCharBuf)-strlen(pCharBuf), "%c", cch);
                }
                sprintf_s(&pCharBuf[strlen(pCharBuf)], sizeof(pCharBuf)-strlen(pCharBuf), "%u", spNodeStats->abNeighbors[i]);
            }
            sprintf_s(&pCharBuf[strlen(pCharBuf)], sizeof(pCharBuf) - strlen(pCharBuf), "]");
            mainlog_wr(pCharBuf);
        }
        else
        {
            mainlog_wr("NO NEIGHBORS");
        }

		mainlog_wr("Average for Successful transmit %.0f ms", spNodeStats->average);
		mainlog_wr("Minimum transmit time           %li ms", spNodeStats->min);
		mainlog_wr("Maximum transmit time           %li ms", spNodeStats->max);
		if (!fLogAllNHs)
		{
			mainlog_wr("NH  = %02i", spNodeStats->NetworkHealthNumber);
		}
		else
		{
			mainlog_wr("NH  = %02i [%02i(%02i),%02i(%02i),%02i(%02i),%02i(%02i),%02i(%02i),%02i(%02i)]", 
				       spNodeStats->NetworkHealthNumber,
					   spNodeStats->aNetworkHealthNumber[0], spNodeStats->aNetworkHealthNumberPriorToPowerLevelTest[0],
					   spNodeStats->aNetworkHealthNumber[1], spNodeStats->aNetworkHealthNumberPriorToPowerLevelTest[1],
					   spNodeStats->aNetworkHealthNumber[2], spNodeStats->aNetworkHealthNumberPriorToPowerLevelTest[2],
					   spNodeStats->aNetworkHealthNumber[3], spNodeStats->aNetworkHealthNumberPriorToPowerLevelTest[3],
					   spNodeStats->aNetworkHealthNumber[4], spNodeStats->aNetworkHealthNumberPriorToPowerLevelTest[4],
					   spNodeStats->aNetworkHealthNumber[5], spNodeStats->aNetworkHealthNumberPriorToPowerLevelTest[5]);
		}
		mainlog_wr("NHS = %s\n", spNodeStats->NetworkHealthSymbol);
    }
}


/*================= LogCurrentAndInitiateNextNetworkHealthTest ===============
** Function description
**      Logs the Network Health result for current node and initiates next
** Network Health test for next node.
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
NextNodeNetworkHealthTest()
{
    if (NULL != nodeStats)
    {
        LogNodeNetworkHealth(nodeStats, true);
        NetworkManagement_NH_TestComplete(TRUE);
    }
}


/*========================= logPowerlevelTestResult ==========================
** Function description
**      Log in mainlog the result of specified Powerlevel Test result
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
void 
logPowerlevelTestResult(
    POWLEV_STATUS powStatus)
{
    char* pStatus = "";

    pStatus = powerStatus2String(powStatus.Status);
    mainlog_wr("PowerLevel %03d-%03d, POW %u, tx %02u[%02u], %s", 
                powStatus.sourceNode,
                powStatus.destNode,
                powStatus.PowerLevel,
                powStatus.ackCountSuccess, 
                powStatus.NbrTestFrames,
                pStatus);
}

/*==================== NetworkManagement_NH_PowerLevelTest_Completed =================
** Function description
**      Callback staus function for NetworkManagement_NH_PowerLevelTest(...)
**		
** Side effects:
**
**--------------------------------------------------------------------------*/
void
NetworkManagement_NH_PowerLevelTest_Completed(
    POWLEV_STATUS powStatus)
{
    BYTE firstNode = 0;
    BYTE secondNode = 0;

    if ((NULL != spNetworkManagement) && (NULL != nodeStats))
    {
        if (MAX_ROUTE_LINKS > bLWRRouteIndex)
        {
			memcpy(&nodeStats->powStatus[bLWRRouteIndex], &powStatus, sizeof(POWLEV_STATUS));
		}

        logPowerlevelTestResult(powStatus);

        if (powStatus.Status == POWERLEV_TEST_NODE_REPORT_ZW_TEST_SUCCES)
        {
            if (MAX_ROUTE_LINKS > bLWRRouteIndex)
            {
                nodeStats->bPowerTestSuccess[bLWRRouteIndex] = powStatus.ackCountSuccess;
            }
            if (4 > bLWRRouteIndex)
            {
                if (0 < pabLWRRoute[bLWRRouteIndex])
                {
                    if (4 > ++bLWRRouteIndex)
                    {
                        if (0 < pabLWRRoute[bLWRRouteIndex])
                        {
                            if (pafLWRNodesSupportPowerLevel[bLWRRouteIndex])
                            {
                                /* Previous Repeater - Source of Powerlevel Test  */
                                firstNode = pabLWRRoute[bLWRRouteIndex - 1];
                                /* Current Repeater - Destination of Powerlevel Test */
                                secondNode = pabLWRRoute[bLWRRouteIndex];
                            }
                            else
                            {
                                /* Current Repeater - Source of Powerlevel Test */
                                firstNode = pabLWRRoute[bLWRRouteIndex];
                                /* Previous Repeater - Destination of Powerlevel Test  */
                                secondNode = pabLWRRoute[bLWRRouteIndex - 1];
                            }
                        }
                    }
                    if (0 == firstNode)
                    {
                        if (pafLWRNodesSupportPowerLevel[bLWRRouteIndex + 1])
                        {
                            /* Destination - Source of Powerlevel Test */
                            firstNode = nodeStats->bDestNodeID;
                            /* Last Repeater */
                            secondNode = pabLWRRoute[bLWRRouteIndex - 1];
                        }
                        else
                        {
                            /* Last Repeater - Source of Powerlevel Test */
                            firstNode = pabLWRRoute[bLWRRouteIndex - 1];
                            /* Destination */
                            secondNode = nodeStats->bDestNodeID;
                        }
                    }
                }
            }
        }
        if (0 < firstNode)
        {
            PowerLevelTest_Init(spNetworkManagement->bControllerNodeID, firstNode, secondNode, POWERLEVELTEST_LEVEL, POWERLEVELTEST_COUNT, false);
            PowerLevelTest_Set(&NetworkManagement_NH_PowerLevelTest_Completed);
        }
        else
        {
            int i = 0;
            bool fPowerLevelTestSuccess = true;
            while ((i < MAX_ROUTE_LINKS) && ((i == 0) || (0 < pabLWRRoute[i - 1])))
            {
                if (nodeStats->bPowerTestSuccess[i] != powStatus.NbrTestFrames)
                {
                    fPowerLevelTestSuccess = false;
                    break;
                }
                i++;
            }
            /* If PowerLevel success then NetworkHealthNumber staýs at the current level */
            if (!fPowerLevelTestSuccess)
            {
                /* Failed PowerLevel Test - Degrade */
                if (9 <= nodeStats->NetworkHealthNumber)
                {
                    /* Was a 9 or 10 and due to failed PowerLeveltest NH is degraded to 7 */
                    nodeStats->NetworkHealthNumber = 7;
                }
                else if (8 == nodeStats->NetworkHealthNumber)
                {
                    /* Was an 8 and due to failed PowerLeveltest NH is degraded to 6 */
                    nodeStats->NetworkHealthNumber = 6;
                }
                NetworkManagement_UpdateTrafficLight(nodeStats);
            }
			/* RSSI test is only necessary for NH values of 8-10 */
			if (nodeStats->NetworkHealthNumber >= 8)
			{
				PerformRSSITest();
			}
			else
			{
				NextNetworkHealthTestRound();
			}

        }
    }
}


/*================== NetworkManagement_NH_PowerLevelTest =====================
** Function description
**      If all nodes in LWR supports the COMMAND_CLASS_POWER_LEVEL then all
** links are tested for link margin with the Powerlevel Command Class
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
NetworkManagement_NH_PowerLevelTest(
    BYTE* spRoute)
{
    if ((NULL != spNetworkManagement) && (NULL != nodeStats) && (NULL != spRoute))
    {
        /* */
        pabLWRRoute = spRoute;
        bLWRRouteIndex = 0;
        PowerLevelTest_CPowerLevelTest(&api, TRUE);
        /* Do more than Source and Destination exist in LWR */
        if (2 < iLWRNumberOfNodesInLink)
        {
            /* Repeaters in LWR */
            /* ControllerNodeID/GW to first Repeater Powerlevel Test */
            if (pafLWRNodesSupportPowerLevel[0])
            {
                /* Controller supports PowerLevel -> use Controller as Source in the PowerLevel Test */
                PowerLevelTest_Init(spNetworkManagement->bControllerNodeID, spNetworkManagement->bControllerNodeID, pabLWRRoute[bLWRRouteIndex], POWERLEVELTEST_LEVEL, POWERLEVELTEST_COUNT, false);
            }
            else
            {
                /* Controller do not support PowerLevel -> use first Repeater as Source in the PowerLevel Test */
                PowerLevelTest_Init(spNetworkManagement->bControllerNodeID, pabLWRRoute[bLWRRouteIndex], spNetworkManagement->bControllerNodeID, POWERLEVELTEST_LEVEL, POWERLEVELTEST_COUNT, false);
            }
        }
        else
        {
            /* Direct route */
            if (pafLWRNodesSupportPowerLevel[0] && !spNetworkManagement->afIsNodeAFLiRS[nodeStats->bDestNodeID - 1])
            {
                /* Controller supports PowerLevel -> use Controller as Source in the PowerLevel Test */
                PowerLevelTest_Init(spNetworkManagement->bControllerNodeID, spNetworkManagement->bControllerNodeID, nodeStats->bDestNodeID, POWERLEVELTEST_LEVEL, POWERLEVELTEST_COUNT, false);
            }
            else
            {
                /* Controller do not support PowerLevel -> use Destination as Source in the PowerLevel Test */
                PowerLevelTest_Init(spNetworkManagement->bControllerNodeID, nodeStats->bDestNodeID, spNetworkManagement->bControllerNodeID, POWERLEVELTEST_LEVEL, POWERLEVELTEST_COUNT, false);
            }
        }
        PowerLevelTest_Set(&NetworkManagement_NH_PowerLevelTest_Completed);
    }
}


/*======================== IsCommandClassSupported ===========================
** Function description
**      Determines if bNodeID supports the bCommandClass according to the
** registered nodeDescriptor in spNetworkManagement.
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
bool
IsCommandClassSupported(
    BYTE bNodeID,
    BYTE bCommandClassID)
{
    BYTE bIndex;
    bool fSupported = false;

    if (NULL != spNetworkManagement)
    {
        if ((bNodeID > 0) && (bNodeID <= ZW_MAX_NODES))
        {
            bIndex = bNodeID - 1;
            for (int n = 0; n < sizeof(spNetworkManagement->nodeDescriptor[bIndex].cmdClasses); n++)
            {
                if (0 == spNetworkManagement->nodeDescriptor[bIndex].cmdClasses[n])
                {
                    /* No more COMMAND CLASSES supported by bNodeID */
                    break;
                }
                if (bCommandClassID == spNetworkManagement->nodeDescriptor[bIndex].cmdClasses[n])
                {
                    fSupported = true;
                    break;
                }
            }
        }
    }
    return fSupported;
}


/*================ NetworkManagement_NH_Calculate_MinMaxAverage =================
** Function description
**      Updates average, min and max for node transmit metrics specified with 
** specified transmit sample
**		
** Side effects:
**
**-----------------------------------------------------------------------------*/
void
NetworkManagement_NH_Calculate_MinMaxAverage(
    sNetworkManagement_Stat *spNodeStats, int sampleNo)
{
    if ((NULL != spNodeStats) && (TESTCOUNT > sampleNo))
    {
        if (!spNodeStats->samples[sampleNo].failed)
        {
            /* Success */
            if (spNodeStats->min > spNodeStats->samples[sampleNo].sampleTime)
            {
                spNodeStats->min = spNodeStats->samples[sampleNo].sampleTime;
            }
            if (spNodeStats->max < spNodeStats->samples[sampleNo].sampleTime)
            {
                spNodeStats->max = spNodeStats->samples[sampleNo].sampleTime;
            }
        }
        else
        {
            /* Failed - Increment PER */
            spNodeStats->testTXErrors++;
        }
        spNodeStats->average = (double)(spNodeStats->samples[sampleNo].sampleTime + sampleNo * spNodeStats->average) / (sampleNo + 1);
    }
}


/*==================== NetworkManagement_NH_Calculate_StdDev ====================
** Function description
**      Calculates sample standard deviation on the current node transmit metrics 
**		
** Side effects:
**
**-----------------------------------------------------------------------------*/
void
NetworkManagement_NH_Calculate_StdDev(
    sNetworkManagement_Stat *spNodeStats)
{
    double sum = 0;
	double hwSum = 0;

    if (NULL != spNodeStats)
    {
        for (int i = 0; i < spNodeStats->testCount; i++)
        {
            sum += pow((double)spNodeStats->samples[i].sampleTime - spNodeStats->average, 2);
        }
        spNodeStats->sampleStdDev = sqrt(((double)1/(spNodeStats->testCount - 1)) * sum);
    }
}


/*========================== RouteChangeByLatencyJitter =========================
** Function description
**      Checks if testRouteDurationMAX has been exceeded when comparing specified
** sampleNo and sampleNo - 1 transmit latencies, if so the sample is classified
** as a RC and the function returns true. 
**		
** Side effects:
**
**-----------------------------------------------------------------------------*/
bool
RouteChangeByLatencyJitter(
	sNetworkManagement_Stat *spNodeStats, int sampleNo)
{
	return ((spNodeStats->samples[sampleNo].sampleTime > spNodeStats->samples[sampleNo - 1].sampleTime) && 
            (spNodeStats->testRouteDurationMAX < spNodeStats->samples[sampleNo].sampleTime - spNodeStats->samples[sampleNo - 1].sampleTime));
}


/*================== NetworkManagement_NH_Calculate_RouteChange =================
** Function description
**      Traverses through the current node transmit metrics and determines if any
** route changes has occured either through long transmit time or LWR change
**		
** Side effects:
**
**-----------------------------------------------------------------------------*/
void
NetworkManagement_NH_Calculate_RouteChange(
    sNetworkManagement_Stat *spNodeStats, int sampleNo, bool doLog)
{
    if ((NULL != spNodeStats) && (TESTCOUNT > sampleNo))
    {
        /* Route Change detection - First Test Sample no route change can be detected */
        if (0 < sampleNo)
        {
            bool fEqual = false;
            for (int i = 0; i < sampleNo; i++)
            {
                if ((spNodeStats->samples[sampleNo].failed) ||
                    (memcmp(spNodeStats->samples[i].pRoute, 
                            spNodeStats->samples[sampleNo].pRoute, 
                            sizeof(spNodeStats->samples[i].pRoute)) == 0))
                {
                    /* Either a matching LWR has been found or current LWR failed */
                    /* A failed transmit have no LWR, so it cannot be uniq... */
                    fEqual = true;
                    break;
                }
            }
            if (false == fEqual)
            {
                spNodeStats->samples[sampleNo].routeUniq = true;
                spNodeStats->routeCount++;
            }
            else
            {
                spNodeStats->samples[sampleNo].routeUniq = false;
            }
            /* If either testRouteDurationMAX Route Change detection time has been crossed or */
            /* a change in the LWR has been detected then the transmission will be marked as having */
            /* a "Route Change" occurence */
            spNodeStats->samples[sampleNo].routeChanged = false;
            if ((memcmp(spNodeStats->samples[sampleNo - 1].pRoute, 
                        spNodeStats->samples[sampleNo].pRoute, 
                        sizeof(spNodeStats->samples[sampleNo - 1].pRoute)) != 0) ||
				RouteChangeByLatencyJitter(spNodeStats, sampleNo))
            {
                /* Test Transmit classified as a "Route Change" occured */
                spNodeStats->routeChange++;
                spNodeStats->samples[sampleNo].routeChanged = true;
                /* */
                doLog = true;
            }
            if (doLog)
            {
                mainlog_wr("Route quality - %s (%li), uniq LWR %s",
                           spNodeStats->samples[sampleNo].routeChanged ? "Change" : "Stable", 
                           spNodeStats->samples[sampleNo].sampleTime - spNodeStats->samples[sampleNo - 1].sampleTime,
                           spNodeStats->samples[sampleNo].routeUniq ? "TRUE" : "FALSE");
            }
        }
        else
        {
            if (doLog)
            {
                spNodeStats->samples[0].routeUniq = true;
                spNodeStats->routeCount = 1;
                mainlog_wr("Route quality - %s (%li), uniq LWR TRUE",
                           spNodeStats->samples[sampleNo].routeChanged ? "Change" : "Stable", 0);
            }
        }
        if (doLog)
        {
			mainlog_wr("Node %03u test %03u %s... Latency %li ms", spNodeStats->bDestNodeID, sampleNo, !spNodeStats->samples[sampleNo].failed ? "Success" : "Failed", 
				       spNodeStats->samples[sampleNo].sampleTime);
            mainlog_wr("Node %03u LWR - %s [0x%02X, 0x%02X, 0x%02X, 0x%02X at %s]", spNodeStats->bDestNodeID, 
				       (spNodeStats->samples[sampleNo].routeFound == 0) ? "NONE" : "FOUND", 
					   spNodeStats->samples[sampleNo].pRoute[ROUTECACHE_LINE_REPEATER_1_INDEX], 
					   spNodeStats->samples[sampleNo].pRoute[ROUTECACHE_LINE_REPEATER_2_INDEX], 
					   spNodeStats->samples[sampleNo].pRoute[ROUTECACHE_LINE_REPEATER_4_INDEX], 
					   spNodeStats->samples[sampleNo].pRoute[ROUTECACHE_LINE_REPEATER_4_INDEX],
					   (spNodeStats->samples[sampleNo].pRoute[ROUTECACHE_LINE_CONF_INDEX] == ZW_LAST_WORKING_ROUTE_SPEED_100K) ? "100k" :
					    (spNodeStats->samples[sampleNo].pRoute[ROUTECACHE_LINE_CONF_INDEX] == ZW_LAST_WORKING_ROUTE_SPEED_40K) ? "40k" :
					     (spNodeStats->samples[sampleNo].pRoute[ROUTECACHE_LINE_CONF_INDEX] == ZW_LAST_WORKING_ROUTE_SPEED_9600) ? "9.6k" : "Auto");
        }
    }
    else if (TESTCOUNT > sampleNo)
    {
        mainlog_wr("Node %03u test %03u NOT valid - Only test 000..%03u", spNodeStats->bDestNodeID, sampleNo, TESTCOUNT - 1);
    }
}


/*========================== MaintenanceState2String =========================
** Function description
**		Converts specified Maintenance State to a string
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
char 
*MaintenanceState2String(
    eMAINTENANCESTATE eMaintenanceState)
{
    char *pStateStr = "";

    switch(eMaintenanceState)
    {
        case MAINTENANCE_STATE_IDLE:
            pStateStr = "IDLE";
            break;

        case MAINTENANCE_STATE_SENDTEST:
            pStateStr = "MAINTENANCE_STATE_SENDTEST";
            break;

        case MAINTENANCE_STATE_UPDATEPERRC:
            pStateStr = "MAINTENANCE_STATE_UPDATEPERRC";
            break;

		case MAINTENANCE_STATE_CHECKPERRC:
			pStateStr = "MAINTENANCE_STATE_CHECKPERRC";
			break;

        case MAINTENANCE_STATE_NH_RUN_NODE_N_NOTFAILED:
            pStateStr = "MAINTENANCE_STATE_NH_RUN_NODE_N_NOTFAILED";
            break;

		case MAINTENANCE_STATE_NH_CHECK_NOT_FAILED:
			pStateStr = "MAINTENANCE_STATE_NH_CHECK_NOT_FAILED";
			break;

        case MAINTENANCE_STATE_FLAG_NODE_REDISCOV:
            pStateStr = "MAINTENANCE_STATE_FLAG_NODE_REDISCOV";
            break;

		case MAINTENANCE_STATE_USER_CHECK:
			pStateStr = "MAINTENANCE_STATE_USER_CHECK";
			break;

        case MAINTENANCE_STATE_USER_CHECK_DONE:
            pStateStr = "MAINTENANCE_STATE_USER_CHECK_DONE";
            break;

		case MAINTENANCE_STATE_NH_RUN_NODE_N_FAILED:
			pStateStr = "MAINTENANCE_STATE_NH_RUN_NODE_N_FAILED";
			break;

        case MAINTENANCE_STATE_NH_CHECK_FAILED:
            pStateStr = "MAINTENANCE_STATE_NH_CHECK_FAILED";
            break;

		case MAINTENANCE_STATE_USER_FULL_REDISCOV_DO:
			pStateStr = "MAINTENANCE_STATE_USER_FULL_REDISCOV_DO";
			break;

        case MAINTENANCE_STATE_USER_FULL_REDISCOV_DONE:
            pStateStr = "MAINTENANCE_STATE_USER_FULL_REDISCOV_DONE";
            break;

		case MAINTENANCE_STATE_CLEAR_ALL_RCLIFETIME:
			pStateStr = "MAINTENANCE_STATE_CLEAR_ALL_RCLIFETIME";
			break;

        case MAINTENANCE_STATE_CALLCENTER_NOTIFY:
            pStateStr = "MAINTENANCE_STATE_CALLCENTER_NOTIFY";
            break;

		case MAINTENANCE_STATE_UNDEFINED:
			pStateStr = "MAINTENANCE_STATE_UNDEFINED";
			break;

        default:
            pStateStr = "UNKNOWN MAINTENANCE STATE";
            break;
    }
    return pStateStr;
}
    

/*=========================== GetNodeMaintenanceState ===========================
** Function description
**      Get Maintenance state for specified node
**		
** Side effects:
**-----------------------------------------------------------------------------*/
eMAINTENANCESTATE
GetNodeMaintenanceState(
    sNetworkManagement_Stat *spNodeStats)
{
    return spNodeStats->sNodeState.eMaintenanceState;
}


/*=========================== SetNodeMaintenanceState ===========================
** Function description
**      Set Maintenance state for specified node according to specified state
**		
** Side effects:
**		Update last MAINTENANCESTATE_BACKLOGSIZE Maintenance States FIFO 
**-----------------------------------------------------------------------------*/
void
SetNodeMaintenanceState(
    sNetworkManagement_Stat *spNodeStats,
    eMAINTENANCESTATE eNodeState)
{
    /* The last MAINTENANCESTATE_BACKLOGSIZE Maintenance States are saved in a FIFO buffer with element ZERO oldest */
    for (int i = 1; i < MAINTENANCESTATE_BACKLOGSIZE; i++)
    {
        spNodeStats->sNodeState.aeMaintenanceState[i - 1] = spNodeStats->sNodeState.aeMaintenanceState[i];
    }
    /* Save previous Maintenance State at element MAINTENANCESTATE_BACKLOGSIZE - 1 */
    spNodeStats->sNodeState.aeMaintenanceState[MAINTENANCESTATE_BACKLOGSIZE - 1] = spNodeStats->sNodeState.eMaintenanceState;
    spNodeStats->sNodeState.eMaintenanceState = eNodeState;
    if (eNodeState < MAINTENANCE_STATE_UNDEFINED)
    {
        if (spNodeStats->sNodeState.aeMaintenanceState[MAINTENANCESTATE_BACKLOGSIZE - 1] != eNodeState)
        {
            /* Increment number of transistions to this state */
            spNodeStats->sNodeState.aiTransistionCount[eNodeState]++;
        }
        mlog.Write(networkHealthlog, "Network Health Maintenance State: Node %03u, %s (%u)", spNodeStats->bDestNodeID,
                   MaintenanceState2String(eNodeState), spNodeStats->sNodeState.aiTransistionCount[eNodeState]);
    }
    else
    {
        mlog.Write(networkHealthlog, "Network Health Maintenance State: Node %03u, %s", spNodeStats->bDestNodeID, 
                   MaintenanceState2String(eNodeState));
    }
}


/*=================== NetworkManagement_Maintenance_StateUpdate =================
** Function description
**      Update Maintenance state for node according to specified transmit metrics
**		
** Side effects:
**		
**-----------------------------------------------------------------------------*/
void
NetworkManagement_Maintenance_StateUpdate(
    sNetworkManagement_Stat *spNodeStats)
{
    if (NULL != spNodeStats)
    {
        if ((((10 > spNodeStats->testTXErrors) && (0 < spNodeStats->testTXErrors)) || 
             ((spNodeStats->routeChangeLifetime) && (spNodeStats->routeChangeLifetime + 2 > spNodeStats->neighborCount))) && 
            (!spNetworkManagement->afMaintenanceNeedsRediscovery[spNodeStats->bDestNodeID - 1]))
        {
            SetNodeMaintenanceState(spNodeStats, MAINTENANCE_STATE_NH_RUN_NODE_N_NOTFAILED);
            if (!spNetworkManagement->afMaintenanceNeedsNetworkHealthTest[spNodeStats->bDestNodeID - 1])
            {
                mainlog_wr("NMSNF - Node %03u flaged in 'needing Network Health Test' list", spNodeStats->bDestNodeID);
				/* TO#3936 fix */
                if ((spNodeStats->routeChangeLifetime) && (spNodeStats->routeChangeLifetime + 2 > spNodeStats->neighborCount))
                {
                    mainlog_wr("NMSNF - Node %03u flaged reason: RCLifetime && (RCLiftime %u+2 > %u NB)", spNodeStats->bDestNodeID, spNodeStats->routeChangeLifetime, spNodeStats->neighborCount);
                }
                else
                {
                    mainlog_wr("NMSNF - Node %03u flaged reason: PER %u inside '10 > PER > 0'", spNodeStats->bDestNodeID, spNodeStats->testTXErrors);
                }
            }
			/* TO#3941 */
			if (spNodeStats->routeChangeLifetime)
			{
				spNodeStats->routeChangeLifetime--;
			}
            /* Node needs an Network Health Update */
            spNetworkManagement->afMaintenanceNeedsNetworkHealthTest[spNodeStats->bDestNodeID - 1] = true;
        }
        else if ((10 == spNodeStats->testTXErrors) && (!spNetworkManagement->afMaintenanceNeedsRediscovery[spNodeStats->bDestNodeID - 1]))
        {
            if (MAINTENANCE == spNetworkManagement->bTestMode)
            {
                SetNodeMaintenanceState(spNodeStats, MAINTENANCE_STATE_USER_CHECK);
                /* Only do this if in maintenance mode */
                mainlog_wr("** ALERT ** NODE %03u NEEDS POWER and MANUAL OPERATION CHECK", spNodeStats->bDestNodeID);
                mainlog_wr("Press key when POWER and MANUAL OPERATION CHECK has been executed");
                _getch();
                SetNodeMaintenanceState(spNodeStats, MAINTENANCE_STATE_USER_CHECK_DONE);
            }
            if (!spNetworkManagement->afMaintenanceNeedsNetworkHealthTest[spNodeStats->bDestNodeID - 1])
            {
                mainlog_wr("NMSF - Node %03u flaged in 'needing Network Health Test' list ", spNodeStats->bDestNodeID);
            }
            SetNodeMaintenanceState(spNodeStats, MAINTENANCE_STATE_NH_RUN_NODE_N_FAILED);
            /* Node needs an Network Health Update */
            spNetworkManagement->afMaintenanceNeedsNetworkHealthTest[spNodeStats->bDestNodeID - 1] = true;
        }
    }
}


/*================= NetworkManagement_NH_Maintenance_Recalculate ===============
** Function description
**      Recalculates NH on current node transmit metrics
**		
** Side effects:
**		
**-----------------------------------------------------------------------------*/
void
NetworkManagement_NH_Maintenance_Recalculate(
    BYTE bNodeID)
{
    if ((NULL != spNetworkManagement) && (0 < bNodeID) && (ZW_MAX_NODES >= bNodeID))
    {
        if (bNodeID == spNetworkManagement->nodeUT[bNodeID - 1].bDestNodeID)
        {
            sNetworkManagement_Stat *spNodeStat = &spNetworkManagement->nodeUT[bNodeID - 1];
            /* Reinitialize */
            spNodeStat->routeCount = 0;
            spNodeStat->routeChange = 0;
            spNodeStat->testTXErrors = 0;
            spNodeStat->average = 0;	
            spNodeStat->max = 0;
            spNodeStat->min = MAXLONG;
            spNodeStat->sampleStdDev = 0;
            /* Traverse all TESTCOUNT samples and recalculate NH */
            for (int i = 0; i < TESTCOUNT; i++)
            {
                NetworkManagement_NH_Calculate_MinMaxAverage(spNodeStat, i);
                NetworkManagement_NH_Calculate_RouteChange(spNodeStat, i, false);
            }
            NetworkManagement_NH_Calculate_StdDev(spNodeStat);
            if (spNodeStat->samples[TESTCOUNT - 1].routeChanged)
            {
                spNodeStat->routeChangeLifetime++;
            }
            for (int i = TESTROUNDS - 1; i > 0; i--)
            {
                spNodeStat->aNetworkHealthNumber[i - 1] = spNodeStat->aNetworkHealthNumber[i];
            }
			/* TO#3935 fix */
			spNodeStat->aNetworkHealthNumber[TESTROUNDS - 1] = spNodeStat->NetworkHealthNumber;
            /* Update Network Health Indicators */
            NetworkManagement_UpdateNetworkHealthIndicators(spNodeStat);
            /* In Maintenance we do not do Powerlevel Tests - just set it anyway */
            spNodeStat->NetworkHealthNumberPriorToPowerLevelTest = spNodeStat->NetworkHealthNumber;
            spNodeStat->fRecalculateNetworkHealth = false;
            NetworkManagement_Maintenance_StateUpdate(spNodeStat);
            if (spNodeStat->routeChangeLifetimeOld != spNodeStat->routeChangeLifetime)
            {
                spNodeStat->routeChangeLifetimeOld = spNodeStat->routeChangeLifetime;
                mainlog_wr("Network Health Maintenance - Route Change in Node %03u, RClifetime %i", spNodeStat->bDestNodeID, spNodeStat->routeChangeLifetime);
            }
			if (spNodeStat->aNetworkHealthNumber[TESTROUNDS - 1] != spNodeStat->NetworkHealthNumber)
            {
                mainlog_wr("Network Health Maintenance - Change detected in Node %03u", spNodeStat->bDestNodeID);
                mainlog_wr("Network Health Maintenance - NH was %u, now %u", 
						   spNodeStat->aNetworkHealthNumber[TESTROUNDS - 1], spNodeStat->NetworkHealthNumber);
                /* If any change in NetworkHealthNumber detected then log it */
                LogNodeNetworkHealth(spNodeStat, false);
            }
        }
    }
}


/*================== NetworkManagement_NH_TestConnectionComplete ================
** Function description
**      Callback called when the NetworkManagement_NH_TestConnection has been executed
** and called by the NetworkManagement Network Health Test functionality if specified.
**		
** Side effects:
**		
**-----------------------------------------------------------------------------*/
void
NetworkManagement_NH_TestConnectionComplete(
    BYTE bTxStatus, 
	TX_STATUS_TYPE *psTxInfo)
{
    if (NULL != nodeStats)
    {
        NetworkManagement_NH_Calculate_MinMaxAverage(nodeStats, nodeStats->testCount);
        NetworkManagement_NH_Calculate_RouteChange(nodeStats, nodeStats->testCount, true);

        if (nodeStats->testCount < TESTCOUNT)
        {
            nodeStats->testCount++;
        }
        if (nodeStats->testCount < TESTCOUNT)
        {
            spCurrentSample = &nodeStats->samples[nodeStats->testCount];
            NetworkManagement_NH_TestConnection((BYTE)nodeStats->bDestNodeID, NetworkManagement_NH_TestConnectionComplete);
        }
        else
        {
            bool doLWRNodesSupportPowerLevel;
            int n;
            int bNumberOfRepeatersFound = 0;

            NetworkManagement_NH_Calculate_StdDev(nodeStats);
            /* Calculate traffic light indication */
            NetworkManagement_UpdateNetworkHealthIndicators(nodeStats);
            /* Do Source/Controller Support POWERLEVEL COMMAND CLASS */
            pafLWRNodesSupportPowerLevel[0] = spNetworkManagement->fControllerSupportPowerLevelTest;
            /* Max 4 repeaters - ZERO based */
            n = 4 - 1;
            /* Find last repeater in LWR */
            /* We use the last sampled Last Working Route */
            while ((n >= 0) && ((0 == nodeStats->samples[TESTCOUNT - 1].pRoute[n])))
            {
                /* No repeater */
                n--;
            }
            if (n >= 0)
            {
                /* LWR repeaters defined */
                bNumberOfRepeatersFound = n + 1;
                /* Repeater found */
                for (; n >= 0; n--)
                {
                    pafLWRNodesSupportPowerLevel[n + 1] = IsCommandClassSupported(nodeStats->samples[TESTCOUNT - 1].pRoute[n], COMMAND_CLASS_POWERLEVEL);
                }
            }
            pafLWRNodesSupportPowerLevel[bNumberOfRepeatersFound + 1] = IsCommandClassSupported(nodeStats->bDestNodeID, COMMAND_CLASS_POWERLEVEL);
            for (n = 0; n < bNumberOfRepeatersFound + 1; n++)
            {
                /* Do any of the nodes in Link support PowerLevel Command Class */
                doLWRNodesSupportPowerLevel = pafLWRNodesSupportPowerLevel[n] || pafLWRNodesSupportPowerLevel[n + 1];
                if (!doLWRNodesSupportPowerLevel)
                {
                    /* Found a link with no PowerLevel Command Class support - No need for testing anymore */
                    break;
                }
            }
			
            if (doLWRNodesSupportPowerLevel)
			{
				/* Make sure if a FLiRS is under test then it is the FLiRS which must execute the */
				/* PowerLevel test on link in question */
				if (true == spNetworkManagement->afIsNodeAFLiRS[nodeStats->bDestNodeID - 1])
				{
					doLWRNodesSupportPowerLevel = pafLWRNodesSupportPowerLevel[bNumberOfRepeatersFound + 1];
				}
			}
            nodeStats->NetworkHealthNumberPriorToPowerLevelTest = nodeStats->NetworkHealthNumber;
            nodeStats->aNetworkHealthNumberPriorToPowerLevelTest[nodeStats->testRounds] = nodeStats->NetworkHealthNumber;
            /* Check if NH = 8, 9 or 10 and if so do a PowerLevel test on all links in the LWR for DUT */
            if (doLWRNodesSupportPowerLevel && (nodeStats->NetworkHealthNumber >= 8))
            {
                /* Number of nodes in LWR including source and destination */
                iLWRNumberOfNodesInLink = bNumberOfRepeatersFound + 1 + 1;
                NetworkManagement_NH_PowerLevelTest(nodeStats->samples[TESTCOUNT - 1].pRoute);
            }
            else
            {
                if (!doLWRNodesSupportPowerLevel)
                {
                    mainlog_wr("PowerLevel Test not executed - One node in every link");
                    mainlog_wr(" in LWR must support PowerLevel Command Class");
                }
                NextNetworkHealthTestRound();
            }
        }
    }
}


/*========================== InitializeTransmitMetrics ==========================
** Function description
**      Initialize transmit metrics specified by nodeStats
**		
** Side effects:
**		
**-----------------------------------------------------------------------------*/
void 
InitializeTransmitMetrics()
{
    if (NULL != nodeStats)
    {
        nodeStats->testCount = 0;
        nodeStats->routeCount = 0;
        nodeStats->routeChange = 0;
        nodeStats->testTXErrors = 0;
        nodeStats->average = 0;	
        nodeStats->max = 0;
        nodeStats->min = MAXLONG;
        nodeStats->sampleStdDev = 0;
		memset(nodeStats->powStatus, 0, sizeof(nodeStats->powStatus));
        memset(nodeStats->samples, 0, sizeof(nodeStats->samples));
    }
}


/*========================== NextNetworkHealthTestRound =========================
** Function description
**      Executes the Network Health Test on every node TESTROUNDS times and 
** calculates the resulting NH as the average of the NH results from the 
** TESTROUNDS Network Health Test runs (10 NOPs + [10 Powerlevel Tests])
**
** Side effects:
**		
**-----------------------------------------------------------------------------*/
void
NextNetworkHealthTestRound()
{
    if (NULL != nodeStats)
    {
        nodeStats->aNetworkHealthNumber[nodeStats->testRounds] = nodeStats->NetworkHealthNumber;
        nodeStats->testRounds++;
        if (TESTROUNDS > nodeStats->testRounds)
        {
            InitializeTransmitMetrics();
            spCurrentSample = &nodeStats->samples[nodeStats->testCount];
            NetworkManagement_NH_TestConnection((BYTE)nodeStats->bDestNodeID, NetworkManagement_NH_TestConnectionComplete);
        }
        else
        {
            nodeStats->NetworkHealthNumber = 0;
            nodeStats->NetworkHealthNumberPriorToPowerLevelTest = 0;
            for (int i = 0; i < TESTROUNDS; i++)
            {
                nodeStats->NetworkHealthNumber += nodeStats->aNetworkHealthNumber[i];
                nodeStats->NetworkHealthNumberPriorToPowerLevelTest += nodeStats->aNetworkHealthNumberPriorToPowerLevelTest[i];
            }
            nodeStats->NetworkHealthNumber /= TESTROUNDS;
            nodeStats->NetworkHealthNumberPriorToPowerLevelTest /= TESTROUNDS;
            NetworkManagement_UpdateTrafficLight(nodeStats);
            NextNodeNetworkHealthTest();
        }
    }
}


/*======================== NetworkManagement_NH_Start =========================
** Function description
**      Determines the Z-Wave Network Health for the node described in
** the structure pointed to by spNodeUT
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
bool 
NetworkManagement_NH_Start(
    sNetworkManagement_Stat *spNodeUT)
{
    bool retVal = false;

    nodeStats = spNodeUT;
    if ((NULL != spNetworkManagement) && (NULL != nodeStats))
    {
        if ((nodeStats->bDestNodeID > 0) || (nodeStats->bDestNodeID <= 232))
        {
            if (spNetworkManagement->testNeeded <= TESTCOUNT)
            {
                nodeStats->testRounds = 0;
                InitializeTransmitMetrics();
                spCurrentSample = &nodeStats->samples[nodeStats->testCount];
                NetworkManagement_NH_TestConnection((BYTE)nodeStats->bDestNodeID, NetworkManagement_NH_TestConnectionComplete);
                retVal = true;
            }
            else
            {
                mainlog_wr("Testcount not valid. Min 1, Max %u.\n", TESTCOUNT);
            }
        }
        else
        {
            mainlog_wr("NodeID not valid must be between 1 - %u, parameter specified %u.\n", ZW_MAX_NODES, nodeStats->bDestNodeID);
        }
    }
    else
    {
        mainlog_wr("Network Health not initialized.\n");
    }
    return retVal;
}


/*============== NetworkManagement_CalculateSerialAPIComDelay ================
** Function description
**		Makes a serial transmit time measurement to determine the Tapp, 
** which is the Serial communication delay introduced by the interface between
** the HOST SerialAPI implementation and the Z-Wave SerialAPI Module.
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
long
NetworkManagement_CalculateSerialAPIComDelay()
{
	/* HOST transmit metrics sampling specifics - Only needed if hw do not support transmit latency sampling */
    double retVal = MAXLONG;
    TimingStart(&sTxTiming);
    BYTE bSUCNodeID = api.ZW_GetSUCNodeID();
    TimingStop(&sTxTiming);

    return (long)TimingGetElapsedMSec(sTxTiming.elapsedTime);
}


/*======================= StartNextNetworkHealthTest =========================
** Function description
**		Starts the Network Health test on the node identified by the
** spNetworkManagement->abNodesUnderTest[spNetworkManagement->bNHSCurrentIndex] nodeID
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
bool
StartNextNetworkHealthTest()
{
    bool retVal = false;

    if (NULL != spNetworkManagement)
    {
        switch (spNetworkManagement->bTestMode)
        {
            case FULL:
                {
                    BYTE bNextNodeID = spNetworkManagement->abNodesUnderTest[spNetworkManagement->bNHSCurrentIndex];
                    if (0 < bNextNodeID)
                    {
                        BYTE bNextIndex = bNextNodeID - 1;
                        mainlog_wr("Starting Z-Wave Network Health for Node %03u", bNextNodeID);
                        spNetworkManagement->nodeUT[bNextIndex].bDestNodeID = bNextNodeID;
                        /* Number of Test transmits used to determine the Z-Wave Network Health */
                        spNetworkManagement->testNeeded = TESTCOUNT;
                        spNetworkManagement->testRoundsNeeded = TESTROUNDS;
                        /* If a Test Transmit takes more than  TRANSMIT_LATENCY_JITTER_MAX ms */
						/* then classify the transmit as a "Route Change" */
                        spNetworkManagement->nodeUT[bNextIndex].testRouteDurationMAX = TRANSMIT_LATENCY_JITTER_MAX + spNetworkManagement->dTapp;
                        spNetworkManagement->fTestStarted = NetworkManagement_NH_Start(&spNetworkManagement->nodeUT[bNextIndex]);
                        if (spNetworkManagement->fTestStarted)
                        {
                            spNetworkManagement->bNHSCurrentIndex++;
                        }
                        else
                        {
                            spNetworkManagement->bNHSCurrentIndex = 0;
                            printf("\n");
                            mainlog_wr("Done with Z-Wave Network Health");
                            if (spNetworkManagement->CurrentTestCompleted)
                            {
                                spNetworkManagement->CurrentTestCompleted(1);
                            }
                        }
                    }
                }
                break;

            case SINGLE:
                {
                    for (spNetworkManagement->bNHSCurrentIndex = 0; spNetworkManagement->bNHSCurrentIndex < spNetworkManagement->bNodesUnderTestSize; spNetworkManagement->bNHSCurrentIndex++)
                    {
                        if (spNetworkManagement->abNodesUnderTest[spNetworkManagement->bNHSCurrentIndex] == spNetworkManagement->bCurrentTestNodeID)
                        {
                            break;
                        }
                    }
                    if (spNetworkManagement->bNHSCurrentIndex >= spNetworkManagement->bNodesUnderTestSize)
                    {
                        mainlog_wr("Could not start Z-Wave Network Health for Node %03u - Not a member of the Nodes Under Test", spNetworkManagement->bCurrentTestNodeID);
                    }
                    else
                    {
                        BYTE bNextIndex = spNetworkManagement->bCurrentTestNodeID - 1;
                        mainlog_wr("Starting Z-Wave Network Health for Node %03u", spNetworkManagement->bCurrentTestNodeID);
                        spNetworkManagement->nodeUT[bNextIndex].bDestNodeID = spNetworkManagement->bCurrentTestNodeID;
                        /* Number of Test transmits used to determine the Z-Wave Network Health for every testround */
                        spNetworkManagement->testNeeded = TESTCOUNT;
                        /* Number of Test Rounds used to determine the Z-Wave Network Health */
                        spNetworkManagement->testRoundsNeeded = TESTROUNDS;
                        /* If a Test Transmit takes more than  TRANSMIT_LATENCY_JITTER_MAX ms */
						/* then classify the transmit as a "Route Change" */
                        spNetworkManagement->nodeUT[bNextIndex].testRouteDurationMAX = TRANSMIT_LATENCY_JITTER_MAX + spNetworkManagement->dTapp;
                        spNetworkManagement->fTestStarted = NetworkManagement_NH_Start(&spNetworkManagement->nodeUT[bNextIndex]);
                    }
                    if (!spNetworkManagement->fTestStarted)
                    {
                        spNetworkManagement->bNHSCurrentIndex = 0;
                        printf("\n");
                        mainlog_wr("Could not start Z-Wave Network Health");
                        if (spNetworkManagement->CurrentTestCompleted)
                        {
                            spNetworkManagement->CurrentTestCompleted(1);
                        }
                    }
                }
                break;

            default:
                {
                    mainlog_wr("Unknown Z-Wave Network Health test mode %i", spNetworkManagement->bTestMode);
                }
                break;
        }
        retVal = spNetworkManagement->fTestStarted;
    }
    return retVal;
}


/*================ NetworkHealthUpdate_Maintenance_Complete ==================
** Function description
**      Callback function called when the Rediscovery functionality started
** by StartNextSingleNodeRediscovery has finished.
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
NetworkHealthUpdate_Maintenance_Complete(
    BYTE bStatus)
{
    if (NULL != spNetworkManagement)
    {
        BYTE bNodeID;
        sNetworkManagement_Stat *spNodeStats;

        spNetworkManagement->bTestMode = MAINTENANCE;
        bNodeID = spNetworkManagement->abNodesUnderTest[spNetworkManagement->bMaintenanceNHSCurrentIndex];
        spNodeStats = &spNetworkManagement->nodeUT[bNodeID - 1];
        if (7 < spNodeStats->NetworkHealthNumber)
        {
            /* NH Green */
            SetNodeMaintenanceState(spNodeStats, MAINTENANCE_STATE_IDLE);
        }
        else if (8 > spNodeStats->NetworkHealthNumber)
        {
            /* NH below Green */
            if (((MAINTENANCE_STATE_NH_RUN_NODE_N_NOTFAILED == GetNodeMaintenanceState(spNodeStats)) &&
                 ((2 < spNodeStats->testTXErrors) || (1 >= spNodeStats->NetworkHealthNumber)))
                || (0 == spNodeStats->NetworkHealthNumber))
            {
                /* Not possible to get in touch with Node Under Test */
                if (MAINTENANCE_STATE_NH_RUN_NODE_N_FAILED == GetNodeMaintenanceState(spNodeStats))
                {
                    SetNodeMaintenanceState(spNodeStats, MAINTENANCE_STATE_USER_FULL_REDISCOV_DO);
                    mainlog_wr("NMC - Node %03u trickered 'full network Rediscovery'", bNodeID);
                    /* Somethings not right... */
                    mainlog_wr("** ALERT ** NODE %03u TRICKERED FULL NETWORK REDISCOVERY", spNodeStats->bDestNodeID);
                    mainlog_wr("Press key when to initiate FULL NETWORK REDISCOVERY");
                    _getch();
                    spNetworkManagement->fMaintenanceNeedsFullRediscovery = true;
                }
                else
                {
                    SetNodeMaintenanceState(spNodeStats, MAINTENANCE_STATE_USER_CHECK);
                    /* Only do this if in maintenance mode */
                    mainlog_wr("** ALERT ** NODE %03u NEEDS POWER and MANUAL OPERATION CHECK", spNodeStats->bDestNodeID);
                    mainlog_wr("Press key when POWER and MANUAL OPERATION CHECK has been executed");
                    _getch();
                    SetNodeMaintenanceState(spNodeStats, MAINTENANCE_STATE_USER_CHECK_DONE);
                    if (!spNetworkManagement->afMaintenanceNeedsNetworkHealthTest[spNodeStats->bDestNodeID - 1])
                    {
                        mainlog_wr("NMC - Node %03u flaged in 'needing Network Health Test' list ", spNodeStats->bDestNodeID);
                    }
                    SetNodeMaintenanceState(spNodeStats, MAINTENANCE_STATE_NH_RUN_NODE_N_FAILED);
                    /* Node needs an Network Health Update */
                    spNetworkManagement->afMaintenanceNeedsNetworkHealthTest[spNodeStats->bDestNodeID - 1] = true;
                }
            }
            else
            {
                SetNodeMaintenanceState(spNodeStats, MAINTENANCE_STATE_FLAG_NODE_REDISCOV);
                if (!spNetworkManagement->afMaintenanceNeedsRediscovery[bNodeID - 1])
                {
                    mainlog_wr("Node %03u flagged in 'needing Rediscovery' list", bNodeID);
                }
                spNetworkManagement->afMaintenanceNeedsRediscovery[bNodeID - 1] = true;
                SetNodeMaintenanceState(spNodeStats, MAINTENANCE_STATE_IDLE);
            }
        }
        /* Next node index to test */
        if (++spNetworkManagement->bMaintenanceNHSCurrentIndex >= spNetworkManagement->bNodesUnderTestSize)
        {
            spNetworkManagement->bMaintenanceNHSCurrentIndex = 0;
        }
    }
}


/*===== NetworkManagement_NetworkHealth_Maintenance_TestNode_Complete ========
** Function description
**      Callback function called when NetworkManagement_ZW_SendData
** functionality has been executed or when a node needs a NetworkHealth 
** Recalculation (used by NetworkManagement_NetworkHealth_Maintenance).
**		A Network Health test can be started for node under Maintenance test.
** If no Network Health is started then the next node for Maintenance test
** is determined by incrementing the bMaintenanceNHSCurrentIndex.
**
** Side effects:
**
**--------------------------------------------------------------------------*/
void
NetworkManagement_NetworkHealth_Maintenance_TestNode_Complete(
    BYTE bStatus, 
	TX_STATUS_TYPE *psTxInfo)
{
    bool retVal = false;

    if (NULL != spNetworkManagement)
    {
        if (spNetworkManagement->nodeUT[spNetworkManagement->abNodesUnderTest[spNetworkManagement->bMaintenanceNHSCurrentIndex] - 1].fRecalculateNetworkHealth)
        {
            NetworkManagement_NH_Maintenance_Recalculate(spNetworkManagement->abNodesUnderTest[spNetworkManagement->bMaintenanceNHSCurrentIndex]);
            if (spNetworkManagement->afMaintenanceNeedsNetworkHealthTest[spNetworkManagement->abNodesUnderTest[spNetworkManagement->bMaintenanceNHSCurrentIndex] - 1])
            {
                /* We need to make Network Health Test on node */
                spNetworkManagement->bTestMode = SINGLE;
                spNetworkManagement->bCurrentTestNodeID = spNetworkManagement->abNodesUnderTest[spNetworkManagement->bMaintenanceNHSCurrentIndex];
                retVal = NetworkManagement_NetworkHealth_Start(NetworkHealthUpdate_Maintenance_Complete);
                mainlog_wr("Maintenance Network Health Update needed for Node %03u - %s", spNetworkManagement->abNodesUnderTest[spNetworkManagement->bMaintenanceNHSCurrentIndex], retVal ? "Initiated" : "Failed initiation");
                spNetworkManagement->afMaintenanceNeedsNetworkHealthTest[spNetworkManagement->abNodesUnderTest[spNetworkManagement->bMaintenanceNHSCurrentIndex] - 1] = (false == retVal);
            }
        }
        if (false == retVal)
        {
            /* Next node index to test */
            if (++spNetworkManagement->bMaintenanceNHSCurrentIndex >= spNetworkManagement->bNodesUnderTestSize)
            {
                spNetworkManagement->bMaintenanceNHSCurrentIndex = 0;
            }
        }
    }
}


/*================= NetworkManagement_ReDiscoverySingle_Compl ================
** Function description
**   Callback function called when ZW_RequestNodeNeighborUpdate (started in
**	 NetworkManagement_DoRediscoverySingle) returns
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
NetworkManagement_ReDiscoverySingle_Compl(
  BYTE bStatus) /*IN Status of neighbor update process*/
{
    if (requestUpdateOn)
    {
        if (bStatus != 0x21)
        {
            if (bStatus == 0x22)
            {
                /* Node rediscovered */
                mainlog_wr("Rediscovery done successful");
            }
            else
            {
                /* Node failed rediscovery */
                mainlog_wr("Rediscovery done unsuccessfully");
            }
            if (NULL != cbNetworkManagement_DoRediscoverSingle)
            {
                cbNetworkManagement_DoRediscoverSingle(bStatus);
            }
        }
        else
        {
            mainlog_wr("Rediscovery status %02X", bStatus);
        }
    }
}


/*=================== NetworkManagement_DoRediscoverySingle ==================
** Function description
**      Do Request Node neighbor Update on specified node
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
bool
NetworkManagement_DoRediscoverySingle(
    sNetworkManagement_Stat *spNodeStats,
    void (__cdecl *TestCompleted)(BYTE bStatus))
{
    bool retVal = false;
    if ((NULL != spNodeStats) && ((0 < spNodeStats->bDestNodeID) && (ZW_MAX_NODES >= spNodeStats->bDestNodeID)))
    {
        cbNetworkManagement_DoRediscoverSingle = TestCompleted;
        requestUpdateOn = true;
        /* Assume all Rediscovered */
        api.ZW_RequestNodeNeighborUpdate(spNodeStats->bDestNodeID, NetworkManagement_ReDiscoverySingle_Compl);
        retVal = true;
    }
    else
    {
        requestUpdateOn = false;
    }
    return retVal;
}


/*===== NetworkManagement_NetworkHealth_Maintenance_RediscoveryComplete ======
** Function description
**      Callback function called when NetworkManagement_DoRediscoveryStart
** functionality (used by NetworkManagement_NetworkHealth_Maintenance)
** has been executed.
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
NetworkManagement_NetworkHealth_Maintenance_RediscoveryComplete(
    BYTE bStatus)
{
    if (NULL != spNetworkManagement)
    {
        spNetworkManagement->fMaintenanceFullDiscoveryStarted = false;
        spNetworkManagement->fMaintenanceNeedsFullRediscovery = false;
        /* If failed then we need to */
        spNetworkManagement->fMaintenanceNotifyCallCenter = (0 == bStatus);
        mainlog_wr("Maintenance Network Full Rediscovery stopped - Status %u", bStatus);
    }
}


/*======================== AnyNodesNeedingRediscovery ========================
** Function description
**      Test the afMaintenanceNeedsRediscovery array if any nodes needs to
** rediscovered and returns true if any.
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
bool
AnyNodesNeedingRediscovery()
{
    bool retVal = false;

    if (NULL != spNetworkManagement)
    {
        for (int i = 0; i < spNetworkManagement->bNodesUnderTestSize; i++)
        {
            if (true == spNetworkManagement->afMaintenanceNeedsRediscovery[spNetworkManagement->abNodesUnderTest[i] - 1])
            {
                /* Found a node needing a neighbor update */
                retVal = true;
                break;
            }
        }
    }
    return retVal;
}


/*======================== SingleNodeRediscovery_Done ========================
** Function description
**      Callback function called when the Rediscovery functionality started
** by StartNextSingleNodeRediscovery has finished.
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
SingleNodeRediscovery_Done(
    BYTE bStatus)
{
    if (NULL != spNetworkManagement)
    {
        BYTE bNodeID = spNetworkManagement->abNodesUnderTest[spNetworkManagement->bMaintenanceNHSCurrentIndex];

        if (0x22 == bStatus)
        {
            sNetworkManagement_Stat *spNodeStats = &spNetworkManagement->nodeUT[bNodeID - 1];

            mainlog_wr("Node %03u now removed from 'needing Rediscovery' list", bNodeID);
            /* Node rediscovery successfull */
            spNetworkManagement->afMaintenanceNeedsRediscovery[bNodeID - 1] = false;
            /* Reset lifetime route change */
            spNodeStats->routeChangeLifetime = 0;
            spNodeStats->routeChangeLifetimeOld = 0;
        }
        else
        {
            mainlog_wr("Node %03u, Rediscovery failed", bNodeID);
        }
		NetworkManagement_DumpNodeNeighbors(spNetworkManagement->bMaintenanceNHSCurrentIndex);
        spNetworkManagement->bMaintenanceNHSCurrentIndex++;
        StartNextSingleNodeRediscovery();
    }
}


/*====================== StartNextSingleNodeRediscovery ======================
** Function description
**      Find Next Node which is marked as needing a Rediscovery and start
** the rediscovery.
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
bool
StartNextSingleNodeRediscovery()
{
    bool retVal = false;

    if (NULL != spNetworkManagement)
    {
        while (spNetworkManagement->bMaintenanceNHSCurrentIndex < spNetworkManagement->bNodesUnderTestSize)
        {
            if (spNetworkManagement->afMaintenanceNeedsRediscovery[spNetworkManagement->abNodesUnderTest[spNetworkManagement->bMaintenanceNHSCurrentIndex] - 1])
            {
                /* Do we need a timer... */
                retVal = NetworkManagement_DoRediscoverySingle(&spNetworkManagement->nodeUT[spNetworkManagement->abNodesUnderTest[spNetworkManagement->bMaintenanceNHSCurrentIndex] - 1], SingleNodeRediscovery_Done);
                mainlog_wr("Node %03u needs to be Rediscovered - %s", spNetworkManagement->abNodesUnderTest[spNetworkManagement->bMaintenanceNHSCurrentIndex], retVal ? "Initiated" : "Failed");
                return retVal;
            }
            spNetworkManagement->bMaintenanceNHSCurrentIndex++;
        }
        if (spNetworkManagement->bMaintenanceNHSCurrentIndex >= spNetworkManagement->bNodesUnderTestSize)
        {
            /* Rediscovery done */
            spNetworkManagement->bMaintenanceNHSCurrentIndex = 0;
            spNetworkManagement->fMaintenanceSingleDiscoveryInitiated = false;
        }
    }
    return retVal;
}


/*=============== NetworkManagement_NetworkHealth_Maintenance ===============
** Function description
**      NetworkManagement API Maintenance function which do the actual 
** Network Health Maintenance in Runtime.
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
bool
NetworkManagement_NetworkHealth_Maintenance()
{
    static long currentTick;
    /* Return value true if next Network Health Maintenance sample has been initiated */
    bool retVal = false;
    /* Is it time to touch next node */
    if ((NULL != spNetworkManagement) && (MAINTENANCE == spNetworkManagement->bTestMode))
    {
        /* We only check on tick transition */
        if (currentTick != lNetworkManagementTimeSecondsTicker)
        {
            currentTick = lNetworkManagementTimeSecondsTicker;
            if  (iSecondsBetweenEveryMaintenanceSample <= lNetworkManagementTimeSecondsTicker - lNetworkManagementCurrentSecondsTickerSample)
            {
                if ((!spNetworkManagement->fMaintenanceFullDiscoveryStarted) && (!spNetworkManagement->fMaintenanceSingleDiscoveryInitiated))
                {
                    sNetworkManagement_Stat *spNodeStats = &spNetworkManagement->nodeUT[spNetworkManagement->abNodesUnderTest[spNetworkManagement->bMaintenanceNHSCurrentIndex] - 1];
                    BYTE bNodeID = spNetworkManagement->abNodesUnderTest[spNetworkManagement->bMaintenanceNHSCurrentIndex];
                    /* TODO - The Full Rediscovery situation should be decided howto and when to handle it */
                    /* For now we just do it, with no regard to time of day etc. */
                    if (spNetworkManagement->fMaintenanceNeedsFullRediscovery)
                    {
                        mainlog_wr("Maintenance: Full network rediscovery - Current Node %03u", bNodeID);
                        spNetworkManagement->fMaintenanceFullDiscoveryStarted = NetworkManagement_DoRediscoveryStart(NetworkManagement_NetworkHealth_Maintenance_RediscoveryComplete);
                    }
                    else
                    {
                        if (AnyNodesNeedingRediscovery() && (0 == spNetworkManagement->bMaintenanceNHSCurrentIndex))
                        {
                            /* TODO - when to do single rediscovery should be decided */
                            /* For now we do it when a specified number of Maintenance Rounds have been executed, with no regard to time of day etc. */
                            /* Also all flagged nodes are rediscovered one after the other */
                            if (++spNetworkManagement->iMaintenanceRoundsSinceLastRediscovery >= spNetworkManagement->iMaintenanceRoundsBeforeRediscoveryExecuted)
                            {
                                mainlog_wr("Maintenance: Rediscovery of flagged nodes trickered");
                                /* Restart Maintenance Round count */
                                spNetworkManagement->iMaintenanceRoundsSinceLastRediscovery = 0;
                                /* Start the Rediscovery of Nodes needing it... */
                                spNetworkManagement->fMaintenanceSingleDiscoveryInitiated = StartNextSingleNodeRediscovery();
                            }
                        }
                        else
                        {
                            if (spNodeStats->fRecalculateNetworkHealth)
                            {
                                mainlog_wr("Maintenance: Recalc - Node %03u", bNodeID);
                                NetworkManagement_NetworkHealth_Maintenance_TestNode_Complete(TRANSMIT_COMPLETE_OK, 0);
                            }
                            else
                            {
                                mainlog_wr("Maintenance: Sample - Node %03u, NH %02u, NB %u", bNodeID, spNodeStats->NetworkHealthNumber, spNodeStats->neighborCount);
                                /* We are in Maintenance Network Health Test mode and its time for next sample */
                                retVal = (0 != NetworkManagement_ZW_SendData(bNodeID, testConnectionPayload, sizeof(testConnectionPayload), NetworkManagement_NetworkHealth_Maintenance_TestNode_Complete));
                            }
                        }
                    }
                }
                /* Either way - Setup next Maintenance sample event */
                lNetworkManagementCurrentSecondsTickerSample = lNetworkManagementTimeSecondsTicker;
            }
        }
    }
    return retVal;
}


/*============= NetworkManagement_NetworkHealth_MaintenanceStart =============
** Function description
**      NetworkManagement API Maintenance Start function needed to be called
** before using NetworkManagement_NetworkHealth_Maintenance.
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
bool
NetworkManagement_NetworkHealth_MaintenanceStart()
{
    bool retVal = false;

    if ((NULL != spNetworkManagement) && (0 < spNetworkManagement->bNodesUnderTestSize))
    {
        switch (spNetworkManagement->bTestMode)
        {
            case IDLE:
                /* Drop through */

            case MAINTENANCE:
                {
                    if ((NULL != spNetworkManagement) && (0 < spNetworkManagement->bNodesUnderTestSize))
                    {
                        if (MAINTENANCE != spNetworkManagement->bTestMode)
                        {
                            mainlog_wr("Maintenance restart...");
                        }
                        spNetworkManagement->bTestMode = MAINTENANCE;
                        mainlog_wr("Maintenance initiated");
                        for (int i = 0; i < spNetworkManagement->bNodesUnderTestSize; i++)
                        {
                            if (spNetworkManagement->nodeUT[spNetworkManagement->abNodesUnderTest[i] - 1].fRecalculateNetworkHealth)
                            {
                                mainlog_wr("Node %03u needs Network Health to be recalculated", spNetworkManagement->abNodesUnderTest[i]);
                                NetworkManagement_NH_Maintenance_Recalculate(spNetworkManagement->abNodesUnderTest[i]);
                            }
                            /* Reinitialiaze routeChangeLifeTime variables */
                            spNetworkManagement->nodeUT[spNetworkManagement->abNodesUnderTest[i] - 1].routeChangeLifetime = 0;
                            spNetworkManagement->nodeUT[spNetworkManagement->abNodesUnderTest[i] - 1].routeChangeLifetimeOld = 0;
                        }
                        /* Sample Maintenance start second tick */
                        lNetworkManagementCurrentSecondsTickerSample = lNetworkManagementTimeSecondsTicker;
                        iSecondsBetweenEveryMaintenanceSample = (0 != spNetworkManagement->iMaintenanceSamplePeriod) ? spNetworkManagement->iMaintenanceSamplePeriod : MAINTENANCE_PERIOD_SECS / spNetworkManagement->bNodesUnderTestSize;
                        spNetworkManagement->bMaintenanceNHSCurrentIndex = 0;
                        mainlog_wr("Maintenance start tick %lu, sample period %u seconds", 
                                   lNetworkManagementCurrentSecondsTickerSample, iSecondsBetweenEveryMaintenanceSample);
                        retVal = true;
                    }
                }
                break;

            case SINGLE:
                {
                    mainlog_wr("Maintenance not possible Network Health are in SINGLE testmode ");
                }
                break;

            case FULL:
                {
                    mainlog_wr("Maintenance not possible Network Health are in FULL testmode ");
                }
                break;

            default:
                {
                    mainlog_wr("Network Health in undefined testmode %u, Reset to IDLE", spNetworkManagement->bTestMode);
                    spNetworkManagement->bTestMode = IDLE;
                }
        }
    }
    return retVal;
}


/*============= NetworkManagement_NetworkHealth_MaintenanceStop ==============
** Function description
**      NetworkManagement API Maintenance Stop function - If in Network Health
** Maintenance Mode then Mode set to IDLE. Return true if Network Health
** Maintenance Mode was actually stopped.
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
bool
NetworkManagement_NetworkHealth_MaintenanceStop()
{
    bool retVal = false;

    if ((NULL != spNetworkManagement) && (0 < spNetworkManagement->bNodesUnderTestSize))
    {
        switch (spNetworkManagement->bTestMode)
        {
            case IDLE:
                {
                    mainlog_wr("Not in Network Health Maintenance Mode - IDLE");
                }
                break;

            case MAINTENANCE:
                {
                    spNetworkManagement->bTestMode = IDLE;
					spNetworkManagement->fTestStarted = false;
                    mainlog_wr("Network Health Maintenance Mode stopped");
                    retVal = true;
                }
                break;

            case SINGLE:
                {
                    mainlog_wr("Not in Network Health Maintenance Mode - SINGLE");
                }
                break;

            case FULL:
                {
                    mainlog_wr("Not in Network Health Maintenance Mode - FULL");
                }
                break;

            default:
                {
                    mainlog_wr("Not in Network Health Maintenance Mode - Undefined mode %i, Reset to IDLE", spNetworkManagement->bTestMode);
                    spNetworkManagement->bTestMode = IDLE;
					spNetworkManagement->fTestStarted = false;
                }
                break;
        }
    }
    return retVal;
}


/*============== NetworkManagement_UpdateNeighborInformation =================
** Function description
**		Update the Neighbor information for all Nodes Under Test
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
NetworkManagement_UpdateNeighborInformation()
{
    for (int i = 0; i < spNetworkManagement->bNodesUnderTestSize; i++)
    {
        BYTE n = spNetworkManagement->abNodesUnderTest[i] - 1;
        spNetworkManagement->nodeUT[n].bDestNodeID = spNetworkManagement->abNodesUnderTest[i];
        NetworkManagement_UpdateNodeNeighbor(spNetworkManagement->bControllerNodeID, &spNetworkManagement->nodeUT[n]);
    }
}


/*===================== NetworkManagement_ReDiscovery_Compl ==================
** Function description
**   Callback function called when ZW_RequestNodeNeighborUpdate (started in
**	 NetworkManagement_DoRediscoveryStart) returns
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
NetworkManagement_ReDiscovery_Compl(
  BYTE bStatus) /*IN Status of neighbor update process*/
{
    if (bStatus != 0x21)
    {
        if (requestUpdateOn && spNetworkManagement->fTestStarted)
        {
            if (bStatus == 0x22)
            {
                /* Node rediscovered */
                spNetworkManagement->abNodesReDiscovered[spNetworkManagement->bNHSCurrentIndex] = true;
                mainlog_wr("Rediscovery Round %u, Node %u Rediscovered", spNetworkManagement->bReDiscoverRound, spNetworkManagement->abNodesUnderTest[spNetworkManagement->bNHSCurrentIndex]);
            }
            else
            {
                /* Node failed rediscovery */
                mainlog_wr("Rediscovery Round %u, Node %u failed Rediscovery", spNetworkManagement->bReDiscoverRound, spNetworkManagement->abNodesUnderTest[spNetworkManagement->bNHSCurrentIndex]);
                /* Now at least 1 failed the rediscovery try again if applicable */
                spNetworkManagement->fAllDiscovered = false;
            }
			NetworkManagement_DumpNodeNeighbors(spNetworkManagement->bNHSCurrentIndex);
            while (++spNetworkManagement->bNHSCurrentIndex < spNetworkManagement->bNodesUnderTestSize)
            {
                if (!spNetworkManagement->abNodesReDiscovered[spNetworkManagement->bNHSCurrentIndex])
                {
                    /* Found a node which havent been reached yet */
                    break;
                }
            }
            if ((spNetworkManagement->bNHSCurrentIndex >= spNetworkManagement->bNodesUnderTestSize) && !spNetworkManagement->fAllDiscovered)
            {
                /* New rediscovery round */
                if (++spNetworkManagement->bReDiscoverRound < MAX_REDISCOVER_ROUNDS)
                {
                    for (spNetworkManagement->bNHSCurrentIndex = 0; spNetworkManagement->bNHSCurrentIndex < spNetworkManagement->bNodesUnderTestSize; spNetworkManagement->bNHSCurrentIndex++)
                    {
                        if (!spNetworkManagement->abNodesReDiscovered[spNetworkManagement->bNHSCurrentIndex])
                        {
                            /* Assume Success */
                            spNetworkManagement->fAllDiscovered = true;
                            /* Found a node which havent been reached yet */
                            break;
                        }
                    }
                }
            }
            if (spNetworkManagement->bNHSCurrentIndex >= spNetworkManagement->bNodesUnderTestSize)
            {
                spNetworkManagement->bNHSCurrentIndex = 0;
                spNetworkManagement->fTestStarted = false;
                requestUpdateOn = false;
                NetworkManagement_UpdateNeighborInformation();
                /* Done */
                if (spNetworkManagement->fAllDiscovered)
                {
                    mainlog_wr("NetworkManagement ReDiscovery Done - SUCCESS");
                    if (spNetworkManagement->CurrentTestCompleted)
                    {
                        spNetworkManagement->CurrentTestCompleted(1);
                    }
                }
                else
                {
                    mainlog_wr("NetworkManagement ReDiscovery Done - FAILED");
                    if (spNetworkManagement->CurrentTestCompleted)
                    {
                        spNetworkManagement->CurrentTestCompleted(0);
                    }
                }
            }
            else
            {
                bDutNodeID = spNetworkManagement->abNodesUnderTest[spNetworkManagement->bNHSCurrentIndex];
                api.ZW_RequestNodeNeighborUpdate(bDutNodeID, NetworkManagement_ReDiscovery_Compl);
            }
        }
        else
        {
            if (requestUpdateOn)
            {
                requestUpdateOn = false;
                spNetworkManagement->fTestStarted = false;
                NetworkManagement_UpdateNeighborInformation();
                mainlog_wr("NetworkManagement ReDiscovery Stopping");
                if (spNetworkManagement->CurrentTestCompleted)
                {
                    spNetworkManagement->CurrentTestCompleted(0);
                }
            }
            else
            {
                mainlog_wr("NetworkManagement ReDiscovery not running");
            }
        }
    }
    else
    {
        mainlog_wr("Rediscovery Round %u, Node %u (%02X)", spNetworkManagement->bReDiscoverRound, bDutNodeID, bStatus);
    }
}


/*=================== NetworkManagement_DoRediscoveryStart ===================
** Function description
**      Start Network Health if running
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
bool
NetworkManagement_DoRediscoveryStart(
    void (__cdecl *TestCompleted)(BYTE bStatus))
{
    bool retVal = false;
    if (spNetworkManagement->bNodesUnderTestSize > 0)
    {
        spNetworkManagement->CurrentTestCompleted = TestCompleted;
        spNetworkManagement->bNHSCurrentIndex = 0;
        spNetworkManagement->fTestStarted = true;
        spNetworkManagement->bReDiscoverRound = 0;
        /* Initially no nodes have been rediscovered/touched */
        memset(spNetworkManagement->abNodesReDiscovered, 0, sizeof(spNetworkManagement->abNodesReDiscovered));

        bDutNodeID = spNetworkManagement->abNodesUnderTest[spNetworkManagement->bNHSCurrentIndex];
        requestUpdateOn = true;
        /* Assume all Rediscovered */
        spNetworkManagement->fAllDiscovered = true;
        api.ZW_RequestNodeNeighborUpdate(bDutNodeID, NetworkManagement_ReDiscovery_Compl);
        retVal = true;;
    }
    else
    {
        spNetworkManagement->fTestStarted = false;
        spNetworkManagement->fAllDiscovered = false;
        requestUpdateOn = false;
    }
    return retVal;
}


/*=================== NetworkManagement_DoRediscoveryStop ====================
** Function description
**      Stop Network Rediscovery if running
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
NetworkManagement_DoRediscoveryStop(
    sNetworkManagement *spNetworkMan)
{
    /* ReDiscovery stopping */
    spNetworkMan->fTestStarted = false;
}


/*=========================== SecondTickerTimeout ============================
** Function description
**      Second ticker used by Maintenance functionality
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
SecondTickerTimeout(
    int H)
{
    /* One more second passed */
    lNetworkManagementTimeSecondsTicker++;
    /* Start Second timer again */
    timerSecH = pSecTimer->Timeout(SECONDTICKERTIMEOUT, SecondTickerTimeout);
}


/*============== NetworkManagement_IsFuncIDsSupported =================
** Function description
**      Checks if the specified function IDs are supported according to the
** SerialAPI supported functionality bitmask
**		
** Side effects:
**
**--------------------------------------------------------------------------*/
bool
NetworkManagement_IsFuncIDsSupported(
    BYTE *pabFuncIDsToSupport,
    BYTE bFuncIDsToSupportSize)
{
    bool retVal = true;
    BYTE *psCap = spNetworkManagement->psSerialAPIControllerCapabilities->abFuncIDSupportedBitmask;

    for (int i = 0; i < bFuncIDsToSupportSize; i++)
    {
        int bCmd = pabFuncIDsToSupport[i];
        retVal = api.BitMask_isBitSet(psCap, bCmd);
        if (!retVal)
        {
            break;
        }
    }
    return retVal;
}


/*=================== NetworkManagement_NetworkHealth_Init ===================
** Function description
**      Initialize Network Management structures for Network Health, Rediscovery
** and Check Repeater usage
**		
** Side effects:
**		Sets spNetworkManagement to point to Application specified data structures
**--------------------------------------------------------------------------*/
bool
NetworkManagement_Init(
    S_SERIALAPI_CAPABILITIES *psSerialAPIControllerCapabilities,
    BYTE *abListeningNodeList, 
    BYTE bListeningNodeListSize, 
    bool *afIsNodeAFLiRS, 
    BYTE bNetworkRepeaterCount,
    sNetworkManagement *spNetworkMan)
{
    bool retVal = false;
    BYTE abNeedSerialAPIFunctionality[] = {FUNC_ID_ZW_SEND_DATA,
                                           FUNC_ID_ZW_REQUEST_NODE_NEIGHBOR_UPDATE,
                                           FUNC_ID_ZW_GET_SUC_NODE_ID,
                                           FUNC_ID_ZW_REQUEST_NODE_INFO,
                                           FUNC_ID_GET_ROUTING_TABLE_LINE,
                                           FUNC_ID_ZW_GET_LAST_WORKING_ROUTE};
    BYTE abNeedForPowerLevelSupport[] = {FUNC_ID_ZW_RF_POWER_LEVEL_SET,
                                         FUNC_ID_ZW_RF_POWER_LEVEL_GET,
                                         FUNC_ID_ZW_SEND_TEST_FRAME};
    spNetworkManagement = spNetworkMan;
    if (NULL != spNetworkManagement)
    {
        if (NULL == networkHealthlog)
        {
            struct _timeb timebuffer;
            struct tm timeinfo;
            errno_t err;
            const char* timestamp = "log_networkHealth_%u%02u%02u-%02u%02u%02u.txt";
            err = _ftime64_s(&timebuffer);
            err = _localtime64_s(&timeinfo, &timebuffer.time);
            sprintf_s(networkHealth_logfilenameStr, sizeof(networkHealth_logfilenameStr), timestamp,
                      timeinfo.tm_year+1900, timeinfo.tm_mon+1, timeinfo.tm_mday,
                      timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
            /* No need to initialize log more than once */
            networkHealthlog = mlog.AddLog(networkHealth_logfilenameStr);
			mlog.Write(networkHealthlog, "Network Health Maintenance log initialized");
        }
        spNetworkManagement->psSerialAPIControllerCapabilities = psSerialAPIControllerCapabilities;
        spNetworkManagement->abNodesUnderTest = abListeningNodeList;
        spNetworkManagement->bNodesUnderTestSize = bListeningNodeListSize;
		spNetworkManagement->afIsNodeAFLiRS = afIsNodeAFLiRS;
        spNetworkManagement->bNetworkRepeaterCount = bNetworkRepeaterCount;
        spNetworkManagement->fTestStarted = false;
        spNetworkManagement->bTestMode = IDLE;
        /* Use - 60Min / NumberofNodeInNetwork */
        spNetworkManagement->iMaintenanceSamplePeriod = 0;
        spNetworkManagement->iMaintenanceRoundsBeforeRediscoveryExecuted = MAINTENANCEPRIORREDISCOVERY;
		/* HOST transmit metrics sampling specifics - Only needed if hw do not support transmit latency sampling */
        spNetworkManagement->dTapp = NetworkManagement_CalculateSerialAPIComDelay();

        api.MemoryGetID(spNetworkManagement->abHomeID, &spNetworkManagement->bControllerNodeID);

        NetworkManagement_UpdateNeighborInformation();
        pSecTimer = CTime::GetInstance();
        //If timer is running, kill it!
        if (0 != timerSecH)
        {
            pSecTimer->Kill(timerSecH);
        }
        timerSecH = pSecTimer->Timeout(SECONDTICKERTIMEOUT, SecondTickerTimeout);
        
        spNetworkManagement->fControllerSupportPowerLevelTest = NetworkManagement_IsFuncIDsSupported(abNeedForPowerLevelSupport, sizeof(abNeedForPowerLevelSupport));
        retVal = NetworkManagement_IsFuncIDsSupported(abNeedSerialAPIFunctionality, sizeof(abNeedSerialAPIFunctionality));
    }
    return retVal;
}


/*=================== NetworkManagement_NetworkHealthStart ===================
** Function description
**      Start Network Health if initialized with 
** NetworkManagement_InitNetworkHealth
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
bool
NetworkManagement_NetworkHealth_Start(
    void (__cdecl *TestCompleted)(BYTE bStatus))
{
    bool retVal = false;

    if (NULL != spNetworkManagement) 
    {
        if (spNetworkManagement->bNodesUnderTestSize > 0)
        {
            spNetworkManagement->CurrentTestCompleted = TestCompleted;
            spNetworkManagement->bNHSCurrentIndex = 0;
            spNetworkManagement->fTestStarted = false;

            StartNextNetworkHealthTest();
            retVal = true;
        }
        else
        {
            spNetworkManagement->fTestStarted = false;
        }
    }
    else
    {
        mainlog_wr("Network Health not initialized - use NetworkManagement_NetworkHealth_Init");
    }
    return retVal;
}


/*=================== NetworkManagement_NetworkHealthStop ====================
** Function description
**      Stop Network Health if running
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
NetworkManagement_NetworkHealth_Stop(
    sNetworkManagement *spNetwork)
{
    /* ReDiscovery stopping */
    spNetwork->fTestStarted = false;
}

