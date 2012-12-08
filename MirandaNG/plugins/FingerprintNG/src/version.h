/*
Fingerprint NG (client version) icons module for Miranda NG
Copyright � 2006-12 ghazan, mataes, HierOS, FYR, Bio, nullbie, faith_healer and all respective contributors.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#define __MAJOR_VERSION				0
#define __MINOR_VERSION				1
#define __RELEASE_NUM				0
#define __BUILD_NUM					0

#define __FILEVERSION_STRING      __MAJOR_VERSION,__MINOR_VERSION,__RELEASE_NUM,__BUILD_NUM
#define __FILEVERSION_DOTS        __MAJOR_VERSION.__MINOR_VERSION.__RELEASE_NUM.__BUILD_NUM

#define __STRINGIFY_IMPL(x)       #x
#define __STRINGIFY(x)            __STRINGIFY_IMPL(x)
#define __VERSION_STRING          __STRINGIFY(__FILEVERSION_DOTS)

#define __PLUGIN_NAME             "Fingerprint NG"
#define __INTERNAL_NAME           "Fingerprint NG"
#define __FILENAME                "Fingerprint.dll"
#define __DESCRIPTION             "Fingerprint NG (client version) icons module for Miranda NG."
#define __AUTHOR                  "faith_healer, ghazan, mataes"
#define __AUTHOREMAIL             "faith_healer@miranda.im"
#define __AUTHORWEB               "http://miranda-ng.org/"
#define __COPYRIGHT               "�2006-12 ghazan, mataes, HierOS, FYR, Bio, nullbie, faith_healer and all respective contributors."

// Fingerprint interface ID. Should be the same for ALL fingerprint plugins.
// {FFF4B77A-CE40-11DB-A5CD-06A755D89593}
#define MIID_FINGERPRINT	{0xFFF4B77A, 0xCE40, 0x11DB, { 0xA5, 0xCD, 0x06, 0xA7, 0x55, 0xD8, 0x95, 0x93}}