/****************************** Module Header ******************************\
* Module Name:  SampleService.h
* Project:      CppWindowsService
* Copyright (c) Microsoft Corporation.
* 
* Provides a sample service class that derives from the service base class - 
* CServiceBase. The sample service logs the service start and stop 
* information to the Application event log, and shows how to run the main 
* function of the service in a thread pool worker thread.
* 
* This source is subject to the Microsoft Public License.
* See http://www.microsoft.com/en-us/openness/resources/licenses.aspx#MPL.
* All other rights reserved.
* 
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, 
* EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED 
* WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#pragma once

#include "ServiceBase.h"
#include "AutobackupFileUtil.h"
#include <thread>


class AutobackupService : public CServiceBase
{
public:

    AutobackupService(PWSTR pszServiceName, 
        BOOL fCanStop = TRUE, 
        BOOL fCanShutdown = TRUE, 
        BOOL fCanPauseContinue = FALSE);
    virtual ~AutobackupService(void);

protected:

    virtual void OnStart(DWORD dwArgc, PWSTR *pszArgv);
    virtual void OnStop();
	virtual void handleSystemEventMsg(DWORD dwCtrl,
		DWORD dwEventType,
		LPVOID lpEventData,
		LPVOID lpContext);

	virtual void handleDeviceChangeNotif(DWORD dwEventType, LPVOID lpEventData);

	void logToFile(PCSTR msg);
	void logToFile(std::string msg);
	void logToFile(char msg);
	void logToFile(const wchar_t* msg);
	void logToFile(int number, const wchar_t* msg...);
	void logToFile(std::wstring msg);

	std::vector< std::wstring >* listDirFiles(LPCWSTR volumeName);

    void ServiceWorkerThread(void);

	void registerForDeviceNotifications();

	HANDLE getFileHandle(LPCWSTR filepath);

	PBY_HANDLE_FILE_INFORMATION getFileInfo(LPCWSTR filepath);

	void backup(LPCWSTR path, LPCWSTR storageDevice);

private:

	void backupFile(LPCWSTR filepath, LPCWSTR storageDevice);

	std::wstring getFolderPathOnStorage(LPCWSTR filepath, LPCWSTR storageDevice);

    BOOL m_fStopping;
    HANDLE m_hStoppedEvent;
	HDEVNOTIFY m_hDevNotify;
	const PCSTR LOG_FILE_LOCATION = "C:\\Users\\haith\\Desktop\\Release\\log.txt";
	std::ofstream m_logFile;
	std::wofstream m_logFileW;
	PCSTR AUTOBACKUP_FILENAME = ".autobackup";
};