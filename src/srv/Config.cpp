#include "Config.h"

#include "Consts.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace EGL3::Service {
    int RunInstall()
    {
        SC_HANDLE SCManager;
        SC_HANDLE Service;
        CHAR FilePath[MAX_PATH];

        if (!GetModuleFileName(NULL, FilePath, MAX_PATH))
        {
            printf("Cannot install service (%d)\n", GetLastError());
            return GetLastError();
        }

        std::string QuotedFilePath = "\"" + std::string(FilePath) + "\"";

        // Get a handle to the SCM database. 

        SCManager = OpenSCManager(
            NULL,                    // local computer
            NULL,                    // ServicesActive database 
            SC_MANAGER_ALL_ACCESS);  // full access rights 

        if (SCManager == NULL)
        {
            printf("OpenSCManager failed (%d)\n", GetLastError());
            return GetLastError();
        }

        // Create the service

        Service = CreateService(
            SCManager,                 // SCM database 
            GetServiceName(),          // name of service 
            GetServiceName(),          // service name to display 
            SERVICE_ALL_ACCESS,        // desired access 
            SERVICE_WIN32_OWN_PROCESS, // service type 
            SERVICE_AUTO_START,        // start type 
            SERVICE_ERROR_NORMAL,      // error control type 
            QuotedFilePath.c_str(),    // path to service's binary 
            NULL,                      // no load ordering group 
            NULL,                      // no tag identifier 
            NULL,                      // no dependencies 
            NULL,                      // LocalSystem account 
            NULL);                     // no password 

        if (Service == NULL)
        {
            printf("CreateService failed (%d)\n", GetLastError());
            CloseServiceHandle(SCManager);
            return GetLastError();
        }

        printf("Service installed successfully\n");
        CloseServiceHandle(Service);
        CloseServiceHandle(SCManager);

        return ERROR_SUCCESS;
    }

    int RunQuery()
    {
        SC_HANDLE SCManager;
        SC_HANDLE Service;
        LPQUERY_SERVICE_CONFIG ServiceConfig;
        LPSERVICE_DESCRIPTION ServiceDesc;

        // Get a handle to the SCM database. 

        SCManager = OpenSCManager(
            NULL,                    // local computer
            NULL,                    // ServicesActive database 
            SC_MANAGER_ALL_ACCESS);  // full access rights 

        if (SCManager == NULL)
        {
            printf("OpenSCManager failed (%d)\n", GetLastError());
            return GetLastError();
        }

        // Get a handle to the service.

        Service = OpenService(
            SCManager,             // SCM database 
            GetServiceName(),      // name of service 
            SERVICE_QUERY_CONFIG); // need query config access 

        if (Service == NULL)
        {
            printf("OpenService failed (%d)\n", GetLastError());
            CloseServiceHandle(SCManager);
            return GetLastError();
        }

        // Get the configuration information.

        DWORD BytesNeeded;
        DWORD BufSize;
        if (!QueryServiceConfig(Service, NULL, 0, &BytesNeeded))
        {
            DWORD Error = GetLastError();
            if (Error == ERROR_INSUFFICIENT_BUFFER)
            {
                BufSize = BytesNeeded;
                ServiceConfig = (LPQUERY_SERVICE_CONFIG)LocalAlloc(LMEM_FIXED, BufSize);
            }
            else
            {
                printf("QueryServiceConfig failed (%d)", Error);
                goto cleanup;
            }
        }

        if (!QueryServiceConfig(Service, ServiceConfig, BufSize, &BytesNeeded))
        {
            printf("QueryServiceConfig failed (%d)", GetLastError());
            goto cleanup;
        }

        if (!QueryServiceConfig2(Service, SERVICE_CONFIG_DESCRIPTION, NULL, 0, &BytesNeeded))
        {
            DWORD Error = GetLastError();
            if (ERROR_INSUFFICIENT_BUFFER == Error)
            {
                BufSize = BytesNeeded;
                ServiceDesc = (LPSERVICE_DESCRIPTION)LocalAlloc(LMEM_FIXED, BufSize);
            }
            else
            {
                printf("QueryServiceConfig2 failed (%d)", Error);
                goto cleanup;
            }
        }

        if (!QueryServiceConfig2(Service, SERVICE_CONFIG_DESCRIPTION, (LPBYTE)ServiceDesc, BufSize, &BytesNeeded))
        {
            printf("QueryServiceConfig2 failed (%d)", GetLastError());
            goto cleanup;
        }

        // Print the configuration information.

        printf("%s configuration: \n", GetServiceName());
        printf("  Type: 0x%x\n", ServiceConfig->dwServiceType);
        printf("  Start Type: 0x%x\n", ServiceConfig->dwStartType);
        printf("  Error Control: 0x%x\n", ServiceConfig->dwErrorControl);
        printf("  Binary path: %s\n", ServiceConfig->lpBinaryPathName);
        printf("  Account: %s\n", ServiceConfig->lpServiceStartName);

        if (ServiceDesc->lpDescription != NULL && lstrcmp(ServiceDesc->lpDescription, TEXT("")) != 0)
            printf("  Description: %s\n", ServiceDesc->lpDescription);
        if (ServiceConfig->lpLoadOrderGroup != NULL && lstrcmp(ServiceConfig->lpLoadOrderGroup, TEXT("")) != 0)
            printf("  Load order group: %s\n", ServiceConfig->lpLoadOrderGroup);
        if (ServiceConfig->dwTagId != 0)
            printf("  Tag ID: %d\n", ServiceConfig->dwTagId);
        if (ServiceConfig->lpDependencies != NULL && lstrcmp(ServiceConfig->lpDependencies, TEXT("")) != 0)
            printf("  Dependencies: %s\n", ServiceConfig->lpDependencies);

        LocalFree(ServiceConfig);
        LocalFree(ServiceDesc);

    cleanup:
        CloseServiceHandle(Service);
        CloseServiceHandle(SCManager);

        return GetLastError();
    }

    int RunDescribe()
    {
        SC_HANDLE SCManager;
        SC_HANDLE Service;
        SERVICE_DESCRIPTION ServiceDesc;

        // Get a handle to the SCM database. 

        SCManager = OpenSCManager(
            NULL,                    // local computer
            NULL,                    // ServicesActive database 
            SC_MANAGER_ALL_ACCESS);  // full access rights 

        if (SCManager == NULL)
        {
            printf("OpenSCManager failed (%d)\n", GetLastError());
            return GetLastError();
        }

        // Get a handle to the service.

        Service = OpenService(
            SCManager,              // SCM database 
            GetServiceName(),       // name of service 
            SERVICE_CHANGE_CONFIG); // need change config access 

        if (Service == NULL)
        {
            printf("OpenService failed (%d)\n", GetLastError());
            CloseServiceHandle(SCManager);
            return GetLastError();
        }

        // Change the service description.

        ServiceDesc.lpDescription = (LPSTR)GetServiceDescription();

        if (!ChangeServiceConfig2(Service, SERVICE_CONFIG_DESCRIPTION, &ServiceDesc))
        {
            printf("ChangeServiceConfig2 failed\n");
        }
        else {
            printf("Service description updated successfully.\n");
        }

        CloseServiceHandle(Service);
        CloseServiceHandle(SCManager);

        return GetLastError();
    }

    int RunEnable()
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
            return GetLastError();
        }

        // Get a handle to the service.

        Service = OpenService(
            SCManager,              // SCM database 
            GetServiceName(),       // name of service 
            SERVICE_CHANGE_CONFIG); // need change config access 

        if (Service == NULL)
        {
            printf("OpenService failed (%d)\n", GetLastError());
            CloseServiceHandle(SCManager);
            return GetLastError();
        }

        // Change the service start type.

        if (!ChangeServiceConfig(Service, SERVICE_NO_CHANGE, SERVICE_AUTO_START, SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL, NULL, NULL))
        {
            printf("ChangeServiceConfig failed (%d)\n", GetLastError());
        }
        else {
            printf("Service enabled successfully.\n");
        }

        CloseServiceHandle(Service);
        CloseServiceHandle(SCManager);

        return GetLastError();
    }

    int RunDisable()
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
            return GetLastError();
        }

        // Get a handle to the service.

        Service = OpenService(
            SCManager,              // SCM database 
            GetServiceName(),       // name of service 
            SERVICE_CHANGE_CONFIG); // need change config access 

        if (Service == NULL)
        {
            printf("OpenService failed (%d)\n", GetLastError());
            CloseServiceHandle(SCManager);
            return GetLastError();
        }

        // Change the service start type.

        if (!ChangeServiceConfig(Service, SERVICE_NO_CHANGE, SERVICE_DISABLED, SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL, NULL, NULL))
        {
            printf("ChangeServiceConfig failed (%d)\n", GetLastError());
        }
        else {
            printf("Service disabled successfully.\n");
        }

        CloseServiceHandle(Service);
        CloseServiceHandle(SCManager);

        return GetLastError();
    }

    int RunDelete()
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
            return GetLastError();
        }

        // Get a handle to the service.

        Service = OpenService(
            SCManager,        // SCM database 
            GetServiceName(), // name of service 
            DELETE);          // need delete access 

        if (Service == NULL)
        {
            printf("OpenService failed (%d)\n", GetLastError());
            CloseServiceHandle(SCManager);
            return GetLastError();
        }

        // Delete the service.

        if (!DeleteService(Service))
        {
            printf("DeleteService failed (%d)\n", GetLastError());
        }
        else {
            printf("Service deleted successfully\n");
        }

        CloseServiceHandle(Service);
        CloseServiceHandle(SCManager);

        return GetLastError();
    }
}