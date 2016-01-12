/************************************************************************
** FILE NAME..... Utility.h
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
#ifndef _UTILITY_H
#define _UTILITY_H

#include <vector>

// Given the full path, return file's base name w/o '/' or '\'
extern const char* GetBaseName(const char* ipName);

// Return true if the path is a directory
extern bool IsDirectory(const char * ipPath);

// return true if succeed
extern bool SetCurrentWorkingDirectory( const char * ipPath );


// Return true if succeed
extern bool GetAllFilesInDirectory(const char * ipDirPath,
								   const char * ipMatchExpr,
								   std::vector<char*>& orFileNames);

#endif // _UTILITY_H
