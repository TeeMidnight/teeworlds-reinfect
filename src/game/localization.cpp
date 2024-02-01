/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <base/system.h>
#include <base/tl/algorithm.h>

#include <engine/external/json-parser/json.h>
#include <engine/shared/config.h>
#include <engine/console.h>
#include <engine/storage.h>

#include "localization.h"

const char *Localize(int Code, const char* pStr, const char *pContext)
{
	const char *pNewStr = g_Localization.FindString(Code, str_quickhash(pStr), str_quickhash(pContext));
	return pNewStr ? pNewStr : pStr;
}

CLocConstString::CLocConstString(int Code, const char *pStr, const char *pContext)
{
	m_pDefaultStr = pStr;
	m_Hash = str_quickhash(m_pDefaultStr);
	m_ContextHash = str_quickhash(pContext);
	m_Code = Code;
}

void CLocConstString::Reload()
{
	const char *pNewStr = g_Localization.FindString(m_Code, m_Hash, m_ContextHash);
	m_pCurrentStr = pNewStr;
	if(!m_pCurrentStr)
		m_pCurrentStr = m_pDefaultStr;
}

CLocalizationDatabase::CLocalizationDatabase()
{
}

void CLocalizationDatabase::Init(CConfig *pConfig)
{
	m_pConfig = pConfig;
}

void CLocalizationDatabase::AddString(int Code, const char *pKeygStr, const char *pNewStr, const char *pContext)
{
	CString s;
	s.m_Hash = str_quickhash(pKeygStr);
	s.m_ContextHash = str_quickhash(pContext);
	s.m_pReplacement = m_StringsHeap.StoreString(*pNewStr ? pNewStr : pKeygStr);
	m_aStrings[Code].add(s);
}

const char *CLocalizationDatabase::GetLanguageStr(int Code)
{
	if(m_LanguagesMap.count(Code))
		return m_LanguagesMap[Code].first;
	
	return "en";
}

int CLocalizationDatabase::GetLanguageCode(const char *pLanguage) const
{
	for(auto& LanguageInfo : m_LanguagesMap)
	{
		if(str_comp_nocase(LanguageInfo.second.first, pLanguage) == 0 || str_comp_nocase(LanguageInfo.second.second, pLanguage) == 0)
		{
			return LanguageInfo.first;
		}
	}
	return -1;
}

bool CLocalizationDatabase::IsLanguageLoaded(int Code)
{
	return m_aStrings.count(Code);
}

bool CLocalizationDatabase::Load(const char *pFilename, IStorage *pStorage, IConsole *pConsole)
{
	// empty string means unload
	if(pFilename[0] == 0)
	{
		m_aStrings.clear();
		m_StringsHeap.Reset();
		return true;
	}
	
	char aFilename[IO_MAX_PATH_LENGTH];
	str_format(aFilename, sizeof(aFilename), "./data/languages/%s.json", pFilename);

	// read file data into buffer
	IOHANDLE File = pStorage->OpenFile(aFilename, IOFLAG_READ, IStorage::TYPE_ALL);
	if(!File)
		return false;
	int FileSize = (int)io_length(File);
	char *pFileData = (char *)mem_alloc(FileSize);
	io_read(File, pFileData, FileSize);
	io_close(File);

	// init
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "loaded '%s'", aFilename);
	pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "localization", aBuf);
	m_aStrings[GetLanguageCode(pFilename)] = sorted_array<CString>(); // add a array
	m_StringsHeap.Reset();

	// parse json data
	json_settings JsonSettings;
	mem_zero(&JsonSettings, sizeof(JsonSettings));
	char aError[256];
	json_value *pJsonData = json_parse_ex(&JsonSettings, pFileData, FileSize, aError);
	mem_free(pFileData);
	
	if(pJsonData == 0)
	{
		pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, aFilename, aError);
		return false;
	}

	// extract data
	const json_value &rStart = (*pJsonData)["translated strings"];
	if(rStart.type == json_array)
	{
		for(unsigned i = 0; i < rStart.u.array.length; ++i)
		{
			bool Valid = true;
			const char *pKey = (const char *)rStart[i]["key"];
			const char *pValue = (const char *)rStart[i]["value"];
			while(pKey[0] && pValue[0])
			{
				for(; pKey[0] && pKey[0] != '%'; ++pKey);
				for(; pValue[0] && pValue[0] != '%'; ++pValue);
				if(pKey[0] && pValue[0] && ((pKey[1] == ' ' && pValue[1] == 0) || (pKey[1] == 0 && pValue[1] == ' ')))	// skip  false positive
					break;
				if((pKey[0] && (!pValue[0] || pKey[1] != pValue[1])) || (pValue[0] && (!pKey[0] || pValue[1] != pKey[1])))
				{
					Valid = false;
					str_format(aBuf, sizeof(aBuf), "skipping invalid entry key:'%s', value:'%s'", (const char *)rStart[i]["key"], (const char *)rStart[i]["value"]);
					pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "localization", aBuf);
					break;
				}
				if(pKey[0])
					++pKey;
				if(pValue[0])
					++pValue;
			}
			if(Valid)
				AddString(GetLanguageCode(pFilename), (const char *)rStart[i]["key"], (const char *)rStart[i]["value"], (const char *)rStart[i]["context"]);
		}
	}

	// clean up
	json_value_free(pJsonData);
	return true;
}

bool CLocalizationDatabase::LoadIndexFile(const char* pFilename, IStorage *pStorage, IConsole *pConsole)
{
	// read file data into buffer
	IOHANDLE File = pStorage->OpenFile(pFilename, IOFLAG_READ, IStorage::TYPE_ALL);
	if(!File)
		return false;
	int FileSize = (int)io_length(File);
	char *pFileData = (char *)mem_alloc(FileSize);
	io_read(File, pFileData, FileSize);
	io_close(File);

	// init
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "loaded '%s'", pFilename);
	pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "localization", aBuf);

	// parse json data
	json_settings JsonSettings;
	mem_zero(&JsonSettings, sizeof(JsonSettings));
	char aError[256];
	json_value *pJsonData = json_parse_ex(&JsonSettings, pFileData, FileSize, aError);
	mem_free(pFileData);
	
	if(pJsonData == 0)
	{
		pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, pFilename, aError);
		return false;
	}

	// extract data
	const json_value &rStart = (*pJsonData)["index"];
	if(rStart.type == json_array)
	{
		for(unsigned i = 0; i < rStart.u.array.length; ++i)
		{
			const char *pFile = (const char *)rStart[i]["file"];
			const char *pName = (const char *)rStart[i]["name"];
			int Code = (int)((int64_t) rStart[i]["code"]);

			str_copy(m_LanguagesMap[Code].first, pFile, sizeof(m_LanguagesMap[Code].first));
			str_copy(m_LanguagesMap[Code].second, pName, sizeof(m_LanguagesMap[Code].second));

			if(str_comp_nocase(pFile, m_pConfig->m_SvDefaultLanguage) == 0)
				Load(pFile, pStorage, pConsole);
		}
	}

	// clean up
	json_value_free(pJsonData);
	return true;
}

const char *CLocalizationDatabase::FindString(int Code, unsigned Hash, unsigned ContextHash)
{
	if(Code == -1 || Code == 826)
		return 0;

	CString String;
	String.m_Hash = Hash;
	String.m_ContextHash = ContextHash;
	String.m_pReplacement = 0x0;
	sorted_array<CString>::range r = ::find_binary(m_aStrings[Code].all(), String);
	if(r.empty())
		return 0;

	unsigned DefaultHash = str_quickhash("");
	unsigned DefaultIndex = 0;
	for(unsigned i = 0; i < r.size() && r.index(i).m_Hash == Hash; ++i)
	{
		const CString &rStr = r.index(i);
		if(rStr.m_ContextHash == ContextHash)
			return rStr.m_pReplacement;
		else if(rStr.m_ContextHash == DefaultHash)
			DefaultIndex = i;
	}
	
    return r.index(DefaultIndex).m_pReplacement;
}

static char s_aList[1024];
const char* CLocalizationDatabase::GetLanguageList()
{
	str_copy(s_aList, "=", sizeof(s_aList));
	for(auto& LanguageInfo : m_LanguagesMap)
	{
		char bBuf[64];
		str_format(bBuf, sizeof(bBuf), "%s(%s), ", LanguageInfo.second.first, LanguageInfo.second.second);

		str_append(s_aList, bBuf, sizeof(s_aList));
	}

	return s_aList;
}

void CLocalizationDatabase::DoUnload(int Code)
{
	if(m_aStrings.count(Code))
		m_aStrings[Code].clear();
}

CLocalizationDatabase g_Localization;
