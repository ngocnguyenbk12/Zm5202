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
 * Description:		Interface for the CSerialAPI class
 *
 * Last Changed By:  $Author$: 
 * Revision:         $Rev$: 
 * Last Changed:     $Date$: 
 * 
 ****************************************************************************/

#if !defined(AFX_SERIALAPI_H__58BB7480_D824_11D5_B2F8_000102F362B5__INCLUDED_)
#define AFX_SERIALAPI_H__58BB7480_D824_11D5_B2F8_000102F362B5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ZW_classcmd.h"


// Defines
#define CMDBUF_CMDCLASS_OFFSET 0
#define CMDBUF_CMD_OFFSET 1
#define CMDBUF_PARM1_OFFSET 2
#define CMDBUF_PARM2_OFFSET 3
#define CMDBUF_PARM3_OFFSET 4
#define CMDBUF_PARM4_OFFSET 5
#define CMDBUF_PARM5_OFFSET 6
#define CMDBUF_PARM6_OFFSET 7

/* Flags used by capabilities byte in SerialAPI_GetInitData */
#define GET_INIT_DATA_FLAG_SLAVE_API		0x01
#define GET_INIT_DATA_FLAG_TIMER_SUPPORT	0x02
#define GET_INIT_DATA_FLAG_SECONDARY_CTRL   0x04
#define GET_INIT_DATA_FLAG_IS_SUC			0x08

/* Defined libraries - Received when calling ZW_Version */
#define ZW_LIB_CONTROLLER_STATIC  0x01
#define ZW_LIB_CONTROLLER         0x02
#define ZW_LIB_SLAVE_ENHANCED     0x03
#define ZW_LIB_SLAVE              0x04
#define ZW_LIB_INSTALLER          0x05
#define ZW_LIB_SLAVE_ROUTING      0x06
#define ZW_LIB_CONTROLLER_BRIDGE  0x07
#define ZW_LIB_DUT                0x08

/* Defines for ZW_GetControllerCapabilities */
#define CONTROLLER_CAPABILITIES_IS_SECONDARY                 0x01
#define CONTROLLER_CAPABILITIES_ON_OTHER_NETWORK             0x02
#define CONTROLLER_CAPABILITIES_NODEID_SERVER_PRESENT        0x04
#define CONTROLLER_CAPABILITIES_IS_REAL_PRIMARY              0x08
#define CONTROLLER_CAPABILITIES_IS_SUC                       0x10
#define CONTROLLER_CAPABILITIES_NO_NODES_INCUDED             0x20

/*remote status mask defines*/
#define REMOTE_STATUS_SLAVE_BIT        0x01  /*1 indicates slave*/

/* SetLearnNodeState parameter */
/*#define LEARN_NODE_STATE_OFF    0
#define LEARN_NODE_STATE_NEW    1
#define LEARN_NODE_STATE_UPDATE 2
#define LEARN_NODE_STATE_DELETE 3
*/

// Mode parameters to ZW_AddNodeToNetwork
#define ADD_NODE_ANY         0x01
#define ADD_NODE_CONTROLLER  0x02
#define ADD_NODE_SLAVE       0x03
#define ADD_NODE_EXISTING    0x04
#define ADD_NODE_STOP        0x05
#define ADD_NODE_STOP_FAILED 0x06

#define ADD_NODE_MODE_MASK                   0x0F
#define ADD_NODE_OPTION_HIGH_POWER           0x80
#define ADD_NODE_OPTION_NETWORK_WIDE         0x40

// Callback states from ZW_AddNodeToNetwork
#define ADD_NODE_STATUS_LEARN_READY        0x01
#define ADD_NODE_STATUS_NODE_FOUND         0x02
#define ADD_NODE_STATUS_ADDING_SLAVE       0x03
#define ADD_NODE_STATUS_ADDING_CONTROLLER  0x04
#define ADD_NODE_STATUS_PROTOCOL_DONE      0x05
#define ADD_NODE_STATUS_DONE               0x06
#define ADD_NODE_STATUS_FAILED             0x07

#define CONTROLLER_CHANGE_START       ADD_NODE_CONTROLLER
#define CONTROLLER_CHANGE_STOP        ADD_NODE_STOP
#define CONTROLLER_CHANGE_STOP_FAILED ADD_NODE_STOP_FAILED

// Mode parameters to ZW_RemoveNodeFromNetwork 
#define REMOVE_NODE_ANY                        ADD_NODE_ANY
#define REMOVE_NODE_CONTROLLER                 ADD_NODE_CONTROLLER
#define REMOVE_NODE_SLAVE                      ADD_NODE_SLAVE
#define REMOVE_NODE_STOP                       ADD_NODE_STOP

// Callback states to ZW_RemoveNodeFrom
#define REMOVE_NODE_STATUS_LEARN_READY         ADD_NODE_STATUS_LEARN_READY
#define REMOVE_NODE_STATUS_NODE_FOUND          ADD_NODE_STATUS_NODE_FOUND
#define REMOVE_NODE_STATUS_REMOVING_SLAVE      ADD_NODE_STATUS_ADDING_SLAVE
#define REMOVE_NODE_STATUS_REMOVING_CONTROLLER ADD_NODE_STATUS_ADDING_CONTROLLER
#define REMOVE_NODE_STATUS_DONE                ADD_NODE_STATUS_DONE
#define REMOVE_NODE_STATUS_FAILED              ADD_NODE_STATUS_FAILED

/* ZW_RemoveFailedNode and ZW_ReplaceFailedNode return value definitions */
#define  NOT_PRIMARY_CONTROLLER             1 /* The removing process was */
                                              /* aborted because the controller */
                                              /* is not the primary one */

#define  NO_CALLBACK_FUNCTION               2 /* The removing process was */
                                              /* aborted because no call back */
                                              /* function is used */
#define  FAILED_NODE_NOT_FOUND              3 /* The removing process aborted */
                                              /* because the node was found */
#define  FAILED_NODE_REMOVE_PROCESS_BUSY    4 /* The removing process is busy */
#define  FAILED_NODE_REMOVE_FAIL            5 /* The removing process could not */
                                              /* be started */

#define ZW_FAILED_NODE_REMOVE_STARTED       0 /* The removing/replacing failed node process started */
#define ZW_NOT_PRIMARY_CONTROLLER           (1 << NOT_PRIMARY_CONTROLLER)
#define ZW_NO_CALLBACK_FUNCTION             (1 << NO_CALLBACK_FUNCTION)
#define ZW_FAILED_NODE_NOT_FOUND            (1 << FAILED_NODE_NOT_FOUND)
#define ZW_FAILED_NODE_REMOVE_PROCESS_BUSY  (1 << FAILED_NODE_REMOVE_PROCESS_BUSY)
#define ZW_FAILED_NODE_REMOVE_FAIL          (1 << FAILED_NODE_REMOVE_FAIL)


/* ZW_RemoveFailedNode and ZW_ReplaceFailedNode callback status definitions */
#define ZW_NODE_OK                          0 /* The node is working properly (removed from the failed nodes list ) */

/* ZW_RemoveFailedNode callback status definitions */
#define ZW_FAILED_NODE_REMOVED              1 /* The failed node was removed from the failed nodes list */
#define ZW_FAILED_NODE_NOT_REMOVED          2 /* The failed node was not removed from the failing nodes list */

/* ZW_ReplaceFailedNode callback status definitions */
#define ZW_FAILED_NODE_REPLACE              3 /* The failed node are ready to be replaced and controller */
                                              /* is ready to add new node with nodeID of the failed node */
#define ZW_FAILED_NODE_REPLACE_DONE         4 /* The failed node has been replaced */
#define ZW_FAILED_NODE_REPLACE_FAILED       5 /* The failed node has not been replaced */


// SetLearnMode callback status
#define LEARN_MODE_STARTED                 ADD_NODE_STATUS_LEARN_READY
#define LEARN_MODE_DONE                    ADD_NODE_STATUS_DONE
#define LEARN_MODE_FAILED                  ADD_NODE_STATUS_FAILED
#define LEARN_MODE_DELETED                 0x80

// SetLearnNodeState callback status 
#define LEARN_STATE_ROUTING_PENDING    0x21
#define LEARN_STATE_DONE               0x22
#define LEARN_STATE_FAIL               0x20

// idleLearnState callback status
#define UPDATE_STATE_NODE_INFO_RECEIVED     0x84
#define UPDATE_STATE_NODE_INFO_REQ_DONE     0x82
#define UPDATE_STATE_NODE_INFO_REQ_FAILED   0x81
#define UPDATE_STATE_ROUTING_PENDING  0x80
#define UPDATE_STATE_ADD_DONE         0x40       
#define UPDATE_STATE_DELETE_DONE      0x20 
#define UPDATE_STATE_SUC_ID           0x10


/* ZW_NewController parameter */
#define NEW_CTRL_STATE_SEND				0x01
#define NEW_CTRL_STATE_RECEIVE			0x02
#define NEW_CTRL_STATE_STOP_OK			0x03
#define NEW_CTRL_STATE_STOP_FAIL		0x04
#define NEW_CTRL_STATE_ABORT			0x05
#define NEW_CTRL_STATE_CHANGE			0x06
#define NEW_CTRL_STATE_DELETE			0x07
#define NEW_CTRL_STATE_MAKE_NEW         0x08

// NewController callback status 
#define NEW_CONTROLLER_FAILED			0x00
#define NEW_CONTROLLER_DONE				0x01
#define NEW_CONTROLLER_LEARNED			0x02
#define NEW_CONTROLLER_CHANGE_DONE		0x03
#define NEW_CONTROLLER_DELETED			0x04
#define NEW_CONTROLLER_MAKE_NEW_DONE    0x05

/* Listening bit in NODEINFO capability */
#define NODEINFO_LISTENING_MASK     0x80
#define NODEINFO_LISTENING_SUPPORT  0x80
/* Routing bit in the NODEINFO capability byte */
#define NODEINFO_ROUTING_SUPPORT            0x40

/* Beam wakeup mode type bits in the NODEINFO security byte */
#define NODEINFO_ZWAVE_SENSOR_MODE_WAKEUP_1000   0x40
#define NODEINFO_ZWAVE_SENSOR_MODE_WAKEUP_250    0x20

#define ZW_SUC_SET_SUCCEEDED    0x05
#define ZW_SUC_SET_FAILED       0x06

// Received frame status flags
// Received Z-Wave frame status flags - as received in ApplicationCommandHandler
#define RECEIVE_STATUS_ROUTED_BUSY  0x01
// Received Z-Wave frame status flag telling that received frame was sent with low power
#define RECEIVE_STATUS_LOW_POWER    0x02
// Received Z-Wave frame status mask used to mask Z-Wave frame type flag bits out
#define RECEIVE_STATUS_TYPE_MASK    0x1C
// Receíved Z-wave Received frame is singlecast frame - (status == xxxx00xx)
#define RECEIVE_STATUS_TYPE_SINGLE  0x00
// Receíved Z-wave Received frame is broadcast frame - (status == xxxx01xx)
#define RECEIVE_STATUS_TYPE_BROAD   0x04
// Receíved Z-wave Received frame is multicast frame - (status == xxxx10xx)
#define RECEIVE_STATUS_TYPE_MULTI   0x08
// Received Z-wave Received frame is explorer frame - (statux == xxx100xx)
#define RECEIVE_STATUS_TYPE_EXPLORE 0x10

typedef union _convertFloatByteAr_
{
	float fVal;
	unsigned char baVal[4];
} convertFloatByteAr;

typedef struct _NODE_TYPE_
{
   BYTE basic;
   BYTE generic;
   BYTE specific;
} NODE_TYPE;

typedef struct _APPL_NODE_TYPE_
{
   BYTE generic;
   BYTE specific;
} APPL_NODE_TYPE;

/* Node info stored within the non-volatile memory */
/* This are the first (protocol part) payload bytes from the Node Infomation frame */
typedef struct _NODEINFO_
{
   BYTE  capability;     /* Network capabilities */
   BYTE  security;       /* Network security */
   BYTE  reserved;
   NODE_TYPE nodeType;   /* Basic, Generic and Specific Device Types */
} NODEINFO;

typedef struct _CLOCK_
{
   BYTE  weekday;  /* Weekday 1:monday...7:sunday   */
   BYTE  hour;     /* Hour 0...23                   */
   BYTE  minute;   /* Minute 0...59                 */
} CLOCK;

typedef struct _RTC_TIMER_
{
   BYTE  status;         /* Timer element status                                  */
   CLOCK timeOn;         /* Weekday, all days, work days or weekend callback time */
   CLOCK timeOff;        /*  , Hour callback time                                 */
                         /*  , Minute callback time                               */
   BYTE  repeats;        /* Number of callback times (-1: forever)                */
   void (*func)(BYTE status, BYTE parm);   /* Timer function address              */
   BYTE  parm;           /* Parameter that is returned to the timer function      */
} RTC_TIMER;    

/* RTC_TIMER status */
#define RTC_STATUS_FIRED    0x40    /* Set when the "on" timer callback function is called */
                                    /* Cleared when the "off" timer callback function is called */
#define RTC_STATUS_APPL_MASK 0x0F   /* Bits reserved for the Application programs */
                                    /*   The appl. bits are stored when the timer is created */                

/* day definitions */
#define RTC_ALLDAYS   0
#define RTC_MONDAY    1
#define RTC_TUESDAY   2
#define RTC_WEDNESDAY 3
#define RTC_THURSDAY  4
#define RTC_FRIDAY    5 
#define RTC_SATURDAY  6
#define RTC_SUNDAY    7
#define RTC_WORKDAYS  8
#define RTC_WEEKEND   9


/* From ZW_transport.h --- begin */
/* Transmit frame option flags */
#define TRANSMIT_OPTION_ACK         0x01   /* request acknowledge from destination node */
#define TRANSMIT_OPTION_LOW_POWER   0x02   /* transmit at low output power level (1/3 of normal RF range)*/  
#define TRANSMIT_OPTION_AUTO_ROUTE  0x04   /* request retransmission via repeater nodes */
/* do not use response route - Even if available */
#define TRANSMIT_OPTION_NO_ROUTE      0x10

/* Use explore frame if needed */
#define TRANSMIT_OPTION_EXPLORE       0x20

/* Transmit frame option flag which are valid when sending explore frames  */
#define TRANSMIT_EXPLORE_OPTION_ACK         TRANSMIT_OPTION_ACK
#define TRANSMIT_EXPLORE_OPTION_LOW_POWER   TRANSMIT_OPTION_LOW_POWER

/* Received frame status flags */
#define RECEIVE_STATUS_ROUTED_BUSY    0x01
#define RECEIVE_STATUS_LOW_POWER	    0x02  /* received at low output power level */
#define RECEIVE_STATUS_TYPE_MASK      0x1C  /* Mask for masking out the received frametype bits */
#define RECEIVE_STATUS_TYPE_SINGLE    0x00  /* Received frame is singlecast frame (rxOptions == xxxx00xx) */
#define RECEIVE_STATUS_TYPE_BROAD     0x04  /* Received frame is broadcast frame  (rxOptions == xxxx01xx) */
#define RECEIVE_STATUS_TYPE_MULTI     0x08  /* Received frame is multiecast frame (rxOptions == xxxx10xx) */
#define RECEIVE_STATUS_FOREIGN_FRAME  0x40  /* Received frame is not send to me (useful only in promiscuous mode) */
#define RECEIVE_STATUS_TYPE_EXPLORE   0x10  /* Received frame is an explore frame */


/* Predefined Node ID's */
#define NODE_BROADCAST    0xFF    /* broadcast */

/* Transmit complete codes */
#define TRANSMIT_COMPLETE_OK      0x00  
#define TRANSMIT_COMPLETE_NO_ACK  0x01  /* retransmission error */
#define TRANSMIT_COMPLETE_FAIL    0x02  /* transmit error */ 
/*#ifdef ZW_CONTROLLER*/
/* Assign route transmit complete but no routes was found */
#define TRANSMIT_COMPLETE_NOROUTE 0x04  /* no route found in assignroute */
                                        /* therefore nothing was transmitted */
/*#endif*/
#define TRANSMIT_COMPLETE_HOP_0_FAIL  0x05
#define TRANSMIT_COMPLETE_HOP_1_FAIL  0x06
#define TRANSMIT_COMPLETE_HOP_2_FAIL  0x07
#define TRANSMIT_COMPLETE_HOP_3_FAIL  0x08
#define TRANSMIT_COMPLETE_HOP_4_FAIL  0x09

/* Max hops in route */
#define TRANSMIT_ROUTED_ATTEMPT   0x08

/* From ZW_transport.h --- end */

#define ZW_MAX_NODES					232
#define ZW_MAX_REPEATERS				4
#define ZW_MAX_NODEMASK_LENGTH			(ZW_MAX_NODES/8)

#define ZW_MAX_NODES_IN_MULTICAST		64     /* Maximum number of nodes in a multicast */

/* HOST Power Management defines */

/* Power Management Command definitions. */
#define PM_PIN_UP_CONFIGURATION_CMD           0x01
#define PM_MODE_CONFIGURATION_CMD             0x02
#define PM_POWERUP_ZWAVE_CONFIGURATION_CMD    0x03
#define PM_POWERUP_TIMER_CONFIGURATION_CMD    0x04
#define PM_POWERUP_EXTERNAL_CONFIGURATION_CMD 0x05
#define PM_SET_POWER_MODE_CMD                 0x06
#define PM_GET_STATUS                         0x07

/* HOST Power Up reason definitions */
#define PM_WAKEUP_REASON_NONE                 0x00
#define PM_WAKEUP_REASON_EXTERNAL             0x01
#define PM_WAKEUP_REASON_RF_ALL               0x02
#define PM_WAKEUP_REASON_RF_ALL_NO_BROADCAST  0x03
#define PM_WAKEUP_REASON_RF_MASK              0x04
#define PM_WAKEUP_REASON_TIMER_SECONDS        0x05
#define PM_WAKEUP_REASON_TIMER_MINUTES        0x06

/* Pin definitions */
#define PM_PHYSICAL_PIN_P00         0x00
#define PM_PHYSICAL_PIN_P01         0x01
#define PM_PHYSICAL_PIN_P02         0x02
#define PM_PHYSICAL_PIN_P03         0x03
#define PM_PHYSICAL_PIN_P04         0x04
#define PM_PHYSICAL_PIN_P05         0x05
#define PM_PHYSICAL_PIN_P06         0x06
#define PM_PHYSICAL_PIN_P07         0x07
#define PM_PHYSICAL_PIN_P10         0x10
#define PM_PHYSICAL_PIN_P11         0x11
#define PM_PHYSICAL_PIN_P12         0x12
#define PM_PHYSICAL_PIN_P13         0x13
#define PM_PHYSICAL_PIN_P14         0x14
#define PM_PHYSICAL_PIN_P15         0x15
#define PM_PHYSICAL_PIN_P16         0x16
#define PM_PHYSICAL_PIN_P17         0x17
#define PM_PHYSICAL_PIN_P22         0x22
#define PM_PHYSICAL_PIN_P23         0x23
#define PM_PHYSICAL_PIN_P24         0x24
#define PM_PHYSICAL_PIN_P30         0x30
#define PM_PHYSICAL_PIN_P31         0x31
#define PM_PHYSICAL_PIN_P32         0x32
#define PM_PHYSICAL_PIN_P33         0x33
#define PM_PHYSICAL_PIN_P34         0x34
#define PM_PHYSICAL_PIN_P35         0x35
#define PM_PHYSICAL_PIN_P36         0x36
#define PM_PHYSICAL_PIN_P37         0x37
#define PM_PHYSICAL_PIN_MAX         PM_PHYSICAL_PIN_P37
#define PM_PHYSICAL_PIN_UNDEFINED   0xFF

#define PM_IO_PIN_MAX               4

/* Power Management RF Wakeup modes */
#define PM_WAKEUP_UNDEFINED         0x00
#define PM_WAKEUP_ALL               0x01
#define PM_WAKEUP_ALL_NO_BROADCAST  0x02
#define PM_WAKEUP_MASK              0x03
#define PM_WAKEUP_MODE_MAX          PM_WAKEUP_MASK

#define PM_WAKEUP_MAX_BYTES         8
#define PM_MASK_DONTCARE            0

/* Power Management Timer Resolution definitions */
#define PM_TIMER_UNDEFINED          0x00
#define PM_TIMER_SECONDS            0x01
#define PM_TIMER_MINUTES            0x02
#define PM_TIMER_MODE_MAX           PM_TIMER_MINUTES

/* Z-Wave RF speed definitions */
#define ZW_RF_SPEED_NONE                        0x00
#define ZW_RF_SPEED_9600                        0x01
#define ZW_RF_SPEED_40K                         0x02
#define ZW_RF_SPEED_100K                        0x03
#define ZW_RF_SPEED_MASK                        0x07

/* ZW_GetLastWorkingRoute and ZW_SetLastWorkingRoute Route structure definitions */
#define ROUTECACHE_LINE_CONF_SIZE               1
#define ROUTECACHE_LINE_SIZE                    (ZW_MAX_REPEATERS + ROUTECACHE_LINE_CONF_SIZE)

/* LastWorkingRoute index definitions */
#define ROUTECACHE_LINE_REPEATER_1_INDEX        0
#define ROUTECACHE_LINE_REPEATER_2_INDEX        1
#define ROUTECACHE_LINE_REPEATER_3_INDEX        2
#define ROUTECACHE_LINE_REPEATER_4_INDEX        3
#define ROUTECACHE_LINE_CONF_INDEX              4

/* ZW_GetLastWorkingRoute and ZW_SetLastWorkingRoute speed definitions */
#define ZW_LAST_WORKING_ROUTE_SPEED_AUTO		ZW_RF_SPEED_NONE
#define ZW_LAST_WORKING_ROUTE_SPEED_9600        ZW_RF_SPEED_9600
#define ZW_LAST_WORKING_ROUTE_SPEED_40K         ZW_RF_SPEED_40K
#define ZW_LAST_WORKING_ROUTE_SPEED_100K        ZW_RF_SPEED_100K

/* RSSI defines used in ZW_SendData callback and ZW_GetCurrentNoiseLevel */
#define RSSI_NOT_AVAILABLE 127       /* RSSI measurement not available */
#define RSSI_MAX_POWER_SATURATED 126 /* Receiver saturated. RSSI too high to measure precisely. */
#define RSSI_BELOW_SENSITIVITY 125   /* No signal detected. The RSSI is too low to measure precisely. */
#define RSSI_RESERVED_START    11    /* All values above and including RSSI_RESERVED_START are reserved,
                                        except those defined above.*/

/* Additional tx status information. Delivered with ZW_Senddata() callback. */
typedef struct {
	WORD wTime;
	BYTE bRepeaters;
	BYTE *rssiVal;
	BYTE bACKChannelNo;
	BYTE bLastTxChannelNo;
	BYTE bRouteScheme;
	BYTE *pLastUsedRoute;
	BYTE bRouteTries;
	BYTE bLastFailedLinkFrom;
	BYTE bToLastFailedLinkTo;
} TX_STATUS_TYPE;

class CSerialAPI  
{
public:
	bool BitMask_isBitSet(BYTE* pabBitMask, BYTE bBit);
	BYTE ZW_Version(BYTE *pBuf);
	bool ZW_GetControllerCapabilities(BYTE *pbControllerCapabilities);
	void ZW_AssignReturnRoute(BYTE bSrcNodeID, BYTE bDstNodeID, void (*completedFunc)(BYTE bStatus));
	void ZW_DeleteReturnRoute(BYTE nodeID, void (*completedFunc)(BYTE bStatus));
	BYTE ZW_GetSUCNodeID(void);
	BYTE ZW_SetSUCNodeID(BYTE bNodeID, BYTE state, BYTE capabilities, BYTE txOption, void (__cdecl *complFunc)(BYTE));
	BYTE ZW_SendSUCID(BYTE bNodeID, BYTE txOption, void (__cdecl *complFunc)(BYTE));
	void ZW_AssignSUCReturnRoute(BYTE bSrcNodeID, BYTE bDstNodeID, void (*completedFunc)(BYTE bStatus));
	void ZW_DeleteSUCReturnRoute(BYTE nodeID, void (*completedFunc)(BYTE bStatus));        
	void Shutdown();
	BOOL SerialAPI_GetInitData(BYTE *ver, BYTE *capabilities, BYTE *len, BYTE *nodesList );
	void SerialAPI_PowerManagement(BYTE bPM_Cmd, BYTE cmdDataLength, BYTE *pCmdData);
	void SerialAPI_PowerManagement_PoweredUp(BYTE bIOPin);
	void SerialAPI_PowerManagement_Mode_Configuration(BYTE bNumberOfPins, BYTE *bIOPin, BYTE *bIOPinLevel);
	void SerialAPI_PowerManagement_Set_PowerMode_Configuration(BYTE bNumberOfPins, BYTE *bIOPin, BYTE *bIOPinLevel);
	void SerialAPI_PowerManagement_PowerUpOn_ZWCommand_Configuration(BYTE bMatchMode, BYTE bNumberOfMatchBytes, BYTE *bWakeUpValue, BYTE *bWakeUpMask);
	void SerialAPI_PowerManagement_PowerUpOn_External_Configuration(BYTE bIOPin, BYTE bLevel);
	void SerialAPI_PowerManagement_PowerUpOn_Timer_Configuration(BYTE bTimerResolution, WORD bTimerCount);
	BYTE SerialAPI_PowerManagement_GetStatus(void);
	void SerialAPI_ApplicationNodeInformation(BYTE  listening, APPL_NODE_TYPE nodeType, BYTE *nodeParm, BYTE parmLength );
	void ZW_SetLearnMode( BYTE mode, void (*completedFunc)(BYTE bStatus, BYTE bSource, BYTE *pCmd, BYTE bLen));
	void ZW_Clear_Tx_Timers(void);
	BYTE ZW_Get_Tx_Timers(DWORD *dwChannel0TxTimer,	DWORD *dwChannel1TxTimer, DWORD *dwChannel2TxTimer);
	void RTCTimerDelete(BYTE timerHandle);
	BYTE RTCTimerRead(BYTE *timerHandle, RTC_TIMER *timer);
	BYTE RTCTimerCreate(RTC_TIMER *timer, void (*func)(void));
	char ClockCmp(CLOCK *pTime);
	void ClockGet(CLOCK *pTime);
	BYTE ClockSet(CLOCK *pNewTime);
	void ZW_ReplicationCommandComplete();
	BYTE ZW_ReplicationSendData(BYTE nodeID, BYTE *pData, BYTE dataLength, BYTE txOptions, void (*completedFunc)(BYTE txStatus));
	BYTE ZW_ReplaceFailedNode(BYTE nodeID, void (__cdecl *completedFunc)(BYTE));
	BYTE ZW_RemoveFailedNode(BYTE nodeID, void (__cdecl *completedFunc)(BYTE));
	void ZW_AddNodeToNetwork(BYTE mode, void (__cdecl *completedFunc)(BYTE,BYTE,BYTE *,BYTE));
	void ZW_ControllerChange(BYTE mode, void (__cdecl *completedFunc)(BYTE,BYTE,BYTE *,BYTE));
	void ZW_RemoveNodeFromNetwork(BYTE mode, void (__cdecl *completedFunc)(BYTE,BYTE,BYTE *,BYTE));
	void ZW_SetDefault(void (*completedFunc)());
	void ZW_SoftReset(BYTE resetMode);
	void ZW_GetNodeProtocolInfo(BYTE bNodeID, NODEINFO *nodeInfo);
	void ZW_SetLearnNodeState(BYTE mode, void (*completedFunc)(BYTE bStatus, BYTE bSource, BYTE *pCmd, BYTE bLen));
	void ZW_RequestNetworkUpdate(void (__cdecl *completedFunc)(BYTE));
	void ZW_RequestNodeNeighborUpdate(BYTE nodeID, void (*completedFunc)(BYTE bStatus));
	void ZW_RequestNodeNeighborUpdate_Option(BYTE nodeID, BYTE bTxOption, void (__cdecl *completedFunc)(BYTE));
	BYTE MemoryPutBuffer(WORD offset, BYTE *buf, WORD length, void (*func)());
	void MemoryGetBuffer(WORD offset, BYTE *buffer, BYTE length);
	BYTE MemoryPutByte(WORD offset, BYTE data);
	BYTE MemoryGetByte(WORD offset);
	void MemoryGetID(BYTE *pHomeID, BYTE *pNodeID);
	void SerialAPI_Get_Capabilities(BYTE *capabilities);
	BYTE ZW_GetLastWorkingRoute(BYTE bNodeID, BYTE *pRoute);
	BYTE ZW_SetLastWorkingRoute(BYTE bNodeID, BYTE *pRoute);
	BYTE ZW_SetRoutingMAX(BYTE maxRouteTries);
	BYTE TimerCancel(BYTE timerHandle);
	BYTE TimerRestart(BYTE timerHandle);
	BYTE TimerStart(void (*func)(), BYTE timerTicks, BYTE repeats);
	void ZW_SetRFReceiveMode(BYTE mode);
	void ZW_SendNodeInformation(BYTE destNode, BYTE txOptions, void (*completedFunc)(BYTE txStatus));
	BYTE ZW_EnableSUC(BYTE state, BYTE capabilities);
	BOOL Initialize(BYTE byPort, BYTE bySpeed, void(*funcApplCmdHandler)(BYTE,BYTE,BYTE*,BYTE), void(*funcApplCmdHandler_Bridge)(BYTE,BYTE,BYTE,BYTE*,BYTE*,BYTE), void(*funcCommError)(BYTE));
	BYTE ZW_SendTestFrame(BYTE nodeID, BYTE bPowerLevel, void (__cdecl *completedFunc)(BYTE));
	BYTE ZW_RF_Power_Level_Set(BYTE bPowerLevel);
	BYTE ZW_RF_Power_Level_Get();
	BYTE ZW_SendData(BYTE nodeID, BYTE *pData, BYTE dataLength, BYTE txOptions, void (*completedFunc)(BYTE txStatus, TX_STATUS_TYPE *txInfo));
	BYTE ZW_SendDataMeta(BYTE nodeID, BYTE *pData, BYTE dataLength, BYTE txOptions, void (*completedFunc)(BYTE txStatus));
	BYTE ZW_SendDataMulti(BYTE *pNodeIDList, BYTE numberNodes, BYTE *pData, BYTE dataLength, BYTE txOptions, void (*completedFunc)(BYTE txStatus));
	BYTE ZW_SendDataMR(BYTE nodeID, BYTE *pData, BYTE dataLength, BYTE txOptions, BYTE pRoute[4], void (*completedFunc)(BYTE txStatus));
	BYTE ZW_SendDataMetaMR(BYTE nodeID, BYTE *pData, BYTE dataLength, BYTE txOptions, BYTE pRoute[4], void (*completedFunc)(BYTE txStatus));
	BYTE ZW_SendData_Bridge(BYTE srcNodeID, BYTE destNodeID, BYTE *pData, BYTE dataLength, BYTE txOptions, BYTE pRoute[4], void (*completedFunc)(BYTE txStatus));
	BYTE ZW_SendDataMeta_Bridge(BYTE srcNodeID, BYTE destNodeID, BYTE *pData, BYTE dataLength, BYTE txOptions, BYTE pRoute[4], void (*completedFunc)(BYTE txStatus));
	BYTE ZW_SendDataMulti_Bridge(BYTE srcNodeID, BYTE *pNodeIDList, BYTE numberNodes, BYTE *pData, BYTE dataLength, BYTE txOptions, void (*completedFunc)(BYTE txStatus));
	BYTE ZW_Support9600Only(BYTE mode);
	BYTE ZW_RequestNewRouteDestinations(BYTE pDestList, void (*completedFunc)(BYTE txStatus));
	BYTE ZW_IsNodeWithinDirectRange(BYTE bNodeID);
	BYTE ZW_ExploreRequestInclusion(void);
	void ZW_SetIdleNodeLearn(void (*completedFunc)(BYTE bStatus, BYTE bSource, BYTE *pCmd, BYTE bLen));
	void ZW_GetRoutingInfo(BYTE bNodeID, BYTE *buf, BYTE bRemoveBad, BYTE bRemoveNonReps);
	BYTE ZW_RequestNodeInfo(BYTE bNodeID, void (__cdecl *completedFunc)(BYTE));
#ifdef ZW_INSTALLER
	void ZW_ResetTXCounter(void);
	BYTE ZW_GetTXCounter(void);
#endif
	void ZW_LockRoutes(BYTE bValue);
	void ZW_WatchDogEnable(void);
	void ZW_WatchDogKick(void);
	void ZW_WatchDogDisable(void);
	void ZW_WatchDogStart(void);
	void ZW_WatchDogStop(void);
	void ZW_GetBackgroundRSSI(BYTE *buf, BYTE *bLen);
	CSerialAPI();
	virtual ~CSerialAPI();
	BOOL m_bEndThread;
	BYTE learnState;
	BOOL TimeOut;

	enum { SPEED_115200=0, SPEED_57600, SPEED_19200, SPEED_9600 };
	enum T_CommNotification { COMM_RETRY_EXCEEDED, COMM_NO_RESPONSE };

protected:
	void CommLost(BYTE byReason);
	int ReceiveData(unsigned char *buffer, BYTE *len, BOOL *bStop);
	void TransmitData(unsigned char *buffer, BYTE length);
	void SendData(unsigned char *buffer, BYTE length, BYTE *bStatus);
	BYTE CalculateChecksum(BYTE *pData, int nLength);
	void SendDataAndWaitForResponse(unsigned char *buffer, BYTE length,
											BYTE byFunc, unsigned char *out, BYTE *byLen);
	bool SendDataAndWaitForAck(unsigned char *buffer, BYTE length);
	bool WaitForAck(BYTE *bStatus);
	void WaitForResponse(BYTE byFunc, unsigned char *buffer, BYTE *byLen);
	BOOL GetTimeOut(LARGE_INTEGER timerVal);
	LARGE_INTEGER SetTimeOut(WORD timeOut10msec);
	void Dispatch(unsigned char *pData, BYTE bylen);

private:
	friend UINT APIThreadProc( LPVOID pParam);
	friend UINT DispatchThreadProc( LPVOID pParam);
};

#endif // !defined(AFX_SERIALAPI_H__58BB7480_D824_11D5_B2F8_000102F362B5__INCLUDED_)
