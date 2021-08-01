/****************************************************************************
 *
 * Copyright (c) 2001-2014
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description: Application NVM variable definitions
 *
 * Author:  Erik Friis Harck
 *
 * Last Changed By:  $Author: efh $
 * Revision:         $Revision: 11509 $
 * Last Changed:     $Date: 2012-03-05 10:45:45 +0200 (Wed, 5 Mar 2012) $
 *
 ****************************************************************************/
#pragma USERCLASS(CONST=NVM)

/* Make sure compiler won't shuffle around with these variables,            */
/* as there are external dependencies.                                      */
/* Note from Keil C51 manual:                                               */
/* Variables with memory type, initilalization, and without initilalization */
/* have all different tables. Therefore only variables with the same        */
/* attributes are kept within order.                                        */
/* Note from Keil knowledgebase: http://www.keil.com/support/docs/901.htm   */
/* "The order is not necessarily taken from the variable declarations, but  */
/* the first use of the variables."                                         */
/* Therefore, when using #pragma ORDER to order variables, declare them in  */
/* the order they should be in a collection. And none of them may be        */
/* declared or known in any way from other header files.                    */
#pragma ORDER

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include "config_app.h"
#include <ZW_basis_api.h>
#include "eeprom.h"
#include <ZW_nvm_descriptor.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/************************************/
/*      NVM variable definition     */
/************************************/

/*--------------------------------------------------------------------------*/
/* NVM layout MyProductPlus (as in t_nvmModule) (begin)                     */

/* Offset from &nvmModule where nvmModuleDescriptor structure is placed     */
t_NvmModuleSize far nvmApplicationSize = (t_NvmModuleSize)&_FD_EEPROM_L_;

/* NVM variables for your application                                       */
BYTE far EEOFFSET_ASSOCIATION_START_far[(NUMBER_OF_ENDPOINTS_NVM_MAX + 1) * ASSOCIATION_SIZE_NVM_MAX];
BYTE far EEOFFSET_ASSOCIATION_MAGIC_far;
BYTE far EEOFFSET_ASSOCIATION_ENDPOINT_START_far[(NUMBER_OF_ENDPOINTS_NVM_MAX + 1) * ASSOCIATION_SIZE_NVM_MAX];
EEOFFSET_TRANSPORT_CAPABILITIES_STRUCT far EEOFFSET_TRANSPORT_CAPABILITIES_START_far[(NUMBER_OF_ENDPOINTS_NVM_MAX + 1) * ASSOCIATION_SIZE_NVM_MAX];
BYTE far EEOFFSET_ASSOCIATION_ENDPOINT_MAGIC_far;
#if defined(BATTERY) || defined(BATTERY__CC_WAKEUP)
// Used by battery_plus module.
DWORD far EEOFFSET_SLEEP_PERIOD_far;
// Used by Wake Up CC.
BYTE far EEOFFSET_MASTER_NODEID_far;
#endif //BATTERY || BATTERY__CC_WAKEUP
BYTE far EEOFFSET_MAGIC_far;
BYTE far EEOFFSET_MAGIC_SDK_6_70_ASSOCIATION_SECURE_far;

/* NVM module descriptor for module. Located at the end of NVM module.      */
/* During the initialization phase, the NVM still contains the NVM contents */
/* from the old version of the firmware.                                    */
t_nvmModuleDescriptor far nvmApplicationDescriptor =
{
  (t_NvmModuleSize)&_FD_EEPROM_L_,      /* t_NvmModuleSize wNvmModuleSize   */
  NVM_MODULE_TYPE_APPLICATION,          /* eNvmModuleType bNvmModuleType    */
  (WORD)&_APP_VERSION_                  /* WORD wNvmModuleVersion           */
};

/* NVM layout MyProductPlus (as in t_nvmModule) (end)                       */
/*--------------------------------------------------------------------------*/

/* NVM module update descriptor for this new version of firmware. Located   */
/* in code space.                                                           */
const t_nvmModuleUpdate code nvmApplicationUpdate =
{
  (p_nvmModule)&nvmApplicationSize,     /* nvmModulePtr                     */
  /* nvmApplicationDescriptor is the first new NVM variable since devkit_6_5x_branch */
  (t_NvmModuleSize)((WORD)&nvmApplicationDescriptor),  /* wNvmModuleSizeOld */
  {
    (t_NvmModuleSize)&_FD_EEPROM_L_,    /* t_NvmModuleSize wNvmModuleSize   */
    NVM_MODULE_TYPE_APPLICATION,        /* eNvmModuleType bNvmModuleType    */
    (WORD)&_APP_VERSION_                /* WORD wNvmModuleVersion           */
  }
};

