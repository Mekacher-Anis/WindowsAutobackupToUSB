#include "AutobackupFileUtil.h"



AutobackupFileUtil::AutobackupFileUtil()
{
}


AutobackupFileUtil::~AutobackupFileUtil()
{
}

//static variables definitions
const char* AutobackupFileUtil::FILE_DATA_DELIM = " ==> ";
const wchar_t* AutobackupFileUtil::FILE_DATA_DELIM_W = L" ==> ";
std::vector< std::pair<std::wstring, std::wstring> > AutobackupFileUtil::filesList;
std::vector<std::wstring> AutobackupFileUtil::ignoredFilesList;


std::wfstream * AutobackupFileUtil::deviceHasAutobackupFile(WCHAR * volumeName)
{
	WIN32_FIND_DATA ffd;
	TCHAR szDir[MAX_PATH];
	HANDLE hFind = INVALID_HANDLE_VALUE;

	StringCchCopy(szDir, MAX_PATH, volumeName);
	StringCchCat(szDir, MAX_PATH, TEXT("\\.autobackup"));

	// Find the first file in the directory.

	hFind = FindFirstFile(szDir, &ffd);

	if (hFind != INVALID_HANDLE_VALUE) {
		FindClose(hFind);

		std::wfstream *autobackFile = new std::wfstream(std::wstring(volumeName) + TEXT("\\") + std::wstring(ffd.cFileName), std::ios_base::in | std::ios_base::out);

		return autobackFile;
	}

	FindClose(hFind);
	return nullptr;
}

void AutobackupFileUtil::parseFile(std::wfstream* file){
	while (!file->eof()) {
		std::wstring line;
		std::getline(*file, line);
		if (line.empty()) continue;
		switch (line[0]) {
		case '#': continue; // ignores
		case '-': { // add to neglected files
			ignoredFilesList.push_back(line.substr(2));
			break;
		}
		case '+': { // add to backup files
			size_t splitPos = line.find(FILE_DATA_DELIM_W);
			std::wstring localFileLoc = line.substr(2, splitPos - 2);
			std::wstring mediaFileLoc = line.substr(splitPos+strlen(FILE_DATA_DELIM),line.size() - splitPos);
			filesList.push_back(std::make_pair(localFileLoc, mediaFileLoc));
			break;
		}
		}
	}
}

const std::vector<std::pair<std::wstring, std::wstring>>& AutobackupFileUtil::getBackupFilesList() {
	return filesList;
}

const std::vector<std::wstring>& AutobackupFileUtil::getNeglectedFilesList()
{
	return ignoredFilesList;
}

bool AutobackupFileUtil::isIgnored(std::wstring filepath)
{
	return std::find(ignoredFilesList.begin(),ignoredFilesList.end(),filepath) != ignoredFilesList.end();
}
