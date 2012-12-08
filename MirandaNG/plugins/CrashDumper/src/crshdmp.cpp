/*
Miranda Crash Dumper Plugin
Copyright (C) 2008 - 2012 Boris Krasnovskiy All Rights Reserved

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation version 2
of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "utils.h"
#include <m_options.h>
#include "m_folders.h"
#include "m_toptoolbar.h"
#include "version.h"

int hLangpack;

HINSTANCE hInst;
DWORD mirandaVersion;
LCID packlcid;
HANDLE hCrashLogFolder, hVerInfoFolder;
HMODULE hRichModule;

TCHAR* vertxt;
TCHAR* profname;
TCHAR* profpath;

TCHAR CrashLogFolder[MAX_PATH];
TCHAR VersionInfoFolder[MAX_PATH];

bool servicemode;
bool clsdates;
bool dtsubfldr;

extern HWND hViewWnd;

static const PLUGININFOEX pluginInfoEx =
{
	sizeof(PLUGININFOEX),
	"Crash Dumper",
	__VERSION_DWORD,
	"Crash Dumper for Miranda NG.",
	"borkra",
	"borkra@miranda-im.org",
	"Copyright� 2008 - 2012 Boris Krasnovskiy All Rights Reserved",
	"http://miranda-ng.org/",
	UNICODE_AWARE,
	// {F62C1D7A-FFA4-4065-A251-4C9DD9101CC8}
	{ 0xf62c1d7a, 0xffa4, 0x4065, { 0xa2, 0x51, 0x4c, 0x9d, 0xd9, 0x10, 0x1c, 0xc8 } }
};

const PLUGININFOEX* GetPluginInfoEx(void) { return &pluginInfoEx; }


extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirVersion)
{
	::mirandaVersion = mirVersion;
	return (PLUGININFOEX*)&pluginInfoEx;
}

#define MIID_CRASHDUMP  { 0x36a31a50, 0xcb55, 0x46d0, { 0xab, 0x9c, 0x1e, 0xac, 0xfb, 0x24, 0x0, 0x2a } }

// MirandaInterfaces - returns the protocol interface to the core
extern "C" __declspec(dllexport) const MUUID MirandaInterfaces[] = { MIID_SERVICEMODE, MIID_CRASHDUMP, MIID_LAST };

INT_PTR StoreVersionInfoToFile(WPARAM, LPARAM lParam)
{
	CreateDirectoryTree(VersionInfoFolder);

	TCHAR path[MAX_PATH];
	crs_sntprintf(path, MAX_PATH, TEXT("%s\\VersionInfo.txt"), VersionInfoFolder);

	HANDLE hDumpFile = CreateFile(path, GENERIC_WRITE, 0, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hDumpFile != INVALID_HANDLE_VALUE)
	{
		bkstring buffer;

		buffer.reserve(0x1800);
		PrintVersionInfo(buffer, (unsigned int)lParam | VI_FLAG_PRNVAR);

		char* bufu = mir_utf8encodeT(buffer.c_str());
		WriteUtfFile(hDumpFile, bufu);
		mir_free(bufu);

		CloseHandle(hDumpFile);

		ShowMessage(3, TranslateT("VersionInfo stored into file %s"), path);
	}
	else
		ShowMessage(2, TranslateT("VersionInfo file %s is inaccessible"), path);

	return 0;
}

INT_PTR StoreVersionInfoToClipboard(WPARAM, LPARAM lParam)
{
	bkstring buffer;
	buffer.reserve(0x1800);

	WriteBBFile(buffer, true);
	PrintVersionInfo(buffer, (unsigned int)lParam | VI_FLAG_PRNVAR | VI_FLAG_FORMAT);
	WriteBBFile(buffer, false);

	StoreStringToClip(buffer);

	return 0;
}

INT_PTR UploadVersionInfo(WPARAM, LPARAM lParam)
{
	bkstring buffer;
	buffer.reserve(0x1800);
	PrintVersionInfo(buffer);

	VerTrnsfr *trn = (VerTrnsfr*)mir_alloc(sizeof(VerTrnsfr));
	trn->buf = mir_utf8encodeT(buffer.c_str());
	trn->autot = lParam == 0xa1;

	mir_forkthread(VersionInfoUploadThread, trn);

	return 0;
}


INT_PTR ViewVersionInfo(WPARAM wParam, LPARAM)
{
	//	unsigned *p = (unsigned*)0x15;
	//	*p = 324;

	if (hRichModule == NULL && GetModuleHandle(TEXT("Riched20.dll")) == NULL)
		hRichModule = LoadLibrary(TEXT("Riched20.dll"));
	if(hViewWnd)
	{
		SetForegroundWindow(hViewWnd);
		SetFocus(hViewWnd);
	}
	else
	{
		CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_VIEWVERSION), NULL,
			DlgProcView, wParam ? (VI_FLAG_PRNVAR | VI_FLAG_PRNDLL) : VI_FLAG_PRNVAR);
	}

	return 0;
}

INT_PTR OpenUrl(WPARAM wParam, LPARAM)
{
	switch (wParam)
	{
	case 0:
		ShellExecute(NULL, TEXT("explore"), CrashLogFolder, NULL, NULL, SW_SHOW);
		break;

	case 1:
		OpenAuthUrl("http://%s.miranda-vi.org");
		break;
	}
	return 0;
}

INT_PTR ServiceModeLaunch(WPARAM, LPARAM)
{
	servicemode = true;
	return SERVICE_ONLYDB;
}


static int FoldersPathChanged(WPARAM, LPARAM)
{
	FOLDERSGETDATA fgd = {0};
	fgd.cbSize = sizeof(FOLDERSGETDATA);
	fgd.nMaxPathSize = MAX_PATH;
	fgd.szPathT = CrashLogFolder;
	fgd.flags = FF_TCHAR;
	CallService(MS_FOLDERS_GET_PATH, (WPARAM) hCrashLogFolder, (LPARAM) &fgd);

	fgd.szPathT = VersionInfoFolder;
	CallService(MS_FOLDERS_GET_PATH, (WPARAM) hVerInfoFolder, (LPARAM) &fgd);
	return 0;
}

int OptionsInit(WPARAM wParam, LPARAM)
{
	OPTIONSDIALOGPAGE odp = { 0 };

	odp.cbSize      = sizeof(odp);
	odp.position    = -790000000;
	odp.hInstance   = hInst;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPTIONS);
	odp.pszTitle    = PluginName;
	odp.pszGroup    = LPGEN("Services");
	odp.flags       = ODPF_BOLDGROUPS;
	odp.pfnDlgProc  = DlgProcOptions;
	Options_AddPage(wParam, &odp);

	return 0;
}

static int ToolbarModulesLoaded(WPARAM, LPARAM)
{
	TTBButton tbb = {0};
	tbb.cbSize = sizeof(TTBButton);

	tbb.pszService = MS_CRASHDUMPER_STORETOCLIP;
	tbb.name = tbb.pszTooltipUp = LPGEN("Version Information To Clipboard");
	tbb.hIconHandleUp = GetIconHandle(IDI_VITOCLIP);
	tbb.dwFlags = TTBBF_VISIBLE;
	TopToolbar_AddButton(&tbb);

	tbb.pszService = MS_CRASHDUMPER_STORETOFILE;
	tbb.name = tbb.pszTooltipUp = LPGEN("Version Information To File");
	tbb.hIconHandleUp = GetIconHandle(IDI_VITOFILE);
	tbb.dwFlags = 0;
	TopToolbar_AddButton(&tbb);

	tbb.pszService = MS_CRASHDUMPER_VIEWINFO;
	tbb.name = tbb.pszTooltipUp = LPGEN("Show Version Information");
	tbb.hIconHandleUp = GetIconHandle(IDI_VISHOW);
	TopToolbar_AddButton(&tbb);

	tbb.pszService = MS_CRASHDUMPER_UPLOAD;
	tbb.name = tbb.pszTooltipUp = LPGEN("Upload Version Information");
	tbb.hIconHandleUp = GetIconHandle(IDI_VIUPLOAD);
	TopToolbar_AddButton(&tbb);
	return 0;
}

static int ModulesLoaded(WPARAM, LPARAM)
{
	char temp[MAX_PATH];
	CallService(MS_SYSTEM_GETVERSIONTEXT, (WPARAM)SIZEOF(temp), (LPARAM)temp);
	crs_a2t(vertxt, temp);

	profname = Utils_ReplaceVarsT(_T("%miranda_profilename%.dat"));
	if (ServiceExists(MS_FOLDERS_REGISTER_PATH))
		profpath = mir_tstrdup( _T("%miranda_userdata%"));
	else
		profpath = Utils_ReplaceVarsT(_T("%miranda_userdata%"));

	crs_sntprintf(CrashLogFolder, MAX_PATH, TEXT("%s\\CrashLog"), profpath);
	crs_sntprintf(VersionInfoFolder, MAX_PATH, TEXT("%s"), profpath);

	SetExceptionHandler();

	hCrashLogFolder = FoldersRegisterCustomPathT(PluginName, "Crash Reports", CrashLogFolder);
	hVerInfoFolder  = FoldersRegisterCustomPathT(PluginName, "Version Information", VersionInfoFolder);

	FoldersPathChanged(0, 0);

	HookEvent(ME_FOLDERS_PATH_CHANGED, FoldersPathChanged);
	HookEvent(ME_TTB_MODULELOADED, ToolbarModulesLoaded);

	UploadInit();

	CLISTMENUITEM mi = { sizeof(mi) };
	mi.popupPosition = 2000089999;
	mi.position = 2000089999;
	mi.flags = CMIF_ROOTPOPUP | CMIF_ICONFROMICOLIB | CMIF_TCHAR;
	mi.icolibItem = GetIconHandle(IDI_VI);
	mi.ptszName = LPGENT("Version Information");
	mi.pszPopupName = (char *)-1;
	HANDLE hMenuRoot = Menu_AddMainMenuItem(&mi);

	mi.flags = CMIF_CHILDPOPUP | CMIF_ICONFROMICOLIB | CMIF_TCHAR;
	mi.pszPopupName = (char *)hMenuRoot;
	mi.popupPosition = 0;

	mi.position = 2000089995;
	mi.ptszName = LPGENT("Copy to clipboard");
	mi.icolibItem = GetIconHandle(IDI_VITOCLIP);
	mi.pszService = MS_CRASHDUMPER_STORETOCLIP;
	Menu_AddMainMenuItem(&mi);

	mi.position = 2000089996;
	mi.ptszName = LPGENT("Store to file");
	mi.icolibItem = GetIconHandle(IDI_VITOFILE);
	mi.pszService = MS_CRASHDUMPER_STORETOFILE;
	Menu_AddMainMenuItem(&mi);

	mi.position  = 2000089997;
	mi.ptszName = LPGENT("Show");
	mi.icolibItem = GetIconHandle(IDI_VISHOW);
	mi.pszService = MS_CRASHDUMPER_VIEWINFO;
	Menu_AddMainMenuItem(&mi);

	mi.popupPosition = 1;
	mi.position  = 2000089998;
	mi.ptszName = LPGENT("Show with DLLs");
	mi.icolibItem = GetIconHandle(IDI_VIUPLOAD);
	mi.pszService = MS_CRASHDUMPER_VIEWINFO;
	Menu_AddMainMenuItem(&mi);

	mi.popupPosition = 0;
	mi.position  = 2000089999;
	mi.ptszName = LPGENT("Upload");
	mi.icolibItem = GetIconHandle(IDI_VIUPLOAD);
	mi.pszService = MS_CRASHDUMPER_UPLOAD;
	Menu_AddMainMenuItem(&mi);

	mi.position  = 2000099990;
	mi.ptszName = LPGENT("Open crash report directory");
	mi.icolibItem = LoadSkinnedIconHandle(SKINICON_EVENT_FILE);
	mi.pszService = MS_CRASHDUMPER_URL;
	Menu_AddMainMenuItem(&mi);

	mi.popupPosition = 1;
	mi.position  = 2000099991;
	mi.ptszName = LPGENT("Open miranda-vi.org");
	mi.icolibItem = LoadSkinnedIconHandle(SKINICON_EVENT_URL);
	mi.pszService = MS_CRASHDUMPER_URL;
	Menu_AddMainMenuItem(&mi);

	HOTKEYDESC hk = {0};
	hk.cbSize = sizeof(hk);
	hk.pszSection = PluginName;

	hk.pszDescription = LPGEN("Copy Version Info to clipboard");
	hk.pszName = "CopyVerInfo";
	hk.pszService = MS_CRASHDUMPER_STORETOCLIP;
	Hotkey_Register(&hk);

	hk.pszDescription = LPGEN("Show Version Info");
	hk.pszName = "ShowVerInfo";
	hk.pszService = MS_CRASHDUMPER_VIEWINFO;
	Hotkey_Register(&hk);

	if (servicemode)
		ViewVersionInfo(0, 0);
	else if (DBGetContactSettingByte(NULL, PluginName, "UploadChanged", 0) && !ProcessVIHash(false))
		UploadVersionInfo(0, 0xa1);

	CheckForOtherCrashReportingPlugins();
	return 0;
}

static int PreShutdown(WPARAM, LPARAM)
{
	DestroyAllWindows();
	UploadClose();
	return 0;
}

extern "C" int __declspec(dllexport) Load(void)
{
	clsdates = DBGetContactSettingByte(NULL, PluginName, "ClassicDates", 1) != 0;

	dtsubfldr = DBGetContactSettingByte(NULL, PluginName, "SubFolders", 1) != 0;
	mir_getLP(&pluginInfoEx);

	HookEvent(ME_SYSTEM_MODULESLOADED, ModulesLoaded);
	HookEvent(ME_OPT_INITIALISE, OptionsInit);
	HookEvent(ME_TTB_MODULELOADED, ToolbarModulesLoaded);
	HookEvent(ME_SYSTEM_PRESHUTDOWN, PreShutdown);

	packlcid = (LCID)CallService(MS_LANGPACK_GETLOCALE, 0, 0);

	InitIcons();

	InitExceptionHandler();

	CreateServiceFunction(MS_CRASHDUMPER_STORETOFILE, StoreVersionInfoToFile);
	CreateServiceFunction(MS_CRASHDUMPER_STORETOCLIP, StoreVersionInfoToClipboard);
	CreateServiceFunction(MS_CRASHDUMPER_VIEWINFO, ViewVersionInfo);
	CreateServiceFunction(MS_CRASHDUMPER_UPLOAD, UploadVersionInfo);
	CreateServiceFunction(MS_CRASHDUMPER_URL, OpenUrl);
	CreateServiceFunction(MS_SERVICEMODE_LAUNCH, ServiceModeLaunch);
	return 0;
}


extern "C" int __declspec(dllexport) Unload(void)
{
	DestroyAllWindows();
	FreeLibrary(hRichModule);

	DestroyExceptionHandler();

	mir_free(profpath);
	mir_free(profname);
	mir_free(vertxt);
	return 0;
}


extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID /*lpvReserved*/)
{
	switch(fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hinstDLL);
		hInst = hinstDLL;
		break;

	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}