/**
 * @file SensorPIR.c
 * @brief Z-Wave SensorPIR Sample Application
 * @copyright Copyright (c) 2001-2017
 * Sigma Designs, Inc.
 * All Rights Reserved
 * @details
 * Last changed by: $Author: $
 * Revision:        $Revision: $
 * Last changed:    $Date: $
 *
 * @author Thomas Roll
 * @author Christian Salmony Olsen
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
#include <nvm_util.h>

/*IO control*/
#include <io_zdp03a.h>

#include <ZW_task.h>
#include <ev_man.h>
#include <ZW_timer_api.h>

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
#include <CommandClassBasic.h>

#include <CommandClassBattery.h>
#include <CommandClassNotification.h>

#include <CommandClassMultiChan.h>
#include <CommandClassMultiChanAssociation.h>
#include <CommandClassSupervision.h>
#include <notification.h>

#include <zaf_version.h>

#include <ZAF_pm.h>
#include <battery_monitor.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

/**
 * @def ZW_DEBUG_SENSORPIR_SEND_BYTE(data)
 * Transmits a given byte to the debug port.
 * @def ZW_DEBUG_SENSORPIR_SEND_STR(STR)
 * Transmits a given string to the debug port.
 * @def ZW_DEBUG_SENSORPIR_SEND_NUM(data)
 * Transmits a given number to the debug port.
 * @def ZW_DEBUG_SENSORPIR_SEND_WORD_NUM(data)
 * Transmits a given WORD number to the debug port.
 * @def ZW_DEBUG_SENSORPIR_SEND_NL()
 * Transmits a newline to the debug port.
 */
#ifdef ZW_DEBUG_APP
#define ZW_DEBUG_SENSORPIR_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_SENSORPIR_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_SENSORPIR_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_SENSORPIR_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_SENSORPIR_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_SENSORPIR_SEND_BYTE(data)
#define ZW_DEBUG_SENSORPIR_SEND_STR(STR)
#define ZW_DEBUG_SENSORPIR_SEND_NUM(data)
#define ZW_DEBUG_SENSORPIR_SEND_WORD_NUM(data)
#define ZW_DEBUG_SENSORPIR_SEND_NL()
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
  EVENT_APP_BATTERY_LOW_TX,
  EVENT_APP_IS_POWERING_DOWN,
  EVENT_APP_BASIC_STOP_JOB,
  EVENT_APP_BASIC_START_JOB,
  EVENT_APP_NOTIFICATION_START_JOB,
  EVENT_APP_NOTIFICATION_STOP_JOB,
  EVENT_APP_START_TIMER_EVENTJOB_STOP,
  EVENT_APP_WAKE_UP_NOTIFICATION_TX,
  EVENT_APP_SMARTSTART_IN_PROGRESS
}
EVENT_APP;

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
  STATE_APP_POWERDOWN,
  STATE_APP_BASIC_SET_TIMEOUT_WAIT
}
STATE_APP;

/**
 * The following values determines how long the sensor sleeps
 * i.e. it sets the delay before next wakeup. This value
 * is stored in 24-bits and is converted during execution
 * depending on the chip. Default value is 10 minutes
 * 0x258 = 600 seconds
 */
#define DEFAULT_SLEEP_TIME 300
#define MIN_SLEEP_TIME     60
#define MAX_SLEEP_TIME     86400 // 24 hours
#define STEP_SLEEP_TIME    60

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
  COMMAND_CLASS_ASSOCIATION,
  COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
  COMMAND_CLASS_ASSOCIATION_GRP_INFO,
  COMMAND_CLASS_TRANSPORT_SERVICE_V2,
  COMMAND_CLASS_VERSION,
  COMMAND_CLASS_MANUFACTURER_SPECIFIC,
  COMMAND_CLASS_DEVICE_RESET_LOCALLY,
  COMMAND_CLASS_POWERLEVEL,
  COMMAND_CLASS_BATTERY,
  COMMAND_CLASS_SECURITY_2,
  COMMAND_CLASS_NOTIFICATION_V3,
  COMMAND_CLASS_WAKE_UP,
  COMMAND_CLASS_SUPERVISION
#ifdef BOOTLOADER_ENABLED
  ,COMMAND_CLASS_FIRMWARE_UPDATE_MD_V2
#endif
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
  COMMAND_CLASS_SECURITY_2
};


/**
 * Secure node information list.
 * Be sure Command classes are not duplicated in both lists.
 * CHANGE THIS - Add all supported secure command classes here
 **/
static code BYTE cmdClassListSecure[] =
{
  COMMAND_CLASS_VERSION,
  COMMAND_CLASS_ASSOCIATION,
  COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
  COMMAND_CLASS_ASSOCIATION_GRP_INFO,
  COMMAND_CLASS_MANUFACTURER_SPECIFIC,
  COMMAND_CLASS_DEVICE_RESET_LOCALLY,
  COMMAND_CLASS_POWERLEVEL,
  COMMAND_CLASS_BATTERY,
  COMMAND_CLASS_NOTIFICATION_V3,
  COMMAND_CLASS_WAKE_UP
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
 * AGI lifeline string
 */
const char GroupName[]   = "Lifeline";

/**
 * Setup AGI lifeline table from app_config.h
 */
CMD_CLASS_GRP  agiTableLifeLine[] = {AGITABLE_LIFELINE_GROUP};

/**
 * Setup AGI root device groups table from app_config.h
 */
AGI_GROUP agiTableRootDeviceGroups[] = {AGITABLE_ROOTDEVICE_GROUPS};

static const AGI_PROFILE lifelineProfile = {
    ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL,
    ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL_LIFELINE
};

/**
 * Application node ID
 */
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
 * flag for wakeup notification is send. Used on power down.
 */
static BOOL wakeupNotificationSend = FALSE;

/**
 * A timer handle use in relation with handling sending basic set command
 * when an event occur. When the timer time out a basic set command
 * with the negated value is sent
 */
static bTimerHandle_t eventJobsTimerHandle = 0xFF;

static BYTE supportedEvents = NOTIFICATION_EVENT_HOME_SECURITY_MOTION_DETECTION_UNKNOWN_LOCATION;

static BYTE basicValue = 0x00;

#ifdef APP_SUPPORTS_CLIENT_SIDE_AUTHENTICATION
s_SecurityS2InclusionCSAPublicDSK_t sCSAResponse = { 0, 0, 0, 0};
#endif /* APP_SUPPORTS_CLIENT_SIDE_AUTHENTICATION */

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/
void ZCB_BattReportSentDone(TRANSMISSION_RESULT * pTransmissionResult);
void ZCB_DeviceResetLocallyDone(TRANSMISSION_RESULT * pTransmissionResult);
void ZCB_ResetDelay(void);
STATE_APP GetAppState();
void AppStateManager( EVENT_APP event);
void ChangeState( STATE_APP newState);
void ZCB_JobStatus(TRANSMISSION_RESULT * pTransmissionResult);
void SetDefaultConfiguration(void);
void LoadConfiguration(void);
BOOL ApplicationIdle(void);
#ifdef BOOTLOADER_ENABLED
void ZCB_OTAFinish(OTA_STATUS otaStatus);
BOOL ZCB_OTAStart();
#endif

void ZCB_EventJobsTimer(void);

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
  SetPinIn(ZDP03A_KEY_1,TRUE);
  SetPinIn(ZDP03A_KEY_2,TRUE);
  SetPinIn(ZDP03A_KEY_4,TRUE);
  SetPinOut(ZDP03A_LED_D1); /**< Learn mode indication*/
  Led(ZDP03A_LED_D1,OFF);

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
  UNUSED(nvmStatus);
  /* Init state machine*/
  currentState = STATE_APP_STARTUP;
  wakeupNotificationSend = FALSE;

  /* Do not reinitialize the UART if already initialized for ISD51 in ApplicationInitHW() */
#ifndef ZW_ISD51_DEBUG
  ZW_DEBUG_INIT(1152);
#endif
  ZW_DEBUG_SEND_STR("\r\nAppInitSW ");
  ZW_DEBUG_SEND_NUM((BYTE)wakeupReason);

#ifdef WATCHDOG_ENABLED
  ZW_WatchDogEnable();
#endif
  ZAF_eventSchedulerInit(AppStateManager);

  /*Check battery level*/
  /*Check if battery Monitor has no state change and last time was battery state ST_BATT_DEAD*/
  if((FALSE == BatterySensorRead(NULL)) && (ST_BATT_DEAD ==BatteryMonitorState()))
  {
    ZW_DEBUG_SENSORPIR_SEND_NL();
    ZW_DEBUG_SENSORPIR_SEND_STR("DEAD BATT!");
    ZW_DEBUG_SENSORPIR_SEND_NL();
    ZW_DEBUG_SENSORPIR_SEND_NL();
    AppStateManager(EVENT_APP_IS_POWERING_DOWN);
    return application_node_type;
  }

  /* Get this sensors identification on the network */
  LoadConfiguration();
  ZAF_pm_Init(wakeupReason);

  /* Setup AGI group lists*/
  AGI_Init();
  AGI_LifeLineGroupSetup(agiTableLifeLine, (sizeof(agiTableLifeLine)/sizeof(CMD_CLASS_GRP)), GroupName, ENDPOINT_ROOT );
  AGI_ResourceGroupSetup(agiTableRootDeviceGroups, (sizeof(agiTableRootDeviceGroups)/sizeof(AGI_GROUP)), ENDPOINT_ROOT);

  InitNotification();
  AddNotification(
      &lifelineProfile,
      NOTIFICATION_TYPE_HOME_SECURITY,
      &supportedEvents,
      sizeof(supportedEvents),
      FALSE,
      0);

#ifdef BOOTLOADER_ENABLED
  /* Initialize OTA module */
  OtaInit(ZCB_OTAStart, NULL,ZCB_OTAFinish);
#endif

  CC_Version_SetApplicationVersionInfo(APP_VERSION, APP_REVISION, APP_VERSION_PATCH, APP_BUILD_NUMBER);

  /*
   * Initialize Event Scheduler.
   */
  Transport_OnApplicationInitSW( &m_AppNIF);
  ZCB_eventSchedulerEventAdd(EVENT_APP_INIT);

  //ZAF_pm_KeepAwake(ZAF_PM_LEARNMODE_TIMEOUT);

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
  BOOL poweroff = ApplicationIdle();

#ifdef WATCHDOG_ENABLED
  ZW_WatchDogKick();
#endif

// TODO: enable line below and remove debug
//  return (TRUE == ApplicationIdle()) ? E_APPLICATION_STATE_READY_FOR_POWERDOWN : E_APPLICATION_STATE_ACTIVE;

  if((TRUE == poweroff) && (E_PROTOCOL_STATE_READY_FOR_SHUTDOWN == bProtocolState))
  {
    ZW_DEBUG_SEND_STR("\r\n PowerOFF AppPoll\r\n");
  }
  return (TRUE == poweroff) ? E_APPLICATION_STATE_READY_FOR_POWERDOWN : E_APPLICATION_STATE_ACTIVE;
}


/**
 * @brief Returns whether the application is ready to power down.
 * @return TRUE if ready to power down, FALSE otherwise.
 */
static BOOL ApplicationIdle(void)
{
  BOOL status = FALSE;

  if (STATE_APP_POWERDOWN == GetAppState()) //low battery
  {
    status = TRUE;
  }
  else
  {
    BOOL task_poll_status = TaskApplicationPoll();
    STATE_APP appState = GetAppState();
    BOOL activeTxJobs = ZAF_mutex_isActive();
    BOOL plStatus = CommandClassPowerLevelIsInProgress();
    BOOL keepAwakeTimerActive = ZAF_pm_IsActive();

    if ((FALSE == task_poll_status) && // Check Task Handler for active task.
        (STATE_APP_IDLE == appState) && //Check application is in idle state.
        (FALSE == activeTxJobs) && //Check ZW_tx_mutex for active jobs, then buffers occupied.
        (FALSE == plStatus)) //Check CC power level for active test job.
    {
      BOOL changeState = FALSE;

      //ZW_DEBUG_SENSORPIR_SEND_STR("\r\nApplicationIdle");
      /*
       * Check event queue for queued up jobs
       */
      if(0 != ZAF_jobQueueCount())
      {
        ZW_DEBUG_SENSORPIR_SEND_BYTE('q');
        // changeState to TRUE for ZAF_jobDequeue in state machine
        changeState = TRUE;
      }
      /*
       * It is important that the Wake Up Notification is the last thing to do before going to sleep.
       * The Wake Up Notification will notify the controller that we are awake and it'll start sending
       * commands if it has any. Hence, we need to be done with everything else before these commands
       * arrive.
       */
      if ((SW_WAKEUP_EXT_INT != wakeupReason) && (TRUE == TimeToSendBattReport()))
      {
        ZW_DEBUG_SENSORPIR_SEND_BYTE('B');
        /*Add event's on job-queue*/
        ZAF_jobEnqueue(EVENT_APP_BATTERY_LOW_TX);
        changeState = TRUE;
      }
      else if (((SW_WAKEUP_RESET == wakeupReason) ||
             (SW_WAKEUP_WUT == wakeupReason)) &&
             (FALSE == wakeupNotificationSend))
      {
        ZW_DEBUG_SENSORPIR_SEND_BYTE('W');
        wakeupNotificationSend = TRUE;
        ZAF_jobEnqueue(EVENT_APP_WAKE_UP_NOTIFICATION_TX);
        changeState = TRUE;
      }

      if (TRUE == changeState)
      {
        ZW_DEBUG_SENSORPIR_SEND_BYTE('T');
        ZCB_eventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
        ChangeState(STATE_APP_TRANSMIT_DATA);
      }
      else
      {
        /*
         * Do not power down if eventJobsTimerHandle is active!
         * Do not power down if keepAwakeTimerAcitve is active!
         * Do power down if wakeupReason equals SMART START and we are not included
         */
        if ((0xFF == eventJobsTimerHandle) &&
            ((FALSE == keepAwakeTimerActive) ||
             ((0 == myNodeID) && (SW_WAKEUP_SMARTSTART == wakeupReason))))
        {
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
  ZW_DEBUG_SENSORPIR_SEND_NL();
  ZW_DEBUG_SENSORPIR_SEND_STR("TAppH");
  ZW_DEBUG_SENSORPIR_SEND_NUM(pCmd->ZW_Common.cmdClass);


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

   case COMMAND_CLASS_BATTERY:
      frame_status = handleCommandClassBattery(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_NOTIFICATION_V3:
      frame_status = handleCommandClassNotification(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_WAKE_UP:
      frame_status = HandleCommandClassWakeUp(rxOpt, pCmd, cmdLength);
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
handleCommandClassVersionAppl(BYTE cmdClass)
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

    case COMMAND_CLASS_BATTERY:
      commandClassVersion = CommandClassBatteryVersionGet();
      break;

    case COMMAND_CLASS_NOTIFICATION_V3:
      commandClassVersion = CommandClassNotificationVersionGet();
      break;

    case COMMAND_CLASS_WAKE_UP:
      commandClassVersion = CmdClassWakeupVersion();
      break;

    case COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2:
      commandClassVersion = CmdClassMultiChannelAssociationVersion();
      break;

    case COMMAND_CLASS_SUPERVISION:
      commandClassVersion = CommandClassSupervisionVersionGet();
      break;

      /*
       * If both S0 & S2 are supported, ZW_Transport_CommandClassVersionGet can be used in default
       * instead of handling each of the CCs. Please see the other Z-Wave Plus apps for an example.
       *
       * In this case, S0 is not supported. Hence we must not return the version of S0 CC. Using
       * ZW_Transport_CommandClassVersionGet will return the version.
       */

    case COMMAND_CLASS_SECURITY_2:
      commandClassVersion = SECURITY_2_VERSION;
      break;

    case COMMAND_CLASS_TRANSPORT_SERVICE_V2:
      commandClassVersion = TRANSPORT_SERVICE_VERSION_V2;
      break;

    default:
      commandClassVersion = UNKNOWN_VERSION;
      break;
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
  ZW_DEBUG_SENSORPIR_SEND_NL();
  ZW_DEBUG_SENSORPIR_SEND_STR("UPDATE!");
}

/**
 * @brief See description for function prototype in ZW_slave_api.h.
 */
void ApplicationNetworkLearnModeCompleted(uint8_t bNodeID)
{
  ZW_DEBUG_SENSORPIR_SEND_STR("\r\nLearnComp id=");
  ZW_DEBUG_SENSORPIR_SEND_NUM(bNodeID);
  ZW_DEBUG_SENSORPIR_SEND_NL();

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
        /*Success*/
        myNodeID = bNodeID;
        if (0 == myNodeID)
        {
          MemoryPutByte((WORD)&EEOFFSET_MAGIC_far, APPL_MAGIC_VALUE + 1);
          ZCB_eventSchedulerEventAdd((EVENT_APP) EVENT_SYSTEM_WATCHDOG_RESET);
        }
        else
        {
          /* We are included! Inform controller device battery status by clearing flag*/
          ActivateBattNotificationTrigger();
          ZAF_jobEnqueue(EVENT_APP_WAKE_UP_NOTIFICATION_TX);
          ZW_DEBUG_SENSORPIR_SEND_NL();
          ZW_DEBUG_SENSORPIR_SEND_STR("INCLUDED!");
          ZAF_pm_ActivateCCWakeUp();
        }
      }
    }
    ZCB_eventSchedulerEventAdd((EVENT_APP) EVENT_SYSTEM_LEARNMODE_FINISH);
    Transport_OnLearnCompleted(bNodeID);
  }
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
AppStateManager(EVENT_APP event)
{
  ZW_DEBUG_SENSORPIR_SEND_NL();
  ZW_DEBUG_SENSORPIR_SEND_STR("AppStMan ev ");
  ZW_DEBUG_SENSORPIR_SEND_NUM(event);
  ZW_DEBUG_SENSORPIR_SEND_STR(" st ");
  ZW_DEBUG_SENSORPIR_SEND_NUM(currentState);

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
        ZW_DEBUG_SENSORPIR_SEND_STR("LEARN_MODE_INCLUSION_SMARTSTART");
        ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_INCLUSION_SMARTSTART);
      }

      ChangeState(STATE_APP_IDLE);
      break;

    case STATE_APP_IDLE:
      if(EVENT_APP_REFRESH_MMI == event)
      {
        Led(ZDP03A_LED_D1,OFF);
      }

      if(EVENT_APP_SMARTSTART_IN_PROGRESS == event)
      {
        ChangeState(STATE_APP_LEARN_MODE);
      }

      if((EVENT_KEY_B1_DOWN == event) ||(EVENT_SYSTEM_LEARNMODE_START == event))
      {
        if (myNodeID)
        {
          ZW_DEBUG_SENSORPIR_SEND_STR("LEARN_MODE_EXCLUSION");
          ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_EXCLUSION_NWE);
        }
        else
        {
          ZW_DEBUG_SENSORPIR_SEND_STR("LEARN_MODE_INCLUSION");
          ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_INCLUSION);
        }
        ChangeState(STATE_APP_LEARN_MODE);
      }

      if ((EVENT_KEY_B4_DOWN == event) || (EVENT_SYSTEM_RESET == event))
      {
        /*
         * Since this application is a routing slave, it'll use the internal NVM also known as the
         * MTP. The MTP is getting flushed 300 ms after the latest write which means we'll have to
         * wait some time before resetting the device.
         */
        MemoryPutByte((WORD)&EEOFFSET_MAGIC_far, 1 + APPL_MAGIC_VALUE);
        ZW_TimerLongStart(ZCB_ResetDelay, 300, TIMER_ONE_TIME);
        ZAF_pm_KeepAwake(ZAF_PM_HALF_SECOND);
      }

      if(EVENT_KEY_B2_DOWN == event)
      {
        ZW_DEBUG_SENSORPIR_SEND_STR("\r\nEVENT_KEY_B2_DOWN\r\n");
        /*Add event's on job-queue*/
        ZAF_jobEnqueue(EVENT_APP_BASIC_START_JOB);
        ZAF_jobEnqueue(EVENT_APP_NOTIFICATION_START_JOB);
        ZAF_jobEnqueue(EVENT_APP_START_TIMER_EVENTJOB_STOP);
      }

      if(EVENT_SYSTEM_OTA_START == event)
      {
        ChangeState(STATE_APP_OTA);
      }
      break;

    case STATE_APP_LEARN_MODE:
      if(EVENT_APP_REFRESH_MMI == event)
      {
        Led(ZDP03A_LED_D1,ON);
      }

      if((EVENT_KEY_B1_DOWN == event)||(EVENT_SYSTEM_LEARNMODE_END == event))
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

      ZW_DEBUG_SENSORPIR_SEND_STR("STATE_APP_WATCHDOG_RESET");
      ZW_DEBUG_SENSORPIR_SEND_NL();
      ZW_MemoryFlush();
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

      if(EVENT_KEY_B2_DOWN == event)
      {
        if (0xFF != eventJobsTimerHandle)
        {
          ZW_TimerLongRestart(eventJobsTimerHandle);
        }
      }

      if(EVENT_APP_NEXT_EVENT_JOB == event)
      {
        BYTE event;
        /*check job-queue*/
        ZW_DEBUG_SENSORPIR_SEND_NL();
        ZW_DEBUG_SENSORPIR_SEND_BYTE('Q');

        if(TRUE == ZAF_jobDequeue(&event))
        {
          /*Let the event scheduler fire the event on state event machine*/
          ZCB_eventSchedulerEventAdd(event);
        }
        else
        {
          ZCB_eventSchedulerEventAdd(EVENT_APP_FINISH_EVENT_JOB);
        }
      }

      if(EVENT_APP_BASIC_START_JOB == event)
      {
        if(JOB_STATUS_SUCCESS != CmdClassBasicSetSend( &agiTableRootDeviceGroups[0].profile, ENDPOINT_ROOT, BASIC_SET_TRIGGER_VALUE, ZCB_JobStatus))
        {
          basicValue = BASIC_SET_TRIGGER_VALUE;
          /*Kick next job*/
          ZCB_eventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
        }
      }

      if(EVENT_APP_BASIC_STOP_JOB == event)
      {
        if(JOB_STATUS_SUCCESS != CmdClassBasicSetSend( &agiTableRootDeviceGroups[0].profile, ENDPOINT_ROOT, 0, ZCB_JobStatus))
        {
          basicValue = 0;
          /*Kick next job*/
          ZCB_eventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
        }
      }


      if(EVENT_APP_NOTIFICATION_START_JOB == event)
      {
        ZW_DEBUG_SENSORPIR_SEND_STR("\r\nEVENT_APP_NOTIFICATION_START_JOB");
        NotificationEventTrigger(&lifelineProfile,
            NOTIFICATION_TYPE_HOME_SECURITY,
            supportedEvents,
            NULL, 0,
            ENDPOINT_ROOT);
        if(JOB_STATUS_SUCCESS !=  UnsolicitedNotificationAction(&lifelineProfile, ENDPOINT_ROOT, ZCB_JobStatus))
        {
          /*Kick next job*/
          ZCB_eventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
        }
      }

      if(EVENT_APP_NOTIFICATION_STOP_JOB == event)
      {
        ZW_DEBUG_SENSORPIR_SEND_STR("\r\nEVENT_APP_NOTIFICATION_STOP_JOB");
        NotificationEventTrigger(&lifelineProfile,
            NOTIFICATION_TYPE_HOME_SECURITY,
            0,
            &supportedEvents,
            1,
            ENDPOINT_ROOT);
        if(JOB_STATUS_SUCCESS !=  UnsolicitedNotificationAction(&lifelineProfile, ENDPOINT_ROOT, ZCB_JobStatus))
        {
          /*Kick next job*/
          ZCB_eventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
        }
        ClearLastNotificationAction(&lifelineProfile, ENDPOINT_ROOT);
      }

      if( EVENT_APP_START_TIMER_EVENTJOB_STOP== event)
      {
         eventJobsTimerHandle = ZW_TimerLongStart(ZCB_EventJobsTimer, BASIC_SET_TIMEOUT, 1);
      }

      if (EVENT_APP_BATTERY_LOW_TX == event)
      {
        if (JOB_STATUS_SUCCESS != SendBattReport(ZCB_BattReportSentDone))
        {
          ActivateBattNotificationTrigger();
          ZCB_eventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
        }
      }

      if (EVENT_APP_WAKE_UP_NOTIFICATION_TX == event)
      {
        ZW_DEBUG_SENSORPIR_SEND_STR("NOTIFIC***");
        CC_WakeUp_notification_tx(ZCB_JobStatus);
      }

      if(EVENT_APP_FINISH_EVENT_JOB == event)
      {
        ZW_DEBUG_SENSORPIR_SEND_NL();
        ZW_DEBUG_SENSORPIR_SEND_STR("#EVENT_APP_FINISH_EVENT_JOB");
        ZW_DEBUG_SENSORPIR_SEND_NL();
        ChangeState(STATE_APP_IDLE);
      }
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
  ZW_DEBUG_SENSORPIR_SEND_NL();
  ZW_DEBUG_SENSORPIR_SEND_STR("State changed: ");
  ZW_DEBUG_SENSORPIR_SEND_NUM(currentState);
  ZW_DEBUG_SENSORPIR_SEND_STR(" -> ");
  ZW_DEBUG_SENSORPIR_SEND_NUM(newState);

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
  if (TRANSMISSION_RESULT_FINISHED == pTransmissionResult->isFinished)
  {
    ZW_DEBUG_SENSORPIR_SEND_NL();
    ZW_DEBUG_SENSORPIR_SEND_STR("DRLD");

    ZCB_eventSchedulerEventAdd((EVENT_APP) EVENT_SYSTEM_WATCHDOG_RESET);
  }
}

/**
 * @brief Reset delay callback.
 */
PCB(ZCB_ResetDelay)(void)
{
  AGI_PROFILE lifelineProfile = {
      ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL,
      ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL_LIFELINE
  };
  ZW_DEBUG_SENSORPIR_SEND_NL();
  ZW_DEBUG_SENSORPIR_SEND_STR("Call locally reset");

  handleCommandClassDeviceResetLocally(&lifelineProfile, ZCB_DeviceResetLocallyDone);

  ZW_DEBUG_SENSORPIR_SEND_NL();
  ZW_DEBUG_SENSORPIR_SEND_STR("Delay");
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

  if (STATE_APP_IDLE == GetAppState() ||
      STATE_APP_TRANSMIT_DATA == GetAppState())
  {
    ZCB_eventSchedulerEventAdd((EVENT_APP) EVENT_SYSTEM_OTA_START);
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
 * @brief Callback function used for unsolicited commands.
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
 * @brief Function resets configuration to default values.
 */
void
SetDefaultConfiguration(void)
{
  AssociationInit(TRUE);
  MemoryPutByte((WORD)&EEOFFSET_alarmStatus_far, 0xFF);
  SetDefaultBatteryConfiguration(DEFAULT_SLEEP_TIME);
  MemoryPutByte((WORD)&EEOFFSET_MAGIC_far, APPL_MAGIC_VALUE);
  CmdClassWakeUpNotificationMemorySetDefault();
  ActivateBattNotificationTrigger();
}

/**
 * @brief This function loads the application settings from EEPROM.
 * If no settings are found, default values are used and saved.
 */
void
LoadConfiguration(void)
{
  /*load the application configuration*/
  MemoryGetID(NULL, &myNodeID);
  ManufacturerSpecificDeviceIDInit();
  SetWakeUpConfiguration(WAKEUP_PAR_DEFAULT_SLEEP_TIME, DEFAULT_SLEEP_TIME);
  SetWakeUpConfiguration(WAKEUP_PAR_MAX_SLEEP_TIME,     MAX_SLEEP_TIME);
  SetWakeUpConfiguration(WAKEUP_PAR_MIN_SLEEP_TIME,     MIN_SLEEP_TIME);
  SetWakeUpConfiguration(WAKEUP_PAR_SLEEP_STEP,         STEP_SLEEP_TIME);

  /* Check to see, if any valid configuration is stored in the EEPROM */
  if (MemoryGetByte((WORD)&EEOFFSET_MAGIC_far) == APPL_MAGIC_VALUE)
  {
    /* Initialize association module */
    AssociationInit(FALSE);

    loadStatusPowerLevel();
  }
  else
  {
    ZW_DEBUG_SENSORPIR_SEND_NL();
    ZW_DEBUG_SENSORPIR_SEND_STR("reset");
    /* Initialize transport layer NVM */
    Transport_SetDefault();
    /* Reset protocol */
    ZW_SetDefault();
    loadInitStatusPowerLevel();
    /* Apparently there is no valid configuration in EEPROM, so load */
    /* default values and save them to EEPROM. */
    SetDefaultConfiguration();
  }

}


/**
 * @brief Handler for basic set.
 *
 * Handles received basic set commands.
 *
 * @param[in] val Parameter dependent of the application device class.
 */
void
handleBasicSetCommand(BYTE val, BYTE endpoint)
{
  UNUSED(val);
  UNUSED(endpoint);
  /* CHANGE THIS - Fill in your application code here */
}

/**
 * @brief Handler for basic get. Handles received basic get commands.
 */
BYTE
getAppBasicReport(BYTE endpoint)
{
  /* CHANGE THIS - Fill in your application code here */
  UNUSED(endpoint);
  return basicValue;
}

/**
 * @brief Report the target value
 * @return target value.
 */
BYTE
getAppBasicReportTarget(BYTE endpoint)
{
  UNUSED(endpoint);
  /* CHANGE THIS - Fill in your application code here */
  return 0;
}

/**
 * @brief Report transition duration time.
 * @return duration time.
 */
BYTE
getAppBasicReportDuration(BYTE endpoint)
{
  UNUSED(endpoint);
  /* CHANGE THIS - Fill in your application code here */
  return 0;
}

/**
 * @brief Callback function used when sending battery report.
 */
PCB(ZCB_BattReportSentDone)(TRANSMISSION_RESULT * pTransmissionResult)
{
  ZW_DEBUG_SENSORPIR_SEND_NL();
  ZW_DEBUG_SENSORPIR_SEND_STR("BR CB: ");
  ZW_DEBUG_SENSORPIR_SEND_NUM(pTransmissionResult->nodeId);
  ZW_DEBUG_SENSORPIR_SEND_NUM(pTransmissionResult->status);
  ZW_DEBUG_SENSORPIR_SEND_NUM(pTransmissionResult->isFinished);
  if (TRANSMIT_COMPLETE_OK != pTransmissionResult->status)
  {
    // If one of the nodes does not receive the battery status, we activate the trigger once again.
    ActivateBattNotificationTrigger();
  }

  if (TRANSMISSION_RESULT_FINISHED == pTransmissionResult->isFinished)
  {
    ZCB_eventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
  }
}

/**
 * @brief event jobs timeout event
 */
PCB(ZCB_EventJobsTimer)(void)
{
  eventJobsTimerHandle = 0xFF;
  ZAF_jobEnqueue(EVENT_APP_NOTIFICATION_STOP_JOB);
  ZAF_jobEnqueue(EVENT_APP_BASIC_STOP_JOB);
  /*Kick next job*/
  ZCB_eventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
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
        ZW_SetSecurityS2InclusionPublicDSK_CSA(&sCSAResponse);
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

