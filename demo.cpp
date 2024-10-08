//
// demo.cpp
//
// This is a small program to demonstrate how to hook the
// CreateProcess Windows API using the Microsoft Detours
// library.  Since CreateProcess is actually two APIs
// depending on the string format, we will be hooking both
// of them (CreateProcessA and CreateProcessW).  For
// purposes of this demo, we are only hooking APIs in the
// current process.
//
// General Description:
//
// 1. Hooks the CreateProcessA and CreateProcessW functions
//    to call our hook functions instead.  The API hooking
//    is done with the help of the Detours library.
// 
// 2. Each time one of our hook functions gets called, it
//    increments a count of how many times the hook
//    was called, and then it makes a pass-through call
//    to the original API function that our hook replaced.
// 
// 3. In order to demonstrate that our hook functions can
//    actually be called through the (hooked) Windows APIs,
//    the program then launches several common Windows
//    utility applications, using CreateProcess API calls,
//    over a time period of about ten seconds.
// 
// 4. When the tests are complete, we unhook CreateProcessA
//    and CreateProcessW.
// 
// 5. Lastly, we print the number of times each hook
//    function was called.  If that number matches the
//    number of times our test called CreateProcess APIs,
//    then the test passes.
//
// Note all program output goes to stdout, including errors.
//
// Compiles with Microsoft C++ compiler from Visual Studio
// 2022.  Also requires the Microsoft Detours library.
//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include "detours.h"

// Function signature of CreateProcessW system API.
typedef BOOL (WINAPI * CREATEPROCESSWFUNC)(LPCWSTR, LPWSTR,
            LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES,
            BOOL, DWORD, LPVOID, LPCWSTR, LPSTARTUPINFOW,
            LPPROCESS_INFORMATION);

// Function signature of CreateProcessA system API.
typedef BOOL (WINAPI * CREATEPROCESSAFUNC)(LPCSTR, LPSTR,
            LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES,
            BOOL, DWORD, LPVOID, LPCSTR, LPSTARTUPINFOA,
            LPPROCESS_INFORMATION);

// Pointers to the API functions we'll be hooking into.
static CREATEPROCESSWFUNC PtrCreateProcessW = CreateProcessW;
static CREATEPROCESSAFUNC PtrCreateProcessA = CreateProcessA;

// For keeping track of how many times our hooks were called.
static int numCallsToCreateProcessA = 0;
static int numCallsToCreateProcessW = 0;

// Synchronization lock.
static volatile CHAR busy = 0;

//---------------------------------------------------------------
// API HOOKING CODE
//---------------------------------------------------------------

//
// Windows will call this hook function whenever CreateProcessW
// is called.
//
BOOL WINAPI HookedCreateProcessW(
    LPCWSTR               lpApplicationName,
    LPWSTR                lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL                  bInheritHandles,
    DWORD                 dwCreationFlags,
    LPVOID                lpEnvironment,
    LPCWSTR               lpCurrentDirectory,
    LPSTARTUPINFOW        lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    )
{
    // Only allow one thread to access our globals at the same time.
    // This isn't actually needed for a single-threaded demo, but
    // it's good practice.
    while (InterlockedExchange8(&busy, 1)) { /* Intentionally empty */ }

    // Keep track of how many times we were called.
    numCallsToCreateProcessW++;

    // If we wanted to do any other processing or data exchange
    // during this API call, the code would go here.
    // ...
    // ...

    // Unlock.
    busy = 0;

    // Pass-thru call to the original API that we hooked into.
    return PtrCreateProcessW(lpApplicationName, lpCommandLine,
        lpProcessAttributes, lpThreadAttributes, bInheritHandles,
        dwCreationFlags, lpEnvironment, lpCurrentDirectory,
        lpStartupInfo, lpProcessInformation);
}

//
// Windows will call this hook function whenever CreateProcessA
// is called.
//
BOOL WINAPI HookedCreateProcessA(
    LPCSTR                lpApplicationName,
    LPSTR                 lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL                  bInheritHandles,
    DWORD                 dwCreationFlags,
    LPVOID                lpEnvironment,
    LPCSTR                lpCurrentDirectory,
    LPSTARTUPINFOA        lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    )
{
    // Only allow one thread to access our globals at the same time.
    // This isn't actually needed for a single-threaded demo, but
    // it's good practice.
    while (InterlockedExchange8(&busy, 1)) { /* Intentionally empty */ }

    // Keep track of how many times we were called.
    numCallsToCreateProcessA++;

    // If we wanted to do any other processing or data exchange
    // during this API call, the code would go here.
    // ...
    // ...

    // Unlock.
    busy = 0;

    // Pass-thru call to the original API that we hooked into.
    return PtrCreateProcessA(lpApplicationName, lpCommandLine,
        lpProcessAttributes, lpThreadAttributes, bInheritHandles,
        dwCreationFlags, lpEnvironment, lpCurrentDirectory,
        lpStartupInfo, lpProcessInformation);
}

//
// Installs our API function hooks using Detours.
// Returns true if successful.
//
static bool InstallHooks()
{
    printf("Installing API hooks.\n");

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    if (DetourAttach(&(PVOID &)PtrCreateProcessW, HookedCreateProcessW) != NO_ERROR)
    {
        printf("ERROR: Failed hooking CreateProcessW!\n");
        return false;
    }
    if (DetourAttach(&(PVOID &)PtrCreateProcessA, HookedCreateProcessA) != NO_ERROR)
    {
        printf("ERROR: Failed hooking CreateProcessA!\n");
        return false;
    }
    DetourTransactionCommit();

    return true;
}

//
// Removes our previously installed API function hooks.
//
static void RemoveHooks()
{
    printf("Removing API hooks.\n");

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach(&(PVOID &)PtrCreateProcessW, HookedCreateProcessW);
    DetourDetach(&(PVOID &)PtrCreateProcessA, HookedCreateProcessA);
    DetourTransactionCommit();
}

//---------------------------------------------------------------
// TESTING CODE
//---------------------------------------------------------------

//
// Kills the Windows process associated with the given process ID.
//
static void KillProcess(DWORD processid)
{
    if (!processid)
        return;

    const auto hprocess = OpenProcess(PROCESS_TERMINATE, false, processid);
    TerminateProcess(hprocess, 1);
    CloseHandle(hprocess);
}

//
// Launch an app by name, using the CreateProcessA API.
// Returns the process ID if successful, zero if error.
//
static DWORD RunAppWithCreateProcessA(const char *appname)
{
    if (!appname || !*appname)
        return 0;

    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);

    char cmdline[MAX_PATH] = {0};
    strcpy_s(cmdline, appname);
    printf("Calling CreateProcessA with \"%s\"\n", cmdline);
    if (!CreateProcessA(nullptr, cmdline, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi))
    {
        printf("Failed running \"%s\"\n", appname);
        return 0;
    }

    printf("Created process ID %lu\n", pi.dwProcessId);
    return pi.dwProcessId;
}

//
// Launch an app by name, using the CreateProcessW API.
// Returns the process ID if successful, zero if error.
//
static DWORD RunAppWithCreateProcessW(const wchar_t *appname)
{
    if (!appname || !*appname)
        return 0;

    STARTUPINFOW si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);

    wchar_t cmdline[MAX_PATH] = {0};
    wcscpy_s(cmdline, appname);
    printf("Calling CreateProcessW with \"%S\"\n", cmdline);
    if (!CreateProcessW(nullptr, cmdline, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi))
    {
        printf("Failed running \"%S\"\n", appname);
        return 0;
    }

    printf("Created process ID %lu\n", pi.dwProcessId);
    return pi.dwProcessId;
}

//
// Launches several common Windows apps and utilities over a
// period of several seconds, so we can verify that our hooks
// are actually being called.  Returns the number of apps we
// tried to run.
//
static int RunAppsForTesting()
{
    int count = 0;

    printf("\n============================================================\n");
    printf("Test: Running some Windows apps using CreateProcess calls.\n");
    printf("============================================================\n");

    // Run some apps using CreateProcessA.
    const char *appnames[] = { "charmap", "dxdiag", "find \"README\" readme*", "msinfo32", "mspaint", "app_that_doesnt_exist", nullptr };
    for (int i = 0; appnames[i] != nullptr; i++)
    {
        Sleep(500);
        DWORD processId = RunAppWithCreateProcessA(appnames[i]);
        if (processId)
        {
            Sleep(500);
            printf("Killing process ID %lu\n", processId);
            KillProcess(processId);
        }

        count++;
    }

    // Run some apps using CreateProcessW.
    const wchar_t *appnamesW[] = { L"charmap", L"comp /N=1 /M README.md README.md", L"tasklist /m explorer*", L"systeminfo", L"findstr README readme*", L"app_that_doesnt_exist", nullptr };
    for (int i = 0; appnamesW[i] != nullptr; i++)
    {
        Sleep(500);
        DWORD processId = RunAppWithCreateProcessW(appnamesW[i]);
        if (processId)
        {
            Sleep(500);
            printf("Killing process ID %lu\n", processId);
            KillProcess(processId);
        }

        count++;
    }

    return count;
}

//
// Prints test results to the console.
// Returns true if test passes, false if test fails.
//
static bool CheckResults(int numAppsRun)
{
    printf("\n============================================================\n");
    printf("TEST RESULTS:\n");
    printf("* Number of CreateProcessA calls during test:  %d\n", numCallsToCreateProcessA);
    printf("* Number of CreateProcessW calls during test:  %d\n", numCallsToCreateProcessW);

    int numHookCalls = numCallsToCreateProcessA + numCallsToCreateProcessW;
    if (numAppsRun > numHookCalls)
    {
        printf("\nTEST FAIL: Received %d total hook calls, but expected at least %d!\n", numHookCalls, numAppsRun);
        printf("============================================================\n");
        return false;
    }

    printf("\nTEST PASS: Received the expected number of hook calls.\n");
    printf("============================================================\n");
    return true;
}

//---------------------------------------------------------------

int main()
{
    if (!InstallHooks())
        return -1;

    int numAppsRun = 0;

    try
    {
        numAppsRun = RunAppsForTesting();
    }
    catch(...)
    {
        RemoveHooks();
        printf("ERROR: Program aborting due to exception!\n");
        return -1;
    }

    RemoveHooks();
    if (!CheckResults(numAppsRun))
        return -1;

    return 0;
}

