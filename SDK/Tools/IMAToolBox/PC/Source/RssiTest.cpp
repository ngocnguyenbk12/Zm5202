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
 * Description:      RSSI test module for Z-Wave Network Health Toolbox
 *                   Functions for testing if a Last Working Route has the
 *                   required RSSI margin.
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
#include <stdlib.h>
#include <RssiTest.h>
#include <RssiMap.h>
#include <IMAtoolbox.h>
#include <networkManagement.h>
#include <SerialAPI.h>
#include <LIMITS.H>

/****************************************************************************/
/*                           TYPES AND DEFINITIONS                          */
/****************************************************************************/
#define RSSI_TEST_MARGIN_REQUIRED 17 /* minimum required RSSI margin [dB] */
#define NUM_RSSI_BACKGR_SAMPLES 10   /* Number of backrgound RSSI samples to
                                        average*/
#define LINK_MARGIN_MISSING 255      /* Link margin assigned when RSSI values
										are not available (e.g. due to older
										repeater nodes */

/****************************************************************************/
/*                           PRIVATE VARIABLES                              */
/****************************************************************************/
static BOOL fIs3ChannelSystem;

/****************************************************************************/
/*                          PRIVATE PROTOTYPES                              */
/****************************************************************************/

/*========================   rssi2dBm   =========================
** Function description
**   Callbak for the RSSI transmission.
** 
**--------------------------------------------------------------------------*/
signed char rssi2dBm(signed char val)
{
	if (val == RSSI_MAX_POWER_SATURATED)	
	{
		return 1; /* Treat saturated as 1 dBm */
	}
	else if (val == RSSI_BELOW_SENSITIVITY)
	{
		return -100; 
	}
	else if(val == RSSI_NOT_AVAILABLE)
	{
		return val; /* making it explicit that we don't change NOT_AVAILABLE */
	}
	else
	{
		return val;
	}
}

/*========================   FailRssiTest()   =========================
** Function description
** RSSI test has failed. Update Network Health value accordingly.
** The new value depends on the old value
** 
**--------------------------------------------------------------------------*/
void 
FailRssiTest()
{
    /* Failed RSSI Test - Degrade */
    if (nodeStats->NetworkHealthNumber > 8)
    {
        /* Was a 9 or 10 and due to failed RSSI test NH is degraded to 7 */
        nodeStats->NetworkHealthNumber = 7;
    }
    else if (8 == nodeStats->NetworkHealthNumber)
    {
        /* Was an 8 and due to failed RSSI test NH is degraded to 6 */
        nodeStats->NetworkHealthNumber = 6;
    }
    NetworkManagement_UpdateTrafficLight(nodeStats);
}


/*========================   RSSI_test_callback   =========================
** Function description
** Callback for the RSSI transmission.
** 
**--------------------------------------------------------------------------*/
void 
RSSI_test_callback(
    BYTE bTxStatus, 
	TX_STATUS_TYPE *psTxInfo)
{
	BYTE i;
	char fRssiNotAvailable = false;
	signed char current_backgr;
	signed char tmp_rssi;
	char *formatStrings[5];
	
	/* Assume RSSI test failed if the probe transmission fails */
	if (TRANSMIT_COMPLETE_OK != bTxStatus)
	{
		mainlog_wr("RSSI test transmission failed with status %d", bTxStatus);
		FailRssiTest();
		NextNetworkHealthTestRound();
		return;
	}
	
	/* determine background channel to use */
	if (fIs3ChannelSystem)
	{
		/* find minimum background value */
		current_backgr = SCHAR_MAX;
		for (i = 0; i < 3; i++)
		{
			tmp_rssi = rssi2dBm(nodeStats->backgroundRssi[i]);
		    if (tmp_rssi < current_backgr)
			{
				current_backgr = tmp_rssi;
			}
		}
	}
	else
	{
		current_backgr = nodeStats->backgroundRssi[psTxInfo->bACKChannelNo];
	}
	printf("current backgr is %d (3ch: %u)\n", current_backgr, fIs3ChannelSystem);
	for (i = 0; i < psTxInfo->bRepeaters + 1; i++)
	{
		signed char rssi_val;
		
		if(psTxInfo->rssiVal[i] == RSSI_NOT_AVAILABLE)
		{
			/* pretend missing rssi values are within specifications,
			   but emit warning after completing */
			fRssiNotAvailable = true;
			nodeStats->linkMargins[i] = LINK_MARGIN_MISSING;
			continue;
		}
		rssi_val = rssi2dBm(psTxInfo->rssiVal[i]);

		nodeStats->linkMargins[i] = rssi_val - current_backgr;
	}

	formatStrings[0] =	"RSSI margin: %4d dB";
	formatStrings[1] =  "RSSI margin: %4d %4d  dB";
	formatStrings[2] =	"RSSI margin: %4d %4d %4d dB";
	formatStrings[3] =	"RSSI margin: %4d %4d %4d %4d dB";
	formatStrings[4] =	"RSSI margin: %4d %4d %4d %4d %4d dB";
	mainlog_wr(formatStrings[psTxInfo->bRepeaters], nodeStats->linkMargins[0], 
		nodeStats->linkMargins[1], nodeStats->linkMargins[2], 
		nodeStats->linkMargins[3],nodeStats->linkMargins[4]);

	if(fRssiNotAvailable)
	{
		mainlog_wr("Warning: RSSI information missing for some links.");
	}
	for (i = 0; i < psTxInfo->bRepeaters + 1; i++)
	{
		if (nodeStats->linkMargins[i] < RSSI_TEST_MARGIN_REQUIRED)
		{
			FailRssiTest();
		}
	}
	NextNetworkHealthTestRound();
}

/*======================= compfunc ============================
** Function description
**      Comparison function for the qsort() library
**      function. Compares two signed chars
**		
** Side effects:
**
**-------------------------------------------------------------*/
int compfunc(const void * elem1, const void * elem2) 
{
    int f = *((signed char*)elem1);
    int s = *((signed char*)elem2);
    if (f > s) return  1;
    if (f < s) return -1;
    return 0;
}


/*=========================== usleep ============================
** Function description
**      Platform-specific sleep method for microseconds.
**      
** Supply your own platform specific version when porting.
**
** Side effects:
**
**---------------------------------------------------------------*/
void usleep(int usec) 
{
	Sleep(usec / 1000);
}

/*======================= CollectBackgroundRssi ============================
** Function description
**      Sample and average background RSSI samples.
**      Samples rssi 10 times 50 ms apart. Discard the 5 highest samples
**      for each channel. Average the remaining samples for each channel.
**		
** Side effects:
**   Detect if the Z-Wave network is 2- or 3-channel and return the result
**   in numChannels paramter.
**
**--------------------------------------------------------------------------*/
void
CollectBackgroundRssi(signed char abBackgrRssi[], BYTE *numChannels)
{
	signed char samples[MAX_CHANNELS][NUM_RSSI_BACKGR_SAMPLES];
	signed char tempBuf[MAX_CHANNELS];
	BYTE sampleRound;
	BYTE _numChannels;
	BYTE ch;
	BYTE reducedSampleCount;
	int cumSamples[MAX_CHANNELS];
	
	mainlog_wr("Background RSSI sampling starting...");
	/* Sample NUM_RSSI_BACKGR_SAMPLES rounds of background rssi */
	for (sampleRound = 0; sampleRound < NUM_RSSI_BACKGR_SAMPLES; sampleRound++)
	{
		//mainlog_wr("starting sleep");
		usleep(50*1000); /* Wait for previous transmissions to cool down */
		//mainlog_wr("sleep done");
		api.ZW_GetBackgroundRSSI((BYTE*)&tempBuf, &_numChannels);
		//mainlog_wr("GetBackgroundRssi done");
		for (ch = 0; ch < _numChannels; ch++)
		{
			samples[ch][sampleRound] = rssi2dBm(tempBuf[ch]);
		}
	}
	mainlog_wr("Sampling done.");
	*numChannels = _numChannels;
	/* Number of samples to keep (after discarding highest values) */
	reducedSampleCount = (NUM_RSSI_BACKGR_SAMPLES + 1) / 2;
	for (ch = 0; ch < _numChannels; ch++)
	{
		/* sort the samples to find the low 50% values */
		qsort(samples[ch], NUM_RSSI_BACKGR_SAMPLES, sizeof(signed char), compfunc);
		/* sum up the low values*/
		cumSamples[ch] = 0;
		for (int i = 0; i < reducedSampleCount; i++)
		{
			cumSamples[ch] += samples[ch][i];
		}
		/* calulate the channel average and store the result*/
		abBackgrRssi[ch] = cumSamples[ch] / reducedSampleCount;
	}
}

/*=========================== PerformRSSITest ===============================
** Function description
**      Perform the RSSI test step for the node currently undergoing
**  Network Health Test.
**		
** Side effects:
**
**--------------------------------------------------------------------------*/
void
PerformRSSITest(void)
{
	BYTE retVal;
	/* Speed optimization. Background RSSI sampling takes 5+ seconds
	   due to latency in the imatoolbox serialapi layer
	   As a workaround, we only sample the background on the first testround. */
	if (nodeStats->testRounds == 0)
	{
		BYTE numChannels;
		CollectBackgroundRssi(nodeStats->backgroundRssi, &numChannels);
		if (3 == numChannels)
		{
			fIs3ChannelSystem = true;
		}
		else
		{
			fIs3ChannelSystem = false;
		}
	}
	mainlog_wr("Background RSSI: %4d %4d %4d", 
		rssi2dBm(nodeStats->backgroundRssi[0]), rssi2dBm(nodeStats->backgroundRssi[1]),
		rssi2dBm(nodeStats->backgroundRssi[2]));

	/* Send a frame to destination to collect RSSI information */
	BYTE nop_cmd[] = {0};
	retVal = api.ZW_SendData(nodeStats->bDestNodeID, (BYTE*)&nop_cmd, sizeof(nop_cmd),
                                      TRANSMIT_OPTION_ACK,
                                      RSSI_test_callback);
    if(!retVal)
	{
		mainlog_wr("Could not send RSSI test frame. Skipping RSSI test.");
		NextNetworkHealthTestRound();
	}
}

/*=========================== ShowBackgroundRSSI ===============================
** Function description
**      Obtain and print background rssi for all channels
**		
** Side effects:
**
**--------------------------------------------------------------------------*/
void ShowBackgroundRSSI(void)
{
	BYTE rssiLength;
	BYTE i;
	signed char buf[3];
	api.ZW_GetBackgroundRSSI((BYTE*)buf, &rssiLength);
	for (i = 0; i < rssiLength; i++)
	{
		buf[i] = rssi2dBm(buf[i]);
	}
	if (3 == rssiLength)
	{
		mainlog_wr("Background rssi: %4d %4d %4d", buf[0], buf[1], buf[2]);
	}
	else
	{
		mainlog_wr("Background rssi: %4d %4d", buf[0], buf[1]);
	}
}