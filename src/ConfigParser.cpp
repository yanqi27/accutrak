/************************************************************************
** FILE NAME..... ConfigParser.cpp
**
** (c) COPYRIGHT
**
** FUNCTION......... Parse configuration file and collect commands for
** Patch manager.
**
** NOTES............
**
** ASSUMPTIONS...... Config file is ascii text file
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
#else
#include <unistd.h>
#include <dlfcn.h>
#endif

#include <stdio.h>
#include "PatchManager.h"
#include "ReplacementMallocImpl.h"
#include "PatchSym.h"
#include "MallocLogger.h"

// Read the configuration file.(see design doc) The file looks like
//
// modules=libA.so,libB.so
// mode=deadpage
bool PatchManager::ParseConfigFile(const char* iConfigFile)
{
	// Open the file
	FILE* fp = ::fopen(iConfigFile, "r");
	if(!fp)
	{
		::fprintf(stderr, "Failed to open configuration file %s\n", iConfigFile);
		return false;
	}

	const int LnBufSz = 2048;
	char LineBuffer[LnBufSz];

	// Key words used by configuration file
	const char key_output[] = "output=";
	const char key_message_level[] = "message_level=";
	const char key_patch_debug[] = "patch_debug=";
	const char key_batch_mode[] = "batch_mode=";

	const char key_modules[] = "modules=";
	const char key_self_contained[] = "self_contained=";
	const char key_patch_dlopen[] = "patch_dlopen=";
	const char key_incremental_patch[] = "incremental_patch=";

	const char key_mode[] = "mode=";
	const char key_min_block_size[] = "min_block_size=";
	const char key_max_block_size[] = "max_block_size=";
	const char key_alignment[] = "alignment=";
	const char key_check_underrun[] = "check_underrun=";
	const char key_check_access_freed[] = "check_access_freed=";
	const char key_max_cached_blocks[] = "max_cached_blocks=";
	const char key_max_defer_free_bytes[] = "max_defer_free_bytes=";
	const char key_flood_free[] = "flood_free=";
	const char key_fill_new[] = "fill_new=";
	const char key_check_frequency[] = "check_frequency=";

	const char key_event[] = "event=";
	const char key_action[] = "event_action=";
	const char key_back_trace_depth[] = "back_trace_depth=";
	const char key_back_trace_free[]  = "back_trace_free=";

	const int key_check_underrun_size = sizeof(key_check_underrun)-1;
	const int key_modules_size = sizeof(key_modules)-1;
	const int key_mode_size    = sizeof(key_mode)-1;
	const int key_alignment_size = sizeof(key_alignment)-1;
	const int key_self_contained_size = sizeof(key_self_contained)-1;
	const int key_output_size  = sizeof(key_output)-1;
	const int key_message_level_size = sizeof(key_message_level)-1;
	const int key_event_size   = sizeof(key_event)-1;
	const int key_action_size   = sizeof(key_action)-1;
	const int key_patch_dlopen_size = sizeof(key_patch_dlopen)-1;
	const int key_max_cached_blocks_size = sizeof(key_max_cached_blocks)-1;
	const int key_max_defer_free_bytes_size = sizeof(key_max_defer_free_bytes)-1;
	const int key_patch_debug_size = sizeof(key_patch_debug)-1;
	const int key_batch_mode_size = sizeof(key_batch_mode)-1;
	const int key_back_trace_depth_size = sizeof(key_back_trace_depth)-1;
	const int key_back_trace_free_size  = sizeof(key_back_trace_free)-1;
	const int key_check_access_freed_size = sizeof(key_check_access_freed)-1;
	const int key_incremental_patch_size  = sizeof(key_incremental_patch)-1;
	const int key_flood_free_size = sizeof(key_flood_free)-1;
	const int key_fill_new_size   = sizeof(key_fill_new)-1;
	const int key_check_frequency_size = sizeof(key_check_frequency)-1;
	const int key_min_block_size_size = sizeof(key_min_block_size)-1;
	const int key_max_block_size_size = sizeof(key_max_block_size)-1;

	// Read one line a time until the end of the file
	while(!feof(fp))
	{
		if(::fgets(LineBuffer, LnBufSz, fp))
		{
			// Skip comment which starts with character #
			if(LineBuffer[0] == '#')
			{
				continue;
			}

			// Get rid of new line charactor <CR>
			LineBuffer[::strlen(LineBuffer)-1] = '\0';
			// Get rid of window file's line feed if any <LF>
			if(LineBuffer[::strlen(LineBuffer)-1] == '\r')
			{
				LineBuffer[::strlen(LineBuffer)-1] = '\0';
			}

			// Simply check all keywords just like yacc would do
			char* cursor;
			long lLongValue;
			int lIntValue;

			// Memory underrun
			if(::strncmp(LineBuffer, key_check_underrun, key_check_underrun_size) == 0)
			{
				cursor = LineBuffer + key_check_underrun_size;
				if(::strncmp(cursor, "yes", 3) == 0)
				{
					mCheckUnderrun = true;
				}
				else if(::strncmp(cursor, "no", 2) == 0)
				{
					mCheckUnderrun = false;
				}
				else
				{
					::fprintf(stderr, "Invalid parameter %s\n", LineBuffer);
					::fprintf(stderr, "Valid parameter: [yes|no]\n");
				}
			}
			// Invalid access of freed memory
			else if(::strncmp(LineBuffer, key_check_access_freed, key_check_access_freed_size) == 0)
			{
				cursor = LineBuffer + key_check_access_freed_size;
				if(::strncmp(cursor, "yes", 3) == 0)
				{
					mCheckAccessFreedBlock = true;
				}
				else if(::strncmp(cursor, "no", 2) == 0)
				{
					mCheckAccessFreedBlock = false;
				}
				else
				{
					::fprintf(stderr, "Invalid parameter %s\n", LineBuffer);
					::fprintf(stderr, "Valid parameter: [yes|no]\n");
				}
			}
			// List of modules to track
			else if(::strncmp(LineBuffer, key_modules, key_modules_size) == 0)
			{
				cursor = LineBuffer + key_modules_size;
				// Special module name "all"
				if(::strcmp(cursor, "all") == 0)
				{
					mPatchAllUserModules = true;
				}
				else if(mPatchAllUserModules)
				{
					::fprintf(stderr, "[%s] conflicts with Previous command [modules=all]\n", LineBuffer);
					::fprintf(stderr, "This line is ignored\n");
				}
				else
				{
					// Get all module names, they are delimited by comma
					char* lpName = cursor;
					while(*cursor)
					{
						if(*cursor==',')
						{
							*cursor = '\0';
							mModuleConfiguration.AddModule(lpName);
							lpName = cursor+1;
						}
						++cursor;
						if(*cursor=='\0')
						{
							// We have come to the last module name
							mModuleConfiguration.AddModule(lpName);
							break;
						}
					}
				}
			}
			// Mode determines the behavior of Accutrak
			else if(::strncmp(LineBuffer, key_mode, key_mode_size) == 0)
			{
				cursor = LineBuffer + key_mode_size;

				ModuleConfig::AT_MODE lCurrentMode;
				if(::strcmp(cursor, "deadpage") == 0)
				{
					lCurrentMode = ModuleConfig::AT_MODE_ADD_DEAD_PAGE;
				}
				else if(::strcmp(cursor, "padding") == 0)
				{
					lCurrentMode = ModuleConfig::AT_MODE_ADD_PADDING;
				}
				else if(::strcmp(cursor, "private_heap") == 0)
				{
					lCurrentMode = ModuleConfig::AT_MODE_ADD_PRIVATE_HEAP;
				}
				else if(::strcmp(cursor, "record") == 0)
				{
					lCurrentMode = ModuleConfig::AT_MODE_ADD_RECORD;
				}
				else
				{
					::fprintf(stderr, "Invalid parameter %s\n", LineBuffer);
					::fprintf(stderr, "Valid parameter: [deadpage|padding|private_heap|record]\n");
				}

				// Multi-mode is not supported at this point
				if(mModuleConfiguration.mMode!=ModuleConfig::AT_MODE_ADD_DEAD_PAGE
					&& mModuleConfiguration.mMode!=lCurrentMode)
				{
					::fprintf(stderr, "Fatal: Invalid parameter %s\n", LineBuffer);
					::fprintf(stderr, "Fatal: Multi-mode is not supported for this release\n");
					return false;
				}
				else
				{
					mModuleConfiguration.mMode = lCurrentMode;
				}
			}
			// Minimum size of blocks to track
			else if (::strncmp(LineBuffer, key_min_block_size, key_min_block_size_size) == 0)
			{
				cursor = LineBuffer + key_min_block_size_size;
				lLongValue = ::atol(cursor);
				if (lLongValue <= mMaxBlockSize)
				{
					mMinBlockSize = lLongValue;
				}
				else
				{
					::fprintf(stderr, "Invalid parameter: %s\n", LineBuffer);
					::fprintf(stderr, "Maximum block size is previously set to %ld\n", mMaxBlockSize);
				}
			}
			// Maximum size of blocks to track
			else if (::strncmp(LineBuffer, key_max_block_size, key_max_block_size_size) == 0)
			{
				cursor = LineBuffer + key_max_block_size_size;
				lLongValue = ::atol(cursor);
				if (lLongValue >= mMinBlockSize)
				{
					mMaxBlockSize = lLongValue;
				}
				else
				{
					::fprintf(stderr, "Invalid parameter: %s\n", LineBuffer);
					::fprintf(stderr, "Minimum block size is previously set to %ld\n", mMinBlockSize);
				}
			}
			// User block alignment requirement
			else if(::strncmp(LineBuffer, key_alignment, key_alignment_size) == 0)
			{
				// Alignment requirement
				cursor = LineBuffer + key_alignment_size;
				lIntValue = ::atoi(cursor);
				// make sure the number is of 2^n, n is [0,8]
				bool lbValidAlignment = false;
				for(int lPower=0; lPower<=8; lPower++)
				{
					if(( 1 << lPower) == lIntValue)
					{
						lbValidAlignment = true;
						break;
					}
				}

				if(lbValidAlignment)
				{
					mAlignment = lIntValue;
				}
				else
				{
					::fprintf(stderr, "Invalid parameter: %s\n", LineBuffer);
					::fprintf(stderr, "Valid parameter: 2^n where n is in the range of [0,8]\n");
				}
			}
			// How often to go through in-use block list to check memory corruption
			else if(::strncmp(LineBuffer, key_check_frequency, key_check_frequency_size) == 0)
			{
				cursor = LineBuffer + key_check_frequency_size;
				mCheckFrequency = ::atoi(cursor);
			}
			// Number of frames of call stack to retrieve
			else if(::strncmp(LineBuffer, key_back_trace_depth, key_back_trace_depth_size) == 0)
			{
				cursor = LineBuffer + key_back_trace_depth_size;
				lIntValue = ::atoi(cursor);
				if(lIntValue > 0)
				{
					if (lIntValue > MAX_STACK_DEPTH)
					{
						lIntValue = MAX_STACK_DEPTH;
						::fprintf(stderr, "Invalid parameter: %s\n", LineBuffer);
						::fprintf(stderr, "Maximum stack depth is %d\n", MAX_STACK_DEPTH);
					}
					mBackTraceDepth = lIntValue;
				}
			}
			// Retrieve call stack when block is freed
			else if(::strncmp(LineBuffer, key_back_trace_free, key_back_trace_free_size) == 0)
			{
				cursor = LineBuffer + key_back_trace_free_size;
				if(::strncmp(cursor, "yes", 3) == 0)
				{
					mBackTraceFree = true;
				}
				else if(::strncmp(cursor, "no", 2) == 0)
				{
					mBackTraceFree = false;
				}
				else
				{
					::fprintf(stderr, "Invalid parameter %s\n", LineBuffer);
					::fprintf(stderr, "Valid parameter: [yes|no]\n");
				}
			}
			// mallc/free are self_contained in the specified modules
			else if(::strncmp(LineBuffer, key_self_contained, key_self_contained_size) == 0)
			{
				cursor = LineBuffer + key_self_contained_size;
				if(::strcmp(cursor, "no") == 0)
				{
					mSelfContained = false;
				}
				else if(::strcmp(cursor, "yes") == 0)
				{
					mSelfContained = true;
				}
				else
				{
					::fprintf(stderr, "Invalid parameter %s\n", LineBuffer);
					::fprintf(stderr, "Valid parameter: [yes|no]\n");
				}
			}
			// patch dynamic loading routines
			else if(::strncmp(LineBuffer, key_patch_dlopen, key_patch_dlopen_size) == 0)
			{
				cursor = LineBuffer + key_patch_dlopen_size;
				if(::strcmp(cursor, "no") == 0)
				{
					mPatchDlopen = false;
				}
				else if(::strcmp(cursor, "yes") == 0)
				{
					mPatchDlopen = true;
				}
				else
				{
					::fprintf(stderr, "Invalid parameter %s\n", LineBuffer);
					::fprintf(stderr, "Valid parameter: [yes|no]\n");
				}
			}
			// Incremental patch
			else if(::strncmp(LineBuffer, key_incremental_patch, key_incremental_patch_size) == 0)
			{
				cursor = LineBuffer + key_incremental_patch_size;
				if(::strcmp(cursor, "no") == 0)
				{
					mIncrementalPatch = false;
				}
				else if(::strcmp(cursor, "yes") == 0)
				{
					mIncrementalPatch = true;
				}
				else
				{
					::fprintf(stderr, "Invalid parameter %s\n", LineBuffer);
					::fprintf(stderr, "Valid parameter: [yes|no]\n");
				}
			}
			// Fill new use block with byte pattern
			else if(::strncmp(LineBuffer, key_fill_new, key_fill_new_size) == 0)
			{
				cursor = LineBuffer + key_fill_new_size;
				if(::strcmp(cursor, "no") == 0)
				{
					mInitUserSpace = false;
				}
				else if(::strcmp(cursor, "yes") == 0)
				{
					mInitUserSpace = true;
				}
				else
				{
					::fprintf(stderr, "Invalid parameter %s\n", LineBuffer);
					::fprintf(stderr, "Valid parameter: [yes|no]\n");
				}
			}
			// Flood freed block with byte pattern
			else if(::strncmp(LineBuffer, key_flood_free, key_flood_free_size) == 0)
			{
				cursor = LineBuffer + key_flood_free_size;
				if(::strcmp(cursor, "no") == 0)
				{
					mFloodFreeSpace = false;
				}
				else if(::strcmp(cursor, "yes") == 0)
				{
					mFloodFreeSpace = true;
				}
				else
				{
					::fprintf(stderr, "Invalid parameter %s\n", LineBuffer);
					::fprintf(stderr, "Valid parameter: [yes|no]\n");
				}
			}
			// Spit out what patch manager is doing
			else if(::strncmp(LineBuffer, key_patch_debug, key_patch_debug_size) == 0)
			{
				cursor = LineBuffer + key_patch_debug_size;
				if(::strcmp(cursor, "no") == 0)
				{
					mPatchDebug = false;
				}
				else if(::strcmp(cursor, "yes") == 0)
				{
					mPatchDebug = true;
				}
				else
				{
					::fprintf(stderr, "Invalid parameter %s\n", LineBuffer);
					::fprintf(stderr, "Valid parameter: [yes|no]\n");
				}
			}
			// Batch mode has limited output message
			else if(::strncmp(LineBuffer, key_batch_mode, key_batch_mode_size) == 0)
			{
				cursor = LineBuffer + key_batch_mode_size;
				if(::strcmp(cursor, "no") == 0)
				{
					mBatchMode = false;
				}
				else if(::strcmp(cursor, "yes") == 0)
				{
					mBatchMode = true;
					if (mpOutput == stdout || mpOutput == stderr)
					{
						mpOutput = NULL;
					}
				}
				else
				{
					::fprintf(stderr, "Invalid parameter %s\n", LineBuffer);
					::fprintf(stderr, "Valid parameter: [yes|no]\n");
				}
			}
			// Output stream
			else if(::strncmp(LineBuffer, key_output, key_output_size) == 0)
			{
				// where to dump the message
				cursor = LineBuffer + key_output_size;
				if(::strcmp(cursor, "stdout") == 0)
				{
					mpOutput = stdout;
				}
				else if(::strcmp(cursor, "stderr") == 0)
				{
					mpOutput = stderr;
				}
				else if(::strlen(cursor) > 0)
				{
					if(!(mpOutput = ::fopen(cursor, "w")))
					{
						::fprintf(stderr, "Failed to open file [%s] for output message\n", cursor);
						return false;
					}
				}
			}
			// Event action
			else if(::strncmp(LineBuffer, key_action, key_action_size) == 0)
			{
				// where to dump the message
				cursor = LineBuffer + key_action_size;
				if(::strncmp(cursor, "output", 6) == 0)
				{
					mActionType = AT_ACTION_OUTPUT;
				}
				else if(::strncmp(cursor, "pause", 5) == 0)
				{
					mActionType = AT_ACTION_PAUSE;
				}
				else
				{
					::fprintf(stderr, "Invalid parameter %s\n", LineBuffer);
					::fprintf(stderr, "Valid parameter: [output|pause]\n");
				}
			}
			// Event
			else if(::strncmp(LineBuffer, key_event, key_event_size) == 0)
			{
				// where to dump the message
				cursor = LineBuffer + key_event_size;
				AT_EVENT lEventType;
				if(::strncmp(cursor, "malloc@", 7) == 0)
				{
					lEventType = AT_EVENT_MALLOC;
					cursor += 7;
				}
				else if(::strncmp(cursor, "malloc_around@", 14) == 0)
				{
					lEventType = AT_EVENT_MALLOC_AROUND;
					cursor += 14;
				}
				else if(::strncmp(cursor, "free@", 5) == 0)
				{
					lEventType = AT_EVENT_FREE;
					cursor += 5;
				}
				else if(::strncmp(cursor, "free_around@", 12) == 0)
				{
					lEventType = AT_EVENT_FREE_AROUND;
					cursor += 12;
				}
				else
				{
					::fprintf(stderr, "Invalid parameter %s\n", LineBuffer);
					continue;
				}
				BlockEvent* lpNewEvent = new BlockEvent;
				if(lpNewEvent)
				{
					lpNewEvent->mType = lEventType;
					lpNewEvent->mValue = ::strtoul(cursor, NULL, 16);
					mEventList.push_front(lpNewEvent);
				}
			}
			// Max number of blocks to be put in cache for deferred release
			else if(::strncmp(LineBuffer, key_max_cached_blocks, key_max_cached_blocks_size) == 0)
			{
				cursor = LineBuffer + key_max_cached_blocks_size;
				lLongValue = ::atol(cursor);
				if (lLongValue >= 32 && lLongValue <= 1024*1024)
					mMaxCachedBlocks = lLongValue;
				else
				{
					::fprintf(stderr, "Invalid parameter %s\n", LineBuffer);
					::fprintf(stderr, "Valid maximum cached blocks is: [32, 1048576]\n");
				}
			}
			// Max bytes for deferred free in padding mode
			else if(::strncmp(LineBuffer, key_max_defer_free_bytes, key_max_defer_free_bytes_size) == 0)
			{
				cursor = LineBuffer + key_max_defer_free_bytes_size;
				lLongValue = ::atol(cursor);
				if (lLongValue >= 4096)
					mMaxDeferFreeBytes = lLongValue;
				else
				{
					::fprintf(stderr, "Invalid parameter %s\n", LineBuffer);
					::fprintf(stderr, "Valid maximum deferred free bytes > %ld\n", 4096);
				}
			}
			// How much detail info user want to see
			else if(::strncmp(LineBuffer, key_message_level, key_message_level_size) == 0)
			{
				cursor = LineBuffer + key_message_level_size;
				if(::strcmp(cursor, "none") == 0)
				{
					mOutputLevel = AT_NONE;
				}
				else if(::strcmp(cursor, "fatal") == 0)
				{
					mOutputLevel = AT_FATAL;
				}
				else if(::strcmp(cursor, "error") == 0)
				{
					mOutputLevel = AT_ERROR;
				}
				else if(::strcmp(cursor, "info") == 0)
				{
					mOutputLevel = AT_INFO;
				}
				else if(::strcmp(cursor, "all") == 0)
				{
					mOutputLevel = AT_ALL;
				}
				else
				{
					::fprintf(stderr, "Invalid parameter %s\n", LineBuffer);
					::fprintf(stderr, "Valid parameter: [none|fatal|error|info|all]\n");
				}
			}
			else if(::strlen(LineBuffer) > 0)
			{
				::fprintf(stderr, "Invalid input line: [%s]\n", LineBuffer);
			}
		}
		else if(feof(fp))
		{
			break;
		}
		else
		{
			::fprintf(stderr, "Failed to read line from configuration file %s\n", iConfigFile);
			break;
		}
	}

	// Done
	::fclose(fp);

	// Check if the settings make sense.
	return CheckConfigOptions();
}

// After reading in all config settings,
// check to ensure the integrity of them, spit out conflicts
// prepare for patching, f.g. find out "all" modules, open output stream, etc.
// Return true if all go well
bool PatchManager::CheckConfigOptions()
{
	// Find out conflicting settings.
	if(mPatchAllUserModules && !mPatchDlopen)
	{
		::fprintf(stderr, "Warning: [patch_dlopen=no] conflicts with command [modules=all]\n");
	}

	/*
	// Get the name of library that implements default malloc/free
	FunctionPtr lpFuncPtr = (FunctionPtr)::dlsym(RTLD_DEFAULT, "malloc");
	if(!lpFuncPtr)
	{
		::fprintf(stderr, "Failed to find library of default malloc\n");
		return false;
	}
	const char* lpModuleName = GetModuleNameByFuncAddr(lpFuncPtr);
	if(!lpModuleName)
	{
		::fprintf(stderr, "Failed to find library of default malloc\n");
		return false;
	}
	mDefaultMallocModuleName = ::strdup(lpModuleName);*/

	// Prepare for patching
	if (mPatchAllUserModules
		|| (mModuleConfiguration.NumModuleToPatch() == 0 &&
			(mModuleConfiguration.mMode == ModuleConfig::AT_MODE_ADD_DEAD_PAGE
			|| mModuleConfiguration.mMode == ModuleConfig::AT_MODE_ADD_PADDING)))
	{
		AddAllModulesInCore();
	}

	if(mModuleConfiguration.NumModuleToPatch() > 0)
	{
		mPatchFinished = false;
	}

	// Go through each module and set the apropriate replace malloc/free routines
	std::list<ModuleConfig::ModuleToPatch*>::iterator it2;
	int lModuleIndex = 0;
	for(it2=mModuleConfiguration.mModules.begin(); it2!=mModuleConfiguration.mModules.end(); it2++)
	{
		ModuleConfig::ModuleToPatch* lpModule = *it2;
		// If this module is declared before mode is chosen,
		// its replacement routine is not installed yet.
		if(lpModule->mNewMalloc == NULL)
		{
			mModuleConfiguration.InstallReplacementRoutines(lpModule, lModuleIndex);
		}
		lModuleIndex++;
	}

	if(mModuleConfiguration.mMode == ModuleConfig::AT_MODE_ADD_RECORD)
	{
		AT_InitMallocLogger();
		UpdateRecordInfoFile();
	}

#ifdef linux
	// Linux is strange, its link_map doesn't give excutable name,
	// instead the l_name field is ""
	char lProcCmdPath[64];
	sprintf(lProcCmdPath, "/proc/%ld/cmdline", getpid());

	// file stat doesn't work.
	FILE* lpFile = ::fopen(lProcCmdPath, "rb");
	if(lpFile)
	{
		const int BufferSize = 512;
		char lpBuffer[BufferSize];
		char* lpCursor = &lpBuffer[0];
		while(!feof(lpFile))
		{
			if(1 != ::fread(lpCursor, 1, 1, lpFile))
			{
				fprintf(stderr, "Failed to read file %s\n", lProcCmdPath);
				break;
			}
			if(*lpCursor == '\0')
			{
				break;
			}
			lpCursor++;
			if(lpCursor >= lpBuffer+BufferSize)
			{
				// Our buffer for program name is too small.
				lpCursor = &lpBuffer[BufferSize-1];
				*lpCursor = '\0';
				break;
			}
		}
		if(*lpCursor == '\0' && strlen(lpBuffer)>0)
		{
			mProgramName = ::strdup(lpBuffer);
		}
		::fclose(lpFile);
	}
	else
	{
		fprintf(stderr, "Failed to open file %s\n", lProcCmdPath);
	}
#endif
	return true;
}

// spit out all the choices in configuration file
void PatchManager::PrintConfigDefinition()
{
	if(!mPatchDebug)
		return;

	OUTPUT_MSG0(AT_FATAL, "===================== Configuration Definition ==========================\n")

	// mode
	if(mModuleConfiguration.mMode == ModuleConfig::AT_MODE_ADD_DEAD_PAGE)
	{
		OUTPUT_MSG0(AT_FATAL, "mode=deadpage\n")
	}
	else if(mModuleConfiguration.mMode == ModuleConfig::AT_MODE_ADD_PADDING)
	{
		OUTPUT_MSG0(AT_FATAL, "mode=padding\n")
	}
	else if(mModuleConfiguration.mMode == ModuleConfig::AT_MODE_ADD_PRIVATE_HEAP)
	{
		OUTPUT_MSG0(AT_FATAL, "mode=private_heap\n")
	}
	else if(mModuleConfiguration.mMode == ModuleConfig::AT_MODE_ADD_RECORD)
	{
		OUTPUT_MSG0(AT_FATAL, "mode=record\n")
	}
	else
	{
		OUTPUT_MSG0(AT_FATAL, "mode=UNKNOWN <please report to author>\n")
	}

	// Module list. Go through each module in the group
	OUTPUT_MSG0(AT_FATAL, "\nThe following modules are configured to be tracked\n")
	std::list<ModuleConfig::ModuleToPatch*>::iterator it2;
	for(it2=mModuleConfiguration.mModules.begin(); it2!=mModuleConfiguration.mModules.end(); it2++)
	{
		ModuleConfig::ModuleToPatch* lpModule = *it2;
#ifdef linux
		if(strlen(lpModule->mModuleName)==0 && mProgramName)
		{
			OUTPUT_MSG1(AT_FATAL, "\t[%s]\n", mProgramName)
		}
		else
#endif
		OUTPUT_MSG1(AT_FATAL, "\t[%s]\n", lpModule->mModuleName)
	}
	if(mPatchAllUserModules)
	{
		OUTPUT_MSG0(AT_FATAL, "\tBecause [modules=all] is specified, additional modules are tracked as they are dynmically loaded\n")
	}

	// Allocator properties
	OUTPUT_MSG0(AT_FATAL, "\nThe following behaviors are specified for allocator\n")
	if (mModuleConfiguration.mMode == ModuleConfig::AT_MODE_ADD_DEAD_PAGE)
	{
		if(mCheckUnderrun)
		{
			OUTPUT_MSG0(AT_FATAL, "\tCheck memory underrun\n")
		}
		else
		{
			OUTPUT_MSG0(AT_FATAL, "\tCheck memory overrun\n")
		}

		if(mCheckAccessFreedBlock)
		{
			OUTPUT_MSG0(AT_FATAL, "\tCheck invalid read/write access of freed blocks\n")
		}
		else
		{
			OUTPUT_MSG0(AT_FATAL, "\tFreed blocks are not protected for checking of access free\n");
		}

		OUTPUT_MSG1(AT_FATAL, "\tMaximum number of cached blocks %ld\n", mMaxCachedBlocks)
	}
	else if (mModuleConfiguration.mMode == ModuleConfig::AT_MODE_ADD_PADDING)
	{
		if (mMaxDeferFreeBytes > 0)
		{
			OUTPUT_MSG1(AT_FATAL, "\tUp to %ld bytes of free blocks may be deferred\n", mMaxDeferFreeBytes);
		}
		else
		{
			OUTPUT_MSG0(AT_FATAL, "\tFreed blocks are not deferred\n");
		}

		if(mFloodFreeSpace)
		{
			OUTPUT_MSG1(AT_FATAL, "\tFreed block is filled with 0x%x\n", USER_FREE_BYTE)
		}

		if(mInitUserSpace)
		{
			OUTPUT_MSG1(AT_FATAL, "\tAllocated block is filled with 0x%x\n", USER_INIT_BYTE)
		}

		if (mCheckFrequency > 0)
		{
			OUTPUT_MSG1(AT_FATAL, "\tAll outstanding blocks are checked every %d times"
				"malloc/free is called\n", mCheckFrequency);
		}
	}

	OUTPUT_MSG1(AT_FATAL, "\tAlignment requirement: %d bytes\n", mAlignment);
	OUTPUT_MSG2(AT_FATAL, "\tBlock sizes to track: [0x%lx, 0x%lx] bytes\n", mMinBlockSize, mMaxBlockSize);

	if (mBackTraceDepth > 0)
	{
		OUTPUT_MSG1(AT_FATAL, "\tBacktrace level: maximum %d stack frames\n", mBackTraceDepth)
		if (mBackTraceFree)
		{
			OUTPUT_MSG0(AT_FATAL, "\t\tBacktrace when memory block is freed\n");
		}
		else
		{
			OUTPUT_MSG0(AT_FATAL, "\t\tBacktrace when memory block is allocated\n");
		}
	}
	else
	{
		OUTPUT_MSG0(AT_FATAL, "\tNo backtrace is retrieved for tracked blocks\n");
	}

	// Events
	if(!mEventList.empty())
	{
		OUTPUT_MSG0(AT_FATAL, "\nThe following events are to be tracked\n")
		std::list<BlockEvent*>::iterator ite;
		for(ite=mEventList.begin(); ite!=mEventList.end(); ite++)
		{
			BlockEvent* lpEvent = *ite;
			if(lpEvent->mType == AT_EVENT_MALLOC)
			{
				OUTPUT_MSG1(AT_FATAL, "\tMalloc block at 0x%lx\n", lpEvent->mValue)
			}
			else if(lpEvent->mType == AT_EVENT_MALLOC_AROUND)
			{
				OUTPUT_MSG1(AT_FATAL, "\tMalloc block that contains address 0x%lx\n", lpEvent->mValue)
			}
			else if(lpEvent->mType == AT_EVENT_FREE)
			{
				OUTPUT_MSG1(AT_FATAL, "\tFree block at 0x%lx\n", lpEvent->mValue)
			}
			else if(lpEvent->mType == AT_EVENT_FREE)
			{
				OUTPUT_MSG1(AT_FATAL, "\tFree block that contains address 0x%lx\n", lpEvent->mValue)
			}
		}
		if(mActionType == AT_ACTION_OUTPUT)
		{
			OUTPUT_MSG0(AT_FATAL, "\tA message is outputed when an event occurs\n")
		}
		else if(mActionType == AT_ACTION_PAUSE)
		{
			OUTPUT_MSG0(AT_FATAL, "\tAssert and pause when an event occurs\n")
		}
	}

	// Patching choices
	OUTPUT_MSG0(AT_FATAL, "\nAccuTrak will patch modules by the following rules:\n")
	if(mSelfContained)
	{
		OUTPUT_MSG0(AT_FATAL, "\tOnly specified modules are patched with allocation routines.\n")
		OUTPUT_MSG0(AT_FATAL, "\tOnly specified modules are patched with deallocation routines\n")
	}
	else
	{
		OUTPUT_MSG0(AT_FATAL, "\tSpecified modules are patched with allocation routines.\n");
		OUTPUT_MSG0(AT_FATAL, "\tAll modules are patched with deallocation routines\n")
	}
	if(mPatchDlopen)
	{
		OUTPUT_MSG0(AT_FATAL, "\tDynamic load routine is going to be patched\n")
	}
	else
	{
		OUTPUT_MSG0(AT_FATAL, "\tDynamic load routine is not going to be patched\n")
	}
	if(mIncrementalPatch)
	{
		OUTPUT_MSG0(AT_FATAL, "\tModules are going to be patched incrementally\n")
	}
	else
	{
		OUTPUT_MSG0(AT_FATAL, "\tPatching are not incremental.\n")
	}

	OUTPUT_MSG0(AT_FATAL, "================= End of Configuration Definition =======================\n\n")
}

// For AIX/XCOFF
// We need to assign the default allocator function before patching
// because the patcher will use it as key in TOC to find the target
PatchManager::ModuleConfig::ModuleConfig()
  : mMode(AT_MODE_ADD_DEAD_PAGE)
{
}

PatchManager::ModuleConfig::~ModuleConfig()
{
	std::list<ModuleToPatch*>::iterator it;
	for(it=mModules.begin(); it!=mModules.end(); it++)
	{
		ModuleToPatch* lpModule = *it;
		delete lpModule;
	}
	mModules.clear();
}

// Be ware this may be calle multiple times, so don't duplicate modules
bool PatchManager::ModuleConfig::AddModule(const char* iModuleName)
{
	// Empty string is not allowed, Linux has executable name as ""
	//if(*iModuleName == '\0')
	//{
	//	return false;
	//}

	// Ignore it if already in the list
	std::list<ModuleToPatch*>::iterator it;
	for(it=mModules.begin(); it!=mModules.end(); it++)
	{
		if(::strcmp(iModuleName, (*it)->mModuleName) == 0)
		{
			return true;
		}
	}

	// pick out special name
	const char* lpModuleName = NULL;
	if(::strcmp(iModuleName, "exec") == 0)
	{
		// "exec" is a special module name for the executable itself
#ifdef WIN32
		lpModuleName = GetExecName();
#else
		// On all UNIX platforms, The fist module on link map list is the executable
		void* lpExecHandle = GetFirstModuleHandle();
		lpModuleName = GetModuleName(lpExecHandle);
#endif
	}
	else
	{
		lpModuleName = iModuleName;
	}
	// Now we know all about this to be patched module
	ModuleToPatch* lpModule = new ModuleToPatch(lpModuleName);
	if(lpModule)
	{
		InstallReplacementRoutines(lpModule, mModules.size());
		mModules.push_back(lpModule);
	}
	return true;
}


static void* PlainCalloc(size_t iNumElement, size_t iElementSize)
{
	size_t lTotalSize = iNumElement*iElementSize;
	void* lpResult = malloc(lTotalSize);
	if(lpResult && lTotalSize)
	{
		::memset(lpResult, 0, lTotalSize);
	}
	return lpResult;
};

static void* PlainRealloc(void* ipBlock, size_t iNewSize)
{
	return realloc(ipBlock, iNewSize);
}

static void PlainFree(void* ipBlock)
{
	::free(ipBlock);
}

void PatchManager::ModuleConfig::InstallReplacementRoutines(ModuleToPatch* lpModule, int iModuleNum)
{
#if defined(sun)
	if(::strstr(lpModule->mModuleName, "libthread.so")
		&& mMode==ModuleConfig::AT_MODE_ADD_RECORD)
	{
		lpModule->mNewMalloc  = malloc;
		lpModule->mNewCalloc  = PlainCalloc;
		lpModule->mNewFree    = PlainFree;
		lpModule->mNewRealloc = PlainRealloc;
		lpModule->mNewOperatorNew    = operator new;
		lpModule->mNewOperatorDelete = operator delete;
		lpModule->mNewOperatorNewArray    = operator new[];
		lpModule->mNewOperatorDeleteArray = operator delete[];
		return;
	}
#endif
	// If mode is known, install the replacement routine to be patched to this module
	switch(mMode)
	{
	case ModuleConfig::AT_MODE_ADD_DEAD_PAGE:
		lpModule->mNewMalloc  = (MallocFuncPtr) MallocBlockWithDeadPage;
		lpModule->mNewCalloc  = (CallocFuncPtr) CallocBlockWithDeadPage;
		lpModule->mNewFree    = (FreeFuncPtr) FreeBlockWithDeadPage;
		lpModule->mNewRealloc = (ReallocFuncPtr) ReallocBlockWithDeadPage;
		lpModule->mNewOperatorNew    = (OperatorNewPtr) NewBlockWithDeadPage;
		lpModule->mNewOperatorDelete = (OperatorDeletePtr) FreeBlockWithDeadPage;
		lpModule->mNewOperatorNewArray    = (OperatorNewArrayPtr) NewBlockWithDeadPage;
		lpModule->mNewOperatorDeleteArray = (OperatorDeleteArrayPtr) FreeBlockWithDeadPage;
		break;
	case ModuleConfig::AT_MODE_ADD_PADDING:
		lpModule->mNewMalloc  = (MallocFuncPtr) MallocBlockWithPadding;
		lpModule->mNewCalloc  = (CallocFuncPtr) CallocBlockWithPadding;
		lpModule->mNewFree    = (FreeFuncPtr) FreeBlockWithPadding;
		lpModule->mNewRealloc = (ReallocFuncPtr) ReallocBlockWithPadding;
		lpModule->mNewOperatorNew    = (OperatorNewPtr) NewBlockWithPadding;
		lpModule->mNewOperatorDelete = (OperatorDeletePtr) FreeBlockWithPadding;
		lpModule->mNewOperatorNewArray    = (OperatorNewArrayPtr) NewBlockWithPadding;
		lpModule->mNewOperatorDeleteArray = (OperatorDeleteArrayPtr) FreeBlockWithPadding;
		break;
	case ModuleConfig::AT_MODE_ADD_PRIVATE_HEAP:
		lpModule->mNewMalloc  = (MallocFuncPtr) MallocBlockFromPrivateHeap;
		lpModule->mNewCalloc  = (CallocFuncPtr) CallocBlockFromPrivateHeap;
		lpModule->mNewFree    = (FreeFuncPtr) FreeBlockFromPrivateHeap;
		lpModule->mNewRealloc = (ReallocFuncPtr) ReallocBlockFromPrivateHeap;
		lpModule->mNewOperatorNew    = (OperatorNewPtr) NewBlockFromPrivateHeap;
		lpModule->mNewOperatorDelete = (OperatorDeletePtr) FreeBlockFromPrivateHeap;
		lpModule->mNewOperatorNewArray    = (OperatorNewArrayPtr) NewBlockFromPrivateHeap;
		lpModule->mNewOperatorDeleteArray = (OperatorDeleteArrayPtr) FreeBlockFromPrivateHeap;
		break;
	case ModuleConfig::AT_MODE_ADD_RECORD:
		// The process has too many modules to handle
		// I haven't figure out a way to do it dynamically
		// We need really rebuild AccuTrak with bigger capacity
		if(iModuleNum >= MAX_MODULES_TO_RECORD)
		{
			::fprintf(stderr, "User selects %d modules to record\n", mModules.size());
			::fprintf(stderr, "But current AccuTrak can only handle %d modules\n", MAX_MODULES_TO_RECORD);
			::fprintf(stderr, "Please refer document to rebuild AccuTrak with bigger capacity\n");
			::fprintf(stderr, "Extra modules are combined with module %d for this run\n", MAX_MODULES_TO_RECORD-1);
			iModuleNum = MAX_MODULES_TO_RECORD-1;
		}
		lpModule->mNewMalloc  = (MallocFuncPtr) MallocBlockWithLog[iModuleNum];
		lpModule->mNewCalloc  = (CallocFuncPtr) CallocBlockWithLog[iModuleNum];
		lpModule->mNewFree    = (FreeFuncPtr) FreeBlockWithLog[iModuleNum];
		lpModule->mNewRealloc = (ReallocFuncPtr) ReallocBlockWithLog[iModuleNum];
		lpModule->mNewOperatorNew    = (OperatorNewPtr) NewBlockWithLog[iModuleNum];
		lpModule->mNewOperatorDelete = (OperatorDeletePtr) DeleteBlockWithLog[iModuleNum];
		lpModule->mNewOperatorNewArray    = (OperatorNewArrayPtr) NewArrayBlockWithLog[iModuleNum];
		lpModule->mNewOperatorDeleteArray = (OperatorDeleteArrayPtr) DeleteArrayBlockWithLog[iModuleNum];
		// Stash the module index in old function pointer in case we need it later.
		lpModule->mOldMalloc  = (MallocFuncPtr)  iModuleNum;
		lpModule->mOldCalloc  = (CallocFuncPtr)  iModuleNum;
		lpModule->mOldFree    = (FreeFuncPtr)    iModuleNum;
		lpModule->mOldRealloc = (ReallocFuncPtr) iModuleNum;
		lpModule->mOldOperatorNew    = (OperatorNewPtr)    iModuleNum;
		lpModule->mOldOperatorDelete = (OperatorDeletePtr) iModuleNum;
		lpModule->mOldOperatorNewArray    = (OperatorNewArrayPtr)    iModuleNum;
		lpModule->mOldOperatorDeleteArray = (OperatorDeleteArrayPtr) iModuleNum;
		break;
	default:
		// We don't know the mode yet
		break;
	}
}

//ModuleConfig* PatchManager::ModuleConfig::GetConfigByName(const char* iModuleName)
//{
//	std::list<ModuleToPatch*>::iterator it;
//	for(it=mModules.begin(); it!=mModules.end(); it++)
//	{
//		if(::strcmp(iModuleName, (*it)->mModuleName) == 0)
//		{
//			return (*it);
//		}
//	}
//	return NULL;
//}

// Ctor of ModuleToPatch
PatchManager::ModuleConfig::ModuleToPatch::ModuleToPatch(const char* iModuleName)
	: mPatched(false),
	mOldMalloc(malloc),   mNewMalloc(NULL),
	mOldFree(free),       mNewFree(NULL),
	mOldRealloc(realloc), mNewRealloc(NULL),
	mOldOperatorNew(operator new),       mNewOperatorNew(NULL),
	mOldOperatorDelete(operator delete), mNewOperatorDelete(NULL),
#if defined(linux) || defined(sun) || defined(__hpux) || defined(_AIX)
	mOldOperatorNewArray(operator new []),       mNewOperatorNewArray(NULL),
	mOldOperatorDeleteArray(operator delete[]), mNewOperatorDeleteArray()
#elif defined(WIN32)
	mOldOperatorNewArray(NULL),       mNewOperatorNewArray(NULL),
	mOldOperatorDeleteArray(NULL), mNewOperatorDeleteArray()
#else
#error Unknown platform
#endif
{
	mModuleName = ::strdup(iModuleName);
}

// Dtor of ModuleToPatch
PatchManager::ModuleConfig::ModuleToPatch::~ModuleToPatch()
{
	free(mModuleName);
}
