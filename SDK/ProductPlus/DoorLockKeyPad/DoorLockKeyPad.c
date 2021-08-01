/**
 * Z-Wave Certified Application Door Lock Key Pad
 *
 * @copyright 2019 Silicon Laboratories Inc.
 */


/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include "config_app.h"
#include <app_version.h>

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

/*IO control*/
#include <io_zdp03a.h>

#include <ZW_task.h>
#include <ev_man.h>

#ifdef ZW_ISD51_DEBUG
#include "ISD51.h"
#endif

#include <association_plus.h>
#include <agi.h>
#include <CommandClassAssociation.h>
#include <CommandClassAssociationGroupInfo.h>
#include <CommandClassVersion.h>
#include <CommandClassZWavePlusInfo.h>
#include <CommandClassPowerLevel.h>
#include <CommandClassDeviceResetLocally.h>
#include <CommandclassDoorLock.h>
#include <CommandclassUserCode.h>
#include <CommandClassBattery.h>
#include <Commandclasssupervision.h>
#include <CommandClassMultiChan.h>
#include <CommandClassMultiChanAssociation.h>

#include <ZAF_pm.h>
#include <battery_monitor.h>

#include <nvm_util.h>

#ifdef TEST_INTERFACE_SUPPORT
#include <ZW_test_interface.h>
#include <ZW_uart_api.h>
#include <ZW_string.h>

#endif /*TEST_INTERFACE_SUPPORT*/

#include <zaf_version.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
/**
 * @def ZW_DEBUG_DOORLOCK_SEND_BYTE(data)
 * Transmits a given byte to the debug port.
 * @def ZW_DEBUG_DOORLOCK_SEND_STR(STR)
 * Transmits a given string to the debug port.
 * @def ZW_DEBUG_DOORLOCK_SEND_NUM(data)
 * Transmits a given number to the debug port.
 * @def ZW_DEBUG_DOORLOCK_SEND_WORD_NUM(data)
 * Transmits a given WORD number to the debug port.
 * @def ZW_DEBUG_DOORLOCK_SEND_NL()
 * Transmits a newline to the debug port.
 */
#ifdef ZW_DEBUG_DOORLOCK
#define ZW_DEBUG_DOORLOCK_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_DOORLOCK_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_DOORLOCK_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_DOORLOCK_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_DOORLOCK_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_DOORLOCK_SEND_BYTE(data)
#define ZW_DEBUG_DOORLOCK_SEND_STR(STR)
#define ZW_DEBUG_DOORLOCK_SEND_NUM(data)
#define ZW_DEBUG_DOORLOCK_SEND_WORD_NUM(data)
#define ZW_DEBUG_DOORLOCK_SEND_NL()
#endif



/**
 * Application events for AppStateManager(..)
 */
typedef enum _EVENT_APP_
{
  EVENT_EMPTY = DEFINE_EVENT_APP_NBR,
  EVENT_APP_INIT,
  EVENT_APP_REFRESH_MMI,
  EVENT_APP_NEXT_EVENT_JOB,
  EVENT_APP_FINISH_EVENT_JOB,
  EVENT_APP_GET_NODELIST,
  EVENT_APP_BATTERY_LOW_TX,
  EVENT_APP_IS_POWERING_DOWN,
  EVENT_APP_START_USER_CODE_EVENT,
  EVENT_APP_DOORLOCK_JOB, //2A
  EVENT_APP_START_KEYPAD_ACTIVE,
  EVENT_APP_FINISH_KEYPAD_ACTIVE,
  EVENT_APP_SMARTSTART_IN_PROGRESS
} EVENT_APP;


/**
 * Application states. Function AppStateManager(..) includes the state
 * event machine.
 */
typedef enum _STATE_APP_
{
  STATE_APP_STARTUP,
  STATE_APP_IDLE,
  STATE_APP_LEARN_MODE,
  STATE_APP_WATCHDOG_RESET,
  STATE_APP_OTA,
  STATE_APP_TRANSMIT_DATA,
  STATE_UNSOLICITED_EVENT,
  STATE_BATT_DEAD,
  STATE_APP_POWERDOWN,
  STATE_APP_USER_KEYPAD
 } STATE_APP;


#define MAX_KEY_LEN 4

#define DOOR_HANDLE_1 (0x1)

#define ENABLE_HANDLE(mask, handle)  do { mask |= handle; } while(0)
#define DISABLE_HANDLE(mask, handle) do { mask &= ~handle; } while(0)

#define ENABLE_OUTSIDE_HANDLE(handle)  ENABLE_HANDLE(myDoorLock.outsideDoorHandleMode, handle)
#define DISABLE_OUTSIDE_HANDLE(handle) DISABLE_HANDLE(myDoorLock.outsideDoorHandleMode, handle)

#define IS_BIT_SET(mask, bit) (mask & bit)
#define IS_BIT_CLEARED(mask, bit) ((~mask) & bit)

#define IS_HANDLE_ENABLED(mask, handle)  IS_BIT_SET(mask, handle)
#define IS_HANDLE_DISABLED(mask, handle) IS_BIT_CLEARED(mask, handle)

#define IS_OUTSIDE_HANDLE_ENABLED(handle) IS_HANDLE_ENABLED(myDoorLock.outsideDoorHandleMode, handle)
#define IS_OUTSIDE_HANDLE_DISABLED(handle) IS_HANDLE_DISABLED(myDoorLock.outsideDoorHandleMode, handle)

#define IS_HANDLE_PRESSED(mask, handle)  IS_BIT_SET(mask, handle)
#define IS_HANDLE_RELEASED(mask, handle) IS_BIT_CLEARED(mask, handle)

#define IS_OUTSIDE_HANDLE_PRESSED(handle) IS_HANDLE_PRESSED(myDoorLock.outsideDoorHandleState, handle)
#define IS_OUTSIDE_HANDLE_RELEASED(handle) IS_HANDLE_RELEASED(myDoorLock.outsideDoorHandleState, handle)

#define UNLOCK_BOLT() do { myDoorLock.condition |= 0x02; } while(0)
#define LOCK_BOLT()   do { myDoorLock.condition &= ~0x02; } while(0)
#define TOGGLE_BOLT() do { myDoorLock.condition ^= 0x02; } while(0)

#define CLOSE_LATCH() do { myDoorLock.condition |= 0x04; } while(0)
#define OPEN_LATCH()  do { myDoorLock.condition &= ~0x04; } while(0)

#define IS_LATCH_CLOSED() IS_BIT_SET(myDoorLock.condition, 0x04)
#define IS_LATCH_OPEN()   IS_BIT_CLEARED(myDoorLock.condition, 0x04)

#define IS_BOLT_UNLOCKED() IS_BIT_SET(myDoorLock.condition, 0x02)
#define IS_BOLT_LOCKED()   IS_BIT_CLEARED(myDoorLock.condition, 0x02)

#define PRESS_OUTSIDE_HANDLE(handle)   do { myDoorLock.outsideDoorHandleState |= handle; } while(0)
#define RELEASE_OUTSIDE_HANDLE(handle) do { myDoorLock.outsideDoorHandleState &= ~handle; } while(0)

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

/**
 * Unsecure node information list.
 * Be sure Command classes are not duplicated in both lists.
 * CHANGE THIS - Add all supported non-secure command classes here
 **/
static code BYTE cmdClassListNonSecureNotIncluded[] =
{
  COMMAND_CLASS_ZWAVEPLUS_INFO,
  COMMAND_CLASS_SUPERVISION,
  COMMAND_CLASS_TRANSPORT_SERVICE_V2,
  COMMAND_CLASS_SECURITY,
  COMMAND_CLASS_SECURITY_2
};


/**
 * Unsecure node information list Secure included.
 * Be sure Command classes are not duplicated in both lists.
 * CHANGE THIS - Add all supported non-secure command classes here
 **/
static code BYTE cmdClassListNonSecureIncludedSecure[] =
{
  COMMAND_CLASS_ZWAVEPLUS_INFO,
  COMMAND_CLASS_SUPERVISION,
  COMMAND_CLASS_TRANSPORT_SERVICE_V2,
  COMMAND_CLASS_SECURITY,
  COMMAND_CLASS_SECURITY_2
};


/**
 * Secure node inforamtion list.
 * Be sure Command classes are not duplicated in both lists.
 * CHANGE THIS - Add all supported secure command classes here
 **/
static code BYTE cmdClassListSecure[] =
{
  COMMAND_CLASS_VERSION,
  COMMAND_CLASS_MANUFACTURER_SPECIFIC,
  COMMAND_CLASS_DEVICE_RESET_LOCALLY,
  COMMAND_CLASS_POWERLEVEL,
  COMMAND_CLASS_BATTERY,
  COMMAND_CLASS_DOOR_LOCK,
  COMMAND_CLASS_USER_CODE,
  COMMAND_CLASS_ASSOCIATION_V2,
  COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
  COMMAND_CLASS_ASSOCIATION_GRP_INFO
#ifdef BOOTLOADER_ENABLED
  ,COMMAND_CLASS_FIRMWARE_UPDATE_MD_V2
#endif
};


/**
 * Structure includes application node information list's and device type.
 */
static APP_NODE_INFORMATION m_AppNIF =
{
  cmdClassListNonSecureNotIncluded, sizeof(cmdClassListNonSecureNotIncluded),
  cmdClassListNonSecureIncludedSecure, sizeof(cmdClassListNonSecureIncludedSecure),
  cmdClassListSecure, sizeof(cmdClassListSecure),
  DEVICE_OPTIONS_MASK, GENERIC_TYPE, SPECIFIC_TYPE
};

/**
 * AGI lifeline string
 */
const char GroupName[]   = "Lifeline";

/**
 * Setup AGI lifeline table from app_config.h
 */
static CMD_CLASS_GRP  agiTableLifeLine[] = {AGITABLE_LIFELINE_GROUP};

/**
 * Setup AGI root device groups table from app_config.h
 */

static AGI_GROUP agiTableRootDeviceGroups[] = {AGITABLE_ROOTDEVICE_GROUPS};

static BYTE myNodeID = 0;


/**
 * Application state-machine state.
 */
static STATE_APP currentState = STATE_APP_IDLE;

/**
 * Parameter is used to save wakeup reason after ApplicationInitHW(..)
 */
static wakeup_reason_t wakeupReason;

/**
 * Command Class Door lock
 * Following parameters are not supported in application:
 * insideDoorHandleMode: Inside Door Handles Mode (4 bits)
 * outsideDoorHandleMode: Outside Door Handles Mode
 * condition: Door condition
 */
static CMD_CLASS_DOOR_LOCK_DATA myDoorLock;


/**
 * Used by the Supervision Get handler. Holds RX options.
 */
static RECEIVE_OPTIONS_TYPE_EX rxOptionSupervision;

/**
 * Used by the Supervision Get handler. Holds Supervision session ID.
 */
static BYTE sessionID;

#ifdef APP_SUPPORTS_CLIENT_SIDE_AUTHENTICATION
#ifndef TEST_INTERFACE_SUPPORT
s_SecurityS2InclusionCSAPublicDSK_t sCSAResponse = { 0, 0, 0, 0};
#endif /* TEST_INTERFACE_SUPPORT */
#endif /* APP_SUPPORTS_CLIENT_SIDE_AUTHENTICATION */

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

void ZCB_BattReportSentDone(TRANSMISSION_RESULT * pTransmissionResult);
void ZCB_DeviceResetLocallyDone(TRANSMISSION_RESULT * pTransmissionResult);
STATE_APP GetAppState();
void AppStateManager( EVENT_APP event);
void ChangeState( STATE_APP newState);
void ZCB_JobStatus(TRANSMISSION_RESULT * pTransmissionResult);
static void SaveStatus(void);
BOOL Feedkey(BYTE keyNbr);
void SetCondition( void );
JOB_STATUS DoorLockOperationReportTransmit(AGI_PROFILE* pAgiProfile);

static BOOL ApplicationIdle(void);
#ifdef BOOTLOADER_ENABLED
void ZCB_OTAFinish(OTA_STATUS otaStatus);
BOOL ZCB_OTAStart();
#endif

void ZCB_CommandClassSupervisionGetReceived(SUPERVISION_GET_RECEIVED_HANDLER_ARGS * pArgs);
void ZCB_SupervisionTimerCallback(void);
void StartKeypadApp(void);
void ZCB_AppKeypad(BYTE channel, BYTE * pString);
void DefaultApplicationsSettings(void);

static void SetTimeoutValues(void)
{
  // No support for timed operation.
  myDoorLock.lockTimeoutMin = DOOR_LOCK_OPERATION_SET_TIMEOUT_NOT_SUPPORTED;
  myDoorLock.lockTimeoutSec = DOOR_LOCK_OPERATION_SET_TIMEOUT_NOT_SUPPORTED;
}

static void ResetMyDoorLock(void)
{
  memset((uint8_t *)&myDoorLock, 0, sizeof(myDoorLock));

  // We support 1 outside door handle and zero inside door handles.
  ENABLE_OUTSIDE_HANDLE(DOOR_HANDLE_1);

  // We support constant operation only.
  myDoorLock.type = DOOR_OPERATION_CONST;
}

/**
 * @brief See description for function prototype in ZW_basis_api.h.
 */
void
ApplicationRfNotify(BYTE rfState)
{
  UNUSED(rfState);
}

/**
 * @brief See description for function prototype in ZW_basis_api.h.
 */
BYTE
ApplicationInitHW(SW_WAKEUP bWakeupReason)
{
  wakeupReason = bWakeupReason;

  /* hardware initialization */
  ZDP03A_InitHW(ZCB_eventSchedulerEventAdd);

  //Read port state and set it before setting port as output.
  Led(ZDP03A_LED_D1, KeyGet((ZDP03A_KEY) ZDP03A_LED_D1));
  SetPinOut(ZDP03A_LED_D1);
  Led(ZDP03A_LED_D2, KeyGet((ZDP03A_KEY) ZDP03A_LED_D2));
  SetPinOut(ZDP03A_LED_D2);
  Led(ZDP03A_LED_D3,OFF);
  SetPinOut(ZDP03A_LED_D3);

  SetPinIn(ZDP03A_KEY_1,TRUE);
  SetPinIn(ZDP03A_KEY_2,TRUE);
  SetPinIn(ZDP03A_KEY_4,TRUE);
  SetPinIn(ZDP03A_KEY_6,TRUE);

  InitBatteryMonitor(wakeupReason);
  Transport_OnApplicationInitHW(bWakeupReason);

  return(TRUE);
}

/**
 * @brief See description for function prototype in ZW_basis_api.h.
 */
BYTE
ApplicationInitSW(ZW_NVM_STATUS nvmStatus)
{
  BYTE application_node_type = DEVICE_OPTIONS_MASK;
  /* Init state machine*/
  currentState = STATE_APP_STARTUP;
  /* Do not reinitialize the UART if already initialized for ISD51 in ApplicationInitHW() */
#ifndef ZW_ISD51_DEBUG
  ZW_DEBUG_INIT(1152);
#endif
  ZW_DEBUG_DOORLOCK_SEND_STR("AppInitSW ");
  ZW_DEBUG_DOORLOCK_SEND_NUM(wakeupReason);
  ZW_DEBUG_DOORLOCK_SEND_NL();

#ifdef WATCHDOG_ENABLED
  ZW_WatchDogEnable();
#endif

  /*Check battery level*/
  /*Check if battery Monitor has no state change and last time was battery state ST_BATT_DEAD*/
  if((FALSE == BatterySensorRead(NULL)) && (ST_BATT_DEAD ==BatteryMonitorState()))
  {
    //ZW_DEBUG_DOORLOCK_SEND_NL();
    //ZW_DEBUG_DOORLOCK_SEND_STR("DEAD BATT!");
    //ZW_DEBUG_DOORLOCK_SEND_NL();
    /*just power down! woltage to low.*/
    AppStateManager(EVENT_APP_IS_POWERING_DOWN);
    return application_node_type;
  }


#ifdef BOOTLOADER_ENABLED
  NvmInit(nvmStatus);

#else
  UNUSED(nvmStatus);
#endif


  ZAF_pm_Init(wakeupReason);

  /*
   * Initialize Event Scheduler.
   */
  ZAF_eventSchedulerInit(AppStateManager);

  /* get stored values */
  if (MemoryGetByte((WORD)&EEOFFSET_MAGIC_far) == APPL_MAGIC_VALUE)
  {
    myDoorLock.condition = MemoryGetByte((WORD)&EEOFFSET_DOOR_LOCK_CONDITION_far);
    myDoorLock.type = MemoryGetByte((WORD)&EEOFFSET_OPERATION_TYPE_far);
    myDoorLock.outsideDoorHandleMode = MemoryGetByte((WORD) &EEOFFSET_HANDLES_MODE_far) >> 4;

    loadStatusPowerLevel();
    AssociationInit(FALSE);

    //ZW_DEBUG_DOORLOCK_SEND_STR("*** LOAD DATA!!*** ");
    //ZW_DEBUG_DOORLOCK_SEND_NUM(myDoorLock.type);
  }
  else
  {

    //ZW_DEBUG_DOORLOCK_SEND_STR("*** DEFAULT DATA!!*** ");
    ZW_MEM_PUT_BYTE((WORD)&EEOFFS_SECURITY_RESERVED.EEOFFS_MAGIC_BYTE_field, EEPROM_MAGIC_BYTE_VALUE);
    DefaultApplicationsSettings();
    ActivateBattNotificationTrigger();

    /* Initialize transport layer NVM */
    Transport_SetDefault();
    /* Reset protocol */
    ZW_SetDefault();
    AssociationInit(TRUE);

    /* Initialize PowerLevel functionality.*/
    loadInitStatusPowerLevel();
    SaveStatus(); /* Now EEPROM should be OK */
  }
  SetTimeoutValues();

  /*1. Set LED condition */
  SetCondition();
  /* Initialize association module */
  /* Setup AGI group lists*/
  AGI_Init();
  AGI_LifeLineGroupSetup(agiTableLifeLine, (sizeof(agiTableLifeLine)/sizeof(CMD_CLASS_GRP)), GroupName, ENDPOINT_ROOT);
  AGI_ResourceGroupSetup(agiTableRootDeviceGroups, (sizeof(agiTableRootDeviceGroups)/sizeof(AGI_GROUP)), ENDPOINT_ROOT);

  /* Get this sensors identification on the network */
  MemoryGetID(NULL, &myNodeID);
  /* Initialize manufactory specific module */
  ManufacturerSpecificDeviceIDInit();

#ifdef BOOTLOADER_ENABLED
  /* Initialize OTA module */
  OtaInit( ZCB_OTAStart, NULL, ZCB_OTAFinish);
#endif

  CC_Version_SetApplicationVersionInfo(APP_VERSION, APP_REVISION, APP_VERSION_PATCH, APP_BUILD_NUMBER);

  CommandClassSupervisionInit(
      CC_SUPERVISION_STATUS_UPDATES_NOT_SUPPORTED,
      ZCB_CommandClassSupervisionGetReceived,
      NULL);

  Transport_OnApplicationInitSW(&m_AppNIF);
  ZCB_eventSchedulerEventAdd(EVENT_APP_INIT);

  return application_node_type;
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
 * @brief Returns whether the application is ready to power down.
 * @return TRUE if ready to power down, FALSE otherwise.
 */
static BOOL ApplicationIdle(void)
{
  BOOL status = FALSE;

  if(STATE_APP_POWERDOWN == GetAppState()) //low battery
  {
    status = TRUE;
  }
  else
  {
    BOOL task_poll_status = TaskApplicationPoll();
    STATE_APP appState = GetAppState();
    BOOL activeTxJobs = ZAF_mutex_isActive();
    BOOL plStatus = CC_Powerlevel_isInProgress();
    BOOL keepAwakeTimerActive = ZAF_pm_IsActive();
    BOOL otaUpdateStatus = Ota_UpdateIsInProgress();

    if ((FALSE == task_poll_status) && // Check Task Handler for active task.
        (STATE_APP_IDLE == appState) && //Check application is in idle state.
        (FALSE == activeTxJobs) && //Check ZW_tx_mutex for active jobs, then buffers occupied.
        (FALSE == plStatus) && //Check CC power level for active test job.
        (FALSE == otaUpdateStatus)) //Check OTA FirmwareUpdate for update in progress.
    {
      /*
       * Check event queue for queued up jobs
       */
      if (0 != ZAF_jobQueueCount())
      {
        ZCB_eventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
        ChangeState(STATE_APP_TRANSMIT_DATA);
      }

      /*Check battery before shut down*/
      if ((SW_WAKEUP_RESET == wakeupReason) && (TRUE == TimeToSendBattReport()))
      {
        ZW_DEBUG_DOORLOCK_SEND_NL();
        ZW_DEBUG_DOORLOCK_SEND_STR("BR");
        ZAF_jobEnqueue(EVENT_APP_BATTERY_LOW_TX);
        ZCB_eventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
        ChangeState(STATE_APP_TRANSMIT_DATA);
      }
      else
      {
        if ((FALSE == keepAwakeTimerActive) ||
            ((0 == myNodeID) && (SW_WAKEUP_SMARTSTART == wakeupReason)))
        {
          Led(ZDP03A_LED_D3, OFF);
          status = TRUE;
        }
      }
    }
  }
  return status;
}


/**
 * @brief See description for function prototype in ZW_TransportEndpoint.h.
 */
received_frame_status_t
Transport_ApplicationCommandHandlerEx(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  BYTE cmdLength)
{
  received_frame_status_t frame_status = RECEIVED_FRAME_STATUS_NO_SUPPORT;
  //ZW_DEBUG_DOORLOCK_SEND_NL();
  //ZW_DEBUG_DOORLOCK_SEND_STR("TAppH");
  //ZW_DEBUG_DOORLOCK_SEND_NUM(pCmd->ZW_Common.cmdClass);

  /* Call command class handlers */
  switch (pCmd->ZW_Common.cmdClass)
  {
    case COMMAND_CLASS_VERSION:
      frame_status = CC_Version_handler(rxOpt, pCmd, cmdLength);
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

    case COMMAND_CLASS_BATTERY:
      frame_status = handleCommandClassBattery(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_USER_CODE:
      frame_status = handleCommandClassUserCode(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_DOOR_LOCK_V2:
      frame_status = handleCommandClassDoorLock(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_SUPERVISION:
      frame_status = handleCommandClassSupervision(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2:
      frame_status = handleCommandClassMultiChannelAssociation(rxOpt, pCmd, cmdLength);
      break;
  }
  return frame_status;
}

/**
 * @brief See description for function prototype in CommandClassVersion.h.
 */
BYTE
handleCommandClassVersionAppl( BYTE cmdClass )
{
  BYTE commandClassVersion = UNKNOWN_VERSION;

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
     commandClassVersion = CC_Powerlevel_getVersion();
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

    case COMMAND_CLASS_DOOR_LOCK_V2:
     commandClassVersion = CommandClassDoorLockVersionGet();
      break;

    case COMMAND_CLASS_BATTERY:
     commandClassVersion = CommandClassBatteryVersionGet();
      break;

    case COMMAND_CLASS_USER_CODE:
     commandClassVersion = CommandClassUserCodeVersionGet();
      break;

    case COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2:
      commandClassVersion = CmdClassMultiChannelAssociationVersion();
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
  BYTE bStatus,
  BYTE bNodeID,
  BYTE* pCmd,
  BYTE bLen)
{
  UNUSED(bStatus);
  UNUSED(bNodeID);
  UNUSED(pCmd);
  UNUSED(bLen);
  ZAF_pm_KeepAwakeAuto();
}

/**
 * See description for function prototype in ZW_slave_api.h.
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
        if (myNodeID == 0)
        {
          /*Clear association*/
          AssociationInit(TRUE);
          DefaultApplicationsSettings();
          ActivateBattNotificationTrigger();

          ZCB_eventSchedulerEventAdd(EVENT_APP_INIT);
          ChangeState(STATE_APP_STARTUP);
        }
        else
        {
          /* We are included! Inform controller device battery status by clearing flag */
          ActivateBattNotificationTrigger();
        }
      }
    }
    ZCB_eventSchedulerEventAdd((EVENT_APP) EVENT_SYSTEM_LEARNMODE_FINISH);
    Transport_OnLearnCompleted(bNodeID);
  }
  /* We need to be awake for controller to finished inclusion/exclusion */
  ZAF_pm_KeepAwakeAuto();
}


/**
 * @brief See description for function prototype in misc.h.
 */
BYTE
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
AppStateManager( EVENT_APP event)
{
   //ZW_DEBUG_DOORLOCK_SEND_STR("\r\nAppStateMan event ");
   //ZW_DEBUG_DOORLOCK_SEND_NUM(event);
   //ZW_DEBUG_DOORLOCK_SEND_STR(" st ");
   //ZW_DEBUG_DOORLOCK_SEND_NUM(currentState);
   //ZW_DEBUG_DOORLOCK_SEND_NL();

   if(EVENT_SYSTEM_WATCHDOG_RESET == event)
   {
     /*Force state change to activate watchdog-reset without taking care of current
       state.*/
     ChangeState(STATE_APP_WATCHDOG_RESET);
   }

   if(EVENT_APP_IS_POWERING_DOWN == event)
   {
     ChangeState(STATE_APP_POWERDOWN);
   }

  switch(currentState)
  {
  case STATE_APP_STARTUP:

    if (EVENT_APP_INIT == event)
    {
      ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_INCLUSION_SMARTSTART);
    }

    ChangeState(STATE_APP_IDLE);
    break;

    case STATE_APP_IDLE:
      if(EVENT_APP_REFRESH_MMI == event)
      {
        SetCondition();
        break;
      }

      if(EVENT_APP_SMARTSTART_IN_PROGRESS == event)
      {
        ChangeState(STATE_APP_LEARN_MODE);
      }

      if ((EVENT_KEY_B1_PRESS == event) || (EVENT_SYSTEM_LEARNMODE_START == event))
      {
        if (myNodeID)
        {
          //ZW_DEBUG_DOORLOCK_SEND_STR("LEARN_MODE_EXCLUSION");
          ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_EXCLUSION_NWE);
        }
        else
        {
          //ZW_DEBUG_DOORLOCK_SEND_STR("LEARN_MODE_INCLUSION");
          ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_INCLUSION);
        }
        ChangeState(STATE_APP_LEARN_MODE);
        break;
      }

      if((EVENT_KEY_B1_HELD_10_SEC == event) || (EVENT_SYSTEM_RESET ==event))
      {
        AGI_PROFILE lifelineProfile = {ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL_NA_V2, ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL_LIFELINE};
        handleCommandClassDeviceResetLocally(&lifelineProfile, ZCB_DeviceResetLocallyDone);
        break;
      }

      if(EVENT_KEY_B2_DOWN == event)
      {
        if (IS_BOLT_UNLOCKED())
        {
          PRESS_OUTSIDE_HANDLE(DOOR_HANDLE_1);
          SetCondition();
          SaveStatus();
        }
        break;
      }

      if(EVENT_KEY_B2_UP == event)
      {
        if (IS_BOLT_UNLOCKED())
        {
          RELEASE_OUTSIDE_HANDLE(DOOR_HANDLE_1);
          SetCondition();
          SaveStatus();
        }
        break;
      }

      if(EVENT_KEY_B4_UP == event)
      {
        ChangeState(STATE_APP_USER_KEYPAD);
        ZCB_eventSchedulerEventAdd(EVENT_APP_START_KEYPAD_ACTIVE);
      }


      if (EVENT_SYSTEM_OTA_START == event)
      {
        ChangeState(STATE_APP_OTA);
      }

      if (EVENT_APP_DOORLOCK_JOB == event)
      {
        ZAF_jobEnqueue(EVENT_APP_DOORLOCK_JOB);
      }

      break;

    case STATE_APP_LEARN_MODE:
      if(EVENT_APP_REFRESH_MMI == event)
      {
        SetCondition();
      }

      if (EVENT_KEY_B1_PRESS == event)
      {
        //ZW_DEBUG_DOORLOCK_SEND_STR("\r\n STATE_APP_LEARN_MODE disable");
        ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_DISABLE);
        ZCB_eventSchedulerEventAdd(EVENT_APP_INIT);
        ChangeState(STATE_APP_STARTUP);
      }
      if (EVENT_SYSTEM_LEARNMODE_FINISH == event)
      {
        //ZW_DEBUG_DOORLOCK_SEND_STR("\r\n STATE_APP_LEARN_MODE finish");
        ChangeState(STATE_APP_IDLE);
      }
      break;

    case STATE_APP_WATCHDOG_RESET:
      if (EVENT_APP_REFRESH_MMI == event){}

      ZW_WatchDogEnable(); /*reset asic*/
      for (;;) {}
      break;
    case STATE_APP_OTA:
      if(EVENT_APP_REFRESH_MMI == event){}
      /*OTA state... do nothing until firmware update is finish*/
      break;

    case STATE_APP_POWERDOWN:
      /* Device is powering down.. wait!*/
      break;

    case STATE_APP_TRANSMIT_DATA:
      if(EVENT_APP_REFRESH_MMI == event)
      {
        SetCondition();
        break;
      }

      if(EVENT_APP_NEXT_EVENT_JOB == event)
      {
        BYTE event;
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

      if(EVENT_APP_DOORLOCK_JOB == event)
      {
        if(JOB_STATUS_SUCCESS != DoorLockOperationReportTransmit(&agiTableRootDeviceGroups[0].profile))
        {
          ZCB_eventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
        }
      }

      if(EVENT_APP_BATTERY_LOW_TX == event)
      {
        if (JOB_STATUS_SUCCESS != SendBattReport( ZCB_BattReportSentDone ))
        {
          ActivateBattNotificationTrigger();
          ZCB_eventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
        }
      }

      if(EVENT_APP_FINISH_EVENT_JOB == event)
      {
        ChangeState(STATE_APP_IDLE);
      }
      break;

#ifdef TEST_INTERFACE_SUPPORT
      case STATE_APP_USER_KEYPAD:
      if(EVENT_APP_REFRESH_MMI == event)
      {
        SetCondition();
        Led(ZDP03A_LED_D3, ON);
        break;
      }

      if(EVENT_APP_START_KEYPAD_ACTIVE == event)
      {
        StartKeypadApp();
      }

      if(EVENT_APP_FINISH_KEYPAD_ACTIVE == event)
      {
        ChangeState(STATE_APP_IDLE);
      }

      if(EVENT_KEY_B4_UP == event)
      {
        ZCB_eventSchedulerEventAdd(EVENT_APP_FINISH_KEYPAD_ACTIVE);
      }

      break;
#endif
  }
}

/**
 * @brief Sets the current state to a new, given state.
 * @param newState New state.
 */
static void
ChangeState(STATE_APP newState)
{
 ZW_DEBUG_DOORLOCK_SEND_STR("ChangeState st = ");
 ZW_DEBUG_DOORLOCK_SEND_NUM(currentState);
 ZW_DEBUG_DOORLOCK_SEND_STR(" -> new st = ");
 ZW_DEBUG_DOORLOCK_SEND_NUM(newState);
 ZW_DEBUG_DOORLOCK_SEND_NL();

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
  ZW_DEBUG_DOORLOCK_SEND_BYTE('d');
  ZW_DEBUG_DOORLOCK_SEND_NUM(pTransmissionResult->status);
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
 * @brief See description for function prototype in CommandClassVersion.h.
 */
void
handleGetFirmwareVersion(
  BYTE bFirmwareNumber,
  VG_VERSION_REPORT_V2_VG *pVariantgroup)
{
  /*firmware 0 version and sub version*/
  if(bFirmwareNumber == 0)
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
handleFirmWareIdGet( BYTE n)
{
  if(n == 0)
  {
    return APP_FIRMWARE_ID;
  }
  return 0;
}


/**
 * @brief Send a Doorlock operation-support report
 * @param pAgiProfile Pointer to AGI profile
 * @return jo status
 */
JOB_STATUS
DoorLockOperationReportTransmit(AGI_PROFILE* pAgiProfile)
{
  CMD_CLASS_DOOR_LOCK_OPERATION_REPORT operation;

  operation.mode = IS_BOLT_LOCKED() ? DOOR_MODE_SECURED : DOOR_MODE_UNSEC;
  operation.insideDoorHandleMode = myDoorLock.insideDoorHandleMode;
  operation.outsideDoorHandleMode = myDoorLock.outsideDoorHandleMode;
  operation.condition = myDoorLock.condition;
  operation.lockTimeoutMin = myDoorLock.lockTimeoutMin;
  operation.lockTimeoutSec = myDoorLock.lockTimeoutSec;
  return CmdClassDoorLockOperationSupportReport( pAgiProfile, ENDPOINT_ROOT, &operation, ZCB_JobStatus);
}

/**
 * @brief Callback function used in CmdClassDoorLockOperationSupportReport.
 * @param pTransmissionResult Result of each transmission.
 */
PCB(ZCB_JobStatus)(TRANSMISSION_RESULT * pTransmissionResult)
{
  if (TRANSMISSION_RESULT_FINISHED == pTransmissionResult->isFinished)
  {
    ZCB_eventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
  }
}

/**
 * @brief validate a code agains idenfier usercode
 * @param[in] identifier user ID
 * @param[in] pCode pointer to pin code for validation agains pin code in NVM
 * @param[in] len length of the pin code
 */
BOOL
ValidateUserCode( BYTE identifier, BYTE* pCode, BYTE len)
{
  BYTE userCode[USERCODE_MAX_LEN];
  user_id_status_t user_id_status = MemoryGetByte((WORD) &EEOFFSET_USERSTATUS_far[identifier - 1]);

  if( (len == MemoryGetByte((WORD)&EEOFFSET_USERCODE_LEN_far[identifier - 1])) &&
      ((USER_ID_OCCUPIED == user_id_status) || (USER_ID_RESERVED == user_id_status)))
  {
    MemoryGetBuffer((WORD) &EEOFFSET_USERCODE_far[identifier - 1], userCode, len);

    if(0 == ZW_memcmp(pCode, userCode, len))
    {
     return TRUE;
    }
  }
  return FALSE;
}

/**
 * @brief See description for function prototype in CommandclassUserCode.h
 */
BOOL handleCommandClassUserCodeSet(
  BYTE identifier,
  user_id_status_t id,
  BYTE* pUserCode,
  BYTE len,
  BYTE endpoint )
{
  UNUSED(endpoint);

  /* If identifier == 0 -> set all user codes! */
  identifier = 01; /* We only have one for DoorLock! */
  if( identifier <= USER_ID_MAX)
  {
    MemoryPutByte((WORD)&EEOFFSET_USERSTATUS_far[identifier - 1], id);
    MemoryPutBuffer((WORD) &EEOFFSET_USERCODE_far[identifier - 1], pUserCode, len, NULL);
    MemoryPutByte((WORD)&EEOFFSET_USERCODE_LEN_far[identifier - 1], len);

    return TRUE;
  }
  return FALSE;
}


/**
 * @brief See description for function prototype in CommandclassUserCode.h
 */
BOOL
handleCommandClassUserCodeIdGet(
  BYTE identifier,
  user_id_status_t * pId,
  BYTE endpoint)
{
  UNUSED(endpoint);
  if( identifier <= USER_ID_MAX)
  {
    *pId = (user_id_status_t)MemoryGetByte((WORD) &EEOFFSET_USERSTATUS_far[identifier - 1]);
    return TRUE;
  }
  return FALSE;
}


/**
 * @brief See description for function prototype in CommandclassUserCode.h
 */
BOOL
handleCommandClassUserCodeReport(
  BYTE identifier,
  BYTE* pUserCode,
  BYTE* pLen,
  BYTE endpoint)
{
  UNUSED(endpoint);
  *pLen = MemoryGetByte((WORD)&EEOFFSET_USERCODE_LEN_far[identifier - 1]);
  if((identifier <= USER_ID_MAX) && (USERCODE_MAX_LEN >= *pLen))
  {
    MemoryGetBuffer((WORD) &EEOFFSET_USERCODE_far[identifier - 1], pUserCode, *pLen );

    return TRUE;
  }
  return FALSE;
}

/**
 * See description for function prototype in CommandClassDoorLock.h.
 */
BYTE
handleCommandClassUserCodeUsersNumberReport(BYTE endpoint)
{
  UNUSED(endpoint);
  return USER_ID_MAX;
}


/**
 * See description for function prototype in CommandClassDoorLock.h.
 */
BOOL handleCommandClassDoorLockOperationSet(door_mode_t mode)
{
  BOOL status = FALSE;
  if (DOOR_MODE_SECURED == mode)
  {
    LOCK_BOLT();
    DISABLE_OUTSIDE_HANDLE(DOOR_HANDLE_1);

    status = TRUE;
  }
  else if(DOOR_MODE_UNSEC == mode)
  {
    UNLOCK_BOLT();
    ENABLE_OUTSIDE_HANDLE(DOOR_HANDLE_1);

    status = TRUE;
  }

  if (TRUE == status)
  {
    /*Update condition and LED's*/
    SetCondition();

    SaveStatus();
  }
  return status;
}


/**
 * @brief See description for function prototype in CommandclassDoorLock.h
 */
void
handleCommandClassDoorLockOperationReport(CMD_CLASS_DOOR_LOCK_OPERATION_REPORT* pData)
{
  //ZW_DEBUG_DOORLOCK_SEND_STR("OperationReport");
  //ZW_DEBUG_DOORLOCK_SEND_NL();

  pData->mode = IS_BOLT_LOCKED() ? DOOR_MODE_SECURED : DOOR_MODE_UNSEC;
  pData->insideDoorHandleMode = myDoorLock.insideDoorHandleMode;
  pData->outsideDoorHandleMode = myDoorLock.outsideDoorHandleMode;
  pData->condition = myDoorLock.condition;
  pData->lockTimeoutMin = myDoorLock.lockTimeoutMin;
  pData->lockTimeoutSec = myDoorLock.lockTimeoutSec;
}


/**
 * @brief See description for function prototype in CommandClassDoorLock.h
 */
BOOL handleCommandClassDoorLockConfigurationSet(cc_door_lock_configuration_t * pData)
{
  // This application only supports the constant operation type.
  if (DOOR_OPERATION_CONST == pData->type)
  {
    // Set only the outside door handles because we do not support inside door handles.
    myDoorLock.outsideDoorHandleMode = pData->outsideDoorHandleMode;
    SaveStatus();
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}


/**
 * @brief See description for function prototype in CommandclassDoorLock.h
 */
void
handleCommandClassDoorLockConfigurationReport(CMD_CLASS_DOOR_LOCK_CONFIGURATION* pData)
{
  //ZW_DEBUG_DOORLOCK_SEND_STR("ConfigReport");
  //ZW_DEBUG_DOORLOCK_SEND_NL();

  pData->type   = myDoorLock.type;
  pData->insideDoorHandleMode = myDoorLock.insideDoorHandleMode;
  pData->outsideDoorHandleMode = myDoorLock.outsideDoorHandleMode;
  pData->lockTimeoutMin = myDoorLock.lockTimeoutMin;
  pData->lockTimeoutSec = myDoorLock.lockTimeoutSec;
}



/**
 * @brief Set lock conditons (LED) out from door handle mode state
 */
void
SetCondition( void )
{
  if (IS_OUTSIDE_HANDLE_ENABLED(DOOR_HANDLE_1) && IS_OUTSIDE_HANDLE_PRESSED(DOOR_HANDLE_1))
  {
    OPEN_LATCH();
  }
  else
  {
    CLOSE_LATCH();
  }

  //ZW_DEBUG_DOORLOCK_SEND_STR("SetCondition con = ");
  //ZW_DEBUG_DOORLOCK_SEND_NUM(myDoorLock.condition );
  //ZW_DEBUG_DOORLOCK_SEND_NL();

  Led(ZDP03A_LED_D1, IS_LATCH_CLOSED() ? ON : OFF);
  Led(ZDP03A_LED_D2, IS_BOLT_LOCKED() ? ON : OFF);
}


/**
 * @brief Callback function used when sending battery report.
 */
PCB(ZCB_BattReportSentDone)(TRANSMISSION_RESULT * pTransmissionResult)
{
  if (TRANSMIT_COMPLETE_OK != pTransmissionResult->status)
  {
    ActivateBattNotificationTrigger();
  }
  if (TRANSMISSION_RESULT_FINISHED == pTransmissionResult->isFinished)
  {
    ZCB_eventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
  }
}



/**
 * @brief Stores the current status of the lock on/off
 * in the application part of the EEPROM.
 */
static void SaveStatus(void)
{
  //ZW_DEBUG_DOORLOCK_SEND_STR("SaveStatus");
  //ZW_DEBUG_DOORLOCK_SEND_NL();
  MemoryPutByte((WORD)&EEOFFSET_OPERATION_TYPE_far, myDoorLock.type);
  MemoryPutByte((WORD)&EEOFFSET_DOOR_LOCK_CONDITION_far, myDoorLock.condition);
  MemoryPutByte((WORD)&EEOFFSET_HANDLES_MODE_far,
                (myDoorLock.insideDoorHandleMode) | (myDoorLock.outsideDoorHandleMode << 4));
  MemoryPutByte((WORD)&EEOFFSET_MAGIC_far, APPL_MAGIC_VALUE);
}

/**
 * @brief Handles a received Supervision Get command.
 * @details The purpose of Supervision is to inform the source node (controller) when the door lock
 * operation is finished. This sample application runs on a ZDP03A board and therefore has no
 * door lock hardware, but to show how Supervision can be used, a timer is implemented. This timer
 * represents the physical bolt of a door lock.
 * The first Supervision report will be transmitted automatically by the Framework, but transmission
 * of the next report(s) need(s) to be handled by the application.
 */
PCB(ZCB_CommandClassSupervisionGetReceived)(SUPERVISION_GET_RECEIVED_HANDLER_ARGS * pArgs)
{
  if (DOOR_LOCK_OPERATION_SET_V2 == pArgs->cmd && COMMAND_CLASS_DOOR_LOCK_V2 == pArgs->cmdClass)
  {
    /* Status for DOOR_LOCK_OPERATION_SET_V2 */
    pArgs->status = CC_SUPERVISION_STATUS_WORKING;
    pArgs->duration = 2;

    // Save the data
    rxOptionSupervision = *(pArgs->rxOpt);
    sessionID = CC_SUPERVISION_EXTRACT_SESSION_ID(pArgs->properties1);

    if(CC_SUPERVISION_STATUS_UPDATES_SUPPORTED == CC_SUPERVISION_EXTRACT_STATUS_UPDATE(pArgs->properties1))
    {
      pArgs->properties1 = CC_SUPERVISION_ADD_MORE_STATUS_UPDATE(CC_SUPERVISION_MORE_STATUS_UPDATES_REPORTS_TO_FOLLOW) | CC_SUPERVISION_ADD_SESSION_ID(sessionID);
      // Start timer which will send another Supervision report when triggered.
      TimerStart(ZCB_SupervisionTimerCallback, 2000/10, 1);
    }
  }
  else
  {
    /* Status for all other commands */
    pArgs->properties1 |= CC_SUPERVISION_ADD_MORE_STATUS_UPDATE(CC_SUPERVISION_MORE_STATUS_UPDATES_THIS_IS_LAST);
    pArgs->duration = 0;
  }
}

/**
 * @brief Transmits a Supervision report.
 * @details This function is triggered by the timer set in the Supervision Get handler.
 */
PCB(ZCB_SupervisionTimerCallback)(void)
{
  TRANSMIT_OPTIONS_TYPE_SINGLE_EX * pTxOptions;
  RxToTxOptions(&rxOptionSupervision, &pTxOptions);
  CmdClassSupervisionReportSend(
      pTxOptions,
      CC_SUPERVISION_ADD_MORE_STATUS_UPDATE(CC_SUPERVISION_MORE_STATUS_UPDATES_THIS_IS_LAST) | CC_SUPERVISION_ADD_SESSION_ID(sessionID),
      CC_SUPERVISION_STATUS_SUCCESS,
      0);
}


/*
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
#ifdef TEST_INTERFACE_SUPPORT
        ti_csa_prompt();
#else /* TEST_INTERFACE_SUPPORT */
        ZW_SetSecurityS2InclusionPublicDSK_CSA(&sCSAResponse);
#endif /* TEST_INTERFACE_SUPPORT */
      }
      break;
#endif /* APP_SUPPORTS_CLIENT_SIDE_AUTHENTICATION */

    default:
      break;
  }
}

#ifdef TEST_INTERFACE_SUPPORT

void
StartKeypadApp(void)
{
  //ZW_DEBUG_DOORLOCK_SEND_STR("\r\nStartKeypadApp");
  ZW_test_interface_allocate('c', ZCB_AppKeypad);
  ZW_UART0_tx_send_str("\r\nEnter code 'c xx..x'\r\n");
  ZW_UART0_tx_send_byte('>');
}


PCB(ZCB_AppKeypad)(BYTE channel, BYTE * pString)
{
  BOOL valid = FALSE;
  BYTE stringLength = ZW_strlen(pString);
  UNUSED(channel);
  //ZW_DEBUG_DOORLOCK_SEND_STR("\r\nZCB_AppKeypad");
  //ZW_DEBUG_DOORLOCK_SEND_NUM(stringLength);

  ZCB_eventSchedulerEventAdd(EVENT_APP_FINISH_KEYPAD_ACTIVE);

  //Validate string length [4,10]
  if((USERCODE_MIN_LEN <= stringLength) && (USERCODE_MAX_LEN >= stringLength))
  {
    BYTE i;

    for(i = 1; i <= USER_ID_MAX; i++ )
    {
      if(TRUE == ValidateUserCode(i, pString, stringLength))
      {
        /*Correct key!*/
        //ZW_DEBUG_DOORLOCK_SEND_STR("key valid ");
        TOGGLE_BOLT();

        /* Save new status in EEPROM */
        SaveStatus();
        valid = TRUE;
        ZCB_eventSchedulerEventAdd(EVENT_APP_DOORLOCK_JOB);
        break;
      }
    }
  }

  if(FALSE == valid)
  {
    ZW_UART0_tx_send_str("\r\nInvalid key!\r\n");
  }
  else{
    ZW_UART0_tx_send_str("\r\nValid key!\r\n");
  }
  Led(ZDP03A_LED_D3, OFF);
  //ZW_DEBUG_DOORLOCK_SEND_NL();
}

#endif

/*
 * Resets the application settings.
 *
 * Must be invoked if
 * - the node is excluded and
 * - the magic value in NVM is changed (Device Reset Locally).
 */
void DefaultApplicationsSettings(void)
{
  BYTE i,j;
  BYTE defaultUserCode[] = DEFAULT_USERCODE;

  //ZW_DEBUG_DOORLOCK_SEND_STR("\r\n DefaultApplicationsSettings");

  ResetMyDoorLock();

  for (i = 0; i < USER_ID_MAX; i++)
  {
    MemoryPutByte((WORD)&EEOFFSET_USERSTATUS_far[i], USER_ID_OCCUPIED);
    MemoryPutByte((WORD)&EEOFFSET_USERCODE_LEN_far[i], sizeof(defaultUserCode));

    for (j = 0; j < USERCODE_MAX_LEN; j++)
    {
      MemoryPutByte((WORD)&EEOFFSET_USERCODE_far[i][j], defaultUserCode[j]);
    }
  }
  SaveStatus();
}

/**
* Set up security keys to request when joining a network.
* These are taken from the config_app.h header file.
*/
BYTE ApplicationSecureKeysRequested(void)
{
  return REQUESTED_SECURITY_KEYS;
}

/**
* Set up security S2 inclusion authentication to request when joining a network.
* These are taken from the config_app.h header file.
*/
BYTE ApplicationSecureAuthenticationRequested(void)
{
  return REQUESTED_SECURITY_AUTHENTICATION;
}

