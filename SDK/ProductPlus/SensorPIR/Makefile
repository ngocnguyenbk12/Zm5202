#****************************************************************************
#
#  Copyright (c) 2001-2014
#  Sigma Designs, Inc.
#  All Rights Reserved
#
#----------------------------------------------------------------------------
#
# Description:
#    Makefile for the ZW0x0x development kit SensorPIR
#    This makefile defines the targets built
#
# Author:   Erik Friis Harck
#
# Last Changed By:  $Author: efh $
# Revision:         $Revision: 19611 $
# Last Changed:     $Date: 2010-12-01 12:45:57 +0100 (on, 01 dec 2010) $
#
#***************************************************************************/

#---------------------------------------------------------------------------------------------------------
# HOW-TO: Specifying which subset of "everything", you wan't to build:
#
# There is a list of build configuration parameters to be set (variants).
# If one is not set (commented out), the application will be built for all possible instances from a list,
# else the single one set will be built.
# You can specify a single instance to build for in 3 different ways:
# 1. By specifying the instance in your command line, like:
#    > make FREQUENCY=EU ....
# 2. By setting the instance just below like
#    FREQUENCY:=EU
# 3. Alternativly you can do the same by setting your environment with:
#    > SET FREQUENCY=EU
#    Remember to UNSET this when you jump to work on other things.
# And yes... You can combine these methods in any way.
# You can always use help to see the possible combinations, like
#    > make help
#
LIBRARY:=slave_routing
#LIBRARY:=slave_enhanced_232
#FREQUENCY:=EU
CODE_MEMORY_MODE:=banking
SENSOR_TYPE:=BATTERY
CHIP:=ZW050x
WATCHDOG:=WATCHDOG_DISABLED

# BOOT must be set to nonBOOT for a routing slave since there's no external NVM.
ifeq ($(LIBRARY),slave_routing)
 BOOTLOADER_ENABLED:=
 BOOT:=nonBOOT
else
 BOOTLOADER_ENABLED:=1
 BOOT:=BOOTLOADER_ENABLED
endif


#---------------------------------------------------------------------------------------------------------

# This is the default target file name end (chip | module | board or whatever you like)
CHIPPACK:=ZW050x

#---------------------------------------------------------------------------------------------------------
# If not supplied by application here, the name of the application will be set equal to it's directory name.
APP_NAME:=SensorPIR

#---------------------------------------------------------------------------------------------------------
# List of all application specific modules. General Framework modules are added in
# make file "\ApplicationUtilities\Makefile.frameworkSupport".
RELFILES:=\
 SensorPIR.obj\
 eeprom.obj\
 ZW_nvr_app_api.obj\
 io.obj\
 key_driver.obj\
 gpio_driver.obj\
 ZW_string.obj\
 association_plus.obj\
 agi.obj\
 ZW_task.obj\
 ZW_util_queue.obj\
 ev_man.obj\
 CommandClass.obj\
 CommandClassVersion.obj\
 CommandClassPowerLevel.obj\
 CommandClassDeviceResetLocally.obj\
 CommandClassAssociation.obj\
 CommandClassAssociationGroupInfo.obj\
 CommandClassManufacturerSpecific.obj\
 CommandClassZWavePlusInfo.obj\
 CommandClassBasic.obj\
 CommandClassBasicController.obj\
 CommandClassBattery.obj\
 CommandClassNotification.obj\
 CommandClassWakeUp.obj\
 notification.obj\
 battery_monitor.obj\
 battery_non_zero_vars.obj\
 ZAF_pm.obj

ifeq ($(CODE_MEMORY_MODE),banking)
# Build the project the banked way.
COMMON_RELFILES+=\
 interrupt_driver.obj
endif

ifneq ($(BOOTLOADER_ENABLED),)
RELFILES+=\
 CommandClassFirmwareUpdate.obj\
 nvm_util.obj\
 ota_util.obj\
 ZW_nvm_descriptor.obj
endif

# The two following make variables, APPLICATION_NVMLOC and NVMCONSTLOC, must have a matching sequence.

# Set APPLICATION_NVMLOC to make sure the order of nvmModules in HDATA space will be correct:
# ?FD?ZW_NVM            ZW_library NVM variables                                MUST be first (lowest address)
# ?FD?ZW_NVM_S2         Security 2 NVM Variables
# ?FD?EEPROM            Your application NVM variables
# ?FD?ZW_NVM_DESCRIPTOR t_nvmDescriptor far nvmDescriptor                       MUST be last (highest address)
# (do not include ?FD?ZW_NVM in APPLICATION_NVMLOC. This is located at fixed address 0 in HDATA because of external dependencies)
APPLICATION_NVMLOC:=?FD?ZW_NVM_S2,?FD?EEPROM


#---------------------------------------------------------------------------------------------------------
# Setup folder location

# Utility location
ifeq ($(ZWUTILDIR),)
ZWUTILDIR:=..\ApplicationUtilities
endif


#---------------------------------------------------------------------------------------------------------
# Setup transport layer varians before recursive engine

# Include Framework support
-include $(ZWUTILDIR)\Makefile.FrameworkSupport

#---------------------------------------------------------------------------------------------------------
# Target name (Name of your target directory and base name of your target files)

TARGET:=$(APP)_$(LIB)
ifneq ($(BOOTLOADER_ENABLED),)
ifeq ($(LIBRARY),slave_routing)
LIBRARY:=slave_enhanced_232
ENH:=_enhanced_232
endif
TARGET:=$(TARGET)_OTA
endif
TARGETDIR:=$(TARGET)_$(CHIPPACK)
TARGET:=$(TARGET)_$(CHIPPACK)_$(COUNTRY)

#---------------------------------------------------------------------------------------------------------
# Compute and setup references to Z-Wave library and utils (framework or whatever)
#
# Don't change this. If you want to reference another Z-Wave library or utils, use these
# environment settings instead:
#
# SET ZWLIBROOT=<full path of your alternative Z-Wave library location>
# SET ZWUTILDIR=<full path of your alternative ZWUTILDIR framework location>

# Find the current directory
# used as a base for all other paths.
BASEDIR=$(shell cd)

# Find the product name from the subdirectoryname
BASEDIRWORDLIST=$(subst \, ,$(BASEDIR))
BASEDIRLASTWORDINDEX=$(words $(BASEDIRWORDLIST))
PRODUCTDIR=$(word $(BASEDIRLASTWORDINDEX),$(BASEDIRWORDLIST))

# Find the Z-Wave library location (if not forced by setting the environment variable ZWLIBROOT)
ifndef ZWLIBROOT
ZWLIBROOT:=$(subst \$(PRODUCTDIR),,$(subst \ProductPlus,\Z-Wave,$(BASEDIR)))
endif

# Library location when making release of application
ifdef SVNLIBRELEASEDIRBIN
ZWLIBROOT:=$(SVNLIBRELEASEDIRBIN)
endif

# Export the variables declared above to the other makefiles.
export BASEDIR ZWLIBROOT UNDERSTAND_C


#---------------------------------------------------------------------------------------------------------
# After setting up the build environment include some common makefiles, which defines how to build.

include $(ZWUTILDIR)\Makefile.common

# Filter out unsupported frequencies for the Z-Wave application.
# Full list of all supported frequencies can be found in $(ZWUTILDIR)\Makefile.common
# (this filter-out must be done after/below the inclusion of $(ZWUTILDIR)\Makefile.common)
LIST_OF_FREQUENCY:=$(filter-out ALL ALL_3CH,$(LIST_OF_FREQUENCY))

#---------------------------------------------------------------------------------------------------------
# ADDITION TO THE VARIANTS TO BUILD
#---------------------------------------------------------------------------------------------------------
# Add a dimension "BOOT" to the build array
#
# BOOT variant must be added in front of LIST_OF_VARIANTS to be handled last,
# and for the filtering below in this Makefile to work. (LIST_OF_VARIANTS+= BOOT # does not work correctly)
LIST_OF_VARIANTS:=BOOT $(LIST_OF_VARIANTS)
# A full list of values for each VariantName must be assigned.
LIST_OF_BOOT:=BOOTLOADER_ENABLED nonBOOT
# A default value for each VariantName must be assigned. They are used in the help texts.
BOOT_DEFAULT:=BOOTLOADER_ENABLED
# An option variable for each VariantName must be assigned to be considered during "make",
# and of course, the application source code must be prepared to distinguish between variants.
ifeq ($(BOOT),BOOTLOADER_ENABLED)
BOOT_OPTION:=BOOTLOADER_ENABLED=1
BOOTLOADER_ENABLED:=1
endif
#---------------------------------------------------------------------------------------------------------

#---------------------------------------------------------------------------------------------------------
# Filtering out targets, which have no meaning, or are just not needed.
#
# The controlling variant name must be listed before the controlled variant name in the LIST_OF_VARIANTS
# for this to work.
#
#LIST_OF_VARIANTS:=LIBRARY FREQUENCY CODE_MEMORY_MODE SENSOR_TYPE CHIP
ifneq ($(BOOTLOADER_ENABLED),)
# BOOTLOADER_ENABLED has no meaning for slave_routing, because there is no room for a new firmware image in NVM
LIST_OF_LIBRARY:=$(filter-out slave_routing, $(LIST_OF_LIBRARY))
# BOOTLOADER_ENABLED cannot build for some of the controller types, because there is no room for code in COMMON
LIST_OF_LIBRARY:=$(filter-out controller_bridge controller_static controller_static_norep, $(LIST_OF_LIBRARY))
endif
#---------------------------------------------------------------------------------------------------------

ifneq ($(CDEFINES),)
CDEFINES+=,
endif
CDEFINES+=\
 $(SENSOR_TYPE),\
 ZW_SMARTSTART_ENABLED

