/**
 * @file
 * This header file contains defines for library version in a generalized way.
 *
 * @copyright 2019 Silicon Laboratories Inc.
 */
#ifndef _CONFIG_LIB_H_
#define _CONFIG_LIB_H_

#ifdef LIBFILE
#include "ZW_lib_defines.h"
#endif

/*
 * The following two definitions (ZW_VERSION_MAJOR, ZW_VERSION_MINOR & ZW_VERSION_PATCH) specifies
 * the Z-Wave Library version. It is manually changed to match the version specified in the following PSP.
 * PSP14529-2A Product Specification for Z-Wave 500 Series Developer's Kit v6.81.05
 */
#define ZW_VERSION_MAJOR 6
#define ZW_VERSION_MINOR 10
#define ZW_VERSION_PATCH 0

/* Configuration defines for all Z-Wave libraries */

/* Include support for ApplicationRfNotify API */
#define APP_RF_NOTIFY

#endif /* _CONFIG_LIB_H_ */

