#include "headers.h"

// {A8A417EF-07AA-4f37-869F-7BFD74886534}
#define MIID_DBEDITOR {0xa8a417ef, 0x7aa, 0x4f37, { 0x86, 0x9f, 0x7b, 0xfd, 0x74, 0x88, 0x65, 0x34}}


HINSTANCE hInst = NULL;

HANDLE hTTBButt = NULL;
BOOL bServiceMode = FALSE;
BOOL usePopUps;
HWND hwnd2watchedVarsWindow;
int UDB;
int hLangpack;
BYTE nameOrder[NAMEORDERCOUNT];
HANDLE hUserMenu;
HANDLE hRestore;
WatchListArrayStruct WatchListArray;
HANDLE sMenuCommand, sRegisterModule, sRegisterSingleModule, sImport, sServicemodeLaunch;
HANDLE hModulesLoadedHook = NULL, hSettingsChangedHook=NULL, hOptInitHook=NULL, hPreShutdownHook=NULL, hTTBHook = NULL;

//========================
//  MirandaPluginInfo
//========================
PLUGININFOEX pluginInfoEx={
    sizeof(PLUGININFOEX),
	__PLUGIN_NAME,
	PLUGIN_MAKE_VERSION(__MAJOR_VERSION, __MINOR_VERSION, __RELEASE_NUM, __BUILD_NUM),
	__DESCRIPTION,
	__AUTHOR,
	__AUTHOREMAIL,
	__COPYRIGHT,
	__AUTHORWEB,
	UNICODE_AWARE,
	MIID_DBEDITOR
};

extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
{
	return &pluginInfoEx;
}

// we implement service mode interface
extern "C" __declspec(dllexport) const MUUID MirandaInterfaces[] = {MIID_SERVICEMODE, MIID_LAST};

//========================
//  WINAPI DllMain
//========================
BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	hInst=hinstDLL;
	return TRUE;
}

void settingChanged(HWND hwnd2Settings, HANDLE hContact, char* module, char* setting);

int DBSettingChanged(WPARAM wParam,LPARAM lParam)
{
	DBCONTACTWRITESETTING *cws=(DBCONTACTWRITESETTING*)lParam;
	HANDLE hContact = (HANDLE)wParam;
	char *setting;
	int i;
	SettingListInfo* info;

	if (hwnd2mainWindow)
	{
		HWND hwnd2Settings = GetDlgItem(hwnd2mainWindow, IDC_SETTINGS);
		if (info = (SettingListInfo*)GetWindowLongPtr(hwnd2Settings,GWLP_USERDATA))
		{
			if ((hContact == info->hContact) && !mir_strcmp(info->module, cws->szModule))
			{
				setting = mir_strdup(cws->szSetting);
				if (cws->value.type == DBVT_DELETED)
				{
					LVFINDINFO lvfi;
					int index;
					lvfi.flags = LVFI_STRING;
					lvfi.psz = setting;
					lvfi.vkDirection = VK_DOWN;
					index = ListView_FindItem(hwnd2Settings,-1,&lvfi);
					if (index > -1)
						ListView_DeleteItem(hwnd2Settings, index);
					mir_free(setting);
					return 0;
				}
				settingChanged(hwnd2Settings, hContact, info->module, setting);
				mir_free(setting);
			}
		}
	}
	// watch list
	if (!hwnd2watchedVarsWindow && !usePopUps) return 0;

	for (i=0; i<WatchListArray.count; i++)
	{
		if (WatchListArray.item[i].module && (hContact == WatchListArray.item[i].hContact))
		{
			if (!mir_strcmp(cws->szModule, WatchListArray.item[i].module))
			{
				if (!WatchListArray.item[i].setting || !mir_strcmp(cws->szSetting, WatchListArray.item[i].setting))
				{
					if (usePopUps)
						popupWatchedVar(hContact, cws->szModule, cws->szSetting);
					if (hwnd2watchedVarsWindow)
						PopulateWatchedWindow(GetDlgItem(hwnd2watchedVarsWindow, IDC_VARS));
					break;
				}
			}
		}
	}
	return 0;
}

INT_PTR DBEditorppMenuCommand(WPARAM wParam, LPARAM lParam)
{

	if (!hwnd2mainWindow) // so only opens 1 at a time
	{
	    hRestore = (HANDLE)wParam;
		SetCursor(LoadCursor(NULL,IDC_WAIT));
		CreateDialog(hInst, MAKEINTRESOURCE(IDD_MAIN), 0, MainDlgProc);
	}
	else
	{
		ShowWindow(hwnd2mainWindow, SW_RESTORE);
		SetForegroundWindow(hwnd2mainWindow);
		if (!hRestore && wParam) {
			hRestore = (HANDLE)wParam;
			refreshTree(4);
		}
	}

	if (hTTBButt)
		CallService(MS_TTB_SETBUTTONSTATE, (WPARAM)hTTBButt, (LPARAM)(TTBST_RELEASED));

	return 0;
}

BOOL IsCP_UTF8(void)
{
	CPINFO CPInfo;

	return GetCPInfo(CP_UTF8, &CPInfo);
}

static int OnTTBLoaded(WPARAM wParam,LPARAM lParam)
{
	TTBButton ttbb = {0};
	HICON ico = LoadIcon(hInst, MAKEINTRESOURCE(ICO_DBE_BUTT));
	UnhookEvent(hTTBHook);

	ttbb.cbSize = sizeof(ttbb);
	ttbb.dwFlags = TTBBF_VISIBLE | TTBBF_SHOWTOOLTIP;
	ttbb.pszService = "DBEditorpp/MenuCommand";
	ttbb.name = LPGEN("Database Editor++");
	ttbb.hIconUp = ico;
	ttbb.pszTooltipUp = LPGEN("Open Database Editor");
	hTTBButt = TopToolbar_AddButton(&ttbb);
	return 0;
}

int ModulesLoaded(WPARAM wParam,LPARAM lParam)
{
	// Register menu item
	CLISTMENUITEM mi = { sizeof(mi) };
	mi.position = 1900000001;
	mi.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(ICO_REGEDIT));
	mi.pszName = modFullname;
	mi.pszService = "DBEditorpp/MenuCommand";
	Menu_AddMainMenuItem(&mi);

	ZeroMemory(&mi, sizeof(mi));
	mi.cbSize = sizeof(mi);
	mi.position = 1900000001;
	mi.flags = DBGetContactSettingByte(NULL,modname,"UserMenuItem",0)?0:CMIF_HIDDEN;
	mi.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(ICO_REGUSER));
	mi.pszName = LPGEN("Open user tree in DBE++");
	mi.pszService = "DBEditorpp/MenuCommand";
	hUserMenu = Menu_AddContactMenuItem(&mi);

	// Register hotkeys
	HOTKEYDESC hkd = {0};
	hkd.cbSize = sizeof(hkd);
	hkd.pszName = "hk_dbepp_open";
	hkd.pszService = "DBEditorpp/MenuCommand";
	hkd.ptszDescription = LPGEN("Open Database Editor");
	hkd.ptszSection = modFullname;
	hkd.DefHotKey = HOTKEYCODE(HOTKEYF_SHIFT|HOTKEYF_EXT, 'D');
	Hotkey_Register(&hkd);

	DBVARIANT dbv;
	char *coreMods = "";
	char *mods;
	char mod[64] = "";
	TCHAR szModuleFileName[MAX_PATH];
	int i=0, len;
	if (!DBGetContactSetting(NULL,modname,"CoreModules",&dbv) && dbv.type == DBVT_ASCIIZ)
		mods = dbv.pszVal;
	else {
		DBWriteContactSettingString(NULL,modname,"CoreModules",coreMods);
		mods = coreMods;
	}

	len = (int)strlen(mods);
	while (i < len) {
		if (mods[i] == '\\' && mods[i+1] == ' ') {
			strcat(mod," ");
			i++;
		}
		else if (mods[i] == ' ' || mods[i] == ',' || mods[i] == '\r' || mods[i] == '\n'|| mods[i] == '\0') {
			if (mod[0])
				CallService("DBEditorpp/RegisterSingleModule",(WPARAM)mod,0);
			mod[0] = '\0';
		}
		else strncat(mod,&mods[i],1);
		i++;
	}

	if (mod[0])
		CallService("DBEditorpp/RegisterSingleModule",(WPARAM)mod,0);

	doOldKnownModulesList(); // add the old plugins which havnt been changed over yet..

	// icons
	if (GetModuleFileName(hInst, szModuleFileName, MAX_PATH))
		addIcons(szModuleFileName);

	DBFreeVariant(&dbv);
	UnhookEvent(hModulesLoadedHook);

	usePopUps = DBGetContactSettingByte(NULL,modname,"UsePopUps",0);

	// Load the name order
	for(i=0; i < NAMEORDERCOUNT; i++)
		nameOrder[i] = i;

	if (!DBGetContactSetting(NULL,"Contact","NameOrder",&dbv)) {
		CopyMemory(nameOrder,dbv.pbVal,dbv.cpbVal);
		DBFreeVariant(&dbv);
	}

	// check DB engine for unicode support
	UDB = FALSE;

	if (ServiceExists(MS_DB_CONTACT_GETSETTING_STR)) {
		DBCONTACTGETSETTING cgs;
		dbv.type = 0;

		if (DBGetContactSettingByte(NULL,modname,"WarnOnDelete",-1) == -1)
			DBWriteContactSettingByte(NULL,modname,"WarnOnDelete",1);

		cgs.szModule=modname;
		cgs.szSetting="WarnOnDelete";
		cgs.pValue=&dbv;

		if (!CallService(MS_DB_CONTACT_GETSETTING_STR, 0,(LPARAM)&cgs))
			if (dbv.type == DBVT_BYTE)
				UDB = TRUE;
	}

	// check OS support for unicode
	// useless if DB doesnt not support unicode
	UOS = (UDB && IsCP_UTF8() && IsWinVerNT());

	hTTBHook = HookEvent(ME_TTB_MODULELOADED, OnTTBLoaded);

	if ( bServiceMode )
		CallService("DBEditorpp/MenuCommand",0,0);

	return 0;
}

int PreShutdown(WPARAM wParam,LPARAM lParam)
{
	if (hwnd2watchedVarsWindow) DestroyWindow(hwnd2watchedVarsWindow);
	if (hwnd2mainWindow) DestroyWindow(hwnd2mainWindow);
	if (hwnd2importWindow) DestroyWindow(hwnd2importWindow);

	UnhookEvent(hSettingsChangedHook);
	UnhookEvent(hOptInitHook);
	UnhookEvent(hPreShutdownHook);

	DestroyServiceFunction(sServicemodeLaunch);
	DestroyServiceFunction(sMenuCommand);
	DestroyServiceFunction(sRegisterModule);
	DestroyServiceFunction(sRegisterSingleModule);
	DestroyServiceFunction(sImport);
	return 0;
}

INT_PTR ServiceMode(WPARAM wParam,LPARAM lParam)
{
	bServiceMode = TRUE;
	return SERVICE_ONLYDB;  // load database and then call us
}

INT_PTR ImportFromFile(WPARAM wParam,LPARAM lParam) 
{
	ImportSettingsFromFileMenuItem((HANDLE)wParam, (char*)lParam);
	return 0;
}

extern "C" __declspec(dllexport) int Load(void)
{
	mir_getLP(&pluginInfoEx);

	hwnd2mainWindow = 0;
	hwnd2watchedVarsWindow = 0;
	hwnd2importWindow = 0;
	hRestore = NULL;
	hSettingsChangedHook = HookEvent(ME_DB_CONTACT_SETTINGCHANGED,DBSettingChanged);
	hOptInitHook = HookEvent(ME_OPT_INITIALISE,OptInit);
	hPreShutdownHook = HookEvent(ME_SYSTEM_PRESHUTDOWN, PreShutdown);
	hModulesLoadedHook = HookEvent(ME_SYSTEM_MODULESLOADED,ModulesLoaded);
	sMenuCommand = CreateServiceFunction("DBEditorpp/MenuCommand", DBEditorppMenuCommand);
	sRegisterModule = CreateServiceFunction("DBEditorpp/RegisterModule", RegisterModule);
	sRegisterSingleModule = CreateServiceFunction("DBEditorpp/RegisterSingleModule", RegisterSingleModule);
	sImport = CreateServiceFunction("DBEditorpp/Import", ImportFromFile);
	
	sServicemodeLaunch = CreateServiceFunction(MS_SERVICEMODE_LAUNCH, ServiceMode);

	CallService("DBEditorpp/RegisterSingleModule",(WPARAM)modname,0);

	// Ensure that the common control DLL is loaded.
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC  = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icex);

	ZeroMemory(&WatchListArray, sizeof(WatchListArray));
	return 0;
}

extern "C" __declspec(dllexport) int Unload(void)
{
	FreeKnownModuleList();
	freeAllWatches();
	return 0;
}



//=======================================================
//DBGetContactSettingString (prob shouldnt use this unless u know how big the string is gonna be..)
//=======================================================

int DBGetContactSettingStringStatic(HANDLE hContact, char* szModule, char* szSetting, char* value, int maxLength)
{
	DBVARIANT dbv;
	if (!DBGetContactSetting(hContact, szModule, szSetting, &dbv))
	{
		strncpy(value, dbv.pszVal, maxLength);
		DBFreeVariant(&dbv);
		return 1;
	}
	else
	{
		DBFreeVariant(&dbv);
		return 0;
	}

	return 0;
}

int WriteBlobFromString(HANDLE hContact,const char *szModule,const char *szSetting, const char *szValue, int len)
{
	int j=0, i = 0;
	BYTE *data = NULL;
	BYTE b;
	int tmp;

	if (!(data = (BYTE *)_alloca(2+len/2)))
	{
		msg(Translate("Couldnt allocate enough memory!"), modFullname);
		return 0;
	}

	while(j<len)
	{
		b = szValue[j];

		if ((b>='0' && b<='9') ||
			(b>='A' && b<='F') ||
			(b>='a' && b<='f'))
		{
			if (sscanf(&szValue[j], "%02X", &tmp) == 1)
			{
				data[i++] = (BYTE)tmp;
				j++;
			}
		}
		j++;
	}

	if (i)
		return DBWriteContactSettingBlob(hContact,szModule,szSetting, data, (WORD)i);

	return 0;
}


int GetSetting(HANDLE hContact, const char *szModule, const char *szSetting, DBVARIANT *dbv)
{
	DBCONTACTGETSETTING cgs;

	cgs.szModule=szModule;
	cgs.szSetting=szSetting;
	cgs.pValue=dbv;
	dbv->type = 0;

	if (UDB)
		return CallService(MS_DB_CONTACT_GETSETTING_STR,(WPARAM)hContact,(LPARAM)&cgs);
	else
	{
		int rr = CallService(MS_DB_CONTACT_GETSETTING,(WPARAM)hContact,(LPARAM)&cgs);

		if 	(dbv->type != DBVT_UTF8)
			return rr;
		else
			return 1;
	}
}


int GetValue(HANDLE hContact, const char* szModule, const char* szSetting, char* Value, int length)
{
    DBVARIANT dbv = {0};

	if (Value && length >= 10 && !GetSetting(hContact, szModule, szSetting, &dbv))
	{
		switch(dbv.type) {
		case DBVT_UTF8:
			if (UOS)
			{
				int len = (int)strlen(dbv.pszVal)+1;
				char *sz = (char*)_alloca(len*3);
				WCHAR *wc = (WCHAR*)_alloca(len*sizeof(WCHAR));
				MultiByteToWideChar(CP_UTF8, 0, dbv.pszVal, -1, wc, len);
				WideCharToMultiByte(CP_ACP, 0, wc, -1, sz, len, NULL, NULL);
				strncpy(Value, sz, length);
				break;
			}// else fall through
		case DBVT_ASCIIZ:
			strncpy(Value, dbv.pszVal, length);
			break;
		case DBVT_DWORD:
			_itoa(dbv.dVal,Value,10);
			break;
		case DBVT_BYTE:
			_itoa(dbv.bVal,Value,10);
			break;
		case DBVT_WORD:
			_itoa(dbv.wVal,Value,10);
			break;
		}

		DBFreeVariant(&dbv);

        Value[length-1] = 0;
		return 1;
	}

	if (Value)
		Value[0] = 0;

	return 0;
}


int GetValueW(HANDLE hContact, const char* szModule, const char* szSetting, WCHAR* Value, int length)
{
	DBVARIANT dbv ={0};

	if (Value && length >= 10 && !GetSetting(hContact, szModule, szSetting, &dbv))
	{
		switch(dbv.type) {
		case DBVT_UTF8:
			{
				int len = (int)strlen(dbv.pszVal) + 1;
				WCHAR *wc = (WCHAR*)_alloca(length*sizeof(WCHAR));
				MultiByteToWideChar(CP_UTF8, 0, dbv.pszVal, -1, wc, len);
				wcsncpy((WCHAR*)Value, wc, length);
			}
			break;
		case DBVT_ASCIIZ:
			{
				int len = (int)strlen(dbv.pszVal) + 1;
				WCHAR *wc = (WCHAR*)_alloca(len*sizeof(WCHAR));
				MultiByteToWideChar(CP_ACP, 0, dbv.pszVal, -1, wc, len);
				wcsncpy((WCHAR*)Value, wc, length);
			}
			break;
		case DBVT_DWORD:
			_itow(dbv.dVal,Value,10);
			break;
		case DBVT_BYTE:
			_itow(dbv.bVal,Value,10);
			break;
		case DBVT_WORD:
			_itow(dbv.wVal,Value,10);
			break;
		}

		DBFreeVariant(&dbv);

		Value[length-1] = 0;
		return 1;
	}

	if (Value)
		Value[0] = 0;

	return 0;
}


char *u2a( wchar_t* src )
{
    if (src)
    {
		int cbLen = WideCharToMultiByte( CP_ACP, 0, src, -1, NULL, 0, NULL, NULL );
		char* result = (char*)mir_calloc((cbLen+1)*sizeof(char));
		if ( result == NULL )
			return NULL;

		WideCharToMultiByte( CP_ACP, 0, src, -1, result, cbLen, NULL, NULL );
		result[ cbLen ] = 0;
		return result;
	}
	else
		return NULL;
}


wchar_t *a2u( char* src , wchar_t *buffer, int len )
{
    wchar_t *result = buffer;
	if ( result == NULL || len < 3)
		return NULL;

	MultiByteToWideChar( CP_ACP, 0, src, -1, result, len - 1 );
		result[ len - 1 ] = 0;

	return result;
}

int mir_snwprintf(WCHAR *buffer, size_t count, const WCHAR* fmt, ...)
{
	va_list va;
	int len;

	va_start(va, fmt);
	len = _vsnwprintf(buffer, count-1, fmt, va);
	va_end(va);

	buffer[count-1] = 0;

	return len;
}


int GetDatabaseString(HANDLE hContact, const char *szModule, const char* szSetting, WCHAR *Value, int length, BOOL unicode)
{
	if (unicode)
		return GetValueW(hContact, szModule, szSetting, Value, length);
	else
		return GetValue(hContact, szModule, szSetting, (char*)Value, length);
}


WCHAR *GetContactName(HANDLE hContact, const char *szProto, int unicode)
{

	int i, r = 0;
	static WCHAR res[512];
	char *proto = (char*)szProto;
	char name[256];

	if (hContact && !proto)
	{
		if (GetValue(hContact,"Protocol","p",name,SIZEOF(name)))
			proto = name;
	}

	if (proto)
	{

		for(i=0;i<NAMEORDERCOUNT-1;i++)
		{
			switch(nameOrder[i])  {
				case 0: // custom name
				{
					r = GetDatabaseString(hContact,"CList","MyHandle",res,SIZEOF(res),unicode);
					break;
				}
				case 1: // nick
				{
					r = GetDatabaseString(hContact,proto,"Nick",res,SIZEOF(res),unicode);
					break;
				}
				case 2: // First Name
				{
					r = GetDatabaseString(hContact,proto,"FirstName",res,SIZEOF(res),unicode);
					break;
				}
				case 3: // E-mail
				{
					r = GetDatabaseString(hContact,proto,"e-mail",res,SIZEOF(res),unicode);
					break;
				}
				case 4: // Last Name
				{
					if (GetDatabaseString(hContact,proto,"LastName",res,SIZEOF(res),unicode))
					break;
				}
				case 5: // Unique id
				{
					// protocol must define a PFLAG_UNIQUEIDSETTING
					char *uid = (char*)CallProtoService(proto,PS_GETCAPS,PFLAG_UNIQUEIDSETTING,0);
					if ((int)uid!=CALLSERVICE_NOTFOUND && uid)
						r = GetDatabaseString(hContact,proto,uid,res,SIZEOF(res),unicode);
					break;
				}
				case 6: // first + last name
				{
					int len = 0;

					if (r = GetDatabaseString(hContact,proto,"FirstName",res,SIZEOF(res),unicode))
					{
						if (unicode)
							len = (int)wcslen(res);
						else
							len = (int)strlen((char*)res);
					}
					else
						res[0] = 0;

					if (len && len < SIZEOF(res) - 2)
					{
						if (unicode)
							wcscat(res,L" ");
						else
							strcat((char*)res," ");

						len++;
					}

					if (SIZEOF(res)-len > 1)
						r |= GetDatabaseString(hContact,proto,"LastName",&res[len],SIZEOF(res)-len,unicode);

					break;
				}
			}

			if (r) return res;
		}
	}

	if (unicode)
		return nick_unknownW;
	else
		return (WCHAR*)nick_unknown;
}