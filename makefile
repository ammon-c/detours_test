# NMAKE script to build program to demonstrate how to use the
# Detours library to hook Windows API CreateProcess.
#
# Assumes Microsoft C++ compiler and linker are installed and
# accessible from the command prompt.

.SUFFIXES: .cpp

.cpp.obj:
    cl -nologo -c -W4 -WX -EHsc -Zi -I.\dependencies $<

all:  demo.exe

demo.exe:  demo.obj dependencies\detours.lib
    link /NOLOGO /DEBUG /OUT:$@ $**

demo.obj:  demo.cpp dependencies\detours.h

clean:
    if exist *.exe del *.exe
    if exist *.obj del *.obj
    if exist *.ilk del *.ilk
    if exist *.pdb del *.pdb
    if exist *.bak del *.bak
    if exist .vs rd /s /q .vs
