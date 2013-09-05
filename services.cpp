#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>
#include "ErrorParser.h"

BOOL InstallServiceEx(HWND hWnd, SC_HANDLE schSCManager, char *scName, DWORD scStartType, char *szPath) {
    SC_HANDLE schService;

	// ������� �� �������.
    schService = CreateService(
        schSCManager,              // SCM ���� � �����
        scName,                    // ��� �� �������
        scName,                    // ���������� ��� �� �������
        SERVICE_ALL_ACCESS,        // ������ ������
        SERVICE_KERNEL_DRIVER,     // ��� �� �������
        scStartType,			   // ��������� ���
        SERVICE_ERROR_NORMAL,      // ��������� ��� �� ������
        szPath,                    // ������� �� �������(*.sys)
        NULL,                      // ��� ��������� �����
        NULL,                      // ��� �������� �� ������������
        NULL,                      // ��� �����������
        NULL,                      // ������� ����
        NULL);                     // ��� ������
    if (schService == NULL) {
		MessageBox(hWnd, GetErrorText(GetLastError()), "FileHide", MB_ICONERROR);
		return FALSE;
    }

	MessageBox(hWnd, "�������� � ������� ����������", "FileHide", MB_ICONEXCLAMATION);
    CloseServiceHandle(schService);
	return TRUE;
}

BOOL DeleteServiceEx(HWND hWnd, SC_HANDLE schSCManager, char *szSvcName)
{
    SC_HANDLE schService;

    schService = OpenService(
        schSCManager,       // SCM ���� � �����
        szSvcName,          // ��� �� �������
        DELETE);            // ������ �� ���������
    if (schService == NULL) {
		MessageBox(hWnd, GetErrorText(GetLastError()), "FileHide", MB_ICONERROR);
        return FALSE;
    }

    // ������� �� �������
    if (!DeleteService(schService)) {
		MessageBox(hWnd, GetErrorText(GetLastError()), "FileHide", MB_ICONERROR);
		CloseServiceHandle(schService);
        return FALSE;
    }

	MessageBox(hWnd, "�������� � ������� �������", "FileHide", MB_ICONEXCLAMATION);
    CloseServiceHandle(schService);
	return TRUE;
}

BOOL StartServiceEx(HWND hWnd, SC_HANDLE schSCManager, char *szSvcName)
{
	SC_HANDLE schService;
    SERVICE_STATUS_PROCESS ssStatus;
    DWORD dwOldCheckPoint;
    DWORD dwStartTickCount;
    DWORD dwWaitTime;
    DWORD dwBytesNeeded;
	char str[50];

    schService = OpenService(
        schSCManager,         // SCM ���� � �����
        szSvcName,            // ��� �� �������
        SERVICE_ALL_ACCESS);  // ����� ������

    if (schService == NULL) {
		MessageBox(hWnd, GetErrorText(GetLastError()), "FileHide", MB_ICONERROR);
        return FALSE;
    }

	// �������� �� ����������� � ������, �� ������� �� � �����.
    if (!QueryServiceStatusEx(
            schService,                     // ������ �� �������
            SC_STATUS_PROCESS_INFO,         // ������������� ����
            (LPBYTE) &ssStatus,             // ������ �� ���������
            sizeof(SERVICE_STATUS_PROCESS), // �������� �� ���������
            &dwBytesNeeded))              	// ���������� ��������, ��� ������ � ����� �����
    {
        MessageBox(hWnd, GetErrorText(GetLastError()), "FileHide", MB_ICONERROR);
        CloseServiceHandle(schService);
        return FALSE;
    }

	// ��������� ���� ������� ������.
    if(ssStatus.dwCurrentState != SERVICE_STOPPED && ssStatus.dwCurrentState != SERVICE_STOP_PENDING) {
		MessageBox(hWnd, "�������� �� ���� �� �� ��������, ������ � ���� ��������� � ������", "FileHide", MB_ICONERROR);
        CloseServiceHandle(schService);
        return FALSE;
    }

	// ��������� �� ��������� � ������������� ���������-�������������� �����.
    dwStartTickCount = GetTickCount();
    dwOldCheckPoint = ssStatus.dwCheckPoint;

	// ��������� �� ������� �� ���� ����� �� �� ����� �� ��������.
    while (ssStatus.dwCurrentState == SERVICE_STOP_PENDING) {
		// �� �� �� ���� ��-�����, ��������� ���������� �����. ��������� �
		// ���� ������ �� ���������� �����, �� �� ������� �� ���� �������
		// � �� ��-���� �� 10 �������.
        dwWaitTime = ssStatus.dwWaitHint / 10;

        if(dwWaitTime < 1000)
            dwWaitTime = 1000;
        else if (dwWaitTime > 10000)
            dwWaitTime = 10000;

        Sleep(dwWaitTime);

		// �������� �� �����������, ������ ������� �� ���� � ��������.
        if (!QueryServiceStatusEx(
                schService,                     // ������ �� �������
                SC_STATUS_PROCESS_INFO,         // ������������� ����
                (LPBYTE) &ssStatus,             // ������ �� ���������
                sizeof(SERVICE_STATUS_PROCESS), // �������� �� ���������
                &dwBytesNeeded))              	// ���������� ��������, ��� ������ � ����� �����
        {
			MessageBox(hWnd, GetErrorText(GetLastError()), "FileHide", MB_ICONERROR);
            CloseServiceHandle(schService);
            return FALSE;
        }

        if (ssStatus.dwCheckPoint > dwOldCheckPoint) {
			// ������������ �� ����������� � ����������.
            dwStartTickCount = GetTickCount();
            dwOldCheckPoint = ssStatus.dwCheckPoint;
        } else {
            if(GetTickCount()-dwStartTickCount > ssStatus.dwWaitHint) {
				MessageBox(hWnd, "������� �� ������ �� ������� �� ������� ������", "FileHide", MB_ICONERROR);
                CloseServiceHandle(schService);
                return FALSE;
            }
        }
    }

	// ���� �� �� �������� �������.
    if (!StartService(
            schService,  // handle to service
            0,           // number of arguments
            NULL) )      // no arguments
    {
        MessageBox(hWnd, GetErrorText(GetLastError()), "FileHide", MB_ICONERROR);
        CloseServiceHandle(schService);
        return FALSE;
    }

	// ������� ����������...
	// �������� �� �����������, ������ ������� �� ������� � ��������.
    if (!QueryServiceStatusEx(
            schService,                     // ������ �� �������
            SC_STATUS_PROCESS_INFO,         // ������������� ����
            (LPBYTE) &ssStatus,             // ������ �� ���������
            sizeof(SERVICE_STATUS_PROCESS), // �������� �� ���������
            &dwBytesNeeded))             	// ��� ������ � ����� �����
    {
        MessageBox(hWnd, GetErrorText(GetLastError()), "FileHide", MB_ICONERROR);
        CloseServiceHandle(schService);
        return FALSE;
    }

	// ��������� �� ��������� � ������������� ���������-�������������� �����.
    dwStartTickCount = GetTickCount();
    dwOldCheckPoint = ssStatus.dwCheckPoint;

    while (ssStatus.dwCurrentState == SERVICE_START_PENDING) {

        dwWaitTime = ssStatus.dwWaitHint / 10;

        if(dwWaitTime < 1000)
            dwWaitTime = 1000;
        else if (dwWaitTime > 10000)
            dwWaitTime = 10000;

        Sleep(dwWaitTime);

        // ������ �������� �� �����������.
        if (!QueryServiceStatusEx(
            schService,
            SC_STATUS_PROCESS_INFO,
            (LPBYTE) &ssStatus,
            sizeof(SERVICE_STATUS_PROCESS),
            &dwBytesNeeded))
        {
            MessageBox(hWnd, GetErrorText(GetLastError()), "FileHide", MB_ICONERROR);
            break;
        }

        if (ssStatus.dwCheckPoint > dwOldCheckPoint) {
			// ������������ �� ����������� � ����������.
            dwStartTickCount = GetTickCount();
            dwOldCheckPoint = ssStatus.dwCheckPoint;
        } else {
            if(GetTickCount()-dwStartTickCount > ssStatus.dwWaitHint) {
				// ��� �������� � ��������� �����.
                break;
            }
        }
    }

	// �������� ���� ������� � ���������.
    if (ssStatus.dwCurrentState == SERVICE_RUNNING) {
        MessageBox(hWnd, "�������� � ���������", "FileHide", MB_ICONEXCLAMATION);
		CloseServiceHandle(schService);
		return TRUE;
    } else {
		sprintf(str, "�������� �� � ���������.\n");
		sprintf(str, "������ ���������: %d\n", ssStatus.dwCurrentState);
        sprintf(str, "  Exit Code: %d\n", ssStatus.dwWin32ExitCode);
        sprintf(str, "  Check Point: %d\n", ssStatus.dwCheckPoint);
        sprintf(str, "  Wait Hint: %d\n", ssStatus.dwWaitHint);
		MessageBox(hWnd, str, "FileHide", MB_ICONERROR);
		CloseServiceHandle(schService);
		return FALSE;
    }

}

BOOL StopDependentServices(SC_HANDLE hSCManager, SC_HANDLE hService);

BOOL StopServiceEx(HWND hWnd, SC_HANDLE schSCManager, char *szSvcName)
{
	SC_HANDLE schService;
    SERVICE_STATUS_PROCESS ssp;
    DWORD dwStartTime = GetTickCount();
    DWORD dwBytesNeeded;
    DWORD dwTimeout = 30000; // 30 �������� �����
    DWORD dwWaitTime;

    schService = OpenService(schSCManager, szSvcName,
							SERVICE_STOP | SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS);
    if (schService == NULL) {
        MessageBox(hWnd, GetErrorText(GetLastError()), "FileHide", MB_ICONERROR);
        return FALSE;
    }

	// ��������� ���� ������� �� � ���� �����.
    if (!QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp,
								sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded))
    {
        MessageBox(hWnd, GetErrorText(GetLastError()), "FileHide", MB_ICONERROR);
        CloseServiceHandle(schService);
		return FALSE;
    }

    if (ssp.dwCurrentState == SERVICE_STOPPED) {
        MessageBox(hWnd, "�������� � ��� �����", "FileHide", MB_ICONERROR);
        CloseServiceHandle(schService);
		return FALSE;
    }

	// ��� ����������� � � ��������, ������� ��.
    while (ssp.dwCurrentState == SERVICE_STOP_PENDING) {
        dwWaitTime = ssp.dwWaitHint / 10;
        if(dwWaitTime < 1000)
            dwWaitTime = 1000;
        else if (dwWaitTime > 10000)
            dwWaitTime = 10000;

        Sleep(dwWaitTime);

        if ( !QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp,
									sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded))
        {
			MessageBox(hWnd, GetErrorText(GetLastError()), "FileHide", MB_ICONERROR);
			CloseServiceHandle(schService);
			return FALSE;
        }

        if (ssp.dwCurrentState == SERVICE_STOPPED) {
			MessageBox(hWnd, "�������� � �����", "FileHide", MB_ICONEXCLAMATION);
			CloseServiceHandle(schService);
			return TRUE;
        }

        if (GetTickCount() - dwStartTime > dwTimeout) {
			MessageBox(hWnd, "������� �� ������� �� ������� ������", "FileHide", MB_ICONERROR);
			CloseServiceHandle(schService);
			return FALSE;
        }
    }

	// ��� ������� ������, ���������� ������� ����� �� ������.
    StopDependentServices(schSCManager, schService);

	// ������� ���� ��� ��� �������.
    if (!ControlService(schService, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&ssp)) {
		MessageBox(hWnd, GetErrorText(GetLastError()), "FileHide", MB_ICONERROR);
		CloseServiceHandle(schService);
        return FALSE;
    }

	// ������ ������� �� ����.
    while (ssp.dwCurrentState != SERVICE_STOPPED) {
        Sleep(ssp.dwWaitHint);

        if (!QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp,
									sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded))
        {
			MessageBox(hWnd, GetErrorText(GetLastError()), "FileHide", MB_ICONERROR);
			CloseServiceHandle(schService);
			return FALSE;
        }

        if (ssp.dwCurrentState == SERVICE_STOPPED)
            break;

        if (GetTickCount() - dwStartTime > dwTimeout) {
			MessageBox(hWnd, "������� �� ������� �� ������� ������", "FileHide", MB_ICONERROR);
			CloseServiceHandle(schService);
			return FALSE;
        }
    }

	MessageBox(hWnd, "�������� � �����", "FileHide", MB_ICONEXCLAMATION);
	CloseServiceHandle(schService);
	return TRUE;
}

BOOL StopDependentServices(SC_HANDLE hSCManager, SC_HANDLE hService)
{
    LPENUM_SERVICE_STATUS   lpDependencies = NULL;
    ENUM_SERVICE_STATUS     ess;
    SC_HANDLE               hDepService;
    SERVICE_STATUS_PROCESS  ssp;
	DWORD i, dwBytesNeeded, dwCount;

    DWORD dwStartTime = GetTickCount();
    DWORD dwTimeout = 30000; // 30 �������� �����

	// ������� �� ����� � ������ �������, �� �� �� ������ ������� ������ �� ������.
    if (EnumDependentServices(hService, SERVICE_ACTIVE, lpDependencies, 0, &dwBytesNeeded, &dwCount))
		// ��� ����������� �� Enum-� � �������, ������ ���� ��������
		// �������, ���� �� �� �� ����� ����.
        return TRUE;
	
	if (GetLastError() != ERROR_MORE_DATA)
		return FALSE; // ���������� ������

	// ��������� �� ����� �� �������������.
	lpDependencies = (LPENUM_SERVICE_STATUS)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwBytesNeeded);
	if (!lpDependencies)
		return FALSE;

	__try {
		// ��������� �� �������������.
		if (!EnumDependentServices(hService, SERVICE_ACTIVE, lpDependencies,
									dwBytesNeeded, &dwBytesNeeded, &dwCount))
			return FALSE;

		for (i=0; i<dwCount; i++) {
			ess = *(lpDependencies + i);
			// ������ �� �������
			hDepService = OpenService(hSCManager, ess.lpServiceName, SERVICE_STOP|SERVICE_QUERY_STATUS);
			if (!hDepService)
			   return FALSE;

			__try {
				// ������� �� ���� ���.
				if (!ControlService(hDepService, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&ssp))
					return FALSE;

				// ������� �� ������� �� ����.
				while (ssp.dwCurrentState != SERVICE_STOPPED) {
					Sleep(ssp.dwWaitHint);
					if (!QueryServiceStatusEx(hDepService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp,
												sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded))
						return FALSE;

					if (ssp.dwCurrentState == SERVICE_STOPPED)
						break;

					if (GetTickCount() - dwStartTime > dwTimeout)
						return FALSE;
				}
			} __finally {
				// ������ �� ������� �������� �� �������.
				CloseServiceHandle(hDepService);
			}
		}
	} __finally {
		// ������ �� ����������� ��������� �����.
		HeapFree(GetProcessHeap(), 0, lpDependencies);
	}
	return TRUE;
}

BOOL IsServiceInstalled(SC_HANDLE schSCManager, char *szSvcName)
{
	SC_HANDLE schService;

	schService = OpenService(
        schSCManager,         // SCM ���� � �����
        szSvcName,            // ��� �� �������
        SERVICE_ALL_ACCESS);  // ����� ������
    if (schService != NULL)
        return TRUE;
	return FALSE;
}

BOOL IsServiceRunning(HWND hWnd, SC_HANDLE schSCManager, char *szSvcName)
{
	SC_HANDLE schService;
    SERVICE_STATUS_PROCESS ssStatus;
    DWORD dwBytesNeeded;

	schService = OpenService(
        schSCManager,         // SCM ���� � �����
        szSvcName,            // ��� �� �������
        SERVICE_ALL_ACCESS);  // ����� ������

    if (schService == NULL) {
        return FALSE;
    }

    if (!QueryServiceStatusEx(
            schService,                     // ������ �� �������
            SC_STATUS_PROCESS_INFO,         // ������������� ����
            (LPBYTE) &ssStatus,             // ������ �� ���������
            sizeof(SERVICE_STATUS_PROCESS), // �������� �� ���������
            &dwBytesNeeded))              	// ���������� ��������, ��� ������ � ����� �����
    {
        MessageBox(hWnd, GetErrorText(GetLastError()), "FileHide", MB_ICONERROR);
        CloseServiceHandle(schService);
        return FALSE;
    }
	// ��������� ���� ������� ������.
    if(ssStatus.dwCurrentState == SERVICE_RUNNING) {
        CloseServiceHandle(schService);
        return TRUE;
    }

	return FALSE;
}