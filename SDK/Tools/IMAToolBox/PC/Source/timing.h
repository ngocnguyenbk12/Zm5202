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
 * Description: Header file for Timing module
 * 
 * Last Changed By:  $Author$: 
 * Revision:         $Rev$: 
 * Last Changed:     $Date$: 
 * 
 ****************************************************************************/
#ifndef TIMING_H
#define TIMING_H


typedef struct _sTiming
{
	LARGE_INTEGER startTime;
    LARGE_INTEGER stopTime;
	LARGE_INTEGER elapsedTime;
} sTiming;


/*=============================== TimingStart ================================
** Function description
**      Sample High Performance timer for Start time in High Performance Ticks
** Side effects:
**		
**--------------------------------------------------------------------------*/
LARGE_INTEGER TimingStart(sTiming *timer);


/*=============================== TimingStop =================================
** Function description
**      Sample High Performance timer Stop time and calculate Elapsed time
**		in High Performance Ticks
** Side effects:
**		
**--------------------------------------------------------------------------*/
LARGE_INTEGER TimingStop(sTiming *timer);


/*=========================== TimingGetElapsedMSec ===========================
** Function description
**      Returns the milli seconds which elapsedTime represent. 
**		ElapsedTime is the number of High Performance Ticks which for a given
**      period has past.
**      if 0 then either 0 milliseconds has past or milliseconds cannot be
**		returned due to the High Performance Counter Resolution.
**
** Side effects:
**		
**--------------------------------------------------------------------------*/
double TimingGetElapsedMSec(LARGE_INTEGER elapsedTime);


/*========================= TimingGetElapsedMSecDbl ==========================
** Function description
**      Returns the milli seconds which elapsedTime represent. 
**		ElapsedTime is the number of High Performance Ticks which for a given
**      period has past.
**      if 0 then either 0 milliseconds has past or milliseconds cannot be
**		returned due to the High Performance Counter Resolution.
**
** Side effects:
**		
**--------------------------------------------------------------------------*/
double TimingGetElapsedMSecDbl(double elapsedTime);



typedef struct _tTimeOut
{
    HANDLE hThread;
    unsigned threadID;
    unsigned timeOutSec;
} tTimeOut;




//**********************************************************************
#include <vector>
using namespace std;

class CTimerJob
{
private:
    time_t timeOutSec;
    int handle;

public:
    CTimerJob():handle(0), timeOutSec(0), CbTimeOut(NULL){};
    CTimerJob(BYTE h, time_t  time, void (*Cb)(int H)):handle(h), timeOutSec(time), CbTimeOut(Cb){};
    time_t GetTimeOutSec();
    void Callback(void);
    int GetHandle();
    void (*CbTimeOut)(int H);
};


class CTime
{
private:
    CTime();
    static CTime* pTimeSingleton;
    HANDLE hThread;
    int handleCounter;
    bool killThread;
    time_t nextTime;
    bool tokenlock;
    static unsigned __stdcall TimeOutThreadFunc( void* pHandle );
    std::vector<CTimerJob> array;
    std::vector<CTimerJob> localCbList;
    void TimerEngine();
    void GetToken();
    void FreeToken();
    int GetHandle();
public:
    static CTime* GetInstance();
    int Timeout( time_t time, void (*CbTimeOut)(int H));
    void Kill( int h);
    ~CTime();
};

#endif TIMING_H