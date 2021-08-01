/****************************************************************************
 *
 * Z-Wave, the wireless language.
 * 
 * Copyright (c) 2012
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 * This source file is subject to the terms and conditions of the
 * Sigma Designs Software License Agreement which restricts the manner
 * in which it may be used.
 *
 *---------------------------------------------------------------------------
 *
 * Description:      Z-Wave Network Health Toolbox using Serial API.
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
#include <conio.h>
#include <time.h>
#include "IMAtoolbox.h"
#include "logging.h"
#include "networkManagement.h"
#include "powerLevelTest.h"
#include "powLevTestMan.h"
#include "timing.h"
#include "ZW_SerialAPI.h"
#include "RssiMap.h"
#include "RssiTest.h"

/* We wait max 8 sec for a Version Report */
#define GETVERSIONREPORTTIMEOUT	8

CSerialAPI api;

sNetworkManagement sNetworkManagementUT;
CTime* pTimer;
int timerH = 0;

CTime *pGetVersionReportTimer;
int timerGetVersionReportH = 0;

unsigned int bNodeID = 1;

S_SERIALAPI_CAPABILITIES sController_SerialAPI_Capabilities;
BYTE bSerialAPIVer = 0;
BYTE bDeviceCapabilities = 0;
BYTE bNodeExistMaskLen = 0;
BYTE bNodeExistMask[ZW_MAX_NODEMASK_LENGTH];
BYTE bControllerCapabilities;
BYTE MySystemSUCID;

BYTE nodeType[ZW_MAX_NODES];
BYTE nodeList[ZW_MAX_NODES];
BYTE nodeListSize;
BYTE abListeningNodeList[232];
BYTE bListeningNodeListSize;
bool afIsNodeAFLiRS[232];
BYTE abPingFailed[232];
BYTE abPingFailedSize;
BYTE abGetReportFailed[232];
BYTE abGetReportFailedSize;
BYTE bPingNodeIndex;
BYTE bNetworkRepeaterCount;
BYTE lastLearnedNodeID;
BYTE lastLearnedNodeType;
BYTE lastRemovedNodeID;
BYTE lastRemovedNodeType;
BOOL learnMode_status = false;
BYTE MyNodeId;
BYTE MySystemHomeId[4];
BYTE bLWRLocked = false;

BOOL bAddRemoveNodeFromNetwork = false;

BOOL testStarted = false;
BYTE testMode = 0;

static BYTE pBasicBuf[64];
char bCharBuf[1024] = "";

static time_t startTime;
static time_t endTime;

bool printIncomming = true;

bool screenLog = false;

CLogging mlog;

/* logging parameters */
char main_logfilenameStr[20];
char debug_logfilenameStr[20];

sLOG *mainlog;
sLOG *debuglog;

void GetVersionReportTimeout(int H);

void RequestNextNodeInformation(int H);
void DoRequestForNodeInformation(BYTE bNextIndex);
bool NetworkHealthStart();



/*==================================  mainlog_wr =============================
** Function description
**      mlog.Write wrapper for writing to mainlog
**
** Side effects:
**		
**--------------------------------------------------------------------------*/
void 
mainlog_wr(
    const char * fmt, ...)
{
    va_list args;
    char *buffer;
    int len;

    va_start(args, fmt);
    /* Remember terminating ZERO */
    len = _vscprintf(fmt, args) + 1;
    buffer = new char[len*sizeof(char)];
    vsprintf_s(buffer, len, fmt, args);
    va_end(args);
    /* Add logline to mainlog LOG and echo logline to screen */
    mlog.AddLine(mainlog, buffer, TRUE);
    delete(buffer);
}


/*==================================  debuglog_wr ============================
** Function description
**      mlog.Write wrapper for writing to debuglog
**
** Side effects:
**		
**--------------------------------------------------------------------------*/
void 
debuglog_wr(
    const char * fmt, ...)
{
    va_list args;
    char *buffer;
    int len;

    va_start(args, fmt);
    /* Remember terminating ZERO */
    len = _vscprintf(fmt, args) + 1;
    buffer = new char[len*sizeof(char)];
    vsprintf_s(buffer, len, fmt, args);
    va_end(args);
    /* Add logline to mainlog LOG and echo logline to screen according to screenLog */
    mlog.AddLine(debuglog, buffer, screenLog);
    delete(buffer);
}


/*==============================   GetValidLibTypeStr ========================
** Function description
**      Returns pointer to string describing the library type if libType is a
**	    valid Z-Wave (ZW_LIB_...) library type
**		Returns pointer to empty string ("") if libType is NOT a 
**		valid Z-Wave (ZW_LIB_...) library type
**
** Side effects:
**		
**--------------------------------------------------------------------------*/
char*
GetValidLibTypeStr(
	BYTE libType)
{
	char *pRetStr;
	switch (libType)
	{
		case ZW_LIB_CONTROLLER_STATIC:
			{
				pRetStr = "Controller Static";
			}
			break;

		case ZW_LIB_CONTROLLER_BRIDGE:
			{
				pRetStr = "Controller Bridge";
			}
			break;

		case ZW_LIB_CONTROLLER:
			{
				pRetStr = "Controller Portable";
			}
			break;

		case ZW_LIB_SLAVE_ENHANCED:
			{
				pRetStr = "Slave Enhanced";
			}
			break;  

		case ZW_LIB_SLAVE_ROUTING:
			{
				pRetStr = "Slave Routing";
			}
			break;  

		case ZW_LIB_SLAVE:
			{
				pRetStr = "Slave";
			}
			break;  

		case ZW_LIB_INSTALLER:
			{
				pRetStr = "Controller Installer";
			}
			break;

		default:
			{
				pRetStr = "";
			}
			break;  
	}
	return pRetStr;
}


/*========================   InitializeSerialAPICapabilities =================
** Function description
**      Initialize sController_SerialAPI_Capabilities with the attached
**	module SerialAPI capabilities using api.SerialAPI_Get_Capabilities()
**		a SUC
**
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
InitializeSerialAPICapabilities(void)
{
    BYTE abController_SerialAPI_Capabilities[64];

    memset(&abController_SerialAPI_Capabilities, 0, sizeof(abController_SerialAPI_Capabilities));
    memset((BYTE*)&sController_SerialAPI_Capabilities, 0, sizeof(sController_SerialAPI_Capabilities));
    api.SerialAPI_Get_Capabilities((BYTE*)&abController_SerialAPI_Capabilities);
    /* If Application Version and Revision both are ZERO then we assume we did not get any useful information */
    if ((0 != abController_SerialAPI_Capabilities[0]) || (0 != abController_SerialAPI_Capabilities[1]))
    {
        memcpy((BYTE*)&sController_SerialAPI_Capabilities, &abController_SerialAPI_Capabilities, sizeof(sController_SerialAPI_Capabilities));
    }
}


/*============================   IdleLearnNodeState_Compl ====================
** Function description
**      Callback which is called when a node is added or removed by
**		a SUC
**
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
IdleLearnNodeState_Compl(
    BYTE bStatus, /*IN Status of learn mode*/
    BYTE bSource, /*IN Source node*/
    BYTE *pCmd,   /*IN Data*/
    BYTE bLen)    /*Length of data*/
{
    static BYTE lastLearnedNodeType;
    if (bLen)
    {
        NODEINFO nodeInfo;
        api.ZW_GetNodeProtocolInfo(bSource, &nodeInfo);
        if (nodeInfo.nodeType.basic < BASIC_TYPE_SLAVE)
        {
            lastLearnedNodeType = nodeInfo.nodeType.basic;  /*For now we store the learned basic node type*/
        }
        else
        {
            lastLearnedNodeType = nodeInfo.nodeType.generic; /* Store learned generic node type */
        }
        if (0 < bSource)
        {
            memcpy(&sNetworkManagementUT.nodeDescriptor[bSource - 1].nodeInfo.capability, &nodeInfo.capability, sizeof(sNetworkManagementUT.nodeDescriptor[bSource - 1].nodeInfo));
        }
    }

    if (bStatus == UPDATE_STATE_ROUTING_PENDING)
    {
        mainlog_wr("waiting ...");
    }
    else if (bStatus == UPDATE_STATE_ADD_DONE)
    {
        mainlog_wr("Node %u included ... type %u", bSource, lastLearnedNodeType);
    }
    else if (bStatus == UPDATE_STATE_DELETE_DONE)
    {
        mainlog_wr("Node %u removed  ... type %u", bSource, lastLearnedNodeType);
    }
    else if (bStatus == UPDATE_STATE_NODE_INFO_RECEIVED)
    {
        mainlog_wr("Node %u sends nodeinformation", bSource);
        if ((0 < bSource) && (bLen >= sizeof(sNetworkManagementUT.nodeDescriptor[bSource - 1].nodeType)))
        {
            memcpy(&sNetworkManagementUT.nodeDescriptor[bSource - 1].nodeType.basic, pCmd, sizeof(sNetworkManagementUT.nodeDescriptor[bSource - 1].nodeType));
            memcpy(sNetworkManagementUT.nodeDescriptor[bSource - 1].cmdClasses, (BYTE *)(pCmd + sizeof(sNetworkManagementUT.nodeDescriptor[bSource - 1].nodeType)), bLen - sizeof(sNetworkManagementUT.nodeDescriptor[bSource - 1].nodeType));
            if (0 != timerH)
            {
                pTimer->Kill(timerH);
                timerH = 0;
                RequestNextNodeInformation(0);
            }
        }
    }
    else
    {
        mainlog_wr("Node %u sends %02x", bSource, *pCmd);
    }
}


void
ApplicationCommandHandler(
    BYTE rxStatus, 
    BYTE sourceNode, 
    BYTE *pCmd, 
    BYTE cmdLength)
{
    ZW_APPLICATION_TX_BUFFER *pdata = (ZW_APPLICATION_TX_BUFFER *) pCmd;
    char *frameType;

    switch (rxStatus & RECEIVE_STATUS_TYPE_MASK)
    {
        case RECEIVE_STATUS_TYPE_SINGLE:
            frameType = "singlecast";
            break;

        case RECEIVE_STATUS_TYPE_BROAD:
            frameType = "broadcast";
            break;

        case RECEIVE_STATUS_TYPE_MULTI:
            frameType = "multicast";
            break;

        case RECEIVE_STATUS_TYPE_EXPLORE:
            frameType = "explorer";
            break;

        default:
            frameType = "Unknown frametype";
            break;
    }
    switch(*(pCmd+CMDBUF_CMDCLASS_OFFSET))
    {
        case COMMAND_CLASS_VERSION:
            {
                switch (*(pCmd+CMDBUF_CMD_OFFSET))
                {
                    case VERSION_REPORT:
                        {
							char *pLibTypeStr;
							pLibTypeStr = GetValidLibTypeStr(*(pCmd+CMDBUF_PARM1_OFFSET));
                            mainlog_wr("Node %u says VERSION REPORT (%s)", sourceNode, frameType);
                            mainlog_wr("Z-Wave %s protocol v%02u.%02u, App. v%02u.%02u", pLibTypeStr, 
																							 *(pCmd+CMDBUF_PARM2_OFFSET),
																							 *(pCmd+CMDBUF_PARM3_OFFSET),
																							 *(pCmd+CMDBUF_PARM4_OFFSET),
																							 *(pCmd+CMDBUF_PARM5_OFFSET));
                            /* If timer is running, kill it! */
                            if (0 != timerGetVersionReportH)
                            {
                                pGetVersionReportTimer->Kill(timerGetVersionReportH);
                            }
                            timerGetVersionReportH = 0;
                            /* We assume the answer we just received was the one we asked for... */
                            GetVersionReportTimeout(0);
                        }
                        break;

                    default:
                        mainlog_wr("Node %u says VERSION command %u (%s)", sourceNode, *(pCmd+CMDBUF_PARM1_OFFSET), frameType);
                        break;
                }
            }
            break;

        case COMMAND_CLASS_POWERLEVEL:
            CmdHandlerPowerLevel(rxStatus, sourceNode, pCmd, cmdLength);
            break;

        case COMMAND_CLASS_SWITCH_MULTILEVEL:
            if (printIncomming)
            {
                if(*(pCmd+CMDBUF_CMD_OFFSET)== SWITCH_MULTILEVEL_REPORT)
                {
                    mainlog_wr("Node %u says SWITCH MULTILEVEL REPORT %u (%s)", sourceNode, *(pCmd+CMDBUF_PARM1_OFFSET), frameType);
                }
                else if (*(pCmd+CMDBUF_CMD_OFFSET)== SWITCH_MULTILEVEL_SET)
                {
                    mainlog_wr("Node %u says SWITCH MULTILEVEL SET %u (%s)", sourceNode, *(pCmd+CMDBUF_PARM1_OFFSET), frameType);
                }
                else if (*(pCmd+CMDBUF_CMD_OFFSET)== SWITCH_MULTILEVEL_GET)
                {
                    mainlog_wr("Node %u says SWITCH MULTILEVEL GET (%s)", sourceNode, frameType);
                }
            }
            break;

        case COMMAND_CLASS_BASIC:
            switch (*(pCmd+CMDBUF_CMD_OFFSET))
            {
                case BASIC_REPORT:
                    {
                        if (printIncomming)
                        {
                            mainlog_wr("Node %u says BASIC REPORT %u (%s)", sourceNode, *(pCmd+CMDBUF_PARM1_OFFSET), frameType);
                        }
                    }
                    break;

                case BASIC_SET:
                    {
                        if (printIncomming)
                        {
                            mainlog_wr("Node %u says BASIC SET %u (%s)", sourceNode, *(pCmd+CMDBUF_PARM1_OFFSET), frameType);
                        }
                    }
                    break;

                case BASIC_GET:
                    {
                        if (printIncomming)
                        {
                            mainlog_wr("Node %u says BASIC GET (%s)", sourceNode, frameType);
                        }
                    }
                    break;

                default:
                    {
                        if (printIncomming)
                        {
                            mainlog_wr("Node %u says none supported BASIC command %u (%s)", sourceNode, *(pCmd+CMDBUF_PARM1_OFFSET), frameType);
                        }
                    }
                    break;
            }
            break;


        case COMMAND_CLASS_SENSOR_BINARY:
            if (*(pCmd + CMDBUF_CMD_OFFSET) == SENSOR_BINARY_REPORT)
            {
            }
            break;

        case COMMAND_CLASS_CONTROLLER_REPLICATION:
            api.ZW_ReplicationCommandComplete();
            break;

        case COMMAND_CLASS_METER:
            if (*(pCmd + CMDBUF_CMD_OFFSET) == METER_REPORT)
            {
                if (printIncomming)
                {
                    mainlog_wr("Node %u says METER REPORT (%s)\n", sourceNode, frameType);
                }
            }
            break;

        default:
            if (printIncomming)
            {
                mainlog_wr("Node %u says commandclass %u, command %u (%s)", sourceNode, *(pCmd+CMDBUF_CMDCLASS_OFFSET), *(pCmd+CMDBUF_CMD_OFFSET), frameType);
            }
            break;
    }
}


void
ApplicationCommandHandler_Bridge(
    BYTE rxStatus,
    BYTE destNode,
    BYTE sourceNode, 
    BYTE *multi,
    BYTE *pCmd, 
    BYTE cmdLength)
{
    ZW_APPLICATION_TX_BUFFER *pdata = (ZW_APPLICATION_TX_BUFFER *) pCmd;
    char *frameType;

    switch (rxStatus & RECEIVE_STATUS_TYPE_MASK)
    {
        case RECEIVE_STATUS_TYPE_SINGLE:
            frameType = "virtual singlecast";
            break;

        case RECEIVE_STATUS_TYPE_BROAD:
            frameType = "virtual broadcast";
            break;

        case RECEIVE_STATUS_TYPE_MULTI:
            frameType = "virtual multicast";
            break;

        default:
            frameType = "virtual Unknown frametype";
            break;
    }
    mainlog_wr("Node %u says to %u - %u, %u (%s)", sourceNode, destNode, *(pCmd+CMDBUF_CMDCLASS_OFFSET), *(pCmd+CMDBUF_PARM1_OFFSET), frameType);
    if ((NULL != multi) && (0 < multi[0]))
    {
        sprintf_s(bCharBuf, sizeof(bCharBuf), " Nodemask: ");
        for (int i = 1; i <= multi[0]; i++)
        {
            sprintf_s(&bCharBuf[strlen(bCharBuf)], sizeof(bCharBuf)-strlen(bCharBuf),"%0X ", multi[i]);
        }
        mainlog_wr(bCharBuf);
        sprintf_s(bCharBuf, sizeof(bCharBuf), "Destination :");
        int lNodeID = 1;
        int bitmask;
        for (int i = 1; i <= multi[0]; i++)
        {
            bitmask = 0x01;
            for (int j = 1; j <= 8; j++, bitmask <<= 1)
            {
                if (bitmask & multi[i])
                {
                    sprintf_s(&bCharBuf[strlen(bCharBuf)], sizeof(bCharBuf)-strlen(bCharBuf), " %u", lNodeID);
                }
                lNodeID++;
            }
        }
        mainlog_wr(bCharBuf);
    }
    mainlog_wr("* ");
    switch (*(pCmd + CMDBUF_CMDCLASS_OFFSET))
    {
        case COMMAND_CLASS_SWITCH_MULTILEVEL:
            if (*(pCmd + CMDBUF_CMD_OFFSET) == SWITCH_MULTILEVEL_REPORT)
            {
                mainlog_wr("SWITCH MULTILEVEL REPORT %02X", sourceNode, *(pCmd + CMDBUF_PARM1_OFFSET));
            }
            else if (*(pCmd + CMDBUF_CMD_OFFSET)== SWITCH_MULTILEVEL_SET)
            {
                mainlog_wr("SWITCH MULTILEVEL SET %02X", sourceNode, *(pCmd + CMDBUF_PARM1_OFFSET));
            }
            else if (*(pCmd + CMDBUF_CMD_OFFSET) == SWITCH_MULTILEVEL_GET)
            {
                mainlog_wr("SWITCH MULTILEVEL GET", sourceNode);
            }
            else
            {
                mainlog_wr("None supported SWITCH MULTILEVEL command %02X", *(pCmd + CMDBUF_CMD_OFFSET));
            }
            break;

        case COMMAND_CLASS_BASIC:
            switch (*(pCmd + CMDBUF_CMD_OFFSET))
            {
                case BASIC_REPORT:
                    {
                        mainlog_wr("BASIC REPORT %02X", *(pCmd + CMDBUF_PARM1_OFFSET));
                    }
                    break;

                case BASIC_SET:
                    {
                        mainlog_wr("BASIC SET %02X", *(pCmd + CMDBUF_PARM1_OFFSET));
                    }
                    break;

                case BASIC_GET:
                    {
                        mainlog_wr("BASIC GET");
                    }
                    break;

                default:
                    {
                        mainlog_wr("None supported BASIC command %02X", *(pCmd + CMDBUF_CMD_OFFSET));
                    }
                    break;
            }
            break;


        case COMMAND_CLASS_SENSOR_BINARY:
            if (*(pCmd + CMDBUF_CMD_OFFSET) == SENSOR_BINARY_REPORT)
            {
                mainlog_wr("SENSOR BINARY REPORT %02X", *(pCmd + CMDBUF_PARM1_OFFSET));
            }
            else
            {
                mainlog_wr("None supported SENSOR BINARY command %02X", *(pCmd + CMDBUF_CMD_OFFSET));
            }
            break;

        case COMMAND_CLASS_CONTROLLER_REPLICATION:
            mainlog_wr("CONTROLLER REPLICATION");
            api.ZW_ReplicationCommandComplete();
            break;

        default:
            mainlog_wr("Commandclass %02X, Command %02X", *(pCmd + CMDBUF_CMDCLASS_OFFSET), *(pCmd + CMDBUF_CMD_OFFSET));
            break;
    }
}


/*============================   CommErrorNotification =======================
** Function description
**      Called when a communication error occours between Z-Wave module and
**		PC
**
** Side effects:
**		
**--------------------------------------------------------------------------*/
void 
CommErrorNotification(
    BYTE byReason)
{
    switch (byReason)
    {
        case CSerialAPI::COMM_RETRY_EXCEEDED:
            mainlog_wr("Communication with Z-Wave module was lost while sending command.\nCheck the cable and either retry the operation or restart the application.", "Error");
            break;

        case CSerialAPI::COMM_NO_RESPONSE:
            mainlog_wr("Communication with Z-Wave module timed out while waiting for response.\nCheck the cable and either retry the operation or restart the application.", "Error");
            break;

        default:
            mainlog_wr("Communication with Z-Wave module was lost.\nCheck the cable and either retry the operation or restart the application.", "Error");
            break;
    }
}


/*============================   WriteNodeTypeString ========================
** Function description
**      Returns a CString containing a fitting name to the nodetype supplied
** Side effects:
**		
**--------------------------------------------------------------------------*/
char* 
WriteNodeTypeString(
    BYTE nodeType)
{
    switch (nodeType)
    {
        case GENERIC_TYPE_GENERIC_CONTROLLER:
            return "Generic Controller";

        case GENERIC_TYPE_STATIC_CONTROLLER:
            return "Static Controller";

        case GENERIC_TYPE_REPEATER_SLAVE:
            return "Repeater Slave";

        case GENERIC_TYPE_SWITCH_BINARY:
            return "Binary Switch";

        case GENERIC_TYPE_SWITCH_MULTILEVEL:
            return "Multilevel Switch";

		case GENERIC_TYPE_SWITCH_REMOTE:
			return "Remote Switch";

		case GENERIC_TYPE_SWITCH_TOGGLE:
			return "Toggle Switch";

        case GENERIC_TYPE_SENSOR_BINARY:
            return "Binary Sensor";

		case GENERIC_TYPE_SENSOR_MULTILEVEL:
			return "Sensor Multilevel";

		case GENERIC_TYPE_SENSOR_ALARM:
			return "Sensor Alarm";

		case GENERIC_TYPE_METER:
			return "Meter";

        case GENERIC_TYPE_METER_PULSE:
            return "Pulse Meter";

        case GENERIC_TYPE_ENTRY_CONTROL:
            return "Entry Control";

		case GENERIC_TYPE_AV_CONTROL_POINT:
			return "AV Control Point";

		case GENERIC_TYPE_DISPLAY:
			return "Display";

		case GENERIC_TYPE_SEMI_INTEROPERABLE:
			return "Semi Interoperable";

		case GENERIC_TYPE_NON_INTEROPERABLE:
			return "Non Interoperable";

		case GENERIC_TYPE_THERMOSTAT:
			return "Thermostat";

		case GENERIC_TYPE_VENTILATION:
			return "Ventilation";

		case GENERIC_TYPE_WINDOW_COVERING:
			return "Window Covering";

		case GENERIC_TYPE_SECURITY_PANEL:
			return "Security Panel";

		case GENERIC_TYPE_WALL_CONTROLLER:
			return "Wall Controller";

		case GENERIC_TYPE_APPLIANCE:
			return "Appliance";

		case GENERIC_TYPE_SENSOR_NOTIFICATION:
			return "Sensor Notification";

		case GENERIC_TYPE_NETWORK_EXTENDER:
			return "Network Extender";

		case GENERIC_TYPE_ZIP_NODE:
			return "Zip Node";

        case 0:			/* No Nodetype registered */
            return "No device";

        default:
            return "Unknown Device type";
    }
}


/*============================ SetLearnNodeStateDelete_Compl ================
** Function description
**      Callback function for when deleting a node
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
SetLearnNodeStateDelete_Compl(
    BYTE bStatus, 
    BYTE bSource, 
    BYTE *pCmd, 
    BYTE bLen)
{
    switch(bStatus)
    {
        case REMOVE_NODE_STATUS_LEARN_READY:
            mainlog_wr("RNN: [REMOVE_NODE_STATUS_LEARN_READY(%02X)] - Waiting... (Nodeinformation length %u)", bStatus, bLen);
            break;

        case REMOVE_NODE_STATUS_NODE_FOUND:
            mainlog_wr("RNN: [REMOVE_NODE_STATUS_NODE_FOUND(%02X)]  - Node found %03u (Nodeinformation length %u)", bStatus, bSource, bLen);
            break;

        case REMOVE_NODE_STATUS_REMOVING_SLAVE:
            {
                NODE_TYPE node_Type = *((NODE_TYPE*)pCmd);
                if (bLen == 0)
                {
                    lastRemovedNodeType = 0;
                }
                else
                {
                    if (node_Type.basic < BASIC_TYPE_SLAVE)
                    {
                        lastRemovedNodeType = node_Type.basic;  /*For now we store the learned basic node type*/
                    }
                    else
                    {
                        lastRemovedNodeType = node_Type.generic; /* Store learned generic node type*/
                    }
                }
                lastRemovedNodeID = bSource;
                mainlog_wr("RNN: [REMOVE_NODE_STATUS_REMOVING_SLAVE(%02X)]  - Removing Slave Node %03u (%s)... (Nodeinformation length %u)", bStatus, bSource, WriteNodeTypeString(lastRemovedNodeType), bLen);
                if (bSource)
                {
                    nodeType[bSource - 1] = 0;
                }
                api.ZW_AddNodeToNetwork(REMOVE_NODE_STOP, SetLearnNodeStateDelete_Compl);
            }
            break;

        case REMOVE_NODE_STATUS_REMOVING_CONTROLLER:
            {
                NODE_TYPE node_Type = *((NODE_TYPE*)pCmd);
                if (bLen == 0)
                {
                    lastRemovedNodeType = 0;
                }
                else
                {
                    if (node_Type.basic < BASIC_TYPE_SLAVE)
                    {
                        lastRemovedNodeType = node_Type.basic;  /* For now we store the learned basic node type */
                    }
                    else
                    {
                        lastRemovedNodeType = node_Type.generic; /* Store learned generic node type*/
                    }
                }
                lastRemovedNodeID = bSource;
                mainlog_wr("RNN: [REMOVE_NODE_STATUS_REMOVING_CONTROLLER(%02X)]  - Removing Controller Node %03u (%s)... (Nodeinformation length %u)", bStatus, bSource, WriteNodeTypeString(lastRemovedNodeType), bLen);
                if (bSource)
                {
                    nodeType[bSource - 1] = 0;
                }
                api.ZW_AddNodeToNetwork(REMOVE_NODE_STOP, SetLearnNodeStateDelete_Compl);
            }
            break;

        case REMOVE_NODE_STATUS_DONE:
            {
                NODE_TYPE node_Type = *((NODE_TYPE*)pCmd);
                if (bLen == 0)
                {
                    lastRemovedNodeType = 0;
                }
                else
                {
                    if (node_Type.basic < BASIC_TYPE_SLAVE)
                    {
                        lastRemovedNodeType = node_Type.basic;  /*For now we store the learned basic node type*/
                    }
                    else
                    {
                        lastRemovedNodeType = node_Type.generic; /* Store learned generic node type*/
                    }
                }
                lastRemovedNodeID = bSource;
                mainlog_wr("RNN: [REMOVE_NODE_STATUS_DONE(%02X)]  - Removing Node %03u (%s(%02X))... (Nodeinformation length %u)", bStatus, bSource, WriteNodeTypeString(lastRemovedNodeType), lastRemovedNodeType, bLen);
                if (bSource)
                {
                    nodeType[bSource - 1] = 0;
                }
                /* TO#3932 fix */
                bAddRemoveNodeFromNetwork = false;
                api.ZW_AddNodeToNetwork(REMOVE_NODE_STOP, NULL);
            }
            break;

        case REMOVE_NODE_STATUS_FAILED:
            {
                mainlog_wr("RNN: [REMOVE_NODE_STATUS_FAILED(%02X)]  - Remove Node failed... (Nodeinformation length %u)", bStatus, bLen);
                bAddRemoveNodeFromNetwork = false;
                api.ZW_AddNodeToNetwork(REMOVE_NODE_STOP, NULL);
            }
            break;

        default:
            {
                /* If were not pending, we want to turn off learn mode... */
                mainlog_wr("RNN: [%02X]  - Unknown status, stopping... (Nodeinformation length %u)", bStatus, bLen);
                bAddRemoveNodeFromNetwork = false;
                api.ZW_AddNodeToNetwork(REMOVE_NODE_STOP, NULL);
            }
            break;
    }
}


/*============================ SetLearnNodeState_Compl ======================
** Function description
**      Called when a new node have been added
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
SetLearnNodeState_Compl(
    BYTE bStatus, /*IN Status of learn process*/
    BYTE bSource, /*IN Node ID of node learned*/
    BYTE *pCmd,	  /*IN Pointer to node information*/
    BYTE bLen)	  /*IN Length of node information*/
{
    switch(bStatus)
    {
        case ADD_NODE_STATUS_LEARN_READY:
            mainlog_wr("ANN: [ADD_NODE_STATUS_LEARN_READY(%02X)]      - Press node button... (Nodeinformation length %u)", bStatus, bLen);
            break;

        case ADD_NODE_STATUS_NODE_FOUND:
            mainlog_wr("ANN: [ADD_NODE_STATUS_NODE_FOUND(%02X)]       - Waiting ... (Nodeinformation length %u)", bStatus, bLen);
            break;

        case ADD_NODE_STATUS_ADDING_SLAVE:
            {
                NODE_TYPE node_Type = *((NODE_TYPE*)pCmd);
                if (bLen == 0)
                {
                    lastRemovedNodeType = 0;
                }
                else
                {
                    if (node_Type.basic < BASIC_TYPE_SLAVE)
                    {
                        lastLearnedNodeType = node_Type.basic;  /*For now we store the learned basic node type*/
                    }
                    else
                    {
                        lastLearnedNodeType = node_Type.generic; /* Store learned generic node type*/
                    }
                }
                lastLearnedNodeID = bSource;
                mainlog_wr("ANN: [ADD_NODE_STATUS_ADDING_SLAVE(%02X)]     - Adding Slave unit... Nodeinformation length %u", bStatus, bLen);
            }
            break;

        case ADD_NODE_STATUS_ADDING_CONTROLLER:
            {
                NODE_TYPE node_Type = *((NODE_TYPE*)pCmd);
                if (bLen == 0)
                {
                    lastRemovedNodeType = 0;
                }
                else
                {
                    if (node_Type.basic < BASIC_TYPE_SLAVE)
                    {
                        lastLearnedNodeType = node_Type.basic;  /*For now we store the learned basic node type*/
                    }
                    else
                    {
                        lastLearnedNodeType = node_Type.generic; /* Store learned generic node type*/
                    }
                }
                lastLearnedNodeID = bSource;
                mainlog_wr("ANN: [ADD_NODE_STATUS_ADDING_CONTROLLER(%02X)] - Adding Controller unit... Nodeinformation length %u", bStatus, bLen);
            }
            break;

        case ADD_NODE_STATUS_PROTOCOL_DONE:
            api.ZW_AddNodeToNetwork(ADD_NODE_STOP, SetLearnNodeState_Compl);
            mainlog_wr("ANN: [ADD_NODE_STATUS_PROTOCOL_DONE(%02X)]    - Now stopping, app has nothing... (Nodeinformation length %u)", bStatus, bLen);
            break;

        case ADD_NODE_STATUS_DONE:
            /* Hmm properly not needed... */
            api.ZW_AddNodeToNetwork(ADD_NODE_STOP, NULL);
            mainlog_wr("ANN: [ADD_NODE_STATUS_DONE(%02X)]             - Node included, stopping... %u - %s(%02X) (Nodeinformation length %u)", bStatus, lastLearnedNodeID, WriteNodeTypeString(lastLearnedNodeType), lastLearnedNodeType, bLen);
            nodeType[lastLearnedNodeID-1] = lastLearnedNodeType;
            bAddRemoveNodeFromNetwork = false;
            break;

        case ADD_NODE_STATUS_FAILED:
            mainlog_wr("ANN: [ADD_NODE_STATUS_FAILED(%02X)]           - AddNodeToNetwork failed, stopping... (Nodeinformation length %u)", bStatus, bLen);
            api.ZW_AddNodeToNetwork(ADD_NODE_STOP, SetLearnNodeState_Compl);
            break;

        default:
            /* If were not pending, we want to turn off learn mode.. */
            mainlog_wr("ANN: [%02X]                        - Undefined status (Nodeinformation length %u)", bStatus, bLen);
            api.ZW_AddNodeToNetwork(ADD_NODE_STOP, SetLearnNodeState_Compl);
            break;
    }
}

/*================================ rssiToText ============================
** Function description
**      Take an RSSI BYTE and return a string of text.
**
**		Note: out pointer destination must be able to hold 5 chars.
**		
**--------------------------------------------------------------------------*/
void
rssiToText(
    char rssiVal, char *out)
{
	const char max[]="Max";
	const char low[]="Low";
	const char na[]="N/A";
	const char res[]= "Res";

	if (RSSI_MAX_POWER_SATURATED == rssiVal)
	{
		memcpy(out, max, sizeof(max));
	}
	else if (RSSI_NOT_AVAILABLE == rssiVal)
	{
		memcpy(out, na, sizeof(na));
	}
	else if (RSSI_BELOW_SENSITIVITY == rssiVal)
	{
		memcpy(out, low, sizeof(low));
	}
	else if (RSSI_RESERVED_START <= rssiVal)
	{
		memcpy(out, res, sizeof(res));
	}
	else
	{
		/* If we get here, it is a signed char */
		sprintf_s(out, RSSI_TO_TEXT_SIZE, "%d", rssiVal);
	}
}

/*================================ SendData_Compl ============================
** Function description
**      Called when a SendData has been executed
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
SendData_Compl(
   BYTE bTxStatus, 
   TX_STATUS_TYPE *psTxInfo)
{
	char rssiStrings[5][5];
	int i;

    if (!bTxStatus)
    {
        mainlog_wr("SendData : Success... [%02X] TxTime = %ums", bTxStatus, psTxInfo->wTime * 10);
		if (0xFF != psTxInfo->bRepeaters)
		{
			for (i = 0; i <5; i++)
			{
				rssiToText(psTxInfo->rssiVal[i], rssiStrings[i]);
			}
			mainlog_wr("Repeaters = %02X. RSSI(dBm) =  %s %s %s %s %s.", psTxInfo->bRepeaters, 
				rssiStrings[0], rssiStrings[1], rssiStrings[2], rssiStrings[3], rssiStrings[4]);
		}
    }
    else
    {
        mainlog_wr("SendData : Failure... [%02X] TxTime = %ums", bTxStatus, psTxInfo->wTime * 10);
    }
}


/*======================== RequestNodeNeighborUpdate_Compl ===================
** Function description
**   ZW_RequestNodeNeighborUpdate callback
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
RequestNodeNeighborUpdate_Compl(
    BYTE bStatus) /*IN Status of neighbor update process*/
{
    mainlog_wr("RequestNodeNeighborUpdate_Compl [%03u]: %02X", bNodeID, bStatus);
}


/*========================= RequestNodeInfo_Compl ============================
** Function description
**   ZW_RequestNodeInfo callback called when the request transmission has 
** been tried.
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
RequestNodeInfo_Compl(
    BYTE bTxStatus)
{
    if (!bTxStatus)
    {
        mainlog_wr("Request Node Info transmitted successfully for node %u", bNodeID);
    }
    else
    {
        mainlog_wr("Request Node Info transmit failed for node %u", bNodeID);
    }
}


/*============================   SetDefault_Compl ============================
** Function description
**      Callback function called when ZW_SetDefault is done.
** Side effects:
**--------------------------------------------------------------------------*/
void
SetDefault_Compl(void)
{
    mainlog_wr("ZW_SetDefault done...");
}


/*==========================   SetLearnMode_Compl ============================
** Function description
**      Callback function called by protocol when in Receive mode
** Side effects:
**--------------------------------------------------------------------------*/
void
SetLearnMode_Compl(
    BYTE bStatus,
    BYTE bSource,
    BYTE *pCmd,
    BYTE bLen)
{
    mainlog_wr("ZW_SetLearnMode callback status %02X, source %02X, len %02X", bStatus, bSource, bLen);
    switch (bStatus)
    {
        case LEARN_MODE_STARTED:
            mainlog_wr("ZW_SetLearnMode LEARN_MODE_STARTED");
            learnMode_status = true;
            break;

        case LEARN_MODE_DONE:
            mainlog_wr("ZW_SetLearnMode LEARN_MODE_DONE");
            learnMode_status = false;
            break;

        case LEARN_MODE_FAILED:
            mainlog_wr("ZW_SetLearnMode LEARN_MODE_FAILED");
            learnMode_status = false;
            break;

        case LEARN_MODE_DELETED:
            mainlog_wr("ZW_SetLearnMode LEARN_MODE_DELETED");
            learnMode_status = false;
            break;

        default:
            mainlog_wr("ZW_SetLearnMode Unknown status");
            api.ZW_SetLearnMode(false, SetLearnMode_Compl);
            break;
    }
}


/*========================   RemoveFailedNode_Compl ==========================
** Function description
**      Callback function called by protocol when ZW_RemoveFailedNode has started
** Side effects:
**--------------------------------------------------------------------------*/
void
RemoveFailedNode_Compl(
    BYTE bStatus)
{
    mainlog_wr("ZW_RemoveFailedNode callback status %02X", bStatus);
    switch (bStatus)
    {
        case ZW_NODE_OK:
            mainlog_wr("Failed node %03d not removed	- ZW_NODE_OK", bNodeID);
            break;

        case ZW_FAILED_NODE_REMOVED:
            mainlog_wr("Failed node %03d removed		- ZW_FAILED_NODE_REMOVED", bNodeID);
            break;

        case ZW_FAILED_NODE_NOT_REMOVED:
            mainlog_wr("Failed node %03d not removed	- ZW_FAILED_NODE_NOT_REMOVED", bNodeID);
            break;

        default:
            mainlog_wr("ZW_RemoveFailedNode unknown status %d", bStatus);
            break;
    }
}


/*==============================   GetControllerCapabilityStr ================
** Function description
**      Returns pointer to string describing the attached Controller device
**		and updates the global bControllerCapabilities variable accordingly
**
** Side effects:
**		
**--------------------------------------------------------------------------*/
char *
GetControllerCapabilityStr(void)
{
	if (api.ZW_GetControllerCapabilities(&bControllerCapabilities))
	{
		if (bControllerCapabilities & CONTROLLER_CAPABILITIES_NODEID_SERVER_PRESENT)
		{
			if (bControllerCapabilities & CONTROLLER_CAPABILITIES_IS_SUC)
			{
				sprintf_s(bCharBuf, sizeof(bCharBuf), "SIS");
			}
			else
			{
				sprintf_s(bCharBuf, sizeof(bCharBuf), "Inclusion");
			}
		}
		else
		{
			if (bControllerCapabilities & CONTROLLER_CAPABILITIES_IS_SUC)
			{
				sprintf_s(bCharBuf, sizeof(bCharBuf), "SUC");
			}
			else
			{
				if (bControllerCapabilities & CONTROLLER_CAPABILITIES_IS_SECONDARY)
				{
					sprintf_s(bCharBuf, sizeof(bCharBuf), "Secondary");
				}
				else
				{
					sprintf_s(bCharBuf, sizeof(bCharBuf), "Primary");
				}
			}
		}
		sprintf_s(&bCharBuf[strlen(bCharBuf)], sizeof(bCharBuf) - strlen(bCharBuf), " Controller");
		if (0 < MySystemSUCID)
		{
			if (bControllerCapabilities & CONTROLLER_CAPABILITIES_NODEID_SERVER_PRESENT)
			{
				sprintf_s(&bCharBuf[strlen(bCharBuf)], sizeof(bCharBuf) - strlen(bCharBuf), " in a SIS network");
			}
			else
			{
				sprintf_s(&bCharBuf[strlen(bCharBuf)], sizeof(bCharBuf) - strlen(bCharBuf), " in a SUC network");
			}
		}
		if (bControllerCapabilities & CONTROLLER_CAPABILITIES_IS_REAL_PRIMARY)
		{
			sprintf_s(&bCharBuf[strlen(bCharBuf)], sizeof(bCharBuf) - strlen(bCharBuf), ", (Real primary)");
		}
		if (bControllerCapabilities & CONTROLLER_CAPABILITIES_ON_OTHER_NETWORK)
		{
			sprintf_s(&bCharBuf[strlen(bCharBuf)], sizeof(bCharBuf) - strlen(bCharBuf), ", (Other network)");
		}
		if (bControllerCapabilities & CONTROLLER_CAPABILITIES_NO_NODES_INCUDED)
		{
			sprintf_s(&bCharBuf[strlen(bCharBuf)], sizeof(bCharBuf) - strlen(bCharBuf), ", (No nodes)");
		}
	}
	else
	{
		sprintf_s(bCharBuf, sizeof(bCharBuf), "Controller Could not determine Controller Capabilities");
	}
	return bCharBuf;
}


/*============================== ReloadNodeList =============================
** Function description
**      Reloads the node IDs and types from the Device module
**		if fInitializeNetworkManagement = true then the NetworkManagement_Init
** Side effects:
**		Reinitializes the nodeType, nodeList and
**		the abListeningNodeList arrays
**
**--------------------------------------------------------------------------*/
BOOL 
ReloadNodeList(
    BOOL fInitializeNetworkManagement)
{
    BOOL bFound = FALSE;
    BYTE byNode = 1;
    BYTE j,i;
    NODEINFO nodeInfo;
    bMaxNodeID = 0;
    nodeListSize = 0;
    bListeningNodeListSize = 0;
    bNetworkRepeaterCount = 0;
    api.MemoryGetID(MySystemHomeId, &MyNodeId);
	MySystemSUCID = api.ZW_GetSUCNodeID();
	mainlog_wr("SUC ID %03u", MySystemSUCID);
    mainlog_wr("Device HomeID %02X%02X%02X%02X, NodeID %03u", 
			MySystemHomeId[0], MySystemHomeId[1], MySystemHomeId[2], MySystemHomeId[3], MyNodeId);
	memset(bNodeExistMask, 0, sizeof(bNodeExistMask));
    api.SerialAPI_GetInitData(&bSerialAPIVer, &bDeviceCapabilities, &bNodeExistMaskLen, bNodeExistMask);
	if (bDeviceCapabilities & GET_INIT_DATA_FLAG_SLAVE_API)
	{
		mainlog_wr("Device is Slave");
	}
	else
	{
		mainlog_wr("Device is %s", GetControllerCapabilityStr());
	}
    /* Reset the nodelists before loading them again */
    memset(nodeType, 0, sizeof(nodeType));
    memset(nodeList, 0, sizeof(nodeList));
    memset(abListeningNodeList, 0, sizeof(abListeningNodeList));
    memset(afIsNodeAFLiRS, 0, sizeof(afIsNodeAFLiRS));
    for (i = 0; i < bNodeExistMaskLen; i++)
    {
        if (bNodeExistMask[i] != 0)
        {
            bFound = TRUE;
            for (j = 0; j < 8; j++)
            {
                if (bNodeExistMask[i] & (1 << j))
                {
                    api.ZW_GetNodeProtocolInfo(byNode, &nodeInfo);
                    memcpy(&sNetworkManagementUT.nodeDescriptor[byNode - 1].nodeInfo, &nodeInfo, sizeof(nodeInfo));
                    nodeType[byNode] = nodeInfo.nodeType.generic;
                    nodeList[nodeListSize++] = byNode;
                    /* We want listening nodes and FLiRS nodes in networkTest list */
                    if (byNode != MyNodeId)
                    {
                        if ((nodeInfo.capability & NODEINFO_LISTENING_SUPPORT) ||
                            (0 != (nodeInfo.security & (NODEINFO_ZWAVE_SENSOR_MODE_WAKEUP_1000 | NODEINFO_ZWAVE_SENSOR_MODE_WAKEUP_250))))
                        {
                            /* Node is either a AC powered Listening node or a Beam wakeup Node */
                            abListeningNodeList[bListeningNodeListSize++] = byNode;
                            if (0 == (nodeInfo.security & (NODEINFO_ZWAVE_SENSOR_MODE_WAKEUP_1000 | NODEINFO_ZWAVE_SENSOR_MODE_WAKEUP_250)))
                            {
                                if (nodeInfo.capability & NODEINFO_ROUTING_SUPPORT)
                                {
                                    /* One more repeater - AC powered Listening nodes are counted as Repeater */
                                    bNetworkRepeaterCount++;
                                }
                            }
                            else
                            {
                                afIsNodeAFLiRS[byNode - 1] = true;
                            }
                        }
                    }
                    else
                    {
                    }
                    mainlog_wr("Node %03u %s %02X %s", 
                               byNode, (nodeInfo.capability & NODEINFO_LISTENING_SUPPORT) ? "TRUE " : "FALSE", 
                               nodeInfo.security & (NODEINFO_ZWAVE_SENSOR_MODE_WAKEUP_1000 | NODEINFO_ZWAVE_SENSOR_MODE_WAKEUP_250), 
							   WriteNodeTypeString(nodeType[byNode]));
                    bMaxNodeID = byNode;
                }
                byNode++;
            }
        }
        else
        {
            byNode += 8;
        }
    }
    mainlog_wr("Listening nodes registered %u", bListeningNodeListSize);

    if (fInitializeNetworkManagement)
    {
		InitializeSerialAPICapabilities();
        if (!NetworkManagement_Init(&sController_SerialAPI_Capabilities, abListeningNodeList, bListeningNodeListSize, afIsNodeAFLiRS, bNetworkRepeaterCount, &sNetworkManagementUT))
        {
            mainlog_wr("NetworkManagement_Init: Essential functionality NOT supported");
        }
        else
        {
            mainlog_wr("NetworkManagement_Init: Essential functionality supported");
        }
    }
    return bFound;
}


/*============================= CommandClassName =============================
** Function description
**      Convert Command Class identifier to char * strings
** Side effects:
**
**--------------------------------------------------------------------------*/
char 
*CommandClassName(
    BYTE bCmdClass)
{
    char *retStr = "Unknown Command Class";

    switch (bCmdClass)
    {
        case COMMAND_CLASS_BASIC:
            retStr = "COMMAND_CLASS_BASIC";
            break;

        case COMMAND_CLASS_ALARM:
            retStr = "COMMAND_CLASS_ALARM";
            break;

        case COMMAND_CLASS_POWERLEVEL:
            retStr = "COMMAND_CLASS_POWERLEVEL";
            break;

        case COMMAND_CLASS_ASSOCIATION:
            retStr = "COMMAND_CLASS_ASSOCIATION";
            break;

        case COMMAND_CLASS_CONFIGURATION:
            retStr = "COMMAND_CLASS_CONFIGURATION";
            break;

        case COMMAND_CLASS_CONTROLLER_REPLICATION:
            retStr = "COMMAND_CLASS_CONTROLLER_REPLICATION";
            break;

        case COMMAND_CLASS_CRC_16_ENCAP:
            retStr = "COMMAND_CLASS_CRC_16_ENCAP";
            break;

        case COMMAND_CLASS_DOOR_LOCK:
            retStr = "COMMAND_CLASS_DOOR_LOCK";
            break;

        case COMMAND_CLASS_HAIL:
            retStr = "COMMAND_CLASS_HAIL";
            break;

        case COMMAND_CLASS_LOCK:
            retStr = "COMMAND_CLASS_LOCK";
            break;

        case COMMAND_CLASS_MANUFACTURER_SPECIFIC:
            retStr = "COMMAND_CLASS_MANUFACTURER_SPECIFIC";
            break;

        case COMMAND_CLASS_PROTECTION:
            retStr = "COMMAND_CLASS_PROTECTION";
            break;

        case COMMAND_CLASS_METER:
            retStr = "COMMAND_CLASS_METER";
            break;

        case COMMAND_CLASS_MULTI_CHANNEL_V2:
            retStr = "COMMAND_CLASS_MULTI_CHANNEL/COMMAND_CLASS_MULTI_INSTANCE";
            break;

        case COMMAND_CLASS_SIMPLE_AV_CONTROL:
            retStr = "COMMAND_CLASS_SIMPLE_AV_CONTROL";
            break;

        case COMMAND_CLASS_SWITCH_ALL:
            retStr = "COMMAND_CLASS_SWITCH_ALL";
            break;

        case COMMAND_CLASS_SWITCH_BINARY:
            retStr = "COMMAND_CLASS_SWITCH_BINARY";
            break;

        case COMMAND_CLASS_SWITCH_MULTILEVEL:
            retStr = "COMMAND_CLASS_SWITCH_MULTILEVEL";
            break;

        case COMMAND_CLASS_VERSION:
            retStr = "COMMAND_CLASS_VERSION";
            break;

        case COMMAND_CLASS_WAKE_UP:
            retStr = "COMMAND_CLASS_WAKE_UP";
            break;

        case COMMAND_CLASS_ZIP_6LOWPAN:
            retStr = "COMMAND_CLASS_ZIP_6LOWPAN";
            break;

        case COMMAND_CLASS_ZIP:
            retStr = "COMMAND_CLASS_ZIP";
            break;

        default:
            break;
    }
    return retStr;
}


/*=============================== DumpNodeInfo ===============================
** Function description
**      Dump the registered nodeinformation for bNodeID to log and screen
** Side effects:
**
**--------------------------------------------------------------------------*/
void
DumpNodeInfo(
    BYTE bNodeID)
{
    int indx = bNodeID - 1;

    if ((0 < bNodeID) && (ZW_MAX_NODES >= bNodeID))
    {
        mainlog_wr("Nodeinformation registered for nodeID %03d", bNodeID);
        mainlog_wr("capability %02X, security %02X, reserved %02X, nodeType Basic %02X, Generic %02X, Specific %02X",
            sNetworkManagementUT.nodeDescriptor[indx].nodeInfo.capability, sNetworkManagementUT.nodeDescriptor[indx].nodeInfo.security, 
            sNetworkManagementUT.nodeDescriptor[indx].nodeInfo.reserved, sNetworkManagementUT.nodeDescriptor[indx].nodeInfo.nodeType.basic, 
            sNetworkManagementUT.nodeDescriptor[indx].nodeInfo.nodeType.generic, sNetworkManagementUT.nodeDescriptor[indx].nodeInfo.nodeType.specific);
        mainlog_wr("Command Classes registered as supported for nodeID %03d", bNodeID);
        for (int n = 0; 0 < sNetworkManagementUT.nodeDescriptor[indx].cmdClasses[n]; n++)
        {
            mainlog_wr("%s - [%02X]", CommandClassName(sNetworkManagementUT.nodeDescriptor[indx].cmdClasses[n]), sNetworkManagementUT.nodeDescriptor[indx].cmdClasses[n]);
        }
    }
    else
    {
        mainlog_wr("DumpNodeInfo nodeid not valid %03d", bNodeID);
    }
}


/*============================ CB_PowerLevelTestReport =======================
** Function description
**      Callback staus function for PowerLeveTest_Set(...)
** Side effects:
**
**--------------------------------------------------------------------------*/
void 
CB_PowerLevelTestReport(
    POWLEV_STATUS powStatus)
{
    char* pStatus = "";

    switch(powStatus.Status)
    {
        case POWERLEV_TEST_TRANSMIT_COMPLETE_OK:
            pStatus = "POWERLEV_TEST_TRANSMIT_COMPLETE_OK";
            break;

        case POWERLEV_TEST_TRANSMIT_COMPLETE_NO_ACK:
            pStatus = "POWERLEV_TEST_TRANSMIT_COMPLETE_NO_ACK";
            break;

        case POWERLEV_TEST_TRANSMIT_COMPLETE_FAIL:
            pStatus = "POWERLEV_TEST_TRANSMIT_COMPLETE_FAIL";
            break;

        case POWERLEV_TEST_NODE_REPORT_ZW_TEST_FAILED:
            pStatus = "POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_FAILED";
            break;

        case POWERLEV_TEST_NODE_REPORT_ZW_TEST_SUCCES:
            pStatus = "POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_SUCCES";
            break;

        case POWERLEV_TEST_NODE_REPORT_ZW_TEST_INPROGRESS:
            pStatus = "POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_INPROGRESS";
            break;

        case POWERLEV_TEST_NODE_REPORT_ZW_TEST_TIMEOUT:
            pStatus = "POWERLEV_TEST_NODE_REPORT_ZW_TEST_TIMEOUT";
            break;

        default:
            pStatus = "POWERLEVEL_TEST_UNKNOWN_CMD?";
            break;
    }

    printf("\n%s : Node %u and  %u, POW = %u transmit = %u / %u \n", 
            pStatus,
            powStatus.sourceNode,
            powStatus.destNode,
            powStatus.PowerLevel,
            powStatus.ackCountSuccess, 
            powStatus.NbrTestFrames);
}


/*==========================   CB_PowerLevelTestManager ======================
** Function description
**      Callback staus function for PowerLevelTestMan_CPowerLevelTestMan(...)
** Side effects:
**
**--------------------------------------------------------------------------*/
void
CB_PowerLevelTestManager(
    POWLEVMAN_STATUS* pDest1,
    POWLEVMAN_STATUS* pDest2)
{
    printf("\n\nPowerLevelTestManager result:\n");
    printf("  Status on Nodes %u <-> %u: ", pDest1->PLTStatus.sourceNode, pDest1->PLTStatus.destNode);
    if (POWLEV_MAN_SUCCESS == pDest1->PLMstatus)
    {
        printf("POWLEV_MAN_SUCCESS\n");
    }
    else if (POWLEV_MAN_WARNING_OVER_INSTALLER_LEVEL == pDest1->PLMstatus)
    {
        printf("POWLEV_MAN_WARNING_OVER_INSTALLER_LEVEL\n");
    }
    else if (POWLEV_MAN_FAILED == pDest1->PLMstatus)
    {
        printf("POWLEV_MAN_FAILED\n");
    }
    else if (POWLEV_MAN_WAITING == pDest1->PLMstatus)
    {
        printf("POWLEV_MAN_WAITING\n");
    }
    CB_PowerLevelTestReport(pDest1->PLTStatus);
    printf("\n\nPowerLevelTestManager result:\n");
    printf("  Status on Nodes %u <-> %u: ", pDest2->PLTStatus.sourceNode, pDest2->PLTStatus.destNode);
    if (POWLEV_MAN_SUCCESS == pDest2->PLMstatus)
    {
        printf("POWLEV_MAN_SUCCESS\n");
    }
    else if (POWLEV_MAN_WARNING_OVER_INSTALLER_LEVEL == pDest2->PLMstatus)
    {
        printf("POWLEV_MAN_WARNING_OVER_INSTALLER_LEVEL\n");
    }
    else if (POWLEV_MAN_FAILED == pDest2->PLMstatus)
    {
        printf("POWLEV_MAN_FAILED\n");
    }
    else if (POWLEV_MAN_WAITING == pDest2->PLMstatus)
    {
        printf("POWLEV_MAN_WAITING\n");
    }
    CB_PowerLevelTestReport(pDest2->PLTStatus);
}


/*========================== RequestNetworkUpdate_Compl ======================
** Function description
**      Callback function called by RequestNetworkUpdate functionality when done
** Side effects:
**
**--------------------------------------------------------------------------*/
void
RequestNetworkUpdate_Compl(
    BYTE bStatus)
{
    mainlog_wr("ZW_RequestNetworkUpdate status - %d\r\n", bStatus);
}


/*========================= NetworkRediscoveryComplete =======================
** Function description
**      Callback function called when Network RediscoveryFunctionality has
** stopped.
** Side effects:
**
**--------------------------------------------------------------------------*/
void
NetworkRediscoveryComplete(
    BYTE bStatus)
{
    testStarted = false;
    mainlog_wr("Network Rediscovery stopped - Status %u", bStatus);
}


/*========================== RequestNextNodeInformation ======================
** Function description
**      Callback function called if timeout waiting for Nodeinformation frame
** requested with DoRequestnodeInformation.
** Side effects:
**
**--------------------------------------------------------------------------*/
void
RequestNextNodeInformation(int H)
{
    bool retVal = false;
    int i;
    timerH = 0; //callback form timeout. Reset handle

    for (i = sNetworkManagementUT.bNHSCurrentIndex + 1; i < bListeningNodeListSize; i++)
    {
        /* If first Command Class is NONE ZERO then we do have Command Class information for Node */
        if (0 == sNetworkManagementUT.nodeDescriptor[abListeningNodeList[i] - 1].cmdClasses[0])
        {
            /* We need to request nodeinformation from at least this node */
            retVal = true;
            break;
        }
    }
    /* Are we done with NodeInformation Requests */
    if (true == retVal)
    {
        DoRequestForNodeInformation(i);
    }
    else
    {
        sNetworkManagementUT.bNHSCurrentIndex = 0;
        /* We got all the nodeinformation so start the real Network Health Test */
        testStarted = NetworkHealthStart();
        if (!testStarted)
        {
            mainlog_wr("NetworkHealthStart Failed to start Network Health Test");
        }
    }
}


/*========================== DoNetworkHealthMaintenance ======================
** Function description
**		Does the actual Maintenance update loop.
** requested with DoRequestnodeInformation.
** Side effects:
**
**--------------------------------------------------------------------------*/
void
DoNetworkHealthMaintenance()
{
    sNetworkManagementUT.bTestMode = MAINTENANCE;
    if (NetworkManagement_NetworkHealth_MaintenanceStart())
    {
        mainlog_wr("Network Health Maintenance Mode started - Press Q to stop");
        while (1)
        {
            if (_kbhit())
            {
                char _ch = toupper(_getch());

                if ('Q' == _ch)
                {
                    NetworkManagement_NetworkHealth_MaintenanceStop();
                    mainlog_wr("Network Health Maintenance Mode stopped");
                    while (_kbhit())
                    {
                        /* Flush keyboard */
                        _ch = toupper(_getch());
                    }
                    return;
                }
                else
                {
                    mainlog_wr("Network Health Maintenance Mode active - Press Q to stop");
                }
            }
            NetworkManagement_NetworkHealth_Maintenance();
            Sleep(10);
        }
    }
    else
    {
        mainlog_wr("Network Health Maintenance Mode could not be started");
    }
}


/*=========================== NetworkHealthComplete ==========================
** Function description
**      Callback function called when Network Health Functionality has stopped.
** Side effects:
**
**--------------------------------------------------------------------------*/
void
NetworkHealthComplete(
    BYTE bStatus)
{
    testStarted = false;
    mainlog_wr("Network Health stopped - Status %u", bStatus);
}


/*=========================== NetworkHealthComplete ==========================
** Function description
**      Function called starting the FULL or SINGLE Network Health Test
** Side effects:
**
**--------------------------------------------------------------------------*/
bool
NetworkHealthStart()
{
    bool retVal = false;
    /* We got all the nodeinformation so start the real Network Health Test */
    switch (sNetworkManagementUT.bTestMode)
    {
        case FULL:
            {
                retVal = NetworkManagement_NetworkHealth_Start(NetworkHealthComplete);
            }
            break;

        case SINGLE:
            {
                sNetworkManagementUT.bCurrentTestNodeID = bNodeID;
                retVal = NetworkManagement_NetworkHealth_Start(NetworkHealthComplete);
            }
            break;

        case MAINTENANCE:
            {
            }
            break;

        default:
            break;
    }
    return retVal;
}


/*===================== RequestNodeInformation_Completed =====================
** Function description
**      Callback function called when the ZW_RequestNodeInfo call started by
** DoRequestForNodeInformation has been executed
** Side effects:
**
**--------------------------------------------------------------------------*/
void
RequestNodeInformation_Completed(
    BYTE bTxStatus)
{
    if (!bTxStatus)
    {
        mainlog_wr("Request Node Info transmitted successfully to node %03d", abListeningNodeList[sNetworkManagementUT.bNHSCurrentIndex]);
    }
    else
    {
        mainlog_wr("Request Node Info transmit failed to node %03d", abListeningNodeList[sNetworkManagementUT.bNHSCurrentIndex]);
    }
}


/* Set Request Nodeinformation timeout to 8 seconds */
#define TIMEOUT_NODEINFORMATION		8

/*======================= DoRequestForNodeInformation ========================
** Function description
**      Function to update 
** Side effects:
**
**--------------------------------------------------------------------------*/
void
DoRequestForNodeInformation(
    BYTE bNextIndex)
{
    sNetworkManagementUT.bNHSCurrentIndex = bNextIndex;
    pTimer = CTime::GetInstance();
    //If timer is running, kill it!
    if (0 != timerH)
    {
        pTimer->Kill(timerH);
    }
    timerH = pTimer->Timeout(TIMEOUT_NODEINFORMATION, RequestNextNodeInformation);
    if (0 == timerH)
    {
        mainlog_wr("DoRequestForNodeInformation no timer! ERROR!");
        /* Go on and test anyway */
        if (!NetworkHealthStart())
        {
            mainlog_wr("Network Health Test could not be started - FAILED");
            /* Could not start NetworkHealth */
            NetworkHealthComplete(0);
        }
    }
    else
    {
        Sleep(10);
        if (api.ZW_RequestNodeInfo(abListeningNodeList[sNetworkManagementUT.bNHSCurrentIndex], RequestNodeInformation_Completed))
        {
            mainlog_wr("ZW_RequestNodeInfo has been initiated for node %u", abListeningNodeList[sNetworkManagementUT.bNHSCurrentIndex]);
        }
        else
        {
            mainlog_wr("ZW_RequestNodeInfo could not be initiated for node %u", abListeningNodeList[sNetworkManagementUT.bNHSCurrentIndex]);
            mainlog_wr("Timeout callback function will initiate and try with next node");
        }
    }
}


/*=============================== NetworkHealth ==============================
** Function description
**      Function called when starting either FULL or SINGLE Network Health 
** test. First is determined if any node needs to be requested for 
** nodeinformation which includes command classes supported. If so then
** this is then initiated. The Network Health Test is then started if no node
** needs to inform Controller about Command Classes supported or when all
** nodes needing it has been queried
** Side effects:
**
**--------------------------------------------------------------------------*/
bool
NetworkHealth(
    eTESTMODE bTestMode)
{
    bool retVal = false;
    int i;
    sNetworkManagementUT.bTestMode = bTestMode;
    for (i = 0; i < bListeningNodeListSize; i++)
    {
        /* If first Command Class is NONE ZERO then we do have Command Class information for Node */
        if (0 == sNetworkManagementUT.nodeDescriptor[abListeningNodeList[i] - 1].cmdClasses[0])
        {
            /* We need to request nodeinformation from at least this node */
            retVal = true;
            break;
        }
    }
    /* Do we need to request NodeInformation from Nodes Under Test */
    if (true == retVal)
    {
        DoRequestForNodeInformation(i);
    }
    else
    {
        /* Start the real Network Health Test */
        retVal = NetworkHealthStart();
    }
    return retVal;
}


/*==================== CB_ToggleBasicSetONOFF_Completed ======================
** Function description
**      Callback function called when NetworkManagement_ZW_SendData has been 
** executed after selecting Network Health Menuitem '5'.
** Side effects:
**
**--------------------------------------------------------------------------*/
void
CB_ToggleBasicSetONOFF_Completed(
    BYTE bTxStatus, 
    TX_STATUS_TYPE *psTxInfo)
{
    mainlog_wr("Maintain  - Node %03u - BASIC SET %s %s (%ums)", 
                bNodeID, (255 == pBasicBuf[2]) ? " ON" : "OFF", 
                (TRANSMIT_COMPLETE_OK == bTxStatus) ? "SUCCESS" : "FAILED", psTxInfo->wTime * 10);
}


/*============================ CB_PingTestComplete ===========================
** Function description
**      Callback function called when Ping test transmit has been executed.
** Side effects:
**
**--------------------------------------------------------------------------*/
void
CB_PingTestComplete(
    BYTE bTxStatus, 
    TX_STATUS_TYPE *psTxInfo)
{
    mainlog_wr("Ping Done - Node %03u - %s - Latency %ums", abListeningNodeList[bPingNodeIndex], 
                (TRANSMIT_COMPLETE_OK == bTxStatus) ? "SUCCESS" : "FAILED", psTxInfo->wTime * 10);
    if (TRANSMIT_COMPLETE_OK != bTxStatus)
    {
        abPingFailed[abPingFailedSize++] = abListeningNodeList[bPingNodeIndex];
    }
    if (++bPingNodeIndex < bListeningNodeListSize)
    {
        testStarted = NetworkManagement_NH_TestConnection(abListeningNodeList[bPingNodeIndex], CB_PingTestComplete);
        if (testStarted)
        {
            mainlog_wr("Ping      - Node %03u", abListeningNodeList[bPingNodeIndex]);
        }
        else
        {
            mainlog_wr("Ping Node - could not be started - stopping");
        }
    }
    else
    {
        testStarted = false;
    }
    if (!testStarted)
    {
        if (0 < abPingFailedSize)
        {
            char bCharBuf[1024];

            sprintf_s(bCharBuf, sizeof(bCharBuf), "Ping Failed - ");
            for (int i = 0; i < abPingFailedSize; i++)
            {
                sprintf_s(&bCharBuf[strlen(bCharBuf)], sizeof(bCharBuf) - strlen(bCharBuf),"%0X ", abPingFailed[i]);
            }
            mainlog_wr(bCharBuf);
        }
        else
        {
            mainlog_wr("Ping Success - All Nodes answered");
        }
        mainlog_wr("Ping Node - End");
        testStarted = false;
    }
}


/*========================= CB_GetVersionTestComplete ========================
** Function description
**      Callback function called when Get Version transmit has been executed.
** Side effects:
**
**--------------------------------------------------------------------------*/
void
CB_GetVersionTestComplete(
    BYTE bTxStatus, 
    TX_STATUS_TYPE *psTxInfo)
{
    /* If timer is running, kill it! */
    if (0 != timerGetVersionReportH)
    {
        pGetVersionReportTimer->Kill(timerGetVersionReportH);
    }
    timerGetVersionReportH = pGetVersionReportTimer->Timeout((TRANSMIT_COMPLETE_OK == bTxStatus) ? 
                                                              GETVERSIONREPORTTIMEOUT :
                                                              GETVERSIONREPORTTIMEOUT / 2,
                                                             GetVersionReportTimeout);
    mainlog_wr("Get Version Done - Node %03u - %s - Latency %ums", abListeningNodeList[bPingNodeIndex], 
                (TRANSMIT_COMPLETE_OK == bTxStatus) ? "SUCCESS" : "FAILED", psTxInfo->wTime * 10);
	/* Assume we get no Report from DUT */
	abGetReportFailed[abGetReportFailedSize++] = abListeningNodeList[bPingNodeIndex];
    if (TRANSMIT_COMPLETE_OK != bTxStatus)
    {
        abPingFailed[abPingFailedSize++] = abListeningNodeList[bPingNodeIndex];
    }
}


/*========================== GetVersionReportTimeout =========================
** Function description
**      Get Version Report Timeout functionality
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
GetVersionReportTimeout(
    int H)
{
	if (0 == H)
	{
		/* Remove current DUT as missing to answer with a REPORT frame */
		abGetReportFailed[--abGetReportFailedSize] = 0;
	}
    /* Make sure timer handle is ZERO indicating not running */
    timerGetVersionReportH = 0;
    if (++bPingNodeIndex < bListeningNodeListSize)
    {
        testStarted = NetworkManagement_ZW_SendData(abListeningNodeList[bPingNodeIndex], pBasicBuf, 2, CB_GetVersionTestComplete);
        if (testStarted)
        {
            mainlog_wr("Get Version - Node %03u", abListeningNodeList[bPingNodeIndex]);
        }
        else
        {
            mainlog_wr("Get Version - could not be started - stopping");
        }
    }
    else
    {
        testStarted = false;
    }
    if (!testStarted)
    {
		if (0 < abGetReportFailedSize)
        {
            char bCharBuf[1024];

			if (0 < abPingFailedSize)
			{
				sprintf_s(bCharBuf, sizeof(bCharBuf), "Get Version Failed - ");
				for (int i = 0; i < abPingFailedSize; i++)
				{
					sprintf_s(&bCharBuf[strlen(bCharBuf)], sizeof(bCharBuf) - strlen(bCharBuf),"%0X ", abPingFailed[i]);
				}
				mainlog_wr(bCharBuf);
			}
            sprintf_s(bCharBuf, sizeof(bCharBuf), "Report Version not returned - ");
			for (int i = 0; i < abGetReportFailedSize; i++)
            {
				sprintf_s(&bCharBuf[strlen(bCharBuf)], sizeof(bCharBuf) - strlen(bCharBuf),"%0X ", abGetReportFailed[i]);
            }
            mainlog_wr(bCharBuf);
        }
        else
        {
            mainlog_wr("Get Version Success - All Nodes answered");
        }
        mainlog_wr("Get Version Node - End");
        testStarted = false;
    }
}


/*========================== CB_InjectNewLWR_Compl ===========================
** Function description
**      Callback function called when api.MemoryPutBuffer has finished
** Side effects:
**
**--------------------------------------------------------------------------*/
void
CB_InjectNewLWR_Compl(void)
{
    mainlog_wr("New LWR has been injected for %03u", bNodeID);
}


/*========================== EnterNewCurrentNodeID ===========================
** Function description
**      Function used to change the 'current NodeID'
** Side effects:
**
**--------------------------------------------------------------------------*/
void 
EnterNewCurrentNodeID()
{
    char instr[10];

    printf("Enter new current NodeID: ");
    gets_s(instr);
    bNodeID = atoi(instr);
    printf("Current NodeID is now %i\n", bNodeID);
}

/*========================== StartNetworkHealthTest ===========================
** Start a single network health test.
** Side effects:
**
**--------------------------------------------------------------------------*/
void
StartNetworkHealthTestSingle(void)
{
    if (!testStarted && (0 < bListeningNodeListSize))
    {
        /* Only if a full Network Health have been run */
        testStarted = NetworkHealth(SINGLE);
        if (testStarted)
        {
            testMode = 5;
        }
        else
        {
            testMode = 0;
        }
    }
    else
    {
        testStarted = false;
        if (0 < bListeningNodeListSize)
        {
            NetworkManagement_NetworkHealth_Stop(&sNetworkManagementUT);
            mainlog_wr("Stopping Z-Wave Network Health Test for Node %i", bNodeID);
        }
        else
        {
            mainlog_wr("No Nodes in network eligible for establishing Z-Wave Network Health");
        }
    }
}

/*==================================== main ==================================
** Function description
**      main
** Side effects:
**
**--------------------------------------------------------------------------*/
int 
main(
    int argc,
    char* argv[])
{
    BYTE uCversion[100];
    BYTE libType, PortValue = 1;
    char instr[10];
    char portstr[16];

    if (argc > 1)
    {
        if (strcmp(argv[1], "-q") == 0)
        {
            return 0;
        }
        PortValue = (BYTE)atoi(argv[1]);
    }
    sprintf_s(portstr, "COM%d", PortValue);
    /* Generate log main_logfilename */
    sprintf_s(main_logfilenameStr, sizeof(main_logfilenameStr), "log_%s.txt", portstr);
    mainlog = mlog.AddLog(main_logfilenameStr);
    /* Generate log debug_logfilename */
    sprintf_s(debug_logfilenameStr, sizeof(debug_logfilenameStr), "debug_%s.txt", portstr);
    debuglog = mlog.AddLog(debug_logfilenameStr);
    mainlog_wr("IMAtoolbox Ver. %u.%u.%u.%u%s - System started", 
               IMATOOLBOX_VERSION_MAJOR, IMATOOLBOX_VERSION_MINOR,
               IMATOOLBOX_REVISION_MAJOR, IMATOOLBOX_REVISION_MINOR,
               IMATOOLBOX_RELEASE_TYPE);
    mainlog_wr("Using serial port %s", portstr);
    if (api.Initialize(PortValue, CSerialAPI::SPEED_115200, ApplicationCommandHandler, ApplicationCommandHandler_Bridge, CommErrorNotification))
    {
		char *pLibTypeStr;
        libType = api.ZW_Version((BYTE*)uCversion);	//Get the lib type and version from module
		pLibTypeStr = GetValidLibTypeStr(libType);
		if (0 != strcmp("", pLibTypeStr))
		{
            mainlog_wr("Z-Wave %s based serial API found", pLibTypeStr);
            sprintf_s((char *)uCversion, 100, "%s - %s", uCversion, pLibTypeStr);
		}
		else
		{
            mainlog_wr("No Serial API module detected, [libtype %u]- check serial connections.\nThis application requires a Serial API\non a Z-Wave module connected via a serialport.\nDownload the correct Serial API code to the Z-Wave Module and restart the application.", libType);
            libType = 0;  /* Indicate we want to end this */

		}
        if (libType)
        {
            static int sendState = 0;
            char ch = 0;
            mainlog_wr("%s", uCversion);
            memset(sNetworkManagementUT.nodeDescriptor, 0, sizeof(sNetworkManagementUT.nodeDescriptor));
            /*Set Idle learn mode function.*/
            api.ZW_SetIdleNodeLearn(IdleLearnNodeState_Compl);
            ReloadNodeList(true);
            if (bNodeID == MyNodeId)
            {
                bNodeID++;
            }
            printf("Press Q to end\n");
            while (ch != 'Q')
            {
                ch = toupper(_getch());
                switch (ch)
                {
                    case 'J':
                        {
                            screenLog = !screenLog;
                            printf("Logging to screen = %u\n", screenLog);
                        }
                        break;

                    case 'A': // Add
                        bAddRemoveNodeFromNetwork = !bAddRemoveNodeFromNetwork;
                        if (bAddRemoveNodeFromNetwork)
                        {
                            api.ZW_AddNodeToNetwork(ADD_NODE_ANY|ADD_NODE_OPTION_HIGH_POWER|ADD_NODE_OPTION_NETWORK_WIDE, SetLearnNodeState_Compl);
                            mainlog_wr("ZW_AddNodeToNetwork ADD_NODE_ANY|ADD_NODE_OPTION_NETWORK_WIDE");
                        }
                        else
                        {
                            api.ZW_AddNodeToNetwork(ADD_NODE_STOP, SetLearnNodeState_Compl);
                            mainlog_wr("ZW_AddNodeToNetwork ADD_NODE_STOP");
                        }
                        break;

                    case 'N':
                        {
                            eTESTMODE currentTestMode = sNetworkManagementUT.bTestMode;
                            int showHelp;
                            int networkHealthTestMenu;
                            networkHealthTestMenu = 1;
                            showHelp = 1;

                            ReloadNodeList(true);
                            while (networkHealthTestMenu)
                            {
                                char ch;
                                if (showHelp)
                                {
                                    showHelp = 0;
                                    ch = 'H';
                                }
                                else
                                {
                                    ch = toupper(_getch());
                                }
                                switch (ch)
                                {
                                    case '\r':
                                    case 'H':
                                        printf("\nNetwork Health Test Menu\n\n");
                                        printf("1 - Z-Wave Network Health - Full network\n");
                                        printf("2 - Z-Wave Network Health - current NodeID %03u\n", bNodeID);
                                        printf("3 - Z-Wave Network Health - Maintenance Mode\n");
                                        printf("4 - Ping all Nodes\n");
                                        printf("5 - Transmit frame to current NodeID %03u using Maintenance ZW_SendData\n", bNodeID);
                                        printf("6 - Rediscovery of known Listening nodes\n");
                                        printf("7 - Check if Repeater is a valid repeater to Node\n");
                                        printf("8 - Dump node neighbors\n");
                                        printf("9 - Get Version from all nodes\n");
                                        printf("P - Power level test\n");
                                        printf("S - Set Network Health Maintenance Sample Period (Default if = 0) - %i\n", sNetworkManagementUT.iMaintenanceSamplePeriod);
                                        printf("R - Set Maintenance rounds before Rediscovery list is executed - %i\n", sNetworkManagementUT.iMaintenanceRoundsBeforeRediscoveryExecuted);
                                        printf("# - Enter new current NodeID %03u\n\n", bNodeID);
                                        printf("I - Inject LWR for current NodeID %03u\n", bNodeID);
										printf("T - get TX Timers\n");
										printf("C - Clear TX Timers\n");
                                        printf("L - Toggle LWR Lock %s\n", bLWRLocked ? "ON" : "OFF");
										printf("M - Measure and show RSSI map\n");
										printf("B - Show background RSSI\n");
                                        printf("0 - Exit Network Health Test Menu\n\n");
                                        break;

                                    case '1':
                                        {
                                            if (!testStarted && (0 < bListeningNodeListSize))
                                            {
                                                testStarted = NetworkHealth(FULL);
                                                if (testStarted)
                                                {
                                                    testMode = 4;
                                                }
                                                else
                                                {
                                                    testMode = 0;
                                                }
                                            }
                                            else
                                            {
                                                testStarted = false;
                                                if (0 < bListeningNodeListSize)
                                                {
                                                    NetworkManagement_NetworkHealth_Stop(&sNetworkManagementUT);
                                                    mainlog_wr("Stopping Z-Wave Network Health Test");
                                                }
                                                else
                                                {
                                                    mainlog_wr("No Nodes in network eligible for establishing Z-Wave Network Health");
                                                }
                                            }
                                        }
                                        break;

                                    case '2':
										StartNetworkHealthTestSingle();
                                        break;

                                    case '3':
                                        {
                                            DoNetworkHealthMaintenance();
                                            showHelp = 1;
                                        }
                                        break;

                                    case '4':
                                        {
                                            if (!testStarted)
                                            {
                                                if (0 < bListeningNodeListSize)
                                                {
                                                    bPingNodeIndex = 0;
                                                    abPingFailedSize = 0;
                                                    memset(abPingFailed, 0, sizeof(abPingFailed));
                                                    testStarted = NetworkManagement_NH_TestConnection(abListeningNodeList[bPingNodeIndex], CB_PingTestComplete);
                                                    if (testStarted)
                                                    {
                                                        mainlog_wr("Ping Node - Start");
                                                        mainlog_wr("Ping      - Node %03u", abListeningNodeList[bPingNodeIndex]);
                                                    }
                                                    else
                                                    {
                                                        mainlog_wr("Ping Node - could not be started");
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                mainlog_wr("A test is allready in progress");
                                            }
                                        }
                                        break;

                                    case '5':
                                        {
                                            pBasicBuf[0] = 32;
                                            pBasicBuf[1] = 1;
                                            if (0 == pBasicBuf[2])
                                            {
                                                pBasicBuf[2] = 255;
                                            }
                                            else
                                            {
                                                pBasicBuf[2] = 0;
                                            }

                                            if (NetworkManagement_ZW_SendData(bNodeID, pBasicBuf, 3, CB_ToggleBasicSetONOFF_Completed))
                                            {
                                                mainlog_wr("Maintain  - Node %03u - BASIC SET %s", 
                                                            bNodeID, (255 == pBasicBuf[2]) ? " ON" : "OFF");
                                            }
                                            else
                                            {
                                                mainlog_wr("Maintain  - Node %03u - BASIC SET %s FALSE", 
                                                            bNodeID, (255 == pBasicBuf[2]) ? " ON" : "OFF");
                                            }
                                        }
                                        break;

                                    case '6':
                                        if (!testStarted && (0 < bListeningNodeListSize))
                                        {
                                            testStarted = NetworkManagement_DoRediscoveryStart(NetworkRediscoveryComplete);
                                            if (testStarted)
                                            {
                                                mainlog_wr("Starting Network Rediscovery");
                                            }
                                            else
                                            {
                                                mainlog_wr("Could not start Network Rediscovery");
                                            }
                                        }
                                        else
                                        {
                                            if (0 < bListeningNodeListSize)
                                            {
                                                NetworkManagement_DoRediscoveryStop(&sNetworkManagementUT);
                                                mainlog_wr("Stopping Network Rediscovery");
                                            }
                                            testStarted = false;
                                        }
                                        break;

                                    case '7':
                                        {
                                            BYTE bRepNodeID;
                                            BYTE bDestNodeID;

                                            /* Verify repeater */
                                            printf("Enter repeater NodeID: ");
                                            gets_s(instr);
                                            bRepNodeID = atoi(instr);
                                            printf("Enter destination NodeID: ");
                                            gets_s(instr);
                                            bDestNodeID = atoi(instr);
                                            if (NetworkManagement_CheckRepeater(bRepNodeID, bDestNodeID, MyNodeId, &sNetworkManagementUT))
                                            {
                                                printf("Repeater %i is a valid repeater to %i\n", bRepNodeID, bDestNodeID);
                                            }
                                            else
                                            {
                                                printf("Repeater %i is not a valid repeater to %i\n", bRepNodeID, bDestNodeID);
                                            }
                                        }
                                        break;

                                    case '8':
                                        {
                                            NetworkManagement_DumpNeighbors();
                                        }
                                        break;

                                    case '9':
                                        {
                                            if (!testStarted)
                                            {
                                                if (0 < bListeningNodeListSize)
                                                {
                                                    bPingNodeIndex = 0;
                                                    abPingFailedSize = 0;
                                                    memset(abPingFailed, 0, sizeof(abPingFailed));
													abGetReportFailedSize = 0;
													memset(abGetReportFailed, 0, sizeof(abGetReportFailed));
                                                    pBasicBuf[0] = COMMAND_CLASS_VERSION;
                                                    pBasicBuf[1] = VERSION_GET;
                                                    testStarted = NetworkManagement_ZW_SendData(abListeningNodeList[bPingNodeIndex], pBasicBuf, 2, CB_GetVersionTestComplete);
                                                    if (testStarted)
                                                    {
                                                        mainlog_wr("Get Version - Start");
                                                        mainlog_wr("Get Version - Node %03u", abListeningNodeList[bPingNodeIndex]);
                                                        pGetVersionReportTimer = CTime::GetInstance();
                                                        //If timer is running, kill it!
                                                        if (0 != timerGetVersionReportH)
                                                        {
                                                            pGetVersionReportTimer->Kill(timerGetVersionReportH);
                                                        }
                                                        timerGetVersionReportH = 0;
                                                    }
                                                    else
                                                    {
                                                        mainlog_wr("Get Version - could not be started");
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                mainlog_wr("A test is allready in progress");
                                            }
                                        }
                                        break;

									case 'C':
										{
											mainlog_wr("Clear TX Timers...");
											api.ZW_Clear_Tx_Timers();
											mainlog_wr("Clear TX Timers - Done");
										}
										break;

									case 'T':
										{
											DWORD dwTXTimerChannel0;
											DWORD dwTXTimerChannel1;
											DWORD dwTXTimerChannel2;

											api.ZW_Get_Tx_Timers(&dwTXTimerChannel0, &dwTXTimerChannel1, &dwTXTimerChannel2);
											mainlog_wr("Tx timer for channel 0 %u", dwTXTimerChannel0);
											mainlog_wr("Tx timer for channel 1 %u", dwTXTimerChannel1);
											mainlog_wr("Tx timer for channel 2 %u", dwTXTimerChannel2);
										}
										break;

                                    case 'S':
                                        {
                                            mainlog_wr("Maintenance Sample Period is currently %i", sNetworkManagementUT.iMaintenanceSamplePeriod);
                                            mainlog_wr("If sample Period = ZERO then (60min/nodes) is in effect");
                                            printf_s("Enter new value in seconds: ");
                                            gets_s(instr);
                                            sNetworkManagementUT.iMaintenanceSamplePeriod = atoi(instr);
                                            mainlog_wr("Maintenance Sample Period in seconds changed to %i", sNetworkManagementUT.iMaintenanceSamplePeriod);
                                            mainlog_wr("New setting effective on Maintenance restart");
                                        }
                                        break;

                                    case 'R':
                                        {
                                            mainlog_wr("Maintenance Rounds before Rediscovery executed");
                                            mainlog_wr("Default its %i, currently its %i", MAINTENANCEPRIORREDISCOVERY, sNetworkManagementUT.iMaintenanceRoundsBeforeRediscoveryExecuted);
                                            printf_s("Enter new Maintenance Rounds value: ");
                                            gets_s(instr);
                                            sNetworkManagementUT.iMaintenanceRoundsBeforeRediscoveryExecuted = atoi(instr);
                                            mainlog_wr("Maintenance Rounds before Rediscovery changed to %i", sNetworkManagementUT.iMaintenanceRoundsBeforeRediscoveryExecuted);
                                            mainlog_wr("New setting effective on Maintenance restart");
                                        }
                                        break;

                                    case 'P':
                                        {
                                            printf("Power level test manager       'm' \n");
                                            printf("Power level test               't' \n");
                                            if(0 != PowerLevelTestMan_GetRepeaterId && 
                                                0 != PowerLevelTestMan_GetSourceId() && 
                                                0 != PowerLevelTestMan_GetDestinationId())
                                            {
                                                printf("Retest '*'\n Source = %u , Repeater = %u, Destination = %u\n",
                                                    PowerLevelTestMan_GetSourceId(), PowerLevelTestMan_GetRepeaterId(), PowerLevelTestMan_GetDestinationId());
                                            }
                                            gets_s(instr);
                                            if('m' == instr[0])
                                            {
                                                printf("Source ID: ");
                                                gets_s(instr);
                                                BYTE bSourceNodeID = atoi(instr);

                                                printf("Repeater ID: ");
                                                gets_s(instr);
                                                BYTE RepeaterId = atoi(instr);


                                                printf("Destination ID: ");
                                                gets_s(instr);
                                                BYTE bDestNodeID = atoi(instr);
                                                PowerLevelTestMan_CPowerLevelTestMan(&api, MyNodeId, bSourceNodeID, RepeaterId, bDestNodeID, &CB_PowerLevelTestManager);
                                            }
                                            else if('t' == instr[0])
                                            {
                                                printf("Source NodeID: ");
                                                gets_s(instr);
                                                BYTE bSourceNodeID = atoi(instr);
                                                printf("Destination NodeID: ");
                                                gets_s(instr);
                                                BYTE bTestNodeID = atoi(instr);
                                                printf("Power level 0 - 9: ");
                                                gets_s(instr);
                                                BYTE bPowerLevel = atoi(instr);
                                                printf("Number test frames: ");
                                                gets_s(instr);
                                                int testFrames = atoi(instr);
                                                PowerLevelTest_CPowerLevelTest(&api, TRUE);
                                                PowerLevelTest_Init( MyNodeId, bSourceNodeID,bTestNodeID, bPowerLevel, testFrames, true);
                                                PowerLevelTest_Set(&CB_PowerLevelTestReport);
                                            }
                                            else if(0 != PowerLevelTestMan_GetRepeaterId && 
                                                0 != PowerLevelTestMan_GetSourceId() && 
                                                0 != PowerLevelTestMan_GetDestinationId() &&
                                                '*' == instr[0])
                                            {
                                                PowerLevelTestMan_ReRun();
                                            }
                                        }
                                        break;

                                    case '#':
                                        EnterNewCurrentNodeID();
                                        break;

                                    case 'I':
                                        {
                                            BYTE pRoute[ROUTECACHE_LINE_SIZE];
                                            int atoiRetval;
                                            int bRouteLen = 0, n = 1;
											BYTE bRouteSpeed;
											BYTE abFuncIDToSupport[] = {FUNC_ID_ZW_SET_LAST_WORKING_ROUTE};
											bool fIsZW_SetLastWorkingRouteSupported = NetworkManagement_IsFuncIDsSupported(abFuncIDToSupport, sizeof(abFuncIDToSupport));

                                            mainlog_wr("Inject new LWR for %03u", bNodeID);
											if (false == fIsZW_SetLastWorkingRouteSupported)
											{
												mainlog_wr("Inject new LWR NOT supported by attached SerialAPI module");
												break;
											}
											if ((0 == bNodeID) && (ZW_MAX_NODES < bNodeID))
											{
												mainlog_wr("Inject new LWR NOT possible for invalid nodeID %03u", bNodeID);
												break;
											}
											if (TRUE != api.ZW_GetLastWorkingRoute(bNodeID, pRoute))
											{
												mainlog_wr("No valid LWR exists for %03u", bNodeID);
											}
											else
											{
												mainlog_wr("Existing valid LWR for %03u to - %03u,%03u,%03u,%03u at %s", 
														   bNodeID, 
														   pRoute[ROUTECACHE_LINE_REPEATER_1_INDEX], 
														   pRoute[ROUTECACHE_LINE_REPEATER_2_INDEX], 
														   pRoute[ROUTECACHE_LINE_REPEATER_3_INDEX], 
														   pRoute[ROUTECACHE_LINE_REPEATER_4_INDEX],
														   (ZW_LAST_WORKING_ROUTE_SPEED_100K == pRoute[ROUTECACHE_LINE_CONF_INDEX]) ? "100k" :
														    (ZW_LAST_WORKING_ROUTE_SPEED_40K == pRoute[ROUTECACHE_LINE_CONF_INDEX]) ? "40k" :
															 (ZW_LAST_WORKING_ROUTE_SPEED_9600 == pRoute[ROUTECACHE_LINE_CONF_INDEX]) ? "9.6k" : "Auto");
											}
											memset(pRoute, 0, sizeof(pRoute));
											/* Get LWR Route configuration byte where the Repeater count is saved */
											do
											{
												mainlog_wr("Repeater %u : ", n);
												gets_s(instr);
												atoiRetval = atoi(instr);
												pRoute[bRouteLen++] = (BYTE)(atoiRetval & 0xFF);
												if (0 == atoiRetval)
												{
													break;
												}
											} while ((++n < sizeof(pRoute)) && (bRouteLen < 4));
											mainlog_wr("Route speeed (1 = 9600, 2 = 40k, 3 = 100k) : ");
											gets_s(instr);
											atoiRetval = atoi(instr);
											if ((0 > atoiRetval) && (ZW_LAST_WORKING_ROUTE_SPEED_100K < atoiRetval))
											{
												mainlog_wr("Entered speed not valid %i - defaults to Auto", atoiRetval);
												bRouteSpeed = ZW_LAST_WORKING_ROUTE_SPEED_AUTO;
											}
											else
											{
												bRouteSpeed = (BYTE)(atoiRetval & 0xFF);
											}
											pRoute[LWR_ENTRY_OFFSET_CONF] = bRouteSpeed;
											if ((0 < bNodeID) && (ZW_MAX_NODES >= bNodeID))
											{
												mainlog_wr("Setting LWR for %03u to - %03u,%03u,%03u,%03u at %s", 
														   bNodeID, 
														   pRoute[ROUTECACHE_LINE_REPEATER_1_INDEX], 
														   pRoute[ROUTECACHE_LINE_REPEATER_2_INDEX], 
														   pRoute[ROUTECACHE_LINE_REPEATER_3_INDEX], 
														   pRoute[ROUTECACHE_LINE_REPEATER_4_INDEX],
														   (ZW_LAST_WORKING_ROUTE_SPEED_100K == pRoute[ROUTECACHE_LINE_CONF_INDEX]) ? "100k" :
														    (ZW_LAST_WORKING_ROUTE_SPEED_40K == pRoute[ROUTECACHE_LINE_CONF_INDEX]) ? "40k" :
															 (ZW_LAST_WORKING_ROUTE_SPEED_9600 == pRoute[ROUTECACHE_LINE_CONF_INDEX]) ? "9.6k" : "Auto");
												/* Write new LWR Entry for current NodeID */
												if (false == api.ZW_SetLastWorkingRoute(bNodeID, pRoute))
												{
													mainlog_wr("Failed could not set new LWR for nodeID %03u", bNodeID);
												}
												else
												{
													mainlog_wr("Success new LWR set for nodeID %03u", bNodeID);
												}
											}
											else
											{
												mainlog_wr("Current NodeID %03u not valid must be inside (0 < NodeID <= 232)");
											}
                                        }
                                        break;

                                    case 'L':
                                        {
                                            bLWRLocked = !bLWRLocked;
                                            api.ZW_LockRoutes(bLWRLocked);
                                            mainlog_wr("LWR Lock %s", bLWRLocked ? "ON" : "OFF");
                                        }	
                                        break;

									case 'M':
										if(!RssiMapStart())
										{
											mainlog_wr("Cannot start RSSI mapping. It is already running.");
										}
										break;
									
									case '0':
                                    case 'Q':
                                        mainlog_wr("Stopping Network Health test", ch);
                                        NetworkManagement_NetworkHealth_MaintenanceStop();
                                        testStarted = false;
                                        testMode = 0;
                                        networkHealthTestMenu = 0;
                                        break;

									case 'B':
										ShowBackgroundRSSI();
										break;

                                    default:
                                        mainlog_wr("Network Health Test - Unknown commmand %c", ch);
                                        showHelp = 1;
                                        break;
                                }
                                if (currentTestMode != sNetworkManagementUT.bTestMode)
                                {
                                    mainlog_wr("Network Health Test Mode change from %u to %u", currentTestMode, sNetworkManagementUT.bTestMode);
                                    currentTestMode = sNetworkManagementUT.bTestMode;
                                }
                            }
                            printf("Leaving Network Health test menu\n");
                        }
                        break;

                    case 'R': // Remove
                        bAddRemoveNodeFromNetwork = !bAddRemoveNodeFromNetwork;
                        if (bAddRemoveNodeFromNetwork)
                        {
                            api.ZW_RemoveNodeFromNetwork(REMOVE_NODE_ANY, SetLearnNodeStateDelete_Compl);
                            mainlog_wr("ZW_RemoveNodeFromNetwork REMOVE_NODE_ANY|ADD_NODE_OPTION_NETWORK_WIDE");
                        }
                        else
                        {
                            api.ZW_RemoveNodeFromNetwork(REMOVE_NODE_STOP, SetLearnNodeStateDelete_Compl);
                            mainlog_wr("ZW_RemoveNodeFromNetwork REMOVE_NODE_STOP");
                        }
                        break;

                    case 'F':	/* remove Failed current nodeID */
                        {
							BYTE abFuncIDToSupport[] = {FUNC_ID_ZW_REMOVE_FAILED_NODE_ID};
                            if (NetworkManagement_IsFuncIDsSupported(abFuncIDToSupport, sizeof(abFuncIDToSupport)))
                            {
                                mainlog_wr("ZW_RemoveFailedNode supported by Z-Wave SerialAPI module");
                                mainlog_wr("ZW_RemoveFailedNode %03d", bNodeID);
                                BYTE retVal = api.ZW_RemoveFailedNode(bNodeID, RemoveFailedNode_Compl);
                                {
                                    switch (retVal)
                                    {
                                        case ZW_FAILED_NODE_REMOVE_STARTED:
                                            {
                                                mainlog_wr("ZW_RemoveFailedNode started on node %03d", bNodeID);
                                            }
                                            break;

                                        case ZW_NOT_PRIMARY_CONTROLLER:
                                            {
                                                mainlog_wr("ZW_RemoveFailedNode - ZW_NOT_PRIMARY_CONTROLLER");
                                            }
                                            break;

                                        case ZW_NO_CALLBACK_FUNCTION:
                                            {
                                                mainlog_wr("ZW_RemoveFailedNode - ZW_NO_CALLBACK_FUNCTION");
                                            }
                                            break;

                                        case ZW_FAILED_NODE_NOT_FOUND:
                                            {
                                                mainlog_wr("ZW_RemoveFailedNode - ZW_FAILED_NODE_NOT_FOUND");
                                            }
                                            break;

                                        case ZW_FAILED_NODE_REMOVE_PROCESS_BUSY:
                                            {
                                                mainlog_wr("ZW_RemoveFailedNode - ZW_FAILED_NODE_REMOVE_PROCESS_BUSY");
                                            }
                                            break;

                                        case ZW_FAILED_NODE_REMOVE_FAIL:
                                            {
                                                mainlog_wr("ZW_RemoveFailedNode - ZW_FAILED_NODE_REMOVE_FAIL");
                                            }
                                            break;

                                        default:
                                            {
                                                mainlog_wr("ZW_RemoveFailedNode failed - %d", retVal);
                                            }
                                            break;
                                    }
                                }
                            }
                            else
                            {
                                mainlog_wr("ZW_RemoveFailedNode Not supported by Z-Wave SerialAPI module");
                            }
                        }
                        break;

                    case 'L':	/* Enter LearnMode */
                        {
                            if (!learnMode_status)
                            {
                                api.ZW_SetLearnMode(true, SetLearnMode_Compl);
                                mainlog_wr("ZW_SetLearnMode enabling learnMode/receive");
                            }
                            else
                            {
                                api.ZW_SetLearnMode(false, SetLearnMode_Compl);
                                mainlog_wr("ZW_SetLearnMode disabling learnMode/receive");
                            }
                            learnMode_status = !learnMode_status;
                        }
                        break;

                    case '?':
                        mainlog_wr("ZW_RequestNetworkUpdate called...");
                        api.ZW_RequestNetworkUpdate(RequestNetworkUpdate_Compl);
                        break;

                    case 'D':	/* call ZW_SetDefault */
                        mainlog_wr("ZW_SetDefault called...");
                        api.ZW_SetDefault(SetDefault_Compl);
                        break;

                    case 'W':
                        {
                            mainlog_wr("ZW_SoftReset called (watchdog)...");
                            api.ZW_SoftReset(1);
                        }
                        break;

                    case 'X':
                        {
                            mainlog_wr("ZW_SoftReset called...");
                            api.ZW_SoftReset(0);
                        }
                        break;

                    case '.':
                        {
                            NODEINFO nodeInfo;

                            api.ZW_GetNodeProtocolInfo(bNodeID, &nodeInfo);
                            mainlog_wr("Get Protocol Info called for Node %u : Basic Nodetype = %u", bNodeID, nodeInfo.nodeType.basic);
                        }
                        break;

                    case '*':
                        {
                            static BYTE pData[3];
                            pData[0] = 32;
                            pData[1] = 1;
                            if (!pData[2])
                            {
                                pData[2] = 255;
                            }
                            else
                            {
                                pData[2] = 0;
                            }
                            mainlog_wr("ZW_SendData Basic Set %02X, NodeID %03d - ", (unsigned int)pData[2], bNodeID);
                            api.ZW_SendData(bNodeID, pData, 3, TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_EXPLORE, SendData_Compl);
                            mainlog_wr("Current NodeID now %03d", bNodeID);
                        }
                        break;

                    case '/':
                        {
                            mainlog_wr("ZW_SendNodeInformation Broadcast - start");
                            api.ZW_SendNodeInformation(255, 0, NULL);
                            mainlog_wr("ZW_SendNodeInformation Broadcast - done");
                        }
                        break;

                    case 'V': // Get version
                        {
                            int i;
                            BYTE buf[20];
                            BYTE libType = api.ZW_Version(buf);
                            for (i = 0; i < 15; i++)
                            {
                                if (buf[i] == 0)
                                {
                                    break;
                                }
                            }
                            if (buf[i] != 0)
                            {
                                buf[i] = 0;
                            }
							char *pLibTypeStr = GetValidLibTypeStr(libType);
                            mainlog_wr("Z-Wave %s protocol Version %s", pLibTypeStr, buf);
                        }
                        break;

                    case 'P':
                        {
                            BYTE pBuf[64];
                            char pCharBuf[256];
                            int bFuncCmd = 0;
                            mainlog_wr("Calling Serial_Get_Capabilities");
                            api.SerialAPI_Get_Capabilities(pBuf);
                            mainlog_wr("SERIAL_APPL_VERSION %02x, SERIAL_APPL_REVISION %02x", pBuf[0], pBuf[1]);
                            mainlog_wr("SERIALAPI_MANUFACTURER_ID1 %02x, SERIALAPI_MANUFACTURER_ID2 %02x", pBuf[2], pBuf[3]);
                            mainlog_wr("SERIALAPI_MANUFACTURER_PRODUCT_TYPE1 %02x, SERIALAPI_MANUFACTURER_PRODUCT_TYPE2 %02x", pBuf[4], pBuf[5]);
                            mainlog_wr("SERIALAPI_MANUFACTURER_PRODUCT_ID1 %02x, SERIALAPI_MANUFACTURER_PRODUCT_ID2 %02x", pBuf[6], pBuf[7]);
                            mainlog_wr("FUNCID_SUPPORTED_BITMASK[]");
                            for (int i = 8; i < 40; i++)
                            {
                                char cch = ' ';
                                sprintf_s(pCharBuf, sizeof(pCharBuf), "%02x [", pBuf[i]);
                                for (int j = 1, mask = 1; j <= 8; j++, mask <<= 1)
                                {
                                    if (mask & pBuf[i])
                                    {
                                        if (cch != ' ')
                                        {
                                            sprintf_s(&pCharBuf[strlen(pCharBuf)], sizeof(pCharBuf)-strlen(pCharBuf),"%c", cch);
                                        }
                                        sprintf_s(&pCharBuf[strlen(pCharBuf)], sizeof(pCharBuf)-strlen(pCharBuf), "%02X", bFuncCmd + j);
                                        cch = ',';
                                    }
                                }
                                sprintf_s(&pCharBuf[strlen(pCharBuf)], sizeof(pCharBuf) - strlen(pCharBuf), "]");
                                mainlog_wr(pCharBuf);
                                bFuncCmd += 8;
                            }
                        }
                        break;

                    case 'I': // Initialize nodeList with node information from device module
                        {
                            int numberOfNodes = 0;

                            mainlog_wr("Initialize nodeList with node info from device module");
                            ReloadNodeList(false);
                            for (int i = 0; i < ZW_MAX_NODES; i++)
                            {
                                if (nodeType[i])
                                {
                                    numberOfNodes++;
                                    mainlog_wr("Node %03u - %s", i, WriteNodeTypeString(nodeType[i]));
                                }
                            }
                            mainlog_wr("Total of %03u nodes in network", numberOfNodes);
                        }
                        break;

                    case 'U':
                        mainlog_wr("ZW_RequestNodeNeighborUpdate");
                        EnterNewCurrentNodeID();
                        api.ZW_RequestNodeNeighborUpdate(bNodeID, RequestNodeNeighborUpdate_Compl);
                        break;

                    case '!':
                        printf("Toggling incomming notifications ");
                        if (printIncomming == true)
                        {
                            printf("off\n");
                            printIncomming = false;
                        }
                        else
                        {
                            printf("on\n");
                            printIncomming = true;
                        }
                        break;

                    case '#':
                        EnterNewCurrentNodeID();
                        break;

                    case '"':
                        mainlog_wr("Request NodeInformation for node %u", bNodeID);
                        if (api.ZW_RequestNodeInfo(bNodeID, RequestNodeInfo_Compl))
                        {
                            mainlog_wr("ZW_RequestNodeInfo initiated for node %u", bNodeID);
                        }
                        else
                        {
                            mainlog_wr("ZW_RequestNodeInfo could not initiated for node %u", bNodeID);
                        }
                        break;

                    case '%':
                        DumpNodeInfo(bNodeID);
                        break;

                    case 'H': // Help
                    default :
                        printf("\n");
                        printf("A   : Add Node (only supported by Controller)\r\n");
                        printf("R   : Remove Node (only supported by Controller)\r\n");
                        printf("F   : remove Failed current nodeID %03d\r\n", bNodeID);
                        printf("U   : ZW_RequestNodeNeighborUpdate\r\n\n");
                        printf("N   : Network Health Test Menu\r\n\n");
                        printf("#   : Set current nodeID\r\n");
                        printf(".   : Call ZW_GetNodeProtocolInfo for current nodeID\r\n");
                        printf("\"   : Call ZW_RequestNodeInfo for current nodeID %03d\r\n", bNodeID);
                        printf("*   : ZW_SendData with explore to current NodeID (%03d)\r\n", bNodeID);
                        printf("%%   : Dump NodeInfo for current NodeID %03d\r\n", bNodeID);
                        printf("\\   : Broadcast Node Information frame\r\n");
                        printf("P   : Get serialapi capabilities\r\n");
                        printf("V   : get Version\r\n");
                        printf("I   : Initialize nodeList and dump nodeinfo (only supported by Controller)\r\n");
                        printf("L   : ZW_SetLearnMode\r\n");
                        printf("D   : ZW_SetDefault\r\n");
                        printf("W   : ZW_SoftReset\r\n");
                        printf("!   : Print incomming frames - %s\r\n", printIncomming ? "TRUE" : "FALSE");
                        printf("J   : Toggle serial logging to screen - %u\r\n\n", screenLog);
                        printf("Q   : Quit\r\n");
                        break;

                }
            }

        }
        api.Shutdown();
    }
	else
	{
		mainlog_wr("No port detected on serial port %s", portstr);
	}
    return 0;
}
