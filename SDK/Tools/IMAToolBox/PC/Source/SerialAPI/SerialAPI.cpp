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
 * Description: Serial API class
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
#include <process.h>
#include <stdlib.h>
#include <conio.h>
#include "SerialAPI.h"
#include "Serial.h"
#include "ZW_SerialAPI.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
//#define new DEBUG_NEW
#endif

#ifdef _CONSOLE
#define ASSERT
#endif
/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

/* Buffer size */
#define BUF_SIZE	256 /* enough */

/* Indexes into receive buffer */
#define IDX_CMD		2
#define IDX_DATA	3

/* Max number of retransmissions/retries */
#define MAX_SERIAL_RETRY 3

/* Defines used by WaitForAck to check the state of a frame in the queue */
#define FRAME_TRANSMITTING		0x22
#define	FRAME_WAITING_FOR_ACK	0x33
#define FRAME_COMPLETED			0x44

#ifdef TIMER_SUPPORT
#define TIMER_MAX 10
static struct timerElement {
  BYTE  timerHandle;
  void  (*func)();
} timerArray[TIMER_MAX];
#define FREE_TIMER_HANDLE 0
#endif

/* Array for storing RTC timercall functions */
#define RTC_TIMER_MAX 20
static struct timerElementRTC {
  BYTE  timerHandle;
  void  (*func)(BYTE,BYTE);
} timerRTCArray[RTC_TIMER_MAX];
#define FREE_RTC_TIMER_HANDLE 0xFF

enum
{
    statusFrameReceived,		// returned when a valid frame has been received
    statusFrameSent,
    statusFrameErr,				// returned if frame has error in Checksum
    statusRxTimeout,			// returned if Rx timeout has happened
    statusTxCANned,				// returned if a CAN has been received, telling tx frame was CANned
    statusExit,					// returned if Thread got signalled to Exit
};

#define QUEUE_MAX 10
/* Command queue */
struct commandElement
{
  BYTE  buf[BUF_SIZE];		/* xxxx       */
  BYTE  length;			/* Frame type */
} commandQueue[QUEUE_MAX];
static BYTE queueIndex = 0;

/* Receive buffer */
static BYTE RecvBuffer[256];

/* Transmit buffer */
static BYTE txBuf[BUF_SIZE] = { SOF };

/* Transmit queue */
#define MAX_TX_QUEUE 4
static struct _transmit_element_
{
  BYTE      wLen;
  BYTE      wBuf[BUF_SIZE];
  BYTE		*transmitStatus;
} transmitQueue[MAX_TX_QUEUE];
static BYTE transmitIndex = 0;

static BOOL waitForAck = FALSE;
static BOOL waitForResponse = FALSE;	// Used to abort WaitForResponse() if all
static BOOL noAckReceived = FALSE; // Used in WaitForAck
                                // retries timed out during transmission
static int retry = 0;
static int retryDelay = 0;		// used to delay the last retry
static int canRetry = 0;
static BYTE seqNo;

/* Global buffer passed by reference to each ZW_SendData() callback */
TX_STATUS_TYPE sGlobalTxInfo;

static HANDLE m_hComPort = 0;
CRITICAL_SECTION CriticalSectionCommandQueue;
CRITICAL_SECTION CriticalSectionTransmitQueue;
static void (*funcApplicationCommandHandler)(BYTE rxStatus,BYTE sourceNode,BYTE*pCmd,BYTE cmdLength);
static void (*funcApplicationCommandHandler_Bridge)(BYTE rxStatus, BYTE destNode, BYTE sourceNode, BYTE *multiNodeMask, BYTE *pCmd, BYTE cmdLength);
static void (*funcCommErrorNotification)(BYTE byReason);
static void (*cbFuncZWSendData)(BYTE txStatus, TX_STATUS_TYPE *psTxInfo);
static void (*cbFuncZWSendDataMeta)(BYTE txStatus);
static void (*cbFuncZWSendDataMulti)(BYTE txStatus);
static void (*cbFuncZWSendDataMR)(BYTE txStatus);
static void (*cbFuncZWSendDataMetaMR)(BYTE txStatus);
static void (*cbFuncZWSendData_Bridge)(BYTE txStatus);
static void (*cbFuncZWSendDataMeta_Bridge)(BYTE txStatus);
static void (*cbFuncZWSendDataMulti_Bridge)(BYTE txStatus);
static void (*cbFuncZWSendNodeInformation)(BYTE txStatus);
static void (*cbFuncZWSendTestFrame)(BYTE txStatus);
static void (*cbFuncMemoryPutBuffer)();
static void (*cbFuncZWSetLearnNodeState)(BYTE, BYTE, BYTE *, BYTE);
static void (*cbFuncZWSetDefault)();
static void (*cbFuncZWNewController)(BYTE, BYTE, BYTE *, BYTE);
static void (*cbFuncZWReplicationSendData)(BYTE txStatus);
static void (*cbFuncZWAssignReturnRoute)(BYTE bStatus);
static void (*cbFuncZWAssignSUCReturnRoute)(BYTE bStatus);
static void (*cbFuncZWDeleteSUCReturnRoute)(BYTE bStatus);
static void (*cbFuncZWDeleteReturnRoute)(BYTE bStatus);
static void (*cbFuncZWSetLearnMode)(BYTE bStatus, BYTE bSource, BYTE *pCmd, BYTE bLen);
static void (*cbFuncRTCTimerCreate)();
static void (*cbIdleLearnNode)(BYTE rxStatus, BYTE sourceNode, BYTE*pCmd, BYTE cmdLength);
static void (*cbFuncZWRequestNetworkUpdate)(BYTE bStatus);
static void (*cbFuncZWRequestNodeNodeNeighborUpdate)(BYTE bStatus);
static void (*cbFuncZWRequestNodeNodeNeighborUpdate_Option)(BYTE bStatus);
static void (*cbFuncZWSetSUCNodeID)(BYTE bStatus);
static void (*cbFuncZWSendSUCID)(BYTE bStatus);
static void (*cbFuncZWReplaceFailedNode)(BYTE bStatus);
static void (*cbFuncZWRemoveFailedNode)(BYTE bStatus);
static void (*cbFuncZWRequestNodeInfo)(BYTE bStatus);

/* WIN32 thread management specifics START */
HANDLE hThreadAPI, hThreadDispatch;
unsigned threadAPIID, threadDispatchID;
/* WIN32 thread management specifics END */


void 
debuglog_wr(
   const char * fmt, ...);


void
debuglog_wr_frame(
	const char *str, 
	unsigned char *buf, 
	BYTE buflen)
{
    /* Every byte in buf will be translated into 3 chars */
    char *framestr = new char[175*3];
    int n = 0;
    for (int i = 0; i < buflen; i++)
    {
        sprintf_s(&framestr[n], 175*3-n, "%02X ", buf[i]);
        n += 3;
    }
    debuglog_wr("%s%s", str, framestr);
    delete framestr;
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSerialAPI::CSerialAPI()
{
    int i;
    cbFuncZWSendData = NULL;
    cbFuncZWSendDataMeta = NULL;
    cbFuncZWSendDataMulti = NULL;
    cbFuncZWSendDataMR = NULL;
    cbFuncZWSendDataMetaMR = NULL;
    cbFuncZWSendData_Bridge = NULL;
    cbFuncZWSendDataMeta_Bridge = NULL;
    cbFuncZWSendDataMulti_Bridge = NULL;
    cbFuncZWSendNodeInformation = NULL;
    cbFuncMemoryPutBuffer = NULL;
    cbFuncZWSetLearnNodeState = NULL;
    cbFuncZWSetDefault = NULL;
    cbFuncZWNewController = NULL;
    cbFuncZWReplicationSendData = NULL;
    cbFuncZWAssignReturnRoute = NULL;
    cbFuncZWDeleteReturnRoute = NULL;
    cbFuncZWSetLearnMode = NULL;
    cbFuncRTCTimerCreate = NULL;
    cbFuncZWAssignSUCReturnRoute = NULL;
    cbFuncZWDeleteSUCReturnRoute = NULL;
    cbFuncZWRequestNetworkUpdate = NULL;
    retry = 0;
    retryDelay = 0;
    canRetry = 0;
    RecvBuffer[0] = 0;
#ifdef TIMER_SUPPORT
    for (i = 0; i < TIMER_MAX; i++)
	{
        timerArray[i].timerHandle = FREE_TIMER_HANDLE;
	}
#endif
    for (i = 0; i < RTC_TIMER_MAX; i++)
    {
        timerRTCArray[i].timerHandle = FREE_RTC_TIMER_HANDLE;
    }
}


CSerialAPI::~CSerialAPI()
{
}


//////////////////////////////////////////////////////////////////////
// Serial API private helper functions

/*=============================   CommLost   ============================
**	Notification function called when communication with Z-Wave module
**	is lost.
**
**	--- NOTE: Only to be called from APIThreadProc ---
**
**	Application state will be unknown after this function is called as
**	a wait for response from Z-Wave module may have been cancelled without
**	the application code calling Z-Wave module knowing about it.
**
**	Returns:	Nothing
**
**	Parameters:
**	byReason:	IN  Reason for notification (see T_CommNotification)
**--------------------------------------------------------------------------*/
void
CSerialAPI::CommLost(
	BYTE byReason)
{
    if (funcCommErrorNotification != NULL)
    {
        funcCommErrorNotification(byReason);
    }
}


/*=============================   ReceiveData   ============================
**	Receive frame from remote side
**
**	--- NOTE: Only to be called from APIThreadProc ---
**
**	Returns: Receive status
**
**	Parameters:
**	buffer:		IN  Pointer to buffer to store data in
**	len:		OUT length of received data (if any)
**	bStop:		IN	Pointer to stop boolean that should be checked frequently
**					Will be set TRUE when the thread is closing
**--------------------------------------------------------------------------*/
int 
CSerialAPI::ReceiveData(
	unsigned char *buffer, 
	BYTE *len, 
	BOOL *bStop)
{
    int i=0;
    HANDLE h = m_hComPort;
    BYTE ch;

    // Wait for SOF (StartOfFrame) or ACK/NAK
    if (CSerial::serGetChar(h, &ch, bStop) == 0)
    {
        return statusRxTimeout;
    }
    while ( ch != SOF )
    {
        if (ch == ACK || ch == NAK)
        {
            *len = 0;
            debuglog_wr("Received - %s [%01u]", (ch == ACK) ? "ACK" : "NAK", waitForAck);
            if (waitForAck)
            {
                return ch == ACK ? statusFrameSent : statusFrameErr;
            }
        }
        else
        {
            if (ch == CAN)
            {
                ///* We received a CAN - treat it as a timeout but only if we have answered with an ACK */
                debuglog_wr("Received - CAN [%01u]", waitForAck);
                if (waitForAck)
                {
                    if (canRetry)
                    {
                        /* We have been CANned */
                        CSerial::serPutChar(m_hComPort, ACK);	// Tell the world we are happy...
                        debuglog_wr("Transmit - ACK CAN [%02u]", canRetry);
                    }
                    return statusTxCANned;
                }
            }
            else
            {
                debuglog_wr("Received - Garbage %02X", ch);
            }
        }
        if (*bStop)
        {
            return statusExit;
        }
        if (CSerial::serGetChar(h, &ch, bStop) == 0)
        {
            debuglog_wr("Received - RxTimeout SYNC hunt");
            return statusRxTimeout;
        }
    }
    if (CSerial::serGetChar(h, &buffer[i], bStop) == 0)
    {
        debuglog_wr("Received - RxTimeout on Length byte");
        return statusRxTimeout;
    }
    i++;
    BYTE bLen = buffer[0];
    // bLen is length of frame WITHOUT SOF and Checksum
    while (i < bLen + 1)
    {
        if (*bStop)
        {
            return statusExit;
        }
        if (CSerial::serGetChar(h, &buffer[i], bStop) == 0)
        {
            debuglog_wr("Received - RxTimeout inframe byte timeout");
            return statusRxTimeout;
        }
        i++;
    }
    if (CalculateChecksum(buffer, i-1) != buffer[i - 1])
    {
        debuglog_wr("Received - LRC error");
        // Checksum did not match
        CSerial::serPutChar(m_hComPort, NAK);	// Tell the sender something went wrong
        debuglog_wr("Transmit - NAK");
        return statusFrameErr;
    }
    buffer[i - 1]=0;
    if (0 == CSerial::serPutChar(m_hComPort, ACK))
    {
        debuglog_wr("Transmit - ACK retry");
        CSerial::serPutChar(m_hComPort, ACK);
    }
    *len = i - 1;
    debuglog_wr_frame("Received - ", buffer, *len);
    debuglog_wr("Transmit - ACK");
    return statusFrameReceived;
}


/*============================   TransmitData   ============================
**	Transmit frame from remote side
**
**	--- NOTE: Only to be called from APIThreadProc ---
**
**	Side Effect: Sets the waitForAck flag to TRUE
**
**	Returns: Receive status
**
**	Parameters:
**	buffer:		IN  Pointer to data to send
**	length:		IN  length of data
**--------------------------------------------------------------------------*/
void 
CSerialAPI::TransmitData(
	unsigned char *buffer, 
	BYTE length)
{
    // Prefix with SOF and suffix with Checksum before sending
    static unsigned char *wBuf; static BYTE wLen;
    if (buffer != NULL)
    {
        wBuf = buffer;
        wLen = length;
    }
    BYTE byChk = CalculateChecksum(wBuf, wLen);
    CSerial::serPutChar(m_hComPort, SOF);
    CSerial::serWrite(m_hComPort, wBuf, wLen);
    CSerial::serPutChar(m_hComPort, byChk);
    waitForAck = TRUE;
    debuglog_wr_frame("Transmit - ", wBuf, wLen);
}


/*===============================   SendData   ==============================
**	Send request to remote side.
**
**	--- NOTE: Only to be called from Serial API function implementations ---
**
**	Returns:	Nothing
**
**	Parameters:
**	buffer:		IN  Pointer to data frame
**	length:		IN  length of data
**--------------------------------------------------------------------------*/
void 
CSerialAPI::SendData(
	unsigned char *buffer, 
	BYTE length, 
	BYTE *bStatus)
{
    EnterCriticalSection(&CriticalSectionTransmitQueue);
    if (transmitIndex < MAX_TX_QUEUE)
    {
        transmitQueue[transmitIndex].transmitStatus = bStatus;
        transmitQueue[transmitIndex].wLen = length;
        memcpy(&transmitQueue[transmitIndex].wBuf[0], buffer, length);
        transmitIndex++;
    }
    LeaveCriticalSection(&CriticalSectionTransmitQueue);
}


/*===============================   SendData   ==============================
**	Send request to remote side.
**
**	--- NOTE: Only to be called from Serial API function implementations ---
**
**	Returns:	true  - ack was received
**				false - no ack received
**
**	Parameters:
**	buffer:		IN  Pointer to data frame
**	length:		IN  length of data
**--------------------------------------------------------------------------*/
bool
CSerialAPI::SendDataAndWaitForAck(
	unsigned char *buffer, 
	BYTE length)
{
    BYTE bStatus = 0;
    SendData(buffer, length, &bStatus);
    return WaitForAck(&bStatus);
}


/* Returns a new sequence number and increments the global seqNo */
BYTE
SeqNo(void)
{
    seqNo++;
    if (!seqNo)
    {
        seqNo++;
    }
    return seqNo;
}


/*===========================   WaitForResponse   ===========================
**	Wait for response on specified Serial API function ID
**	Will block the thread until a response is received
**
**	--- NOTE: Only to be called from Serial API function implementations ---
**
**	Returns:	true  - ack was received
**				false - no ack received
**
**--------------------------------------------------------------------------*/
bool 
CSerialAPI::WaitForAck(
	BYTE *bStatus)
{
    noAckReceived = false;

    // Wait until ack is received or the transmit thread signal us to stop
    // because all of the transmit retries timed out after 2 minuttes.

    /* Wait for buffer to be send */
    while (*bStatus != FRAME_TRANSMITTING)
    {
        Sleep(0);
    }

    /* Wait for ack */
    while (waitForAck && !noAckReceived)
    {
        Sleep(0);
    }
    if (!noAckReceived)
	{
        return true;
	}
    noAckReceived = false;

    return false;
}


/*======================   SendDataAndWaitForResponse   =====================
**	Send request to remote side and wait for response.
**	Will block the thread until a response is received
**
**	--- NOTE: Only to be called from Serial API function implementations ---
**
**	Returns:	Nothing
**
**	Parameters:
**	buffer:		IN  Pointer to data frame
**	length:		IN  length of data
**  byFunc:		IN  wait for response on this Serial API function ID
**  out:		OUT Pointer to buffer to store received data in
**  byLen:		OUT Length of received data
**--------------------------------------------------------------------------*/
void 
CSerialAPI::SendDataAndWaitForResponse(
	unsigned char *buffer, 
	BYTE length,
	BYTE byFunc, 
	unsigned char *out, 
	BYTE *byLen)
{
    SendData(buffer, length, NULL);
    WaitForResponse(byFunc, out, byLen);
}


/*===========================   WaitForResponse   ===========================
**	Wait for response on specified Serial API function ID
**	Will block the thread until a response is received
**
**	--- NOTE: Only to be called from Serial API function implementations ---
**
**	Returns:	Nothing
**
**	Parameters:
**  byFunc:		IN  wait for response to this Serial API function ID
**  buffer:		OUT Pointer to buffer to store received data in
**  byLen:		OUT Length of received data
**--------------------------------------------------------------------------*/
void
CSerialAPI::WaitForResponse(
	BYTE byFunc, 
	unsigned char *buffer, 
	BYTE *byLen)
{
    BOOL bFound = FALSE;
    int ex = 0;
    LARGE_INTEGER timeOutVal;

    waitForResponse = TRUE;
    TimeOut = FALSE;

    // Wait until response is received or the transmit thread signal us to stop
    // because all of the transmit retries timed out after 2 minuttes.
    timeOutVal = SetTimeOut(500); /* We wait max. 5 sec for the report */
    while (waitForResponse && !TimeOut)
    {
        while ((RecvBuffer[0] == 0) && waitForResponse)
        {
            TimeOut = GetTimeOut(timeOutVal);
            if (TimeOut)
            {
                break;
            }
            Sleep(0);
        }
        if (!TimeOut && waitForResponse && (RecvBuffer[1] == RESPONSE) && (RecvBuffer[2] == byFunc))
        {
            bFound = TRUE;
            waitForResponse = FALSE;
            break;
        }
    }
    waitForResponse = FALSE;
    if (!bFound)
    {
        if (byLen != NULL)
        {
            *byLen = 0;
        }
    }
    else
    {
        if (buffer != NULL)
        {
            memcpy(buffer, RecvBuffer, RecvBuffer[0]);
        }
        if (byLen != NULL)
        {
            *byLen = RecvBuffer[0];
        }
    }
    RecvBuffer[0] = 0;
}


//////////////////////////////////////////////////////////////////////////////
// Function : BOOL GetTimeOut()
// Description:
//
//////////////////////////////////////////////////////////////////////////////
BOOL
CSerialAPI::GetTimeOut(
	LARGE_INTEGER timerVal)
{
  FILETIME ft;
  LARGE_INTEGER timeOutCurrentVal;
  GetSystemTimeAsFileTime(&ft);
  timeOutCurrentVal = *((LARGE_INTEGER*)&ft);
  return ((timerVal.QuadPart <= timeOutCurrentVal.QuadPart) ||
          ((timerVal.QuadPart < 0) && (timeOutCurrentVal.QuadPart >= 0)));
}


//////////////////////////////////////////////////////////////////////////////
// Function : LARGE_INTEGER SetTimeOutTimer()
// Description:
//
//////////////////////////////////////////////////////////////////////////////
LARGE_INTEGER 
CSerialAPI::SetTimeOut(
	WORD timeOut10msec)
{
  FILETIME ft;
  LARGE_INTEGER timeOutVal;
  GetSystemTimeAsFileTime(&ft);
  timeOutVal.QuadPart = ((LARGE_INTEGER)*((LARGE_INTEGER*)&ft)).QuadPart + timeOut10msec*10*10000;
  return timeOutVal;
}


/*=========================   CalculateChecksum   ===========================
**	Calculate checksum for given buffer
**
**	Returns:	Checksum
**
**	Parameters:
**  pData:		IN  Pointer to buffer
**  nLength:	IN  Length of buffer
**--------------------------------------------------------------------------*/
BYTE 
CSerialAPI::CalculateChecksum(
	BYTE *pData, 
	int nLength)
{
    BYTE byChecksum = 0xff;
    int i;
    for (i = 0; i<nLength; nLength--)
    {
        byChecksum ^= *pData++;
    }
    return byChecksum;
}


//////////////////////////////////////////////////////////////////////
// Serial API Receiver Thread and command dispatcher

/*=========================   DispatchThreadProc   =========================
**	Command dispatcher thread.
**	Will continuously look for commands in the commandQueue and serve them
**  as they appears.
**  Will terminate when the m_bEndThread member variable of the CSerialAPI
**  object pointed to by pParam is TRUE
**
**	Returns:	Thread return status
**
**	Parameters:
**  pParam:		IN  Pointer to CSerialAPI object
**--------------------------------------------------------------------------*/
UINT 
DispatchThreadProc(
	LPVOID pParam)
{
    CSerialAPI* pObject = (CSerialAPI*)pParam;
    BYTE buf[BUF_SIZE];
    BYTE length;
    while (!pObject->m_bEndThread)
    {
        if (queueIndex > 0)
        {
            EnterCriticalSection(&CriticalSectionCommandQueue);
            queueIndex--;
            memset(buf,0,sizeof(buf));
            length = commandQueue[0].length;
            memset(buf, 0, sizeof(buf));
            memcpy(buf,commandQueue[0].buf,commandQueue[0].length);

            /* POP the message from the queue (fifo) */
            memcpy((BYTE *)&commandQueue[0], (BYTE *)&commandQueue[1],
                    queueIndex * sizeof(struct commandElement));
            LeaveCriticalSection(&CriticalSectionCommandQueue);

            pObject->Dispatch(buf,length);
        }
        else
        {
            Sleep(2);
        }
    }
    return 0;
}


/*===========================   APIThreadProc   ==============================
**	Main API thread.
**	Its state machine handles the communication with the remote side
**  (Enhanced Z-Wave module). Will continuously look for commands in the commandQueue and serve them
**  as they appears.
**  The thread will continuously monitor the transmitQueue and transmit any
**  requests to the remote side. Futhermore it will continuously monitor if
**  new data frames are received from the remote side, and take appropriate
**  action depending on state and received data.
**
**  Will terminate when the m_bEndThread member variable of the CSerialAPI
**  object pointed to by pParam is TRUE
**
**	Returns:	Thread return status
**
**	Parameters:
**  pParam:		IN  Pointer to CSerialAPI object
**--------------------------------------------------------------------------*/
UINT 
APIThreadProc(
	LPVOID pParam)
{
    CSerialAPI* pObject = (CSerialAPI*)pParam;
    unsigned char buf[4100];
    BYTE length;
    BYTE status;

    for (;;)
    {
        if (pObject->m_bEndThread)
        {
            return 0;
        }

        /* If there is something in the transmitQueue and we are not waiting */
        /* for acknowledge, send it */
        if ((transmitIndex > 0) && !waitForAck)
        {
            if (transmitQueue[0].transmitStatus != NULL)
            {
				*(transmitQueue[0].transmitStatus) = FRAME_TRANSMITTING;
            }
            pObject->TransmitData(transmitQueue[0].wBuf, transmitQueue[0].wLen);
            // Have set waitforack
        }

        length = 0;
        status = pObject->ReceiveData(buf, &length, &pObject->m_bEndThread);
        if (pObject->m_bEndThread)
        {
            return 0;
        }
        switch (status)
        {
            case statusFrameReceived:
                {
                    // ReadTelegram hangs until a char is received or timeout occurs.
                    // We need to check if the trace has been stopped in the meantime.

                    if (!pObject->m_bEndThread && length > 0)
                    {
                        if (buf[1] == REQUEST)
                        {
                            // Call command dispatcher
                            EnterCriticalSection(&CriticalSectionCommandQueue);

                            if (queueIndex < QUEUE_MAX)
                            {
                                memcpy(commandQueue[queueIndex].buf,buf,length);
                                commandQueue[queueIndex].length = length;
                                queueIndex++;
                                /* Signal the arrival of a new command */
                            }
                            LeaveCriticalSection(&CriticalSectionCommandQueue);
                        }
                        else if (buf[1] == RESPONSE)
                        {
                            // Put in queue
                            memcpy(&RecvBuffer[1], &buf[1], length-1);
                            RecvBuffer[0] = buf[0];
                        }
                    }
                    Sleep(1);   /* Release CPU, so we do not use all the resources! */
                }
                break;

            case statusFrameSent:
                /* Remove request from transmitQueue if an ACK (FrameSent) */
                /* is received and we are waiting for ACK */
                /* If ACK is received and we are not waiting for ACK we just */
                /* ignore the ACK. This may happen during retransmissions */
                /* caused by the Z-Wave code delaying answers - longer pause */
                /* between calls to ApplicationPoll */
                /* Example: */
                /* After compl.handler on ZW_SetLearnNodeState(DELETE) is */
                /* called the PC appl will normally call ZW_SetLearnNodeState(OFF). */
                /* At this point the Z-Wave code is busy with EEPROM operations */
                /* and will not call ApplicationPoll until done with that. */
                /* All retries may exceed before this is done */
                if (waitForAck)
                {
                    EnterCriticalSection(&CriticalSectionTransmitQueue);
                    transmitIndex--;
                    memcpy((BYTE *)&transmitQueue[0], (BYTE *)&transmitQueue[1],
                           transmitIndex * sizeof(struct _transmit_element_));
                           LeaveCriticalSection(&CriticalSectionTransmitQueue);
                    waitForAck = FALSE;
                    retry = 0;
                    canRetry = 0;
                }
                break;

            case statusFrameErr:
            case statusRxTimeout:
            case statusTxCANned:
                if (waitForAck)
                {
                    if (retry == 0)
                    {
                        retryDelay = 0;
                    }
                    /* retryDelay is used to delay the last retry a couple of seconds. */
                    if (retry == MAX_SERIAL_RETRY - 1)
                    {
                        retryDelay++;
                        if (retryDelay < 2000/SER_TIMEOUT)
						{
                            break;
						}
                    }
                    if (((status == statusTxCANned) && (canRetry < MAX_SERIAL_RETRY)) || (retry++ < MAX_SERIAL_RETRY))
                    {
                        if (status == statusTxCANned)
                        {
                            canRetry++;
                        }
                        else
                        {
                            /* Not a CAN retry */
                            canRetry = 0;;
                        }
                        pObject->TransmitData(NULL, 0);	// Retry...
                    }
                    else
                    {

                        retry = canRetry = 0;
                        EnterCriticalSection(&CriticalSectionTransmitQueue);
                        transmitIndex--;
                        memcpy((BYTE *)&transmitQueue[0], (BYTE *)&transmitQueue[1],
                               transmitIndex * sizeof(struct _transmit_element_));
                        LeaveCriticalSection(&CriticalSectionTransmitQueue);
                        noAckReceived = true;
                        waitForAck = FALSE;
                        // Signal WaitForResponse() that it has to stop waiting before
                        // calling CommLost. WaitForResponse blocks the User Interface thread
                        // and has to be stopped for the application to do any GUI stuff in
                        // the callback from CommLost
                        waitForResponse = FALSE;
                    }
                }
                else if ((pObject->learnState) && (pObject->TimeOut))
                {
                    waitForAck = FALSE;
                    waitForResponse = FALSE;
                    pObject->CommLost(CSerialAPI::COMM_NO_RESPONSE);
                }
                break;

            default:
                break;
        }
        Sleep(2);   /* Release CPU, so we do not use all the resources! */
    }
    return 0;   // thread completed successfully
}


/*==============================   Dispatch   ==============================
**	Handles requests (callback, RTC timer call and ApplicationCommandHandler)
**  received from remote side. Responses received from remote side is not
**  handled by this function.
**
**	Returns:	Nothing
**
**	Parameters:
**  pData:		IN  Pointer to data buffer
**  byLen:		IN  Length of data buffer
**--------------------------------
------------------------------------------*/
void 
CSerialAPI::Dispatch(
	unsigned char *pData, 
	BYTE bylen)
{
    int i;

    switch(pData[IDX_CMD])
    {
        case FUNC_ID_ZW_APPLICATION_CONTROLLER_UPDATE:
            {
                BYTE *pCmd = new BYTE[pData[IDX_DATA + 2]];
                for (i = 0; i < pData[IDX_DATA + 2]; i++)
                {
                    pCmd[i] = pData[IDX_DATA + 3 + i];
                }
                if (cbIdleLearnNode)
                {
                    cbIdleLearnNode(pData[IDX_DATA],pData[IDX_DATA + 1], pCmd, pData[IDX_DATA + 2]);
                }
                delete pCmd;
            }
            break;

        case FUNC_ID_APPLICATION_COMMAND_HANDLER:
            /* ZW->PC: REQ | 0x04 | rxStatus | sourceNode | */
            /*                      cmdLength | pCmd[] */
            {
                BYTE *pCmd = new BYTE[pData[IDX_DATA + 2]];
                for (i = 0; i < pData[IDX_DATA+2]; i++)
                {
                    pCmd[i] = pData[IDX_DATA + 3 + i];
                }
                ASSERT(funcApplicationCommandHandler != NULL);
                funcApplicationCommandHandler(pData[IDX_DATA], pData[IDX_DATA + 1], pCmd, pData[IDX_DATA + 2]);
                delete pCmd;
            }
            break;

        case FUNC_ID_APPLICATION_COMMAND_HANDLER_BRIDGE:
            /* ZW->PC: REQ | 0xA8 | rxStatus | destNode | sourceNode | */
            /*                      cmdLength | pCmd[] | multiDestsOffset_NodeMaskLen | */
            /*                      multiDestsNodeMask[] */
            {
                BYTE cmdLength = pData[IDX_DATA + 3];
                BYTE multiDestsLength = pData[IDX_DATA + 4 + cmdLength];
                BYTE *pCmd = new BYTE[cmdLength];
                BYTE *pMultiDests = new BYTE[multiDestsLength + 1];
                for (i = 0; i < cmdLength; i++)
                {
                    pCmd[i] = pData[IDX_DATA + 4 + i];
                }
                if (multiDestsLength)
                {
                    pMultiDests[0] = pData[IDX_DATA + 4 + cmdLength];
                    for (i = 0; i < multiDestsLength; i++)
                    {
                        pMultiDests[1 + i] = pData[IDX_DATA + 4 + cmdLength + 1 + i];
                    }
                }
                else
                {
                    delete pMultiDests;
                    pMultiDests = NULL;
                }
                ASSERT(funcApplicationCommandHandler_Bridge != NULL);
                funcApplicationCommandHandler_Bridge(pData[IDX_DATA], pData[IDX_DATA + 1], pData[IDX_DATA + 2],
                                                     (multiDestsLength == 0) ? NULL : pMultiDests, pCmd, cmdLength);
                delete pMultiDests;
                delete pCmd;
            }
            break;

        // The rest are callback functions
        case FUNC_ID_ZW_SEND_NODE_INFORMATION:
            if (cbFuncZWSendNodeInformation != NULL)
            {
                cbFuncZWSendNodeInformation(pData[IDX_DATA + 1]);
            }
            break;

        case FUNC_ID_ZW_SEND_TEST_FRAME:
            if (cbFuncZWSendTestFrame != NULL)
            {
				cbFuncZWSendTestFrame(pData[IDX_DATA + 1]);
            }
            break;

		case FUNC_ID_ZW_SEND_DATA:
			if (cbFuncZWSendData != NULL)
			{
				memset(&sGlobalTxInfo, 0, sizeof(sGlobalTxInfo));
				if (bylen > 7)
				{
					/* Copy detailed tx info from serial buffer to sGlobalTxInfo struct */
					BYTE j = IDX_DATA + 2;
					sGlobalTxInfo.wTime = pData[j] * 256 + pData[j+1]; /* txt timing */
					j += 2;
					sGlobalTxInfo.bRepeaters = pData[j++]; /* repeater count */
					sGlobalTxInfo.rssiVal = &(pData[j]); /* rssi values [5] */
					j += 5;
					sGlobalTxInfo.bACKChannelNo = pData[j++];
					sGlobalTxInfo.bLastTxChannelNo = pData[j++];
					sGlobalTxInfo.bRouteScheme = pData[j++];
					sGlobalTxInfo.pLastUsedRoute = &(pData[j]); /* Last used route repeaters [4] */
					j += 4;
					sGlobalTxInfo.bRouteTries = pData[j++]; 
					sGlobalTxInfo.bLastFailedLinkFrom = pData[j++];
					sGlobalTxInfo.bToLastFailedLinkTo = pData[j++];
					cbFuncZWSendData(pData[IDX_DATA + 1], &sGlobalTxInfo); 
				}
				else if (bylen > 5)
				{
					sGlobalTxInfo.wTime = pData[IDX_DATA + 2] * 256 + pData[IDX_DATA + 3]; /* txt timing */
					sGlobalTxInfo.bRepeaters = 0xFF; /* repeater count not available*/
					cbFuncZWSendData(pData[IDX_DATA + 1], &sGlobalTxInfo); 
				}
				else
				{
					sGlobalTxInfo.bRepeaters = 0xFF; /* repeater count not available*/
					cbFuncZWSendData(pData[IDX_DATA + 1], &sGlobalTxInfo);
				}
			}
			break;

        case FUNC_ID_ZW_SEND_DATA_META:
            if (cbFuncZWSendDataMeta != NULL)
            {
              cbFuncZWSendDataMeta(pData[IDX_DATA + 1]);
            }
            break;

        case FUNC_ID_ZW_SEND_DATA_MULTI:
            if (cbFuncZWSendDataMulti != NULL)
            {
                cbFuncZWSendDataMulti(pData[IDX_DATA + 1]);
            }
            break;

        case FUNC_ID_ZW_SEND_DATA_MR:
            if (cbFuncZWSendDataMR != NULL)
            {
              cbFuncZWSendDataMR(pData[IDX_DATA+1]);
            }
            break;

        case FUNC_ID_ZW_SEND_DATA_META_MR:
            if (cbFuncZWSendDataMetaMR != NULL)
            {
              cbFuncZWSendDataMetaMR(pData[IDX_DATA+1]);
            }
            break;

        case FUNC_ID_ZW_SEND_DATA_BRIDGE:
            if (cbFuncZWSendData_Bridge != NULL)
            {
                cbFuncZWSendData_Bridge(pData[IDX_DATA+1]);
            }
            break;

        case FUNC_ID_ZW_SEND_DATA_META_BRIDGE:
            if (cbFuncZWSendDataMeta_Bridge != NULL)
            {
                cbFuncZWSendDataMeta_Bridge(pData[IDX_DATA+1]);
            }
            break;

        case FUNC_ID_ZW_SEND_DATA_MULTI_BRIDGE:
            if (cbFuncZWSendDataMulti_Bridge != NULL)
            {
                cbFuncZWSendDataMulti_Bridge(pData[IDX_DATA+1]);
            }
            break;

#ifdef TIMER_SUPPORT
        case FUNC_ID_TIMER_CALL:
            {
              if (timerArray[pData[IDX_DATA]].func != NULL)
                timerArray[pData[IDX_DATA]].func();
            }
            break;
#endif

        case FUNC_ID_RTC_TIMER_CALL:
            {
               if (timerRTCArray[pData[IDX_DATA]].func != NULL)
               {
                   timerRTCArray[pData[IDX_DATA]].func(pData[IDX_DATA+1],pData[IDX_DATA+2]);
               }
            }
            break;

        case FUNC_ID_RTC_TIMER_CREATE:
            if (cbFuncRTCTimerCreate != NULL)
            {
                cbFuncRTCTimerCreate();
            }
            break;

        case FUNC_ID_MEMORY_PUT_BUFFER:
            if (cbFuncMemoryPutBuffer != NULL)
			{
                cbFuncMemoryPutBuffer();
            }
            break;

        case FUNC_ID_ZW_SET_DEFAULT:
            if (cbFuncZWSetDefault != NULL)
            {
                cbFuncZWSetDefault();
            }
            break;

        case FUNC_ID_ZW_CONTROLLER_CHANGE: 
        case FUNC_ID_ZW_CREATE_NEW_PRIMARY:
        case FUNC_ID_ZW_REMOVE_NODE_FROM_NETWORK:
        case FUNC_ID_ZW_ADD_NODE_TO_NETWORK:
            if (cbFuncZWNewController != NULL)
            {
                BYTE *pCmd = new BYTE[pData[IDX_DATA + 3]];
                for (i = 0; i < pData[IDX_DATA + 3]; i++)
                {
                    pCmd[i] = pData[IDX_DATA + 4 + i];
                }
                cbFuncZWNewController(pData[IDX_DATA + 1], pData[IDX_DATA + 2], pCmd, pData[IDX_DATA + 3]);
                delete pCmd;
            }
            break;

        case FUNC_ID_ZW_REPLICATION_SEND_DATA:
            if (cbFuncZWReplicationSendData != NULL)
            {
                cbFuncZWReplicationSendData(pData[IDX_DATA + 1]);
            }
            break;

        case FUNC_ID_ZW_ASSIGN_RETURN_ROUTE:
            if (cbFuncZWAssignReturnRoute != NULL)
            {
                cbFuncZWAssignReturnRoute(pData[IDX_DATA + 1]);
            }
            break;

        case FUNC_ID_ZW_DELETE_RETURN_ROUTE:
            if (cbFuncZWDeleteReturnRoute != NULL)
            {
                cbFuncZWDeleteReturnRoute(pData[IDX_DATA + 1]);
            }
            break;

        case FUNC_ID_ZW_ASSIGN_SUC_RETURN_ROUTE:
            if (cbFuncZWAssignSUCReturnRoute != NULL)
            {
                cbFuncZWAssignSUCReturnRoute(pData[IDX_DATA + 1]);
            }
            break;

        case FUNC_ID_ZW_DELETE_SUC_RETURN_ROUTE:
            if (cbFuncZWDeleteSUCReturnRoute != NULL)
            {
                cbFuncZWDeleteSUCReturnRoute(pData[IDX_DATA + 1]);
            }
            break;

        case FUNC_ID_ZW_REQUEST_NETWORK_UPDATE:
            if (cbFuncZWRequestNetworkUpdate != NULL)
            {
                cbFuncZWRequestNetworkUpdate(pData[IDX_DATA + 1]);
            }
            break;

        case FUNC_ID_ZW_SET_LEARN_MODE:
            if (cbFuncZWSetLearnMode != NULL)
            {
                BYTE *pCmd = NULL;
                BYTE len = (bylen >= IDX_DATA + 3 + 1) ? pData[IDX_DATA+3] : 0;
                if (len)
                {
                    pCmd = new BYTE[len];
                    for (i = 0; i < pData[IDX_DATA + 3]; i++)
                    {
                        pCmd[i] = pData[IDX_DATA + 4 + i];
                    }
                }
                cbFuncZWSetLearnMode(pData[IDX_DATA + 1], pData[IDX_DATA + 2], pCmd, len);
            }
            break;

        case FUNC_ID_ZW_SET_SUC_NODE_ID:
            if (cbFuncZWSetSUCNodeID != NULL)
            {
                cbFuncZWSetSUCNodeID(pData[IDX_DATA + 1]);
            }
            break;

        case FUNC_ID_ZW_SEND_SUC_ID:
            if (cbFuncZWSendSUCID != NULL)
            {
                cbFuncZWSendSUCID(pData[IDX_DATA + 1]);
            }
            break;

        case FUNC_ID_ZW_REPLACE_FAILED_NODE:
            if (cbFuncZWReplaceFailedNode != NULL)
            {
                cbFuncZWReplaceFailedNode(pData[IDX_DATA + 1]);
            }
            break;

        case FUNC_ID_ZW_REMOVE_FAILED_NODE_ID:
            if (cbFuncZWRemoveFailedNode != NULL)
            {
                cbFuncZWRemoveFailedNode(pData[IDX_DATA + 1]);
            }
            break;

		case FUNC_ID_ZW_REQUEST_NODE_NEIGHBOR_UPDATE:
            if (cbFuncZWRequestNodeNodeNeighborUpdate != NULL)
            {
                cbFuncZWRequestNodeNodeNeighborUpdate(pData[IDX_DATA + 1]);
            }
            break;

		case FUNC_ID_ZW_REQUEST_NODE_NEIGHBOR_UPDATE_OPTION:
            if (cbFuncZWRequestNodeNodeNeighborUpdate_Option != NULL)
            {
                cbFuncZWRequestNodeNodeNeighborUpdate_Option(pData[IDX_DATA + 1]);
            }
            break;
    }
}


//////////////////////////////////////////////////////////////////////
// Serial API Initializer

/*=============================   Initialize   =============================
**	Initialize the Serial API.
**  This includes: opening the com port and starting the two Serial API
**  main threads.
**
**	Returns:	TRUE if initialization succeeded, FALSE if not (eg. unable
**              to open com port)
**
**	Parameters:
**  byPort:		IN  Com port to open. 1 for COM1, 2 for COM2 etc.
**	bySpeed:	IN	Port speed: SPEED_115200 = 115200 baud, SPEED_57600, SPEED_19200
**  funcApplCmdHandler:		IN  ApplicationCommandHandler to be provided
**                          by application. Called when commands are received
**							on RF. Refer to Z-Wave API for further details.
**  funcCommError:			IN CommunicationErrorNotification to be provided
**							by application. Called when communication with
**							Z-Wave module is lost.
**--------------------------------------------------------------------------*/
BOOL CSerialAPI::Initialize(
  BYTE byPort,
  BYTE bySpeed,
  void(*funcApplCmdHandler)(BYTE,BYTE,BYTE*,BYTE),
  void(*funcApplCmdHandler_Bridge)(BYTE,BYTE,BYTE,BYTE*,BYTE*,BYTE),
  void(*funcCommError)(BYTE))
{
    char pComPort[10];

    ASSERT(funcApplCmdHandler != NULL);
    ASSERT(funcApplCmdHandler_Bridge != NULL);
    ASSERT(funcCommError != NULL);
    if (funcApplCmdHandler == NULL || funcCommError == NULL)
	{
        return FALSE;
	}

    funcApplicationCommandHandler = funcApplCmdHandler;
    funcApplicationCommandHandler_Bridge = funcApplCmdHandler_Bridge;
    funcCommErrorNotification = funcCommError;

    waitForAck = FALSE;
    learnState = FALSE;
    TimeOut = FALSE;

    retry = 0;

    sprintf_s(pComPort,"\\\\.\\com%d",byPort);
    switch (bySpeed)
	{
        case SPEED_115200:
            bySpeed = CSerial::serial_115200_8n1;
            break;

        case SPEED_57600:
            bySpeed = CSerial::serial_57600_8n1;
            break;

        case SPEED_19200:
            bySpeed = CSerial::serial_19200_8n1;
            break;

        case SPEED_9600:
            bySpeed = CSerial::serial_9600_8n1;
            break;

		default:
            bySpeed = CSerial::serial_115200_8n1;
            break;
    }
    /*Only start a new thread if we dont have one open already*/
    if (m_hComPort== NULL)
    {
        m_hComPort = CSerial::serOpen(pComPort, bySpeed);
        if (m_hComPort)
        {
            m_bEndThread = FALSE;
            InitializeCriticalSection(&CriticalSectionCommandQueue);
            InitializeCriticalSection(&CriticalSectionTransmitQueue);
            transmitIndex = 0;
            queueIndex = 0;

#ifdef _CONSOLE
			hThreadAPI = (HANDLE)_beginthreadex(NULL, 0, (unsigned int(__stdcall *)(void *))&APIThreadProc, this, 0, &threadAPIID );
			hThreadDispatch = (HANDLE)_beginthreadex( NULL, 0, (unsigned int(__stdcall *)(void *))&DispatchThreadProc, this, 0, &threadDispatchID );
#else
            AfxBeginThread((AFX_THREADPROC)APIThreadProc, this);
            AfxBeginThread((AFX_THREADPROC)DispatchThreadProc, this);
#endif
            CSerial::serPutChar(m_hComPort, ACK);	// Tell the world we are happy...
        }
    }
    return (m_hComPort != NULL);
}


/*===========================   Shutdown   ===========================
**  Shuts down the Serial API. Must be called before the destructor
**	That is; ends the API threads and closes the com port
**
**  Returns:		Nothing
**
**  Parameters		None
**--------------------------------------------------------------------------*/
void
CSerialAPI::Shutdown()
{
    m_bEndThread = TRUE;

    if (m_hComPort)
    {
        ::PurgeComm(m_hComPort, PURGE_TXABORT | PURGE_RXABORT);
        CSerial::serClose(m_hComPort);
        m_hComPort = NULL;
    }
}


/*============================== BitMask_isBitSet ============================
** Function description
**      Check if a bBitNo is set/high/1 in the specified pabBitMask
**
** Returns: true if bBitNo IS set in pabBitMask
**			false if bBitNo NOT set in pabBitMask
**
** Side effects:
**		
**--------------------------------------------------------------------------*/
bool
CSerialAPI::BitMask_isBitSet(
  BYTE* pabBitMask,
  BYTE bBitNo)
{
  return (0 != (*(pabBitMask + ((bBitNo - 1) >> 3)) >> (((bBitNo - 1) & 7)) & 0x01));
}



//////////////////////////////////////////////////////////////////////
// Serial API

/*===============================   ZW_SetRFReceiveMode   ===================
**    Initialize the Z-Wave RF chip.
**    Mode on:  Set the RF chip in receive mode and starts the data sampling.
**    Mode off: Set the RF chip in power down mode.
**
**    Returns:	Nothing
**
**    Parameters:
**    mode:		IN  TRUE: On; FALSE: Off mode
**--------------------------------------------------------------------------*/
void
CSerialAPI::ZW_SetRFReceiveMode(
	BYTE mode)
{
    BYTE idx=0;
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_SET_RF_RECEIVE_MODE;
    buffer[idx++] = mode;
    buffer[0] = idx;	// length
    SendData(buffer, idx, NULL);
}


/*=========================   SerialAPI_PowerManagement   ====================
**    HOST Power Management configuration.
**
**    Returns:	Nothing
**
**    Parameters:
**--------------------------------------------------------------------------*/
void 
CSerialAPI::SerialAPI_PowerManagement(
	BYTE bPM_Cmd, 
	BYTE cmdDataLength, 
	BYTE *pCmdData)
{
    BYTE idx = 0;
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_SERIAL_API_POWER_MANAGEMENT;
    buffer[idx++] = bPM_Cmd;
    for (int i = 0; i < cmdDataLength; i++)
    {
        buffer[idx++] = pCmdData[i];
    }
    buffer[0] = idx;
    printf("Data transmitted on Serial Line: ");
    for (int i = 0; i < idx; i++)
    {
        printf("%02X", buffer[i]);
    }
    printf("\n");
    SendData(buffer, idx, NULL);
}


void 
CSerialAPI::SerialAPI_PowerManagement_PoweredUp(
	BYTE bIOPin)
{
    static BYTE buf[1];

    buf[0] = bIOPin;
    SerialAPI_PowerManagement(PM_PIN_UP_CONFIGURATION_CMD, 1, buf);
}


void
CSerialAPI::SerialAPI_PowerManagement_Mode_Configuration(
	BYTE bNumberOfPins, 
	BYTE *bIOPin, 
	BYTE *bIOPinLevel)
{
    static BYTE buf[1 + (PM_IO_PIN_MAX << 1)];

    buf[0] = (bNumberOfPins <= PM_IO_PIN_MAX) ? bNumberOfPins : PM_IO_PIN_MAX;
    for (int i = 0; i < buf[0]; i++)
    {
        buf[1 + (i << 1)] = bIOPin[i];
        buf[2 + (i << 1)] = bIOPinLevel[i];
    }
    SerialAPI_PowerManagement(PM_MODE_CONFIGURATION_CMD, (1 + (buf[0] << 1)), buf);
}


void 
CSerialAPI::SerialAPI_PowerManagement_Set_PowerMode_Configuration(
	BYTE bNumberOfPins, 
	BYTE *bIOPin, 
	BYTE *bIOPinLevel)
{
    static BYTE buf[1 + (PM_IO_PIN_MAX << 1)];

    buf[0] = (bNumberOfPins <= PM_IO_PIN_MAX) ? bNumberOfPins : PM_IO_PIN_MAX;
    for (int i = 0; i < buf[0]; i++)
    {
        buf[1 + (i << 1)] = bIOPin[i];
        buf[2 + (i << 1)] = bIOPinLevel[i];
    }
    SerialAPI_PowerManagement(PM_SET_POWER_MODE_CMD, (1 + (buf[0] << 1)), buf);
}


void
CSerialAPI::SerialAPI_PowerManagement_PowerUpOn_ZWCommand_Configuration(
	BYTE bMatchMode, 
	BYTE bNumberOfMatchBytes, 
	BYTE *bWakeUpValue, 
	BYTE *bWakeUpMask)
{
    static BYTE buf[1 + (PM_WAKEUP_MAX_BYTES << 1)];

    buf[0] = bMatchMode;
    buf[1] = (bNumberOfMatchBytes <= PM_WAKEUP_MAX_BYTES) ? bNumberOfMatchBytes : PM_WAKEUP_MAX_BYTES;
    for (int i = 0; i < buf[1]; i++)
    {
        buf[2 + i] = bWakeUpValue[i];
        buf[2 + buf[1] + i] = bWakeUpMask[i];
    }
    SerialAPI_PowerManagement(PM_POWERUP_ZWAVE_CONFIGURATION_CMD, (2 + (buf[1] << 1)), buf);
}


void
CSerialAPI::SerialAPI_PowerManagement_PowerUpOn_Timer_Configuration(
	BYTE bTimerResolution, 
	WORD bTimerCount)
{
    static BYTE buf[3];

    buf[0] = bTimerResolution;
    buf[1] = (BYTE)((bTimerCount >> 8) & 0xFF);
    buf[2] = (BYTE)(bTimerCount & 0xFF);
    SerialAPI_PowerManagement(PM_POWERUP_TIMER_CONFIGURATION_CMD, 3, buf);
}


void 
CSerialAPI::SerialAPI_PowerManagement_PowerUpOn_External_Configuration(
	BYTE bIOPin, 
	BYTE bLevel)
{
    static BYTE buf[2];

    buf[0] = bIOPin;
    buf[1] = bLevel;
    SerialAPI_PowerManagement(PM_POWERUP_EXTERNAL_CONFIGURATION_CMD, 2, buf);
}


/*==================   SerialAPI_PowerManagement_GetStatus   =================
**    Get HOST Power Management Status.
**
**    Returns:	Power Management status
**
**    Parameters:
**--------------------------------------------------------------------------*/
BYTE 
CSerialAPI::SerialAPI_PowerManagement_GetStatus(void)
{
    static BYTE buffer[BUF_SIZE];
    BYTE idx=0;
    BYTE byLen = 0;

    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_SERIAL_API_POWER_MANAGEMENT;
    buffer[idx++] = PM_GET_STATUS;
    buffer[0] = idx;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_SERIAL_API_POWER_MANAGEMENT, buffer, &byLen);

    if (byLen > IDX_DATA)
    {
        return buffer[IDX_DATA];
    }
    else
    {
        /* No answer */
        return FALSE;
    }
}


/*========================   ZW_SendNodeInformation   ======================
**    Create and transmit a node informations frame
**
**    Returns:		Nothing
**
**    Parameters:
**    destNode      IN  Destination Node ID
**    txOptions     IN  Transmit option flags
**    completedFunc IN  Transmit completed call back function
**--------------------------------------------------------------------------*/
void CSerialAPI::ZW_SendNodeInformation(
	BYTE destNode, 
	BYTE txOptions, 
	void (__cdecl *completedFunc)(BYTE))
{
    BYTE idx=0;
    BYTE byCompletedFunc = (completedFunc == NULL? 0: 0x43);
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_SEND_NODE_INFORMATION;
    buffer[idx++] = destNode;
    buffer[idx++] = txOptions;
    buffer[idx++] = byCompletedFunc;	// Func id for CompletedFunc
    buffer[0] = idx;	// length
    BYTE byLen = 0;
    cbFuncZWSendNodeInformation = completedFunc;
    SendData(buffer, idx, NULL);
}


/*=========================   ZW_RF_Power_Level_Set   ========================
**    Set current RF Powerlevel setting on Z-Wave SerialAPI module
**
**    Returns:		Powerlevel set
**
**    Parameters:
**--------------------------------------------------------------------------*/
BYTE
CSerialAPI::ZW_RF_Power_Level_Set(
    BYTE bPowerLevel)
{
    static BYTE buffer[BUF_SIZE];
    BYTE retVal = FALSE;
    BYTE idx = 0;
    BYTE byLen = 0;

    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_RF_POWER_LEVEL_SET;
    buffer[idx++] = bPowerLevel;
    buffer[0] = idx;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_ZW_RF_POWER_LEVEL_SET, buffer, &byLen);

    if (byLen > IDX_DATA)
    {
        retVal = buffer[IDX_DATA];
    }
    return retVal;
}


/*=========================   ZW_RF_Power_Level_Get   ========================
**    Get current RF Powerlevel setting from Z-Wave SerialAPI module
**
**    Returns:		Powerlevel setting
**
**    Parameters:
**--------------------------------------------------------------------------*/
BYTE
CSerialAPI::ZW_RF_Power_Level_Get()
{
    static BYTE buffer[BUF_SIZE];
    BYTE retVal = FALSE;
    BYTE idx = 0;
    BYTE byLen = 0;

    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_RF_POWER_LEVEL_GET;
    buffer[0] = idx;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_ZW_RF_POWER_LEVEL_GET, buffer, &byLen);

    if (byLen > IDX_DATA)
    {
        retVal = buffer[IDX_DATA];
    }
    return retVal;
}


/*===========================   ZW_SendTestFrame   ===========================
**    Send test frame to destination node with current powerlevel
**
**    Returns:		FALSE if transmitter queue overflow or no answer
**
**    Parameters:
**    nodeID          IN  Destination node ID
**    completedFunc	  IN  Transmit completed call back function
**--------------------------------------------------------------------------*/
BYTE
CSerialAPI::ZW_SendTestFrame(
    BYTE nodeID,
    BYTE bPowerLevel,
    void (__cdecl *completedFunc)(BYTE))
{
    BYTE retVal = FALSE;
    BYTE byLen = 0;
    BYTE idx = 0;
    BYTE byCompletedFunc = (completedFunc == NULL? 0: SeqNo());
    static BYTE buffer[BUF_SIZE];

    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_SEND_TEST_FRAME;
    buffer[idx++] = nodeID;
    buffer[idx++] = bPowerLevel;
    buffer[idx++] = byCompletedFunc;	// Func id for CompletedFunc
    buffer[0] = idx;	// length
    cbFuncZWSendTestFrame = completedFunc;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_ZW_SEND_TEST_FRAME, buffer, &byLen);
    if (byLen > IDX_DATA)
    {
        retVal = buffer[IDX_DATA];
    }
    return retVal;
}


/*===============================   ZW_SendData   ===========================
**    Transmit data buffer to a single ZW-node or all ZW-nodes (broadcast).
**
**
**    Returns:		FALSE if transmitter queue overflow
**
**    Parameters:
**    nodeID          IN  Destination node ID (0xFF == broadcast)
**    pData           IN  Data buffer pointer
**    dataLength      IN  Data buffer length
**    txOptions       IN  Transmit option flags
**    completedFunc	  IN  Transmit completed call back function
**--------------------------------------------------------------------------*/
BYTE CSerialAPI::ZW_SendData(
	BYTE nodeID, 
	BYTE *pData, 
	BYTE dataLength, 
	BYTE txOptions, 
	void (__cdecl *completedFunc)(BYTE, TX_STATUS_TYPE *))
{
    int i;
    BYTE idx=0;
    BYTE byCompletedFunc = (completedFunc == NULL? 0: SeqNo());
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_SEND_DATA;
    buffer[idx++] = nodeID;
    buffer[idx++] = dataLength;
    for (i = 0; i < dataLength; i++)
    {
        buffer[idx++] = pData[i];
    }
    buffer[idx++] = txOptions;
    buffer[idx++] = byCompletedFunc;	// Func id for CompletedFunc
    buffer[0] = idx;	// length
    BYTE byLen = 0;
    cbFuncZWSendData = completedFunc;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_ZW_SEND_DATA, buffer, &byLen);

    if (byLen > IDX_DATA)
    {
        return buffer[IDX_DATA];
    }
    else
    {
        /* No answer */
        return FALSE;
    }
}


/*=============================   ZW_SendDataMeta   =========================
**    Transmit data buffer to a single ZW-node or all ZW-nodes (broadcast).
**
**
**    Returns:		FALSE if transmitter queue overflow
**
**    Parameters:
**    nodeID          IN  Destination node ID (0xFF == broadcast)
**    pData           IN  Data buffer pointer
**    dataLength      IN  Data buffer length
**    txOptions       IN  Transmit option flags
**    completedFunc	  IN  Transmit completed call back function
**--------------------------------------------------------------------------*/
BYTE CSerialAPI::ZW_SendDataMeta(
	BYTE nodeID, 
	BYTE *pData, 
	BYTE dataLength, 
	BYTE txOptions, 
	void (__cdecl *completedFunc)(BYTE))
{
    int i;
    BYTE idx=0;
    BYTE byCompletedFunc = (completedFunc == NULL? 0: SeqNo());
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_SEND_DATA_META;
    buffer[idx++] = nodeID;
    buffer[idx++] = dataLength;
    for (i=0;i<dataLength;i++)
    {
        buffer[idx++] = pData[i];
    }
    buffer[idx++] = txOptions;
    buffer[idx++] = byCompletedFunc;	// Func id for CompletedFunc
    buffer[0] = idx;	// length
    BYTE byLen = 0;
    cbFuncZWSendDataMeta = completedFunc;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_ZW_SEND_DATA_META, buffer, &byLen);

    return buffer[IDX_DATA];
}


/*===============================   ZW_SendDataMulti   ======================
**    Transmit data buffer to a list of Z-Wave Nodes (multicast frame).
**
**    Returns:			RET  FALSE if transmitter queue overflow
**
**    Parameters:
**    pNodeIDList	IN  List of destination node ID's
**    numberNodes	IN  Number of Nodes
**    pData			IN  Data buffer pointer
**    dataLength	IN  Data buffer length
**    txOptions	    IN  Transmit option flags
**    completedFunc	IN  Transmit completed call back function
**--------------------------------------------------------------------------*/
BYTE
CSerialAPI::ZW_SendDataMulti(
	BYTE *pNodeIDList, 
	BYTE numberNodes, 
	BYTE *pData, 
	BYTE dataLength, 
	BYTE txOptions, 
	void (*completedFunc)(BYTE txStatus))
{
    int i;
    BYTE idx=0;
    BYTE byCompletedFunc = (completedFunc == NULL? 0: SeqNo());
    static BYTE buffer[BUF_SIZE];

    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_SEND_DATA_MULTI;
    numberNodes = (numberNodes <= ZW_MAX_NODES_IN_MULTICAST) ? numberNodes : ZW_MAX_NODES_IN_MULTICAST;
    buffer[idx++] = numberNodes;
    for (i = 0; i < numberNodes; i++)
    {
        buffer[idx++] = pNodeIDList[i];
    }
    buffer[idx++] = dataLength;
    for ( i = 0; i < dataLength; i++)
    {
        buffer[idx++] = pData[i];
    }
    buffer[idx++] = txOptions;
    buffer[idx++] = byCompletedFunc;	// Func id for CompletedFunc
    buffer[0] = idx;	// length
    BYTE byLen = 0;
    cbFuncZWSendDataMulti = completedFunc;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_ZW_SEND_DATA_MULTI, buffer, &byLen);

    return buffer[IDX_DATA];
}


/*=============================   ZW_SendDataMR   ==========================
**    Transmit data buffer to a single ZW-node or all ZW-nodes (broadcast).
**
**
**    Returns:		FALSE if transmitter queue overflow
**
**    Parameters:
**    nodeID          IN  Destination node ID (0xFF == broadcast)
**    pData           IN  Data buffer pointer
**    dataLength      IN  Data buffer length
**    txOptions       IN  Transmit option flags
**    completedFunc	  IN  Transmit completed call back function
**--------------------------------------------------------------------------*/
BYTE
CSerialAPI::ZW_SendDataMR(
	BYTE destNodeID, 
	BYTE *pData, 
	BYTE dataLength, 
	BYTE txOptions, 
	BYTE pRoute[4], 
	void (__cdecl *completedFunc)(BYTE))
{
    /* destNodeID | dataLength | pData[dataLength] | txOptions | pRoute[4] | funcID */
    int i;
    BYTE idx=0;
    BYTE byCompletedFunc = (completedFunc == NULL? 0: SeqNo());
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_SEND_DATA_MR;
    buffer[idx++] = destNodeID;
    buffer[idx++] = dataLength;
    for (i = 0; i < dataLength; i++)
    {
        buffer[idx++] = pData[i];
    }
    buffer[idx++] = txOptions;
    for (i = 0; i < 4; i++)
    {
        buffer[idx++] = pRoute[i];
    }
    buffer[idx++] = byCompletedFunc;	// Func id for CompletedFunc
    buffer[0] = idx;	// length
    BYTE byLen = 0;
    cbFuncZWSendData_Bridge = completedFunc;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_ZW_SEND_DATA_MR, buffer, &byLen);

    return buffer[IDX_DATA];
}


/*==========================   ZW_SendDataMetaMR   ======================
**    Transmit data buffer to a single ZW-node or all ZW-nodes (broadcast).
**
**
**    Returns:		FALSE if transmitter queue overflow
**
**    Parameters:
**    nodeID          IN  Destination node ID (0xFF == broadcast)
**    pData           IN  Data buffer pointer
**    dataLength      IN  Data buffer length
**    txOptions       IN  Transmit option flags
**    completedFunc	  IN  Transmit completed call back function
**--------------------------------------------------------------------------*/
BYTE
CSerialAPI::ZW_SendDataMetaMR(
	BYTE destNodeID, 
	BYTE *pData, 
	BYTE dataLength, 
	BYTE txOptions, 
	BYTE pRoute[4], 
	void (__cdecl *completedFunc)(BYTE))
{
    /* destNodeID | dataLength | pData[dataLength] | txOptions | pRoute[4] | funcID */
    int i;
    BYTE idx=0;
    BYTE byCompletedFunc = (completedFunc == NULL? 0: SeqNo());
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_SEND_DATA_META_MR;
    buffer[idx++] = destNodeID;
    buffer[idx++] = dataLength;
    for (i = 0; i < dataLength; i++)
    {
        buffer[idx++] = pData[i];
    }
    buffer[idx++] = txOptions;
    for (i = 0; i < 4; i++)
    {
        buffer[idx++] = pRoute[i];
    }
    buffer[idx++] = byCompletedFunc;	// Func id for CompletedFunc
    buffer[0] = idx;	// length
    BYTE byLen = 0;
    cbFuncZWSendDataMeta_Bridge = completedFunc;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_ZW_SEND_DATA_META_MR, buffer, &byLen);

    return buffer[IDX_DATA];
}


/*===========================   ZW_SendData_Bridge   =========================
**    Transmit data buffer to a single ZW-node or all ZW-nodes (broadcast).
**
**
**    Returns:		FALSE if transmitter queue overflow
**
**    Parameters:
**    nodeID          IN  Destination node ID (0xFF == broadcast)
**    pData           IN  Data buffer pointer
**    dataLength      IN  Data buffer length
**    txOptions       IN  Transmit option flags
**    completedFunc	  IN  Transmit completed call back function
**--------------------------------------------------------------------------*/
BYTE 
CSerialAPI::ZW_SendData_Bridge(
	BYTE srcNodeID, 
	BYTE destNodeID, 
	BYTE *pData, 
	BYTE dataLength, 
	BYTE txOptions, 
	BYTE pRoute[4], 
	void (__cdecl *completedFunc)(BYTE))
{
    /* srcNodeID | destNodeID | dataLength | pData[dataLength] | txOptions | pRoute[4] | funcID */
    int i;
    BYTE idx=0;
    BYTE byCompletedFunc = (completedFunc == NULL? 0: SeqNo());
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_SEND_DATA_BRIDGE;
    buffer[idx++] = srcNodeID;
    buffer[idx++] = destNodeID;
    buffer[idx++] = dataLength;
    for (i = 0; i < dataLength; i++)
    {
        buffer[idx++] = pData[i];
    }
    buffer[idx++] = txOptions;
    for (i = 0; i < 4; i++)
    {
        buffer[idx++] = pRoute[i];
    }
    buffer[idx++] = byCompletedFunc;	// Func id for CompletedFunc
    buffer[0] = idx;	// length
    BYTE byLen = 0;
    cbFuncZWSendData_Bridge = completedFunc;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_ZW_SEND_DATA_BRIDGE, buffer, &byLen);

    return buffer[IDX_DATA];
}


/*==========================   ZW_SendDataMeta_Bridge   ======================
**    Transmit data buffer to a single ZW-node or all ZW-nodes (broadcast).
**
**
**    Returns:		FALSE if transmitter queue overflow
**
**    Parameters:
**    nodeID          IN  Destination node ID (0xFF == broadcast)
**    pData           IN  Data buffer pointer
**    dataLength      IN  Data buffer length
**    txOptions       IN  Transmit option flags
**    completedFunc	  IN  Transmit completed call back function
**--------------------------------------------------------------------------*/
BYTE
CSerialAPI::ZW_SendDataMeta_Bridge(
	BYTE srcNodeID, 
	BYTE destNodeID, 
	BYTE *pData, 
	BYTE dataLength, 
	BYTE txOptions, 
	BYTE pRoute[4], 
	void (__cdecl *completedFunc)(BYTE))
{
    /* srcNodeID | destNodeID | dataLength | pData[dataLength] | txOptions | pRoute[4] | funcID */
    int i;
    BYTE idx=0;
    BYTE byCompletedFunc = (completedFunc == NULL? 0: SeqNo());
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_SEND_DATA_META_BRIDGE;
    buffer[idx++] = srcNodeID;
    buffer[idx++] = destNodeID;
    buffer[idx++] = dataLength;
    for (i = 0; i < dataLength; i++)
    {
        buffer[idx++] = pData[i];
    }
    buffer[idx++] = txOptions;
    for (i = 0; i < 4; i++)
    {
        buffer[idx++] = pRoute[i];
    }
    buffer[idx++] = byCompletedFunc;	// Func id for CompletedFunc
    buffer[0] = idx;	// length
    BYTE byLen = 0;
    cbFuncZWSendDataMeta_Bridge = completedFunc;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_ZW_SEND_DATA_META_BRIDGE, buffer, &byLen);

    return buffer[IDX_DATA];
}


/*============================   ZW_SendDataMulti_Bridge   ====================
**    Transmit data buffer to a list of Z-Wave Nodes (multicast frame).
**
**    Returns:			RET  FALSE if transmitter queue overflow
**
**    Parameters:
**    pNodeIDList	IN  List of destination node ID's
**    numberNodes	IN  Number of Nodes
**    pData			IN  Data buffer pointer
**    dataLength	IN  Data buffer length
**    txOptions	    IN  Transmit option flags
**    completedFunc	IN  Transmit completed call back function
**--------------------------------------------------------------------------*/
BYTE
CSerialAPI::ZW_SendDataMulti_Bridge(
	BYTE srcNodeID, 
	BYTE *pNodeIDList, 
	BYTE numberNodes, 
	BYTE *pData, 
	BYTE dataLength, 
	BYTE txOptions, 
	void (*completedFunc)(BYTE txStatus))
{
    /* scrNodeID | numberNodes | pNodeIDList[] | dataLength | pData[] | txOptions | funcId */
    int i;
    BYTE idx=0;
    BYTE byCompletedFunc = (completedFunc == NULL? 0: SeqNo());
    static BYTE buffer[BUF_SIZE];

    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_SEND_DATA_MULTI_BRIDGE;
    buffer[idx++] = srcNodeID;
    numberNodes= (numberNodes <= ZW_MAX_NODES_IN_MULTICAST) ? numberNodes : ZW_MAX_NODES_IN_MULTICAST;
    buffer[idx++] = numberNodes;
    for (i = 0; i < numberNodes; i++)
    {
       buffer[idx++] = pNodeIDList[i];
    }
    buffer[idx++] = dataLength;
    for ( i = 0; i < dataLength; i++)
    {
        buffer[idx++] = pData[i];
    }
    buffer[idx++] = txOptions;
    buffer[idx++] = byCompletedFunc;	// Func id for CompletedFunc
    buffer[0] = idx;	// length
    BYTE byLen = 0;
    cbFuncZWSendDataMulti_Bridge = completedFunc;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_ZW_SEND_DATA_MULTI_BRIDGE, buffer, &byLen);

    return buffer[IDX_DATA];
}


/*============================   MemoryGetID   ===============================
**    Copy the Home-ID and Node-ID to the specified RAM addresses
**
**    Returns:		Nothing
**
**    Parameters
**    homeID		OUT  Home-ID pointer
**    nodeID		OUT  Node-ID pointer
**--------------------------------------------------------------------------*/
void
CSerialAPI::MemoryGetID(
	BYTE *pHomeID, 
	BYTE *pNodeID)
{
    ASSERT(pHomeID != NULL);
    ASSERT(pNodeID != NULL);

    BYTE idx=0;
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_MEMORY_GET_ID;
    buffer[0] = idx;	// length
    BYTE byLen = 0;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_MEMORY_GET_ID, buffer, &byLen);

    pHomeID[0] = buffer[IDX_DATA];
    pHomeID[1] = buffer[IDX_DATA+1];
    pHomeID[2] = buffer[IDX_DATA+2];
    pHomeID[3] = buffer[IDX_DATA+3];
    pNodeID[0] = buffer[IDX_DATA+4];
}


/*========================   ZW_GetLastWorkingRoute   ========================
**    Get Last Working Route for bNodeID
**
**    Returns:		TRUE if a Last Working Route was found.
**
**    Parameters
**    bNodeID		IN   NodeID for which the Last Working Route is wanted
**    pRoute		OUT  Pointer to a BYTE[5] structure where the found route
**						 is to be placed. The first 4 byte (index 0-3) are the
**						 the repeaters used, first one ZERO means no more 
**						 repeaters. The 5 byte (index 4) is the routespeed,
**						 will be ZERO if not supported by SerialAPI module
**						 (DevKit 4.5x).
**--------------------------------------------------------------------------*/
BYTE 
CSerialAPI::ZW_GetLastWorkingRoute(
	BYTE bNodeID, 
	BYTE *pRoute)
{
    ASSERT(bNodeID != 0);
    ASSERT(pRoute != NULL);

    BYTE retVal = FALSE;
    BYTE idx = 1;

    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_GET_LAST_WORKING_ROUTE;
    buffer[idx++] = bNodeID;
    buffer[0] = idx;	// length
    static BYTE byLen = 0;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_ZW_GET_LAST_WORKING_ROUTE, buffer, &byLen);

    if (byLen > 0)
    {
        /* Did we find any Last Working Route */
        retVal = buffer[IDX_DATA + 1];
		/* If we found a route the route is here */
        pRoute[0] = buffer[IDX_DATA+2];
        pRoute[1] = buffer[IDX_DATA+3];
        pRoute[2] = buffer[IDX_DATA+4];
        pRoute[3] = buffer[IDX_DATA+5];
		/* Have the SerialAPI module delivered any routspeed info s*/
		if (byLen > 6)
		{
			/* We got the routespeed */
			pRoute[4] = buffer[IDX_DATA+6];
		}
		else
		{
			/* No routespeed received */
			pRoute[4] = 0;
		}
    }
    return retVal;
}


/*========================   ZW_SetLastWorkingRoute   ========================
**    Set Last Working Route for bNodeID
**
**    Returns:		TRUE if a Last Working Route was set.
**
**    Parameters
**    bNodeID		IN   NodeID for which the Last Working Route should be set.
**    pRoute		IN   Pointer to a BYTE[5] structure where the wanted route
**						 is to placed. The first 4 byte (index 0-3) are the
**						 the repeaters used, first one ZERO means no more 
**						 repeaters. The 5 byte (index 4) is the routespeed.
**--------------------------------------------------------------------------*/
BYTE 
CSerialAPI::ZW_SetLastWorkingRoute(
	BYTE bNodeID, 
	BYTE *pRoute)
{
    ASSERT(bNodeID != 0);
    ASSERT(pRoute != NULL);

    BYTE retVal = FALSE;
    BYTE idx = 1;

    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_SET_LAST_WORKING_ROUTE;
    buffer[idx++] = bNodeID;
	buffer[idx++] = pRoute[0];
	buffer[idx++] = pRoute[1];
	buffer[idx++] = pRoute[2];
	buffer[idx++] = pRoute[3];
	buffer[idx++] = pRoute[4];
    buffer[0] = idx;	// length
    static BYTE byLen = 0;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_ZW_SET_LAST_WORKING_ROUTE, buffer, &byLen);

    if (byLen > 0)
    {
        /* Did we find any Last Working Route */
        retVal = buffer[IDX_DATA + 1];
    }
    return retVal;
}


/*=========================   ZW_Clear_Tx_Timers   ==========================
**    Clear the TX active timers
**
**    Returns:		TRUE
**
**    Parameters
**--------------------------------------------------------------------------*/
void
CSerialAPI::ZW_Clear_Tx_Timers(void)
{
    BYTE retVal = FALSE;
    BYTE idx = 1;

    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_CLEAR_TX_TIMERS;
    buffer[0] = idx;	// length
    SendDataAndWaitForAck(buffer, idx);
}


/*=========================   ZW_Clear_Tx_Timers   ==========================
**    Get the TX active timers
**
**    Returns:		TRUE
**
**    Parameters
**--------------------------------------------------------------------------*/
BYTE
CSerialAPI::ZW_Get_Tx_Timers(
	DWORD *dwChannel0TxTimer,
	DWORD *dwChannel1TxTimer,
	DWORD *dwChannel2TxTimer)
{
    BYTE retVal = FALSE;
    BYTE idx = 1;

    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_GET_TX_TIMERS;
    buffer[0] = idx;	// length
    static BYTE byLen = 0;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_GET_TX_TIMERS, buffer, &byLen);

    if (3 * (sizeof(DWORD)) < byLen)
    {
		/* The 3 DWORD Timers are received as MSB | Next_MSB | Next_LSB | LSB */
		*dwChannel0TxTimer = ((DWORD)buffer[IDX_DATA] << 24) +
							 ((DWORD)buffer[IDX_DATA + 1] << 16) +
							 ((DWORD)buffer[IDX_DATA + 2] << 8) +
							 buffer[IDX_DATA + 3];
		*dwChannel1TxTimer = ((DWORD)buffer[IDX_DATA + sizeof(DWORD)] << 24) +
							 ((DWORD)buffer[IDX_DATA + sizeof(DWORD) + 1] << 16) +
							 ((DWORD)buffer[IDX_DATA + sizeof(DWORD) + 2] << 8) +
							 buffer[IDX_DATA + sizeof(DWORD) + 3];
		*dwChannel2TxTimer = ((DWORD)buffer[IDX_DATA + (2 * sizeof(DWORD))] << 24) +
		 					 ((DWORD)buffer[IDX_DATA + (2 * sizeof(DWORD)) + 1] << 16) +
							 ((DWORD)buffer[IDX_DATA + (2 * sizeof(DWORD)) + 2] << 8) +
							 buffer[IDX_DATA + (2 * sizeof(DWORD)) + 3];
    }
    return retVal;
}



/*==========================   ZW_SetRoutingMAX   ===========================
**    Set MAX number of route tries before trying Explore Frame route resolution
**
**    Returns:		TRUE
**
**    Parameters
**    maxRouteTries	IN		MAX number of route tries
**--------------------------------------------------------------------------*/
BYTE
CSerialAPI::ZW_SetRoutingMAX(
	BYTE maxRouteTries)
{
    ASSERT(maxRouteTries != 0);

    BYTE retVal = FALSE;
    BYTE idx = 1;

    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_SET_ROUTING_MAX;
    buffer[idx++] = maxRouteTries;
    buffer[0] = idx;	// length
    static BYTE byLen = 0;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_ZW_SET_ROUTING_MAX, buffer, &byLen);

    if (0 < byLen)
    {
        retVal = buffer[IDX_DATA];
    }
    return retVal;
}


/*=====================   SerialAPI_Get_Capabilities   =======================
**    Get SerialAPI Capabilities - Request module for SerialAPI Capabilities
**
**    Returns:		The SerialAPI Capabilities Structure specified filled with
**					the module capabilities
**
**    Parameters
**    offset		IN   Pointer to SerialAPI Capability Structure where
**						 the module capabilities should be delivered
**--------------------------------------------------------------------------*/
void
CSerialAPI::SerialAPI_Get_Capabilities(
	BYTE *capabilities)
{
    BYTE idx=0;
    static BYTE buffer[BUF_SIZE];
    BYTE *pBuffer = capabilities;
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_SERIAL_API_GET_CAPABILITIES;
    buffer[0] = idx;	// length
    BYTE byLen = 0;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_SERIAL_API_GET_CAPABILITIES, buffer, &byLen);

  for (idx = 0; idx < byLen; idx++)
  {
    *pBuffer++ = buffer[IDX_DATA + idx];
  }
}


/*============================   MemoryGetByte   ============================
**    Read one byte from the EEPROM
**
**    Returns:		Data
**
**    Parameters
**    offset		IN   Application area offset
**--------------------------------------------------------------------------*/
BYTE
CSerialAPI::MemoryGetByte(
	WORD offset)
{
    BYTE idx=0;
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_MEMORY_GET_BYTE;
    buffer[idx++] = offset >> 8;
    buffer[idx++] = offset & 0xFF;
    buffer[0] = idx;	// length
    BYTE byLen = 0;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_MEMORY_GET_BYTE, buffer, &byLen);

    return buffer[IDX_DATA];
}

/*============================   MemoryPutByte   ============================
**    Write one byte to the EEPROM
**
**    Returns:		False if write buffer full
**
**    Parameters
**    offset		IN   Application area offset
**    data			IN   Data to store
**--------------------------------------------------------------------------*/
BYTE
CSerialAPI::MemoryPutByte(
	WORD offset, 
	BYTE data)
{
    BYTE idx=0;
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_MEMORY_PUT_BYTE;
    buffer[idx++] = offset >> 8;
    buffer[idx++] = offset & 0xFF;
    buffer[idx++] = data;
    buffer[0] = idx;	// length
    BYTE byLen = 0;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_MEMORY_PUT_BYTE, buffer, &byLen);

    return buffer[IDX_DATA];
}

/*============================   MemoryGetBuffer   =============================
**    Read number of bytes from the EEPROM to a RAM buffer
**
**    Returns:		Nothing
**
**    Parameters
**    offset		IN   Application area offset
**    buffer		IN   Buffer pointer
**    length		IN   Number of bytes to read
**--------------------------------------------------------------------------*/
void
CSerialAPI::MemoryGetBuffer(
	WORD offset, 
	BYTE *buf, 
	BYTE length)
{
    int i;
    BYTE idx=0;
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_MEMORY_GET_BUFFER;
    buffer[idx++] = offset >> 8;
    buffer[idx++] = offset & 0xFF;
    buffer[idx++] = length;		// Number of bytes to read
    buffer[0] = idx;	// length
    BYTE byLen = 0;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_MEMORY_GET_BUFFER, buffer, &byLen);

    ASSERT(length == byLen-3);
    for (i=0;i<length;i++) {
        buf[i] = buffer[IDX_DATA+i];
    }
}


void
CSerialAPI::ZW_LockRoutes(
	BYTE bValue)
{
    BYTE idx = 0;
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_LOCK_ROUTE_RESPONSE;
    buffer[idx++] = bValue;
    buffer[0] = idx;	// length
    SendData(buffer, idx, NULL);
}


/*============================   ZW_GetRoutingInfo   =============================
**    Read number of bytes from the EEPROM to a RAM buffer
**
**    Returns:		Nothing
**
**    Parameters
**    bNodeID          IN    The NodeID to get routing information from
**    buf   		  IN    Buffer pointer to where the data is placed
**    bRemoveBad      IN    If TRUE remove bad repeaters
**    bRemoveNonReps  IN    If TRUE remove non repeating devices
**--------------------------------------------------------------------------*/
void 
CSerialAPI::ZW_GetRoutingInfo(
	BYTE bNodeID, 
	BYTE *buf, 
	BYTE bRemoveBad, 
	BYTE bRemoveNonReps)
{
    int i;
    BYTE idx=0;
    static BYTE buffer[BUF_SIZE];
	/*  bNodeID | bRemoveBad | bRemoveNonReps | funcID */
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_GET_ROUTING_TABLE_LINE;
    buffer[idx++] = bNodeID;
    buffer[idx++] = bRemoveBad;
    buffer[idx++] = bRemoveNonReps;
    buffer[0] = idx;
    BYTE byLen = 0;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_GET_ROUTING_TABLE_LINE, buffer, &byLen);
	for (i = 0; i < ZW_MAX_NODEMASK_LENGTH; i++) 
	{
        buf[i] = buffer[IDX_DATA+i];
    }
}


BYTE 
CSerialAPI::ZW_RequestNodeInfo(
    BYTE bNodeID, 
	void (__cdecl *completedFunc)(BYTE))
{
    BYTE idx=0;
    BYTE byCompletedFunc = (completedFunc == NULL? 0: SeqNo());
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_REQUEST_NODE_INFO;
    buffer[idx++] = bNodeID;
    buffer[idx++] = byCompletedFunc;	// Func id for CompletedFunc
    buffer[0] = idx;	// length
    BYTE byLen = 0;
    cbFuncZWRequestNodeInfo = completedFunc;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_ZW_REQUEST_NODE_INFO, buffer, &byLen);

    if (byLen > IDX_DATA)
    {
        return buffer[IDX_DATA];
    }
    else
    {
        /* No answer */
        return FALSE;
    }
}


#ifdef ZW_INSTALLER
void 
CSerialAPI::ZW_ResetTXCounter(void)
{
    BYTE idx = 0;
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_RESET_TX_COUNTER;
    buffer[0] = idx;	// length
    SendData(buffer, idx);
}

/*============================   ZW_GetTXCounter ============================
**    Copy number of bytes from a RAM buffer to the EEPROM
**
**    Returns:		The number of frames transmitted since the TX Counter last
**					was reset..
**
**    Parameters	none
**--------------------------------------------------------------------------*/
BYTE 
CSerialAPI::ZW_GetTXCounter(void)
{
    BYTE idx = 0;
    BYTE byLen = 0;
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_GET_TX_COUNTER;
    buffer[0] = idx;	// length
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_GET_TX_COUNTER, buffer, &byLen);
    return buffer[IDX_DATA];
}
#endif

/*============================   MemoryPutBuffer   =============================
**    Write number of bytes specified to the EEPROM starting from offset
**
**    Returns:		TRUE if write done
**
**    Parameters
**    offset		IN   Application area offset
**    buffer        IN   Buffer pointer
**    length        IN   Number of bytes to write
**    func			IN   Buffer put completed function pointer
**--------------------------------------------------------------------------*/
BYTE 
CSerialAPI::MemoryPutBuffer(
	WORD offset, 
	BYTE *buf, 
	WORD length, 
	void (__cdecl *func)(void))
{
    int i;
    BYTE idx=0;
    static BYTE buffer[BUF_SIZE];
    BYTE byCompletedFunc = (func == NULL? 0: SeqNo());
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_MEMORY_PUT_BUFFER;
    buffer[idx++] = offset >> 8;
    buffer[idx++] = offset & 0xFF;
    ASSERT(length + 8 <= BUF_SIZE);
    if (length > BUF_SIZE - 8)
	{
        length = BUF_SIZE-8;
	}
    buffer[idx++] = length >> 8;		// Number of bytes to read into buffer
    buffer[idx++] = length & 0xFF;
    for (i=0; i<length; i++) 
	{
        buffer[idx++] = buf[i];
    }
    buffer[idx++] = byCompletedFunc;
    buffer[0] = idx;	// length
    cbFuncMemoryPutBuffer = func;
    BYTE byLen = 0;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_MEMORY_PUT_BUFFER, buffer, &byLen);

    return buffer[IDX_DATA];
}


/*=========================   ZW_RequestNetworkUpdate   =====================
**    Request SUC/SIS for network an update.
**
**    Returns:		Nothing
**
**    Parameters
**    completedFunc		IN	Completion function
**          bStatus		   IN  Status of find neighbor process
**--------------------------------------------------------------------------*/
void
CSerialAPI::ZW_RequestNetworkUpdate(
	void (__cdecl *completedFunc)(BYTE))
{
    BYTE idx=0;
    static BYTE buffer[BUF_SIZE];
    BYTE byCompletedFunc = (completedFunc == NULL? 0: SeqNo());
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_REQUEST_NETWORK_UPDATE ;
    buffer[idx++] = byCompletedFunc;
    buffer[0] = idx;	// length
    cbFuncZWRequestNetworkUpdate = completedFunc;
    BYTE byLen = 0;
    SendData(buffer, idx, NULL);
}


/*==========================   ZW_RequestNodeNeighborUpdate   =======================
**    Request controller to update neighbor information for a specific node.
**
**    Returns:		Nothing
**
**    Parameters
**    nodeID,           IN nodeID on node to ask for neighbors
**    completedFunc		IN	Completion function
**          bStatus		   IN  Status of find neighbor process
**--------------------------------------------------------------------------*/
void
CSerialAPI::ZW_RequestNodeNeighborUpdate(
	BYTE nodeID, 
	void (__cdecl *completedFunc)(BYTE))
{
    BYTE idx=0;
    static BYTE buffer[BUF_SIZE];
    BYTE byCompletedFunc = (completedFunc == NULL? 0: SeqNo());
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_REQUEST_NODE_NEIGHBOR_UPDATE;
    buffer[idx++] = nodeID;
    buffer[idx++] = byCompletedFunc;
    buffer[0] = idx;	// length
    cbFuncZWRequestNodeNodeNeighborUpdate = completedFunc;
    BYTE byLen = 0;
    if (!SendDataAndWaitForAck(buffer, idx))
	{
        if (cbFuncZWRequestNodeNodeNeighborUpdate != NULL)
		{
            cbFuncZWRequestNodeNodeNeighborUpdate(0x23);
		}
	}
}


/*=================   ZW_RequestNodeNeighborUpdate_Option   =================
**    Request controller to update neighbor information for a specific node.
**
**    Returns:		Nothing
**
**    Parameters
**    nodeID,           IN nodeID on node to ask for neighbors
**    bTxOption			IN Transmit Options to use when requesting node neighbor update
**                         if ZERO then default transmit options are used
**    completedFunc		IN	Completion function
**          bStatus		   IN  Status of find neighbor process
**--------------------------------------------------------------------------*/
void
CSerialAPI::ZW_RequestNodeNeighborUpdate_Option(
	BYTE nodeID, 
	BYTE bTxOption, 
	void (__cdecl *completedFunc)(BYTE))
{
    BYTE idx=0;
    static BYTE buffer[BUF_SIZE];
    BYTE byCompletedFunc = (completedFunc == NULL? 0: SeqNo());
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_REQUEST_NODE_NEIGHBOR_UPDATE_OPTION;
    buffer[idx++] = nodeID;
    buffer[idx++] = bTxOption;
    buffer[idx++] = byCompletedFunc;
    buffer[0] = idx;	// length
    cbFuncZWRequestNodeNodeNeighborUpdate_Option = completedFunc;
    BYTE byLen = 0;
    SendData(buffer, idx, NULL);
}


BYTE
CSerialAPI::ZW_Support9600Only(
	BYTE nodeID)
{
    BYTE idx=1;
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_SUPPORT9600_ONLY;
    buffer[idx++] = nodeID;
    buffer[0] = idx;	// length
    BYTE byLen = 0;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_ZW_SUPPORT9600_ONLY, buffer, &byLen);

    return buffer[IDX_DATA];
}


//
//    Set controller in "learn node mode", when the remote adds a unit.
//    Application data are passed to the application through
//    the call back function.
void
CSerialAPI::ZW_SetIdleNodeLearn(
	void (__cdecl *completedFunc)(BYTE, BYTE, BYTE *, BYTE))
{
    cbIdleLearnNode = completedFunc;
}



/*=====================   ZW_GetNodeProtocolInfo   ==========================
**    Copy the Node's current protocol information from the non-volatile
**    memory.
**
**    Returns:		Nothing
**
**    Parameters
**    nodeID		IN Node ID
**    nodeInfo		OUT Node info buffer
**--------------------------------------------------------------------------*/
void
CSerialAPI::ZW_GetNodeProtocolInfo(
	BYTE bNodeID, 
	NODEINFO *nodeInfo)
{
    ASSERT(nodeInfo != NULL);

    BYTE idx=0;
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_GET_NODE_PROTOCOL_INFO;
    buffer[idx++] = bNodeID;
    buffer[0] = idx;	// length
    BYTE byLen = 0;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_ZW_GET_NODE_PROTOCOL_INFO, buffer, &byLen);

    nodeInfo->capability = buffer[IDX_DATA];
    nodeInfo->security = buffer[IDX_DATA+1];
    nodeInfo->reserved = buffer[IDX_DATA+2];
    nodeInfo->nodeType.basic = buffer[IDX_DATA+3];
    nodeInfo->nodeType.generic = buffer[IDX_DATA+4];
    nodeInfo->nodeType.specific = buffer[IDX_DATA+5];
}

/*===========================   ZW_SetDefault   ================================
**    Remove all Nodes and timers from the EEPROM memory.
**
**    Returns:		Nothing
**
**    Parameters
**    completedFunc	IN  Command completed call back function
**--------------------------------------------------------------------------*/
void
CSerialAPI::ZW_SetDefault(
	void (__cdecl *completedFunc)(void))
{
    BYTE idx=0;
    static BYTE buffer[BUF_SIZE];
    BYTE byCompletedFunc = (completedFunc == NULL? 0: SeqNo());
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_SET_DEFAULT;
    buffer[idx++] = byCompletedFunc;
    buffer[0] = idx;	// length
    cbFuncZWSetDefault = completedFunc;
    SendData(buffer, idx, NULL);
}


/*===========================   ZW_SoftReset   ===============================
**    Make soft reset
**
**    Returns:		Nothing
**
**    Parameters
**--------------------------------------------------------------------------*/
void
CSerialAPI::ZW_SoftReset(
	BYTE resetMode)
{
    BYTE idx=0;
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_SERIAL_API_SOFT_RESET;
    buffer[idx++] = resetMode;
    buffer[0] = idx;	// length
    SendData(buffer, idx, NULL);
}


/*==========================   ZW_WatchDogEnable   ===========================
**    Enable WatchDog on Z-Wave module
**
**    Returns:		Nothing
**
**    Parameters
**--------------------------------------------------------------------------*/
void
CSerialAPI::ZW_WatchDogEnable(void)
{
    BYTE idx=0;
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_WATCHDOG_ENABLE;
    buffer[0] = idx;	// length
    SendData(buffer, idx, NULL);
}


/*===========================   ZW_WatchDogKick   ============================
**    Kick WatchDog on Z-Wave module
**
**    Returns:		Nothing
**
**    Parameters
**--------------------------------------------------------------------------*/
void
CSerialAPI::ZW_WatchDogKick(void)
{
    BYTE idx=0;
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_WATCHDOG_KICK;
    buffer[0] = idx;	// length
    SendData(buffer, idx, NULL);
}


/*==========================   ZW_WatchDogDisable   ==========================
**    Disable WatchDog on Z-Wave module
**
**    Returns:		Nothing
**
**    Parameters
**--------------------------------------------------------------------------*/
void
CSerialAPI::ZW_WatchDogDisable(void)
{
    BYTE idx=0;
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_WATCHDOG_DISABLE;
    buffer[0] = idx;	// length
    SendData(buffer, idx, NULL);
}


/*===========================   ZW_WatchDogStart   ===========================
**    Start WatchDog servicing on the Z-Wave module.
**    This means that the WatcDog is enabled and every ApplicationPoll call
**    it is Kicked. The information that it is started is also saved in NVM
**    so that the WatchDog is enabled again after Reset.
**
**    Returns:		Nothing
**
**    Parameters
**--------------------------------------------------------------------------*/
void
CSerialAPI::ZW_WatchDogStart(void)
{
    BYTE idx=0;
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_WATCHDOG_START;
    buffer[0] = idx;	// length
    SendData(buffer, idx, NULL);
}


/*===========================   ZW_WatchDogStop   ============================
**    Stop WatchDog servicing on the Z-Wave module
**
**    Returns:		Nothing
**
**    Parameters
**--------------------------------------------------------------------------*/
void
CSerialAPI::ZW_WatchDogStop(void)
{
    BYTE idx=0;
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_WATCHDOG_STOP;
    buffer[0] = idx;	// length
    SendData(buffer, idx, NULL);
}


/*========================   ZW_ReplaceFailedNode   ==========================
**    Start a Replace Failed Node sequence
**
**    Returns:		Nothing
**
**    Parameters
**--------------------------------------------------------------------------*/
BYTE
CSerialAPI::ZW_ReplaceFailedNode(
	BYTE nodeID,
	void (__cdecl *completedFunc)(BYTE))
{
	BYTE retVal;
    BYTE idx = 0;
    BYTE byLen = 0;
    static BYTE buffer[BUF_SIZE];
    BYTE byCompletedFunc = (completedFunc == NULL? 0: SeqNo());
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_REPLACE_FAILED_NODE;
    buffer[idx++] = nodeID;
	buffer[idx++] = byCompletedFunc;
    buffer[0] = idx;	// length

    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_ZW_REPLACE_FAILED_NODE, buffer, &byLen);

    if (byLen > IDX_DATA)
    {
		retVal = buffer[IDX_DATA];
		cbFuncZWReplaceFailedNode = completedFunc;
    }
    else
    {
        /* No answer */
        retVal = 0xFF;
		cbFuncZWReplaceFailedNode = NULL;
    }
	return retVal;
}


/*========================   ZW_RemoveFailedNode   ==========================
**    Start a Remove Failed Node sequence
**
**    Returns:		Nothing
**
**    Parameters
**--------------------------------------------------------------------------*/
BYTE
CSerialAPI::ZW_RemoveFailedNode(
	BYTE nodeID,
	void (__cdecl *completedFunc)(BYTE))
{
	BYTE retVal;
    BYTE idx = 0;
    BYTE byLen = 0;
    static BYTE buffer[BUF_SIZE];
    BYTE byCompletedFunc = (completedFunc == NULL? 0: SeqNo());
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
	buffer[idx++] = FUNC_ID_ZW_REMOVE_FAILED_NODE_ID;
    buffer[idx++] = nodeID;
	buffer[idx++] = byCompletedFunc;
    buffer[0] = idx;	// length

	SendDataAndWaitForResponse(buffer, idx, FUNC_ID_ZW_REMOVE_FAILED_NODE_ID, buffer, &byLen);

    if (byLen > IDX_DATA)
    {
		retVal = buffer[IDX_DATA];
		cbFuncZWRemoveFailedNode = completedFunc;
    }
    else
    {
        /* No answer */
        retVal = 0xFF;
		cbFuncZWRemoveFailedNode = NULL;
    }
	return retVal;
}


/*========================   ZW_ControllerChange   ==========================
**    Start a Controller Change sequence
**
**    Returns:		Nothing
**
**    Parameters
**--------------------------------------------------------------------------*/
void
CSerialAPI::ZW_ControllerChange(
	BYTE mode, 
	void (__cdecl *completedFunc)(BYTE,BYTE,BYTE *,BYTE))
{
    BYTE idx = 0;
    static BYTE buffer[BUF_SIZE];
    BYTE byCompletedFunc = (completedFunc == NULL? 0: SeqNo());
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_CONTROLLER_CHANGE;
    buffer[idx++] = mode;
    buffer[idx++] = byCompletedFunc;
    buffer[0] = idx;	// length
    cbFuncZWNewController = completedFunc;
    SendData(buffer, idx, NULL);
}


/*========================   ZW_AddNodeToNetwork   ==========================
**    Start a Add Node To Network sequence
**
**    Returns:		Nothing
**
**    Parameters
**--------------------------------------------------------------------------*/
void
CSerialAPI::ZW_AddNodeToNetwork(
	BYTE mode, 
	void (__cdecl *completedFunc)(BYTE,BYTE,BYTE *,BYTE))
{
    BYTE idx=0;
    static BYTE buffer[BUF_SIZE];
    BYTE byCompletedFunc = (completedFunc == NULL? 0: SeqNo());
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_ADD_NODE_TO_NETWORK;
    buffer[idx++] = mode;
    buffer[idx++] = byCompletedFunc;
    buffer[0] = idx;	// length
    cbFuncZWNewController = completedFunc;
    SendData(buffer, idx, NULL);
}


/*======================   ZW_RemoveNodeFromNetwork   ========================
**    Start a Remove Node From Network sequence
**
**    Returns:		Nothing
**
**    Parameters
**--------------------------------------------------------------------------*/
void
CSerialAPI::ZW_RemoveNodeFromNetwork(
	BYTE mode, 
	void (__cdecl *completedFunc)(BYTE,BYTE,BYTE *,BYTE))
{
    BYTE idx=0;
    static BYTE buffer[BUF_SIZE];
    BYTE byCompletedFunc = (completedFunc == NULL? 0: SeqNo());
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_REMOVE_NODE_FROM_NETWORK;
    buffer[idx++] = mode;
    buffer[idx++] = byCompletedFunc;
    buffer[0] = idx;	// length
    cbFuncZWNewController = completedFunc;
    SendData(buffer, idx, NULL);
}


/*====================   ZW_ReplicationCommandComplete ======================
**    Sends command completed to master remote. Called in replication mode
**    when a command from the sender has been processed.
**
**    Returns:		Nothing
**
**    Parameters	None
**
**--------------------------------------------------------------------------*/
void
CSerialAPI::ZW_ReplicationCommandComplete()
{
    BYTE idx = 0;
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_REPLICATION_COMMAND_COMPLETE;
    buffer[0] = idx;	// length
    SendData(buffer, idx, NULL);
}


BYTE 
CSerialAPI::ZW_EnableSUC(
	BYTE state, 
	BYTE capabilities)
{
    BYTE idx = 0;
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_ENABLE_SUC;
    buffer[idx++] = state;
    buffer[idx++] = capabilities;
    buffer[0] = idx;	// length
    BYTE byLen = 0;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_ZW_ENABLE_SUC, buffer, &byLen);

    return buffer[IDX_DATA];
}


/*======================   ZW_REPLICATION_SEND_DATA   ======================
**    Used when the controller is replication mode.
**    It sends the payload and expects the receiver to respond with a
**    command complete message.
**
**    Returns:		FALSE if transmitter queue overflow
**
**    Parameters
**    nodeID			IN  Destination node ID
**    pData				IN  Data buffer pointer
**    dataLength		IN  Data buffer length
**    txOptions			IN  Transmit option flags
**    completedFunc		IN  Transmit completed call back function
**			txStatus		IN Transmit status
**--------------------------------------------------------------------------*/
BYTE
CSerialAPI::ZW_ReplicationSendData(
	BYTE nodeID, 
	BYTE *pData, 
	BYTE dataLength, 
	BYTE txOptions, 
	void (__cdecl *completedFunc)(BYTE))
{
    int i;
    BYTE idx=0;
    BYTE byCompletedFunc = (completedFunc == NULL? 0: SeqNo());
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_REPLICATION_SEND_DATA;
    buffer[idx++] = nodeID;
    buffer[idx++] = dataLength;
    for (i = 0; i < dataLength; i++)
	{
        buffer[idx++] = pData[i];
	}
    buffer[idx++] = txOptions;
    buffer[idx++] = byCompletedFunc;	// Func id for CompletedFunc
    buffer[0] = idx;	// length
    BYTE byLen = 0;
    cbFuncZWReplicationSendData = completedFunc;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_ZW_REPLICATION_SEND_DATA, buffer, &byLen);

    return buffer[IDX_DATA];
}


/*========================   ZW_AssignReturnRoute   =========================
**
**    Assign static return routes within a Routing Slave node.
**    Calculate the shortest transport routes to a Routing Slave node
**    from the Report destination Node and
**    transmit the return routes to the Routing Slave node.
**
**    Returns:		Nothing
**
**    Parameters
**    bSrcNodeID		IN  Routing Slave Node ID
**    bDstNodeID		IN	Report destination Node ID
**    completedFunc IN	 Completion handler
**			bStatus		IN	Transmit complete status
**
**--------------------------------------------------------------------------*/
void 
CSerialAPI::ZW_AssignReturnRoute(
	BYTE bSrcNodeID, 
	BYTE bDstNodeID, 
	void (__cdecl *completedFunc)(BYTE))
{
    BYTE idx=0;
    BYTE byCompletedFunc = (completedFunc == NULL? 0: SeqNo());
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_ASSIGN_RETURN_ROUTE;
    buffer[idx++] = bSrcNodeID;
    buffer[idx++] = bDstNodeID;
    buffer[idx++] = byCompletedFunc;	// Func id for CompletedFunc
    buffer[0] = idx;	// length
    cbFuncZWAssignReturnRoute = completedFunc;
    SendData(buffer, idx, NULL);
}


/*========================   ZW_DeleteReturnRoute   =========================
**
**    Delete static return routes within a Routing Slave node.
**    Transmit "NULL" routes to the Routing Slave node.
**
**    Returns:		Nothing
**
**    Parameters
**    nodeID		IN   Routing Slave
**    completedFunc IN	 Completion handler
**			bStatus		IN	Transmit complete status
**
**--------------------------------------------------------------------------*/
void
CSerialAPI::ZW_DeleteReturnRoute(
	BYTE nodeID, 
	void (__cdecl *completedFunc)(BYTE))
{
    BYTE idx=0;
    BYTE byCompletedFunc = (completedFunc == NULL? 0: SeqNo());
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_DELETE_RETURN_ROUTE;
    buffer[idx++] = nodeID;
    buffer[idx++] = byCompletedFunc;	// Func id for CompletedFunc
    buffer[0] = idx;	// length
    cbFuncZWDeleteReturnRoute = completedFunc;
    SendData(buffer, idx, NULL);
}


/*========================   ZW_AssignSUCReturnRoute   =========================
**
**    Assign static return routes within a Routing Slave node.
**    Calculate the shortest transport routes to a Routing Slave node
**    from the Static Update Controller (SUC) Node and
**    transmit the return routes to the Routing Slave node.
**
**    Returns:		Nothing
**
**    Parameters
**    bSrcNodeID		IN  Routing Slave Node ID
**    bDstNodeID		IN	SUC node ID
**    completedFunc IN	 Completion handler
**			bStatus		IN	Transmit complete status
**
**--------------------------------------------------------------------------*/
void
CSerialAPI::ZW_AssignSUCReturnRoute(
	BYTE bSrcNodeID, 
	BYTE bDstNodeID, 
	void (__cdecl *completedFunc)(BYTE))
{
    BYTE idx=0;
    BYTE byCompletedFunc = (completedFunc == NULL? 0: SeqNo());
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_ASSIGN_SUC_RETURN_ROUTE;
    buffer[idx++] = bSrcNodeID;
    buffer[idx++] = bDstNodeID;
    buffer[idx++] = byCompletedFunc;	// Func id for CompletedFunc
    buffer[0] = idx;	// length
    cbFuncZWAssignSUCReturnRoute = completedFunc;
    SendData(buffer, idx, NULL);
}


/*========================   ZW_DeleteSUCReturnRoute   =========================
**
**    Delete the ( Static Update Controller -SUC-) static return routes
**    within a Routing Slave node.
**    Transmit "NULL" routes to the Routing Slave node.
**
**    Returns:		Nothing
**
**    Parameters
**    nodeID		IN   Routing Slave
**    completedFunc IN	 Completion handler
**			bStatus		IN	Transmit complete status
**
**--------------------------------------------------------------------------*/
void
CSerialAPI::ZW_DeleteSUCReturnRoute(
	BYTE nodeID, 
	void (__cdecl *completedFunc)(BYTE))
{
    BYTE idx=0;
    BYTE byCompletedFunc = (completedFunc == NULL? 0: SeqNo());
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_DELETE_SUC_RETURN_ROUTE;
    buffer[idx++] = nodeID;
    buffer[idx++] = byCompletedFunc;	// Func id for CompletedFunc
    buffer[0] = idx;	// length
    cbFuncZWDeleteSUCReturnRoute = completedFunc;
    SendData(buffer, idx, NULL);
}


/*============================   GetSUCNodeID   =============================
**    Get the currently registered SUC node ID
**
**    Returns:		SUC node ID, ZERO if no SUC available
**
**--------------------------------------------------------------------------*/
BYTE 
CSerialAPI::ZW_GetSUCNodeID(void)
{
    BYTE idx = 0;
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_GET_SUC_NODE_ID;
    buffer[0] = idx;	// length
    BYTE byLen = 0;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_ZW_GET_SUC_NODE_ID, buffer, &byLen);
    return buffer[IDX_DATA];
}


/*===========================   ZW_SetSUCNodeID ===============================
**    This function is used to select the static controller that should act
**    as SUC in the network. Callback indicates if the Static controller
**    accepts.
**    Returns:		Nothing
**
**    Parameters
**    nodeID		IN  Node ID to try and use as SUC
**    state			IN  state telling if it is a enable SUC (TRUE) or disable (FALSE)
**	  txOption		IN	transmit options
**	  capabilities	IN	SUC capabilities
**    complfunc		IN  call back function with status ZW_SUC_SET_SUCCEEDED if
**						succesfull and ZW_SUC_SET_FAILED if the assignment failed
**--------------------------------------------------------------------------*/
BYTE 
CSerialAPI::ZW_SetSUCNodeID(
	BYTE bNodeID, 
	BYTE state, 
	BYTE capabilities, 
	BYTE txOption, 
	void (__cdecl *complFunc)(BYTE))
{
    BYTE idx=0;
    BYTE byCompletedFunc = (complFunc == NULL? 0: SeqNo());
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_SET_SUC_NODE_ID;
    buffer[idx++] = bNodeID;
    buffer[idx++] = state;  /* Do we want to enable or disable?? */
    buffer[idx++] = txOption;
    buffer[idx++] = capabilities;
    buffer[idx++] = byCompletedFunc;	// Func id for CompletedFunc

    buffer[0] = idx;	// length
    cbFuncZWSetSUCNodeID = complFunc;
    BYTE byLen = 0;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_ZW_SET_SUC_NODE_ID, buffer, &byLen);
    return buffer[IDX_DATA];
}


/*============================   ZW_SendSUCID ================================
**--------------------------------------------------------------------------*/
BYTE 
CSerialAPI::ZW_SendSUCID(
	BYTE bNodeID, 
	BYTE txOption, 
	void (__cdecl *complFunc)(BYTE))
{
    BYTE idx=0;
    BYTE byCompletedFunc = (complFunc == NULL? 0: 0x43);
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_SEND_SUC_ID;
    buffer[idx++] = bNodeID;
    buffer[idx++] = txOption;
    buffer[idx++] = byCompletedFunc;	// Func id for CompletedFunc

    buffer[0] = idx;	// length
    cbFuncZWSendSUCID = complFunc;
    BYTE byLen = 0;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_ZW_SEND_SUC_ID, buffer, &byLen);
    return buffer[IDX_DATA];
}


/*==============================   ClockSet   ===============================
**    Set the Real Time Clock
**
**    Returns:		FALSE if invalid time value
**
**    Parameters
**    pNewTime		IN   New time to set
**--------------------------------------------------------------------------*/
BYTE
CSerialAPI::ClockSet(
	CLOCK *pNewTime)
{
    BYTE idx=0;
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_CLOCK_SET;
    buffer[idx++] = pNewTime->weekday;
    buffer[idx++] = pNewTime->hour;
    buffer[idx++] = pNewTime->minute;
    buffer[0] = idx;	// length
    BYTE byLen = 0;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_CLOCK_SET, buffer, &byLen);
    return buffer[IDX_DATA];
}


/*==============================   ClockGet   ===============================
**    Get the current Real Time Clock time
**
**    Returns:		Nothing
**
**    Parameters
**    pTime			OUT   Pointer to put the current time
**--------------------------------------------------------------------------*/
void 
CSerialAPI::ClockGet(
	CLOCK *pTime)
{
    BYTE idx=0;
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_CLOCK_GET;
    buffer[0] = idx;	// length
    BYTE byLen = 0;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_CLOCK_GET, buffer, &byLen);

    pTime->weekday = buffer[IDX_DATA];
    pTime->hour = buffer[IDX_DATA+1];
    pTime->minute = buffer[IDX_DATA+2];
}


/*==============================   ClockCmp   ===============================
**    Compare the specified time with the current Real Time Clock time
**
**    Returns:		-1:before; 0:equal; 1:past current time
**
**    Parameters
**    pTime			IN   Pointer to the time structure
**--------------------------------------------------------------------------*/
char 
CSerialAPI::ClockCmp(
	CLOCK *pTime)
{
    BYTE idx=0;
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_CLOCK_CMP;
    buffer[idx++] = pTime->weekday;
    buffer[idx++] = pTime->hour;
    buffer[idx++] = pTime->minute;
    buffer[0] = idx;	// length
    BYTE byLen = 0;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_CLOCK_CMP, buffer, &byLen);

    return buffer[IDX_DATA];
}


/*==============================   RTCTimerCreate   =========================
**    Create a timer element that initiate a call to the specified function.
**
**
**    Returns:		RTC timer handle (-1 if failed)
**
**    Parameters
**    timer			IN  Pointer to the new timer structure
**    func			IN  Timer element read completed function pointer
**--------------------------------------------------------------------------*/
BYTE 
CSerialAPI::RTCTimerCreate(
	RTC_TIMER *timer,
	void (__cdecl *func)(void))
{
    BYTE i, idx=0;
    BYTE byCompletedFunc = (func == NULL? 0: SeqNo());
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_RTC_TIMER_CREATE;
    buffer[idx++] = timer->status;
    buffer[idx++] = timer->timeOn.weekday;
    buffer[idx++] = timer->timeOn.hour;
    buffer[idx++] = timer->timeOn.minute;
    buffer[idx++] = timer->timeOff.weekday;
    buffer[idx++] = timer->timeOff.hour;
    buffer[idx++] = timer->timeOff.minute;
    buffer[idx++] = timer->repeats;
    buffer[idx++] = timer->parm;
    for (i=0;i<RTC_TIMER_MAX;i++) 
	{
        // find first free timer element
        if (timerRTCArray[i].timerHandle == FREE_RTC_TIMER_HANDLE)
        {
            timerRTCArray[i].func = timer->func;
            // Handle is set later
            break;
        }
    }
    buffer[idx++] = i;	// Timer func id
    buffer[idx++] = byCompletedFunc;
    cbFuncRTCTimerCreate = func;

    buffer[0] = idx;	// length
    BYTE byLen = 0;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_RTC_TIMER_CREATE, buffer, &byLen);

    if (buffer[IDX_DATA] != (BYTE)-1)
	{
        timerRTCArray[i].timerHandle = buffer[IDX_DATA];
	}

    return buffer[IDX_DATA];
}


/*==============================   RTCTimerRead   ===========================
**    Copy a timer element to the specified memory buffer.
**
**    Returns:		Next RTC timer handle in the list (-1 if end of list)
**
**    Parameters
**    timerHandle	IN/OUT  Timer handle (0 if start of list)/actual Timer handle
**    timer			OUT  Pointer to the memory buffer
**--------------------------------------------------------------------------*/
BYTE 
CSerialAPI::RTCTimerRead(
	BYTE *timerHandle, 
	RTC_TIMER *timer)
{
    int i;
    BYTE idx=0;
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_RTC_TIMER_READ;
    buffer[idx++] = *timerHandle;

    buffer[0] = idx;	// length
    BYTE byLen = 0;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_RTC_TIMER_READ, buffer, &byLen);

    if (buffer[IDX_DATA+1] != (BYTE)-1)
	{
        timer->status = buffer[IDX_DATA+2];
        timer->timeOn.weekday = buffer[IDX_DATA+3];
        timer->timeOn.hour = buffer[IDX_DATA+4];
        timer->timeOn.minute = buffer[IDX_DATA+5];
        timer->timeOff.weekday = buffer[IDX_DATA+6];
        timer->timeOff.hour = buffer[IDX_DATA+7];
        timer->timeOff.minute = buffer[IDX_DATA+8];
        timer->repeats = buffer[IDX_DATA+9];
        timer->parm = buffer[IDX_DATA+10];
        for (i=0;i<RTC_TIMER_MAX;i++) 
		{
            /* find timer element */
            if (timerRTCArray[i].timerHandle == buffer[IDX_DATA+1])
            {
                timer->func = timerRTCArray[i].func;
                break;
            }
        }
    }
    *timerHandle = buffer[IDX_DATA+1];
    return buffer[IDX_DATA];
}


/*==============================   RTCTimerDelete   =========================
**    Remove the timer element from the RTC timer list.
**
**    Returns:		Nothing
**
**    Parameters
**    timerHandle	IN  Timer handle to remove
**--------------------------------------------------------------------------*/
void 
CSerialAPI::RTCTimerDelete(
	BYTE timerHandle)
{
    int i;
    BYTE idx=0;
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_RTC_TIMER_DELETE;
    buffer[idx++] = timerHandle;

    buffer[0] = idx;	// length
    BYTE byLen = 0;
    SendData(buffer, idx, NULL);

    for (i=0;i<RTC_TIMER_MAX;i++) 
	{
        /* Deregister from timerRTCArray */
        if (timerRTCArray[i].timerHandle == timerHandle)
        {
            timerRTCArray[i].timerHandle = FREE_RTC_TIMER_HANDLE;
            timerRTCArray[i].func = NULL;
            break;
        }
    }
}


/*===========================   ZW_SetLearnMode   ===========================
**    Enable/Disable home/node ID learn mode.
**    When learn mode is enabled, received "Assign ID's Command" are handled:
**    If the current stored ID's are zero, the received ID's will be stored.
**    If the received ID's are zero the stored ID's will be set to zero.
**
**    The learnFunc is called when the received assign command has been handled.
**
**    Returns:		Nothing
**
**    Parameters
**    mode			IN  TRUE: Enable; FALSE: Disable
**    learnFunc		IN  Node learn call back function
**--------------------------------------------------------------------------*/
void 
CSerialAPI::ZW_SetLearnMode(
	BYTE mode, 
	void (*completedFunc)(BYTE bStatus, BYTE bSource, BYTE *pCmd, BYTE bLen))
{
    BYTE idx=0;
    BYTE byCompletedFunc = (completedFunc == NULL? 0: SeqNo());
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_SET_LEARN_MODE;
    buffer[idx++] = mode;
    buffer[idx++] = byCompletedFunc;	// Func id for CompletedFunc

    buffer[0] = idx;	// length
    cbFuncZWSetLearnMode = completedFunc;
    SendData(buffer, idx, NULL);
}


/*===============================   ZW_Version   ============================
**    Get the Z-Wave library basis version
**
**    Returns:		Nothing
**
**    Parameters
**    pBuf			OUT Pointer to buffer where version string will be
**						copied to. Buffer must be at least 14 bytes long.
**--------------------------------------------------------------------------*/
BYTE 
CSerialAPI::ZW_Version(
	BYTE *pBuf)
{
    BYTE idx=0;
    BYTE byLen = 0;
    BYTE retVal = 0;
    static BYTE buffer[BUF_SIZE];
    memset(buffer, 0, sizeof(buffer));
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_GET_VERSION;
    buffer[0] = idx;	// length
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_ZW_GET_VERSION, buffer, &byLen);
	if (memcmp(&buffer[IDX_DATA], "Z-Wave", 6) == 0)
	{
		retVal = buffer[IDX_DATA + 12];
		memcpy(pBuf, &buffer[IDX_DATA], 12);
		pBuf[12] = 0;
	}
    return retVal;
}


/*========================   ZW_GetControllerCapabilities   ========================
**	Get Controller Capabilities
**
**	Returns: TRUE if Controller Capabilities has been received and the specified
**			 variable pointed to by pBControllerCapabilities has been updated accordingly
**			 FALSE if Controller Capabilities could not been determined.
**
** Capabilities flags:
** CONTROLLER_CAPABILITIES_IS_SECONDARY
** CONTROLLER_CAPABILITIES_ON_OTHER_NETWORK
** CONTROLLER_CAPABILITIES_NODEID_SERVER_PRESENT
** CONTROLLER_CAPABILITIES_IS_REAL_PRIMARY
** CONTROLLER_CAPABILITIES_IS_SUC
** CONTROLLER_CAPABILITIES_NO_NODES_INCUDED
**--------------------------------------------------------------------------*/
bool
CSerialAPI::ZW_GetControllerCapabilities(
	BYTE *pbControllerCapabilities)
{
    BYTE idx = 0;
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_GET_CONTROLLER_CAPABILITIES;
    buffer[0] = idx;			// length
    BYTE byLen = 0;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_ZW_GET_CONTROLLER_CAPABILITIES, buffer, &byLen);
	*pbControllerCapabilities = buffer[IDX_DATA];
	return (IDX_DATA < byLen);
}


/*================   SerialAPI_ApplicationNodeInformation   =================
**	Set ApplicationNodeInformation data to be used in subsequent calls to
**	ZW_SendNodeInformation.
**
**	Returns:		Nothing
**
**	Parameters
**	listening		IN  TRUE if this node is always on air
**	nodeType		IN  Device type
**	nodeParm		IN  Device parameter buffer
**	parmLength		IN  Number of Device parameter bytes
**--------------------------------------------------------------------------*/
void 
CSerialAPI::SerialAPI_ApplicationNodeInformation(
	BYTE listening, 
	APPL_NODE_TYPE nodeType, 
	BYTE  *nodeParm, 
	BYTE  parmLength)
{
    int i;
    BYTE idx=0;
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_SERIAL_API_APPL_NODE_INFORMATION;
    buffer[idx++] = listening;
    buffer[idx++] = nodeType.generic;
    buffer[idx++] = nodeType.specific;
    buffer[idx++] = parmLength;
    for (i = 0; i < parmLength; i++)
    {
        buffer[idx++] = nodeParm[i];
    }
    buffer[0] = idx;			// length
    SendData(buffer, idx, NULL);
}


/*========================   SerialAPI_GetInitData   ========================
**	Get Serial API initialization data from remote side (Enhanced Z-Wave module)
**
**	Returns:		Nothing
**
**	Parameters
**	ver				OUT  Remote sides Serial API version
**  capabilities	OUT  Capabilities flag (GET_INIT_DATA_FLAG_xxx)
**	len				OUT  Number of bytes in nodesList
**	nodesList		OUT  Bitmask list with nodes known by Z-Wave module
**
** Capabilities flag:
** Bit 0: 0 = Controller API; 1 = Slave API
** Bit 1: 0 = Timer functions not supported; 1 = Timer functions supported.
** Bit 2: 0 = Primary Controller; 1 = Secondary Controller
** Bit 3: 0 = Not SUC; 1 = This node is SUC (static controller only)
** Bit 3-7: reserved
** Timer functions are: TimerStart, TimerRestart and TimerCancel.
**--------------------------------------------------------------------------*/
BOOL 
CSerialAPI::SerialAPI_GetInitData(
	BYTE *ver, 
	BYTE *capabilities, 
	BYTE *len, 
	BYTE *nodesList)
{
    int i;
    BYTE idx=0;
    static BYTE buffer[BUF_SIZE];
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_SERIAL_API_GET_INIT_DATA;
    buffer[0] = idx;			// length
    BYTE byLen = 0;
    *ver = 0;
    *capabilities = 0;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_SERIAL_API_GET_INIT_DATA, buffer, &byLen);
    *ver = buffer[IDX_DATA];

    //controller api eller slave api
    *capabilities = buffer[IDX_DATA + 1];
    *len = buffer[IDX_DATA+2];
    for (i = 0; i < buffer[IDX_DATA + 2]; i++)
    {
        nodesList[i] = buffer[IDX_DATA + 3 + i];
    }
    // Bit 2 tells if it is Primary Controller (FALSE) or Secondary Controller (TRUE).
    if ((*capabilities) & GET_INIT_DATA_FLAG_SECONDARY_CTRL)
    {
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}

/*========================   ZW_GetBackgroundRSSI   =========================
**    Read the background RSSI for all channels (2 or 3 depending on firmware)
**
**    Returns:		Nothing
**
**    Parameters
**    buf   		  OUT   Buffer pointer to where the data is placed
**                          Contains 2 or 3 bytes of RSSI data for 2 or 3-
**                          channel systems repspectively.
**    bLen            OUT   Length of buf
**--------------------------------------------------------------------------*/
void 
CSerialAPI::ZW_GetBackgroundRSSI( 
	BYTE *buf,
	BYTE *bLen)
{
    int i;
    BYTE idx=0;
    static BYTE buffer[BUF_SIZE];
	BYTE rssiDataLen;
	/*  bNodeID | bRemoveBad | bRemoveNonReps | funcID */
    buffer[idx++] = 0;
    buffer[idx++] = REQUEST;
    buffer[idx++] = FUNC_ID_ZW_GET_BACKGROUND_RSSI;
    buffer[0] = idx;
    BYTE byLen = 0;
    SendDataAndWaitForResponse(buffer, idx, FUNC_ID_ZW_GET_BACKGROUND_RSSI, buffer, &byLen);
	rssiDataLen = byLen -  3;
	for (i = 0; i < rssiDataLen; i++) 
	{
        buf[i] = buffer[IDX_DATA+i];
    }
	*bLen = rssiDataLen;
}