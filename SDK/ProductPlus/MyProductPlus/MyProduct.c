/**
 *
 * Copyright (c) 2001-2014
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 * @file Myproduct.c
 *
 * @brief Empty template application for mains-powered Z-Wave slave devices.
 * Please search for "CHANGE THIS"
 *
 * @author: Thomas Roll
 *
 * Last Changed By: $Author: tro $
 * Revision: $Revision: 31825 $
 * Last Changed: $Date: 2015-09-03 12:21:56 +0200 (to, 03 sep 2015) $
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
#include <CommandClassSupervision.h>
#include <CommandClassMultiChan.h>
#include <CommandClassMultiChanAssociation.h>

#if defined(BATTERY) || defined(BATTERY__CC_WAKEUP)
#include <ZAF_pm.h>
#include <battery_monitor.h>
#include <CommandClassBattery.h>
#endif // BATTERY || BATTERY__CC_WAKEUP
#if defined(BATTERY__CC_WAKEUP)
#include <CommandClassWakeUp.h>
#endif //BATTERY__CC_WAKEUP

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
#ifdef ZW_DEBUG_MYPRODUCT
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
  EVENT_APP_BATT_LOW_TX,
  EVENT_APP_WAKE_UP_NOTIFICATION_TX,
  EVENT_APP_IS_POWERING_DOWN,
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
  STATE_APP_POWERDOWN
} STATE_APP;


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
 * CHANGE THIS - Add all supported non-secure command classes here.
 * Remember to add COMMAND_CLASS_MULTI_CHANNEL_V3 if device has end-points!
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
  COMMAND_CLASS_SECURITY,
  COMMAND_CLASS_SECURITY_2,
  COMMAND_CLASS_SUPERVISION
#if defined(BATTERY) || defined(BATTERY__CC_WAKEUP)
  ,COMMAND_CLASS_BATTERY
#endif
#if defined(BATTERY__CC_WAKEUP)
  ,COMMAND_CLASS_WAKE_UP
#endif
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
  COMMAND_CLASS_SECURITY,
  COMMAND_CLASS_SECURITY_2
};

/**
 * Secure node inforamtion list.
 * Be sure Command classes are not duplicated in both lists.
 * CHANGE THIS - Add all supported secure command classes here
 * Remember to add COMMAND_CLASS_MULTI_CHANNEL_V3 if device has end-points!
 **/
static BYTE cmdClassListSecure[] =
{
  COMMAND_CLASS_VERSION,
  COMMAND_CLASS_ASSOCIATION,
  COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
  COMMAND_CLASS_ASSOCIATION_GRP_INFO,
  COMMAND_CLASS_MANUFACTURER_SPECIFIC,
  COMMAND_CLASS_DEVICE_RESET_LOCALLY,
  COMMAND_CLASS_POWERLEVEL
#if defined(BATTERY) || defined(BATTERY__CC_WAKEUP)
  ,COMMAND_CLASS_BATTERY
#endif
#if defined(BATTERY__CC_WAKEUP)
  ,COMMAND_CLASS_WAKE_UP
#endif
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

/**
 * Application node ID
 */
BYTE myNodeID = 0;


/**
 * Application state-machine state.
 */
static STATE_APP currentState = STATE_APP_IDLE;


/**
 * flag for wakeup notification is send. Used on power down.
 */
BOOL wakeupNotificationSend = FALSE;

/**
 * Parameter is used to save wakeup reason after ApplicationInitHW(..)
 */
SW_WAKEUP wakeupReason;


#ifdef APP_SUPPORTS_CLIENT_SIDE_AUTHENTICATION
s_SecurityS2InclusionCSAPublicDSK_t sCSAResponse = { 0, 0, 0, 0};
#endif /* APP_SUPPORTS_CLIENT_SIDE_AUTHENTICATION */


/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/


/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

void ZCB_DeviceResetLocallyDone(TRANSMISSION_RESULT * pTransmissionResult);
STATE_APP GetAppState();
void AppStateManager( EVENT_APP event);
void ChangeState( STATE_APP newState);
BOOL ApplicationIdle(void);
#ifdef BOOTLOADER_ENABLED
void ZCB_OTAFinish(OTA_STATUS otaStatus);
BOOL ZCB_OTAStart(void);
#endif
#if defined(BATTERY) || defined(BATTERY__CC_WAKEUP)
void ZCB_BattReportSentDone(TRANSMISSION_RESULT * pTransmissionResult);
#endif // BATTERY

#if defined(BATTERY__CC_WAKEUP)
void ZCB_JobStatus(TRANSMISSION_RESULT * pTransmissionResult);
#endif // BATTERY__CC_WAKEUP

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

  /* Setup Button S1 on the development board */
  /* CHANGE THIS - Set up your external hardware here */
  wakeupReason = bWakeupReason;
  /* hardware initialization */

  ZDP03A_InitHW(ZCB_eventSchedulerEventAdd);

#if defined(BATTERY) || defined(BATTERY__CC_WAKEUP)
  InitBatteryMonitor(wakeupReason);
#endif
  SetPinIn(ZDP03A_KEY_1,TRUE); //PIN_IN(P24, 1); /*s1 ZDP03A*/
  SetPinOut(ZDP03A_LED_D1); /**< Learn mode indication*/
  Led(ZDP03A_LED_D1,OFF);

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
  EVENT_APP event_start = EVENT_APP_INIT;

  /* Init state machine*/
  currentState = STATE_APP_STARTUP;

  ZW_DEBUG_INIT(1152);
  ZW_DEBUG_MYPRODUCT_SEND_STR("AppInitSW ");
  ZW_DEBUG_MYPRODUCT_SEND_NUM(wakeupReason);
  ZW_DEBUG_MYPRODUCT_SEND_NL();


#ifdef WATCHDOG_ENABLED
  ZW_WatchDogEnable();
#endif

  UNUSED(nvmStatus);


#if defined(BATTERY) || defined(BATTERY__CC_WAKEUP)
  /*Check battery level*/
  /*Check if battery Monitor has no state change and last time was battery state ST_BATT_DEAD*/
  if((FALSE == BatterySensorRead(NULL)) && (ST_BATT_DEAD ==BatteryMonitorState()))
  {
    AppStateManager(EVENT_APP_IS_POWERING_DOWN);
    return application_node_type;
  }
  ZAF_pm_Init(wakeupReason);
#endif // BATTERY || BATTERY__CC_WAKEUP
#if defined(BATTERY__CC_WAKEUP)
  wakeupNotificationSend = FALSE;
#endif // BARRERY__CC_WAKEUP

#ifdef BATTERY__CC_WAKEUP
    SetWakeUpConfiguration(WAKEUP_PAR_DEFAULT_SLEEP_TIME, DEFAULT_SLEEP_TIME);
    SetWakeUpConfiguration(WAKEUP_PAR_MAX_SLEEP_TIME,     MAX_SLEEP_TIME);
    SetWakeUpConfiguration(WAKEUP_PAR_MIN_SLEEP_TIME,     MIN_SLEEP_TIME);
    SetWakeUpConfiguration(WAKEUP_PAR_SLEEP_STEP,         STEP_SLEEP_TIME);
#endif // BARRERY__CC_WAKEUP

  /* Initialize the NVM if needed */
  if (MemoryGetByte((WORD)&EEOFFSET_MAGIC_far) == APPL_MAGIC_VALUE)
  {
    /* Initialize PowerLevel functionality*/
    loadStatusPowerLevel();
    /* Initialize association module */
    AssociationInit(FALSE);
  }
  else
  {
    /* Initialize transport layer NVM */
    Transport_SetDefault();
    /* Reset protocol */
    ZW_SetDefault();
    /* Initialize PowerLevel functionality.*/
    loadInitStatusPowerLevel();

    /* Initialize association module */
    AssociationInit(TRUE);

#if defined(BATTERY) || defined(BATTERY__CC_WAKEUP)
    SetDefaultBatteryConfiguration(DEFAULT_SLEEP_TIME);
    ActivateBattNotificationTrigger();
#endif // BATTERY || BATTERY__CC_WAKEUP
#if defined(BATTERY__CC_WAKEUP)
    CmdClassWakeUpNotificationMemorySetDefault();
#endif // BARRERY__CC_WAKEUP
    ZW_MEM_PUT_BYTE((WORD)&EEOFFSET_MAGIC_far, APPL_MAGIC_VALUE);
  }

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

  /*
   * Initialize Event Scheduler.
   */
  ZAF_eventSchedulerInit(AppStateManager);

  Transport_OnApplicationInitSW( &m_AppNIF);
  ZCB_eventSchedulerEventAdd( EVENT_APP_INIT);

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
 * @brief Check if application is idle
 * @return true if application idle
 */
static BOOL ApplicationIdle(void)
{
  BOOL status =FALSE;
  if (STATE_APP_POWERDOWN == GetAppState()) //low battery
  {
    ZW_DEBUG_SEND_STR("PD");
    status = TRUE;
  }
  else
  {
    BOOL task_poll_status = TaskApplicationPoll();
    STATE_APP appState = GetAppState();
    BOOL activeTxJobs = ZAF_mutex_isActive();
    BOOL plStatus = CommandClassPowerLevelIsInProgress();
    BOOL keepAwakeTimerActive = FALSE;
#if defined(BATTERY) || defined(BATTERY__CC_WAKEUP)
    keepAwakeTimerActive = ZAF_pm_IsActive();
#endif //BATTERY || BATTERY__CC_WAKEUP

    if ((FALSE == task_poll_status) && // Check Task Handler for active task.
        (STATE_APP_IDLE == appState) && //Check application is in idle state.
        (FALSE == activeTxJobs) && //Check ZW_tx_mutex for active jobs, then buffers occupied.
        (FALSE == plStatus) && //Check CC power level for active test job.
        (FALSE == keepAwakeTimerActive)) //Check power down timer is running.
    {
      BOOL changeState = FALSE;
      /*
       * Check event queue for queued up jobs
       */
      if(0 != ZAF_jobQueueCount())
      {
        ZW_DEBUG_MYPRODUCT_SEND_BYTE('q');
        changeState = TRUE;
      }

#if defined(BATTERY) || defined(BATTERY__CC_WAKEUP)
      /*
       * It is important that the Wake Up Notification is the last thing to do before going to sleep.
       * The Wake Up Notification will notify the controller that we are awake and it'll start sending
       * commands if it has any. Hence, we need to be done with everything else before these commands
       * arrive.
       */
      else if(TRUE == TimeToSendBattReport())
      {
        ZW_DEBUG_MYPRODUCT_SEND_BYTE('B');
        /*Add event's on job-queue*/
        ZCB_EventEnqueue(EVENT_APP_BATT_LOW_TX);
        changeState = TRUE;
      }
#if defined(BATTERY__CC_WAKEUP)
      else if(((SW_WAKEUP_RESET == wakeupReason) ||
               (SW_WAKEUP_WUT == wakeupReason)) &&
               (FALSE == wakeupNotificationSend))
      {
        ZW_DEBUG_MYPRODUCT_SEND_BYTE('W');
        wakeupNotificationSend = TRUE;
        ZCB_EventEnqueue(EVENT_APP_WAKE_UP_NOTIFICATION_TX);
        changeState = TRUE;
      }
#endif // BATTERY__CC_WAKEUP

      if (TRUE == changeState)
      {
        ZW_DEBUG_MYPRODUCT_SEND_BYTE('T');
        ChangeState(STATE_APP_TRANSMIT_DATA);
        ZCB_eventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
      }
      else
      {
        //ZW_DEBUG_MYPRODUCT_SEND_BYTE('O');
        status = TRUE;
      }
    }
#else //BATTERY || BATTERY__CC_WAKEUP
      if (TRUE == changeState)
      {
        ChangeState(STATE_APP_TRANSMIT_DATA);
        ZCB_eventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
      }
      status = FALSE;// Always on node!
    }
#endif  //BATTERY || BATTERY__CC_WAKEUP
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

    case COMMAND_CLASS_BASIC:
      frame_status = handleCommandClassBasic(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2:
      frame_status = handleCommandClassMultiChannelAssociation(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_SUPERVISION:
      frame_status = handleCommandClassSupervision(rxOpt, pCmd, cmdLength);
      break;
#ifdef BATTERY__CC_WAKEUP
    case COMMAND_CLASS_WAKE_UP:
      frame_status = HandleCommandClassWakeUp(rxOpt, pCmd, cmdLength);
      break;
#endif //BATTERY__CC_WAKEUP

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

    case COMMAND_CLASS_BASIC:
      commandClassVersion =  CommandClassBasicVersionGet();
      break;

    case COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2:
      commandClassVersion = CmdClassMultiChannelAssociationVersion();
      break;

    case COMMAND_CLASS_SUPERVISION:
      commandClassVersion = CommandClassSupervisionVersionGet();
      break;
#ifdef BATTERY__CC_WAKEUP
    case COMMAND_CLASS_WAKE_UP:
      commandClassVersion = CmdClassWakeupVersion();
      break;
#endif // BATTERY__CC_WAKEUP

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
#if defined(BATTERY) || defined(BATTERY__CC_WAKEUP)
  ZAF_pm_KeepAwakeAuto();
#endif
}


/**
 * @brief See description for function prototype in slave_learn.h.
 */
void ApplicationNetworkLearnModeCompleted(BYTE bNodeID)
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
        /*Success*/
        myNodeID = bNodeID;
        if (0 == myNodeID)
        {
          /*Clear association*/
          AssociationInit(TRUE);
#if defined(BATTERY) || defined(BATTERY__CC_WAKEUP)
          SetDefaultBatteryConfiguration(DEFAULT_SLEEP_TIME);
          ActivateBattNotificationTrigger();
#endif //BATTERY || BATTERY__CC_WAKEUP
#if defined(BATTERY__CC_WAKEUP)
          CmdClassWakeUpNotificationMemorySetDefault();
#endif //BATTERY__CC_WAKEUP
          MemoryPutByte((WORD)&EEOFFSET_MAGIC_far, APPL_MAGIC_VALUE);

          ZCB_eventSchedulerEventAdd(EVENT_APP_INIT);
          ChangeState(STATE_APP_STARTUP);
        }
        else
        {
#if defined(BATTERY) || defined(BATTERY__CC_WAKEUP)
          /* We are included! Inform controller device battery status by clearing flag*/
          ActivateBattNotificationTrigger();

          ZW_DEBUG_MYPRODUCT_SEND_NL();
          ZW_DEBUG_MYPRODUCT_SEND_STR("INCLUDED!");
          /*
           * Make sure that we stay awake until all inclusion related stuff is done.
           * Even though we are actually included at this point.
           */
          ZAF_pm_KeepAwake(ZAF_PM_LEARNMODE_TIMEOUT);
#endif //BATTERY || BATTERY__CC_WAKEUP

#if defined(BATTERY__CC_WAKEUP)
          /*
           * Activate CC Wakeup-state (10 sec powerdown timeout between between frames). Controller get
           * more time to setup device.
           */
          ZAF_jobEnqueue(EVENT_APP_WAKE_UP_NOTIFICATION_TX);
#endif //BATTERY__CC_WAKEUP
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
  ZW_DEBUG_MYPRODUCT_SEND_STR("AppStateManager ev ");
  ZW_DEBUG_MYPRODUCT_SEND_NUM(event);
  ZW_DEBUG_MYPRODUCT_SEND_STR(" st ");
  ZW_DEBUG_MYPRODUCT_SEND_NUM(currentState);
  ZW_DEBUG_MYPRODUCT_SEND_NL();

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

      if(EVENT_APP_INIT == event)
      {
        ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_INCLUSION_SMARTSTART);
      }

      ChangeState(STATE_APP_IDLE);
      break;

    case STATE_APP_IDLE:
      if(EVENT_APP_REFRESH_MMI == event)
      {
        Led(ZDP03A_LED_D1,OFF);
      }

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
      }

      if((EVENT_KEY_B1_HELD_10_SEC == event) || (EVENT_SYSTEM_RESET ==event))
      {
        AGI_PROFILE lifelineProfile = {ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL_NA_V2, ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL_LIFELINE};

        handleCommandClassDeviceResetLocally(&lifelineProfile, ZCB_DeviceResetLocallyDone);
      }
      break;

    case STATE_APP_LEARN_MODE:
      if(EVENT_APP_REFRESH_MMI == event)
      {
        Led(ZDP03A_LED_D1,ON);
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

    case STATE_APP_TRANSMIT_DATA:
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

#if defined(BATTERY) || defined(BATTERY__CC_WAKEUP)
      if(EVENT_APP_BATT_LOW_TX == event)
      {
        if (JOB_STATUS_SUCCESS != SendBattReport( ZCB_BattReportSentDone ))
        {
          ActivateBattNotificationTrigger();
          ZCB_eventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
        }
      }
#endif // BATTERY || BATTERY__CC_WAKEUP

#if defined(BATTERY__CC_WAKEUP)

      if (EVENT_APP_WAKE_UP_NOTIFICATION_TX == event)
      {
        CC_WakeUp_notification_tx(ZCB_JobStatus);
      }
#endif // BATTERY__CC_WAKEUP

      if(EVENT_APP_FINISH_EVENT_JOB == event)
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

    case STATE_APP_POWERDOWN:
      /* Device is powering down.. wait!*/
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
 * @brief Handler for basic set.
 *
 * Handles received basic set commands.
 *
 * @param val Parameter dependent of the application device class.
 */
void
handleBasicSetCommand(BYTE val, BYTE endpoint )
{
  UNUSED(val);
  UNUSED(endpoint);
  /* CHANGE THIS - Fill in your application code here */
}

/**
 * @brief Handler for basic get. Handles received basic get commands.
 */
BYTE
getAppBasicReport( BYTE endpoint )
{
  UNUSED(endpoint);
  /* CHANGE THIS - Fill in your application code here */
  return 0;
}

/**
 * @brief Report the target value
 * @return target value.
 */
BYTE
getAppBasicReportTarget( BYTE endpoint )
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
getAppBasicReportDuration( BYTE endpoint )
{
  UNUSED(endpoint);
  /* CHANGE THIS - Fill in your application code here */
  return 0;

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

#if defined(BATTERY) || defined(BATTERY__CC_WAKEUP)
/**
 * @brief Callback function used when sending battery report.
 */
PCB(ZCB_BattReportSentDone)(TRANSMISSION_RESULT * pTransmissionResult)
{
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
#endif // BATTERY || BATTERY__CC_WAKEUP


#if defined(BATTERY__CC_WAKEUP)

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
#endif // BATTERY__CC_WAKEUP
