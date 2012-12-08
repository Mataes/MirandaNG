/*
WhenWasIt (birthday reminder) plugin for Miranda IM

Copyright � 2006 Cristian Libotean

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "commonheaders.h"
#include "services.h"

#define COMMENT_CHAR '#'

HANDLE hsCheckBirthdays;
HANDLE hsShowList;
HANDLE hsAddBirthday;
HANDLE hsRefreshUserDetails;
HANDLE hsImportBirthdays;
HANDLE hsExportBirthdays;

int bShouldCheckBirthdays = 0;
int bBirthdayFound = 0;

int InitServices()
{
	Log("%s", "Entering function " __FUNCTION__);

	commonData.foreground = db_get_dw(NULL, ModuleName, "Foreground", FOREGROUND_COLOR);
	commonData.background = db_get_dw(NULL, ModuleName, "Background", BACKGROUND_COLOR);
	commonData.checkInterval = db_get_w(NULL, ModuleName, "CheckInterval", CHECK_INTERVAL);
	commonData.daysInAdvance = db_get_w(NULL, ModuleName, "DaysInAdvance", DAYS_TO_NOTIFY);
	commonData.daysAfter = db_get_w(NULL, ModuleName, "DaysAfter", DAYS_TO_NOTIFY_AFTER);
	commonData.popupTimeout = db_get_w(NULL, ModuleName, "PopupTimeout", POPUP_TIMEOUT);
	commonData.popupTimeoutToday = db_get_w(NULL, ModuleName, "PopupTimeoutToday", commonData.popupTimeout);
	commonData.bUsePopups = db_get_b(NULL, ModuleName, "UsePopups", TRUE);
	commonData.bUseDialog = db_get_b(NULL, ModuleName, "UseDialog", TRUE);
	commonData.bIgnoreSubcontacts = db_get_b(NULL, ModuleName, "IgnoreSubcontacts", FALSE);
	commonData.cShowAgeMode = db_get_b(NULL, ModuleName, "ShowCurrentAge", FALSE);
	commonData.bNoBirthdaysPopup = db_get_b(NULL, ModuleName, "NoBirthdaysPopup", FALSE);
	commonData.bOpenInBackground = db_get_b(NULL, ModuleName, "OpenInBackground", FALSE);
	commonData.cSoundNearDays = db_get_b(NULL, ModuleName, "SoundNearDays", BIRTHDAY_NEAR_DEFAULT_DAYS);
	commonData.cDefaultModule = db_get_b(NULL, ModuleName, "DefaultModule", 0);
	commonData.lPopupClick = db_get_b(NULL, ModuleName, "PopupLeftClick", 0);
	commonData.rPopupClick = db_get_b(NULL, ModuleName, "PopupRightClick", 1);
	commonData.bOncePerDay = db_get_b(NULL, ModuleName, "OncePerDay", 0);
	commonData.cDlgTimeout = db_get_w(NULL, ModuleName, "DlgTimeout", POPUP_TIMEOUT);
	commonData.notifyFor = db_get_b(NULL, ModuleName, "NotifyFor", 3);

	hsCheckBirthdays = CreateServiceFunction(MS_WWI_CHECK_BIRTHDAYS, CheckBirthdaysService);
	hsShowList = CreateServiceFunction(MS_WWI_LIST_SHOW, ShowListService);
	hsAddBirthday = CreateServiceFunction(MS_WWI_ADD_BIRTHDAY, AddBirthdayService);
	hsRefreshUserDetails = CreateServiceFunction(MS_WWI_REFRESH_USERDETAILS, RefreshUserDetailsService);
	hsImportBirthdays = CreateServiceFunction(MS_WWI_IMPORT_BIRTHDAYS, ImportBirthdaysService);
	hsExportBirthdays = CreateServiceFunction(MS_WWI_EXPORT_BIRTHDAYS, ExportBirthdaysService);

	Log("%s", "Leaving function " __FUNCTION__);
	return 0;
}

int DestroyServices()
{
	Log("%s", "Entering function " __FUNCTION__);

	DestroyServiceFunction(hsCheckBirthdays);
	DestroyServiceFunction(hsShowList);
	DestroyServiceFunction(hsAddBirthday);
	DestroyServiceFunction(hsRefreshUserDetails);
	DestroyServiceFunction(hsImportBirthdays);
	DestroyServiceFunction(hsExportBirthdays);

	Log("%s", "Leaving function " __FUNCTION__);
	return 0;
}

/*
returns -1 if notify is not necesarry
returns daysToBirthday if it should notify
*/
int NotifyContactBirthday(HANDLE hContact, time_t now, int daysInAdvance)
{
	int year, month, day;
	GetContactDOB(hContact, year, month, day);
	int daysToBirthday = DaysToBirthday(now, year, month, day);
	if (daysInAdvance >= daysToBirthday)
		return daysToBirthday;

	return -1;
}

// returns -1 if notify is not necessary
// returns daysAfterBirthday if it should notify
int NotifyMissedContactBirthday(HANDLE hContact, time_t now, int daysAfter)
{
	if (daysAfter > 0)
	{
		int year, month, day;
		GetContactDOB(hContact, year, month, day);
		int daysAfterBirthday = DaysAfterBirthday(now, year, month, day);
		if ((daysAfterBirthday >= 0) && (daysAfter >= daysAfterBirthday))
		{
			return daysAfterBirthday;
		}
	}

	return -1;
}

//the timer functions call this service with lParam = 1
//lParam = 0 - force check, lParam - 1 do not force it.

INT_PTR CheckBirthdaysService(WPARAM wParam, LPARAM lParam)
{
	bBirthdayFound = 0; //no birthdays have been found in the given interval

	SYSTEMTIME today;
	GetLocalTime(&today);

	DWORD lastChecked = db_get_dw(NULL, ModuleName, "LastChecked", 0); //get last checked date
	int lcDay = LOBYTE(LOWORD(lastChecked));
	int lcMonth = HIBYTE(LOWORD(lastChecked));
	int lcYear = HIWORD(lastChecked);

	int daysAfter = DaysAfterBirthday(Today(), lcYear, lcMonth, lcDay); //get difference between last checked date and today
	int savedDaysAfter = 0;

	savedDaysAfter = commonData.daysAfter; //save value

	if ((daysAfter > commonData.daysAfter) && (commonData.daysAfter > 0))//check for passed birthdays
		commonData.daysAfter = daysAfter; //bigger values of the two

	if ((lParam) && (commonData.bOncePerDay)) //if force check then we don't take OncePerDay into account
		if (lcDay == today.wDay && lcMonth == today.wMonth && lcYear == today.wYear)
			return 0; //already checked today

	bShouldCheckBirthdays = 1;
	RefreshAllContactListIcons();
	if ((!bBirthdayFound) && (commonData.bNoBirthdaysPopup))
		PopupNotifyNoBirthdays();

	bShouldCheckBirthdays = 0;

	commonData.daysAfter = savedDaysAfter; //restore previous value

	if (lParam) //if not forced - i.e. timer check
		db_set_dw(NULL, ModuleName, "LastChecked", MAKELONG(MAKEWORD(today.wDay, today.wMonth), today.wYear)); //write the value in DB so we don't check again today

	return 0;
}

INT_PTR ShowListService(WPARAM wParam, LPARAM lParam)
{
	if ( !hBirthdaysDlg)
		hBirthdaysDlg = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_BIRTHDAYS), NULL, DlgProcBirthdays);

	ShowWindow(hBirthdaysDlg, SW_SHOW);
	return 0;
}

INT_PTR AddBirthdayService(WPARAM wParam, LPARAM lParam)
{
	HWND hWnd = WindowList_Find(hAddBirthdayWndsList, (HANDLE) wParam);
	if ( !hWnd) {
		hWnd = CreateDialogParam(hInstance, MAKEINTRESOURCE(IDD_ADD_BIRTHDAY), NULL, DlgProcAddBirthday, wParam);
		WindowList_Add(hAddBirthdayWndsList, hWnd, (HANDLE) wParam);
	}
	return ShowWindow(hWnd, SW_SHOW);
}

void ShowPopupMessage(TCHAR *title, TCHAR *message, HANDLE icon)
{
	POPUPDATAT pd = {0};
	pd.lchIcon = Skin_GetIconByHandle(icon);
	_tcscpy(pd.lptzContactName, title);
	_tcscpy(pd.lptzText, message);
	pd.colorText = commonData.foreground;
	pd.colorBack = commonData.background;
	PUAddPopUpT(&pd);

	Skin_ReleaseIcon(pd.lchIcon);
}

DWORD WINAPI RefreshUserDetailsWorkerThread(LPVOID param)
{
	ShowPopupMessage(TranslateT("WhenWasIt"), TranslateT("Starting to refresh user details"), hRefreshUserDetails);
	int delay = db_get_w(NULL, ModuleName, "UpdateDelay", REFRESH_DETAILS_DELAY);
	int res;

	HANDLE hContact = db_find_first();
	while (hContact != NULL) {
		res = CallContactService(hContact, PSS_GETINFO, 0, 0);
		hContact = db_find_next(hContact);
		if (hContact)
			Sleep(delay); //sleep for a few seconds between requests
	}
	ShowPopupMessage(TranslateT("WhenWasIt"), TranslateT("Done refreshing user details"), hRefreshUserDetails);
	return 0;
}

INT_PTR RefreshUserDetailsService(WPARAM wParam, LPARAM lParam)
{
	DWORD threadID;
	HANDLE result = CreateThread(NULL, 0, RefreshUserDetailsWorkerThread, NULL, 0, &threadID);
	if ( !result) {
		TCHAR buffer[1024];
		_stprintf(buffer, _T("Could not create worker thread. Error#%d - threadID %d"), GetLastError(), threadID);
		MessageBox(0, buffer, _T("Error"), MB_OK | MB_ICONERROR);
	}

	if ((result != NULL) && (result != INVALID_HANDLE_VALUE))
		CloseHandle(result);

	return 0;
}

INT_PTR ImportBirthdaysService(WPARAM wParam, LPARAM lParam)
{
	TCHAR fileName[1024] = {0};
	OPENFILENAME of = {0};
	of.lStructSize = sizeof(OPENFILENAME);
	//of.hInstance = hInstance;
	TCHAR filter[MAX_PATH];
	mir_sntprintf(filter, SIZEOF(filter), _T("%s (*") _T(BIRTHDAY_EXTENSION) _T(")%c*") _T(BIRTHDAY_EXTENSION) _T("%c"), TranslateT("Birthdays files"), 0, 0);
	of.lpstrFilter = filter;
	of.lpstrFile = fileName;
	of.nMaxFile = 1024;
	of.lpstrTitle = TranslateT("Please select a file to import birthdays from ...");
	of.Flags = OFN_FILEMUSTEXIST;

	if ( GetOpenFileName(&of)) {
		TCHAR buffer[2048];
		mir_sntprintf(buffer, SIZEOF(buffer), TranslateT("Importing birthdays from file: %s"), fileName);
		ShowPopupMessage(TranslateT("WhenWasIt"), buffer, hImportBirthdays);
		DoImport(fileName);
		ShowPopupMessage(TranslateT("WhenWasIt"), TranslateT("Done importing birthdays"), hImportBirthdays);
	}

	return 0;
}

INT_PTR ExportBirthdaysService(WPARAM wParam, LPARAM lParam)
{
	TCHAR fileName[1024]= {0};
	OPENFILENAME of = {0};
	of.lStructSize = sizeof(OPENFILENAME);
	//of.hInstance = hInstance;
	TCHAR filter[MAX_PATH];
	mir_sntprintf(filter, SIZEOF(filter), _T("%s (*") _T(BIRTHDAY_EXTENSION) _T(")%c*") _T(BIRTHDAY_EXTENSION) _T("%c%s (*.*)%c*.*%c"), TranslateT("Birthdays files"), 0, 0, TranslateT("All Files"), 0, 0);
	of.lpstrFilter = filter;
	of.lpstrFile = fileName;
	of.nMaxFile = 1024;
	of.lpstrTitle = TranslateT("Please select a file to export birthdays to ...");

	if ( GetSaveFileName(&of)) {
		TCHAR buffer[2048];
		TCHAR *fn = _tcsrchr(fileName, _T('\\')) + 1;
		if ( !_tcschr(fn, _T('.')))
			_tcscat(fileName, _T(BIRTHDAY_EXTENSION));

		_stprintf(buffer, TranslateT("Exporting birthdays to file: %s"), fileName);
		ShowPopupMessage(TranslateT("WhenWasIt"), buffer, hExportBirthdays);
		DoExport(fileName);
		ShowPopupMessage(TranslateT("WhenWasIt"), TranslateT("Done exporting birthdays"), hExportBirthdays);
	}

	return 0;
}

int DoImport(TCHAR *fileName)
{
	FILE *fin = _tfopen(fileName, _T("rt"));
	if ( !fin) {
		MessageBox(0, _T("Could not open file to import birthdays"), _T("Error"), MB_OK | MB_ICONERROR);
		return 1;
	}

	int mode = commonData.cDefaultModule;

	while (!feof(fin)) {
		TCHAR buffer[4096];
		_fgetts(buffer, SIZEOF(buffer), fin);
		if (buffer[0] == _T(COMMENT_CHAR))
			continue;

		TCHAR *delAccount = _tcsstr(buffer, _T(" : "));
		if (delAccount) {
			int tmp = delAccount[0];
			delAccount[0] = _T('\0');
			TCHAR *delProto = _tcsrchr(buffer, _T('@'));
			if (delProto) {
				int tmp2 = delProto[0];
				delProto[0] = _T('\0');

				TCHAR *szHandle = buffer;
				TCHAR *szProto = delProto + 1;
				HANDLE hContact = GetContactFromID(szHandle, szProto);
				if (hContact) {
					delProto[0] = tmp;
					delAccount[0] = tmp;

					int year, month, day;
					_stscanf(delAccount, _T(" : %02d/%02d/%04d"), &day, &month, &year);
					SaveBirthday(hContact, year, month, day, mode);
				}
				else {
					TCHAR tmp[2048];
					_stprintf(tmp, TranslateT(NOTFOUND_FORMAT), szHandle, szProto);
					ShowPopupMessage(TranslateT("Warning"), tmp, hImportBirthdays);
				}
			}
		}
	}

	fclose(fin);
	return 0;
}

int DoExport(TCHAR *fileName)
{
	FILE *fout = _tfopen(fileName, _T("wt"));
	if ( !fout) {
		MessageBox(0, _T("Could not open file to export birthdays"), _T("Error"), MB_OK | MB_ICONERROR);
		return 1;
	}
	_ftprintf(fout, _T("%c%s"), _T(COMMENT_CHAR), _T("Please do not edit this file by hand. Use the export function of WhenWasIt plugin.\n"));
	_ftprintf(fout, _T("%c%s"), _T(COMMENT_CHAR), _T("Warning! Please do not mix Unicode and Ansi exported birthday files. You should use the same version (Ansi/Unicode) of WhenWasIt that was used to export the info.\n"));
	_ftprintf(fout, _T("%c%s"), _T(COMMENT_CHAR), _T("This file was exported with a Unicode version of WhenWasIt. Please only use a Unicode version of the plugin to import the birthdays.\n"));

	HANDLE hContact = db_find_first();
	while (hContact) {
		int year, month, day;
		GetContactDOB(hContact, year, month, day);
		if ( IsDOBValid(year, month, day)) {
			char *szProto = GetContactProto(hContact);
			TCHAR *szHandle = GetContactID(hContact, szProto);

			if ((szHandle) && (strlen(szProto) > 0))
				_ftprintf(fout, _T(BIRTHDAYS_EXPORT_FORMAT), szHandle, szProto, day, month, year);

			if (szHandle)
				free(szHandle);
		}

		hContact = db_find_next(hContact);
	}

	fclose(fout);
	return 0;
}