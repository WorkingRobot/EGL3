#include "Service.h"

#include "sock/server/Server.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace EGL3::Service {
    SERVICE_STATUS_HANDLE SvcStatusHandle;
    HANDLE SvcStopEvent;
    SERVICE_STATUS SvcStatus;

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

        EGL3::Service::Sock::Server Server;

        // Report running status when initialization is complete.

        ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);

        // Check whether to stop the service.

        WaitForSingleObject(SvcStopEvent, INFINITE);

        ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
    }

    VOID SvcReportEvent(LPCSTR Function)
    {
        HANDLE hEventSource = RegisterEventSource(NULL, SERVICE_NAME);

        if (hEventSource != NULL)
        {
            CHAR Buffer[80];
            sprintf_s(Buffer, "%s failed with %d", Function, GetLastError());

            LPCSTR lpszStrings[2] = { SERVICE_NAME, Buffer };

            ReportEvent(hEventSource,// event log handle
                EVENTLOG_ERROR_TYPE, // event type
                0,                   // event category
                0xC0020001L,         // event identifier
                NULL,                // no security identifier
                2,                   // size of lpszStrings array
                0,                   // no binary data
                lpszStrings,         // array of strings
                NULL);               // no binary data

            DeregisterEventSource(hEventSource);
        }
    }

    VOID WINAPI SvcMain(DWORD argc, LPSTR* argv)
    {
        SvcStopEvent = NULL;

        SvcStatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, SvcCtrlHandler);

        if (SvcStatusHandle == NULL) {
            SvcReportEvent("RegisterServiceCtrlHandler");
            return;
        }

        SvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
        SvcStatus.dwServiceSpecificExitCode = 0;

        ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

        SvcInit(argc, argv);
    }

    void RunMain()
    {
        SERVICE_TABLE_ENTRY DispatchTable[] = {
            { (LPSTR)SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)SvcMain },
            { NULL, NULL }
        };

        if (!StartServiceCtrlDispatcher(DispatchTable)) {
            SvcReportEvent("StartServiceCtrlDispatcher");
        }
    }
}
