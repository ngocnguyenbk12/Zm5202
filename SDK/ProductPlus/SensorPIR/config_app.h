/**
 * @file config_app.h
 * @brief Configuration file for Sensor PIR sample application.
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
#endif
#include <app_config_common.h>

/**
 * Defines device generic and specific types
 */
//@ [GENERIC_TYPE_ID]
#define GENERIC_TYPE GENERIC_TYPE_SENSOR_NOTIFICATION
#define SPECIFIC_TYPE SPECIFIC_TYPE_NOTIFICATION_SENSOR
//@ [GENERIC_TYPE_ID]

/**
 * See ZW_basic_api.h for ApplicationNodeInformation field deviceOptionMask
 */
//@ [DEVICE_OPTIONS_MASK_ID]
#define DEVICE_OPTIONS_MASK   APPLICATION_NODEINFO_NOT_LISTENING | APPLICATION_NODEINFO_OPTIONAL_FUNCTIONALITY
//@ [DEVICE_OPTIONS_MASK_ID]

/**
 * Defines used to initialize the Z-Wave Plus Info Command Class.
 */
//@ [APP_TYPE_ID]
#define APP_ROLE_TYPE ZWAVEPLUS_INFO_REPORT_ROLE_TYPE_SLAVE_SLEEPING_REPORTING
#define APP_NODE_TYPE ZWAVEPLUS_INFO_REPORT_NODE_TYPE_ZWAVEPLUS_NODE
#define APP_ICON_TYPE ICON_TYPE_SPECIFIC_SENSOR_NOTIFICATION_HOME_SECURITY
#define APP_USER_ICON_TYPE ICON_TYPE_SPECIFIC_SENSOR_NOTIFICATION_HOME_SECURITY
//@ [APP_TYPE_ID]


/**
 * Defines used to initialize the Manufacturer Specific Command Class.
 */
#define APP_MANUFACTURER_ID     MFG_ID_SIGMA_DESIGNS

#define APP_PRODUCT_TYPE_ID     PRODUCT_TYPE_ID_ZWAVE_PLUS
#define APP_PRODUCT_ID          PRODUCT_ID_SensorPIR

#define APP_FIRMWARE_ID         APP_PRODUCT_ID | (APP_PRODUCT_TYPE_ID << 8)

#define APP_DEVICE_ID_TYPE      DEVICE_ID_TYPE_SERIAL_NBR
#define APP_DEVICE_ID_FORMAT    DEVICE_ID_FORMAT_BIN

/**
 * Defines used to initialize the Association Group Information (AGI)
 * Command Class.
 */
#define NUMBER_OF_ENDPOINTS         0
#define MAX_ASSOCIATION_GROUPS      2
#define MAX_ASSOCIATION_IN_GROUP    1

//@ [AGI_TABLE_ID]
#define AGITABLE_LIFELINE_GROUP \
  {COMMAND_CLASS_BATTERY, BATTERY_REPORT}, \
  {COMMAND_CLASS_NOTIFICATION_V3, NOTIFICATION_REPORT_V3}, \
  {COMMAND_CLASS_DEVICE_RESET_LOCALLY, DEVICE_RESET_LOCALLY_NOTIFICATION}

//@ [AGITABLE_ROOTDEVICE_GROUPS_no_endpoints]
#define  AGITABLE_ROOTDEVICE_GROUPS \
  {ASSOCIATION_GROUP_INFO_REPORT_PROFILE_NOTIFICATION, NOTIFICATION_REPORT_HOME_SECURITY_V4, {COMMAND_CLASS_BASIC, BASIC_SET}, "Basic set"}
//@ [AGITABLE_ROOTDEVICE_GROUPS_no_endpoints]
//@ [AGI_TABLE_ID]

/**
 * PowerDownTimeout determines the number of seconds before power down.
 */
#define KEY_SCAN_POWERDOWNTIMEOUT     3  /**< 300 ms */
#define MSEC_200_POWERDOWNTIMEOUT     2  /**< 200 ms */
#define SEC_2_POWERDOWNTIMEOUT       20  /**<  2 seconds */
#define SEC_10_POWERDOWNTIMEOUT     110  /**< 11 seconds - the spec says minimum 10 seconds. */
#define LEARNMODE_POWERDOWNTIMEOUT  255  /**< 25 seconds */

#ifdef BOOTLOADER_ENABLED
#define FIRMWARE_UPGRADABLE        0xFF  /**< 0x00 = Not upgradable, 0xFF = Upgradable*/
#else
#define FIRMWARE_UPGRADABLE        0x00  /**< 0x00 = Not upgradable, 0xFF = Upgradable*/
#endif

/**
 * Configuration for ApplicationUtilities/notification.h + .c
 *
 * This definitions tells the Notification CC how many types of notifications memory must be
 * allocated for. Each type of notification can contain a number of events, but this list is
 * defined in the application.
 */
#define MAX_NOTIFICATIONS 1

/**
 * The value basic set command should use  when an event occur
 */
#define BASIC_SET_TRIGGER_VALUE (0xFF)

/**
 * The timeout value in sec that used when sending basic set command due to an event
 */
#define BASIC_SET_TIMEOUT    (10000) // Milliseconds


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
 * Note: Due to the limited NVM space on routing slaves, only one
 *       security key can be supported in this app
 */
//@ [REQUESTED_SECURITY_KEYS_ID]
#define REQUESTED_SECURITY_KEYS (SECURITY_KEY_S2_UNAUTHENTICATED_BIT)
//@ [REQUESTED_SECURITY_KEYS_ID]

#endif /* _CONFIG_APP_H_ */

