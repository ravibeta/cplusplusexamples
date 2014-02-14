#ifndef _WIN_MSMQ_MONITOR_H_
#define _WIN_MSMQ_MONITOR_H_ 1
#define SECURITY_WIN32 1
#include <windows.h>
#include <stdio.h>
#include <Mq.h>
#include "Thread.h"



class WinMSMQMonitorThread;


/*
 *  MSMQ monitor
 */
class WinMSMQMonitor
{
public:
    WinMSMQMonitor();
    virtual ~WinMSMQMonitor(void);


    bool init(void);
    bool run(void);
    void stop(void);




private:
    bool StartMonitorWorker(void);
    void MonitorWorker(void);
    void processData(UCHAR * pbE, ULONG totalSize);


private:


    WinMSMQMonitorThread *                  	_thread;
    HANDLE                                      _completionPort;
    volatile LONG                               _isShuttingDown;
    HANDLE                                      _shutDownEvt;


    friend class WinMSMQMonitorThread;


};


class WinMSMQMonitorThread : public Thread
{
public:
    WinMSMQMonitorThread () : _WinMSMQMonitor (NULL) {}


    void start(WinMSMQMonitor *WinMSMQMonitor);


private:
    virtual void* run();


    WinMSMQMonitor *_WinMSMQMonitor;
};




class CScopedMSMQWrapper
{
	QUEUEHANDLE hQ;


public:
	CScopedMSMQWrapper()
	{
		hQ = INVALID_HANDLE_VALUE;
	}


	~CScopedMSMQWrapper()
	{
            CloseQueue();
	}


	HRESULT InitQueue(const WCHAR* wczQueueName, size_t qLen, const WCHAR* wczMachineName, size_t mLen, DWORD dwAccessMode, DWORD dwSharedMode);
        void CloseQueue();
	HRESULT CScopedMSMQWrapper::ReadQueue(VOID** pLast, ULONG* readLen, ULONG* bytesRead);
	HRESULT SendQueue(const WCHAR* wczPayload, size_t nLen);
	MQMSGPROPS* GetMsgProps();
	void FreeMsgProps(MQMSGPROPS* pMsgProps, int numProperties);
};


#endif

