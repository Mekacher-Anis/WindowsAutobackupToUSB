/****************************** Module Header ******************************\
* Module Name:  SampleService.cpp
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

#pragma region Includes
#include "AutobackupService.h"
#include "ThreadPool.h"
#pragma endregion


AutobackupService::AutobackupService(PWSTR pszServiceName,
	BOOL fCanStop,
	BOOL fCanShutdown,
	BOOL fCanPauseContinue)
	: CServiceBase(pszServiceName, fCanStop, fCanShutdown, fCanPauseContinue)
{
	m_fStopping = FALSE;

	m_logFile.open(LOG_FILE_LOCATION, std::ios::out | std::ios::app);
	m_logFileW.open(LOG_FILE_LOCATION, std::ios::out | std::ios::app);

	// Create a manual-reset event that is not signaled at first to indicate 
	// the stopped signal of the service.
	m_hStoppedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (m_hStoppedEvent == NULL)
	{
		throw GetLastError();
	}
}


AutobackupService::~AutobackupService(void)
{
	if (m_hStoppedEvent)
	{
		CloseHandle(m_hStoppedEvent);
		m_hStoppedEvent = NULL;
		m_logFile.close();
	}
}


//
//   FUNCTION: CSampleService::OnStart(DWORD, LPWSTR *)
//
//   PURPOSE: The function is executed when a Start command is sent to the 
//   service by the SCM or when the operating system starts (for a service 
//   that starts automatically). It specifies actions to take when the 
//   service starts. In this code sample, OnStart logs a service-start 
//   message to the Application log, and queues the main service function for 
//   execution in a thread pool worker thread.
//
//   PARAMETERS:
//   * dwArgc   - number of command line arguments
//   * lpszArgv - array of command line arguments
//
//   NOTE: A service application is designed to be long running. Therefore, 
//   it usually polls or monitors something in the system. The monitoring is 
//   set up in the OnStart method. However, OnStart does not actually do the 
//   monitoring. The OnStart method must return to the operating system after 
//   the service's operation has begun. It must not loop forever or block. To 
//   set up a simple monitoring mechanism, one general solution is to create 
//   a timer in OnStart. The timer would then raise events in your code 
//   periodically, at which time your service could do its monitoring. The 
//   other solution is to spawn a new thread to perform the main service 
//   functions, which is demonstrated in this code sample.
//
void AutobackupService::OnStart(DWORD dwArgc, LPWSTR *lpszArgv)
{
	// Register Service for Device Notifications
	registerForDeviceNotifications();

	// Log a service start message to the Application log.
	/*WriteEventLogEntry(L"CppWindowsService in OnStart",
		EVENTLOG_INFORMATION_TYPE);*/

		// Queue the main service function for execution in a worker thread.
		//CThreadPool::QueueUserWorkItem(&AutobackupService::ServiceWorkerThread, this);
}


//
//   FUNCTION: CSampleService::ServiceWorkerThread(void)
//
//   PURPOSE: The method performs the main function of the service. It runs 
//   on a thread pool worker thread.
//
void AutobackupService::ServiceWorkerThread(void)
{
	// Periodically check if the service is stopping.
	while (!m_fStopping)
	{
		// Perform main service function here...

		::Sleep(2000);  // Simulate some lengthy operations.
	}

	// Signal the stopped event.
	SetEvent(m_hStoppedEvent);
}


// register this service for USB device notifications
void AutobackupService::registerForDeviceNotifications() {
	DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
	ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
	NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
	NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	//NotificationFilter.dbcc_classguid = { 0x71a27cdd, 0x812a, 0x11d0, 0xbe, 0xc7, 0x08, 0x00, 0x2b, 0xe2, 0x09, 0x2f };
	NotificationFilter.dbcc_classguid = GUID_DEVINTERFACE_VOLUME;

	m_hDevNotify = RegisterDeviceNotification(m_statusHandle,
		&NotificationFilter, DEVICE_NOTIFY_SERVICE_HANDLE);
}

HANDLE AutobackupService::getFileHandle(LPCWSTR filepath) {
	if (PathFileExistsW(filepath))
		return CreateFileW(filepath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	else
		return INVALID_HANDLE_VALUE;
}

PBY_HANDLE_FILE_INFORMATION AutobackupService::getFileInfo(LPCWSTR filepath)
{
	HANDLE fileHandle = getFileHandle(filepath);
	if (fileHandle == INVALID_HANDLE_VALUE) return NULL;
	LPBY_HANDLE_FILE_INFORMATION fileInfo = new BY_HANDLE_FILE_INFORMATION;
	GetFileInformationByHandle(fileHandle, fileInfo);
	CloseHandle(fileHandle);
	return fileInfo;
}

void AutobackupService::backup(LPCWSTR path, LPCWSTR storageDevice) {
	//check if path points to a file or directory
	DWORD fileAtrributes = GetFileAttributesW(path);

	logToFile(L"\n\n************************************************************************************************");

	if (fileAtrributes & FILE_ATTRIBUTE_DIRECTORY) {
		logToFile(std::wstring(L"Backing up folder : ") + std::wstring(path));
		// if directory, list files and backup them
		std::vector< std::wstring >* files = listDirFiles(path);
		logToFile(std::to_string(files->size()) + std::string(" Files found in folder : "));
		for (std::vector<std::wstring>::iterator it = files->begin(); it != files->end(); it++)
			logToFile(std::wstring(L"\t+  ") + *it);

		//create folder on storage media if it doesn't exist
		std::wstring folderNameOnMedia = getFolderPathOnStorage(path, storageDevice).c_str();
		if (!PathFileExistsW(folderNameOnMedia.c_str())) {
			logToFile(L"Folder doesn't exist, creating folder...");
			bool created = CreateDirectoryW(folderNameOnMedia.c_str(), NULL);
			if (created == 0)
				logToFile(std::wstring(L"Error creating folder, error : ") + std::to_wstring(GetLastError()));
		}

		for (std::vector<std::wstring>::iterator it = files->begin(); it != files->end(); it++ ) {
			backup( (std::wstring(path) + L"\\" + *it).c_str(), folderNameOnMedia.c_str());
		}
	}
	else {
		logToFile(std::wstring(L"Backing up file : ") + path);
		backupFile(path,storageDevice);
	}
}

void AutobackupService::backupFile(LPCWSTR filepath, LPCWSTR storageDevice) {

	LPBY_HANDLE_FILE_INFORMATION localFileInfo = getFileInfo(filepath);
	if (localFileInfo == NULL) return;

	LPWSTR filename = PathFindFileNameW(filepath);
	LPWSTR mediaFilePath = new WCHAR[1000];
	wcscpy_s(mediaFilePath, 255, storageDevice);
	wcscat_s(mediaFilePath, 255, L"\\");
	wcscat_s(mediaFilePath, 255, filename);
	LPBY_HANDLE_FILE_INFORMATION mediaFileInfo = getFileInfo(mediaFilePath);

	// if the file exists already on the media device check if it's newer
	// if it's newer don't do anything

	if (mediaFileInfo != NULL) {
		ULARGE_INTEGER localFileModifTime;
		localFileModifTime.LowPart = localFileInfo->ftLastWriteTime.dwLowDateTime;
		localFileModifTime.HighPart = localFileInfo->ftLastWriteTime.dwHighDateTime;
		ULARGE_INTEGER mediaFileModifTime;
		mediaFileModifTime.LowPart = mediaFileInfo->ftLastWriteTime.dwLowDateTime;
		mediaFileModifTime.HighPart = mediaFileInfo->ftLastWriteTime.dwHighDateTime;

		// file on local computer is older or the same
		if (localFileModifTime.QuadPart <= mediaFileModifTime.QuadPart) {
			logToFile("Local file is older");
			//clean up first
			delete localFileInfo;
			delete mediaFileInfo;
			return; // don't copy file
		}
	}

	// else copy the file
	bool copied = CopyFileW(filepath, mediaFilePath, false);
	if (!copied) {
		logToFile(std::wstring(L"Error copying file : ") + std::to_wstring(GetLastError()));
	}

	delete localFileInfo;
	delete mediaFileInfo;
}

std::wstring AutobackupService::getFolderPathOnStorage(LPCWSTR filepath, LPCWSTR storageDevice) {
	std::wstring folderPath = std::wstring(filepath);
	std::wstring folderName = folderPath.substr(folderPath.find_last_of(L"\\")+1);
	std::wstring mediaFilePath = std::wstring(storageDevice) + L"\\" + folderName;
	return mediaFilePath;
}


//
//   FUNCTION: CSampleService::OnStop(void)
//
//   PURPOSE: The function is executed when a Stop command is sent to the 
//   service by SCM. It specifies actions to take when a service stops 
//   running. In this code sample, OnStop logs a service-stop message to the 
//   Application log, and waits for the finish of the main service function.
//
//   COMMENTS:
//   Be sure to periodically call ReportServiceStatus() with 
//   SERVICE_STOP_PENDING if the procedure is going to take long time. 
//
void AutobackupService::OnStop()
{
	// Log a service stop message to the Application log.
	/*WriteEventLogEntry(L"CppWindowsService in OnStop",
		EVENTLOG_INFORMATION_TYPE);*/

		// Indicate that the service is stopping and wait for the finish of the 
		// main service function (ServiceWorkerThread).
	m_fStopping = TRUE;
	UnregisterDeviceNotification(m_statusHandle);

	/*if (WaitForSingleObject(m_hStoppedEvent, INFINITE) != WAIT_OBJECT_0)
	{
		throw GetLastError();
	}*/
}


// override the basic ServiceBase behaviour for system event messages
void AutobackupService::handleSystemEventMsg(DWORD dwCtrl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext) {
	switch (dwCtrl) {
	case SERVICE_CONTROL_DEVICEEVENT:
		handleDeviceChangeNotif(dwEventType, lpEventData);
		break;
	default:
		break;
	}
}

void AutobackupService::handleDeviceChangeNotif(DWORD dwEventType, LPVOID lpEventData) {
	switch (dwEventType) {
	case DBT_DEVICEREMOVECOMPLETE: {
	}
								   break;
	case DBT_DEVICEARRIVAL: {
		PDEV_BROADCAST_HDR eventData = (PDEV_BROADCAST_HDR)lpEventData;
		if (eventData->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
		{
			PDEV_BROADCAST_DEVICEINTERFACE lpdbv = (PDEV_BROADCAST_DEVICEINTERFACE)eventData;
			HANDLE deviceHandle = CreateFileW(&(lpdbv->dbcc_name[0]), GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (deviceHandle != NULL) {
				PWSTR nameBuffer = new WCHAR[MAX_PATH + 1];
				PWSTR pfileSystemNameBuf = new WCHAR[MAX_PATH + 1];
				DWORD lpMaximumComponentLength, lpFileSystemFlags;
				GetVolumeInformationByHandleW(deviceHandle, nameBuffer, MAX_PATH + 1, NULL,
					&lpMaximumComponentLength, &lpFileSystemFlags, pfileSystemNameBuf, MAX_PATH + 1);

				WriteEventLogEntry((std::wstring(L"storage device : ") + std::wstring(nameBuffer) + std::wstring(L" connected")).c_str(), EVENTLOG_INFORMATION_TYPE);

				//listVolumeFiles(lpdbv->dbcc_name);

				std::wfstream* autobackupFile;
				char* buffer = new char[2500];
				if ((autobackupFile = AutobackupFileUtil::deviceHasAutobackupFile(lpdbv->dbcc_name)) != nullptr) {
					logToFile(L"Storage device  " + std::wstring(nameBuffer) + L" connected and has a .autobackup file");
					AutobackupFileUtil::parseFile(autobackupFile);
					for (auto file : AutobackupFileUtil::getBackupFilesList()) {
						backup((WCHAR*)file.first.c_str(),lpdbv->dbcc_name);
					}
				}

				delete autobackupFile;
				delete nameBuffer;
				delete pfileSystemNameBuf;
			}
		}
	}
	}
}

void AutobackupService::logToFile(PCSTR msg) {
	m_logFile.clear();
	m_logFile << msg << std::endl;
	m_logFile.flush();
}

void AutobackupService::logToFile(std::string msg) {
	m_logFile.clear();
	m_logFile << msg << std::endl;
	m_logFile.flush();
}

void AutobackupService::logToFile(char msg) {
	m_logFile.clear();
	m_logFile << msg << std::endl;
	m_logFile.flush();
}

void AutobackupService::logToFile(const wchar_t* msg) {
	m_logFileW.clear();
	m_logFileW << msg << std::endl;
	m_logFileW.flush();
}

void AutobackupService::logToFile(std::wstring msg) {
	m_logFileW.clear();
	m_logFileW << msg << std::endl;
}

void AutobackupService::logToFile(int number, const wchar_t* msg...) {
	m_logFileW.clear();
	for (size_t i = 0; i < number; i++)
		while (*(msg++) != '\0')
			m_logFileW << *msg;

	m_logFileW << std::endl;
	m_logFileW.flush();
}

std::vector< std::wstring >* AutobackupService::listDirFiles(LPCWSTR volumeName) {
	WIN32_FIND_DATA ffd;
	LARGE_INTEGER filesize;
	TCHAR szDir[MAX_PATH];
	size_t length_of_arg;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError = 0;

	// Prepare string for use with FindFile functions.  First, copy the
	// string to a buffer, then append '\*' to the directory name.

	StringCchCopy(szDir, MAX_PATH, volumeName);
	StringCchCat(szDir, MAX_PATH, TEXT("\\*"));

	// Find the first file in the directory.

	hFind = FindFirstFile(szDir, &ffd);

	if (INVALID_HANDLE_VALUE == hFind) {
		logToFile("error listing files in device");
	}

	std::vector< std::wstring >* filesList = new std::vector< std::wstring >();

	// List all the files in the directory with some info about them.

	do
	{
		std::wstring filename = std::wstring(ffd.cFileName);
		//skip parent and local folder
		if(filename != L"." && filename != L"..")
			filesList->push_back(filename);
	} while (FindNextFile(hFind, &ffd) != 0);

	FindClose(hFind);

	return filesList;
}
