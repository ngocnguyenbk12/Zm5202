/****************************************************************************
 *
 * Z-Wave, the wireless language.
 * 
 * Copyright (c) 2014
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 * This source file is subject to the terms and conditions of the
 * Sigma Designs Software License Agreement which restricts the manner
 * in which it may be used.
 *
 *---------------------------------------------------------------------------
 *
 * Description:      RSSI map module for Z-Wave Network Health Toolbox
 *
 * Last Changed By:  $Author$: 
 * Revision:         $Rev$: 
 * Last Changed:     $Date$: 
 *
 ****************************************************************************/
#pragma once

/*============================== RssiMapStart =============================
** Function description
**     Start the RSSI map functionality. Begin measuring and displaying an
**     RSSI map of the entire network.
**
**     Returns false if an rssi map operation is already ongoing.
**     
**--------------------------------------------------------------------------*/
bool RssiMapStart(void);
