/**
 * @file config_app.h
 * @brief Configuration file for Switch On/Off sample application.
 * @copyright Copyright (c) 2001-2017
 * Sigma Designs, Inc.
 * All Rights Reserved
 * @details This file contains definitions for the Z-Wave+ Framework as well for the sample app.
 *
 * NOTICE: The file name must not be changed and the two definitions APP_VERSION and APP_REVISION
 * must not be changed since they are used by the build environment.
 */
#ifndef _CONFIG_APP_H_
#define _CONFIG_APP_H_

#ifdef __C51__
#include <ZW_product_id_enum.h>
#include <commandClassManufacturerSpecific.h>
#include <agi.h>
#endif
#include <app_config_common.h>

/**
 * Defines device generic and specific types
 */
//@ [GENERIC_TYPE_ID]
#define GENERIC_TYPE GENERIC_TYPE_SWITCH_BINARY
#define SPECIFIC_TYPE SPECIFIC_TYPE_POWER_SWITCH_BINARY
//@ [GENERIC_TYPE_ID]

/**
 * See ZW_basic_api.h for ApplicationNodeInformation field deviceOptionMask
 */
//@ [DEVICE_OPTIONS_MASK_ID]
#define DEVICE_OPTIONS_MASK   APPLICATION_NODEINFO_LISTENING | APPLICATION_NODEINFO_OPTIONAL_FUNCTIONALITY
//@ [DEVICE_OPTIONS_MASK_ID]

/**
 * Defines used to initialize the Z-Wave Plus Info Command Class.
 */
//@ [APP_TYPE_ID]
#define APP_ROLE_TYPE 		ZWAVEPLUS_INFO_REPORT_ROLE_TYPE_SLAVE_ALWAYS_ON
#define APP_NODE_TYPE		ZWAVEPLUS_INFO_REPORT_NODE_TYPE_ZWAVEPLUS_NODE

#define APP_ICON_TYPE 		ICON_TYPE_GENERIC_ON_OFF_POWER_SWITCH
#define APP_USER_ICON_TYPE 	ICON_TYPE_GENERIC_ON_OFF_POWER_SWITCH
//@ [APP_TYPE_ID]
#define ENDPOINT_ICONS \
{ICON_TYPE_GENERIC_ON_OFF_POWER_SWITCH, ICON_TYPE_GENERIC_ON_OFF_POWER_SWITCH},\
{ICON_TYPE_GENERIC_ON_OFF_POWER_SWITCH, ICON_TYPE_GENERIC_ON_OFF_POWER_SWITCH},\

	
/**
 * Defines used to initialize the Manufacturer Specific Command Class.
 */
#define APP_MANUFACTURER_ID     MFG_ID_SIGMA_DESIGNS

#define APP_PRODUCT_TYPE_ID     PRODUCT_TYPE_ID_ZWAVE_PLUS
#define APP_PRODUCT_ID          PRODUCT_ID_SwitchOnOff

#define APP_FIRMWARE_ID         APP_PRODUCT_ID | (APP_PRODUCT_TYPE_ID << 8)

#define APP_DEVICE_ID_TYPE      DEVICE_ID_TYPE_SERIAL_NBR
#define APP_DEVICE_ID_FORMAT    DEVICE_ID_FORMAT_BIN

/**
 * Defines used to initialize the Association Group Information (AGI)
 * Command Class.
 */
#define NUMBER_OF_INDIVIDUAL_ENDPOINTS    2
#define NUMBER_OF_AGGREGATED_ENDPOINTS    0
#define NUMBER_OF_ENDPOINTS         NUMBER_OF_INDIVIDUAL_ENDPOINTS + NUMBER_OF_AGGREGATED_ENDPOINTS
#define MAX_ASSOCIATION_GROUPS      1
#define MAX_ASSOCIATION_IN_GROUP    5

//@ [AGI_TABLE_ID]
#define AGITABLE_LIFELINE_GROUP \
	{COMMAND_CLASS_DEVICE_RESET_LOCALLY, DEVICE_RESET_LOCALLY_NOTIFICATION}, \
	{COMMAND_CLASS_BASIC, BASIC_REPORT},	\
	{COMMAND_CLASS_SWITCH_BINARY, SWITCH_BINARY_REPORT}

#define AGITABLE_LIFELINE_GROUP_ENDPOINTS	\
	{COMMAND_CLASS_BASIC, BASIC_REPORT},	\
	{COMMAND_CLASS_SWITCH_BINARY, SWITCH_BINARY_REPORT},\
	{COMMAND_CLASS_MULTI_CHANNEL_V3, MULTI_CHANNEL_CMD_ENCAP_V3}

#define  AGITABLE_ROOTDEVICE_GROUPS NULL
//@ [AGI_TABLE_ID]


/**
 * PowerDownTimeout determines the number of seconds before power down.
 */
#define KEY_SCAN_POWERDOWNTIMEOUT     3  /**< 300 mSec*/
#define MSEC_200_POWERDOWNTIMEOUT     2  /*200 mSec*/
#define SEC_2_POWERDOWNTIMEOUT       20  /**<  2 sec*/
#define SEC_10_POWERDOWNTIMEOUT     100  /**< 10 sec*/
#define LEARNMODE_POWERDOWNTIMEOUT  255  /**< 25 sec*/

#define FIRMWARE_UPGRADABLE        0xFF  /**< 0x00 = Not upgradable, 0xFF = Upgradable*/

//@ [SECURITY_AUTHENTICATION_ID]
/*
 * This definition must be set in order for the application to handle CSA. It is used only in the
 * application.
 */
// #define APP_SUPPORTS_CLIENT_SIDE_AUTHENTICATION

/*
 * This definition tells the protocol whether the application uses CSA or not.
 * It can be set to one of the two following values:
 * - SECURITY_AUTHENTICATION_SSA
 * - SECURITY_AUTHENTICATION_CSA
 */
#define REQUESTED_SECURITY_AUTHENTICATION SECURITY_AUTHENTICATION_SSA
//@ [SECURITY_AUTHENTICATION_ID]

/**
 * Security keys
 */
//@ [REQUESTED_SECURITY_KEYS_ID]
#define REQUESTED_SECURITY_KEYS_SECURITY_KEY_NON_MASK // (SECURITY_KEY_S0_BIT | SECURITY_KEY_S2_UNAUTHENTICATED_BIT)
//@ [REQUESTED_SECURITY_KEYS_ID]

#endif /* _CONFIG_APP_H_ */

