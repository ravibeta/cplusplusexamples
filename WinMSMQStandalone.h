#ifdef WIN32
#ifndef _WIN_MSMQ_MONITOR_H_
#define _WIN_MSMQ_MONITOR_H_ 1
#define SECURITY_WIN32 1
#include "Windows.h" 
#include <mq.h>
#include "Thread.h"
#include "stdafx.h"
#include "AtlBase.h"
#pragma comment (lib, "Mqrt.lib")

class WinMSMQMonitorThread;
class CScopedMSMQWrapper;

struct RECEIVE_CONTEXT
{
    OVERLAPPED ov;                  // OVERLAPPED structure
    HANDLE hQueue;                  // Queue handle
    MQMSGPROPS *pmsgprops;          // Message properties structure
};

const int NUMBEROFTHREADS=2;
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
/*
struct cfg
{
	Str _queue;
	Str _index;
	Str _stanza;
};*/

// private:
    bool StartMonitorWorker(int startIndex, int endIndex);
    /*void MonitorWorker(int startIndex, int endIndex);*/
    void processData(const UCHAR * pbE, ULONG totalSize);
    // void processEvent(const Str & item, const Str & index, const Str & stanza, const Str & now);
    // bool configure();
	bool CreateQueues();
	bool AssociateQueuesToPort();
	
	HRESULT WinMSMQMonitor::EnableAsyncReceive(RECEIVE_CONTEXT* prc);
	/*DWORD WinMSMQMonitor::CompletionPortThread();*/
	bool WinMSMQMonitor::AssociateQueues();

//private:

    WinMSMQMonitorThread*                  	   _threads[1024];
    ULONG					_threadCount;
    HANDLE                                      _completionPort;
	HANDLE										_readerPort;
    volatile LONG                               _isShuttingDown;
    CScopedMSMQWrapper*                         _queues[1024];
    ULONG                                       _wrapperCount;
    // Str*                                        _resolvedQueueNames[1024];
	// cfg                                         _queueSpec[1024];
	ULONG                                       _queueCount;
    ULONG										_nNames;
    // ModInputEventWriter                         _writer;
    friend class WinMSMQMonitorThread;


};

extern DWORD WINAPI CompletionPortThread(LPVOID lParam);
class WinMSMQMonitorThread 
{
public:
    WinMSMQMonitorThread (WinMSMQMonitor* monitor = NULL); 
	~WinMSMQMonitorThread()
	{ 
		if (hThread != NULL)
			CloseHandle(hThread);
	}

    void start(WinMSMQMonitor *WinMSMQMonitor, int resolvedQIndexStart, int resolvedQIndexEnd);

//private:
    //virtual void main();

    WinMSMQMonitor *_WinMSMQMonitor;
	int startQIndex;
	int endQIndex;
	HANDLE hThread;
	DWORD dwThreadId;
};


struct pkt
{
  int _index;
  int len;
  BYTE* data;
};


class CScopedMSMQWrapper
{
	QUEUEHANDLE hQ;
	
	bool transactional;

public:
	ULONG indexQSpec;
	void* buffer;
	size_t len;
	bool sent;
	DWORD timeout;
	BYTE lastMD5Hash[16];
	

	CScopedMSMQWrapper(int index = 0)
	{
		hQ = INVALID_HANDLE_VALUE;
		timeout = 0;
		sent = FALSE;
		transactional = FALSE;
		indexQSpec = index;
                buffer = NULL;
                len = 0;
	}

	~CScopedMSMQWrapper()
	{
        CloseQueue();
        if (buffer != NULL)
        { 
            delete buffer;
            buffer = NULL;
        }
	}
	inline QUEUEHANDLE getHandle() { return hQ; }
	//inline setTimeout(DWORD t) { timeout = t;}
	//inine DWORD getTimeout(){ return timeout; }
	/*inline setBuffer(void* bfr, size_t l) { buffer = bfr; len = l;}
	inline getBufferLen() { return len; }
	inline getBuffer() { return buffer; }
	inline setsent(bool fsent) { sent = fsent; }
	inline getsent() { return sent; }*/
	HRESULT InitQueue(const WCHAR* wczQueueName, size_t qLen, const WCHAR* wczMachineName, size_t mLen, DWORD dwAccessMode, DWORD dwSharedMode);
        void CloseQueue();
	HRESULT CScopedMSMQWrapper::ReadQueue(VOID** pLast, ULONG* readLen, ULONG* bytesRead);
	HRESULT SendQueue(const WCHAR* wczPayload, size_t nLen);
	MQMSGPROPS* GetMsgProps();
	void FreeMsgProps(MQMSGPROPS* pMsgProps, int numProperties);
	
    friend class WinMSMQMonitor;
};


#endif
#endif
