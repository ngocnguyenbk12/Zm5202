/**
 * @file
 * Z-Wave Wall Controller Sample Application
 * @copyright 2019 Silicon Laboratories Inc.
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
/* During development of your device, you may add features using command    */
/* classes which are not already included. Remember to include relevant     */
/* classes and utilities and add them in your make file.                    */
/****************************************************************************/
#include <ZW_stdint.h>
#include "config_app.h"
#include "app_version.h"

#include <ZW_slave_api.h>
#ifdef ZW_SLAVE_32
#include <ZW_slave_32_api.h>
#else
#include <ZW_slave_routing_api.h>
#endif  /* ZW_SLAVE_32 */

#include <ZW_classcmd.h>
#include <ZW_mem_api.h>
#include <ZW_TransportLayer.h>

#include <eeprom.h>
#include <ZW_uart_api.h>

#include <misc.h>
#ifdef BOOTLOADER_ENABLED
#include <ota_util.h>
#include <CommandClassFirmwareUpdate.h>
#endif
#include <nvm_util.h>

/*IO control*/
#include <io_zdp03a.h>
#include <ZW_task.h>
#include <ev_man.h>

#ifdef ZW_ISD51_DEBUG
#include "ISD51.h"
#endif

#include <association_plus.h>
#include <agi.h>

#include <CommandClassZWavePlusInfo.h>
#include <CommandClassVersion.h>
#include <CommandClassManufacturerSpecific.h>
#include <CommandClassDeviceResetLocally.h>
#include <CommandClassPowerLevel.h>
#include <CommandClassAssociation.h>
#include <CommandClassAssociationGroupInfo.h>
#include <CommandClassMultiChanAssociation.h>
#include <CommandClassMultiChan.h>
#include <CommandClassBasic.h>
#include <CommandClassMultilevelSwitch.h>
#include <multilevel_switch.h>
#include <CommandClassCentralScene.h>
#include <CommandClassSupervision.h>
#include <CommandClassMultiChan.h>
#include <CommandClassMultiChanAssociation.h>
#include <CommandClassSecurity.h>

#include <ZW_TransportMulticast.h>
#include <ZW_tx_mutex.h>

#include <zaf_version.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
/**
 * @def ZW_DEBUG_MYPRODUCT_SEND_BYTE(data)
 * Transmits a given byte to the debug port.
 * @def ZW_DEBUG_MYPRODUCT_SEND_STR(STR)
 * Transmits a given string to the debug port.
 * @def ZW_DEBUG_MYPRODUCT_SEND_NUM(data)
 * Transmits a given number to the debug port.
 * @def ZW_DEBUG_MYPRODUCT_SEND_WORD_NUM(data)
 * Transmits a given WORD number to the debug port.
 * @def ZW_DEBUG_MYPRODUCT_SEND_NL()
 * Transmits a newline to the debug port.
 */
#ifdef ZW_DEBUG_APP
#define ZW_DEBUG_MYPRODUCT_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_MYPRODUCT_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_MYPRODUCT_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_MYPRODUCT_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_MYPRODUCT_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_MYPRODUCT_SEND_BYTE(data)
#define ZW_DEBUG_MYPRODUCT_SEND_STR(STR)
#define ZW_DEBUG_MYPRODUCT_SEND_NUM(data)
#define ZW_DEBUG_MYPRODUCT_SEND_WORD_NUM(data)
#define ZW_DEBUG_MYPRODUCT_SEND_NL()
#endif


#define EVENT_APP_CC_NO_JOB 0xFF
/**
 * Enumeration of events.
 */
typedef enum
{
  EVENT_EMPTY = DEFINE_EVENT_APP_NBR,
  EVENT_APP_INIT,
  EVENT_APP_OTA_START,
  EVENT_APP_LEARN_MODE_FINISH,
  EVENT_APP_REFRESH_MMI,
  EVENT_APP_NEXT_EVENT_JOB,
  EVENT_APP_FINISH_EVENT_JOB,
  EVENT_APP_TOGGLE_LEARN_MODE,
  EVENT_APP_CC_BASIC_JOB,
  EVENT_APP_CC_SWITCH_MULTILEVEL_JOB,
  EVENT_APP_CENTRAL_SCENE_JOB,
  EVENT_APP_SMARTSTART_IN_PROGRESS
} EVENT_APP;

/**
 * Application states. Function AppStateManager(..) includes the state
 * event machine.
 */
typedef enum
{
  STATE_APP_IDLE,
  STATE_APP_STARTUP,
  STATE_APP_AWAIT_KEYPRESS,
  STATE_APP_LEARN_MODE,
  STATE_APP_GET_NEXT_NODE,
  STATE_APP_INITIATE_TRANSMISSION,
  STATE_APP_AWAIT_TRANSMIT_CALLBACK,
  STATE_APP_WATCHDOG_RESET,
  STATE_APP_OTA,
  STATE_APP_TRANSMIT_DATA
}
STATE_APP;

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

/**
 * Unsecure node information list.
 * Be sure Command classes are not duplicated in both lists.
 * CHANGE THIS - Add all supported non-secure command classes here
 **/
//@ [cmdClassListNonSecureNotIncluded]
static uint8_t cmdClassListNonSecureNotIncluded[] =
//@ [cmdClassListNonSecureNotIncluded]
{
  COMMAND_CLASS_ZWAVEPLUS_INFO,
  COMMAND_CLASS_ASSOCIATION_V2,
  COMMAND_CLASS_ASSOCIATION_GRP_INFO,
  COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
  COMMAND_CLASS_MULTI_CHANNEL_V3,
  COMMAND_CLASS_TRANSPORT_SERVICE_V2,
  COMMAND_CLASS_VERSION,
  COMMAND_CLASS_MANUFACTURER_SPECIFIC,
  COMMAND_CLASS_DEVICE_RESET_LOCALLY,
  COMMAND_CLASS_POWERLEVEL,
  COMMAND_CLASS_SECURITY,
  COMMAND_CLASS_SECURITY_2,
  COMMAND_CLASS_SUPERVISION,
  COMMAND_CLASS_CENTRAL_SCENE_V2
#ifdef BOOTLOADER_ENABLED
  ,COMMAND_CLASS_FIRMWARE_UPDATE_MD_V2
#endif
};

/**
 * Unsecure node information list Secure included.
 * Be sure Command classes are not duplicated in both lists.
 * CHANGE THIS - Add all supported non-secure command classes here
 **/
//@ [cmdClassListNonSecureIncludedSecure]
static uint8_t cmdClassListNonSecureIncludedSecure[] =
//@ [cmdClassListNonSecureIncludedSecure]
{
  COMMAND_CLASS_ZWAVEPLUS_INFO,
  COMMAND_CLASS_TRANSPORT_SERVICE_V2,
  COMMAND_CLASS_SUPERVISION,
  COMMAND_CLASS_SECURITY,
  COMMAND_CLASS_SECURITY_2
};

/**
 * Secure node information list.
 * Be sure Command classes are not duplicated in both lists.
 * CHANGE THIS - Add all supported secure command classes here
 **/
//@ [cmdClassListSecure]
static uint8_t cmdClassListSecure[] =
//@ [cmdClassListSecure]
{
  COMMAND_CLASS_VERSION,
  COMMAND_CLASS_ASSOCIATION_V2,
  COMMAND_CLASS_ASSOCIATION_GRP_INFO,
  COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
  COMMAND_CLASS_MULTI_CHANNEL_V3,
  COMMAND_CLASS_MANUFACTURER_SPECIFIC,
  COMMAND_CLASS_DEVICE_RESET_LOCALLY,
  COMMAND_CLASS_POWERLEVEL,
  COMMAND_CLASS_CENTRAL_SCENE_V2
#ifdef BOOTLOADER_ENABLED
  ,COMMAND_CLASS_FIRMWARE_UPDATE_MD_V2
#endif
};


/**
 * Structure includes application node information list's and device type.
 */
APP_NODE_INFORMATION m_AppNIF =
{
  cmdClassListNonSecureNotIncluded, sizeof(cmdClassListNonSecureNotIncluded),
  cmdClassListNonSecureIncludedSecure, sizeof(cmdClassListNonSecureIncludedSecure),
  cmdClassListSecure, sizeof(cmdClassListSecure),
  DEVICE_OPTIONS_MASK, GENERIC_TYPE, SPECIFIC_TYPE
};


/**
 * Node information for endpoint 1-4.
 * This data can be stored in one array since it's the same for all four
 * endpoints. If an application is to have different types of endpoints, several
 * arrays has to be declared.
 */

/**
 *  Unsecure node information list.
 */
static code uint8_t endpoint1_2_3_4_cmdClassListNonSecureNotIncluded[] =
{
  COMMAND_CLASS_ZWAVEPLUS_INFO_V2,
  COMMAND_CLASS_ASSOCIATION_V2,
  COMMAND_CLASS_ASSOCIATION_GRP_INFO,
  COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
  COMMAND_CLASS_SUPERVISION
};


/**
 * Unsecure node information list Secure included.
 */
static code uint8_t endpoint1_2_3_4_cmdClassListNonSecureIncludedSecure[] =
{
  COMMAND_CLASS_ZWAVEPLUS_INFO_V2,
  COMMAND_CLASS_SUPERVISION,
  COMMAND_CLASS_SECURITY,
  COMMAND_CLASS_SECURITY_2
};

/**
 * Secure node inforamtion list.
 */
static code uint8_t endpoint1_2_3_4_cmdClassListSecure[] =
{
  COMMAND_CLASS_ASSOCIATION_V2,
  COMMAND_CLASS_ASSOCIATION_GRP_INFO,
  COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2
};


static EP_NIF endpointsNIF[NUMBER_OF_ENDPOINTS] =
{  /*Endpoint 1*/
  { GENERIC_TYPE, SPECIFIC_TYPE,
    {
      {endpoint1_2_3_4_cmdClassListNonSecureNotIncluded, sizeof(endpoint1_2_3_4_cmdClassListNonSecureNotIncluded)},
       {{endpoint1_2_3_4_cmdClassListNonSecureIncludedSecure, sizeof(endpoint1_2_3_4_cmdClassListNonSecureIncludedSecure)},
        {endpoint1_2_3_4_cmdClassListSecure, sizeof(endpoint1_2_3_4_cmdClassListSecure)}
      }
    }
  }, /*Endpoint 2*/
  { GENERIC_TYPE, SPECIFIC_TYPE,
    {
      {endpoint1_2_3_4_cmdClassListNonSecureNotIncluded, sizeof(endpoint1_2_3_4_cmdClassListNonSecureNotIncluded)},
       {{endpoint1_2_3_4_cmdClassListNonSecureIncludedSecure, sizeof(endpoint1_2_3_4_cmdClassListNonSecureIncludedSecure)},
        {endpoint1_2_3_4_cmdClassListSecure, sizeof(endpoint1_2_3_4_cmdClassListSecure)}
      }
    }
  }, /*Endpoint 3*/
  { GENERIC_TYPE, SPECIFIC_TYPE,
    {
      {endpoint1_2_3_4_cmdClassListNonSecureNotIncluded, sizeof(endpoint1_2_3_4_cmdClassListNonSecureNotIncluded)},
       {{endpoint1_2_3_4_cmdClassListNonSecureIncludedSecure, sizeof(endpoint1_2_3_4_cmdClassListNonSecureIncludedSecure)},
        {endpoint1_2_3_4_cmdClassListSecure, sizeof(endpoint1_2_3_4_cmdClassListSecure)}
      }
    }
  }, /*Endpoint 4*/
  { GENERIC_TYPE, SPECIFIC_TYPE,
    {
      {endpoint1_2_3_4_cmdClassListNonSecureNotIncluded, sizeof(endpoint1_2_3_4_cmdClassListNonSecureNotIncluded)},
       {{endpoint1_2_3_4_cmdClassListNonSecureIncludedSecure, sizeof(endpoint1_2_3_4_cmdClassListNonSecureIncludedSecure)},
        {endpoint1_2_3_4_cmdClassListSecure, sizeof(endpoint1_2_3_4_cmdClassListSecure)}
      }
    }
  }
};

EP_FUNCTIONALITY_DATA endPointFunctionality =
{
  NUMBER_OF_INDIVIDUAL_ENDPOINTS,     /**< nbrIndividualEndpoints 7 bit*/
  RES_ZERO,                           /**< resIndZeorBit 1 bit*/
  NUMBER_OF_AGGREGATED_ENDPOINTS,     /**< nbrAggregatedEndpoints 7 bit*/
  RES_ZERO,                           /**< resAggZeorBit 1 bit*/
  RES_ZERO,                           /**< resZero 6 bit*/
  ENDPOINT_IDENTICAL_DEVICE_CLASS_YES,/**< identical 1 bit*/
  ENDPOINT_DYNAMIC_NO                /**< dynamic 1 bit*/
};

/**
 * Setup AGI lifeline table from app_config.h
 */
//@ [wall_controller_agi_root_device_lifeline]
CMD_CLASS_GRP agiTableLifeLine[] = {AGITABLE_LIFELINE_GROUP};
//@ [wall_controller_agi_root_device_lifeline]

/**
 * AGI table defining the lifeline groups for all four endpoints.
 */
//@ [wall_controller_agi_endpoints_lifeline]
CMD_CLASS_GRP  agiTableLifeLineEP1_2_3_4[] = {AGITABLE_LIFELINE_GROUP_EP1_2_3_4};
//@ [wall_controller_agi_endpoints_lifeline]

/**
 * @var agiTableRootDeviceGroups
 * AGI table defining the groups for root device.
 * @var agiTableEndpoint1Groups
 * AGI table defining the groups for endpoint 1.
 * @var agiTableEndpoint2Groups
 * AGI table defining the groups for endpoint 2.
 * @var agiTableEndpoint3Groups
 * AGI table defining the groups for endpoint 3.
 * @var agiTableEndpoint4Groups
 * AGI table defining the groups for endpoint 4.
 */
//@ [wall_controller_agi_other_groups]
AGI_GROUP agiTableRootDeviceGroups[] = {AGITABLE_ROOTDEVICE_GROUPS};
AGI_GROUP agiTableEndpoint1Groups[]  = {AGITABLE_ENDPOINT_1_GROUPS};
AGI_GROUP agiTableEndpoint2Groups[]  = {AGITABLE_ENDPOINT_2_GROUPS};
AGI_GROUP agiTableEndpoint3Groups[]  = {AGITABLE_ENDPOINT_3_GROUPS};
AGI_GROUP agiTableEndpoint4Groups[]  = {AGITABLE_ENDPOINT_4_GROUPS};
//@ [wall_controller_agi_other_groups]


/*root grp, endpoint, endpoint group*/
ASSOCIATION_ROOT_GROUP_MAPPING rootGroupMapping[] = { ASSOCIATION_ROOT_GROUP_MAPPING_CONFIG};

/**
 * Holds icons for each of the endpoints.
 */
ST_ENDPOINT_ICONS ZWavePlusEndpointIcons[] = {ENDPOINT_ICONS};
/**
 * This Z-Wave node's ID.
 */
static uint8_t myNodeID = 0;

/**
 * Application state-machine state.
 */
static STATE_APP currentState;

/**
 * Parameter is used to save wakeup reason after ApplicationInitHW(..)
 */
SW_WAKEUP wakeupReason;

typedef struct _NEXT_JOB_Q_
{
  AGI_PROFILE profile;
} NEXT_JOB_Q;

/**
 * Holds the latest button action.
 * The action is used to distinguish between which action to perform:
 * - Press    => Switch on/off
 * - Hold     => Start dimming
 * - Release  => Stop dimming
 */
static KEY_EVENT_T keyEventGlobal;
static uint8_t centralSceneSceneHold;
static uint8_t centralSceneKeyAttributeHold;
static bTimerHandle_t centralSceneHoldTimerHandle = 0xFF;
static CCMLS_PRIMARY_SWITCH_T multiLevelDirection;
static uint8_t buttonStates[NUMBER_OF_ENDPOINTS] = {0, 0, 0, 0};
static uint8_t m_endpoint = 0xFF; // 1 - 4
NEXT_JOB_Q nextJob;

static EVENT_KEY cached_key_event = EVENT_KEY_MAX;

#if defined(APP_SUPPORTS_CLIENT_SIDE_AUTHENTICATION) && !defined(TEST_INTERFACE_SUPPORT)
static s_SecurityS2InclusionCSAPublicDSK_t sCsaResponse = { 0, 0, 0, 0};
#endif /* defined(APP_SUPPORTS_CLIENT_SIDE_AUTHENTICATION) && !defined(TEST_INTERFACE_SUPPORT) */

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

// No exported data.

/****************************************************************************/
/* PRIVATE FUNCTION PROTOTYPES                                              */
/****************************************************************************/

void ZCB_DeviceResetLocallyDone(TRANSMISSION_RESULT * pTransmissionResult);
void AppStateManager(EVENT_APP event);
static void ChangeState(STATE_APP newState);
STATE_APP GetAppState(void);
BOOL ApplicationIdle(void);
static void PrepareAGITransmission(uint8_t profile, uint8_t nextActiveButton);
static JOB_STATUS InitiateTransmission(void);
void ZCB_TransmitCallback(TRANSMISSION_RESULT * pTransmissionResult);
static void InitiateCentralSceneTX(uint8_t keyAttribute, uint8_t sceneNumber);
static void ZCB_CentralSceneHoldTimerCallback(void);

#ifdef BOOTLOADER_ENABLED
void ZCB_OTAFinish(OTA_STATUS otaStatus);
BOOL ZCB_OTAStart();
#endif

/**
 * @brief See description where prototype is declared.
 * @param rfState Current state of the RF.
 */
void
ApplicationRfNotify(uint8_t rfState)
{
  UNUSED(rfState);
}

/**
 * @brief See description where prototype is declared.
 */
uint8_t
ApplicationInitHW(SW_WAKEUP bWakeupReason)
{
  wakeupReason = bWakeupReason;
  /* hardware initialization */
  ZDP03A_InitHW(ZCB_eventSchedulerEventAdd);
  SetPinIn(ZDP03A_KEY_1,TRUE);
  SetPinIn(ZDP03A_KEY_2,TRUE);
  SetPinIn(ZDP03A_KEY_4,TRUE);
  SetPinIn(ZDP03A_KEY_6,TRUE);

  /*
   * Z-Wave specific stuff needs to be called.
   */
  Transport_OnApplicationInitHW(bWakeupReason);

  return(TRUE);
}

/**
 * @brief See description where prototype is declared.
 */
uint8_t
ApplicationInitSW(ZW_NVM_STATUS nvmStatus)
{
  BYTE application_node_type = DEVICE_OPTIONS_MASK;
  /* Init state machine*/
  currentState = STATE_APP_STARTUP;
  /* Do not reinitialize the UART if already initialized for ISD51 in ApplicationInitHW() */

#ifndef ZW_ISD51_DEBUG
  ZW_DEBUG_INIT(1152);
#endif

  ZW_DEBUG_MYPRODUCT_SEND_NL();
  ZW_DEBUG_MYPRODUCT_SEND_STR("*** Wall Controller Sample Application ***");
  ZW_DEBUG_MYPRODUCT_SEND_NL();
  ZW_DEBUG_MYPRODUCT_SEND_NUM(wakeupReason);
  ZW_DEBUG_MYPRODUCT_SEND_NUM(currentState);
  ZW_DEBUG_MYPRODUCT_SEND_NL();

#ifdef WATCHDOG_ENABLED
  ZW_WatchDogEnable();
#endif
#ifdef BOOTLOADER_ENABLED
  NvmInit(nvmStatus);

#else
  UNUSED(nvmStatus);
#endif

  /* Initialize the NVM if needed */
  if (MemoryGetByte((WORD)&EEOFFSET_MAGIC_far) == APPL_MAGIC_VALUE)
  {
    /* Initialize PowerLevel functionality*/
    loadStatusPowerLevel();
  }
  else
  {
    ZW_MEM_PUT_BYTE((WORD)&EEOFFS_SECURITY_RESERVED.EEOFFS_MAGIC_BYTE_field, EEPROM_MAGIC_BYTE_VALUE);
    /* Initialize transport layer NVM */
    Transport_SetDefault();
    /* Reset protocol */
    ZW_SetDefault();
    /* Initialize PowerLevel functionality.*/
    loadInitStatusPowerLevel();
    /* CHANGE THIS - Add initialization of application NVM here */

    /* Force reset of associations. */
    AssociationInit(TRUE);
    ZW_MEM_PUT_BYTE((WORD)&EEOFFSET_CENTRAL_SCENE_SLOW_REFRESH_far, 1);

    ZW_MEM_PUT_BYTE((WORD)&EEOFFSET_MAGIC_far, APPL_MAGIC_VALUE);
  }

  /* Initialize association module */
  AssociationInitEndpointSupport(FALSE, rootGroupMapping, sizeof(rootGroupMapping)/sizeof(ASSOCIATION_ROOT_GROUP_MAPPING));

  /*
   * Initialize AGI.
   */
  //@ [WALL_CONTROLLER_AGI_INIT]
  AGI_Init();
  CC_AGI_LifeLineGroupSetup(agiTableLifeLine, (sizeof(agiTableLifeLine)/sizeof(CMD_CLASS_GRP)), ENDPOINT_ROOT);
  AGI_ResourceGroupSetup(agiTableRootDeviceGroups, (sizeof(agiTableRootDeviceGroups)/sizeof(AGI_GROUP)), ENDPOINT_ROOT);

  CC_AGI_LifeLineGroupSetup(agiTableLifeLineEP1_2_3_4, (sizeof(agiTableLifeLineEP1_2_3_4)/sizeof(CMD_CLASS_GRP)), ENDPOINT_1);
  AGI_ResourceGroupSetup(&agiTableEndpoint1Groups, (sizeof(agiTableEndpoint1Groups)/sizeof(AGI_GROUP)), ENDPOINT_1);
  CC_AGI_LifeLineGroupSetup(agiTableLifeLineEP1_2_3_4, (sizeof(agiTableLifeLineEP1_2_3_4)/sizeof(CMD_CLASS_GRP)), ENDPOINT_2);
  AGI_ResourceGroupSetup(&agiTableEndpoint2Groups, (sizeof(agiTableEndpoint2Groups)/sizeof(AGI_GROUP)), ENDPOINT_2);
  CC_AGI_LifeLineGroupSetup(agiTableLifeLineEP1_2_3_4, (sizeof(agiTableLifeLineEP1_2_3_4)/sizeof(CMD_CLASS_GRP)), ENDPOINT_3);
  AGI_ResourceGroupSetup(&agiTableEndpoint3Groups, (sizeof(agiTableEndpoint3Groups)/sizeof(AGI_GROUP)), ENDPOINT_3);
  CC_AGI_LifeLineGroupSetup(agiTableLifeLineEP1_2_3_4, (sizeof(agiTableLifeLineEP1_2_3_4)/sizeof(CMD_CLASS_GRP)), ENDPOINT_4);
  AGI_ResourceGroupSetup(&agiTableEndpoint4Groups, (sizeof(agiTableEndpoint4Groups)/sizeof(AGI_GROUP)), ENDPOINT_4);
  //@ [WALL_CONTROLLER_AGI_INIT]

  CommandClassZWavePlusInfoInit( ZWavePlusEndpointIcons, sizeof(ZWavePlusEndpointIcons)/sizeof(ST_ENDPOINT_ICONS));

  CC_Version_SetApplicationVersionInfo(APP_VERSION, APP_REVISION, APP_VERSION_PATCH, APP_BUILD_NUMBER);

  /*
   * Get this Z-Wave node's ID.
   */
  MemoryGetID(NULL, &myNodeID);

  /* Initialize manufactory specific module */
  ManufacturerSpecificDeviceIDInit();


#ifdef BOOTLOADER_ENABLED
  /* Initialize OTA module */
  OtaInit(ZCB_OTAStart, NULL, ZCB_OTAFinish);
#endif

  /*
   * Initialize Event Scheduler.
   */
  ZAF_eventSchedulerInit(AppStateManager);

  Transport_OnApplicationInitSW( &m_AppNIF);
  Transport_AddEndpointSupport( &endPointFunctionality, endpointsNIF, NUMBER_OF_ENDPOINTS);
  ZCB_eventSchedulerEventAdd( EVENT_APP_INIT );

  return(application_node_type);
}

/**
 * @brief See description for function prototype in ZW_basis_api.h.
 */
void
ApplicationTestPoll(void)
{
}


/**
 * @brief See description for function prototype in ZW_basis_api.h.
 */
E_APPLICATION_STATE ApplicationPoll(E_PROTOCOL_STATE bProtocolState)
{
  UNUSED(bProtocolState);

#ifdef WATCHDOG_ENABLED
  ZW_WatchDogKick();
#endif
  return (TRUE == ApplicationIdle()) ? E_APPLICATION_STATE_READY_FOR_POWERDOWN : E_APPLICATION_STATE_ACTIVE;
}


/**
 * @brief Check if application is idle
 * @return true if application idle
 */
static BOOL ApplicationIdle(void)
{
  if( (FALSE == TaskApplicationPoll()) && // Check Task Handler for active task.
      (STATE_APP_IDLE == GetAppState())   //Check application is in idle state.
     )
  {
    /*
     * Check event queue for queued up jobs
     */
    if(0 != ZAF_jobQueueCount())
    {
      ZCB_eventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
      ChangeState(STATE_APP_TRANSMIT_DATA);
    }
  }
  return FALSE; // Always on node!
}


/**
 * @brief See description for function prototype in ZW_TransportEndpoint.h.
 */
received_frame_status_t
Transport_ApplicationCommandHandlerEx(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  uint8_t cmdLength)
{
  received_frame_status_t frame_status = RECEIVED_FRAME_STATUS_NO_SUPPORT;
  /* Call command class handlers */
  switch (pCmd->ZW_Common.cmdClass)
  {
    case COMMAND_CLASS_VERSION:
      frame_status = handleCommandClassVersion(rxOpt, pCmd, cmdLength);
      break;

#ifdef BOOTLOADER_ENABLED
    case COMMAND_CLASS_FIRMWARE_UPDATE_MD_V2:
      frame_status = handleCommandClassFWUpdate(rxOpt, pCmd, cmdLength);
      break;
#endif

    case COMMAND_CLASS_ASSOCIATION_GRP_INFO:
      frame_status = CC_AGI_handler( rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_ASSOCIATION:
      frame_status = handleCommandClassAssociation(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_POWERLEVEL:
      frame_status = handleCommandClassPowerLevel(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_MANUFACTURER_SPECIFIC:
      frame_status = handleCommandClassManufacturerSpecific(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_ZWAVEPLUS_INFO:
      frame_status = handleCommandClassZWavePlusInfo(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2:
      frame_status = handleCommandClassMultiChannelAssociation(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_CENTRAL_SCENE_V2:
      frame_status = handleCommandClassCentralScene(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_SUPERVISION:
      frame_status = handleCommandClassSupervision(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_SECURITY:
    case COMMAND_CLASS_SECURITY_2:
      frame_status = handleCommandClassSecurity(rxOpt, pCmd, cmdLength);
      break;
  }
  return frame_status;
}


/**
 * @brief See description for function prototype in CommandClassVersion.h.
 */
uint8_t
handleCommandClassVersionAppl(uint8_t cmdClass)
{
  uint8_t commandClassVersion = UNKNOWN_VERSION;

  switch (cmdClass)
  {
    case COMMAND_CLASS_VERSION:
      commandClassVersion = CommandClassVersionVersionGet();
      break;

#ifdef BOOTLOADER_ENABLED
    case COMMAND_CLASS_FIRMWARE_UPDATE_MD:
      commandClassVersion = CommandClassFirmwareUpdateMdVersionGet();
      break;
#endif

    case COMMAND_CLASS_POWERLEVEL:
      commandClassVersion = CommandClassPowerLevelVersionGet();
      break;

    case COMMAND_CLASS_MANUFACTURER_SPECIFIC:
      commandClassVersion = CommandClassManufacturerVersionGet();
      break;

    case COMMAND_CLASS_ASSOCIATION:
      commandClassVersion = CommandClassAssociationVersionGet();
      break;

    case COMMAND_CLASS_ASSOCIATION_GRP_INFO:
      commandClassVersion = CommandClassAssociationGroupInfoVersionGet();
      break;

    case COMMAND_CLASS_DEVICE_RESET_LOCALLY:
      commandClassVersion = CommandClassDeviceResetLocallyVersionGet();
      break;

    case COMMAND_CLASS_ZWAVEPLUS_INFO:
      commandClassVersion = CommandClassZWavePlusVersion();
      break;
    case COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2:
      commandClassVersion = CmdClassMultiChannelAssociationVersion();
      break;

    case COMMAND_CLASS_MULTI_CHANNEL_V3:
      commandClassVersion = CmdClassMultiChannelGet();
      break;

    case COMMAND_CLASS_CENTRAL_SCENE:
      commandClassVersion = CommandClassCentralSceneVersionGet();
      break;

    case COMMAND_CLASS_SUPERVISION:
      commandClassVersion = CommandClassSupervisionVersionGet();
      break;

    default:
     commandClassVersion = ZW_Transport_CommandClassVersionGet(cmdClass);
  }
  return commandClassVersion;
}


/**
 * @brief See description for function prototype in ZW_slave_api.h.
 */
void
ApplicationSlaveUpdate(
  uint8_t bStatus,
  uint8_t bNodeID,
  uint8_t* pCmd,
  uint8_t bLen)
{
  UNUSED(bStatus);
  UNUSED(bNodeID);
  UNUSED(pCmd);
  UNUSED(bLen);
}

/**
 * @brief See description for function prototype in slave_learn.h.
 */
void ApplicationNetworkLearnModeCompleted(uint8_t bNodeID)
{
  if(APPLICATION_NETWORK_LEARN_MODE_COMPLETED_SMART_START_IN_PROGRESS == bNodeID)
  {
    ZCB_eventSchedulerEventAdd(EVENT_APP_SMARTSTART_IN_PROGRESS);
  }
  else
  {
    if (APPLICATION_NETWORK_LEARN_MODE_COMPLETED_FAILED == bNodeID)
    {
      MemoryPutByte((WORD)&EEOFFSET_MAGIC_far, APPL_MAGIC_VALUE + 1);
      ZCB_eventSchedulerEventAdd((EVENT_APP) EVENT_SYSTEM_WATCHDOG_RESET);
    }
    else
    {
      if (APPLICATION_NETWORK_LEARN_MODE_COMPLETED_TIMEOUT == bNodeID)
      {
        /**
         * Inclusion or exclusion timed out. We need to make sure that the application triggers
         * Smartstart inclusion.
         */
        ZCB_eventSchedulerEventAdd(EVENT_APP_INIT);
        ChangeState(STATE_APP_STARTUP);
      }
      else
      {
        /* Success */
        myNodeID = bNodeID;
        if (0 == myNodeID)
        {
          /* Clear association */
          AssociationInit(TRUE);
          //Set Central Scene to default slow refresh rate.
          ZW_MEM_PUT_BYTE((WORD)&EEOFFSET_CENTRAL_SCENE_SLOW_REFRESH_far, 1);

          ZCB_eventSchedulerEventAdd(EVENT_APP_INIT);
          ChangeState(STATE_APP_STARTUP);
        }
      }
    }
    ZCB_eventSchedulerEventAdd((EVENT_APP) EVENT_SYSTEM_LEARNMODE_FINISH);
    Transport_OnLearnCompleted(bNodeID);
  }
}

/**
 * @brief Returns the Z-Wave node ID.
 * @return BYTE NodeID
 */
uint8_t
GetMyNodeID(void)
{
	return myNodeID;
}

/**
 * @brief Returns the current state of the application state machine.
 * @return Current state
 */
STATE_APP
GetAppState(void)
{
  return currentState;
}


/**
 * @brief The core state machine of this sample application.
 * @param event The event that triggered the call of AppStateManager.
 */
void
AppStateManager(EVENT_APP event)
{
  ZW_DEBUG_MYPRODUCT_SEND_NL();
  ZW_DEBUG_MYPRODUCT_SEND_STR("AppST");
  ZW_DEBUG_MYPRODUCT_SEND_NUM(currentState);
  ZW_DEBUG_MYPRODUCT_SEND_NUM(event);
  ZW_DEBUG_MYPRODUCT_SEND_NL();

  if(EVENT_SYSTEM_WATCHDOG_RESET == event)
  {
    /*Force state change to activate watchdog-reset without taking care of current
      state.*/
    ChangeState(STATE_APP_WATCHDOG_RESET);
  }

  switch(currentState)
  {
    case STATE_APP_STARTUP:

      if(EVENT_APP_INIT == event)
      {
        ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_INCLUSION_SMARTSTART);
      }

      ChangeState(STATE_APP_IDLE);
      break;

    case STATE_APP_IDLE:
      if(EVENT_APP_REFRESH_MMI == event)
      {
        break;
      }

      if(EVENT_APP_SMARTSTART_IN_PROGRESS == event)
      {
        ChangeState(STATE_APP_LEARN_MODE);
      }

      /**************************************************************************************
       * KEY 1
       **************************************************************************************
       */
      if((EVENT_KEY_B1_PRESS == event) ||(EVENT_SYSTEM_LEARNMODE_START == event))
      {
        if (myNodeID)
        {
          ZW_DEBUG_MYPRODUCT_SEND_STR("LEARN_MODE_EXCLUSION");
          ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_EXCLUSION_NWE);
        }
        else{
          ZW_DEBUG_MYPRODUCT_SEND_STR("LEARN_MODE_INCLUSION");
          ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_INCLUSION);
        }
        ChangeState(STATE_APP_LEARN_MODE);
        break;
      }

      if((EVENT_KEY_B1_HELD_10_SEC == event) || (EVENT_SYSTEM_RESET ==event))
      {
//@ [DEVICE_RESET_LOCALLY_TX]
        AGI_PROFILE lifelineProfile = {
                ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL_NA_V2,
                ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL_LIFELINE
        };

        handleCommandClassDeviceResetLocally(&lifelineProfile, ZCB_DeviceResetLocallyDone);
//@ [DEVICE_RESET_LOCALLY_TX]

        ZW_DEBUG_MYPRODUCT_SEND_STR("Call locally reset");
        ZW_DEBUG_MYPRODUCT_SEND_NL();
        break;
      }

      /**************************************************************************************
       * KEY 2
       **************************************************************************************
       */
      if (EVENT_KEY_B2_PRESS == event)
      {
        ZW_DEBUG_MYPRODUCT_SEND_STR("\r\nK2PRESS\r\n");
        keyEventGlobal = KEY_PRESS;
        centralSceneSceneHold = 1;
        centralSceneKeyAttributeHold = CENTRAL_SCENE_NOTIFICATION_KEY_ATTRIBUTES_KEY_PRESSED_1_TIME_V2;

        ZAF_jobEnqueue(EVENT_APP_CENTRAL_SCENE_JOB);

        PrepareAGITransmission(ASSOCIATION_GROUP_INFO_REPORT_PROFILE_CONTROL_KEY01, 1);
        ZCB_eventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
        ChangeState(STATE_APP_TRANSMIT_DATA);
      }

      if (EVENT_KEY_B2_HELD == event)
      {
        ZW_DEBUG_MYPRODUCT_SEND_STR("\r\nK2HOLD\r\n");
        keyEventGlobal = KEY_HOLD;
        centralSceneSceneHold = 1;
        centralSceneKeyAttributeHold = CENTRAL_SCENE_NOTIFICATION_KEY_ATTRIBUTES_KEY_HELD_DOWN_V2;

        ZAF_jobEnqueue(EVENT_APP_CENTRAL_SCENE_JOB);

        PrepareAGITransmission(ASSOCIATION_GROUP_INFO_REPORT_PROFILE_CONTROL_KEY01, 1);
        ZCB_eventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
        ChangeState(STATE_APP_TRANSMIT_DATA);
      }

      if (EVENT_KEY_B2_UP == event && (keyEventGlobal == KEY_HOLD))
      {
        ZW_DEBUG_MYPRODUCT_SEND_STR("\r\nK2UP\r\n");
        keyEventGlobal = KEY_UP;
        centralSceneSceneHold = 1;
        centralSceneKeyAttributeHold = CENTRAL_SCENE_NOTIFICATION_KEY_ATTRIBUTES_KEY_RELEASED_V2;

        ZAF_jobEnqueue(EVENT_APP_CENTRAL_SCENE_JOB);

        PrepareAGITransmission(ASSOCIATION_GROUP_INFO_REPORT_PROFILE_CONTROL_KEY01, 1);
        ZCB_eventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
        ChangeState(STATE_APP_TRANSMIT_DATA);
      }

      /**************************************************************************************
       * KEY 3
       **************************************************************************************
       */
      if (EVENT_KEY_B4_PRESS == event)
      {
        keyEventGlobal = KEY_PRESS;
        centralSceneSceneHold = 2;
        centralSceneKeyAttributeHold = CENTRAL_SCENE_NOTIFICATION_KEY_ATTRIBUTES_KEY_PRESSED_1_TIME_V2;

        ZAF_jobEnqueue(EVENT_APP_CENTRAL_SCENE_JOB);

        PrepareAGITransmission(ASSOCIATION_GROUP_INFO_REPORT_PROFILE_CONTROL_KEY03, 3);
        ZCB_eventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
        ChangeState(STATE_APP_TRANSMIT_DATA);
      }

      if (EVENT_KEY_B4_HELD == event)
      {
        ZW_DEBUG_MYPRODUCT_SEND_STR("\r\nK4HOLD\r\n");
        keyEventGlobal = KEY_HOLD;
        centralSceneSceneHold = 2;
        centralSceneKeyAttributeHold = CENTRAL_SCENE_NOTIFICATION_KEY_ATTRIBUTES_KEY_HELD_DOWN_V2;

        ZAF_jobEnqueue(EVENT_APP_CENTRAL_SCENE_JOB);

        PrepareAGITransmission(ASSOCIATION_GROUP_INFO_REPORT_PROFILE_CONTROL_KEY03, 3);
        ZCB_eventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
        ChangeState(STATE_APP_TRANSMIT_DATA);
      }

      if (EVENT_KEY_B4_UP == event && (keyEventGlobal == KEY_HOLD))
      {
        ZW_DEBUG_MYPRODUCT_SEND_STR("\r\nK4UP\r\n");
        keyEventGlobal = KEY_UP;
        centralSceneSceneHold = 2;
        centralSceneKeyAttributeHold = CENTRAL_SCENE_NOTIFICATION_KEY_ATTRIBUTES_KEY_RELEASED_V2;

        ZAF_jobEnqueue(EVENT_APP_CENTRAL_SCENE_JOB);

        PrepareAGITransmission(ASSOCIATION_GROUP_INFO_REPORT_PROFILE_CONTROL_KEY03, 3);
        ZCB_eventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
        ChangeState(STATE_APP_TRANSMIT_DATA);
      }

      /**************************************************************************************
       * KEY 4
       **************************************************************************************
       */
      if (EVENT_KEY_B6_PRESS == event)
      {
        keyEventGlobal = KEY_PRESS;
        centralSceneSceneHold = 3;
        centralSceneKeyAttributeHold = CENTRAL_SCENE_NOTIFICATION_KEY_ATTRIBUTES_KEY_PRESSED_1_TIME_V2;

        ZAF_jobEnqueue(EVENT_APP_CENTRAL_SCENE_JOB);

        PrepareAGITransmission(ASSOCIATION_GROUP_INFO_REPORT_PROFILE_CONTROL_KEY04, 4);
        ZCB_eventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
        ChangeState(STATE_APP_TRANSMIT_DATA);
      }

      if (EVENT_KEY_B6_HELD == event)
      {
        ZW_DEBUG_MYPRODUCT_SEND_STR("\r\nK5HOLD\r\n");
        keyEventGlobal = KEY_HOLD;
        centralSceneSceneHold = 3;
        centralSceneKeyAttributeHold = CENTRAL_SCENE_NOTIFICATION_KEY_ATTRIBUTES_KEY_HELD_DOWN_V2;

        ZAF_jobEnqueue(EVENT_APP_CENTRAL_SCENE_JOB);

        PrepareAGITransmission(ASSOCIATION_GROUP_INFO_REPORT_PROFILE_CONTROL_KEY04, 4);
        ZCB_eventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
        ChangeState(STATE_APP_TRANSMIT_DATA);
      }

      if (EVENT_KEY_B6_UP == event && (keyEventGlobal == KEY_HOLD))
      {
        ZW_DEBUG_MYPRODUCT_SEND_STR("\r\nK5UP\r\n");
        keyEventGlobal = KEY_UP;
        centralSceneSceneHold = 3;
        centralSceneKeyAttributeHold = CENTRAL_SCENE_NOTIFICATION_KEY_ATTRIBUTES_KEY_RELEASED_V2;

        ZAF_jobEnqueue(EVENT_APP_CENTRAL_SCENE_JOB);

        PrepareAGITransmission(ASSOCIATION_GROUP_INFO_REPORT_PROFILE_CONTROL_KEY04, 4);
        ZCB_eventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
        ChangeState(STATE_APP_TRANSMIT_DATA);
      }
      break;


    case STATE_APP_LEARN_MODE:
      if(EVENT_APP_REFRESH_MMI == event)
      {
        // Nothing here.
      }

      if((EVENT_KEY_B1_PRESS == event)||(EVENT_SYSTEM_LEARNMODE_END == event))
      {
        ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_DISABLE);
        ZCB_eventSchedulerEventAdd(EVENT_APP_INIT);
        ChangeState(STATE_APP_STARTUP);
      }

      if(EVENT_SYSTEM_LEARNMODE_FINISH == event)
      {
        ChangeState(STATE_APP_IDLE);
      }
      break;

    case STATE_APP_WATCHDOG_RESET:
      if(EVENT_APP_REFRESH_MMI == event){}

      ZW_DEBUG_MYPRODUCT_SEND_STR("STATE_APP_WATCHDOG_RESET");
      ZW_DEBUG_MYPRODUCT_SEND_NL();
      ZW_WatchDogEnable(); /*reset asic*/
      for (;;) {}
      break;

    case STATE_APP_OTA:
      if(EVENT_APP_REFRESH_MMI == event){}
      /*OTA state... do nothing until firmware update is finish*/
      break;

    case STATE_APP_TRANSMIT_DATA:
      if(EVENT_APP_REFRESH_MMI == event)
      {
        // Nothing here.
      }

      if ((EVENT_KEY_B6_UP == event) || (EVENT_KEY_B4_UP == event) || (EVENT_KEY_B2_UP == event))
      {
        /*
         * In the case of a key up event we cache the event because we need to be sure that it is
         * processed when the transmission is done (event = EVENT_APP_FINISH_EVENT_JOB). We need to
         * be sure of that because a hold event started an infinite transmission of Central Scene
         * Notifications and if we miss a key up event during transmission, the transmissions
         * will never stop.
         *
         * All other key events are dropped while in the transmit state.
         */
        cached_key_event = event;
      }

      if(EVENT_APP_NEXT_EVENT_JOB == event)
      {
        uint8_t event;
        /*check job-queue*/
        if(TRUE == ZAF_jobDequeue(&event))
        {
          /*Let the event scheduler fire the event on state event machine*/
          ZCB_eventSchedulerEventAdd(event);
        }
        else{
          ZCB_eventSchedulerEventAdd(EVENT_APP_FINISH_EVENT_JOB);
        }
      }

      if (EVENT_APP_CENTRAL_SCENE_JOB == event)
      {
        if (KEY_HOLD == keyEventGlobal)
        {
          if (0xFF == centralSceneHoldTimerHandle)
          {
            // Start the timer only if it's not started already.
            centralSceneHoldTimerHandle = ZW_TimerLongStart(
                ZCB_CentralSceneHoldTimerCallback,
                ( 0 == MemoryGetByte((WORD)&EEOFFSET_CENTRAL_SCENE_SLOW_REFRESH_far) ) ? 200 : 55000,
                TIMER_FOREVER);
          }
        }
        else if (KEY_UP == keyEventGlobal)
        {
          if (0xFF != centralSceneHoldTimerHandle)
          {
            ZW_TimerLongCancel(centralSceneHoldTimerHandle);
            centralSceneHoldTimerHandle = 0xFF;
          }
        }

        InitiateCentralSceneTX(
          centralSceneKeyAttributeHold,
          centralSceneSceneHold);
      }

      if (EVENT_APP_CC_BASIC_JOB == event)
      {
        ZW_DEBUG_MYPRODUCT_SEND_NL();
        ZW_DEBUG_MYPRODUCT_SEND_STR("EVENT_APP_CC_BASIC_JOB");
        if(JOB_STATUS_SUCCESS != CmdClassBasicSetSend( &nextJob.profile, m_endpoint, buttonStates[m_endpoint - 1],ZCB_TransmitCallback))
        {
          ZW_DEBUG_MYPRODUCT_SEND_STR("EVENT_APP_CC_BASIC_JOB failed!");
          ZW_DEBUG_MYPRODUCT_SEND_NL();
          ZCB_eventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
        }
      }

      if(EVENT_APP_CC_SWITCH_MULTILEVEL_JOB == event)
      {
        JOB_STATUS jobStatus = InitiateTransmission();
        ZW_DEBUG_MYPRODUCT_SEND_NL();
        ZW_DEBUG_MYPRODUCT_SEND_STR("EVENT_APP_CC_SWITCH_MULTILEVEL_JOB");
        if (JOB_STATUS_SUCCESS != jobStatus)
        {
          TRANSMISSION_RESULT transmissionResult = {0, 0, TRANSMISSION_RESULT_FINISHED};
          ZW_DEBUG_MYPRODUCT_SEND_STR("CC_SWITCH_MULTILEVEL ERROR");
          ZCB_TransmitCallback(&transmissionResult);
        }
      }

      if (EVENT_APP_FINISH_EVENT_JOB == event)
      {
        ZW_DEBUG_MYPRODUCT_SEND_NL();
        ZW_DEBUG_MYPRODUCT_SEND_STR("TransmitDone.");
        if (EVENT_KEY_MAX != cached_key_event)
        {
          /*
           * Add the cached to the event queue.
           */
          ZCB_eventSchedulerEventAdd(cached_key_event);
          cached_key_event = EVENT_KEY_MAX;
        }
        ChangeState(STATE_APP_IDLE);
      }
      break;

    default:
      // Do nothing.
      break;
  }
}

/**
 * @brief Sets the current state to a new, given state.
 * @param newState New state.
 */
static void
ChangeState(STATE_APP newState)
{
  ZW_DEBUG_MYPRODUCT_SEND_NL();
  ZW_DEBUG_MYPRODUCT_SEND_STR("State changed: ");
  ZW_DEBUG_MYPRODUCT_SEND_NUM(currentState);
  ZW_DEBUG_MYPRODUCT_SEND_STR(" -> ");
  ZW_DEBUG_MYPRODUCT_SEND_NUM(newState);

  currentState = newState;

  /**< Pre-action on new state is to refresh MMI */
  ZCB_eventSchedulerEventAdd(EVENT_APP_REFRESH_MMI);
}

/**
 * @brief Transmission callback for Device Reset Locally call.
 * @param pTransmissionResult Result of each transmission.
 */
PCB(ZCB_DeviceResetLocallyDone)(TRANSMISSION_RESULT * pTransmissionResult)
{
  if(TRANSMISSION_RESULT_FINISHED == pTransmissionResult->isFinished)
  {
    /* CHANGE THIS - clean your own application data from NVM*/
    ZW_MEM_PUT_BYTE((WORD)&EEOFFSET_MAGIC_far, 1 + APPL_MAGIC_VALUE);
    ZCB_eventSchedulerEventAdd((EVENT_APP) EVENT_SYSTEM_WATCHDOG_RESET);
  }
}

#ifdef BOOTLOADER_ENABLED
/**
 * @brief Called when OTA firmware upgrade is finished. Reboots node to cleanup
 * and starts on new FW.
 * @param OTA_STATUS otaStatus
 */
PCB(ZCB_OTAFinish)(OTA_STATUS otaStatus)
{
  UNUSED(otaStatus);
  /*Just reboot node to cleanup and start on new FW.*/
  ZW_WatchDogEnable(); /*reset asic*/
  while(1);
}


/**
 * @brief Function pointer for KEIL.
 */
code const BOOL (code * ZCB_OTAStart_p)(void) = &ZCB_OTAStart;
/**
 * @brief Called before OTA firmware upgrade starts.
 * @details Checks whether the application is ready for a firmware upgrade.
 * @return FALSE if OTA should be rejected, otherwise TRUE.
 */
BOOL
ZCB_OTAStart(void)
{
  BOOL  status = FALSE;
  if (STATE_APP_IDLE == GetAppState())
  {
    status = TRUE;
  }
  return status;
}
#endif

/**
 * @brief See description for function prototype in CommandClassVersion.h.
 */
uint8_t handleNbrFirmwareVersions()
{
  return 1; /*CHANGE THIS - firmware 0 version*/
}


/**
 * See description for function prototype in CommandClassVersion.h.
 */
void CC_Version_GetFirmwareVersion_handler(
    uint8_t firmwareTargetIndex,
    VG_VERSION_REPORT_V2_VG* pVariantgroup)
{
  /*firmware 0 version and sub version*/
  if (0 == firmwareTargetIndex)
  {
    pVariantgroup->firmwareVersion = APP_VERSION;
    pVariantgroup->firmwareSubVersion = APP_REVISION;
  }
  else
  {
    /*Just set it to 0 if firmware n is not present*/
    pVariantgroup->firmwareVersion = 0;
    pVariantgroup->firmwareSubVersion = 0;
  }
}


/**
 * @brief Function return firmware Id of target n (0 => is device FW ID)
 * n read version of firmware number n (0,1..N-1)
 * @return firmaware ID.
 */
WORD
handleFirmWareIdGet( uint8_t n)
{
  if(n == 0)
  {
    return APP_FIRMWARE_ID;
  }
  return 0;
}


/**
 * @brief Prepares the transmission of commands stored in the AGI table.
 *
 * @param profile The profile key.
 * @param srcEndpoint The source endpoint.
 */
static void
PrepareAGITransmission(
        uint8_t profile,
        uint8_t nextActiveButton)
{
      ZW_DEBUG_MYPRODUCT_SEND_NL();
      ZW_DEBUG_MYPRODUCT_SEND_STR("PrepareAGITransmission");
      ZW_DEBUG_MYPRODUCT_SEND_NUM(keyEventGlobal);

    nextJob.profile.profile_MS = ASSOCIATION_GROUP_INFO_REPORT_PROFILE_CONTROL;
    nextJob.profile.profile_LS = profile;

  m_endpoint = nextActiveButton;
  if (KEY_PRESS == keyEventGlobal)
  {
    ZAF_jobEnqueue(EVENT_APP_CC_BASIC_JOB);
    if (0xFF == buttonStates[m_endpoint - 1])
    {
      /*
       * If button is on, turn device off.
       */
      buttonStates[m_endpoint - 1] = 0x00;
      ZW_DEBUG_MYPRODUCT_SEND_NL();
      ZW_DEBUG_MYPRODUCT_SEND_STR("Basic OFF");
    }
    else
    {
      /*
       * If button is off, turn device on.
       */
      buttonStates[m_endpoint - 1] = 0xFF;
      ZW_DEBUG_MYPRODUCT_SEND_NL();
      ZW_DEBUG_MYPRODUCT_SEND_STR("Basic ON");
    }
  }
  else if (KEY_HOLD == keyEventGlobal)
  {
    ZW_DEBUG_MYPRODUCT_SEND_NL();
    ZW_DEBUG_MYPRODUCT_SEND_STR("pre EVENT_APP_CC_SWITCH_MULTILEVEL_JOB");
    ZAF_jobEnqueue(EVENT_APP_CC_SWITCH_MULTILEVEL_JOB);
    if (CCMLS_PRIMARY_SWITCH_UP == multiLevelDirection)
    {
      multiLevelDirection = CCMLS_PRIMARY_SWITCH_DOWN;
    }
    else
    {
      multiLevelDirection = CCMLS_PRIMARY_SWITCH_UP;
    }
  }
  else if (KEY_UP == keyEventGlobal)
  {
    ZW_DEBUG_MYPRODUCT_SEND_NL();
    ZW_DEBUG_MYPRODUCT_SEND_STR("pre KEY_UP EVENT_APP_CC_SWITCH_MULTILEVEL_JOB");
    ZAF_jobEnqueue(EVENT_APP_CC_SWITCH_MULTILEVEL_JOB);
  }
}

/**
 * Initiates a Central Scene Notification to the lifeline.
 * We don't care about the result, since we have to proceed no matter what.
 * Therefore a callback function is called in any case.
 * @param keyAttribute The key attribute in action.
 * @param sceneNumber The scene in action.
 */
static void InitiateCentralSceneTX(uint8_t keyAttribute, uint8_t sceneNumber)
{
  AGI_PROFILE lifelineProfile = {
      ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL_NA_V2,
      ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL_LIFELINE
  };

  JOB_STATUS jobStatus = CommandClassCentralSceneNotificationTransmit(
          &lifelineProfile,
          ENDPOINT_ROOT, /* We always send Central Scene Notification from root device */
          keyAttribute,
          sceneNumber,
          ZCB_TransmitCallback);

  if (JOB_STATUS_SUCCESS != jobStatus)
  {
    TRANSMISSION_RESULT transmissionResult;
    transmissionResult.nodeId = 0;
    transmissionResult.status = TRANSMIT_COMPLETE_FAIL;
    transmissionResult.isFinished = TRANSMISSION_RESULT_FINISHED;
    ZW_DEBUG_MYPRODUCT_SEND_NL();
    ZW_DEBUG_MYPRODUCT_SEND_STR("LL failure");
    ZCB_TransmitCallback(&transmissionResult);
  }
  else
  {
    ZW_DEBUG_MYPRODUCT_SEND_NL();
    ZW_DEBUG_MYPRODUCT_SEND_STR("LL success");
  }
}


/**
 * @brief Processes the transmission to related nodes.
 * @return Status of the job.
 */
static JOB_STATUS
InitiateTransmission(void)
{
    ZW_DEBUG_MYPRODUCT_SEND_NL();
    ZW_DEBUG_MYPRODUCT_SEND_STR("### ITrans");
    ZW_DEBUG_MYPRODUCT_SEND_NUM(keyEventGlobal);
  if (KEY_PRESS == keyEventGlobal)
  {
    ZW_DEBUG_MYPRODUCT_SEND_NL();
    ZW_DEBUG_MYPRODUCT_SEND_STR("### Multilevel TX. Set");
    /*
     * When pressing the button, we want to sent a basic set to the
     * destination node.
     */
    return CmdClassMultiLevelSwitchSetTransmit(
                &nextJob.profile,
                m_endpoint,
                ZCB_TransmitCallback,
                buttonStates[m_endpoint - 1],
                0);
  }
  else if (KEY_HOLD == keyEventGlobal)
  {
    ZW_DEBUG_MYPRODUCT_SEND_NL();
    ZW_DEBUG_MYPRODUCT_SEND_STR("### Multilevel TX.Change");

    return CmdClassMultiLevelSwitchStartLevelChange(
                &nextJob.profile,
                m_endpoint,
                ZCB_TransmitCallback,
                multiLevelDirection,
                CCMLS_IGNORE_START_LEVEL_TRUE,
                CCMLS_SECONDARY_SWITCH_NO_INC_DEC,
                0,
                2,
                0);
  }
  else if (KEY_UP == keyEventGlobal)
  {
    ZW_DEBUG_MYPRODUCT_SEND_NL();
    ZW_DEBUG_MYPRODUCT_SEND_STR("### Multilevel Stop level change");
    return CmdClassMultiLevelSwitchStopLevelChange(
                &nextJob.profile,
                m_endpoint,
                ZCB_TransmitCallback);
  }
  return JOB_STATUS_BUSY;
}

/**
 * @brief Callback function setting the application state.
 * @details Sets the application state when done transmitting.
 * @param pTransmissionResult Result of each transmission.
 */
PCB(ZCB_TransmitCallback)(TRANSMISSION_RESULT * pTransmissionResult)
{
  ZW_DEBUG_MYPRODUCT_SEND_NL();
  ZW_DEBUG_MYPRODUCT_SEND_STR("TX CB for N");
  ZW_DEBUG_MYPRODUCT_SEND_NUM(pTransmissionResult->nodeId);
  ZW_DEBUG_MYPRODUCT_SEND_STR(": ");
  ZW_DEBUG_MYPRODUCT_SEND_NUM(pTransmissionResult->status);

  if (TRANSMISSION_RESULT_FINISHED == pTransmissionResult->isFinished)
  {
    ZCB_eventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
  }
}

/**
 * @brief See description for function prototype in CommandClassCentrralScene.h.
 */
uint8_t getAppCentralSceneReportData(ZW_CENTRAL_SCENE_SUPPORTED_REPORT_1BYTE_V3_FRAME * pData)
{
  pData->supportedScenes = 3; // Number of buttons
  pData->properties1 = (1 << 1) | 1; // 1 bit mask byte & All keys are identical.
  pData->variantgroup1.supportedKeyAttributesForScene1 = 0x07; // 0b00000111.
  return 1;
}

/**
 * @brief See description for function prototype in CommandClassCentrralScene.h.
 */
void getAppCentralSceneConfiguration(central_scene_configuration_t * pConfiguration)
{
  pConfiguration->slowRefresh = MemoryGetByte((uint16_t)&EEOFFSET_CENTRAL_SCENE_SLOW_REFRESH_far);
}

/**
 * @brief See description for function prototype in CommandClassCentrralScene.h.
 */
void setAppCentralSceneConfiguration(central_scene_configuration_t * pConfiguration)
{
  ZW_MEM_PUT_BYTE((WORD)&EEOFFSET_CENTRAL_SCENE_SLOW_REFRESH_far, pConfiguration->slowRefresh);
}

/**
 * @brief Called when protocol needs to inform Application about a Security Event.
 * @details If the app does not need/want the Security Event the handling can be left empty.
 *
 *    Event E_APPLICATION_SECURITY_EVENT_S2_INCLUSION_REQUEST_DSK_CSA
 *          If CSA is needed, the app must do the following when this event occures:
 *             1. Obtain user input with first 4 bytes of the S2 including node DSK
 *             2. Store the user input in a response variable of type s_SecurityS2InclusionCSAPublicDSK_t.
 *             3. Call ZW_SetSecurityS2InclusionPublicDSK_CSA(response)
 *
 */
void
ApplicationSecurityEvent(
  s_application_security_event_data_t *securityEvent)
{
  switch (securityEvent->event)
  {
#ifdef APP_SUPPORTS_CLIENT_SIDE_AUTHENTICATION
    case E_APPLICATION_SECURITY_EVENT_S2_INCLUSION_REQUEST_DSK_CSA:
      {
        ZW_SetSecurityS2InclusionPublicDSK_CSA(&sCsaResponse);
      }
      break;
#endif /* APP_SUPPORTS_CLIENT_SIDE_AUTHENTICATION */

    default:
      break;
  }
}

/**
* Set up security keys to request when joining a network.
* These are taken from the config_app.h header file.
*/
uint8_t ApplicationSecureKeysRequested(void)
{
  return REQUESTED_SECURITY_KEYS;
}

/**
* Set up security S2 inclusion authentication to request when joining a network.
* These are taken from the config_app.h header file.
*/
uint8_t ApplicationSecureAuthenticationRequested(void)
{
  return REQUESTED_SECURITY_AUTHENTICATION;
}

PCB(ZCB_CentralSceneHoldTimerCallback)(void)
{
  ZAF_jobEnqueue(EVENT_APP_CENTRAL_SCENE_JOB);
  if (STATE_APP_TRANSMIT_DATA != currentState)
  {
    ZCB_eventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
    ChangeState(STATE_APP_TRANSMIT_DATA);
  }
}
