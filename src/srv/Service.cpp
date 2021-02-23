#include "Service.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define SVC_ERROR ((DWORD)0xC0020001L)

SERVICE_STATUS_HANDLE SvcStatusHandle;
SERVICE_STATUS SvcStatus;
HANDLE SvcStopEvent = NULL;

VOID WINAPI SvcCtrlHandler(DWORD Ctrl);
VOID WINAPI SvcMain(DWORD argc, LPSTR* argv);

VOID ReportSvcStatus(DWORD CurrentState, DWORD ExitCode, DWORD WaitHint);
VOID SvcInit(DWORD argc, LPSTR* argv);
VOID SvcReportEvent(LPCSTR Function);

void EGL3SvcMain()
{
    SERVICE_TABLE_ENTRY DispatchTable[] = {
        { (LPSTR)SVCNAME, (LPSERVICE_MAIN_FUNCTION)SvcMain },
        { NULL, NULL }
    };

    if (!StartServiceCtrlDispatcher(DispatchTable)) {
        SvcReportEvent("StartServiceCtrlDispatcher");
    }
}

void EGL3SvcInstall()
{
    SC_HANDLE SCManager;
    SC_HANDLE Service;
    CHAR FilePath[MAX_PATH];

    if (!GetModuleFileName(NULL, FilePath, MAX_PATH))
    {
        printf("Cannot install service (%d)\n", GetLastError());
        return;
    }

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

    // Create the service

    Service = CreateService(
        SCManager,                 // SCM database 
        SVCNAME,                   // name of service 
        SVCNAME,                   // service name to display 
        SERVICE_ALL_ACCESS,        // desired access 
        SERVICE_WIN32_OWN_PROCESS, // service type 
        SERVICE_DEMAND_START,      // start type 
        SERVICE_ERROR_NORMAL,      // error control type 
        FilePath,                  // path to service's binary 
        NULL,                      // no load ordering group 
        NULL,                      // no tag identifier 
        NULL,                      // no dependencies 
        NULL,                      // LocalSystem account 
        NULL);                     // no password 

    if (Service == NULL)
    {
        printf("CreateService failed (%d)\n", GetLastError());
        CloseServiceHandle(SCManager);
        return;
    }

    printf("Service installed successfully\n");
    CloseServiceHandle(Service);
    CloseServiceHandle(SCManager);
}

VOID WINAPI SvcMain(DWORD argc, LPSTR* argv)
{
    SvcStatusHandle = RegisterServiceCtrlHandler(SVCNAME, SvcCtrlHandler);

    if (SvcStatusHandle == NULL) {
        SvcReportEvent("RegisterServiceCtrlHandler");
        return;
    }

    SvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    SvcStatus.dwServiceSpecificExitCode = 0;

    ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

    SvcInit(argc, argv);
}

VOID SvcInit(DWORD argc, LPSTR* argv)
{
    SvcStopEvent = CreateEvent(
        NULL,    // default security attributes
        TRUE,    // manual reset event
        FALSE,   // not signaled
        NULL);   // no name

    if (SvcStopEvent == NULL)
    {
        ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
        return;
    }

    // Report running status when initialization is complete.

    ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);

    // TO_DO: Perform work until service stops.

    while (1)
    {
        // Check whether to stop the service.

        WaitForSingleObject(SvcStopEvent, INFINITE);

        ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
        return;
    }
}

VOID ReportSvcStatus(DWORD CurrentState, DWORD ExitCode, DWORD WaitHint)
{
    static DWORD dwCheckPoint = 1;

    SvcStatus.dwCurrentState = CurrentState;
    SvcStatus.dwWin32ExitCode = ExitCode;
    SvcStatus.dwWaitHint = WaitHint;

    if (CurrentState == SERVICE_START_PENDING) {
        SvcStatus.dwControlsAccepted = 0;
    }
    else {
        SvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    }

    if (CurrentState == SERVICE_RUNNING || CurrentState == SERVICE_STOPPED) {
        SvcStatus.dwCheckPoint = 0;
    }
    else {
        SvcStatus.dwCheckPoint = dwCheckPoint++;
    }

    SetServiceStatus(SvcStatusHandle, &SvcStatus);
}

VOID WINAPI SvcCtrlHandler(DWORD Ctrl)
{
    switch (Ctrl)
    {
    case SERVICE_CONTROL_STOP:
        ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
        SetEvent(SvcStopEvent);
        ReportSvcStatus(SvcStatus.dwCurrentState, NO_ERROR, 0);
        break;
    case SERVICE_CONTROL_INTERROGATE:
        break;
    default:
        break;
    }
}

VOID SvcReportEvent(LPCSTR Function)
{
    HANDLE hEventSource;
    LPCSTR lpszStrings[2];
    CHAR Buffer[80];

    hEventSource = RegisterEventSource(NULL, SVCNAME);

    if (NULL != hEventSource)
    {
        sprintf_s(Buffer, "%s failed with %d", Function, GetLastError());

        lpszStrings[0] = SVCNAME;
        lpszStrings[1] = Buffer;

        ReportEvent(hEventSource,// event log handle
            EVENTLOG_ERROR_TYPE, // event type
            0,                   // event category
            SVC_ERROR,           // event identifier
            NULL,                // no security identifier
            2,                   // size of lpszStrings array
            0,                   // no binary data
            lpszStrings,         // array of strings
            NULL);               // no binary data

        DeregisterEventSource(hEventSource);
    }
}