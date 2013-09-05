#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>
#include "ErrorParser.h"

BOOL InstallServiceEx(HWND hWnd, SC_HANDLE schSCManager, char *scName, DWORD scStartType, char *szPath) {
    SC_HANDLE schService;

	// Създава го сервиса.
    schService = CreateService(
        schSCManager,              // SCM база с данни
        scName,                    // име на сервиса
        scName,                    // изложеното име на сервиса
        SERVICE_ALL_ACCESS,        // желаещ достъп
        SERVICE_KERNEL_DRIVER,     // тип на сервиса
        scStartType,			   // стартуващ тип
        SERVICE_ERROR_NORMAL,      // контролен тип за грешки
        szPath,                    // пътечка до сервиса(*.sys)
        NULL,                      // без зареждаща група
        NULL,                      // без прибавка за установяване
        NULL,                      // без зависимости
        NULL,                      // локален опис
        NULL);                     // без парола
    if (schService == NULL) {
		MessageBox(hWnd, GetErrorText(GetLastError()), "FileHide", MB_ICONERROR);
		return FALSE;
    }

	MessageBox(hWnd, "Сервисът е успешно инсталиран", "FileHide", MB_ICONEXCLAMATION);
    CloseServiceHandle(schService);
	return TRUE;
}

BOOL DeleteServiceEx(HWND hWnd, SC_HANDLE schSCManager, char *szSvcName)
{
    SC_HANDLE schService;

    schService = OpenService(
        schSCManager,       // SCM база с данни
        szSvcName,          // име на сервиса
        DELETE);            // достъп за изтриване
    if (schService == NULL) {
		MessageBox(hWnd, GetErrorText(GetLastError()), "FileHide", MB_ICONERROR);
        return FALSE;
    }

    // Изтрива го сервиса
    if (!DeleteService(schService)) {
		MessageBox(hWnd, GetErrorText(GetLastError()), "FileHide", MB_ICONERROR);
		CloseServiceHandle(schService);
        return FALSE;
    }

	MessageBox(hWnd, "Сервисът е успешно изтриен", "FileHide", MB_ICONEXCLAMATION);
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
        schSCManager,         // SCM база с данни
        szSvcName,            // име на сервиса
        SERVICE_ALL_ACCESS);  // пълен достъп

    if (schService == NULL) {
		MessageBox(hWnd, GetErrorText(GetLastError()), "FileHide", MB_ICONERROR);
        return FALSE;
    }

	// Проверка на състоянието в случай, че сервиса не е спрял.
    if (!QueryServiceStatusEx(
            schService,                     // дръжка до сервиса
            SC_STATUS_PROCESS_INFO,         // информационно ниво
            (LPBYTE) &ssStatus,             // адреса на структура
            sizeof(SERVICE_STATUS_PROCESS), // големина на структура
            &dwBytesNeeded))              	// необходима големина, ако буфера е много малък
    {
        MessageBox(hWnd, GetErrorText(GetLastError()), "FileHide", MB_ICONERROR);
        CloseServiceHandle(schService);
        return FALSE;
    }

	// Проверява дали сервиса работи.
    if(ssStatus.dwCurrentState != SERVICE_STOPPED && ssStatus.dwCurrentState != SERVICE_STOP_PENDING) {
		MessageBox(hWnd, "Сервисът не може да се стартува, защото е вече стартуван и работи", "FileHide", MB_ICONERROR);
        CloseServiceHandle(schService);
        return FALSE;
    }

	// Спасяване на цъканията и първоначалния контролно-пропусквателен пункт.
    dwStartTickCount = GetTickCount();
    dwOldCheckPoint = ssStatus.dwCheckPoint;

	// Изчакване на сервиса да спре преди да се опита да стартува.
    while (ssStatus.dwCurrentState == SERVICE_STOP_PENDING) {
		// Да не се чака по-дълго, отколкото изчакващия намек. Интервала е
		// една десета от изчакващия намек, но не помалко от една секунда
		// и не по-вече от 10 секунди.
        dwWaitTime = ssStatus.dwWaitHint / 10;

        if(dwWaitTime < 1000)
            dwWaitTime = 1000;
        else if (dwWaitTime > 10000)
            dwWaitTime = 10000;

        Sleep(dwWaitTime);

		// Проверка на състоянието, докато сервиса не спре в очакване.
        if (!QueryServiceStatusEx(
                schService,                     // дръжка до сервиса
                SC_STATUS_PROCESS_INFO,         // информационно ниво
                (LPBYTE) &ssStatus,             // адреса на структура
                sizeof(SERVICE_STATUS_PROCESS), // големина на структура
                &dwBytesNeeded))              	// необходима големина, ако буфера е много малък
        {
			MessageBox(hWnd, GetErrorText(GetLastError()), "FileHide", MB_ICONERROR);
            CloseServiceHandle(schService);
            return FALSE;
        }

        if (ssStatus.dwCheckPoint > dwOldCheckPoint) {
			// Продължаване на изчакването и проверката.
            dwStartTickCount = GetTickCount();
            dwOldCheckPoint = ssStatus.dwCheckPoint;
        } else {
            if(GetTickCount()-dwStartTickCount > ssStatus.dwWaitHint) {
				MessageBox(hWnd, "Времето за чакане на сервиса за спиране измина", "FileHide", MB_ICONERROR);
                CloseServiceHandle(schService);
                return FALSE;
            }
        }
    }

	// Опит да се стартува сервиса.
    if (!StartService(
            schService,  // handle to service
            0,           // number of arguments
            NULL) )      // no arguments
    {
        MessageBox(hWnd, GetErrorText(GetLastError()), "FileHide", MB_ICONERROR);
        CloseServiceHandle(schService);
        return FALSE;
    }

	// Започва очакването...
	// Проверка на състоянието, докато сервиса не започне в очакване.
    if (!QueryServiceStatusEx(
            schService,                     // дръжка до сервиса
            SC_STATUS_PROCESS_INFO,         // информационно ниво
            (LPBYTE) &ssStatus,             // адреса на структура
            sizeof(SERVICE_STATUS_PROCESS), // големина на структура
            &dwBytesNeeded))             	// ако буфера е много малък
    {
        MessageBox(hWnd, GetErrorText(GetLastError()), "FileHide", MB_ICONERROR);
        CloseServiceHandle(schService);
        return FALSE;
    }

	// Спасяване на цъканията и първоначалния контролно-пропусквателен пункт.
    dwStartTickCount = GetTickCount();
    dwOldCheckPoint = ssStatus.dwCheckPoint;

    while (ssStatus.dwCurrentState == SERVICE_START_PENDING) {

        dwWaitTime = ssStatus.dwWaitHint / 10;

        if(dwWaitTime < 1000)
            dwWaitTime = 1000;
        else if (dwWaitTime > 10000)
            dwWaitTime = 10000;

        Sleep(dwWaitTime);

        // Отново проверка на състоянието.
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
			// Продължаване на изчакването и проверката.
            dwStartTickCount = GetTickCount();
            dwOldCheckPoint = ssStatus.dwCheckPoint;
        } else {
            if(GetTickCount()-dwStartTickCount > ssStatus.dwWaitHint) {
				// Без напредък в изчекащия намек.
                break;
            }
        }
    }

	// Определя дали сервиса е стартуван.
    if (ssStatus.dwCurrentState == SERVICE_RUNNING) {
        MessageBox(hWnd, "Сервисът е стартуван", "FileHide", MB_ICONEXCLAMATION);
		CloseServiceHandle(schService);
		return TRUE;
    } else {
		sprintf(str, "Сервисът не е стартуван.\n");
		sprintf(str, "Текущо състояние: %d\n", ssStatus.dwCurrentState);
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
    DWORD dwTimeout = 30000; // 30 секундна пауза
    DWORD dwWaitTime;

    schService = OpenService(schSCManager, szSvcName,
							SERVICE_STOP | SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS);
    if (schService == NULL) {
        MessageBox(hWnd, GetErrorText(GetLastError()), "FileHide", MB_ICONERROR);
        return FALSE;
    }

	// Проверява дали сервиса не е вече спрян.
    if (!QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp,
								sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded))
    {
        MessageBox(hWnd, GetErrorText(GetLastError()), "FileHide", MB_ICONERROR);
        CloseServiceHandle(schService);
		return FALSE;
    }

    if (ssp.dwCurrentState == SERVICE_STOPPED) {
        MessageBox(hWnd, "Сервисът е бил спрен", "FileHide", MB_ICONERROR);
        CloseServiceHandle(schService);
		return FALSE;
    }

	// Ако стопирането е в очакване, изчакай го.
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
			MessageBox(hWnd, "Сервисът е спрян", "FileHide", MB_ICONEXCLAMATION);
			CloseServiceHandle(schService);
			return TRUE;
        }

        if (GetTickCount() - dwStartTime > dwTimeout) {
			MessageBox(hWnd, "Времето за спиране на сервиса изтече", "FileHide", MB_ICONERROR);
			CloseServiceHandle(schService);
			return FALSE;
        }
    }

	// Ако сервиса работи, зависимите сервиси първо се спират.
    StopDependentServices(schSCManager, schService);

	// Изпраща стоп код към сервиса.
    if (!ControlService(schService, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&ssp)) {
		MessageBox(hWnd, GetErrorText(GetLastError()), "FileHide", MB_ICONERROR);
		CloseServiceHandle(schService);
        return FALSE;
    }

	// Изчака сервиса да спре.
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
			MessageBox(hWnd, "Времето за спиране на сервиса изтече", "FileHide", MB_ICONERROR);
			CloseServiceHandle(schService);
			return FALSE;
        }
    }

	MessageBox(hWnd, "Сервисът е спрян", "FileHide", MB_ICONEXCLAMATION);
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
    DWORD dwTimeout = 30000; // 30 секундна пауза

	// Пускане на буфер с нулева дължина, за да се получи желания размер на буфера.
    if (EnumDependentServices(hService, SERVICE_ACTIVE, lpDependencies, 0, &dwBytesNeeded, &dwCount))
		// Ако извикването на Enum-а е успешно, тогава няма зависими
		// сервиси, така че не се прави нищо.
        return TRUE;
	
	if (GetLastError() != ERROR_MORE_DATA)
		return FALSE; // Неочаквана грешка

	// Алокиране на буфер за зависимостите.
	lpDependencies = (LPENUM_SERVICE_STATUS)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwBytesNeeded);
	if (!lpDependencies)
		return FALSE;

	__try {
		// Избройват се зависимостите.
		if (!EnumDependentServices(hService, SERVICE_ACTIVE, lpDependencies,
									dwBytesNeeded, &dwBytesNeeded, &dwCount))
			return FALSE;

		for (i=0; i<dwCount; i++) {
			ess = *(lpDependencies + i);
			// Отваря се сервиса
			hDepService = OpenService(hSCManager, ess.lpServiceName, SERVICE_STOP|SERVICE_QUERY_STATUS);
			if (!hDepService)
			   return FALSE;

			__try {
				// Изпраща се стоп код.
				if (!ControlService(hDepService, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&ssp))
					return FALSE;

				// Изчаква се сервиса да спре.
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
				// Винаги се изпуска дръжката на сервиса.
				CloseServiceHandle(hDepService);
			}
		}
	} __finally {
		// Винаги се освобождава изброения буфер.
		HeapFree(GetProcessHeap(), 0, lpDependencies);
	}
	return TRUE;
}

BOOL IsServiceInstalled(SC_HANDLE schSCManager, char *szSvcName)
{
	SC_HANDLE schService;

	schService = OpenService(
        schSCManager,         // SCM база с данни
        szSvcName,            // име на сервиса
        SERVICE_ALL_ACCESS);  // пълен достъп
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
        schSCManager,         // SCM база с данни
        szSvcName,            // име на сервиса
        SERVICE_ALL_ACCESS);  // пълен достъп

    if (schService == NULL) {
        return FALSE;
    }

    if (!QueryServiceStatusEx(
            schService,                     // дръжка до сервиса
            SC_STATUS_PROCESS_INFO,         // информационно ниво
            (LPBYTE) &ssStatus,             // адреса на структура
            sizeof(SERVICE_STATUS_PROCESS), // големина на структура
            &dwBytesNeeded))              	// необходима големина, ако буфера е много малък
    {
        MessageBox(hWnd, GetErrorText(GetLastError()), "FileHide", MB_ICONERROR);
        CloseServiceHandle(schService);
        return FALSE;
    }
	// Проверява дали сервиса работи.
    if(ssStatus.dwCurrentState == SERVICE_RUNNING) {
        CloseServiceHandle(schService);
        return TRUE;
    }

	return FALSE;
}