#define __MAJOR_VERSION				0
#define __MINOR_VERSION				2
#define __RELEASE_NUM				6
#define __BUILD_NUM					0

#define __FILEVERSION_STRING		__MAJOR_VERSION,__MINOR_VERSION,__RELEASE_NUM,__BUILD_NUM
#define __FILEVERSION_DOTS			__MAJOR_VERSION.__MINOR_VERSION.__RELEASE_NUM.__BUILD_NUM

#define __STRINGIFY_IMPL(x)			#x
#define __STRINGIFY(x)				__STRINGIFY_IMPL(x)
#define __VERSION_STRING			__STRINGIFY(__FILEVERSION_DOTS)

#define __PLUGIN_NAME 				"Spell Checker"
#define __INTERNAL_NAME				"SpellChecker"
#define __FILENAME					"SpellChecker.dll"
#define __DESCRIPTION 				"Spell checker for the message windows. Uses Hunspell to do the checking."
#define __AUTHOR					"Ricardo Pescuma Domenecci, FREAK_THEMIGHTY"
#define __AUTHOREMAIL				"pescuma@miranda-im.org"
#define __AUTHORWEB					"http://miranda-ng.org/"
#define __COPYRIGHT					"� 2006-2010 Ricardo Pescuma Domenecci"