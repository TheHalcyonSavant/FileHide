#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")
#include "resource.h"
#include "tabs.h"
#include "services.h"
#include "ErrorParser.h"

// ��������� ������
#define IOCTL_HIDE_FILE \
		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_OUT_DIRECT, FILE_READ_DATA | FILE_WRITE_DATA)
#define	IOCTL_CLEAN_POOL \
		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_OUT_DIRECT, FILE_READ_DATA | FILE_WRITE_DATA)

BOOL CALLBACK MainHandler(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabHandler1(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabHandler2(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY SubClass_ListView_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// �������� ����������
HINSTANCE hInst;
HWND hList = NULL;			// ������ �� ������� � ���������
HWND hEdit = NULL;			// ������ �� ������������ ����
char DriverName[50] = {0};	// ��� �� ��������

// ������ �������
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	INITCOMMONCONTROLSEX InitCtrls;

	InitCtrls.dwICC = ICC_LISTVIEW_CLASSES;
	InitCtrls.dwSize = sizeof(INITCOMMONCONTROLSEX);
	if (InitCommonControlsEx(&InitCtrls)) {
		hInst = hInstance;
		DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, (DLGPROC)MainHandler,0);
	}
	return EXIT_SUCCESS;
}

// ������ ���������
BOOL CALLBACK MainHandler(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static HWND Window1, Window2;
	static HWND TabWindow;

	switch(Message)
	{
		case WM_INITDIALOG:
		{
			// ��������� ��� ������� ���� ���� �� ������� ������
			Window1 = CreateDialog(hInst, MAKEINTRESOURCE(IID_TAB1), Window, &TabHandler1);
			Window2 = CreateDialog(hInst, MAKEINTRESOURCE(IID_TAB2), Window, &TabHandler2);
			// �������������� �� TabWindow ������ ��� ���������� ������ - TAB_MAIN
			TabWindow = GetDlgItem(Window, TAB_MAIN);
			// ��������� �� ������ ������ (0 = ����)
			AddTab(TabWindow, Window1, "����������� ������", 0);
			// ��������� �� ������ ������
			AddTab(TabWindow, Window2, "�������", 1);
			// ��������� �� ������ ������
			TabToFront(TabWindow, 0);
		}
		break;

		case WM_NOTIFY:
			switch(((NMHDR *)lParam)->code)
			{
			case TCN_SELCHANGE:		// ��� ������� �� ���������...
			{
				char buf[50];
				// ��� ������� ������� � TabWindow
				if (((LPNMHDR)lParam)->hwndFrom == TabWindow) {
					// ���� � ���������� ������ ������
					if (SendMessage(((LPNMHDR)lParam)->hwndFrom,(UINT)TCM_GETCURFOCUS,0,0) == 1) {
						GetDlgItemText(Window1, IDC_STATIC_STATUS, buf, 50);
						// ���� �������� � ���������� � ������
						if (strcmp("Installed And Running", buf) != 0) {
							MessageBox(Window, "�������� ������ �� � ���������", "FileHide", 0);
							TabToFront(TabWindow, 0);
							return FALSE;
						}
					}
					// ��������� �� ��������� ������ (��� � �������� -> ����)
					TabToFront(TabWindow, -1);
				}
			}
			break;
			
			default: return FALSE;
			}
			break;

		case WM_CLOSE:
		{
			// ������� �� WM_CLOSE ��� ������ ������ �� ��������� �
			// ��������� �� ��������, ����� �� �� ������� ����������
			SendMessage(Window1, WM_CLOSE, 0, 0);
			DestroyWindow(Window2);
			DestroyWindow(Window1);
			TabCleanup(TabWindow);
			EndDialog(Window, 0);
		}
		break;

		default:
			return FALSE;
	}
	return TRUE;
}

// ��������� �� ������ ������ (������ ������)
BOOL CALLBACK TabHandler1(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static HWND hComboBox;
	const char *aServiceTypes[] = {"SERVICE_AUTO_START", "SERVICE_BOOT_START",
									"SERVICE_DEMAND_START", "SERVICE_DISABLED",
									"SERVICE_SYSTEM_START"};
	static SC_HANDLE hSCManager;
	char szFile[260];
	
	switch(Message)
	{
		case WM_INITDIALOG:
		{
			int i;
			
			// �������������� �� ����� ���� �� ���������� ������
			hComboBox = GetDlgItem(Window, IDC_COMBO_TYPE);
			for(i = 0; i < 5; i++) {
				SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)aServiceTypes[i]);
			}
			// ����������� �� ������ ������� �� �����-�������
			SendMessage(hComboBox, CB_SETCURSEL, 2, 0);
			// ��������� ������ ��� SCM ������
			hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
			if (hSCManager == INVALID_HANDLE_VALUE) {
				MessageBox(Window, GetErrorText(GetLastError()), "FileHide", MB_ICONERROR);
				break;
			}
		}
		break;

		case WM_COMMAND:
		{
			switch(LOWORD(wParam)) {
				case IDC_DRIVER_NAME_SELECT:
				{
					OPENFILENAME openFileDialog = {0};
					char szExtendedFileName[260];
					
					openFileDialog.lStructSize = sizeof(openFileDialog);
					openFileDialog.hwndOwner = GetParent(Window);
					openFileDialog.lpstrFilter = "�������(*.sys)\0*.SYS\0";
					openFileDialog.nFilterIndex = 0;
					openFileDialog.lpstrFileTitle = NULL;
					openFileDialog.nMaxFileTitle = 0;
					openFileDialog.lpstrInitialDir = NULL;
					openFileDialog.lpstrFile = szFile;
					openFileDialog.lpstrFile[0] = '\0';
					openFileDialog.nMaxFile = sizeof(szFile);
					openFileDialog.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
					// �������� �� ������ �� ����� �� *.sys ����
					if (GetOpenFileName(&openFileDialog)) {
						// ��������� �� ������� � ��������� �������
						// �.�. � ���������� \\?\, ������ �������� ���� �� ������ �
						// ���� �������� ��� �� ������ � ����� �������� (������ � D:\)
						sprintf(szExtendedFileName,"\\\\?\\%s",szFile);
						SetDlgItemText(Window, IDC_DRIVER_PATHNAME, szExtendedFileName);
						// ��������� �� ����� �� �������� �� ������ �������, ��� ������������ (.sys)
						_splitpath(szFile, NULL, NULL, DriverName, NULL);
						if (IsServiceInstalled(hSCManager, DriverName))
							SetDlgItemText(Window, IDC_STATIC_STATUS, "Installed");
						if (IsServiceRunning(Window, hSCManager, DriverName))
							SetDlgItemText(Window, IDC_STATIC_STATUS, "Installed And Running");
					}
				}
				break;
				
				case IDC_BUT_INST:
				{
					DWORD startType = 0;
					
					GetDlgItemText(Window, IDC_DRIVER_PATHNAME, szFile, 260);
					// ��������� �� ����� �� �������� �� ������ �������, ��� ������������ (.sys)
					_splitpath(szFile, NULL, NULL, DriverName, NULL);
					if (!strlen(DriverName)) {
						MessageBox(Window, "������ �� �������� � ������", "FileHide", MB_ICONERROR);
						break;
					}
					// ������� �� startType � ��������� ��������.
					// ���������� �� SERVICE_DEMAND_START
					switch (SendMessage(hComboBox, CB_GETCURSEL, 0, 0)) {
						case 0: startType = SERVICE_AUTO_START; break;
						case 1: startType = SERVICE_BOOT_START; break;
						case 2: startType = SERVICE_DEMAND_START; break;
						case 3: startType = SERVICE_DISABLED; break;
						case 4: startType = SERVICE_SYSTEM_START; break;
					}
					// InstallServiceEx �� ������ � services.h
					if (InstallServiceEx(Window, hSCManager, DriverName, startType, szFile))
						SetDlgItemText(Window, IDC_STATIC_STATUS, "Installed");
				}
				break;
				
				case IDC_BUT_UNINST:
				{
					GetDlgItemText(Window, IDC_DRIVER_PATHNAME, szFile, 260);
					_splitpath(szFile, NULL, NULL, DriverName, NULL);
					if (!strlen(DriverName)) {
						MessageBox(Window, "������ �� �������� � ������", "FileHide", MB_ICONERROR);
						break;
					}
					if (DeleteServiceEx(Window, hSCManager, DriverName))
						SetDlgItemText(Window, IDC_STATIC_STATUS, "Uninstalled");
					ZeroMemory(DriverName, sizeof(DriverName));
				}
				break;
				
				case IDC_BUT_RUN:
				{
					GetDlgItemText(Window, IDC_DRIVER_PATHNAME, szFile, 260);
					_splitpath(szFile, NULL, NULL, DriverName, NULL);
					if (!strlen(DriverName)) {
						MessageBox(Window, "������ �� �������� � ������", "FileHide", MB_ICONERROR);
						break;
					}
					if (StartServiceEx(Window, hSCManager, DriverName))
						SetDlgItemText(Window, IDC_STATIC_STATUS, "Installed And Running");
				}
				break;
				
				case IDC_BUT_STOP:
				{
					GetDlgItemText(Window, IDC_DRIVER_PATHNAME, szFile, 260);
					_splitpath(szFile, NULL, NULL, DriverName, NULL);
					if (!strlen(DriverName)) {
						MessageBox(Window, "������ �� �������� � ������", "FileHide", MB_ICONERROR);
						break;
					}
					if (StopServiceEx(Window, hSCManager, DriverName))
						SetDlgItemText(Window, IDC_STATIC_STATUS, "Installed");
					ZeroMemory(DriverName, sizeof(DriverName));
				}
				break;
			}
		}
		break;
		
		case WM_CLOSE:
		{
			CloseServiceHandle(hSCManager);
		}
		break;

		default:
			return FALSE;
	}
	return TRUE;
}

BOOL CALLBACK TabHandler2(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static int Index = -1;
	static int lastEntry = 0;
	static int flag = 0;
	
	switch(Message)
	{
		case WM_LBUTTONDOWN:
			// ��� ����������� ������ ����� ������� � �������,
			// �������� �� ������������� �� ��������� ��� � ����� �� ������ �����
			if (hEdit != NULL)
				DestroyWindow(hEdit);
			break;
		case WM_INITDIALOG:
		{
			LVCOLUMN lvCol = {0};
			LOGFONT logFont;
			HFONT hFont;
			// ������������� �� ������� � �������,
			// �������� �� ������� ���� � �������� �� ��������
			hList = GetDlgItem(Window, IDC_LISTVIEW);
			memset(&logFont, 0, sizeof(LOGFONT));
			logFont.lfHeight = 15;
			strcpy(logFont.lfFaceName, "Courier New");
			hFont = CreateFontIndirect(&logFont);
			SendDlgItemMessage(Window, IDC_LISTVIEW, WM_SETFONT, (WPARAM)hFont, TRUE);
			SendMessage(hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);
			
			lvCol.mask = LVCF_TEXT | LVCF_WIDTH;
			lvCol.pszText = "";
			lvCol.cx = 280;
			SendMessage(hList, LVM_INSERTCOLUMN, 0, (LPARAM)&lvCol);
		}
		break;

		case WM_NOTIFY:			// ��������� �� ����������� �� �������
			switch(LOWORD(wParam)) {
				case IDC_LISTVIEW:
				{
					if (((LPNMHDR)lParam)->code == NM_CLICK) {		// ��� ���� ������ � ��������
						Index = SendMessage(hList, LVM_GETNEXTITEM, -1, LVIS_FOCUSED);
						if (Index == -1) {
							flag = 0;
							break;
						}
						flag = 1;
						break;
					} else if (((LPNMHDR)lParam)->code == NM_DBLCLK) {		// ��� ���� ��� ���� � ��������
						if (hEdit != NULL)
							DestroyWindow(hEdit);
						// ��� ����������� ������ ��� ���� ����� ���������� ������� ���
						// �� "���������" ��������:
						{
							RECT r;
							WNDPROC wpOld;
							LVITEM lvItem = {0};
							char text[50] = "";
							
							lvItem.mask = LVIF_TEXT;
							lvItem.cchTextMax = 50;
							lvItem.pszText = text;
							SendMessage(hList, LVM_GETITEMTEXT, ((NMLISTVIEW*)lParam)->iItem, (LPARAM)&lvItem);
							// ����� �� ������� ������������ �� ��������� �� ������������� ����
							ListView_GetSubItemRect(hList, ((NMLISTVIEW*)lParam)->iItem, 0, LVIR_LABEL, &r);
							// ���� ���� �� ������� ����������� ���� ����� �������, ����� � �����������
							hEdit = CreateWindowEx(0, "EDIT", text, WS_CHILD|WS_VISIBLE|WS_BORDER|ES_LEFT|ES_MULTILINE,
													r.left, r.top, r.right-r.left, r.bottom-r.top,
													((NMLISTVIEW*)lParam)->hdr.hwndFrom, NULL, hInst, 0);
							SendMessage(hEdit, EM_LIMITTEXT, 50, 0);	// �� ������ �� 50 �����
							SendMessage(hEdit, EM_SETSEL, 0, 50);		// ������ �� ���������
							SetFocus(hEdit);							// ������������� ���� �������� ����� 
							// ������ �� ������������� ��������� �� ������������� ���� � SubClass_ListView_WndProc
							wpOld = (WNDPROC)SetWindowLong(hEdit, GWL_WNDPROC, (LONG)SubClass_ListView_WndProc);
							// �������� �� ����� ����� �������� ...
							SetProp(hEdit, "WP_OLD", (HANDLE)wpOld);
							SetProp(hEdit, "ITEM", (HANDLE)((NMLISTVIEW*)lParam)->iItem);
						}
						break;
					} else if (((LPNMHDR)lParam)->code == NM_RCLICK) {		// ����� ���� � ��������
						// ������� �� ������ � ����� ������ "������", "������" � "������ ������"
						HMENU hMenu = LoadMenu(NULL, MAKEINTRESOURCE(IDR_MENU));
						HMENU hPopupMenu = GetSubMenu(hMenu, 0);
						POINT pt;
						SetMenuDefaultItem(hPopupMenu, (UINT)-1, TRUE);
						GetCursorPos(&pt);
						SetForegroundWindow(Window);
						TrackPopupMenu(hPopupMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, Window, NULL);
						SetForegroundWindow(Window);
						// ���������� �� ��������
						DestroyMenu(hPopupMenu);
						DestroyMenu(hMenu);
						break;
					}
				}
				break;
			}
			break;
			
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case ID_ADD:				// ������� �� ���� ��� � ��������
				{
					char text[50] = "";
					LVITEM lvItem = {0};	// ListView Item struct

					lvItem.mask = LVIF_TEXT;     // Text Style
					lvItem.cchTextMax = 50;
					lvItem.iItem = SendMessage(hList, LVM_GETITEMCOUNT, 0, 0);
					SendMessage(hList,LVM_INSERTITEM,0,(LPARAM)&lvItem);
					sprintf(text,"FILENAMETOHIDE%d.EXE", lastEntry);
					lvItem.pszText = text;
					SendMessage(hList,LVM_SETITEM,0,(LPARAM)&lvItem);
					lastEntry++;
					break;
				}
				break;
				
				case ID_REMOVE:				// ��������� �� ��� �� ��������
					if (flag)
					   SendMessage(hList, LVM_DELETEITEM, Index, 0);
					flag = 0;
					Index = -1;
					break;
					
				case ID_REMOVEALL:			// ��������� �� ������ ����� �� ��������
					SendMessage(hList, LVM_DELETEALLITEMS, 0, 0);
					lastEntry = 0;
					flag = 0;
					Index = -1;
					break;

				case IDC_BUT_HIDE:			// ������ �� �������� �� ������ ����� �� ��������
				{
					HANDLE hFile;
					int i;
					char drvName[50];
					char fileName[50] = "";
					char driverText[50] = "";
					DWORD dwReturn;
					LVITEM LvItem = {0};
					// �������� ���� �������� � ������
					int count = SendMessage(hList, LVM_GETITEMCOUNT, 0, 0);
					if (!count) {
						MessageBox(Window, "����� ������ ������� ����� (� ����� ����).", "FileHide", 0);
						break;
					}
					// �������� �� ���������� ����� ����� �� ��������
					// ���� �������� � ������������ ��� �� ��������
					sprintf(drvName, "\\\\.\\%s", DriverName);
					// ��������� �� ������ ��� �������� �� ������ � �����
					hFile = CreateFile(drvName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
					if (hFile == INVALID_HANDLE_VALUE) {
						MessageBox(Window, GetErrorText(GetLastError()), "FileHide", MB_ICONERROR);
					} else {
						// ��� � ������ �� ���, ��������� ��������� ��� IOCTL_CLEAN_POOL
						// � ����� "clear pool", �� �� �� �������� ���������� ������ �������
						DeviceIoControl(hFile, IOCTL_CLEAN_POOL, "clear pool", sizeof("clear pool"),
										driverText, sizeof(driverText), &dwReturn, NULL);
						LvItem.mask = LVIF_TEXT;     // Text Style
						LvItem.cchTextMax = 50;
						LvItem.pszText = fileName;
						// ������� �� ������� � �� ����� �������
						// �� ������� IOCTL_HIDE_FILE ��������� ��� ��� ��������
						for (i=0; i<count; i++) {
							LvItem.iItem = i;
							SendMessage(hList, LVM_GETITEMTEXT, i, (LPARAM)&LvItem);
							ZeroMemory(driverText, sizeof(driverText));
							DeviceIoControl(hFile, IOCTL_HIDE_FILE, fileName, sizeof(fileName),
											driverText, sizeof(driverText), &dwReturn, NULL);
							MessageBox(Window, driverText, DriverName, 0);
							//ZeroMemory(fileName, sizeof(fileName));
						}
						CloseHandle(hFile);
					}
				}
				break;
			}
			break;
			
		default:
			return FALSE;
	}
	return TRUE;
}

// ��������� ���������
LRESULT APIENTRY SubClass_ListView_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg) {
		case WM_KEYDOWN:
			if (LOWORD(wParam) == VK_RETURN)
			{
				// ������ �NTER �� ������� ������ �� �� ������ ��������� ����� � ��������
				LV_ITEM LvItem;
				char text[50]="";

				LvItem.iItem = (UINT)GetProp(hEdit, "ITEM");				// ��������� �� �����
				LvItem.pszText = text;
				// ����� ����� �� ��������� �� ������� � LvItem
				GetWindowText(hEdit, text, sizeof(text));
				// ����������� �� LvItem � �������� ����� ���� LVM_SETITEMTEXT �����������
				SendMessage(hList, LVM_SETITEMTEXT, (WPARAM)GetProp(hEdit, "ITEM"), (LPARAM)&LvItem);
				// ����������� �� ���������
				DestroyWindow(hEdit);
			} else if (LOWORD(wParam) == VK_ESCAPE)				// ��� � ��������� ESCAPE,
				DestroyWindow(hEdit);							// �������� �� ����������
			break;
		case WM_CHAR:
			// �������� ����� �� �� ���������
			if (LOWORD(wParam)==VK_SPACE || LOWORD(wParam)==VK_TAB)
				return 0;
			// ���������� �� ������� ����� � ������
			else if (LOWORD(wParam)>=97 && LOWORD(wParam)<=122)
				wParam -= 32;
			break;
		case WM_DESTROY:
			RemoveProp(hEdit, "WP_OLD");
			RemoveProp(hEdit, "ITEM");
			// ���������� �� ���������� ���������
			SetWindowLong(hEdit, GWL_WNDPROC, (LONG)(WNDPROC)GetProp(hEdit, "WP_OLD"));
			hEdit = NULL;
			break;
	}
	// ������ �������� ������
	return CallWindowProc((WNDPROC)GetProp(hEdit, "WP_OLD"), hwnd, uMsg, wParam, lParam);
}

// ����

