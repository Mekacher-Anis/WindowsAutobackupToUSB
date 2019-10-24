#pragma once

#include <iostream>
#include <fstream>
#include <Windows.h>
#include <atlbase.h>
#include <stdio.h>
#include <tchar.h>
#include <strsafe.h>
#include <dbt.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

class AutobackupFileUtil
{
private:
	AutobackupFileUtil();
	~AutobackupFileUtil();
	static std::vector< std::pair<std::wstring, std::wstring> > filesList;
	static std::vector<std::wstring> ignoredFilesList;
	static const char* FILE_DATA_DELIM;
	static const wchar_t* FILE_DATA_DELIM_W;
public:
	static std::wfstream* deviceHasAutobackupFile(WCHAR* volumeName);
	static void parseFile(std::wfstream* file);
	static const std::vector< std::pair<std::wstring, std::wstring> >& getBackupFilesList();
	static const std::vector<std::wstring>& getNeglectedFilesList();
	static bool isIgnored(std::wstring filepath);
};

