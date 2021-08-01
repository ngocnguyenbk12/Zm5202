#****************************************************************************
#
#  Copyright (c) 2001-2014
#  Sigma Designs, Inc.
#  All Rights Reserved
#
#----------------------------------------------------------------------------
#
# Description:
#    Makefile for the ZW0x0x development kit ProdTestDUT
#    This makefile defines the targets built
#
# Author:   Erik Friis Harck
#
# Last Changed By:  $Author: efh $
# Revision:         $Revision: 29050 $
# Last Changed:     $Date: 2014-06-19 09:57:36 +0200 (to, 19 jun 2014) $
#
#****************************************************************************

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
LIBRARY:=slave_prodtest_dut
#FREQUENCY:=EU
CODE_MEMORY_MODE:=banking
SENSOR_TYPE:=NON_BATT
CHIP:=ZW050x
WATCHDOG:=WATCHDOG_DISABLED
BOOT:=nonBOOT
#---------------------------------------------------------------------------------------------------------

# This is the default target file name end (chip | module | board or whatever you like)
CHIPPACK:=ZW050x

#---------------------------------------------------------------------------------------------------------

# If not supplied by application here, the name of the application will be set equal to it's directory name.
APP_NAME:=prodtestdut
DUT_TYPE:=

#---------------------------------------------------------------------------------------------------------
# List of all application modules.
RELFILES:=\
 prodtestdut.obj

#---------------------------------------------------------------------------------------------------------
# Target name (Name of your target directory and base name of your target files)

TARGETDIR:=$(APP)_$(CHIPPACK)$(TEST)
TARGET:=$(APP)_$(CHIPPACK)_$(COUNTRY)$(TEST)

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

# Utility location
ifeq ($(ZWUTILDIR),)
ZWUTILDIR:=..\ApplicationUtilities
endif

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
LIST_OF_BOOT:=BOOTLOADER_ENABLED
# A default value for each VariantName must be assigned. They are used in the help texts.
BOOT_DEFAULT:=BOOTLOADER_ENABLED
# An option variable for each VariantName must be assigned to be considered during "make",
# and of course, the application source code must be prepared to distinguish between variants.
ifeq ($(BOOT),BOOTLOADER_ENABLED)
BOOT_OPTION:=BOOTLOADER_ENABLED=1
BOOTLOADER_ENABLED:=1
endif
#---------------------------------------------------------------------------------------------------------

ifneq ($(CDEFINES),)
CDEFINES+=,
endif
CDEFINES+=\
 ZW_PRODUCTION_TEST,\
 $(CHIP)

ifneq ($(CHIPPACK),)
ifneq ($(CDEFINES),)
CDEFINES+=,
endif
CDEFINES+=\
 $(CHIPPACK)
endif

ifneq ($(ADEFINES),)
ADEFINES+=,
endif
ADEFINES+=\
 $(CHIP)

