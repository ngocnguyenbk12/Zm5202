/**
 * @file
 * Configuration file for Door Lock Key Pad.
 *
 * This file contains definitions for the Z-Wave+ Application Framework as well for the app itself.
 *
 * @copyright 2019 Silicon Laboratories Inc.
 */
#ifndef _CONFIG_APP_H_
#define _CONFIG_APP_H_

#ifdef __C51__
#include <ZW_product_id_enum.h>
#include <commandClassManufacturerSpecific.h>
#endif
#include <app_config_common.h>

/**
 * Defines device generic and specific types
 */
//@ [GENERIC_TYPE_ID]
#define GENERIC_TYPE GENERIC_TYPE_ENTRY_CONTROL
#define SPECIFIC_TYPE SPECIFIC_TYPE_SECURE_KEYPAD_DOOR_LOCK
//@ [GENERIC_TYPE_ID]

/**
 * See ZW_basic_api.h for ApplicationNodeInformation field deviceOptionMask
 */
//@ [DEVICE_OPTIONS_MASK_ID]
#define DEVICE_OPTIONS_MASK  (APPLICATION_FREQ_LISTENING_MODE_1000ms | APPLICATION_NODEINFO_OPTIONAL_FUNCTIONALITY)
//@ [DEVICE_OPTIONS_MASK_ID]

/**
 * Defines used to initialize the Z-Wave Plus Info Command Class.
 */
//@ [APP_TYPE_ID]
#define APP_ROLE_TYPE ZWAVEPLUS_INFO_REPORT_ROLE_TYPE_SLAVE_SLEEPING_LISTENING
#define APP_NODE_TYPE ZWAVEPLUS_INFO_REPORT_NODE_TYPE_ZWAVEPLUS_NODE
#define APP_ICON_TYPE ICON_TYPE_GENERIC_DOOR_LOCK_KEYPAD
#define APP_USER_ICON_TYPE ICON_TYPE_GENERIC_DOOR_LOCK_KEYPAD
//@ [APP_TYPE_ID]

/**
 * Defines used to initialize the Manufacturer Specific Command Class.
 */
#define APP_MANUFACTURER_ID     MFG_ID_SIGMA_DESIGNS

#define APP_PRODUCT_TYPE_ID     PRODUCT_TYPE_ID_ZWAVE_PLUS
#define APP_PRODUCT_ID          PRODUCT_ID_DoorLockKeyPad

#define APP_FIRMWARE_ID         APP_PRODUCT_ID | (APP_PRODUCT_TYPE_ID << 8)

#define APP_DEVICE_ID_TYPE          DEVICE_ID_TYPE_SERIAL_NBR
#define APP_DEVICE_ID_FORMAT        DEVICE_ID_FORMAT_BIN

/**
 * Defines used to initialize the Association Group Information (AGI)
 * Command Class.
 */
#define NUMBER_OF_ENDPOINTS         0
#define MAX_ASSOCIATION_GROUPS      2
#define MAX_ASSOCIATION_IN_GROUP    5

//@ [AGI_TABLE_ID]
#define AGITABLE_LIFELINE_GROUP \
  {COMMAND_CLASS_BATTERY, BATTERY_REPORT}, \
  {COMMAND_CLASS_DOOR_LOCK_V2, DOOR_LOCK_OPERATION_REPORT_V2}, \
  {COMMAND_CLASS_DEVICE_RESET_LOCALLY, DEVICE_RESET_LOCALLY_NOTIFICATION}

#define  AGITABLE_ROOTDEVICE_GROUPS \
 {ASSOCIATION_GROUP_INFO_REPORT_PROFILE_CONTROL, ASSOCIATION_GROUP_INFO_REPORT_PROFILE_CONTROL_KEY01, {COMMAND_CLASS_DOOR_LOCK_V2, DOOR_LOCK_OPERATION_REPORT_V2},"Doorlock Op"}
//@ [AGI_TABLE_ID]

//@ [DEFAULT_USERCODE_ID]
/**
 * Door lock key pad default user code
 */
#define DEFAULT_USERCODE {'0', '0', '0', '0'}
//@ [DEFAULT_USERCODE_ID]


/**
 * PowerDownTimeout determines the number of seconds before powerdown.
 */
#define KEY_SCAN_POWERDOWNTIMEOUT     3  /**< 300 mSec*/
#define MSEC_200_POWERDOWNTIMEOUT     2  /*200 mSec*/
#define SEC_2_POWERDOWNTIMEOUT       20  /**<  2 sec*/
#define SEC_10_POWERDOWNTIMEOUT     100  /**< 10 sec*/
#define LEARNMODE_POWERDOWNTIMEOUT  255  /**< 25 sec*/
#define KEY_CODE_TIMEOUT            100  /*10 sec*/
#define KEY_CODE_TIMEOUT_STOP         1  /*100 mSec*/

#define FIRMWARE_UPGRADABLE        0xFF  /**< 0x00 = Not upgradable, 0xFF = Upgradable*/

/**
 * CLIENT SIDE AUTHENTIFICATION
 */

//@ [SECURITY_AUTHENTICATION_ID]
/*
 * This definition must be set in order for the application to handle CSA. It is used only in the
 * application.
 */
#define APP_SUPPORTS_CLIENT_SIDE_AUTHENTICATION

/*
 * This definition tells the protocol whether the application uses CSA or not.
 * It can be set to one of the two following values:
 * - SECURITY_AUTHENTICATION_SSA
 * - SECURITY_AUTHENTICATION_CSA
 */
#ifdef AUTH_TYPE_CSA
#define REQUESTED_SECURITY_AUTHENTICATION SECURITY_AUTHENTICATION_CSA
#endif
#ifdef AUTH_TYPE_SSA
#define REQUESTED_SECURITY_AUTHENTICATION SECURITY_AUTHENTICATION_SSA
#endif
//@ [SECURITY_AUTHENTICATION_ID]

/**
 * Security keys
 */
//@ [REQUESTED_SECURITY_KEYS_ID]
#define REQUESTED_SECURITY_KEYS ( SECURITY_KEY_S0_BIT | SECURITY_KEY_S2_ACCESS_BIT)
//@ [REQUESTED_SECURITY_KEYS_ID]

#endif /* _CONFIG_APP_H_ */


