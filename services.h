
#ifndef SERVICES_H
#define SERVICES_H

BOOL InstallServiceEx(HWND hWnd, SC_HANDLE schSCManager, char *scName, DWORD scStartType, char *szPath);
BOOL DeleteServiceEx(HWND hWnd, SC_HANDLE schSCManager, char *szSvcName);
BOOL StartServiceEx(HWND hWnd, SC_HANDLE schSCManager, char *szSvcName);
BOOL StopServiceEx(HWND hWnd, SC_HANDLE schSCManager, char *szSvcName);
BOOL IsServiceInstalled(SC_HANDLE schSCManager, char *szSvcName);
BOOL IsServiceRunning(HWND hWnd, SC_HANDLE schSCManager, char *szSvcName);

#endif