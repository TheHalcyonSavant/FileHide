#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>

char *GetErrorText(DWORD dwError)
{
	HLOCAL hlocal = NULL;
	HMODULE hDll;
	static char str[] = "Can't load netmsg.dll";
	DWORD strSize = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                   NULL, dwError, 0, (LPSTR)&hlocal, 0, NULL);
    if (strSize != NULL) {
        return (char*)LocalLock(hlocal);
    }
    hDll = LoadLibraryEx(TEXT("netmsg.dll"), NULL, DONT_RESOLVE_DLL_REFERENCES);
    if (hDll == INVALID_HANDLE_VALUE)
        return (char*)str;
    strSize = FormatMessageA(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM,
                             hDll, dwError, 0, (LPSTR)&hlocal, 0, NULL);
    if (strSize != NULL)
        return (char*)LocalLock(hlocal);
	strcpy(str,"Unknown error.");
	return (char*)str;
}