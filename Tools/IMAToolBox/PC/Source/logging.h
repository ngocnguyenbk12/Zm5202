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
 * Description: Headerfile for logging module
 *
 * Last Changed By:  $Author$: 
 * Revision:         $Rev$: 
 * Last Changed:     $Date$: 
 *
 ****************************************************************************/
#if !defined(_LOGGING_H_)
#define _IMATOOLBOX_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

typedef struct _sLOGENTRY
{
	char *entry;
	_sLOGENTRY *next;
	_sLOGENTRY *prev;
} sLOGENTRY;


typedef struct _sLOG
{
	char *logname;
	long int loglineno;
	CRITICAL_SECTION CritSection;
	sLOGENTRY *first;
	_sLOG *nextlog;
	_sLOG *prevlog;
} sLOG;


typedef struct _sLOGROOT
{
	sLOG *first;
} sLOGROOT;

extern char main_logfilename[20];
extern char debug_logfilename[20];

class CLogging
{
public:
	CLogging();
	virtual ~CLogging();
	sLOG* CLogging::AddLog(const char *logname);
	void CLogging::Write(sLOG *log, const char * fmt, ...);
	void CLogging::AddLine(sLOG *log, const char *logline);
	void CLogging::AddLine(sLOG *log, const char *logline, BOOL logToScreen);
	BOOL m_bEndThread;

protected:
	sLOGROOT *rootLog;

private:
	friend UINT LoggingThreadProc(LPVOID pParam);
};

#endif /* _LOGGING_H_ */