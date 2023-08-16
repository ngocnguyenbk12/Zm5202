$NOMOD51
;#***************************************************************************
;#
;# Copyright (c) 2001-2011
;# Sigma Designs, Inc.
;# All Rights Reserved
;#
;#---------------------------------------------------------------------------
;#
;# Description: ZW_segment_tail.a51
;#              This module shall be linked into Z-Wave applications with the
;#              Keil LX51 linker option "LAST" for segment location. This will
;#              make it possible to obtain the real tail address of segments.
;#
;# Author:   Erik Friis Harck
;#
;# Last Changed By: $Author: efh $
;# Revision:        $Revision: 13932 $
;# Last Changed:    $Date: 2009-08-04 16:27:15 +0200 (Mon, 25 May 2009) $
;#
;#****************************************************************************

  PUBLIC      xdataFooter

?XD?ZW_XDATA_TAIL    SEGMENT 'XDATA_PATCH'
	RSEG  ?XD?ZW_XDATA_TAIL
xdataFooter:
	DS	0

	END
