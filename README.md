## README - Program to demonstrate how to hook the CreateProcess API using Detours.lib 

This is a small program to demonstrate how to hook the
CreateProcess Windows API using the Microsoft Detours library. 
Since CreateProcess is actually two APIs depending on the string
format, the program hooks both of them (CreateProcessA and
CreateProcessW).  

For purposes of this demo, we are only hooking APIs in the
current process.  


### What this program does:

1. Hooks the CreateProcessA and CreateProcessW functions
   to call our hook functions instead.  The API hooking
   is done with the help of the Detours library.

2. Each time one of our hook functions gets called, it
   increments a count of how many times the hook
   was called, and then it makes a pass-through call
   to the original API function that our hook replaced.

3. In order to demonstrate that our hook functions can
   actually be called through the (hooked) Windows APIs,
   the program then runs a test where it launches several
   common Windows utility applications, using
   CreateProcess API calls, over a time period of about
   ten seconds. 

4. When the tests are complete, we unhook CreateProcessA
   and CreateProcessW.

5. Lastly, we print the number of times each hook function
   was called.  If that number matches the number of times
   our test called CreateProcess APIs, then the test
   passes.

---

### Build:

This assumes the Microsoft C++ compiler for x64 (from Visual
Studio 2019 or 2022) is installed and accessible from the
Windows command prompt.  

At the Windows command prompt, **CD** to the directory that
contains the demo project, and then run **NMAKE**.

If the build is successful, the **demo.exe** file is created.  

---

### Run:

Run **demo.exe** at the Windows command prompt.  For about ten
seconds, the demo application will launch a handful of Windows
applications while counting how many times the hooked API
functions get called, after which it will print how many times
each hook function was called.  

Note that all program output goes to stdout, including error
messages.  

---

### Example program output:

```
Installing API hooks.

============================================================
Test: Running some Windows apps using CreateProcess calls.
============================================================
Calling CreateProcessA with "charmap"
Created process ID 26184
Killing process ID 26184
Calling CreateProcessA with "dxdiag"
Created process ID 14448
Killing process ID 14448
Calling CreateProcessA with "find "README" readme*"
Created process ID 13572
Killing process ID 13572
Calling CreateProcessA with "msinfo32"
Created process ID 15304
Killing process ID 15304
Calling CreateProcessA with "mspaint"
Created process ID 12832
Killing process ID 12832
Calling CreateProcessA with "app_that_doesnt_exist"
Failed running "app_that_doesnt_exist"
Calling CreateProcessW with "charmap"
Created process ID 24164
Killing process ID 24164
Calling CreateProcessW with "comp /N=1 /M README.md README.md"
Created process ID 17064
Killing process ID 17064
Calling CreateProcessW with "tasklist /m explorer*"
Created process ID 12108
Killing process ID 12108
Calling CreateProcessW with "systeminfo"
Created process ID 24896
Killing process ID 24896
Calling CreateProcessW with "findstr README readme*"
Created process ID 24736
Killing process ID 24736
Calling CreateProcessW with "app_that_doesnt_exist"
Failed running "app_that_doesnt_exist"
Removing API hooks.

============================================================
TEST RESULTS:
* Number of CreateProcessA calls during test:  6
* Number of CreateProcessW calls during test:  6

TEST PASS: Received the expected number of hook calls.
============================================================
```

-*- end -*-
