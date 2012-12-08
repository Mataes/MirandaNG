/*
Copyright (C) 2010 Mataes

This is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this file; see the file license.txt.  If
not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.
*/

#include "common.h"

PlugOptions opts;
WNDPROC g_pOldProc;

LRESULT CALLBACK MyEditProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_KEYDOWN:
		SendMessage(GetParent(GetParent(hwnd)), PSM_CHANGED, 0, 0);
		break;
	}
	return CallWindowProc (g_pOldProc, hwnd, message, wParam, lParam);
}

INT_PTR CALLBACK UpdateNotifyOptsProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	DBVARIANT dbv;

	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		CheckDlgButton(hwndDlg, IDC_UPDATEONSTARTUP, opts.bUpdateOnStartup);
		CheckDlgButton(hwndDlg, IDC_ONLYONCEADAY, opts.bOnlyOnceADay);
		EnableWindow(GetDlgItem(hwndDlg, IDC_ONLYONCEADAY), opts.bUpdateOnStartup);
		CheckDlgButton(hwndDlg, IDC_UPDATEONPERIOD, opts.bUpdateOnPeriod);
		EnableWindow(GetDlgItem(hwndDlg, IDC_PERIOD), opts.bUpdateOnPeriod);
		EnableWindow(GetDlgItem(hwndDlg, IDC_PERIODSPIN), opts.bUpdateOnPeriod);
		EnableWindow(GetDlgItem(hwndDlg, IDC_PERIODMEASURE), opts.bUpdateOnPeriod);

		SendDlgItemMessage(hwndDlg, IDC_PERIODSPIN, UDM_SETRANGE, 0, MAKELONG(99, 1));
		SendDlgItemMessage(hwndDlg, IDC_PERIODSPIN, UDM_SETPOS, 0, (LPARAM)opts.Period);

		Edit_LimitText(GetDlgItem(hwndDlg, IDC_PERIOD), 2);
		g_pOldProc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_PERIOD), GWLP_WNDPROC, (LPARAM)MyEditProc);

		ComboBox_InsertString(GetDlgItem(hwndDlg, IDC_PERIODMEASURE), 0, TranslateT("hours"));
		ComboBox_InsertString(GetDlgItem(hwndDlg, IDC_PERIODMEASURE), 1, TranslateT("days"));
		ComboBox_SetCurSel(GetDlgItem(hwndDlg, IDC_PERIODMEASURE), opts.bPeriodMeasure);

		CheckDlgButton(hwndDlg, IDC_UPDATEICONS, opts.bUpdateIcons);

		EnableWindow(GetDlgItem(hwndDlg, IDC_CUSTOMURL), FALSE);
		if ( DBGetContactSettingString(NULL, MODNAME, "UpdateURL", &dbv)) {
			SetDlgItemText(hwndDlg, IDC_CUSTOMURL, _T(DEFAULT_UPDATE_URL));
			CheckDlgButton(hwndDlg, IDC_STABLE, TRUE);
		}
		else {
			SetDlgItemTextA(hwndDlg, IDC_CUSTOMURL, dbv.pszVal);
			if ( !strcmp(dbv.pszVal, DEFAULT_UPDATE_URL))
				CheckDlgButton(hwndDlg, IDC_STABLE, TRUE);
			else if ( !strcmp(dbv.pszVal, DEFAULT_UPDATE_URL_TRUNK))
				CheckDlgButton(hwndDlg, IDC_TRUNK, TRUE);
			else {
				CheckDlgButton(hwndDlg, IDC_CUSTOM, TRUE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_CUSTOMURL), TRUE);
			}
			db_free(&dbv);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_UPDATEONSTARTUP:
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			EnableWindow(GetDlgItem(hwndDlg, IDC_ONLYONCEADAY), IsDlgButtonChecked(hwndDlg, IDC_UPDATEONSTARTUP));
			break;

		case IDC_ONLYONCEADAY:
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;

		case IDC_UPDATEONPERIOD:
			{
				BOOL value = IsDlgButtonChecked(hwndDlg, IDC_UPDATEONPERIOD);
				EnableWindow(GetDlgItem(hwndDlg, IDC_PERIOD), value);
				EnableWindow(GetDlgItem(hwndDlg, IDC_PERIODSPIN), value);
				EnableWindow(GetDlgItem(hwndDlg, IDC_PERIODMEASURE), value);
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			}
			break;

		case IDC_TRUNK:
		case IDC_STABLE:
		case IDC_CUSTOM:
			EnableWindow(GetDlgItem(hwndDlg, IDC_CUSTOMURL), IsDlgButtonChecked(hwndDlg, IDC_CUSTOM));
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;

		case IDC_PERIODMEASURE:
			if (HIWORD(wParam) == CBN_SELCHANGE)
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;

		case IDC_UPDATEICONS:
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;

		case IDC_LINK_HOTKEY:
			{
				OPENOPTIONSDIALOG ood = {0};
				ood.cbSize = sizeof(ood);
				ood.pszGroup = "Customize";
				ood.pszPage = "Hotkeys";
				Options_Open(&ood);
			}
			return true;
		}
		break;

	case WM_NOTIFY:
		{
			NMHDR *hdr = (NMHDR *)lParam;
			if (hdr && hdr->code == UDN_DELTAPOS)
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);

			if (hdr && hdr->code == PSN_APPLY) {
				opts.bUpdateOnStartup = IsDlgButtonChecked(hwndDlg, IDC_UPDATEONSTARTUP);
				opts.bOnlyOnceADay = IsDlgButtonChecked(hwndDlg, IDC_ONLYONCEADAY);

				opts.bUpdateOnPeriod = IsDlgButtonChecked(hwndDlg, IDC_UPDATEONPERIOD);

				TCHAR buffer[3] = {0};
				Edit_GetText(GetDlgItem(hwndDlg, IDC_PERIOD), buffer, SIZEOF(buffer));
				opts.Period = _ttoi(buffer);

				opts.bPeriodMeasure = ComboBox_GetCurSel(GetDlgItem(hwndDlg, IDC_PERIODMEASURE));

				InitTimer();

				DBWriteContactSettingByte(NULL, MODNAME, "UpdateOnStartup", opts.bUpdateOnStartup);
				DBWriteContactSettingByte(NULL, MODNAME, "OnlyOnceADay", opts.bOnlyOnceADay);
				DBWriteContactSettingByte(NULL, MODNAME, "UpdateOnPeriod", opts.bUpdateOnPeriod);
				DBWriteContactSettingByte(NULL, MODNAME, "PeriodMeasure", opts.bPeriodMeasure);
				DBWriteContactSettingDword(NULL, MODNAME, "Period", opts.Period);
				opts.bUpdateIcons = IsDlgButtonChecked(hwndDlg, IDC_UPDATEICONS);
				DBWriteContactSettingByte(NULL, MODNAME, "UpdateIcons", opts.bUpdateIcons);

				if ( IsDlgButtonChecked(hwndDlg, IDC_STABLE))
					db_set_s(NULL, MODNAME, "UpdateURL", DEFAULT_UPDATE_URL);
				else if ( IsDlgButtonChecked(hwndDlg, IDC_TRUNK))
					db_set_s(NULL, MODNAME, "UpdateURL", DEFAULT_UPDATE_URL_TRUNK);
				else {
					char szUrl[100];
					GetDlgItemTextA(hwndDlg, IDC_CUSTOMURL, szUrl, SIZEOF(szUrl));
					db_set_s(NULL, MODNAME, "UpdateURL", szUrl);
				}
			}
			break;
		}
	}
	return FALSE;
}

INT_PTR CALLBACK DlgPopUpOpts(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int i;
	char str[20] = {0}, str2[20] = {0};

	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hdlg);

		//Colors
		if (PopupOptions.DefColors == byCOLOR_OWN) {
			CheckDlgButton(hdlg, IDC_USEOWNCOLORS, BST_CHECKED);
			CheckDlgButton(hdlg, IDC_USEPOPUPCOLORS, BST_UNCHECKED);
			CheckDlgButton(hdlg, IDC_USEWINCOLORS, BST_UNCHECKED);
		}
		if (PopupOptions.DefColors == byCOLOR_WINDOWS) {
			CheckDlgButton(hdlg, IDC_USEOWNCOLORS, BST_UNCHECKED);
			CheckDlgButton(hdlg, IDC_USEPOPUPCOLORS, BST_UNCHECKED);
			CheckDlgButton(hdlg, IDC_USEWINCOLORS, BST_CHECKED);
		}
		if (PopupOptions.DefColors == byCOLOR_POPUP) {
			CheckDlgButton(hdlg, IDC_USEOWNCOLORS, BST_UNCHECKED);
			CheckDlgButton(hdlg, IDC_USEPOPUPCOLORS, BST_CHECKED);
			CheckDlgButton(hdlg, IDC_USEWINCOLORS, BST_UNCHECKED);
		}
		SendDlgItemMessage(hdlg, (42071), CPM_SETCOLOUR, 0, PopupsList[0].colorBack);
		SendDlgItemMessage(hdlg, (41071), CPM_SETCOLOUR, 0, PopupsList[0].colorText);
		for (i = 1; i < POPUPS; i++) {
			SendDlgItemMessage(hdlg, (i+42071), CPM_SETCOLOUR, 0, PopupsList[i].colorBack);
			SendDlgItemMessage(hdlg, (i+41071), CPM_SETCOLOUR, 0, PopupsList[i].colorText);
			EnableWindow(GetDlgItem(hdlg, (i+42071)), (PopupOptions.DefColors == byCOLOR_OWN));
			EnableWindow(GetDlgItem(hdlg, (i+41071)), (PopupOptions.DefColors == byCOLOR_OWN));
		}
		//Timeout
		SendDlgItemMessage(hdlg, IDC_TIMEOUT_VALUE, EM_LIMITTEXT, 4, 0);
		SendDlgItemMessage(hdlg, IDC_TIMEOUT_VALUE_SPIN, UDM_SETRANGE32, -1, 9999);
		SetDlgItemInt(hdlg, IDC_TIMEOUT_VALUE, PopupOptions.Timeout, TRUE);
		//Mouse actions
		for (i = 0; i < SIZEOF(PopupActions); i++) {
			SendMessage(GetDlgItem(hdlg, IDC_LC), CB_SETITEMDATA, SendMessage(GetDlgItem(hdlg, IDC_LC), CB_ADDSTRING, 0, (LPARAM)TranslateTS(PopupActions[i].Text)), PopupActions[i].Action);
			SendMessage(GetDlgItem(hdlg, IDC_RC), CB_SETITEMDATA, SendMessage(GetDlgItem(hdlg, IDC_RC), CB_ADDSTRING, 0, (LPARAM)TranslateTS(PopupActions[i].Text)), PopupActions[i].Action);
		}
		SendDlgItemMessage(hdlg, IDC_LC, CB_SETCURSEL, PopupOptions.LeftClickAction, 0);
		SendDlgItemMessage(hdlg, IDC_RC, CB_SETCURSEL, PopupOptions.RightClickAction, 0);

		//Popups nitified
		for (i = 0; i < POPUPS; i++) {
			mir_snprintf(str, SIZEOF(str), "Popups%d", i);
			mir_snprintf(str2, SIZEOF(str2), "Popups%dM", i);
			CheckDlgButton(hdlg, (i+40071), (DBGetContactSettingByte(NULL, MODNAME, str, DEFAULT_POPUP_ENABLED)) ? BST_CHECKED: BST_UNCHECKED);
			CheckDlgButton(hdlg, (i+1024), (DBGetContactSettingByte(NULL, MODNAME, str2, DEFAULT_MESSAGE_ENABLED)) ? BST_CHECKED: BST_UNCHECKED);
			if (IsDlgButtonChecked(hdlg, (i+40071)))
				EnableWindow(GetDlgItem(hdlg, (i+1024)), FALSE);
			else if (i > 0)
				EnableWindow(GetDlgItem(hdlg, (i+1024)), TRUE);
		}

		EnableWindow(GetDlgItem(hdlg, (40071)), FALSE);
		EnableWindow(GetDlgItem(hdlg, (41071)), FALSE);
		EnableWindow(GetDlgItem(hdlg, (42071)), FALSE);
		return TRUE;

	case WM_SHOWWINDOW:
		EnableWindow(GetDlgItem(hdlg, (40071)), FALSE);
		EnableWindow(GetDlgItem(hdlg, (41071)), FALSE);
		EnableWindow(GetDlgItem(hdlg, (42071)), FALSE);
		return TRUE;

	case WM_COMMAND:
		{
			WORD idCtrl = LOWORD(wParam), wNotifyCode = HIWORD(wParam);
			if (wNotifyCode == CPN_COLOURCHANGED) {
				if (idCtrl > 40070) 				{
					//It's a color picker change. idCtrl is the control id.
					COLORREF color = SendDlgItemMessage(hdlg, idCtrl, CPM_GETCOLOUR, 0, 0);
					int ctlID = idCtrl;
					if ((ctlID > 41070) && (ctlID < 42070)) //It's 41071 or above => Text color.
						PopupsList[ctlID-41071].colorText = color;
					else if (ctlID > 42070)//Background color.
						PopupsList[ctlID-42071].colorBack = color;
					SendMessage(GetParent(hdlg), PSM_CHANGED, 0, 0);
					return TRUE;
				}
			}

			if (wNotifyCode == CBN_SELCHANGE) {
				if (idCtrl == IDC_LC)
					PopupOptions.LeftClickAction = (BYTE)SendDlgItemMessage(hdlg, IDC_LC, CB_GETCURSEL, 0, 0);
				else if (idCtrl == IDC_RC)
					PopupOptions.RightClickAction = (BYTE)SendDlgItemMessage(hdlg, IDC_RC, CB_GETCURSEL, 0, 0);

				SendMessage(GetParent(hdlg), PSM_CHANGED, 0, 0);
				return TRUE;
			}
			switch(idCtrl) {
			case IDC_USEOWNCOLORS:
				if (wNotifyCode != BN_CLICKED)
					break;

				PopupOptions.DefColors = byCOLOR_OWN;

				EnableWindow(GetDlgItem(hdlg, (41071)), FALSE);
				EnableWindow(GetDlgItem(hdlg, (42071)), FALSE);

				for (i = 1; i < POPUPS; i++) {
					EnableWindow(GetDlgItem(hdlg, (i+42071)), TRUE); //Background
					EnableWindow(GetDlgItem(hdlg, (i+41071)), TRUE); //Text
				}
				SendMessage(GetParent(hdlg), PSM_CHANGED, 0, 0);
				break;

			case IDC_USEWINCOLORS:
				if (wNotifyCode != BN_CLICKED)
					break;

				//Use Windows colors
				PopupOptions.DefColors = byCOLOR_WINDOWS;
				for (i = 0; i < POPUPS; i++) {
					EnableWindow(GetDlgItem(hdlg, (i+42071)), FALSE); //Background
					EnableWindow(GetDlgItem(hdlg, (i+41071)), FALSE); //Text
				}
				SendMessage(GetParent(hdlg), PSM_CHANGED, 0, 0);
				break;

			case IDC_USEPOPUPCOLORS:
				if (wNotifyCode != BN_CLICKED)
					break;

				//Use Popup colors
				PopupOptions.DefColors = byCOLOR_POPUP;
				for (i = 0; i < POPUPS; i++)  {
					EnableWindow(GetDlgItem(hdlg, (i+42071)), FALSE); //Background
					EnableWindow(GetDlgItem(hdlg, (i+41071)), FALSE); //Text
				}
				SendMessage(GetParent(hdlg), PSM_CHANGED, 0, 0);
				break;

			case IDC_PREVIEW:
				{//Declarations and initializations
					LPCTSTR Title = TranslateT("Plugin Updater");
					LPCTSTR Text = TranslateT("Test");
					for (int i = 0; i < POPUPS; i++) {
						if ((!IsDlgButtonChecked(hdlg, (i+40071))) || (!IsWindowEnabled(GetDlgItem(hdlg, (i+40071)))))
							continue;
						ShowPopup(0, Title, Text, i, 0);
					}
				}
				break;

			case IDC_TIMEOUT_VALUE:
			case IDC_MSG_BOXES:
			case IDC_ERRORS:
				EnableWindow(GetDlgItem(hdlg, IDC_ERRORS_MSG), !IsDlgButtonChecked(hdlg, IDC_ERRORS));
				if ((HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == EN_CHANGE) && (HWND)lParam == GetFocus())
					SendMessage(GetParent(hdlg), PSM_CHANGED, 0, 0);
				break;

			case IDC_INFO_MESSAGES:
				EnableWindow(GetDlgItem(hdlg, IDC_INFO_MESSAGES_MSG), !IsDlgButtonChecked(hdlg, IDC_INFO_MESSAGES));
				if ((HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == EN_CHANGE) && (HWND)lParam == GetFocus())
					SendMessage(GetParent(hdlg), PSM_CHANGED, 0, 0);
				break;

			case IDC_PROGR_DLG:
				EnableWindow(GetDlgItem(hdlg, IDC_PROGR_DLG_MSG), !IsDlgButtonChecked(hdlg, IDC_PROGR_DLG));
				if ((HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == EN_CHANGE) && (HWND)lParam == GetFocus())
					SendMessage(GetParent(hdlg), PSM_CHANGED, 0, 0);
				break;

			case IDC_MSG_BOXES_MSG:
			case IDC_ERRORS_MSG:
			case IDC_INFO_MESSAGES_MSG:
			case IDC_PROGR_DLG_MSG:
				if ((HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == EN_CHANGE) && (HWND)lParam == GetFocus())
					SendMessage(GetParent(hdlg), PSM_CHANGED, 0, 0);
				break;
			}
		}
		break;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_RESET:
			//Restore the options stored in memory.
			LoadOptions();
			InitPopupList();
			return TRUE;

		case PSN_APPLY:
			{
				//Text color
				char szSetting[20] = {0};
				DWORD ctlColor = 0;
				for (i = 0; i <= POPUPS-1; i++) {
					ctlColor = SendDlgItemMessage(hdlg, (i+42071), CPM_GETCOLOUR, 0, 0);
					PopupsList[i].colorBack = ctlColor;
					mir_snprintf(szSetting, SIZEOF(szSetting), "Popups%iBg", i);
					DBWriteContactSettingDword(NULL, MODNAME, szSetting, ctlColor);
					ctlColor = SendDlgItemMessage(hdlg, (i+41071), CPM_GETCOLOUR, 0, 0);
					PopupsList[i].colorText = ctlColor;
					mir_snprintf(szSetting, SIZEOF(szSetting), "Popups%iTx", i);
					DBWriteContactSettingDword(NULL, MODNAME, szSetting, ctlColor);
				}
				//Colors
				DBWriteContactSettingByte(NULL, MODNAME, "DefColors", PopupOptions.DefColors);
				//Timeout
				PopupOptions.Timeout = GetDlgItemInt(hdlg, IDC_TIMEOUT_VALUE, 0, TRUE);
				DBWriteContactSettingDword(NULL, MODNAME, "Timeout", PopupOptions.Timeout);
				//Left mouse click
				DBWriteContactSettingByte(NULL, MODNAME, "LeftClickAction", PopupOptions.LeftClickAction);
				//Right mouse click
				DBWriteContactSettingByte(NULL, MODNAME, "RightClickAction", PopupOptions.RightClickAction);
				//Notified popups
				for (i = 0; i < POPUPS; i++) {
					mir_snprintf(str, SIZEOF(str), "Popups%d", i);
					DBWriteContactSettingByte(NULL, MODNAME, str, (BYTE)(IsDlgButtonChecked(hdlg, (i+40071))));
					mir_snprintf(str2, SIZEOF(str2), "Popups%dM", i);
					DBWriteContactSettingByte(NULL, MODNAME, str2, (BYTE)(IsDlgButtonChecked(hdlg, (i+1024))));
				}
				return TRUE;
			} //case PSN_APPLY
		} // switch code
		break; //End WM_NOTIFY
	} //switch message
	return FALSE;
}

int OptInit(WPARAM wParam, LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp = {0};
	odp.cbSize = sizeof(odp);
	odp.position = 100000000;
	odp.hInstance = hInst;
	odp.flags = ODPF_BOLDGROUPS | ODPF_TCHAR;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_UPDATENOTIFY);
	odp.ptszGroup = LPGENT("Services");
	odp.ptszTitle = LPGENT("Plugin Updater");
	odp.pfnDlgProc = UpdateNotifyOptsProc;
	Options_AddPage(wParam, &odp);

	if ( ServiceExists(MS_POPUP_ADDPOPUP)) {
		odp.pszTemplate = MAKEINTRESOURCEA(IDD_POPUP);
		odp.ptszGroup = LPGENT("PopUps");
		odp.ptszTitle = LPGENT("Plugin Updater");
		odp.pfnDlgProc = DlgPopUpOpts;
		Options_AddPage(wParam, &odp);
	}
	return 0;
}