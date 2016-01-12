/************************************************************************
** FILE NAME..... Utility.cpp
** 
** (c) COPYRIGHT 
** 
** FUNCTION......... Common routines
**
** NOTES............  
** 
** ASSUMPTIONS...... 
** 
** RESTRICTIONS..... 
**
** LIMITATIONS...... 
** 
** DEVIATIONS....... 
** 
** RETURN VALUES.... 0  - successful
**                   !0 - error
** 
** AUTHOR(S)........ Michael Q Yan
** 
** CHANGES:
** 
************************************************************************/
#ifdef WIN32
#include <windows.h>
#pragma warning(disable:4786) // debug symbol is longer than 255
#else
#include <pthread.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <dirent.h>
#endif

#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <set>

#include "CrossPlatform.h"
#include "Utility.h"

// Given the full path, return file's base name w/o '/' or '\'
const char* GetBaseName(const char* ipName)
{
	// Starting from end of string, walk backwards.
	const char* lResult = ipName + ::strlen(ipName);
	while(lResult >= ipName)
	{
		if(*lResult == PATHDELIMINATOR)
		{
			return lResult+1;
		}
		lResult--;
	}
	return ipName;
}

// Return true if the path is a directory
bool IsDirectory(const char * ipPath)
{
#ifdef WIN32
	const DWORD lAttributes = ::GetFileAttributes(ipPath);
	if(lAttributes == 0xFFFFFFFF
		|| (lAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
#else
	struct stat lStatBuf;
	if(::stat(ipPath, &lStatBuf))
	{
		return false;
	}
	else if(!S_ISDIR(lStatBuf.st_mode))
#endif
	{
		return false;
	}
	return true;
}

//-------------------------------------------------------------------------------------
//
// SetCurrentWorkingDirectory
// return true if succeed
//-------------------------------------------------------------------------------------
bool SetCurrentWorkingDirectory( const char * ipPath )
{
#ifdef WIN32
	if( ::SetCurrentDirectory( ipPath ) == 0 )
#else // UNIX
	int lnResult = ::chdir( ipPath );
	if( lnResult == -1 )
#endif
	{
		::fprintf(stderr, "Failed to change current working directory to %s\n", ipPath);
		return false;
	}
	return true;
}

struct lt_STRING
{
  bool operator()(const char* f1, const char* f2) const
  {
	  return ::strcmp(f1,f2) < 0;
  }
};

// Return true if succeed
bool GetAllFilesInDirectory(const char * ipDirPath,
							const char * ipMatchExpr,
							std::vector<char*>& orFileNames)
{
	std::set<char*, lt_STRING> lFileNames;	// sort files by name
	/* Loop through directory entries. */
#ifdef WIN32
	char DirSpec[MAX_PATH + 1];  // directory specification
	strncpy (DirSpec, ipDirPath, strlen(ipDirPath)+1);
	strncat (DirSpec, ipMatchExpr, 11);
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = FindFirstFile(DirSpec, &FindFileData);
	if (hFind != INVALID_HANDLE_VALUE) 
	{
		//orFileNames.push_back(::strdup(FindFileData.cFileName));
		lFileNames.insert(::strdup(FindFileData.cFileName));
		while (FindNextFile(hFind, &FindFileData) != 0) 
		{
			//orFileNames.push_back(::strdup(FindFileData.cFileName));
			lFileNames.insert(::strdup(FindFileData.cFileName));
		}
	}
	else
	{
		return false;
	}
#else
	DIR* lpDir = ::opendir(ipDirPath);
	if(!lpDir)
	{
		::fprintf(stderr, "Failed to open directory %s\n", ipDirPath);
		return false;
	}

	struct dirent* dp;
	while ((dp = readdir(lpDir)) != NULL) 
	{
		if(::strncmp(dp->d_name, ipMatchExpr, strlen(ipMatchExpr)) == 0)
		{
			//orFileNames.push_back(::strdup(dp->d_name));
			lFileNames.insert(::strdup(dp->d_name));
		}
	}
#endif
	// swap file names from sorted set to vector
	std::set<char*, lt_STRING>::iterator it;
	for(it=lFileNames.begin(); it!=lFileNames.end(); it++)
	{
		orFileNames.push_back(*it);
	}
	return true;
}
