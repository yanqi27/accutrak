# Microsoft Developer Studio Generated NMAKE File, Based on AccuTrak.dsp

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe


OUTDIR=..\lib\Win32
INTDIR=.\obj

ALL : "$(OUTDIR)" "$(INTDIR)" "$(OUTDIR)\AccuTrak.dll" ToolSuite TestSuite

ToolSuite : MallocLogViewer.exe MallocLogConsolidator.exe MallocReplay.exe MallocHeapViewer.exe

#TestSuite: TestPrivateHeap
TestSuite : TestPatch.exe TestOverrun.exe TestUnderrun.exe TestPatchDSO.exe foo.dll TestRealloc.exe \
	TestNew.exe TestRecord.exe

CLEAN :
	-@erase "$(INTDIR)\*.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\AccuTrak.*"
	-@erase "Malloc*"
	-@erase "foo*"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CXXFLAG=/nologo /W3 /GX /Od /D "WIN32" /D "_MBCS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c /I ../inc -I ./

# release compiler option
CPP_PROJ=     /MD /ZI /D "NDEBUG" /D "_WINDOWS" $(CXXFLAG) /D "_USRDLL" /YX
CPP_EXEC_PROJ=/MD /Zi /D "NDEBUG" /D "_CONSOLE" $(CXXFLAG) /D "_SMARTHEAP" /D "_HEAP_PROFILE"

# debug compiler option
#CPP_PROJ=     /MDd /Gm /ZI /D "_DEBUG" /D "_WINDOWS" $(CXXFLAG) /D "_USRDLL" /YX
#CPP_EXEC_PROJ=/MDd /Zi     /D "_DEBUG" /D "_CONSOLE" $(CXXFLAG) /D "_SMARTHEAP"


#BSC32=bscmake.exe
#BSC32_FLAGS=/nologo /o"$(OUTDIR)\AccuTrak.bsc" 
#BSC32_SBRS= \


LINK32=link.exe

SYS_LIBS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib

# release link libs
LINK32_FLAGS=$(SYS_LIBS) psapi.lib /nologo /dll /incremental:yes /debug /machine:I386 /nodefaultlib:"libcmt.lib" /pdbtype:sept /pdb:"$*.pdb" /out:"$@" /implib:"$*.lib" $**

LINK32_EXEC_FLAGS=$(SYS_LIBS) accutrak.lib /nodefaultlib:"libcd.lib" /nologo /subsystem:console /incremental:yes /debug /machine:I386 /pdbtype:sept /LIBPATH:"$(OUTDIR)" /pdb:"$*.pdb" /out:$@ $**

# debug link libs
#LINK32_FLAGS=Kernel32.lib user32.lib psapi.lib /nologo /dll /incremental:yes /debug /machine:I386 /nodefaultlib:"libcmtd.lib" /pdbtype:sept 

ACCUTRAK_OBJS= \
	"$(INTDIR)\AccuTrak.obj" \
	"$(INTDIR)\ConfigParser.obj" \
	"$(INTDIR)\MallocDeadPage.obj" \
	"$(INTDIR)\MallocLogger.obj" \
	"$(INTDIR)\MallocPadding.obj" \
	"$(INTDIR)\MallocPrivateHeap.obj" \
	"$(INTDIR)\PatchManager.obj" \
	"$(INTDIR)\PatchPE32.obj" \
	"$(INTDIR)\SysAlloc.obj" \
	"$(INTDIR)\Utility.obj"

##########################################################
# AccuTrak DLL
##########################################################
"$(OUTDIR)\AccuTrak.dll" : $(ACCUTRAK_OBJS)
    $(LINK32) $(LINK32_FLAGS) 

##########################################################
# Utility tools
##########################################################
"MallocLogViewer.exe" : $(INTDIR)\MallocLogViewer.obj $(INTDIR)\Utility.obj
    $(LINK32) $(LINK32_EXEC_FLAGS)

MallocLogConsolidator.exe : $(INTDIR)\MallocLogConsolidator.obj $(INTDIR)\Utility.obj
    $(LINK32) $(LINK32_EXEC_FLAGS)

MallocReplay.exe : $(INTDIR)\MallocReplay.obj $(INTDIR)\Utility.obj $(INTDIR)\StaticBufferMgr.obj $(INTDIR)\CustomMalloc.obj $(INTDIR)\PerfCounter.obj
    $(LINK32) $(LINK32_EXEC_FLAGS) pdh.lib shsmp.lib

MallocHeapViewer.exe : $(INTDIR)\MallocHeapViewer.obj $(INTDIR)\HeapImage.obj
    $(LINK32) $(LINK32_EXEC_FLAGS) M8Chart.lib chrtxsdk.lib

##########################################################
# Test Suite
##########################################################
foo.dll : $(INTDIR)\foo.obj
    $(LINK32) $(LINK32_FLAGS) 

TestRecord.exe : $(INTDIR)\TestRecord.obj
    $(LINK32) $(LINK32_EXEC_FLAGS)

TestPatch.exe : $(INTDIR)\TestPatch.obj
    $(LINK32) $(LINK32_EXEC_FLAGS)

TestOverrun.exe : $(INTDIR)\TestOverrun.obj
    $(LINK32) $(LINK32_EXEC_FLAGS)

TestUnderrun.exe : $(INTDIR)\TestUnderrun.obj
    $(LINK32) $(LINK32_EXEC_FLAGS)

TestPatchDSO.exe : $(INTDIR)\TestPatchDSO.obj
    $(LINK32) $(LINK32_EXEC_FLAGS)

TestRealloc.exe : $(INTDIR)\TestRealloc.obj
    $(LINK32) $(LINK32_EXEC_FLAGS)

TestNew.exe : $(INTDIR)\TestNew.obj
    $(LINK32) $(LINK32_EXEC_FLAGS)

##########################################################
# Run Test
##########################################################
run: Test1 Test2 Test3 Test4 Test5 Test6 Test7 Test8 Test9

Test1 : TestPatch.exe
	TestPatch.bat

Test2 : TestOverrun.exe
	TestOverrun.bat

Test3 : TestUnderrun.exe
	TestUnderrun.bat

Test4 : TestPatchDSO.exe
	TestPatchDSO.bat

Test5 : TestRealloc.exe
	TestRealloc.bat

Test6 : "$(OUTDIR)\AccuTrak.dll"
	echo Preload is not supported on Window32

Test7 : TestNew.exe
	TestNew.bat

Test8 : "$(OUTDIR)\AccuTrak.dll"
	echo Private heap is coming soon on WIN32

Test9 : TestRecord.exe
	TestRecord.bat

##########################################################
# Object rule
##########################################################
#.cpp{$(INTDIR)}.obj::
#   $(CPP) $(CPP_PROJ) $< 

#.cpp{$(INTDIR)}.sbr::
#   $(CPP) $(CPP_PROJ) $< 


SOURCE=..\src\AccuTrak.cpp

"$(INTDIR)\AccuTrak.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\src\ConfigParser.cpp

"$(INTDIR)\ConfigParser.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\src\MallocDeadPage.cpp

"$(INTDIR)\MallocDeadPage.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\src\MallocLogger.cpp

"$(INTDIR)\MallocLogger.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\src\MallocPadding.cpp

"$(INTDIR)\MallocPadding.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\src\MallocPrivateHeap.cpp

"$(INTDIR)\MallocPrivateHeap.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\src\PatchManager.cpp

"$(INTDIR)\PatchManager.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\src\PatchPE32.cpp

"$(INTDIR)\PatchPE32.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\src\SysAlloc.cpp

"$(INTDIR)\SysAlloc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\src\Utility.cpp

"$(INTDIR)\Utility.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\src\StaticBufferMgr.cpp

"$(INTDIR)\StaticBufferMgr.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\src\CustomMalloc.cpp

"$(INTDIR)\CustomMalloc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\src\PerfCounter.cpp

"$(INTDIR)\PerfCounter.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\src\MallocLogViewer.cpp

$(INTDIR)\MallocLogViewer.obj : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_EXEC_PROJ) $(SOURCE)


SOURCE=..\src\MallocLogConsolidator.cpp

$(INTDIR)\MallocLogConsolidator.obj : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_EXEC_PROJ) $(SOURCE)


SOURCE=..\src\MallocReplay.cpp

$(INTDIR)\MallocReplay.obj : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_EXEC_PROJ) $(SOURCE)


SOURCE=..\src\MallocHeapViewer.cpp

$(INTDIR)\MallocHeapViewer.obj : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_EXEC_PROJ) $(SOURCE)


SOURCE=..\src\HeapImage.cpp

$(INTDIR)\HeapImage.obj : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_EXEC_PROJ) $(SOURCE)


SOURCE=..\test\TestPatch.cpp

$(INTDIR)\TestPatch.obj : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_EXEC_PROJ) $(SOURCE)


SOURCE=..\test\TestOverrun.cpp

$(INTDIR)\TestOverrun.obj : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_EXEC_PROJ) $(SOURCE)


SOURCE=..\test\TestUnderrun.cpp

$(INTDIR)\TestUnderrun.obj : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_EXEC_PROJ) $(SOURCE)


SOURCE=..\test\TestPatchDSO.cpp

$(INTDIR)\TestPatchDSO.obj : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_EXEC_PROJ) $(SOURCE)


SOURCE=..\test\TestRealloc.cpp

$(INTDIR)\TestRealloc.obj : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_EXEC_PROJ) $(SOURCE)


SOURCE=..\test\TestNew.cpp

$(INTDIR)\TestNew.obj : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_EXEC_PROJ) $(SOURCE)


SOURCE=..\test\foo.cpp

"$(INTDIR)\foo.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\test\TestRecord.cpp

$(INTDIR)\TestRecord.obj : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_EXEC_PROJ) $(SOURCE)


