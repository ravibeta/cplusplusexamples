//#ifdef WIN32
//#define WIN32_LEAN_AND_MEAN 1
#include "stdafx.h"
#include <stdio.h>
#include "Windows.h"
#include "WinMSMQStandalone.h"
#pragma comment (lib, "MqRt.lib")
#include "Mq.h"

#define MSMQ_MAX_SIZE_OF_EVENT_TIMESTAMP        256
static const LPSTR SRC  = "WinMSMQ";
static const LPSTR COLLECTION_PREFIX = "winmsmqmon:";

WinMSMQMonitor::WinMSMQMonitor():
    _isShuttingDown(0), 
    _completionPort(NULL),
    _threadCount(0),
    _queueCount(0)
//	_writer()
{
	memset(_threads, 0, 1024 * sizeof(WinMSMQMonitorThread*));
	memset(_queues, 0, 1024 * sizeof(CScopedMSMQWrapper*));	
}

WinMSMQMonitor::~WinMSMQMonitor(void)
{
        for (int i = 0; i < 1024; i++)
        {
              delete _queues[i];
              delete _threads[i];
        }
}

/*
 * WinMSMQMonitor::init - Initializes a bunch of things before msmqmon is started.
 */
bool
WinMSMQMonitor::init(void)
{
    bool bSuccess = true;
    
    // bSuccess = configure();   

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
	HRESULT hr = E_FAIL;
	_queueCount = 1; // TODO: remove this Temporary override; and to limit to 1024.

	// create one completion port for monitor to interact with queue readers
    _completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
    bSuccess = (NULL != _completionPort);

	// create another completion port for queue readers to read different queues
	_readerPort = CreateIoCompletionPort(
              INVALID_HANDLE_VALUE,  // Do not associate a queue handle.
              NULL,                  // New port
              0,                     // No completion key
              NUMBEROFTHREADS        // Number of concurrently executed threads
              );
  

    if (!bSuccess)
    {
        goto exit;
    }

	bSuccess = CreateQueues();
	if (!bSuccess)
	{
		goto exit;
	}

	bSuccess = AssociateQueuesToPort();
	if (!bSuccess)
	{
		goto exit;
	}

	  // Initiate several overlapped receive operations.
  RECEIVE_CONTEXT* prc[1024];
  for (ULONG i = 0; i < _wrapperCount; i++) 
  {
	CScopedMSMQWrapper* queue = _queues[i];
	if (queue == NULL) continue;

    // Allocate and initialize a receive context.
    prc[i] = new RECEIVE_CONTEXT;
    if (prc[i] == NULL)
    {
      hr =  MQ_ERROR_INSUFFICIENT_RESOURCES;
	  goto exit;
    }
    memset((void*)prc[i], 0, sizeof(RECEIVE_CONTEXT));
    prc[i]->hQueue = queue->getHandle();
	prc[i]->pmsgprops = queue->GetMsgProps();
    if (prc[i]->pmsgprops == NULL)
    {
      hr =  MQ_ERROR_INSUFFICIENT_RESOURCES;
	  goto exit;
    }

    hr = EnableAsyncReceive(prc[i]);
    if (FAILED(hr))
    {
      break;
    }
  }

  
	int numQueuesPerWorker = 1;
	for (int i = 0 ; i < (int)_queueCount / numQueuesPerWorker; i++) // TODO: Assuming queueCount is for resolved discrete queues
	{
		int startIndex = i * numQueuesPerWorker;
		int endIndex = startIndex + numQueuesPerWorker;

		bSuccess = StartMonitorWorker(startIndex, endIndex);
		if (!bSuccess)
		{
			goto exit;
		}
	}

    {
        while(!_isShuttingDown && _completionPort)
        {
            DWORD       numBytes = 0;
            OVERLAPPED* pBlock = NULL;
            ULONG_PTR   key = NULL;


            if (GetQueuedCompletionStatus(_completionPort, &numBytes, &key, &pBlock, 500))
            {
                if (numBytes)
                {
					// call process data but 
					// eliminate duplicates
					CScopedMSMQWrapper* queue = _queues[key];
					ULONG len = (ULONG)queue->len;
					ULONG nRead = len < numBytes ? len : numBytes;
					if (memcmp(queue->buffer, pBlock, nRead) != 0)
					{
						processData(reinterpret_cast<const UCHAR*>(pBlock), numBytes);
						memcpy_s(queue->buffer, len, pBlock, numBytes);
					}
                }
                else
                {
                    printf( "%s(): Data block size is 0", __FUNCTION__);
                }
            }
        }
    }
	bSuccess = true;
exit:
    printf( "%s(): MSMQ Monitoring is stopping", __FUNCTION__);


    stop();


    printf( "%s(): MSMQ Monitoring is shutting down", __FUNCTION__);

  // Free the memory allocated for the RECEIVE_CONTEXT structures.
  for ( ULONG i = 0; i < _wrapperCount; i++)
  {
	CScopedMSMQWrapper* queue = _queues[i];
	if (queue != NULL)
	{
		queue->FreeMsgProps(prc[i]->pmsgprops, 5);	
		delete queue;
	}
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
    if (_completionPort)
    {
        CloseHandle(_completionPort);
		_completionPort = NULL;
    }

	if (_readerPort)
	{
		CloseHandle(_readerPort);
		_readerPort = NULL;
	}

}
/*
bool WinMSMQMonitor::configure()
    printf("Read configuration for WinMSMQMon");
    try
    {
        ConfigXmlStreamingParser confParser;

        confParser.readFromStdin();

        const std::map<Str, PropertiesMap> & props = confParser.getConf();

        std::map<Str, PropertiesMap>::const_iterator it(props.begin());
        std::map<Str, PropertiesMap>::const_iterator end(props.end());

		_queueCount = 0;
        for (; it != end; ++it) // we need not iterate, there's only one for now
        {

            if (it->first == "default" || it->second.isDisabled())
            {
                continue;
            }

            const PropertiesMap& toMonitor = it->second;

            Str stanzaName = it->first;

            Str collection = stanzaName.asLowercase();
            if (collection.compare(0, COLLECTION_PREFIX.length(), COLLECTION_PREFIX) != 0)
            {
                gLogger.info("Found a stanza named '%s' without the WinMSMQMon prefix, this will not be processed.", stanzaName.c_str());
                continue;
            }

            Str stanza = ConfigXmlStreamingParser::removeStanzaPrefix(stanzaName).asLowercase();

            if (stanza.empty())
            {
                TraceAndLogError("Found a stanza named '%s' without valid queue, this will not be processed.", stanzaName.c_str());
                continue;
            }
			int count = _queueCount;
			_queueCount++;
			_queueSpec[count]._queue = stanza; //TODO: not require user to give individual queue name
            _queueSpec[count]._queue = stanza;
            _queueSpec[count]._index = toMonitor.get("index"); // TODO: use default if none is provided.
            _queueSpec[count]._stanza = stanzaName;
        }


    }
    catch (const ModularInputsException &e)
    {
        gLogger.error("Failed to read configuration: %s", e.what());

        return false;
    }
    catch(const ComponentException& e)
    {
        gLogger.error("Failed to read configuration: %s", e.what());

        return false;
    }
    catch(const std::exception& e)
    {
        gLogger.error("Failed to read configuration: %s", e.what());

        return false;
    }

    return true;
}  
*/

/*
 * WinMSMQMonitor::processData
 */
void
WinMSMQMonitor::processData(const UCHAR * pbE, ULONG totalSize)
{
	printf( "%s(): MSMQ Monitoring is processing data : %s", __FUNCTION__, pbE);
        //Str msmqEvtData;
        //Str tmpIndex;
        //Str tmpStanza;
        //Str temp((const wchar *) pbE, totalSize);
        //int qindex = (int)(*((UCHAR*)pbE));
        //tmpIndex = cfg[i]._index;
        //tmpQueue = cfg[i]._queue;
        //tmpStanza = cfg[i]._stanza;
        //msmqEvtData.appendFormat(
        //                                "data=\"%s\"\n",
        //                                temp.c_str()));
        //CurrentTimeval now;
        //processEvent(msmqEvtData, tmpIndex, tmpStanza, now);
}

/*
 * WinRegistryMonitor::processEvent
 */
//void
//WinMSMQMonitor::processEvent(const Str & item, const Str & index, const Str & stanza, const Str & now)
//{
//    ModInputEventWriter::Event evt;
//    evt._stanza = stanza;
//    evt._index = index;
//
//    evt._source = SRC;
//    evt._sourcetype = SRC;
//
//    evt._data.appendFormat("%s\n%s", now, item.c_str());
//    evt._data.trimInPlace(CharacterClass::ALL_WHITESPACES);
//
//    _writer.writeXml(evt);
//
//    postEvent(item);
//}

/*
 * WinMSMQMonitor::StartMonitorWorker
 */
bool
WinMSMQMonitor::StartMonitorWorker(int startIndex, int endIndex)
{
    printf( "%s()", __FUNCTION__);
    ULONG nPos = _threadCount;	
    InterlockedIncrement(&_threadCount);

    bool bSuccess = FALSE;
    printf( "%s(): Create WinMSMQ monitoring thread", __FUNCTION__);

    nPos = _threadCount;
    _threads[_threadCount] = new WinMSMQMonitorThread(this);
    bSuccess = _threads[_threadCount] != NULL;

    if (!bSuccess)
    {
        DWORD dwErr = GetLastError();
        printf("%s: Failed to create the MSMQ monitor worker thread: '(%d)%m'", __FUNCTION__, dwErr);
        goto exit;
    }

    _threads[nPos]->start(this, startIndex, endIndex);
    printf( "%s(): MSMQ monitor worker thread started", __FUNCTION__);
    if (bSuccess == FALSE)
    {
        goto exit; 
    }

exit:
    if (!bSuccess)
    {
          if (_threads[nPos] != NULL) 
          {
             delete _threads[nPos];
             _threads[nPos] = NULL;
          }
    }
    return bSuccess;
}

bool WinMSMQMonitor::CreateQueues()
{
	bool bSuccess = FALSE;
	int startIndex = 0;
	int endIndex = _queueCount;
	HRESULT hr = E_FAIL;

	    DWORD outSize = 0;
	_wrapperCount = startIndex;

	for (int i = startIndex; i < endIndex;  i++)
	{
		int count = _wrapperCount;
		InterlockedIncrement(&_wrapperCount);

		CScopedMSMQWrapper* queue = new CScopedMSMQWrapper(i);
		if (queue == NULL)
		{
			 hr = E_OUTOFMEMORY;
			 goto exit;
		}
		_queues[count] = queue;
		

		VOID * pBlock = NULL;
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
		
		if (FAILED(hr))
		{
			goto exit;
		}
		memset(pBlock, 0, BlockSize);
		queue->buffer = pBlock;
		queue->len = BlockSize;
		queue->indexQSpec = i;
		
		LPWSTR wszComputerName = new WCHAR[256];
		DWORD nLen = 256;
		memset(wszComputerName, 0, 256);
		if (GetComputerName(wszComputerName, &nLen) == FALSE)
		{
			goto exit;
		}

		// Win32WideStr name(_queueSpec[i]._queue); // TODO: remove this sample
		LPCWSTR name = L"private$\\Sample";
		hr = queue->InitQueue(name, wcslen(name), wszComputerName, nLen, MQ_RECEIVE_ACCESS, MQ_DENY_NONE);
		if (FAILED(hr))
		{
			goto exit;
		}
	}
	bSuccess = true;
exit:
	if(FAILED(hr))
	{
		for (int i = startIndex; i < endIndex; i++)
		{
			if (_queues[i] != NULL) _queues[i]->CloseQueue();
			delete _queues[i];
			_queues[i] = NULL;
		}
	}
	return bSuccess;
}

bool WinMSMQMonitor::AssociateQueuesToPort()
{
	bool bSuccess = FALSE;
  int cThread = 0, cReceive = 0;
  const int NUMBEROFRECEIVES = 4;


  for (ULONG i = 0; i < _wrapperCount; i++)
  {
	  CScopedMSMQWrapper* queue = _queues[i];
	  HANDLE hQueue = queue->getHandle();
	  if ( queue == NULL || hQueue == NULL || hQueue == INVALID_HANDLE_VALUE) continue;

  _readerPort = CreateIoCompletionPort(
                            hQueue,           // Queue handle
                            _readerPort,            // Port handle
                            queue->indexQSpec,                // No completion key
                            NUMBEROFTHREADS
                            );
		if (_readerPort == NULL)
		{
		printf("Could not associate queue with the completion port due to windows error : %d", GetLastError());
		goto exit;
		}

  }
  bSuccess = TRUE;

exit:
  return bSuccess;
}



/*
 * WinMSMQMonitorthread::start - Calls Thread::go and begins the thread execution.
 */
void
WinMSMQMonitorThread::start(WinMSMQMonitor *winMSMQMonitor, int resolvedQIndexStart, int resolvedQIndexEnd)
{
    _WinMSMQMonitor = winMSMQMonitor;
	startQIndex = resolvedQIndexStart;
	endQIndex = resolvedQIndexEnd;
 //   Thread::start();
	if (hThread != NULL)
		ResumeThread(hThread);
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

HRESULT CScopedMSMQWrapper::ReadQueue(VOID** pLast, ULONG* readLen, ULONG* bytesRead)
{
	MQMSGPROPS* pmsgProps = GetMsgProps();
	HRESULT hr = E_FAIL;


	for(;;)
	{
		hr = MQReceiveMessage(hQ, 1000, MQ_ACTION_PEEK_CURRENT, pmsgProps, NULL, NULL, NULL, MQ_NO_TRANSACTION);

		// timeout handled outside
		//if (hr == MQ_ERROR_IO_TIMEOUT)
		//{
		//	Sleep(1000);
		//	continue;
		//}


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

		VOID* pPayload = pmsgProps->aPropVar[3].caub.pElems;
		ULONG size = pmsgProps->aPropVar[3].caub.cElems;
		*bytesRead =  *readLen < size ? *readLen : size;
		if (memcmp(pPayload, *pLast, *bytesRead ) == 0) 
		{
			//// we read the same sitting message
			//Sleep(1000);
			//continue;
			hr = MQ_ERROR_IO_TIMEOUT;
			goto exit;
		}
		else
		{
			memcpy_s(*pLast, *readLen, pPayload, size); // TODO: do not truncate
		}

 		if (FAILED(hr))
		{
			goto exit;
		}


		break;
	}


exit:
	// FreeMsgProps(pmsgProps, 5);
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

HRESULT WinMSMQMonitor::EnableAsyncReceive(
                           RECEIVE_CONTEXT* prc
                           )
{

  // Validate the input parameter.
  if (prc == NULL)
  {
    return MQ_ERROR_INVALID_PARAMETER;
  }

  HRESULT hr = E_FAIL;

  for (;;)
  {
  // Initiate a receive operation.
  hr =  MQReceiveMessage(
                 prc->hQueue,         // Queue handle
                 INFINITE,            // Time to wait for a message
                 MQ_ACTION_PEEK_CURRENT,
                 prc->pmsgprops,      // Pointer to an MQMSGPROPS structure
                 &prc->ov,            // OVERLAPPED structure 
                 NULL,                // No callback function
                 NULL,                // No cursor
                 MQ_NO_TRANSACTION    // Not in a transaction
                 );

  if (hr == MQ_ERROR_BUFFER_OVERFLOW)
		{
			VOID* pBuffer = prc->pmsgprops->aPropVar[3].caub.pElems;
			ULONG ulBufferSize = prc->pmsgprops->aPropVar[2].ulVal * sizeof(UCHAR);
			delete pBuffer;
			pBuffer = NULL;
			pBuffer = (VOID*)malloc(ulBufferSize);
			prc->pmsgprops->aPropVar[3].caub.pElems = (UCHAR*)pBuffer;
			prc->pmsgprops->aPropVar[3].caub.cElems = ulBufferSize;
			if(pBuffer == NULL)
			{
				return MQ_ERROR_INSUFFICIENT_RESOURCES;
			}


			memset(pBuffer, 0, ulBufferSize);
			continue;
		}

  		VOID* pPayload = prc->pmsgprops->aPropVar[3].caub.pElems;
		ULONG size = prc->pmsgprops->aPropVar[2].ulVal * sizeof(UCHAR);
		//map hQueue to Queue
		bool fFound = FALSE;
		int index = 0;
		for (ULONG i = 0; i < _wrapperCount; i++)
		{
			if (_queues[i]->getHandle() == prc->hQueue)
			{
				fFound = TRUE;
				index = i;
				break;
			}
		}
		if (!fFound)
		{
			hr = MQ_ERROR_INVALID_PARAMETER;
			goto exit;
		}
		//CScopedMSMQWrapper* queue = _queues[index];
		//void* pBuffer = queue->buffer;
		//ULONG len = (ULONG)queue->len;
		//ULONG nRead = len < size ? len : size;
		//if (memcmp(pPayload, pBuffer, nRead) == 0) 
		//{
		//	//// we read the same sitting message
		//	//Sleep(1000);
		//	//continue;
		//	hr = MQ_ERROR_IO_TIMEOUT;
		//	goto exit;
		//}
		//else
		//{
		//	memcpy_s(pBuffer, len, pPayload, size); // TODO: do not truncate
		//	queue->sent = FALSE;
		//}

 		if (FAILED(hr))
		{
			goto exit;
		}


		break;
  }
exit:
  return hr;
}

WinMSMQMonitorThread::WinMSMQMonitorThread(WinMSMQMonitor* monitor) : _WinMSMQMonitor (NULL) 
{	    
    hThread = CreateThread(
              NULL,                  // Default thread security descriptor
              0,                     // Default stack size
              CompletionPortThread,  // Start routine
              (LPVOID)monitor,                 // Start routine parameter
              CREATE_SUSPENDED,                     // Run immediately
              &dwThreadId            // Thread ID
              );
}

DWORD WINAPI CompletionPortThread(LPVOID lParam)
{
  WinMSMQMonitor *monitor = (WinMSMQMonitor*) lParam;
  HANDLE hPort = monitor->_completionPort;
  DWORD dwNoOfBytes = 0;
  ULONG_PTR ulKey = 0;
  OVERLAPPED* pov = NULL;
  HRESULT hr = MQ_OK;

  for ( ; ; )
  {
    
    // Wait for a completion notification.
    pov = NULL;
    BOOL fSuccess = GetQueuedCompletionStatus(
                    monitor->_readerPort,         // Completion port handle
                    &dwNoOfBytes,  // Bytes transferred
                    &ulKey,
                    &pov,          // OVERLAPPED structure
                    INFINITE       // Notification time-out interval
                    );
    
    //
    // Add code to handle specific errors when fSuccess is FALSE 
    // and provide a way to exit the loop.
    //
    
    if (pov == NULL)
    {
      //TODO : change this ?
      // An unrecoverable error occurred in the completion port. Wait for the next notification.
      goto exit;
    }


    // Get the base address of the RECEIVE_CONTEXT structure 
    // containing the OVERLAPPED structure received.
    RECEIVE_CONTEXT* prc = CONTAINING_RECORD(pov, RECEIVE_CONTEXT, ov);


    // Get the final status (last error) for the message received.
    hr = MQGetOverlappedResult(&prc->ov);
    if (FAILED(hr))
    {
       return hr;
    }
    
    
    // Retrieve the application-specific data in the message.
    
	VOID* pBuffer = prc->pmsgprops->aPropVar[3].caub.pElems;
	ULONG ulBufferSize = prc->pmsgprops->aPropVar[2].ulVal * sizeof(UCHAR);
	CScopedMSMQWrapper* queue = (CScopedMSMQWrapper*)(monitor->_queues[ulKey]);
	wprintf(L"Message received. Thread ID: %x. Application-specific data size: %ld\n",
            ulKey,
            ulBufferSize
            );

	
		ULONG len = (ULONG)queue->len;
		ULONG nRead = len < ulBufferSize ? len : ulBufferSize;
		// if (memcmp(queue->buffer, pBuffer, nRead) != 0 ||  !queue->sent)
		{

						if (!PostQueuedCompletionStatus(monitor->_completionPort, ulBufferSize, ulKey, (LPOVERLAPPED)pBuffer))
					{
						DWORD dwError = GetLastError();
						printf( "%s(): PostQueuedCompletionStatus() failed with error %d", __FUNCTION__, dwError);
						hr = HRESULT_FROM_WIN32(dwError);
						goto exit;
					}
					/*	else
						{					
						queue->sent = TRUE;
						}*/

		}

							// Initiate the next message receive operation.
					hr = monitor->EnableAsyncReceive(prc);
					if ( hr == MQ_ERROR_IO_TIMEOUT)
					{
						hr = S_OK;
						continue;
					}
					if (FAILED(hr))
					{	  
					  goto exit;
					}
  }
exit:
  if (FAILED(hr))
  {
	  monitor->stop(); // Interlocked ?
  }
  return hr;

}
//#endif
