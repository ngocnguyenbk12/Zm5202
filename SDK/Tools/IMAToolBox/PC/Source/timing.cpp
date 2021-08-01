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
 * Description: Timing module
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
#include <process.h>    /* _beginthread, _endthread */
#include "timing.h"
#include <time.h>

LARGE_INTEGER timeFreq;


/*=============================== TimingStart ================================
** Function description
**      Sample High Performance timer for Start time in High Performance Ticks
** Side effects:
**		
**--------------------------------------------------------------------------*/
LARGE_INTEGER
TimingStart(
    sTiming *timer)
{
    /* Sample time in startTime */
    QueryPerformanceCounter(&timer->startTime);
    /* return startTime */
    return timer->startTime;
}


/*=============================== TimingStop =================================
** Function description
**      Sample High Performance timer Stop time and calculate Elapsed time
**		in High Performance Ticks
** Side effects:
**		
**--------------------------------------------------------------------------*/
LARGE_INTEGER
TimingStop(
    sTiming *timer)
{
    /* Sample time in stopTime */
    QueryPerformanceCounter(&timer->stopTime);
    timer->elapsedTime.QuadPart = timer->stopTime.QuadPart - timer->startTime.QuadPart;
    /* return stopTime */
    return timer->stopTime;
}


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
double
TimingGetElapsedMSec(
    LARGE_INTEGER elapsedTime)
{
    double elapsedMSec;
    /* Get the resolution of the High Performance timer in ticks pr. second */
    QueryPerformanceFrequency(&timeFreq);
    if (timeFreq.QuadPart >= 1000)
    {
        /* As we want the milliseconds elapsed then divide with timeFreq / 1000 */
        elapsedMSec = (double)elapsedTime.QuadPart / ((double)timeFreq.QuadPart / 1000);
    }
    else
    {
        /* Timer resolution not big enough for generating millisec tick */
        elapsedMSec = 0;
    }
    return elapsedMSec;
}


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
double
TimingGetElapsedMSecDbl(
    double elapsedTime)
{
    double elapsedMSec;
    /* Get the resolution of the High Performance timer in ticks pr. second */
    QueryPerformanceFrequency(&timeFreq);
    if (timeFreq.QuadPart >= 1000)
    {
        /* As we want the milliseconds elapsed then divide with timeFreq / 1000 */
        elapsedMSec = elapsedTime / ((double)timeFreq.QuadPart / 1000);
    }
    else
    {
        /* Timer resolution not big enough for generating millisec tick */
        elapsedMSec = 0;
    }
    return elapsedMSec;
}

//**********************************************************************
//                    CTimerJob
//**********************************************************************

/*=============================== GetTimeOutSec =============================
** Function description
** Side effects:
**--------------------------------------------------------------------------*/
time_t  CTimerJob::GetTimeOutSec()
{
    return timeOutSec;
}


/*=============================== Callback =================================
** Function description
** Side effects:
**--------------------------------------------------------------------------*/
void CTimerJob::Callback()
{
   //printf( "Callback on %u\n", this->timeOutSec );
   if(NULL != CbTimeOut)
   {
       CbTimeOut( this->handle);
   }
}

/*=============================== GetHandle =================================
** Function description
** Side effects:
**--------------------------------------------------------------------------*/
int CTimerJob::GetHandle()
{
    return handle;
}

//**********************************************************************
//                    CTime
//**********************************************************************

CTime* CTime::pTimeSingleton = NULL;

/*=============================== CTime =================================
** Function description
** Side effects:
**--------------------------------------------------------------------------*/
CTime::CTime()
{
    killThread = false;
    handleCounter = 0;
    this->hThread = 0;
    #ifdef _CONSOLE
    // Create the second thread.
    this->hThread = (HANDLE)_beginthreadex( NULL, 2000, pTimeSingleton->TimeOutThreadFunc, NULL, CREATE_SUSPENDED , NULL );
    #else
   //AfxBeginThread((AFX_THREADPROC)CTime::TimeOutThreadFunc, this);
    #endif
}

/*=============================== ~CTime =================================
** Function description
** Side effects:
**--------------------------------------------------------------------------*/
CTime::~CTime()
{
}

/*=============================== GetInstance =================================
** Function description
** Side effects:
**--------------------------------------------------------------------------*/
CTime* CTime::GetInstance()
{
    if(NULL == pTimeSingleton)
    {
        pTimeSingleton = new CTime();
    }
    return pTimeSingleton;
}

/*=============================== Timeout =================================
** Function description
** Side effects:
**--------------------------------------------------------------------------*/
int CTime::Timeout( time_t _time, void (*CbTimeOut)(int H))
{
    time_t rawtime  = _time + time(NULL);
    int h = GetHandle();
    //struct tm* timeinfo;
    //timeinfo = localtime(&rawtime);
    //printf("\n h %u = Timeout() time = %s\n", h, asctime(timeinfo));
    pTimeSingleton->GetToken();
    array.push_back(CTimerJob(h, rawtime, CbTimeOut));
    pTimeSingleton->FreeToken();
    //printf("array size = %u \n", array.size());
    nextTime = 0;
    //printf("\nResumeThread\n");
    ResumeThread(hThread);
    return h;
}
    
/*=============================== Kill ======================================
** Function description
** Side effects:
**--------------------------------------------------------------------------*/
void CTime::Kill( int h)
{
    if(0 != h)
    {
        pTimeSingleton->GetToken();
        if(0 != pTimeSingleton->array.size())
        {
            vector<CTimerJob>::iterator p = pTimeSingleton->array.begin();

            while(p != pTimeSingleton->array.end())
            {
                if( h == (*p).GetHandle())
                {
                    //printf( "Kill H = %u\n",  h );
                    pTimeSingleton->array.erase( p);
                    break;
                }
                p++;
            }
        }
        pTimeSingleton->FreeToken();
    }
}

/*=============================== TimerEngine ===============================
** Function description
** Side effects:
**--------------------------------------------------------------------------*/
void CTime::TimerEngine()
{
    //printf( "   Start timer = %E\n",  pTimeSingleton->nextTime );
    static time_t counter =  time(NULL);

    //printf("   counter = %E\n", counter);
    while ( counter <  pTimeSingleton->nextTime)
    {
        Sleep(200);
        counter = time(NULL);
         //printf( "+");
    }
    pTimeSingleton->GetToken();

    //1. Find Callbacks.
    //printf("\n   1. array size = %u\n", pTimeSingleton->array.size());
    vector<CTimerJob>::iterator p = pTimeSingleton->array.begin();
    while(p != pTimeSingleton->array.end())
    {
        if(pTimeSingleton->nextTime == (*p).GetTimeOutSec())
        {
            //(*p).CbTimeOut();
            //(*p).Callback();
            //queue cb-function in localCbList
            pTimeSingleton->localCbList.push_back( *p);
            //remove job from array queue
            pTimeSingleton->array.erase( p);
            p = pTimeSingleton->array.begin();
        }
        else
        {
            //Searh for more timers.
            p++;
        }
    }
    pTimeSingleton->FreeToken();

    //2. Call Callbacks when token is not taken. Secure deadlock
    vector<CTimerJob>::iterator pLocal = pTimeSingleton->localCbList.begin();
    while(0 != pTimeSingleton->localCbList.size())
    {
        (*pLocal).Callback(); // Call callback function.
        pTimeSingleton->localCbList.erase(pLocal);//erase it
        pLocal = pTimeSingleton->localCbList.begin();
    }    

    //3. More timers to start?
    //printf("   3. array size = %u\n", pTimeSingleton->array.size());
    pTimeSingleton->GetToken();
    if( !pTimeSingleton->array.empty())
    {
        //3.1 find next timer
        vector<CTimerJob>::iterator next = pTimeSingleton->array.begin();
        p = pTimeSingleton->array.begin();

        while(p != pTimeSingleton->array.end())
        {
            if( (*p).GetTimeOutSec() < (*next).GetTimeOutSec())
            {
                //printf("   *Next timer H = %u\n", (*p).GetHandle());
                next = p;
            }
            p++;
        }
        //3.2 Start next timeout by calling TimeOutThreadFunc.
        pTimeSingleton->nextTime = (*next).GetTimeOutSec();
    }
    pTimeSingleton->FreeToken();
}
/*=============================== TimeOutThreadFunc ==========================
** Function description
** Side effects:
**--------------------------------------------------------------------------*/
unsigned __stdcall CTime::TimeOutThreadFunc( void* dummy )
{

    while(FALSE == pTimeSingleton->killThread)
    {
        pTimeSingleton->GetToken();
        bool emptyArray = pTimeSingleton->array.empty();
        pTimeSingleton->FreeToken();
        if( !emptyArray )
        {
            pTimeSingleton->TimerEngine();
        }
        else{
            //printf("\nSuspendThread\n");
            SuspendThread(pTimeSingleton->hThread);
        }
    }
   
    // Destroy the thread object.
    CloseHandle( pTimeSingleton->hThread );
    pTimeSingleton->hThread = 0;
    
    //End thread
    _endthreadex( 0 );
    return 0;
} 

/*=============================== GetHandle =================================
** Function description
** Find a free handle.
**--------------------------------------------------------------------------*/
int CTime::GetHandle()
{
    int h = 1;

    pTimeSingleton->GetToken();
    vector<CTimerJob>::iterator p = pTimeSingleton->array.begin();
    
    //Search for a free handle.
    while(p != pTimeSingleton->array.end())
    {
        if( h == (*p).GetHandle())
        {
            //Increase h and start over againg.
            h++;
            p = pTimeSingleton->array.begin();
        }
        p++;
    }
    pTimeSingleton->FreeToken();

    return h;
}


/*=============================== GetToken  ================================
** Function description
** Get token to start working on array. Current thread is pause if another 
** thread is working on array.
**--------------------------------------------------------------------------*/
void CTime::GetToken()
{
    while( TRUE == tokenlock)
    {
        Sleep(10);
    }
    this->tokenlock = TRUE;
}

/*=============================== GetToken  ================================
** Function description
** Free token must be called after finish working on array! Otherwise, the array is 
** locked forever.
**--------------------------------------------------------------------------*/
void CTime::FreeToken()
{
    this->tokenlock = FALSE;
}

