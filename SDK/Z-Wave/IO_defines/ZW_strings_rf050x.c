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

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include "ZW_rf050x.h"
#include "ZW_strings_rf050x.h"

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/* Valid strings for RF_ID */
const taRF_strings code RF_strings =
{
  "EU",         /* 0, EU frequency (868.42MHz) */
  "US",         /* 1, US frequency (908.42MHz) */
  "ANZ",        /* 2, Australia/Newzealand frequency (921.42MHz) */
  "HK",         /* 3, hongkong frequency (919.82MHz) */
  "TF_866",     /* 4, test frequency (866.xxMHz) */
  "TF_870",     /* 5, test frequency (870.xxMHz) */
  "TF_906",     /* 6, test frequency (906.xxMHz) */
  "TF_910",     /* 7, test frequency (910.xxMHz) */
  "MY",         /* 8, Maylasyia frequency (868.2MHz) */
  "IN",         /* 9, Indian frequency (865.22MHz) */
  "JP",         /* 10, Japan frequency (920.1MHz) */
  "TF_878",     /* 11, test frequency (878.xxMHz) */
  "TF_882",     /* 12, test frequency (882.xxMHz) */
  "TF_886",     /* 13, test frequency (886.xxMHz) */
  "TF_932_3CH", /* 14, test frequency (932.xxMHz) */
  "TF_940_3CH", /* 15, test frequency (940.xxMHz) */
  "TF_840_3CH", /* 16, test frequency (840.xxMHz) */
  "TF_850_3CH", /* 17, test frequency (850.xxMHz) */
  "JP_32MHZ",   /* 18, test Japan frequency (xxx.xxMHz) */
  "TF_836_3CH", /* 19, test frequency (836.xxMHz) */
  "TF_841_3CH", /* 20, test frequency (841.xxMHz) */
  "TF_851_3CH", /* 21, test frequency (851.xxMHz) */
  "TF_933_3CH", /* 22, test frequency (933.xxMHz) */
  "TF_941_3CH", /* 23, test frequency (941.xxMHz) */
  "TF_835_3CH", /* 24, test frequency (835.xxMHz) */
  "JP_950",     /* 25, test Japan frequency (951.1MHz) */
  "RU",         /* 26, Russia Frequency */
  "IL",         /* 27, Israelian frequency (916.0xMHz) */
  "KR",         /* 28, Korean Republic frequency (919.7xMHz) */
  "CN"          /* 29, Chinese (PRC) frequency (868.4xMHz) */
};

/* 3CH indication for RF_ID */
const char code aRF_3CH[] =
{
  0,         /* 0, EU frequency (868.42MHz) */
  0,         /* 1, US frequency (908.42MHz) */
  0,         /* 2, Australia/Newzealand frequency (921.42MHz) */
  0,         /* 3, hongkong frequency (919.82MHz) */
  0,         /* 4, test frequency (866.xxMHz) */
  0,         /* 5, test frequency (870.xxMHz) */
  0,         /* 6, test frequency (906.xxMHz) */
  0,         /* 7, test frequency (910.xxMHz) */
  0,         /* 8, Maylasyia frequency (868.2MHz) */
  0,         /* 9, Indian frequency (865.22MHz) */
  1,         /* 10, Japan frequency (920.1MHz) */
  0,         /* 11, test frequency (878.xxMHz) */
  0,         /* 12, test frequency (882.xxMHz) */
  0,         /* 13, test frequency (886.xxMHz) */
  1,         /* 14, test frequency (932.xxMHz) */
  1,         /* 15, test frequency (940.xxMHz) */
  1,         /* 16, test frequency (840.xxMHz) */
  1,         /* 17, test frequency (850.xxMHz) */
  1,         /* 18, test Japan frequency (xxx.xxMHz) */
  1,         /* 19, test frequency (836.xxMHz) */
  1,         /* 20, test frequency (841.xxMHz) */
  1,         /* 21, test frequency (851.xxMHz) */
  1,         /* 22, test frequency (933.xxMHz) */
  1,         /* 23, test frequency (941.xxMHz) */
  1,         /* 24, test frequency (835.xxMHz) */
  1,         /* 25, test Japan frequency (951.1MHz) */
  0,         /* 26, Russia Frequency */
  0,         /* 27, Israelian frequency (916.0xMHz) */
  1,         /* 28, Korean Republic frequency (919.7xMHz) */
  0          /* 29, Chinese (PRC) frequency (868.4xMHz) */
};


