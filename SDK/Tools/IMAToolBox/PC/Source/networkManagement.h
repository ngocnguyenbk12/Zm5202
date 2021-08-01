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
 * Description: Headerfile for the Z-Wave Network Health module
 *
 * Last Changed By:  $Author$: 
 * Revision:         $Rev$: 
 * Last Changed:     $Date$: 
 *
 ****************************************************************************/
#ifndef NETWORKMANAGEMENT_H
#define NETWORKMANAGEMENT_H

#include "PowerLevelTest.h"

#define MAX_REDISCOVER_ROUNDS		5

#define TESTROUNDS					6
#define TESTCOUNT					10

#define POWERLEVELTEST_LEVEL		6
#define POWERLEVELTEST_COUNT		10

#define MAINTENANCE_PERIOD_SECS		(60 * 60)

#define SECONDTICKERTIMEOUT			1

#define MAINTENANCEPRIORREDISCOVERY 5

/* Number of ms a transmit latency must increase before the sample is determined as a RC */
#define TRANSMIT_LATENCY_JITTER_MAX	160

#define MAX_ROUTE_LINKS				(ZW_MAX_REPEATERS + 1)


/*--------------------------------------------------------------------------*/

typedef struct _sNODETYPE_CMDCLASS
{
	NODEINFO nodeInfo;
	/* Node type */
	NODE_TYPE nodeType;
	/* Which Command Classes do the node support */
	BYTE cmdClasses[30];
} sNODETYPE_CMDCLASS;


typedef struct _sSample
{
	/* Transmit latency for current sample */
	long sampleTime;	/* */
	/* LWR for current sample */
	BYTE pRoute[ROUTECACHE_LINE_SIZE];
	/* Has route changed been detected */
	bool routeChanged;
	/* Is LWR in pRoute uniq for the current TESTCOUNT samples */
	bool routeUniq;
	/* Actually redundant - if transmit failed  = no LWR is present */
	/*						if transmit success =  a LWR is present */
	bool routeFound;	
	/* Did the timed ZW_SendData succeed */
	bool failed;		
} sSample;


typedef enum _eMAINTENANCESTATE
{
	MAINTENANCE_STATE_IDLE,
	MAINTENANCE_STATE_SENDTEST,
	MAINTENANCE_STATE_UPDATEPERRC,
	MAINTENANCE_STATE_CHECKPERRC,
	MAINTENANCE_STATE_NH_RUN_NODE_N_NOTFAILED,
	MAINTENANCE_STATE_NH_CHECK_NOT_FAILED,
	MAINTENANCE_STATE_FLAG_NODE_REDISCOV,
	MAINTENANCE_STATE_USER_CHECK,
	MAINTENANCE_STATE_USER_CHECK_DONE,
	MAINTENANCE_STATE_NH_RUN_NODE_N_FAILED,
	MAINTENANCE_STATE_NH_CHECK_FAILED,
	MAINTENANCE_STATE_USER_FULL_REDISCOV_DO,
	MAINTENANCE_STATE_USER_FULL_REDISCOV_DONE,
	MAINTENANCE_STATE_CLEAR_ALL_RCLIFETIME,
	MAINTENANCE_STATE_CALLCENTER_NOTIFY,
	MAINTENANCE_STATE_UNDEFINED
} eMAINTENANCESTATE;


#define MAINTENANCESTATE_BACKLOGSIZE	10


typedef struct _sNodeMaintenanceState
{
	/* Current Maintenance state */
	eMAINTENANCESTATE eMaintenanceState;
	/* Last 10 States */
	eMAINTENANCESTATE aeMaintenanceState[MAINTENANCESTATE_BACKLOGSIZE];
	/* Count of transistions to state */
	int aiTransistionCount[MAINTENANCE_STATE_UNDEFINED];
} sNodeMaintenanceState;


typedef struct _sNetworkManagement_Stat
{
	/* Destination Node ID for sampled transmit time and LWR in pRoute */
	BYTE bDestNodeID;
	BYTE abRoutingInfoNodeMask[ZW_MAX_NODEMASK_LENGTH];
	BYTE abNeighbors[ZW_MAX_NODES];
	bool bControllerIsANeighborAndNotARepeater;
	int neighborCount;
	/* How many ms may a Test Transmit be longer than the previous Test Transmit */
	/* before the Test Transmit is classified as having a "Route Change" occurence */
	long testRouteDurationMAX;	
					
	/* If true then a new sample has been done and current Network Health should be updated */
	bool fRecalculateNetworkHealth;
	/* Current testCount */
	int	testCount;
	/* Current testRound */
	int testRounds;
	/* Total Number of transmits made are "testCount", */
	/* Number of successful transmit to nodeID are "testCount-testTXErrors" */
	/* Number of unsuccessful Transmits - PER */
	int testTXErrors;
	/* How many "Route Change" occurences has been detected during network health test - RC */
	int routeChange;
	/* How many "Route Change" occurences has been detected in total during maintenance - RCLifetime */
	int routeChangeLifetime;
	/* Last route change life time value */
	int routeChangeLifetimeOld;
	/* How many different routes has successfully been tried */
	int routeCount;
	/* Current TESTCOUNT timing samples of Transmission to */
	sSample samples[TESTCOUNT];
	/* SUM(samples)/testCount */
	double average;	
	/* SQRT(1/(testCount-1)*SUM(SQ(samples-average)) */
	double sampleStdDev;
	/* MIN(samples) */
	long min;
	/* MAX(samples) */
	long max;
	signed char backgroundRssi[MAX_CHANNELS]; /* 3 channels max */
	int NetworkHealthNumberPriorToPowerLevelTest;
	int aNetworkHealthNumberPriorToPowerLevelTest[TESTROUNDS];
	/* The NetworkHealthNumber for all testrounds */
	/* The current status of the node in the network */
	int NetworkHealthNumber;
	int aNetworkHealthNumber[TESTROUNDS];
	/* The color of the "traffic light" */
	char* NetworkHealthSymbol; 
	/* If NH >= 8 then if applicable we do PowerLevel test for all links in LWR */
	/* and act accordingly to the result */
	BYTE bPowerTestSuccess[MAX_ROUTE_LINKS];
	POWLEV_STATUS powStatus[MAX_ROUTE_LINKS];
	sNodeMaintenanceState sNodeState;
	signed int linkMargins[MAX_ROUTE_LINKS]; /* RSSI test link margins */
} sNetworkManagement_Stat;


typedef enum _eTESTMODE
{
	IDLE,
	SINGLE,
	FULL,
	MAINTENANCE,
	UNDEF
} eTESTMODE;


typedef struct _sNetworkManagement
{
	bool fTestStarted;
	eTESTMODE bTestMode;
	int iMaintenanceSamplePeriod;
	int iMaintenanceRoundsBeforeRediscoveryExecuted;
	int iMaintenanceRoundsSinceLastRediscovery;
	int testNeeded;
	int testRoundsNeeded;
	/* Node ID on node under test if testMode = SINGLE */
	BYTE bCurrentTestNodeID;
	/* NodeId on Controller */
	BYTE bControllerNodeID;
	/* HomeID on Controller */
	BYTE abHomeID[4];
	/* SerialAPI capabilities - Supported functionality */
	S_SERIALAPI_CAPABILITIES *psSerialAPIControllerCapabilities;
	/* Do SerialAPI module support needed PowerLevel functionality to execute a PowerLevel link Test */
	bool fControllerSupportPowerLevelTest;
	/* Pointer to Application delivered BYTE list containing ALL nodeIDs under test */
	BYTE *abNodesUnderTest;
	/* Contains the number of NodeIDs in the BYTE list identified by abNodesUnderTest */
	BYTE bNodesUnderTestSize;
	bool *afIsNodeAFLiRS;
	BYTE bNetworkRepeaterCount;
	BYTE bReDiscoverRound;
	bool abNodesReDiscovered[ZW_MAX_NODES];
	bool fAllDiscovered;
	/* Tapp = SerialAPI serial communication delay introduced by current HOST implementation */
	long dTapp;
	/* Structure containing the sampled Transmit metrics which is used for determining the Network Health Number */
	sNetworkManagement_Stat nodeUT[ZW_MAX_NODES];
	/* Structure containing the nodeinformation for the existing nodes in network */
	sNODETYPE_CMDCLASS nodeDescriptor[ZW_MAX_NODES];
	/* List containing one bit for every potential node in network. */
	/* If true then the corresponding NodeID need a Rediscovery done */
	bool afMaintenanceNeedsRediscovery[ZW_MAX_NODES];
	/* List containing one bit for every potential node in network. */
	/* If true then the corresponding NodeID need a Network Health Test done */
	bool afMaintenanceNeedsNetworkHealthTest[ZW_MAX_NODES];
	/* Network needs a full rediscovery to be done */
	bool fMaintenanceNeedsFullRediscovery;
	bool fMaintenanceFullDiscoveryStarted;
	bool fMaintenanceSingleDiscoveryInitiated;
	bool fMaintenanceNotifyCallCenter;
	/* Current Maintenance index */
	BYTE bMaintenanceNHSCurrentIndex;
	/* Current index */
	BYTE bNHSCurrentIndex;
	void (__cdecl *CurrentTestCompleted)(BYTE bStatus);
} sNetworkManagement;

/* Used by the RssiMap module */
extern sNetworkManagement_Stat *nodeStats;

/*--------------------------------------------------------------------------*/

void NetworkManagement_DumpNeighbors();

/* Network Management - Network Health Installation Functionality */

/* Network Management Initialize function should be called prior to using any Network Management functionality */
bool NetworkManagement_Init(S_SERIALAPI_CAPABILITIES *psSerialAPIControllerCapabilities, BYTE *abListeningNodeList, BYTE bListeningNodeListSize, bool *afIsNodeAFLiRS, BYTE bNetworkRepeaterCount, sNetworkManagement *spNetworkMan);

/* Function which can be called after NetworkManagement_Init has been executed and returns if specified  */
/* Command Classes are supported by current attached Z-Wave SerialAPI module */
bool NetworkManagement_IsFuncIDsSupported(BYTE *pabCommandClassesToSupport, BYTE bCommandClassesToSupportSize);

/*  */
bool NetworkManagement_NetworkHealth_Start(void (__cdecl *TestCompleted)(BYTE bStatus));
void NetworkManagement_NetworkHealth_Stop(sNetworkManagement *spNetworkMan);

/* */
bool NetworkManagement_DoRediscoveryStart(void (__cdecl *TestCompleted)(BYTE bStatus));
void NetworkManagement_DoRediscoveryStop(sNetworkManagement *spNetworkMan);

/*===================== NetworkManagement_CheckRepeater =====================
** Function description
**      Check if a repeater is neighbor to the destination and if it has 
**      a Green neighbor or is neighbor to the controller
** Side effects:
**		
**--------------------------------------------------------------------------*/
bool NetworkManagement_CheckRepeater(BYTE bRepNodeID, BYTE bDestNodeID, BYTE bMyNodeId, sNetworkManagement *spNetworkMan);

/* Network Management - Network Health Maintenance Functionality */

/* Should be called regularly when Network are in Maintenance/Runtime mode */
bool NetworkManagement_NetworkHealth_Maintenance();

/* Should be called to initiate Maintenance/Runtime mode */
bool NetworkManagement_NetworkHealth_MaintenanceStart();

/* Should be called to stop Maintenance/Runtime mode */
bool NetworkManagement_NetworkHealth_MaintenanceStop();

/* Sends testframe to bNodeID and updates the potential existing Network Health transmit metrics */
/* Calls the callback function CompletedFunc with the transmission status */
/* Function Return value TRUE if transmission put in queue in Z-Wave module */
/* Function Return value FALSE if transmission could NOT be put in queue in Z-Wave module */
BYTE NetworkManagement_NH_TestConnection(BYTE bNodeID, void (*CompletedFunc)(BYTE bStatus, TX_STATUS_TYPE *psTxInfo));

/* When Transmitting to bNodeID the potential existing Network Health transmit metrics for bNodeID is updated */
/* and node is marked as needing a Network Health recalculation - Should be handled by the Maintenance functionality */
BYTE NetworkManagement_ZW_SendData(BYTE bNodeID, BYTE *pData, BYTE bDataLength, void (__cdecl *completedFunc)(BYTE, TX_STATUS_TYPE *));

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
NextNetworkHealthTestRound();

/*=================== NetworkManagement_UpdateTrafficLight ===================
** Function description
**      Update Traffic Light indicator in structure pointed to by spNodeStat
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
NetworkManagement_UpdateTrafficLight(sNetworkManagement_Stat *spNodeStats);
#endif
