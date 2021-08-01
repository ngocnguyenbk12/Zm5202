/**
 * @file SwitchOnOff.c
 * @copyright Copyright (c) 2001-2015
 * Sigma Designs, Inc.
 * All Rights Reserved�
 * @brief Z-Wave Switch On/Off Sample Application
 * @details This sample application is a Z-Wave slave node which has an LED (D2
 * on ZDP03A) that can be turned on or off from another Z-Wave node by sending
 * a Basic Set On or a Basic Set Off command.
 *
 * It can be included and excluded from a Z-Wave network by pressing S1 switch
 * on the ZDP03A board 3 times. S2 switch toggles LED D2. S3 switch transmits
 * a Node Information Frame (NIF).
 *
 * Last changed by: $Author: $
 * Revision:        $Revision: $
 * Last changed:    $Date: $
 *
 * @author Someone who started this sample application at some point in time.
 * @author Thomas Roll (TRO)
 * @author Christian Salmony Olsen (COLSEN)
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
#include <CommandClass.h>
#include <CommandClassAssociation.h>
#include <CommandClassAssociationGroupInfo.h>
#include <CommandClassVersion.h>
#include <CommandClassZWavePlusInfo.h>
#include <CommandClassPowerLevel.h>
#include <CommandClassDeviceResetLocally.h>
#include <CommandClassBasic.h>
#include <CommandClassBinarySwitch.h>
#include <CommandClassSupervision.h>
#include <CommandClassMultiChan.h>
#include <CommandClassMultiChanAssociation.h>
#include <zaf_version.h>
#include <gpio_driver.h>

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



#define Pin3 0x11
#define Pin4 0x10
#define Pin5 0x04

typedef enum Blink_Mode
{
	Fast,
	Slow
} Blink_Mode;


/**
 * Application events for AppStateManager(..)
 */
typedef enum _EVENT_APP_
{
  EVENT_EMPTY = DEFINE_EVENT_APP_NBR,
  EVENT_APP_INIT,
  EVENT_APP_REFRESH_MMI,
  EVENT_APP_OTA_HOST_WRITE_DONE,
  EVENT_APP_OTA_HOST_STATUS,
  EVENT_APP_SMARTSTART_IN_PROGRESS,
  EVENT_DEBUG 
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
  STATE_APP_OTA_HOST,
  STATE_DEBUG
} STATE_APP;


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
  COMMAND_CLASS_SWITCH_BINARY,
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
 **/
static code BYTE cmdClassListSecure[] =
{
  COMMAND_CLASS_VERSION,
  COMMAND_CLASS_SWITCH_BINARY,
  COMMAND_CLASS_ASSOCIATION,
  COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
  COMMAND_CLASS_ASSOCIATION_GRP_INFO,
  COMMAND_CLASS_MANUFACTURER_SPECIFIC,
  COMMAND_CLASS_DEVICE_RESET_LOCALLY,
  COMMAND_CLASS_POWERLEVEL
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
 * Application node ID
 */
BYTE myNodeID = 0;

/**
 * Application state-machine state.
 */
static STATE_APP currentState = STATE_APP_IDLE;

/**
 * Parameter is used to save wakeup reason after ApplicationInitHW(..)
 */
SW_WAKEUP wakeupReason;

/**
 * LED state
 */
static BYTE onOffState;

/**
 * Use to tell if the host OTA required auto rebooting or not
 */
BOOL userReboot = FALSE;

#ifdef APP_SUPPORTS_CLIENT_SIDE_AUTHENTICATION
s_SecurityS2InclusionCSAPublicDSK_t sCSAResponse = { 0, 0, 0, 0};
#endif /* APP_SUPPORTS_CLIENT_SIDE_AUTHENTICATION */

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

// No exported data.

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

void ZCB_DeviceResetLocallyDone(TRANSMISSION_RESULT * pTransmissionResult);
void ZCB_ResetDelay(void);
STATE_APP GetAppState();
void AppStateManager( EVENT_APP event);
void ChangeState( STATE_APP newState);
#ifdef BOOTLOADER_ENABLED
void ZCB_OTAFinish(OTA_STATUS otaStatus);
BOOL ZCB_OTAStart(void);
void ZCB_OTAWrite(BYTE *pData, BYTE len);
#endif

void LoadConfiguration(ZW_NVM_STATUS nvmStatus);
void SetDefaultConfiguration(void);


void RefreshMMI(void);

void controlRelay(void);
void delay1s(void);
void delay1ms(void);
void Blinkled(BYTE Pin,Blink_Mode Mode);
void Fast_delay(void);
void Slow_delay(void);
void FastBlink(BYTE Pin);
void Toggle(BYTE Pin);


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
	
	
	
// GPIO 
	
//	SetPinOut(Pin13);
	SetPinOut(Pin3);
	Led(Pin3,0);
//  Led(Pin13 ,0);     
	
	SetPinIn(Pin4, TRUE);
	SetPinIn(Pin5, TRUE);

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
#ifdef WATCHDOG_ENABLED
  ZW_WatchDogEnable();
#endif

  /* Signal that the sensor is awake */
  LoadConfiguration(nvmStatus);

  /* Setup AGI group lists*/
  AGI_Init();
  AGI_LifeLineGroupSetup(agiTableLifeLine, (sizeof(agiTableLifeLine)/sizeof(CMD_CLASS_GRP)), GroupName, ENDPOINT_ROOT);

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
  ZCB_eventSchedulerEventAdd(EVENT_APP_INIT);

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
	static WORD i = 0;
  UNUSED(bProtocolState);

#ifdef WATCHDOG_ENABLED
  ZW_WatchDogKick();
#endif
	if(i == 10000){
	}
	else{
		if(!KeyGet(Pin4)){
	Blinkled(Pin3, Fast);
    ChangeState(STATE_APP_IDLE);        
    ZCB_eventSchedulerEventAdd(EVENT_SYSTEM_LEARNMODE_START);
		}
  	if(!KeyGet(Pin5)){ 
	Blinkled(Pin3, Slow);
    	ChangeState(STATE_APP_IDLE);
      ZCB_eventSchedulerEventAdd(EVENT_SYSTEM_RESET);
  	}
    if(i<=1000){
  		++i;
  	}
		}
  TaskApplicationPoll();

  return E_APPLICATION_STATE_ACTIVE;
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

    case COMMAND_CLASS_SWITCH_BINARY:
      frame_status = handleCommandClassBinarySwitch(rxOpt, pCmd, cmdLength);
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
    case COMMAND_CLASS_SWITCH_BINARY:
     commandClassVersion = CommandClassBinarySwitchVersionGet();
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
}

/**
 * See description for function prototype in ZW_slave_api.h.
 */
void ApplicationNetworkLearnModeCompleted(uint8_t bNodeID)
{
  ZW_DEBUG_SEND_STR("\r\nApplicationNetworkLearnModeCompleted ");
    ZW_DEBUG_SEND_NUM(bNodeID);
    ZW_DEBUG_SEND_NL();
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
          SetDefaultConfiguration();

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
//        Led(ZDP03A_LED_D1,OFF);
        RefreshMMI();
      }

      if(EVENT_APP_SMARTSTART_IN_PROGRESS == event)
      {
        ChangeState(STATE_APP_LEARN_MODE);
      }

      if((EVENT_KEY_B1_PRESS == event) ||(EVENT_SYSTEM_LEARNMODE_START == event))
      {
        if (myNodeID)
        {
          ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_EXCLUSION_NWE);
        }
        else{
			gpio_SetPin(Pin3, TRUE);
          ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_INCLUSION);
        }
        ChangeState(STATE_APP_LEARN_MODE);
      }

      if((EVENT_KEY_B1_HELD_10_SEC == event) || (EVENT_SYSTEM_RESET ==event))
      {
        /*
         * Since this application is a routing slave, it'll use the internal NVM also known as the
         * MTP. The MTP is getting flushed 300 ms after the latest write which means we'll have to
         * wait some time before resetting the device.
         */
        MemoryPutByte((WORD)&EEOFFSET_MAGIC_far, 1 + APPL_MAGIC_VALUE);
        ZW_TIMER_START(ZCB_ResetDelay, 50, 1); // 50 * 10 = 500 ms  to be sure.
      }
      break;

    case STATE_APP_LEARN_MODE:
      if(EVENT_APP_REFRESH_MMI == event)
      {
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
      ZW_WatchDogEnable(); /*reset asic*/
      for (;;) {}
      break;

    case STATE_APP_OTA:
      if(EVENT_APP_REFRESH_MMI == event){}
      /*OTA state... do nothing until firmware update is finish*/
      break;

#ifdef BOOTLOADER_ENABLED
    case STATE_APP_OTA_HOST:
      /*
       * Wait here until host firmware update is finished.
       */
      if ( EVENT_APP_REFRESH_MMI == event)
      {
        RefreshMMI();
      }
      if (EVENT_APP_OTA_HOST_WRITE_DONE == event)
      {
        OtaHostFWU_WriteFinish();
      }
      if (EVENT_APP_OTA_HOST_STATUS == event)
      {

      // we assume we always updated successfully
        userReboot = FALSE;
        OtaHostFWU_Status(userReboot, TRUE);
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
  handleCommandClassDeviceResetLocally(&lifelineProfile, ZCB_DeviceResetLocallyDone);
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
  if (STATE_APP_OTA_HOST == GetAppState())
  {
    ChangeState(STATE_APP_IDLE);
    if (userReboot)
    {
      userReboot = FALSE;
      return;
    }
  }
  if (OTA_STATUS_DONE == otaStatus)
  {
  /*Just reboot node to cleanup and start on new FW.*/
    ZW_WatchDogEnable(); /*reset asic*/
    while(1);
  }
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


/**
 * @brief Called OTA firmware upgrade want to write an image
 * @param BYTE *pData pointer to the image data to write.
 * @param BYTE len length of the image data
 */
PCB(ZCB_OTAWrite)(BYTE *pData, BYTE len)
{
  static WORD adr = 0;
  UNUSED(pData);
  if (STATE_APP_IDLE == GetAppState())
  {
	ChangeState(STATE_APP_OTA_HOST);
  }
  if (len)
  {
    adr += len;
    while(--len)
    {
    }
    ZCB_eventSchedulerEventAdd(EVENT_APP_OTA_HOST_WRITE_DONE);

  }
  else
  {
    // set event ota host status
    ZCB_eventSchedulerEventAdd(EVENT_APP_OTA_HOST_STATUS);
  }

}
#endif


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
  CommandClassBinarySwitchSupportSet(val, endpoint);
}


/**
 * @brief Handler for basic get. Handles received basic get commands.
 */
BYTE
getAppBasicReport( BYTE endpoint )
{
  return handleAppltBinarySwitchGet(endpoint);
}


/**
 * @brief Report the target value
 * @return target value.
 */
BYTE
getAppBasicReportTarget( BYTE endpoint )
{
  return handleAppltBinarySwitchGet(endpoint);
}


/**
 * @brief Report transition duration time.
 * @return duration time.
 */
BYTE
getAppBasicReportDuration( BYTE endpoint )
{
  UNUSED(endpoint);
  return 0;
}

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
        VG_VERSION_REPORT_V2_VG* pVariantgroup)
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
 * Function return firmware Id of target n (0 => is device FW ID)
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
  else if (n == 1)
  {
    return 0x1234;
  }
  return 0;
}


/**
 * @brief See description for function prototype in CommandClassBianrySwitch.h
 */
BYTE
handleAppltBinarySwitchGet(BYTE endpoint)
{
  UNUSED(endpoint);
  return MemoryGetByte((WORD)&OnOffState_far);
}


/**
 * @brief See description for function prototype in CommandClassBianrySwitch.h
 */
void
handleApplBinarySwitchSet(CMD_CLASS_BIN_SW_VAL val, BYTE endpoint )
{
  UNUSED(endpoint);
  onOffState = val;
  MemoryPutByte((WORD)&OnOffState_far, onOffState);
  controlRelay();
  RefreshMMI();
}


/**
 * @brief Sets the configuration to default values and saves it to EEPROM.
 */
void
SetDefaultConfiguration(void)
{

  onOffState = 0;
  /* Mark stored configuration as OK */
  MemoryPutByte((WORD)&OnOffState_far, onOffState);
  MemoryPutByte((WORD)&EEOFFSET_MAGIC_far, APPL_MAGIC_VALUE);
  MemoryPutByte( (WORD)&EEOFFSET_SWITCH_ALL_MODE_far[0],
    SWITCH_ALL_REPORT_INCLUDED_IN_THE_ALL_ON_ALL_OFF_FUNCTIONALITY);
}


/**
 * @brief Loads configuration from EEPROM. If no configuration is stored,
 * default values are loaded and then saved to EEPROM.
 */
void
LoadConfiguration(ZW_NVM_STATUS nvmStatus)
{
  uint8_t magicValue;

  /* Get this sensors identification on the network */
  MemoryGetID( NULL, &myNodeID);
  ManufacturerSpecificDeviceIDInit();
#ifdef BOOTLOADER_ENABLED
  NvmInit(nvmStatus);
#else
  UNUSED(nvmStatus);
#endif
  /* Check to see, if any valid configuration is stored in the EEPROM */
  magicValue = MemoryGetByte((WORD)&EEOFFSET_MAGIC_far);
  if (APPL_MAGIC_VALUE == magicValue)
  {
    /* There is a configuration stored, so load it */
    onOffState = MemoryGetByte((WORD)&OnOffState_far);
    AssociationInit(FALSE);
  }
  else
  {
    ZW_MEM_PUT_BYTE((WORD)&EEOFFS_SECURITY_RESERVED.EEOFFS_MAGIC_BYTE_field, EEPROM_MAGIC_BYTE_VALUE);
    /* Initialize transport layer NVM */
    Transport_SetDefault();
    /* Reset protocol */
    ZW_SetDefault();

    /* Apparently there is no valid configuration in EEPROM, so load */
    /* default values and save them to EEPROM. */
    SetDefaultConfiguration();

    /*Clear association*/
    AssociationInit(TRUE);

    loadInitStatusPowerLevel();
  }
  RefreshMMI();
}


/**
 * @brief Toggles LED state variable and refreshes MMI.
 */
void
controlRelay(void)
{
  if (onOffState)
  {
//    onOffState = 0;
	gpio_SetPin(Pin3,OFF);
  }
  else
  {
 //   onOffState = 0xff;
	gpio_SetPin(Pin3, ON);
  }
  RefreshMMI();
  MemoryPutByte((WORD)&OnOffState_far, onOffState);
}


/**
 * @brief Refreshes MMI.
 */
void
RefreshMMI(void)
{
  if (CMD_CLASS_BIN_OFF == onOffState)
  {
    Led(ZDP03A_LED_D2,OFF);
  }
  else if (CMD_CLASS_BIN_ON == onOffState)
  {
    Led(ZDP03A_LED_D2,ON);
  }
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


//*************************************** Timer **************************************

void Fast_delay(void){
		int time;
	for(time = 0; time < 50; time++){
		delay1ms();
		}
}
void Slow_delay(void){
		int time;
	for(time = 0; time < 300; time++){
		delay1ms();
		}
}

void delay1s(void){
	int time;
	for(time =0; time <500; time++){
		delay1ms();
	}
}
void delay1ms(void)                          //định nghĩa hàm delay
{
                        TMOD=0x01;           //chọn timer0 chế độ 1 16Bit
                        TL0=0xD5;                //nạp giá trị cho TL0
                        TH0=0x97;               //nạp giá trị cho TH0
                        TR0=1;                       //khởi động timer0
                        while(!TF0){}           //vòng lặp kiểm tra cờ TF0
                        TR0=0;                       //ngừng timer0
                        TF0=0;                       //xóa cờ TF0
}


//**************************************** GPIO  ***********************************************

void Blinkled(BYTE Pin,Blink_Mode Mode){
	int i;
	if(Mode == Fast){
		for(i = 0; i < 5; i++){
			gpio_SetPin(Pin, TRUE);
			Fast_delay();
			gpio_SetPin(Pin, FALSE);
			Fast_delay();
		}
		}
	if(Mode == Slow){
		for(i = 0; i< 5; i++){
			gpio_SetPin(Pin, TRUE);
			Slow_delay();
			gpio_SetPin(Pin, FALSE);
			Slow_delay();
			}
		}
}

