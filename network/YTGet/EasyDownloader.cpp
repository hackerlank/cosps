// EasyDownloader.cpp: implementation of the CEasyDownloader class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EasyDownloader.h"
#include "CommonUtils.h"
#include "DownloaderContext.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

DECLARE_THE_LOGGER_NAME("ESYD")

size_t CEasyDownloader::HeaderCallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	CEasyDownloader* pThis = (CEasyDownloader*)userdata;
	return pThis->ProcessHeader(ptr, size, nmemb);
}

size_t CEasyDownloader::DataCallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	CEasyDownloader* pThis = (CEasyDownloader*)userdata;
	return pThis->ProcessData(ptr, size, nmemb);
}

int CEasyDownloader::ProgressCallback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
	CEasyDownloader* pThis = (CEasyDownloader*)clientp;
	return pThis->ProcessProgress(dltotal, dlnow, ultotal, ulnow);
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEasyDownloader::CEasyDownloader(CDownloaderContext* pContext)
 : m_pContext(pContext), m_curl(NULL)
{
}

CEasyDownloader::~CEasyDownloader()
{
}

void CEasyDownloader::Init(const CDownloadParam& param)
{
	m_dlParam = param;
}

int CEasyDownloader::Start()
{
	VerifyTempFolderExist();

	if(m_connInfo.m_nDlNow <= 0)
	{
		StartConnection();
	}
	else
	{
		m_connInfo.m_nRetry = 0;
		RestartConnection(CCommonUtils::INT_OPER_KEEP, CCommonUtils::MAX_INTEGER);
	}

	return DoDownload();
}


int CEasyDownloader::DoDownload()
{
	CDownloadState dlState;

	CURLcode res;
	BOOL bRetry;
	do 
	{
		m_progressMeter.Reset(clock(), m_dlParam.m_nFileSize, m_connInfo.m_nDlNow);

		//transfer
		res = curl_easy_perform(m_curl);

		//process when the transfer done, check if we should retry to download?
		bRetry = ProcessTransferDone(res, dlState);

	} while (bRetry);
	
	return PostDownload(dlState);
}

BOOL CEasyDownloader::ProcessTransferDone(CURLcode res, CDownloadState& dlResultState)
{
	BOOL bRetry = FALSE;
	DWORD dwCurrState = m_pContext->GetState();
	//log
	CString szLog;
	if(IS_LOG_ENABLED(THE_LOGGER, log4cplus::DEBUG_LOG_LEVEL))
	{
		szLog.Format("Task[%02d]: State: %s, Transfer result: %s", m_dlParam.m_nTaskID, 
			CCommonUtils::State2Str(dwCurrState), CCommonUtils::CurlCode2Str(res));
		LOG4CPLUS_DEBUG_STR(THE_LOGGER, (LPCTSTR)szLog)
	}

	//First close connection, this function doesn't change "m_dlInfo" data
	CloseConnection();
	
	dlResultState.SetState(TSE_COMPLETE);

	if(dwCurrState == TSE_DESTROYING)
	{
		dlResultState.SetState(TSE_DESTROYED);
	}
	else if(dwCurrState == TSE_PAUSING)
	{
		dlResultState.SetState(TSE_PAUSED);
	}
	else if(dwCurrState == TSE_TRANSFERRING)
	{
		switch(res)
		{
		case CURLE_OK:
			{
				//This case should rarely happen
				if(m_connInfo.m_headerInfo.m_nHTTPCode != 200)
				{
					bRetry = RestartConnection(CCommonUtils::INT_OPER_INCREASE);
					
					if(!bRetry)
					{
						dlResultState.SetState(TSE_END_WITH_ERROR, m_connInfo.m_headerInfo.m_szStatusLine);
					}
					szLog.Format("Task[%02d]: [Server Error]: %s. Retry=%d", m_dlParam.m_nTaskID, 
						m_connInfo.m_headerInfo.m_szStatusLine, bRetry);
					LOG4CPLUS_ERROR_STR(THE_LOGGER, (LPCTSTR)szLog)
				}
			}
			break;
		case CURLE_ABORTED_BY_CALLBACK:
			{
				szLog.Format("Task[%02d]: Connection stopped by abort callback. Unexpected State: %d", 
					m_dlParam.m_nTaskID, dwCurrState);
				LOG4CPLUS_FATAL_STR(THE_LOGGER, (LPCTSTR)szLog)
					
				ASSERT(FALSE);
			}
			break;
		case CURLE_WRITE_ERROR:
			{
				if(m_connInfo.m_headerInfo.m_nHTTPCode != 200)
				{
					dlResultState.SetState(TSE_END_WITH_ERROR, m_connInfo.m_headerInfo.m_szStatusLine);
				}
				if(IS_LOG_ENABLED(THE_LOGGER, log4cplus::DEBUG_LOG_LEVEL))
				{
					szLog.Format("Task[%02d]: Connection stopped by write data function", m_dlParam.m_nTaskID);
					LOG4CPLUS_DEBUG_STR(THE_LOGGER, (LPCTSTR)szLog)
				}
			}
			break;
		case CURLE_OPERATION_TIMEDOUT:
			{
				//Always retry
				bRetry = RestartConnection(CCommonUtils::INT_OPER_KEEP, CCommonUtils::MAX_INTEGER);
				if(IS_LOG_ENABLED(THE_LOGGER, log4cplus::DEBUG_LOG_LEVEL))
				{
					szLog.Format("Task[%02d]: Timeout, retry [%d]", m_dlParam.m_nTaskID, (int)bRetry);
					LOG4CPLUS_DEBUG_STR(THE_LOGGER, (LPCTSTR)szLog)
				}
			}
			break;
		default:
			{
				bRetry = RestartConnection(CCommonUtils::INT_OPER_INCREASE);
				if(!bRetry)
				{
					dlResultState.SetState(TSE_END_WITH_ERROR, CCommonUtils::CurlCode2Str(res));
				}

				if(IS_LOG_ENABLED(THE_LOGGER, log4cplus::DEBUG_LOG_LEVEL))
				{
					szLog.Format("Task[%02d]: Retry=[%d], times = %d", m_dlParam.m_nTaskID, (int)bRetry, m_connInfo.m_nRetry);
					LOG4CPLUS_DEBUG_STR(THE_LOGGER, (LPCTSTR)szLog)
				}
			}
			break;
		}
	}
	else
	{
		szLog.Format("Task[%02d]: [ProcessTransferDone] Unexpected state = %d", m_dlParam.m_nTaskID, dwCurrState);
		LOG4CPLUS_FATAL_STR(THE_LOGGER, (LPCTSTR)szLog)
			
		ASSERT(FALSE);
	}

	return bRetry;
}

int CEasyDownloader::PostDownload(CDownloadState& dlState)
{
	//1. Clean up
	m_curl = NULL;
	
	//Result Msg
	CString szLog;
	szLog.Format("Task[%02d]: Download result: %s", m_dlParam.m_nTaskID, dlState.ToString(TRUE));
	LOG4CPLUS_INFO_STR(THE_LOGGER, (LPCTSTR)szLog)
		
	//2. Post process
	DWORD dwResultState = dlState.GetState();
	
	BOOL bValidateResult = (dwResultState == TSE_COMPLETE || dwResultState == TSE_PAUSED
		|| dwResultState == TSE_END_WITH_ERROR || dwResultState == TSE_DESTROYED);
	if(!bValidateResult)
	{
		ASSERT(FALSE);
	}
	
	//(1). Complete successfully
	if(dwResultState == TSE_COMPLETE)
	{
		//1. copy file
		CString szTempFolder;
		CCommonUtils::GetTempFolder(szTempFolder, m_dlParam);
		
		CString szSrcFile, szDstFile;
		szSrcFile.Format("%s\\data", szTempFolder);
		szDstFile.Format("%s\\%s", SYS_OPTIONS()->m_szSaveToFolder, m_dlParam.m_szFileName);
		
		::CopyFile(szSrcFile, szDstFile, FALSE);
		
		//2. delete task related folder
		if(!(SYS_OPTIONS()->m_bKeepTempFiles))
		{
			CCommonUtils::RemoveDirectory(szTempFolder);
		}
	}
	//(2). Destroyed
	else if(dwResultState == TSE_DESTROYED)
	{
		//delete task related folder
		if(!(SYS_OPTIONS()->m_bKeepTempFiles))
		{
			CString szTempFolder;
			CCommonUtils::GetTempFolder(szTempFolder, m_dlParam);
			CCommonUtils::RemoveDirectory(szTempFolder);
		}
	}
	//Nothing to do with other cases
	
	int nResult;
	m_pContext->Lock();
	
	//Special case for destroy case
	if(m_pContext->NoLockGetState() == TSE_DESTROYING && dwResultState != TSE_DESTROYED)
	{
		szLog.Format("Task[%02d]: PostDownload, explicitly change state from %s to %d(Destroyed)", 
			m_dlParam.m_nTaskID, dlState.ToString(TRUE), TSE_DESTROYED);
		LOG4CPLUS_INFO_STR(THE_LOGGER, (LPCTSTR)szLog)
			
		dlState.SetState(TSE_DESTROYED);
	}
	
	//update the task state
	m_pContext->NoLockSetState(dlState.GetState(), dlState.m_szDetail);
	nResult = m_pContext->NoLockGetState();
	
	m_pContext->Unlock();
	
	return nResult;
}

size_t CEasyDownloader::ProcessHeader(char *ptr, size_t size, size_t nmemb)
{
	if( IS_LOG_ENABLED(THE_LOGGER, log4cplus::DEBUG_LOG_LEVEL) )
	{
		CString szLine(ptr, size * nmemb);
		CCommonUtils::ReplaceCRLF(szLine);

		CString szMsg;
		szMsg.Format("Task[%02d]: %s", m_dlParam.m_nTaskID, szLine);
		LOG4CPLUS_DEBUG_STR(THE_LOGGER, (LPCTSTR)szMsg)
	}

	fwrite(ptr, size, nmemb, m_connInfo.lpFileHeader);

	CString szScratch(ptr, size * nmemb);
	do 
	{
		//1. check if status line
		int nRspCode = CCommonUtils::GetHTTPStatusCode(szScratch);
		
		//This is a status line
		if(nRspCode > 0)
		{
			m_connInfo.m_headerInfo.Reset();
			m_connInfo.m_headerInfo.m_nHTTPCode = nRspCode;
			m_connInfo.m_headerInfo.m_szStatusLine = szScratch;
			CCommonUtils::ReplaceCRLF(m_connInfo.m_headerInfo.m_szStatusLine, NULL, NULL);
			break;
		}
	}
	while(FALSE);	

	return (size_t)(size * nmemb);
}

size_t CEasyDownloader::ProcessData(char *ptr, size_t size, size_t nmemb)
{
	CString szLog;
	if(m_connInfo.m_headerInfo.m_nHTTPCode != 200)
	{
		szLog.Format("Task[%02d]: HTTP failed. Status=%s", m_dlParam.m_nTaskID, 
			m_connInfo.m_headerInfo.m_szStatusLine);
		LOG4CPLUS_ERROR_STR(THE_LOGGER, (LPCTSTR)szLog)

		return -1;
	}

	//When the HTTP response code is 200
	size_t nBytes = size * nmemb;

	CRange rLocal(m_connInfo.m_nDlNow, m_dlParam.m_nFileSize);
	if(m_dlParam.m_nFileSize <= 0)
	{
		rLocal.cy = 0x7FFFFFFF;
	}
	CRange rRemote(m_connInfo.m_nRemotePos, m_connInfo.m_nRemotePos + nBytes - 1);
	CRange rResult;

	int rc = CCommonUtils::Intersection(rLocal, rRemote, rResult);
	
	if(IS_LOG_ENABLED(THE_LOGGER, log4cplus::TRACE_LOG_LEVEL))
	{
		szLog.Format("rc=%d, local(%d-%d), remote(%d-%d), result(%d-%d)", rc, 
			rLocal.cx, rLocal.cy, rRemote.cx, rRemote.cy, rResult.cx, rResult.cy);
		LOG4CPLUS_TRACE_STR(THE_LOGGER, (LPCTSTR)szLog)
	}
	
	m_connInfo.m_nRemotePos += nBytes;

	//The data range doesn't reach yet
	if(rc > 0)
	{
		return nBytes;
	}
	//reach out, stop this connection, this should not happen in easy handle
	else if(rc < 0)
	{
		if(IS_LOG_ENABLED(THE_LOGGER, log4cplus::DEBUG_LOG_LEVEL))
		{
			szLog.Format("range reach out, stop connection");
			LOG4CPLUS_DEBUG_STR(THE_LOGGER, (LPCTSTR)szLog)
		}
		
		return -1;
	}
	//There are data we cared
	else
	{			
		int nDataLength = rResult.cy - rResult.cx + 1;
		fwrite(ptr + rResult.cx - rRemote.cx,  1, nDataLength, m_connInfo.lpFileData);
		
		m_connInfo.m_nDlNow += nDataLength;
		
		return nBytes;
	}
}

int CEasyDownloader::ProcessProgress(double dltotal, double dlnow, double ultotal, double ulnow)
{
	DWORD dwState = m_pContext->GetState();
	if(dwState == TSE_DESTROYING)
	{
		return 1;
	}

	m_progressMeter.UpdateSample(clock(), m_connInfo.m_nDlNow, (DWORD64)dlnow);
	if(m_progressMeter.IsProgressTimeOut() || m_connInfo.m_nDlNow == (DWORD64)m_dlParam.m_nFileSize)
	{
		/*
		//Send progress notification
		CProgressInfo progressInfo;

		progressInfo.m_pProgressMetric = &m_progressMeter;
		progressInfo.m_nTaskID = m_dlParam.m_nTaskID;
		progressInfo.dltotal = (DWORD64)m_dlParam.m_nFileSize;
		//The current downloaded data length is stored in m_dlInfo.m_nDlNow
		progressInfo.dlnow = (DWORD64)m_connInfo.m_nDlNow;
		progressInfo.ultotal = (DWORD64)ultotal;
		progressInfo.ulnow = (DWORD64)ulnow;
		*/

		CTaskInfo updateInfo;
		updateInfo.m_nTaskID = m_dlParam.m_nTaskID;
		updateInfo.mask = CTaskInfo::TIF_PROGRESS;
		updateInfo.m_nFileSize = m_dlParam.m_nFileSize;
		updateInfo.m_nDlNow = m_connInfo.m_nDlNow;
		updateInfo.m_nCurrSpeed = m_progressMeter.GetCurrentSpeed();
		updateInfo.m_nCostTime = m_progressMeter.GetCostTime();
		updateInfo.m_nLeftTime = m_progressMeter.GetLeftTime();

		::SendMessage(m_dlParam.m_hWnd, WM_DOWNLOAD_PROGRESS, (WPARAM)m_dlParam.m_nTaskID, (LPARAM)&updateInfo);
	}

	//Pause
	if(dwState == TSE_PAUSING)
	{
		return 2;
	}

	return 0;
}

void CEasyDownloader::VerifyTempFolderExist()
{
	CString szTempFolder;
	CCommonUtils::GetTempFolder(szTempFolder, m_dlParam);
	
	CCommonUtils::VerifyDirectoryExist(szTempFolder);
}

void CEasyDownloader::StartConnection()
{
	InitEasyHandle();

	m_connInfo.m_nDlNow = 0;
	m_connInfo.m_nRetry = 0;
	m_connInfo.m_nRemotePos = 0;
	
	//File create
	CString szTempFolder, szFileName;
	CCommonUtils::GetTempFolder(szTempFolder, m_dlParam);
	
	szFileName.Format("%s\\head.txt", szTempFolder);
	m_connInfo.lpFileHeader = fopen(szFileName, "wb");

	szFileName.Format("%s\\data", szTempFolder);
	m_connInfo.lpFileData = fopen(szFileName, "wb");
}
BOOL CEasyDownloader::RestartConnection(int nRetryOperType, int nMaxRetryLimit)
{
	if(nMaxRetryLimit < 0)
	{
		nMaxRetryLimit = SYS_OPTIONS()->m_nMaxRetryTimes;
	}
	
	if(m_connInfo.m_nRetry >= nMaxRetryLimit)
	{
		if(IS_LOG_ENABLED(THE_LOGGER, log4cplus::DEBUG_LOG_LEVEL))
		{
			CString szLog;
			szLog.Format("Retry times over, RestartConnection failed. retry=%d, max retry=%d", 
				m_connInfo.m_nRetry, nMaxRetryLimit);
			LOG4CPLUS_DEBUG_STR(THE_LOGGER, (LPCTSTR)szLog)
		}

		return FALSE;
	}

	InitEasyHandle();
	
	CCommonUtils::IntegerOper(m_connInfo.m_nRetry, nRetryOperType);
	m_connInfo.m_nRemotePos = 0;
	m_connInfo.m_headerInfo.Reset();

	//File create
	CString szTempFolder, szFileName;
	CCommonUtils::GetTempFolder(szTempFolder, m_dlParam);
	
	szFileName.Format("%s\\head.txt", szTempFolder);
	m_connInfo.lpFileHeader = fopen(szFileName, "wb");
	
	szFileName.Format("%s\\data", szTempFolder);
	m_connInfo.lpFileData = fopen(szFileName, "ab");

	return TRUE;
}

void CEasyDownloader::CloseConnection()
{
	/* always cleanup */
	curl_easy_cleanup(m_curl);
	m_curl = NULL;
	
	//self cleanup
	fclose(m_connInfo.lpFileHeader);
	fclose(m_connInfo.lpFileData);

	m_connInfo.lpFileHeader = NULL;
	m_connInfo.lpFileData = NULL;
}
void CEasyDownloader::InitEasyHandle()
{
	m_curl = curl_easy_init();
	
	//URL address
	curl_easy_setopt(m_curl, CURLOPT_URL, (LPCTSTR)m_dlParam.m_szUrl);
	
	
	//agent: IE8
	curl_easy_setopt(m_curl, CURLOPT_USERAGENT, USER_AGENT_IE8);
	//redirect
	curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1L);
	//no verbose
	curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 0L);
	//connect timeout: 10s
	curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT, 10L);
	
	//low speed limit
	curl_easy_setopt(m_curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
	curl_easy_setopt(m_curl, CURLOPT_LOW_SPEED_TIME, 15L);
	
	//Proxy setting
	if(SYS_OPTIONS()->GetInstance()->GetProxy() != NULL)
	{
		curl_easy_setopt(m_curl, CURLOPT_PROXY, SYS_OPTIONS()->GetInstance()->GetProxy());
	}
	
	//header
	curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, CEasyDownloader::HeaderCallback);
	curl_easy_setopt(m_curl, CURLOPT_HEADERDATA, this);
	
	//data
	curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, CEasyDownloader::DataCallback);
	curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);
	
	//progress
	curl_easy_setopt(m_curl, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(m_curl, CURLOPT_PROGRESSFUNCTION, CEasyDownloader::ProgressCallback);
	curl_easy_setopt(m_curl, CURLOPT_PROGRESSDATA, this);
}

