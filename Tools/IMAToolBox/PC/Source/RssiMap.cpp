/****************************************************************************
 *
 * Z-Wave, the wireless language.
 * 
 * Copyright (c) 2014
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 * This source file is subject to the terms and conditions of the
 * Sigma Designs Software License Agreement which restricts the manner
 * in which it may be used.
 *
 *---------------------------------------------------------------------------
 *
 * Description:      RSSI map module for Z-Wave Network Health Toolbox
 *
 * Last Changed By:  $Author$: 
 * Revision:         $Rev$: 
 * Last Changed:     $Date$: 
 *
 ****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <stdafx.h>
#include <RssiMap.h>
#include <IMAtoolbox.h>
#include <RssiTest.h>
#include <LIMITS.H>

/****************************************************************************/
/*                           TYPES AND DEFINITIONS                          */
/****************************************************************************/
enum { INVALID_NODE_ID = 0xFF };

enum { STATE_IDLE, STATE_BUSY } bRssiMapState = STATE_IDLE;

/****************************************************************************/
/*                           PRIVATE VARIABLES                              */
/****************************************************************************/
BYTE fRssiMapAbortRequested = false;
static BYTE bRssiMapCurrentNode;

// Global state vars for the bitmask iterator.
static BYTE iter_i, iter_j;

signed char abBackgrRssi[MAX_CHANNELS];
BYTE numChannels;

/****************************************************************************/
/*                          PRIVATE PROTOTYPES                              */
/****************************************************************************/
bool RssiMap_NextNode(bool fRestart);

/*============================== DoRssiMap =============================
** Function description
**     Callback function for RssiMap feature.
**
**	   This is a ZW_SendData callback that delivers hop-by-hop RSSI
**	   values for use in the RSSI map. It also stamrts RSSI measurement
**	   for the next node.
**     
** Side effects:
**
**--------------------------------------------------------------------------*/
void RssiMap_cb(BYTE txStatus, TX_STATUS_TYPE *psTxInfo)
{
	char msgbuf[400];
	char rssi_val_str[5][RSSI_TO_TEXT_SIZE]; /* char buffer text encoding of rssi samples */
	BYTE dummy_hops[5];
	char *formatStrings1[5];
	char *formatStrings2[5];
	char *formatStrings3[5];
	char *rssiVal;
	signed char rssi_margin[ZW_MAX_REPEATERS + 1];
	signed char current_backgr;
	rssiVal = (char*)psTxInfo->rssiVal; /* re-interpret unsigned char as signed */

	formatStrings1[0] = "  %03u ----> %03u";
	formatStrings1[1] = "  %03u ----> %03u ----> %03u";
	formatStrings1[2] = "  %03u ----> %03u ----> %03u ----> %03u";
	formatStrings1[3] = "  %03u ----> %03u ----> %03u ----> %03u ----> %03u";
	formatStrings1[4] = "  %03u ----> %03u ----> %03u ----> %03u ----> %03u ----> %03u";
	
	formatStrings2[0] =	"RSSI: %4s      dBm";
	formatStrings2[1] = "RSSI: %4s      %4s  dBm";
	formatStrings2[2] =	"RSSI: %4s      %4s      %4s  dBm";
	formatStrings2[3] =	"RSSI: %4s      %4s      %4s      %4s  dBm";
	formatStrings2[4] =	"RSSI: %4s      %4s      %4s      %4s      %4s  dBm";

	formatStrings3[0] =	"SNR : %4d      dB";
	formatStrings3[1] = "SNR : %4d      %4d  dB";
	formatStrings3[2] =	"SNR : %4d      %4d      %4d  dB";
	formatStrings3[3] =	"SNR : %4d      %4d      %4d      %4d  dB";
	formatStrings3[4] =	"SNR : %4d      %4d      %4d      %4d      %4d  dB";

	memset(dummy_hops, 0xFF, sizeof(dummy_hops));
	mainlog_wr("LWR between Node %03u and Node %03u", MyNodeId, bRssiMapCurrentNode);
	if (psTxInfo->bRepeaters > 4) /* sanity check */
	{
		mainlog_wr("Invalid repeater count");
		txStatus = TRANSMIT_COMPLETE_NO_ACK;
	}
	if (TRANSMIT_COMPLETE_OK != txStatus)
	{
		mainlog_wr("Transmission failed. Aborting.", msgbuf);
		fRssiMapAbortRequested = true;
	}
	else
	{
		/* Print hop list */
		dummy_hops[0] = MyNodeId;
		for (BYTE i = 0; i <= psTxInfo->bRepeaters; i++)
		{
			dummy_hops[i+1] = psTxInfo->pLastUsedRoute[i];
		}
		dummy_hops[psTxInfo->bRepeaters + 1] = bRssiMapCurrentNode;
		sprintf_s(msgbuf, sizeof(msgbuf), formatStrings1[psTxInfo->bRepeaters],
			dummy_hops[0], dummy_hops[1], dummy_hops[2], dummy_hops[3],
			dummy_hops[4]);
		mainlog_wr("%s", msgbuf);

		if (numChannels == 3)
		{
			signed char tmp_rssi;
			/* find minimum background value */
			current_backgr = SCHAR_MAX;
			for (BYTE i = 0; i < 3; i++)
			{
				tmp_rssi = rssi2dBm(abBackgrRssi[i]);
				if (tmp_rssi < current_backgr)
				{
					current_backgr = tmp_rssi;
				}
			}
		}
		else
		{
			current_backgr = rssi2dBm(abBackgrRssi[psTxInfo->bACKChannelNo]);
		}
		/* Convert rssi value from each hop to string */
		for (BYTE i = 0; i <= psTxInfo->bRepeaters; i++)
		{
			rssiToText(rssiVal[i], rssi_val_str[i]);
			rssi_margin[i] = rssi2dBm(rssiVal[i]) - current_backgr;
		}
		/* Print rssi values */
		mainlog_wr(formatStrings2[psTxInfo->bRepeaters], 
			rssi_val_str[0], rssi_val_str[1], rssi_val_str[2], rssi_val_str[3],
			rssi_val_str[4]);
		/* Print RSSI margins */
		mainlog_wr(formatStrings3[psTxInfo->bRepeaters], rssi_margin[0], rssi_margin[1], 
			rssi_margin[2], rssi_margin[3],	rssi_margin[4]);

	}
	if (fRssiMapAbortRequested)
	{
		bRssiMapCurrentNode = INVALID_NODE_ID;
		mainlog_wr("RSSI map aborted");
		bRssiMapState = STATE_IDLE;
		return;
	}
	/* Proceed to next node */
	if(!RssiMap_NextNode(false))
	{
		mainlog_wr("RSSI map complete. All nodes processed.");
	}
}

/*============================== DoRssiMap =============================
** Function description
**     Measure the RSSI map to a node and print the result.
**
**     Called from RssiMap_NextNode() when measuring all nodes in network.
**     
** Side effects:
**		Increments the local static variables i and j to keep track of next
**    node.
**
**--------------------------------------------------------------------------*/
void DoRssiMap(BYTE bNode)
{
	BYTE retVal;
	BYTE NOP_cmd[] = {COMMAND_CLASS_NO_OPERATION};
	
	bRssiMapCurrentNode = bNode;
	retVal = api.ZW_SendData(bNode, NOP_cmd, sizeof(NOP_cmd), TRANSMIT_OPTION_ACK, RssiMap_cb);
	if (0 == retVal)
	{
		RssiMap_cb(TRANSMIT_COMPLETE_NO_ACK, 0);
	}
}

/*========================== ResetNodeMaskIterator ===========================
** Function description
**     Resets the iterator over the bNodeExistMask bitmask.
**     After calling this function, NodeMaskIterator_Next will start over
**     and return all nodes in the bitmask, one at a time.
**     
** Side effects:
**		Zero the global variables iter_i and iter_j to keep track 
**      of next node.
**
**--------------------------------------------------------------------------*/
void ResetNodeMaskIterator()
{
	iter_i = 0;
	iter_j = 0;
}

/*========================== NodeMaskIterator_Next ===========================
** Function description
**     Iterate over the bNodeExistMask bitmask and return the next nodeid.
**     When called again will return the subsequent nodeIDs.
**
**     To start over with the first existing nodeID call 
**     ResetNodeMaskIterator().
**
**     Returns INVALID_NODE_ID when all existing nodes in the bitmask have
**     been traversed.
**     
** Side effects:
**		Increments the global variables iter_i and iter_j to keep track 
**      of next node.
**
**--------------------------------------------------------------------------*/
BYTE NodeMaskIterator_Next()
{
	BYTE nid;

	while(iter_i < bNodeExistMaskLen)
	{
		if (0 == bNodeExistMask[iter_i])
        {
			iter_i++;
			iter_j = 0;
			continue;
        }
		while(iter_j < 8)
		{
			if (bNodeExistMask[iter_i] & (1 << iter_j))
            {
				nid = 1 + iter_i * 8 + iter_j;
				iter_j++;
				return nid;
			}
			iter_j++;
		}
		iter_i++;
	}
	return INVALID_NODE_ID;
}


/*============================== RssiMap_NextNode =============================
** Function description
**     Measure the RSSI map to next node and print the result.
**     Call again with fRestart = FALSE to iterate over all nodes in the network
**     Call again with fRestart = True to start over from the first node in 
**     the network. 
**
**     Returns FALSE when the last node has been measured.
**     
** Side effects:
**		Increments the static variables iter_i and iter_j to keep track of next
**    node.
**
**--------------------------------------------------------------------------*/
bool RssiMap_NextNode(bool fRestart)
{
    static BYTE i = 0;
    static BYTE j = 0;
    BYTE bCurNode;
    
    if (fRestart)
    {
		ResetNodeMaskIterator();
    }
    // Traverse all nodes
	bCurNode = NodeMaskIterator_Next();
	if(bCurNode == MyNodeId)
	{
		bCurNode = NodeMaskIterator_Next();
	}
	if(INVALID_NODE_ID == bCurNode)
	{
		bRssiMapState = STATE_IDLE;
		return FALSE;
	}
	DoRssiMap(bCurNode);
	return TRUE;
}


/*============================== RssiMapStart =============================
** Function description
**     Start the RSSI map functionality. Begin measuring and displaying an
**     RSSI map of the entire network.
**     First measures and displays
**
**     Returns false if an rssi map operation is already ongoing.
**     
**--------------------------------------------------------------------------*/
bool RssiMapStart(void)
{
	char rssi_val_str[MAX_CHANNELS][RSSI_TO_TEXT_SIZE]; /* char buffer text encoding of rssi samples */
	if (STATE_IDLE != bRssiMapState)
	{
		return false;
	}
	bRssiMapState = STATE_BUSY;

	CollectBackgroundRssi(abBackgrRssi, &numChannels);
	rssiToText(abBackgrRssi[0], rssi_val_str[0]);
	rssiToText(abBackgrRssi[1], rssi_val_str[1]);
	if (numChannels == 3)
	{
		rssiToText(abBackgrRssi[2], rssi_val_str[2]);
		mainlog_wr("Background RSSI levels: %4s %4s %4s dBm", rssi_val_str[0], rssi_val_str[1], rssi_val_str[2]);
	}
	else
	{
		mainlog_wr("Background RSSI levels: %4s %4s dBm", rssi_val_str[0], rssi_val_str[1]);
	}
	RssiMap_NextNode(true);
	return true;
}