;#***************************************************************************
;#
;# Copyright (c) 2001-2013
;# Sigma Designs, Inc.
;# All Rights Reserved
;#
;#---------------------------------------------------------------------------
;*
;* Description: Makes the version for the application accessible from the
;*              library file during linking.
;*              Generates no code. Only public symbols in the obj file.
;*
;* Author:   Erik Friis Harck
;*
;* Last Changed By:  $Author: efh $
;* Revision:         $Revision: 9763 $
;* Last Changed:     $Date: 2008-01-10 11:28:42 +0100 (Thu, 10 Jan 2008) $
;*
;****************************************************************************

#include "app_version.h"

; Make APP_VERSION and APP_REVISION public, so that Z-Wave protocol code can access it.
; Use it from C-code like this:
; extern unsigned char _APP_VERSION_;
; WORD_variable = (WORD)&_APP_VERSION_);

PUBLIC  _APP_VERSION_MAJOR_
PUBLIC  _APP_VERSION_MINOR_
PUBLIC  _APP_VERSION_

_APP_VERSION_MAJOR_     EQU     APP_VERSION
_APP_VERSION_MINOR_     EQU     APP_REVISION
_APP_VERSION_           EQU     (APP_VERSION << 8) | APP_REVISION

        END
