/****************************************************************************
 *
 * Z-Wave, the wireless language.
 *
 * Copyright (c) 2012
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *----------------------------------------------------------------------------
 *
 * Description: Logging module
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
#include <stdlib.h>
#include <sys/timeb.h>
#include <time.h>
#include <conio.h>
#include <share.h>
#include <process.h>
#include "logging.h"


HANDLE hThreadLogging;
unsigned threadLoggingID;

CRITICAL_SECTION CriticalSectionLogQueue;


/*================================   AddLog   ===============================
**  Add a logfile for logging module to service
**
**	Returns: Pointer to logfile structure used when writing to file
**
**	Parameters:
**	logname:		IN  pointer to logfile name
**--------------------------------------------------------------------------*/
sLOG*
CLogging::AddLog(
	const char *logname)
{
	sLOG *tsLOG, *tsLOGItem;

	tsLOG = new sLOG;
	tsLOG->first = NULL;
	tsLOG->nextlog = NULL;
	tsLOG->logname = new char[255];
	tsLOG->loglineno = 0;
	strcpy_s(tsLOG->logname, 255, logname);
	InitializeCriticalSection(&tsLOG->CritSection);
	EnterCriticalSection(&CriticalSectionLogQueue);
	if (rootLog != NULL)
	{
		tsLOGItem = rootLog->first;
		if (tsLOGItem == NULL)
		{
			rootLog->first = tsLOG;
			tsLOG->prevlog = NULL;
		}
		else
		{
			while ((tsLOGItem != NULL) && (tsLOGItem->nextlog != NULL))
			{
				tsLOGItem = tsLOGItem->nextlog;
			}
			tsLOGItem->nextlog = tsLOG;
			tsLOG->prevlog = tsLOGItem;
		}
	}
	LeaveCriticalSection(&CriticalSectionLogQueue);
	return tsLOG;
}


/*================================   AddLine  ===============================
**  Add a logline to specific log
**
**	Returns: Nothing
**
**	Parameters:
**		log 		IN  The log to which the logline is to be written
**		logline		IN  The logline which is to be written
**		logToScreen IN	Is the logline to be written to screen
**--------------------------------------------------------------------------*/
void
CLogging::AddLine(
	sLOG *log,
	const char *logline,
	BOOL logToScreen)
{
	const char* timestamp = "%u%02u%02u-%02u.%02u.%02u.%03u %s\n";
	struct _timeb timebuffer;
	struct tm timeinfo;
	errno_t err;
	sLOGENTRY *tsLOGEntryPrev, *tsLOGEntry = new sLOGENTRY;
	tsLOGEntry->entry = new char[1024];

	err = _ftime64_s(&timebuffer);
	if (0 != err)
	{
		puts("Error _ftime64_s returned None ZERO");
		puts(logline);
		return;
	}
	if ((err = _localtime64_s(&timeinfo, &timebuffer.time)) != 0)
	{
		puts("Invalid arg to _localtime64_s");
		puts(logline);
		return;
	}
	sprintf_s(tsLOGEntry->entry, sizeof(char[1024]), timestamp,
		      timeinfo.tm_year+1900, timeinfo.tm_mon+1, timeinfo.tm_mday,
			  timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
			  timebuffer.millitm, logline);
	tsLOGEntry->next = NULL;
	if (logToScreen)
	{
		printf_s(tsLOGEntry->entry);
	}
	EnterCriticalSection(&log->CritSection);
	tsLOGEntryPrev = log->first;
	if (tsLOGEntryPrev != NULL)
	{
		while (tsLOGEntryPrev->next != NULL)
		{
			tsLOGEntryPrev = tsLOGEntryPrev->next;
		}
	}
	log->loglineno++;
	if (log->first == NULL)
	{
		tsLOGEntry->prev = NULL;
		log->first = tsLOGEntry;
	}
	else
	{
		tsLOGEntry->prev = tsLOGEntryPrev;
		tsLOGEntryPrev->next = tsLOGEntry;
	}
	LeaveCriticalSection(&log->CritSection);
}


/*================================   AddLog   ===============================
**  Add a logline to specific log
**
**	Returns: Nothing
**
**	Parameters:
**		log 		IN  The log to which the logline is to be written
**		logline		IN  The logline which is to be written
**--------------------------------------------------------------------------*/
void
CLogging::AddLine(
	sLOG *log, 
	const char *logline)
{
	CLogging::AddLine(log, logline, FALSE);
}


/*=================================   Write   ================================
**  Write a logline to specified log
**
**	Returns: Nothing
**
**	Parameters:
**		log 		IN  The log to which the logline is to be written
**		fmt,  ...	IN  The parameters for establishing the logline to write
**					the syntax is the same as for printf
**--------------------------------------------------------------------------*/
void 
CLogging::Write(
	sLOG *log,
	const char * fmt, ...)
{
	va_list args;
	int len;
	char *buf;

	va_start(args, fmt);
	/* Add one for the terminating '\0' */
	len =_vscprintf(fmt, args) + 1;
	buf = new char[len*sizeof(char)];
	vsprintf_s(buf, len, fmt, args);
	va_end(args);
	AddLine(log, buf);
	delete buf;
}


/*================================   CLogging   ==============================
**  Constructor - Initialize the logging functionality
**
**	Returns: Nothing
**
**--------------------------------------------------------------------------*/
CLogging::CLogging()
{
	rootLog = new sLOGROOT;
	rootLog->first = NULL;
	InitializeCriticalSection(&CriticalSectionLogQueue);
	hThreadLogging = (HANDLE)_beginthreadex(NULL, 0, (unsigned int(__stdcall *)(void *))&LoggingThreadProc, this, 0, &threadLoggingID);
}


/*===============================   ~CLogging   ==============================
**  Destructor - Tear down logging structure
**
**	Returns: Nothing
**
**--------------------------------------------------------------------------*/
CLogging::~CLogging()
{
	sLOG *tsLOGNext, *tsLOG = rootLog->first;
	sLOGENTRY *tsLOGEntryNext, *tsLOGEntry;

	EnterCriticalSection(&CriticalSectionLogQueue);
	while (tsLOG != NULL)
	{
		tsLOGEntry = tsLOG->first;
		while (tsLOGEntry != NULL)
		{
			delete tsLOGEntry->entry;
			tsLOGEntryNext = tsLOGEntry->next;
			delete tsLOGEntry;
			tsLOGEntry = tsLOGEntryNext;
		}
		tsLOGNext = tsLOG->nextlog;
		delete tsLOG;
		tsLOG = tsLOGNext;
	}
	delete rootLog;
	rootLog = NULL;
	LeaveCriticalSection(&CriticalSectionLogQueue);
}


/*===========================   LoggingThreadProc   ==========================
**  Logging functionality worker thread.
**  Does the actual writing of loglines to logfiles.
**
**	Parameters:
**		pParam 		IN  Parameter as specified by _beginthreadex
**--------------------------------------------------------------------------*/
UINT LoggingThreadProc(
	LPVOID pParam)
{
	CLogging* pObject = (CLogging*)pParam;
	FILE *pFile;
	errno_t err = 0;
	sLOG *tsLOG;
	sLOGENTRY *tsLOGEntry;

	while (!pObject->m_bEndThread)
	{
		EnterCriticalSection(&CriticalSectionLogQueue);
		if (pObject->rootLog != NULL)
		{
			tsLOG = pObject->rootLog->first;
			while (tsLOG != NULL)
			{
				if (tsLOG->first != NULL)
				{
					if ((pFile = _fsopen(tsLOG->logname, "a+", _SH_DENYNO)) == NULL)
					{
						static char str[50];
						sprintf_s(str, sizeof(str), "_fsopen call failed");
						puts(str);
						puts(tsLOGEntry->entry);
					}
					else
					{
						EnterCriticalSection(&tsLOG->CritSection);
						tsLOGEntry = tsLOG->first;
						do
						{
							if (EOF == fputs(tsLOGEntry->entry, pFile))
							{
								if ((err = ferror(pFile)) != 0)
								{
									static char str[50];
									sprintf_s(str, sizeof(str), "ferror after fputs returned %i", err);
									puts(str);
									puts(tsLOGEntry->entry);
									break;
								}
							}
							delete tsLOGEntry->entry;
							if (tsLOGEntry->next == NULL)
							{
								delete tsLOGEntry;
								break;
							}
							else
							{
								tsLOGEntry = tsLOGEntry->next;
								delete tsLOGEntry->prev;
								tsLOG->first = tsLOGEntry;
							}
						} while (tsLOGEntry != NULL);
						if (err == 0)
						{
							tsLOG->first = NULL;
						}
						LeaveCriticalSection(&tsLOG->CritSection);
					}
					fclose(pFile);
				}
				tsLOG = tsLOG->nextlog;
			}
		}
		LeaveCriticalSection(&CriticalSectionLogQueue);
		Sleep(100);
	}
	return 0;
}

