/****************************************************************************
 *
 * Copyright (c) 2001-2013
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description: Attributes for RF table
 *
 * Author:   Erik Friis Harck
 *
 * Last Changed By:  $Author: sse $
 * Revision:         $Revision: 10862 $
 * Last Changed:     $Date: 2008-08-04 16:27:12 +0200 (Mon, 04 Aug 2008) $
 *
 ****************************************************************************/
#ifndef _ZW_STRINGS_RF040X_H_
#define _ZW_STRINGS_RF040X_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include "ZW_rf050x.h"

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

typedef char code * code string;
typedef string code taRF_strings[];

/* Valid strings for RF_ID */
extern const taRF_strings code RF_strings;

/* 3CH indication for RF_ID */
extern const char code aRF_3CH[];

#endif /* _ZW_STRINGS_RF040X_H_ */
