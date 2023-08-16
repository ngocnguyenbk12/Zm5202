/****************************************************************************
 *
 * Z-Wave, the wireless language.
 *
 * Copyright (c) 2001-2011
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description: Interface for the CSerial class
 * 
 * Last Changed By:  $Author$: 
 * Revision:         $Rev$: 
 * Last Changed:     $Date$: 
 * 
 ****************************************************************************/

#if !defined(AFX_SERIAL_H__74C54CA9_B0C9_11D5_B2F7_000102F362B5__INCLUDED_)
#define AFX_SERIAL_H__74C54CA9_B0C9_11D5_B2F7_000102F362B5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define SER_TIMEOUT		600 /*Timeout value in milliseconds*/

class CSerial  
{
public:
	static unsigned int serPutChar(HANDLE h, unsigned char c);
	static unsigned int serWrite(HANDLE h, LPVOID buf, DWORD len);
	static unsigned int serGetChar(HANDLE h, unsigned char *c, BOOL *bStop);
	static unsigned int serRead(HANDLE h, LPVOID buf, DWORD len);
	static void serClose(HANDLE h);
	static HANDLE serOpen( char *port, int mode);
	CSerial();
	virtual ~CSerial();
	/* Mode definitions */
	enum { serial_115200_8n1=0, serial_57600_8n1, serial_19200_8n1, serial_9600_8n1, serial_max };

private:
	char serGetLastError();
	/* Error definitions */
	#define SERIAL_ERROR_CREATING_DCB 1
	#define SERIAL_ERROR_SETTING_BAUDRATE 2 
	#define SERIAL_ERROR_OPENING_PORT 3
};

#endif // !defined(AFX_SERIAL_H__74C54CA9_B0C9_11D5_B2F7_000102F362B5__INCLUDED_)
