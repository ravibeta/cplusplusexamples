#include "stdafx.h"
#include "Windows.h"
#include "WinMSMQ.h"
// #import "mqoa.dll" named_guids // no_namespace
#pragma comment (lib, "Mqrt.lib")
#include "mq.h"

WinMSMQMonitor::WinMSMQMonitor():
    _isShuttingDown(0), 
    _completionPort(NULL),
    _shutDownEvt(NULL)
{
}

WinMSMQMonitor::~WinMSMQMonitor(void)
{
}



/*
 * WinMSMQMonitor::init - Initializes a bunch of things before msmqmon is started.
 */
bool
WinMSMQMonitor::init(void)
{
    bool bSuccess = true;

    return bSuccess;
}


/*
 * WinMSMQMonitor::start
 */
bool
WinMSMQMonitor::run(void)
{
    bool bSuccess = false;
    bool bProcess = false;

    _completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
    bSuccess = (NULL != _completionPort);

    if (!bSuccess)
    {
        goto exit;
    }


    _shutDownEvt = CreateEvent(NULL, TRUE, FALSE, NULL);
    bSuccess = (NULL != _shutDownEvt);

    if (!bSuccess)
    {
        goto exit;
    }


    bSuccess = StartMonitorWorker();

    if (!bSuccess)
    {
        goto exit;
    }


    {
        while(!_isShuttingDown)
        {
            DWORD       numBytes = 0;
            OVERLAPPED* pBlock = NULL;
            ULONG_PTR   key = NULL;

            if (GetQueuedCompletionStatus(_completionPort, &numBytes, &key, &pBlock, 500))
            {
                if (numBytes)
                {
                    processData((UCHAR*)pBlock, numBytes);
                }
                else
                {
                    printf( "%s(): Data block size is 0", __FUNCTION__);
                }

                delete [] pBlock; 
            }
        }
    }
exit:
    printf( "%s(): MSMQ Monitoring is stopping", __FUNCTION__);

    stop();


    if (_shutDownEvt)
    {
        WaitForSingleObject(_shutDownEvt, INFINITE);
        CloseHandle(_shutDownEvt);
    }

    printf( "%s(): MSMQ Monitoring is shutting down", __FUNCTION__);

    if (_completionPort)
    {
        CloseHandle(_completionPort);
    }


    return bSuccess;
}

/*
 * WinMSMQMonitor::stop
 */
void
WinMSMQMonitor::stop(void)
{
    InterlockedExchange(&_isShuttingDown, 1);
}

/*
 * WinMSMQMonitor::processData
 */
void
WinMSMQMonitor::processData(UCHAR * pbE, ULONG totalSize)
{
	printf( "%s(): MSMQ Monitoring is processing data", __FUNCTION__);
}

/*
 * WinMSMQMonitor::StartMonitorWorker
 */
bool
WinMSMQMonitor::StartMonitorWorker(void)
{
    printf( "%s()", __FUNCTION__);

    bool bSuccess = true;

    if (bSuccess)
    {
        printf( "%s(): Create WinMSMQ monitoring thread", __FUNCTION__);

        _thread = new WinMSMQMonitorThread();
        bSuccess = _thread != NULL;

        if (!bSuccess)
        {
            DWORD dwErr = GetLastError();
            printf("%s: Failed to create the MSMQ monitor worker thread: '(%d)%m'", __FUNCTION__, dwErr);
        }
    }

    if (bSuccess)
    {
        _thread->start(this);
        printf( "%s(): MSMQ monitor worker thread started", __FUNCTION__);
    }

    return bSuccess;
}


/*
 * WinMSMQMonitor::MonitorWorker
 */
void
WinMSMQMonitor::MonitorWorker(void)
{
    printf( "%s()", __FUNCTION__);

    HRESULT hr = S_OK;
    DWORD outSize = 0;


    UCHAR * pBlock = NULL;
    ULONG BlockSize = 4096;
  
        if (!pBlock)
        {
            pBlock = new UCHAR[BlockSize];
            hr = (NULL != pBlock) ? S_OK : E_OUTOFMEMORY;

            if (!pBlock)
            {
                printf( "%s(): Failed to allocate block memory", __FUNCTION__);
            }
        }
 
        CScopedMSMQWrapper* queue = new CScopedMSMQWrapper();
		LPWSTR wszComputerName = new WCHAR[256];
		DWORD nLen = 256;
		memset(wszComputerName, 0, 256);
		if (GetComputerName(wszComputerName, &nLen) == FALSE)
		{
				   goto exit;
		}

		LPCWSTR wczQueueName = L"private$\\Sample"; 
		hr = queue->InitQueue(wczQueueName, wcslen(wczQueueName), wszComputerName, nLen, MQ_RECEIVE_ACCESS, MQ_DENY_NONE);
        if (FAILED(hr))
        {
               goto exit;
        }
    do
    {

            outSize = 0;

            printf( "%s(): Worker issuing ReceiveQueue with buffer size %d", __FUNCTION__, BlockSize);
           
        hr = queue->ReadQueue();
        if (FAILED(hr))
        {
               goto exit;
        }
 
            // expected outSize != 0
            printf("%s(): ReceiveQueue returns with payload size = %d", __FUNCTION__, outSize);

           // memcompare the message received with the last message, if its the same message we are peeking at it again
           // the same message could be requeued for processing but there will only be one copy on the queue


        if (_isShuttingDown)
        {
            printf( "%s(): Shutdown flag is set. Shutting down", __FUNCTION__);
        }

        if (SUCCEEDED(hr))
        {
            if (outSize > 0)
            {
                        if (!PostQueuedCompletionStatus(_completionPort, outSize, NULL, (LPOVERLAPPED)pBlock))
                        {
                            DWORD dwError = GetLastError();
                            printf( "%s(): PostQueuedCompletionStatus() failed with error %d", __FUNCTION__, dwError);
                            hr = HRESULT_FROM_WIN32(dwError);
                        }
            }
            else
            {
                printf( "%s(): Wait a bit before issuing ReceiveQueue again", __FUNCTION__);

                Sleep(1000);
            }
        }
    }
    while(SUCCEEDED(hr) && !_isShuttingDown);
exit:
    if (pBlock)
    {
        delete [] pBlock;
		delete queue;
    }

    printf("%s: Monitor Worker Exiting...", __FUNCTION__);

    stop();

    SetEvent(_shutDownEvt);

    return;
}


/*
 * WinMSMQMonitorthread::main - Entrypoint for the thread
 */
void
WinMSMQMonitorThread::main(void)
{
    _WinMSMQMonitor->MonitorWorker();
}

/*
 * WinMSMQMonitorthread::start - Calls Thread::go and begins the thread execution.
 */
void
WinMSMQMonitorThread::start(WinMSMQMonitor *WinMSMQMonitor)
{
    _WinMSMQMonitor = WinMSMQMonitor;
    main();
}

HRESULT CScopedMSMQWrapper::InitQueue(const WCHAR* wczQueueName, size_t qLen, const WCHAR* wczMachineName, size_t mLen, DWORD dwAccess, DWORD dwShareMode)
{
	HRESULT hr = MQ_ERROR_INVALID_PARAMETER;

	if (wczQueueName == NULL)
		return hr;
	  // Create a direct format name.
	  WCHAR * wszPrefix = L"DIRECT=OS:";
	  WCHAR * wszFormatName = NULL;
	  DWORD dwFormatNameLength = 0;
	  dwFormatNameLength = wcslen(wczQueueName) + wcslen(wczMachineName) + 12;
	  wszFormatName = new WCHAR[dwFormatNameLength];
	  if (wszFormatName == NULL)
	  {
		return MQ_ERROR_INSUFFICIENT_RESOURCES;
	  }
	  memset(wszFormatName, 0, dwFormatNameLength);
	  wcscat_s(wszFormatName, dwFormatNameLength, wszPrefix);
	  wcscat_s(wszFormatName, dwFormatNameLength, wczMachineName);
	  wcscat_s(wszFormatName, dwFormatNameLength, L"\\");
	  wcscat_s(wszFormatName, dwFormatNameLength, wczQueueName);


	hr = MQOpenQueue(wszFormatName,
					 dwAccess,
					 dwShareMode,
					 &hQ);
	if (FAILED(hr))
	{
		return hr;
	}

	return hr;
}

HRESULT CScopedMSMQWrapper::ReadQueue()
{
	MQMSGPROPS* pmsgProps = GetMsgProps();
	HRESULT hr = E_FAIL;

	for(;;)
	{
		hr = MQReceiveMessage(hQ, 1000, MQ_ACTION_PEEK_CURRENT, pmsgProps, NULL, NULL, NULL, MQ_NO_TRANSACTION);

		if (hr == MQ_ERROR_BUFFER_OVERFLOW)
		{
			VOID* pBuffer = pmsgProps->aPropVar[3].caub.pElems;
			ULONG ulBufferSize = pmsgProps->aPropVar[2].ulVal * sizeof(UCHAR);
			delete pBuffer;
			pBuffer = NULL;
			pBuffer = (VOID*)malloc(ulBufferSize);
			pmsgProps->aPropVar[3].caub.pElems = (UCHAR*)pBuffer;
			pmsgProps->aPropVar[3].caub.cElems = ulBufferSize;
			if(pBuffer == NULL)
			{
				return MQ_ERROR_INSUFFICIENT_RESOURCES;
			}

			memset(pBuffer, 0, ulBufferSize);
			continue;
		}

		if (FAILED(hr))
		{
			goto exit;
		}

		break;
	}

exit:
	FreeMsgProps(pmsgProps, 5);
	if (FAILED(hr))
	{
		return hr;
	}
	return MQ_OK;
}

void CScopedMSMQWrapper::CloseQueue()
{
		HRESULT hr;
		if ( hQ != INVALID_HANDLE_VALUE)
		{
			hr = MQCloseQueue(hQ);
                        if (FAILED(hr))
                        {
                            // try again
                            MQCloseQueue(hQ);
                              
                        }
                     hQ = INVALID_HANDLE_VALUE;
		}
}

HRESULT CScopedMSMQWrapper::SendQueue(const WCHAR* wczPayload, size_t len)
{
	HRESULT hr = E_FAIL;
	MQMSGPROPS* pmsgProps = GetMsgProps();
	VOID* pBuffer = pmsgProps->aPropVar[3].caub.pElems;
	ULONG ulBufferSize = pmsgProps->aPropVar[3].caub.cElems;

	if (ulBufferSize < len * sizeof(UCHAR))
	{
		ulBufferSize = (len + 1) * sizeof(UCHAR);
		delete pBuffer;
		pBuffer = NULL;
		pBuffer = (VOID*)malloc(ulBufferSize);		
		pmsgProps->aPropVar[3].caub.pElems = (UCHAR*) pBuffer;
		pmsgProps->aPropVar[3].caub.cElems = ulBufferSize;
		if(pBuffer == NULL)
		{
			hr =  MQ_ERROR_INSUFFICIENT_RESOURCES;
			goto exit;
		}
		memset(pBuffer, 0, ulBufferSize);
	}
	memcpy_s(pBuffer, ulBufferSize, wczPayload, len*sizeof(WCHAR));

	hr = MQSendMessage(hQ, pmsgProps, NULL);
exit:
	FreeMsgProps(pmsgProps, 5);
	return hr;
}

MQMSGPROPS* CScopedMSMQWrapper::GetMsgProps()
{
	bool fSuccess = FALSE;
	const int NUMBEROFPROPERTIES = 5;
	MQMSGPROPS* pmsgProps = NULL;
	MSGPROPID* pMsgPropId = NULL;
	MQPROPVARIANT* pMsgPropVar = NULL;

	pmsgProps = new MQMSGPROPS;
	if (pmsgProps == NULL)
		goto exit;
	memset(pmsgProps, 0, sizeof(MQMSGPROPS));
	

	pMsgPropId = new MSGPROPID[NUMBEROFPROPERTIES];
	if (pMsgPropId == NULL)
		goto exit;	
	pmsgProps->aPropID = pMsgPropId;
	memset(pMsgPropId, 0, sizeof(MSGPROPID[NUMBEROFPROPERTIES]));


	pMsgPropVar = new MQPROPVARIANT[NUMBEROFPROPERTIES];
	if (pMsgPropVar == NULL)
		goto exit;	
	pmsgProps->aPropVar = pMsgPropVar;
	memset(pMsgPropVar, 0, sizeof(MQPROPVARIANT[NUMBEROFPROPERTIES]));


	HRESULT* pMsgStatus;
	pMsgStatus = new HRESULT[NUMBEROFPROPERTIES];
	if (pMsgStatus == NULL)
		goto exit;	
	pmsgProps->aStatus = pMsgStatus;
	memset(pMsgStatus, 0, sizeof(HRESULT[NUMBEROFPROPERTIES]));
	
	DWORD cPropId = 0;
	pMsgPropId[cPropId] = PROPID_M_LABEL_LEN;
	pMsgPropVar[cPropId].vt = VT_UI4;
	pMsgPropVar[cPropId].ulVal = MQ_MAX_MSG_LABEL_LEN;
	cPropId++;

	PWCHAR pwszLabelBuffer = new WCHAR[MQ_MAX_MSG_LABEL_LEN];
	if (pwszLabelBuffer == NULL)
		goto exit;
	pMsgPropId[cPropId] = PROPID_M_LABEL;
	pMsgPropVar[cPropId].vt = VT_LPWSTR;
	pMsgPropVar[cPropId].pwszVal = pwszLabelBuffer;
	cPropId++;

	pMsgPropId[cPropId] = PROPID_M_BODY_SIZE;
	pMsgPropVar[cPropId].vt = VT_NULL;
	cPropId++;

	ULONG ulBufferSize = 256;
	VOID* pBuffer = (VOID*) malloc(ulBufferSize);
	if (pBuffer == NULL)
		goto exit;
	memset(pBuffer, 0, ulBufferSize);
	pMsgPropId[cPropId] = PROPID_M_BODY;
	pMsgPropVar[cPropId].vt = VT_VECTOR | VT_UI1;
	pMsgPropVar[cPropId].caub.pElems = (UCHAR*) pBuffer;
	pMsgPropVar[cPropId].caub.cElems = ulBufferSize;
	cPropId++;


	pMsgPropId[cPropId] = PROPID_M_BODY_TYPE;
	pMsgPropVar[cPropId].vt = VT_NULL;
	cPropId++;
	
	pmsgProps->cProp = cPropId;
	fSuccess = true;

exit:
	if (fSuccess == FALSE)
	{
		FreeMsgProps(pmsgProps, NUMBEROFPROPERTIES);
	}
	return pmsgProps;
}

void CScopedMSMQWrapper::FreeMsgProps(MQMSGPROPS* pMsgProps, int numProperties)
{
	if (pMsgProps != NULL)
	{
		delete [] pMsgProps->aPropID;

		MQPROPVARIANT* props = pMsgProps->aPropVar;
		if (props != NULL)
		{
			for (int i = 0; i < numProperties; i++)
			{
				MQPROPVARIANT pv = props[i];
				if (pv.vt == VT_LPWSTR) delete pv.pwszVal;
				if (pv.vt == (VT_VECTOR | VT_UI1)) delete pv.caub.pElems;				
			}
			delete[] props;
		}
		delete[] pMsgProps->aStatus;
	}
}
