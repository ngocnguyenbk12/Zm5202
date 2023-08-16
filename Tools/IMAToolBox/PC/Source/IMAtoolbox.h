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
 * Description: Headerfile for the IMAtoolbox Z-Wave Network Health Test
 *
 * Last Changed By:  $Author: $ 
 * Revision:         $Rev: $
 * Last Changed:     $Date: $
 *
 ****************************************************************************/

#if !defined(_IMATOOLBOX_H_)
#define _IMATOOLBOX_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SerialAPI.h"
#include "logging.h"

#define IMATOOLBOX_VERSION_MAJOR		0
#define IMATOOLBOX_VERSION_MINOR		0
#define IMATOOLBOX_REVISION_MAJOR		9
#define IMATOOLBOX_REVISION_MINOR		9
#define IMATOOLBOX_RELEASE_TYPE			"a"

static BYTE bMaxNodeID;
static BYTE nodeState[ZW_MAX_NODES];
static BYTE nodeTypeList[] = 
	{0,									//No device
		GENERIC_TYPE_GENERIC_CONTROLLER,
		GENERIC_TYPE_STATIC_CONTROLLER, 
		GENERIC_TYPE_SWITCH_BINARY,
		GENERIC_TYPE_SWITCH_MULTILEVEL,
		GENERIC_TYPE_SENSOR_BINARY,
		GENERIC_TYPE_SENSOR_MULTILEVEL,
		GENERIC_TYPE_METER_PULSE,
		GENERIC_TYPE_ENTRY_CONTROL,
		0xFF							//Unkown device
	};


/* SerialAPIGetCapabilities structure definition */
typedef struct _S_SERIALAPI_CAPABILITIES
{
	BYTE bSerialAPIApplicationVersion;
	BYTE bSerialAPIApplicationRevision;
	BYTE bSerialAPIManufacturerID1;
	BYTE bSerialAPIManufacturerID2;
	BYTE bSerialAPIManufacturerProductType1;
	BYTE bSerialAPIManufacturerProductType2;
	BYTE bSerialAPIManufacturerProductID1;
	BYTE bSerialAPIManufacturerProductID2;
	BYTE abFuncIDSupportedBitmask[256 / 8];
} S_SERIALAPI_CAPABILITIES;

/* LWR definitions - Only valid for 4.5x static controllers */
#define LWR_OFFSET_4_54_02_STATIC_NOSUC_NOREP	(WORD)(0x10000 - 0x2C00 + 0x268E)
#define LWR_ENTRY_SIZE							5
#define LWR_ENTRY_OFFSET_REPEATER_1				0
#define LWR_ENTRY_OFFSET_REPEATER_2				1
#define LWR_ENTRY_OFFSET_REPEATER_3				2
#define LWR_ENTRY_OFFSET_REPEATER_4				3
#define LWR_ENTRY_OFFSET_CONF					4

#define LWR_DIRECT_ROUTE						0xFE

#define MAX_CHANNELS 3               /* 3 channels is maximum in Z-Wave */

extern BYTE bNodeExistMaskLen;
extern BYTE bNodeExistMask[ZW_MAX_NODEMASK_LENGTH];
extern BYTE MyNodeId;

extern CSerialAPI api;

/*================================ rssiToText ============================
** Function description
**      Take an RSSI BYTE and return a string of text.
**
**		Note: out pointer destination must be able to hold 5 chars.
**		
**--------------------------------------------------------------------------*/
void
rssiToText(
    char rssiVal, char *out);

/* Minimum size of output buffer for rssiToText */
#define RSSI_TO_TEXT_SIZE 5

/*==================================  mainlog_wr =============================
** Function description
**      mlog.Write wrapper for writing to mainlog
**
** Side effects:
**		
**--------------------------------------------------------------------------*/
void 
mainlog_wr(
    const char * fmt, ...);
#endif // !defined(_MYSERIALAPI_H_)
