/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_LOCALIZATION_H
#define GAME_LOCALIZATION_H

#include <base/tl/sorted_array.h>

#include <engine/shared/memheap.h>

#include <map>

class CLocalizationDatabase
{
	class CString
	{
	public:
		unsigned m_Hash;
		unsigned m_ContextHash;
		const char *m_pReplacement;

		bool operator <(const CString &Other) const { return m_Hash < Other.m_Hash || (m_Hash == Other.m_Hash && m_ContextHash < Other.m_ContextHash); }
		bool operator <=(const CString &Other) const { return m_Hash < Other.m_Hash || (m_Hash == Other.m_Hash && m_ContextHash <= Other.m_ContextHash); }
		bool operator ==(const CString &Other) const { return m_Hash == Other.m_Hash && m_ContextHash == Other.m_ContextHash; }
	};

	// Language Code + CString
	std::map<int, sorted_array<CString>> m_aStrings;
	// char[16] = Language String Code, char[32] = Language Name
	std::map<int, std::pair<char[16], char[32]>> m_LanguagesMap;

	array<int> m_UnloadLanguages;

	CHeap m_StringsHeap;

	class CConfig *m_pConfig;

public:
	CLocalizationDatabase();

	void Init(class CConfig *pConfig);

	bool IsLanguageLoaded(int Code);
	bool Load(const char* pFilename, class IStorage *pStorage, class IConsole *pConsole);
	bool LoadIndexFile(const char* pFilename, class IStorage *pStorage, class IConsole *pConsole);

	void AddString(int Code, const char *pOrgStr, const char *pNewStr, const char *pContext);
	const char *FindString(int Code, unsigned Hash, unsigned ContextHash);
	const char *GetLanguageStr(int Code);
	int GetLanguageCode(const char *pLanguage) const;

	const char* GetLanguageList();
	void DoUnload(int Code);
};

extern CLocalizationDatabase g_Localization;

class CLocConstString
{
	const char *m_pDefaultStr;
	const char *m_pCurrentStr;
	unsigned m_Hash;
	unsigned m_ContextHash;
	int m_Code;
public:
	CLocConstString(int Code, const char *pStr, const char *pContext="");
	void Reload();

	inline operator const char *()
	{
		return m_pCurrentStr;
	}
};

const char *Localize(int Code, const char *pStr, const char *pContext="")
GNUC_ATTRIBUTE((format_arg(2)));
#endif
