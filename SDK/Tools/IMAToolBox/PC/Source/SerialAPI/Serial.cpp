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
 * Description: Serial interface functions.
 *
 * Last Changed By:  $Author$: 
 * Revision:         $Rev$: 
 * Last Changed:     $Date$: 
 *
 ****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include "stdafx.h"
#include "Serial.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

static char lastError=0;

CSerial::CSerial()
{

}

CSerial::~CSerial()
{

}

/****************************************************************************
 *
 * Function:  serGetLastError
 *
 *   Get the error code for the last serial.c function that failed.
 *
 * Input: Nothing
 *
 * Return: The error code
 *
 ***************************************************************************/
char CSerial::serGetLastError()
{
	return lastError;
}

/****************************************************************************
 *
 * Function:  serOpen
 *
 *   Opens a serial port in a given mode.
 *
 * Input: port, string describing with port to open (ie. COM1, COM2..)
 *        mode, the bitrate plus the number of databits,stopbits and
 *        the parity. Should be given as one of the serial_xxxx_yyy
 *        constants (see serial.h)
 *
 * Return: A win32 HANDLE to the file corresponding to the serial port.
 *
 ***************************************************************************/
HANDLE CSerial::serOpen(char *port, int mode)
{
	static DCB dcb;
	HANDLE hComm;
  
	FillMemory(&dcb, sizeof(dcb), 0);
	dcb.DCBlength = sizeof(dcb);

	dcb.BaudRate = 115200;
	if (mode == serial_57600_8n1)
	{
		dcb.BaudRate = 57600;
	}
	else if (mode == serial_19200_8n1)
	{
		dcb.BaudRate = 19200;
	}
	else if (mode == serial_9600_8n1)
	{
		dcb.BaudRate = 9600;
	}
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;

	hComm = CreateFile(port, GENERIC_READ | GENERIC_WRITE,0, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH, NULL);


	if (hComm == INVALID_HANDLE_VALUE)
	{
		lastError = SERIAL_ERROR_OPENING_PORT;
	    return 0;
	}

	if ( !SetCommState(hComm, &dcb) )
	{
		lastError = SERIAL_ERROR_SETTING_BAUDRATE;
	    return 0;
	}

	COMMTIMEOUTS Timeouts;
	// Read operation will wait max SER_TIMEOUT (ms) for a character before returning
	// See also serRead()
	
	Timeouts.ReadIntervalTimeout=MAXDWORD;
	Timeouts.ReadTotalTimeoutMultiplier=MAXDWORD;
	Timeouts.ReadTotalTimeoutConstant=SER_TIMEOUT;
	SetCommTimeouts(hComm,&Timeouts);

	return hComm;
}

/****************************************************************************
 *
 * Function:  serClose
 *
 *   Free a serial port after use.
 *
 * Input: h, the win32 handle returned by serOpen().
 *
 * Return: Nothing
 *
 ***************************************************************************/
void CSerial::serClose(HANDLE h)
{
	if (!CloseHandle(h))
  {
		DWORD a = GetLastError();
	}
}

/****************************************************************************
 *
 * Function:  serRead
 *
 *   Reads a number of bytes from a serial port. Will wait until 
 *   all request bytes are available.
 *
 * Input: h, the win32 handle returned by serOpen
 *        buf, pointer to where the data should be stored
 *        len, the number of bytes to receive
 *
 * Return: The number of bytes actually read.
 *
 ***************************************************************************/
unsigned int CSerial::serRead(HANDLE h, LPVOID buf, DWORD len)
{
  DWORD numread;
  ReadFile(h, buf, len, &numread,NULL);
  return numread;
}

/****************************************************************************
 *
 * Function:  serGetChar
 *
 *   Read a single byte from a serial port.
 *
 * Input: h, the win32 handle returned by serOpen()
 *        bStop, pointer to boolean indicating if the thread is stopping
 *
 * Return: The read byte.
 *
 ***************************************************************************/
unsigned int CSerial::serGetChar(HANDLE h, unsigned char *c, BOOL *bStop)
{
	int res = serRead(h, c, 1);

	return res;
}

/****************************************************************************
 *
 * Function:  serWrite
 *
 *   Writes a number of bytes to a serial port. Will wait until 
 *   all bytes are sent.
 *
 * Input: h, the win32 handle returned by serOpen
 *        buf, pointer to where the data are be stored
 *        len, the number of bytes to send
 *
 * Return: The number of bytes actually send.
 *
 ***************************************************************************/
unsigned int CSerial::serWrite(HANDLE h, LPVOID buf, DWORD len)
{
	DWORD num;
	WriteFile(h, buf, len, &num, NULL);
	return num;
} 

/****************************************************************************
 *
 * Function:  serPutChar
 *
 *   Write a single byte to a serial port.
 *
 * Input: h, the win32 handle returned by serOpen()
 *        c, the char to write/send
 *
 * Return: The number of bytes actually send.
 *
 ***************************************************************************/
unsigned int CSerial::serPutChar(HANDLE h, unsigned char c)
{
	return serWrite(h, &c, 1);
}
