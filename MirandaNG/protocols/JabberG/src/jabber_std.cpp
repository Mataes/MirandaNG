/*

Jabber Protocol Plugin for Miranda IM
Copyright (C) 2002-04  Santithorn Bunchua
Copyright (C) 2005-12  George Hazan
Copyright (C) 2012-13  Miranda NG Project

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

#include "jabber.h"

/////////////////////////////////////////////////////////////////////////////////////////

void CJabberProto::JLoginFailed(int errorCode)
{
	*m_savedPassword = 0;
	ProtoBroadcastAck(NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, errorCode);
}

// save/load crypted strings
void __forceinline sttCryptString(char *str)
{
	for (;*str; ++str) {
		const char c = *str ^ 0xc3;
		if (c) *str = c;
	}
}

TCHAR* CJabberProto::JGetStringCrypt(HANDLE hContact, char *valueName)
{
	DBVARIANT dbv;
	if ( db_get_s(hContact, m_szModuleName, valueName, &dbv))
		return NULL;

	sttCryptString(dbv.pszVal);
	WCHAR *res = mir_utf8decodeW(dbv.pszVal);
	db_free(&dbv);
	return res;
}

DWORD CJabberProto::JSetStringCrypt(HANDLE hContact, char *valueName, const TCHAR *parValue)
{
	char *tmp = mir_utf8encodeT(parValue);
	sttCryptString(tmp);
	setString(hContact, valueName, tmp);
	mir_free(tmp);
	return 0;
}
