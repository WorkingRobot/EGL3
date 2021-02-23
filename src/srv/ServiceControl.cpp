#include "ServiceControl.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <AclAPI.h>

void EGL3SvcStart()
{
    SC_HANDLE SCManager;
    SC_HANDLE Service;
    SERVICE_STATUS_PROCESS Status;

    // Get a handle to the SCM database. 

    SCManager = OpenSCManager(
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 

    if (SCManager == NULL)
    {
        printf("OpenSCManager failed (%d)\n", GetLastError());
        return;
    }

    // Get a handle to the service.

    Service = OpenService(
        SCManager,           // SCM database 
        SVCNAME,             // name of service 
        SERVICE_ALL_ACCESS); // full access

    if (Service == NULL)
    {
        printf("OpenService failed (%d)\n", GetLastError());
        CloseServiceHandle(SCManager);
        return;
    }

    // Check the status in case the service is not stopped. 

    DWORD BytesNeeded;
    if (!QueryServiceStatusEx(Service, SC_STATUS_PROCESS_INFO, (LPBYTE)&Status, sizeof(Status), &BytesNeeded))
    {
        printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
        CloseServiceHandle(Service);
        CloseServiceHandle(SCManager);
        return;
    }

    // Check if the service is already running. It would be possible 
    // to stop the service here, but for simplicity this example just returns. 

    if (Status.dwCurrentState != SERVICE_STOPPED && Status.dwCurrentState != SERVICE_STOP_PENDING)
    {
        printf("Cannot start the service because it is already running\n");
        CloseServiceHandle(Service);
        CloseServiceHandle(SCManager);
        return;
    }

    // Save the tick count and initial checkpoint.

    DWORD StartTickCount = GetTickCount();
    DWORD OldCheckPoint = Status.dwCheckPoint;

    // Wait for the service to stop before attempting to start it.

    while (Status.dwCurrentState == SERVICE_STOP_PENDING)
    {
        // Do not wait longer than the wait hint. A good interval is 
        // one-tenth of the wait hint but not less than 1 second  
        // and not more than 10 seconds. 

        DWORD WaitTime = Status.dwWaitHint / 10;

        if (WaitTime < 1000)
            WaitTime = 1000;
        else if (WaitTime > 10000)
            WaitTime = 10000;

        Sleep(WaitTime);

        // Check the status until the service is no longer stop pending. 

        if (!QueryServiceStatusEx(Service, SC_STATUS_PROCESS_INFO, (LPBYTE)&Status, sizeof(Status), &BytesNeeded))
        {
            printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
            CloseServiceHandle(Service);
            CloseServiceHandle(SCManager);
            return;
        }

        if (Status.dwCheckPoint > OldCheckPoint)
        {
            // Continue to wait and check.

            StartTickCount = GetTickCount();
            OldCheckPoint = Status.dwCheckPoint;
        }
        else
        {
            if (GetTickCount() - StartTickCount > Status.dwWaitHint)
            {
                printf("Timeout waiting for service to stop\n");
                CloseServiceHandle(Service);
                CloseServiceHandle(SCManager);
                return;
            }
        }
    }

    // Attempt to start the service.

    if (!StartService(Service, 0, NULL))
    {
        printf("StartService failed (%d)\n", GetLastError());
        CloseServiceHandle(Service);
        CloseServiceHandle(SCManager);
        return;
    }
    else
    {
        printf("Service start pending...\n");
    }

    // Check the status until the service is no longer start pending. 

    if (!QueryServiceStatusEx(Service, SC_STATUS_PROCESS_INFO, (LPBYTE)&Status, sizeof(Status), &BytesNeeded))
    {
        printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
        CloseServiceHandle(Service);
        CloseServiceHandle(SCManager);
        return;
    }

    // Save the tick count and initial checkpoint.

    StartTickCount = GetTickCount();
    OldCheckPoint = Status.dwCheckPoint;

    while (Status.dwCurrentState == SERVICE_START_PENDING)
    {
        // Do not wait longer than the wait hint. A good interval is 
        // one-tenth the wait hint, but no less than 1 second and no 
        // more than 10 seconds. 

        DWORD WaitTime = Status.dwWaitHint / 10;

        if (WaitTime < 1000)
            WaitTime = 1000;
        else if (WaitTime > 10000)
            WaitTime = 10000;

        Sleep(WaitTime);

        // Check the status again. 

        if (!QueryServiceStatusEx(Service, SC_STATUS_PROCESS_INFO, (LPBYTE)&Status, sizeof(Status), &BytesNeeded))
        {
            printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
            break;
        }

        if (Status.dwCheckPoint > OldCheckPoint)
        {
            // Continue to wait and check.

            StartTickCount = GetTickCount();
            OldCheckPoint = Status.dwCheckPoint;
        }
        else
        {
            if (GetTickCount() - StartTickCount > Status.dwWaitHint)
            {
                // No progress made within the wait hint.
                break;
            }
        }
    }

    // Determine whether the service is running.

    if (Status.dwCurrentState == SERVICE_RUNNING)
    {
        printf("Service started successfully.\n");
    }
    else
    {
        printf("Service not started. \n");
        printf("  Current State: %d\n", Status.dwCurrentState);
        printf("  Exit Code: %d\n", Status.dwWin32ExitCode);
        printf("  Check Point: %d\n", Status.dwCheckPoint);
        printf("  Wait Hint: %d\n", Status.dwWaitHint);
    }

    CloseServiceHandle(Service);
    CloseServiceHandle(SCManager);
}

void EGL3SvcStop()
{
    SC_HANDLE SCManager;
    SC_HANDLE Service;
    SERVICE_STATUS_PROCESS Status;
    ULONGLONG StartTime = GetTickCount64();
    DWORD Timeout = 30000; // 30-second time-out

    // Get a handle to the SCM database. 

    SCManager = OpenSCManager(
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 

    if (SCManager == NULL)
    {
        printf("OpenSCManager failed (%d)\n", GetLastError());
        return;
    }

    // Get a handle to the service.

    Service = OpenService(
        SCManager,           // SCM database 
        SVCNAME,             // name of service 
        SERVICE_STOP |
        SERVICE_QUERY_STATUS |
        SERVICE_ENUMERATE_DEPENDENTS);

    if (Service == NULL)
    {
        printf("OpenService failed (%d)\n", GetLastError());
        CloseServiceHandle(SCManager);
        return;
    }

    // Check the status in case the service is not stopped. 

    DWORD BytesNeeded;
    if (!QueryServiceStatusEx(Service, SC_STATUS_PROCESS_INFO, (LPBYTE)&Status, sizeof(Status), &BytesNeeded))
    {
        printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
        CloseServiceHandle(Service);
        CloseServiceHandle(SCManager);
        return;
    }

    if (Status.dwCurrentState == SERVICE_STOP)
    {
        printf("Service is already stopped.\n");
        CloseServiceHandle(Service);
        CloseServiceHandle(SCManager);
        return;
    }

    // If a stop is pending, wait for it.

    while (Status.dwCurrentState == SERVICE_STOP_PENDING)
    {
        printf("Service stop pending...\n");

        // Do not wait longer than the wait hint. A good interval is 
        // one-tenth of the wait hint but not less than 1 second  
        // and not more than 10 seconds. 

        DWORD WaitTime = Status.dwWaitHint / 10;

        if (WaitTime < 1000)
            WaitTime = 1000;
        else if (WaitTime > 10000)
            WaitTime = 10000;

        Sleep(WaitTime);

        // Check the status until the service is no longer stop pending. 

        if (!QueryServiceStatusEx(Service, SC_STATUS_PROCESS_INFO, (LPBYTE)&Status, sizeof(Status), &BytesNeeded))
        {
            printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
            CloseServiceHandle(Service);
            CloseServiceHandle(SCManager);
            return;
        }

        if (Status.dwCurrentState == SERVICE_STOPPED)
        {
            printf("Service stopped successfully.\n");
            CloseServiceHandle(Service);
            CloseServiceHandle(SCManager);
            return;
        }

        if (GetTickCount64() - StartTime > Timeout)
        {
            printf("Service stop timed out.\n");
            CloseServiceHandle(Service);
            CloseServiceHandle(SCManager);
            return;
        }
    }

    // If the service is running, dependencies must be stopped first.
    // But EGL3 doesn't have any dependent services, so this isn't necessary.

    // StopDependentServices();

    // Send a stop code to the service.

    if (!ControlService(Service, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&Status))
    {
        printf("ControlService failed (%d)\n", GetLastError());
        CloseServiceHandle(Service);
        CloseServiceHandle(SCManager);
        return;
    }

    // Wait for the service to stop.

    while (Status.dwCurrentState != SERVICE_STOPPED)
    {
        Sleep(Status.dwWaitHint);
        if (!QueryServiceStatusEx(Service, SC_STATUS_PROCESS_INFO, (LPBYTE)&Status, sizeof(Status), &BytesNeeded))
        {
            printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
            CloseServiceHandle(Service);
            CloseServiceHandle(SCManager);
            return;
        }

        if (Status.dwCurrentState == SERVICE_STOPPED)
            break;

        if (GetTickCount64() - StartTime > Timeout)
        {
            printf("Wait timed out\n");
            CloseServiceHandle(Service);
            CloseServiceHandle(SCManager);
            return;
        }
    }
    printf("Service stopped successfully\n");

    CloseServiceHandle(Service);
    CloseServiceHandle(SCManager);
}

void EGL3SvcDacl()
{
    SC_HANDLE SCManager;
    SC_HANDLE Service;

    // Get a handle to the SCM database. 

    SCManager = OpenSCManager(
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 

    if (SCManager == NULL)
    {
        printf("OpenSCManager failed (%d)\n", GetLastError());
        return;
    }

    // Get a handle to the service.

    Service = OpenService(
        SCManager,                 // SCM database 
        SVCNAME,                   // name of service 
        READ_CONTROL | WRITE_DAC);

    if (Service == NULL)
    {
        printf("OpenService failed (%d)\n", GetLastError());
        CloseServiceHandle(SCManager);
        return;
    }

    // Get the current security descriptor.

    PSECURITY_DESCRIPTOR PSecDesc;
    SECURITY_DESCRIPTOR SecDesc;
    DWORD BytesNeeded;

    PACL PAcl;
    PACL PNewAcl;
    BOOL DaclPresent = FALSE;
    BOOL DaclDefaulted = FALSE;

    EXPLICIT_ACCESS Ea;

    DWORD Error;

    if (!QueryServiceObjectSecurity(Service, DACL_SECURITY_INFORMATION, &PSecDesc, 0, &BytesNeeded))
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            DWORD Size = BytesNeeded;
            PSecDesc = (PSECURITY_DESCRIPTOR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Size);
            if (PSecDesc == NULL)
            {
                // Note: HeapAlloc does not support GetLastError.
                printf("HeapAlloc failed\n");
                goto dacl_cleanup;
            }

            if (!QueryServiceObjectSecurity(Service, DACL_SECURITY_INFORMATION, PSecDesc, Size, &BytesNeeded))
            {
                printf("QueryServiceObjectSecurity failed (%d)\n", GetLastError());
                goto dacl_cleanup;
            }
        }
        else
        {
            printf("QueryServiceObjectSecurity failed (%d)\n", GetLastError());
            goto dacl_cleanup;
        }
    }

    // Get the DACL.

    if (!GetSecurityDescriptorDacl(PSecDesc, &DaclPresent, &PAcl, &DaclDefaulted))
    {
        printf("GetSecurityDescriptorDacl failed(%d)\n", GetLastError());
        goto dacl_cleanup;
    }

    // Build the ACE.

    BuildExplicitAccessWithName(&Ea, (LPSTR)"GUEST",
        SERVICE_START | SERVICE_STOP | READ_CONTROL | DELETE,
        SET_ACCESS, NO_INHERITANCE);

    Error = SetEntriesInAcl(1, &Ea, PAcl, &PNewAcl);
    if (Error != ERROR_SUCCESS)
    {
        printf("SetEntriesInAcl failed(%d)\n", Error);
        goto dacl_cleanup;
    }

    // Initialize a new security descriptor.

    if (!InitializeSecurityDescriptor(&SecDesc, SECURITY_DESCRIPTOR_REVISION))
    {
        printf("InitializeSecurityDescriptor failed(%d)\n", GetLastError());
        goto dacl_cleanup;
    }

    // Set the new DACL in the security descriptor.

    if (!SetSecurityDescriptorDacl(&SecDesc, TRUE, PNewAcl, FALSE))
    {
        printf("SetSecurityDescriptorDacl failed(%d)\n", GetLastError());
        goto dacl_cleanup;
    }

    // Set the new DACL for the service object.

    if (!SetServiceObjectSecurity(Service, DACL_SECURITY_INFORMATION, &SecDesc))
    {
        printf("SetServiceObjectSecurity failed(%d)\n", GetLastError());
        goto dacl_cleanup;
    }
    else printf("Service DACL updated successfully\n");

dacl_cleanup:
    CloseServiceHandle(SCManager);
    CloseServiceHandle(Service);

    if (NULL != PNewAcl)
        LocalFree((HLOCAL)PNewAcl);
    if (NULL != PSecDesc)
        HeapFree(GetProcessHeap(), 0, (LPVOID)PSecDesc);
}