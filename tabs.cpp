#include <windows.h>
#include <commctrl.h> // Tabs

char PropName[] = "TabCtrlCurWnd";

int AddTab(HWND TabWindow, HWND Window, char * Caption, int Index) {
	TC_ITEM TabData;
	RECT TabRect;
	// ��� ������� � -1, ����� �� ��������� �� ���������
	if(Index == -1)
		Index = SendMessage(TabWindow, TCM_GETITEMCOUNT, 0, 0);

	TabData.mask = TCIF_TEXT | TCIF_PARAM;
	TabData.pszText = Caption;
	TabData.cchTextMax = strlen(Caption) + 1;
	TabData.lParam = (LPARAM)Window;

	Index = SendMessage(TabWindow, TCM_INSERTITEM, Index, (LPARAM)&TabData);
	if(Index != -1) {
		// ���������� ������ �� ���������� � ����������� ������� ��������
		// �� �� �� ������ � ���������� �� �������
		GetWindowRect(TabWindow, &TabRect);
		MapWindowPoints(HWND_DESKTOP, GetParent(TabWindow), (POINT *)&TabRect, 2);
		SendMessage(TabWindow, TCM_ADJUSTRECT, FALSE, (LPARAM)&TabRect);
		TabRect.right  -= TabRect.left; // .right  == width
		TabRect.bottom -= TabRect.top;  // .bottom == heigth
		SetWindowPos(Window, HWND_BOTTOM, TabRect.left, TabRect.top, TabRect.right, TabRect.bottom, SWP_HIDEWINDOW);
		// ���������� �� �������� �� ������������ �� �����
		// � ���������� "TabCtrlCurWnd" �� ���������� �� �������
		// �� ������� �������� ��� �����������
		SetProp(TabWindow, PropName, (HANDLE)Window);
	}

	return Index;
}

int TabToFront(HWND TabWindow, int Index) {
	TC_ITEM TabData;
	// ��� ������� � -1, ����� �� ������� ������
	if(Index == -1)
		Index = SendMessage(TabWindow, TCM_GETCURSEL, 0, 0);

	TabData.mask = TCIF_PARAM;

	if(SendMessage(TabWindow, TCM_GETITEM, Index, (LPARAM)&TabData)) {
		// ������ �� ��������� �����������
		ShowWindow((HWND)GetProp(TabWindow, PropName), SW_HIDE);
		// ������� �� ������� �����������
		SetWindowPos((HWND)TabData.lParam, HWND_TOP, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
		// ������� �� ���������� �� �������� �� ������������ � ���������� �� ���������� �� �������
		SetProp(TabWindow, PropName, (HANDLE)TabData.lParam);
		SendMessage(TabWindow, TCM_SETCURSEL, Index, 0);
		return Index;
	}

	return -1;
}

BOOL RemoveTab(HWND TabWindow, int Index) {
	TC_ITEM TabData;
	int CurIndex, Count;

	TabData.mask = TCIF_PARAM;
	if(SendMessage(TabWindow, TCM_GETITEM, Index, (LPARAM)&TabData)) {
		CurIndex = SendMessage(TabWindow, TCM_GETCURSEL, 0, 0);
		if(SendMessage(TabWindow, TCM_DELETEITEM, Index, 0)) {
			Count = SendMessage(TabWindow, TCM_GETITEMCOUNT, 0, 0);
			if(!Count){
				ShowWindow((HWND)TabData.lParam, SW_HIDE);
				RemoveProp(TabWindow, PropName);
			} else if(Index == CurIndex){ // ��� �� ������� �������, ����� � ����� � �������
				if(Index == Count) Index--; // ��� � ��������� ������
				TabToFront(TabWindow, Index);
			}
			return TRUE;
		}
	}
	
	return FALSE;
}

BOOL TabCleanup(HWND TabWindow) {
	int Count, i;
	BOOL Result = TRUE;

	Count = SendMessage(TabWindow, TCM_GETITEMCOUNT, 0, 0);
	// ����� ������ �� ������� ���������
	for(i = 1; i <= Count; i++) {
		Result = RemoveTab(TabWindow, Count-i) && Result;
	}

	return Result;
}
