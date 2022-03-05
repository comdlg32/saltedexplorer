/******************************************************************
 *
 * Project: Helper
 * File: Favorites.cpp
 *
 * Implements a Favorites system, with both Favorite folders
 * and Favorites.
 *
 * www.saltedexplorer.ml
 *
 *****************************************************************/

#include "stdafx.h"
#include <list>
#include <algorithm>
#include "Favorites.h"
#include "Macros.h"
#include "RegistrySettings.h"
#include "Helper.h"

Favorite::Favorite(const std::wstring &strName,const std::wstring &strLocation,const std::wstring &strDescription) :
	m_strName(strName),
	m_strLocation(strLocation),
	m_strDescription(strDescription),
	m_iVisitCount(0)
{
	CoCreateGuid(&m_guid);
	GetSystemTimeAsFileTime(&m_ftCreated);
}

Favorite::~Favorite()
{

}

std::wstring Favorite::GetName() const
{
	return m_strName;
}

std::wstring Favorite::GetLocation() const
{
	return m_strLocation;
}

std::wstring Favorite::GetDescription() const
{
	return m_strDescription;
}

void Favorite::SetName(const std::wstring &strName)
{
	m_strName = strName;
}

void Favorite::SetLocation(const std::wstring &strLocation)
{
	m_strLocation = strLocation;
}

void Favorite::SetDescription(const std::wstring &strDescription)
{
	m_strDescription = strDescription;
}

GUID Favorite::GetGUID() const
{
	return m_guid;
}

int Favorite::GetVisitCount() const
{
	return m_iVisitCount;
}

FILETIME Favorite::GetDateLastVisited() const
{
	return m_ftLastVisited;
}

FILETIME Favorite::GetDateCreated() const
{
	return m_ftCreated;
}

FILETIME Favorite::GetDateModified() const
{
	return m_ftModified;
}

FavoriteFolder FavoriteFolder::Create(const std::wstring &strName)
{
	return FavoriteFolder(strName,INITIALIZATION_TYPE_NORMAL);
}

FavoriteFolder *FavoriteFolder::CreateNew(const std::wstring &strName)
{
	return new FavoriteFolder(strName,INITIALIZATION_TYPE_NORMAL);
}

FavoriteFolder FavoriteFolder::UnserializeFromRegistry(const std::wstring &strKey)
{
	return FavoriteFolder(strKey,INITIALIZATION_TYPE_REGISTRY);
}

FavoriteFolder::FavoriteFolder(const std::wstring &str,InitializationType_t InitializationType)
{
	switch(InitializationType)
	{
	case INITIALIZATION_TYPE_REGISTRY:
		InitializeFromRegistry(str);
		break;

	default:
		Initialize(str);
		break;
	}
}

FavoriteFolder::~FavoriteFolder()
{

}

void FavoriteFolder::Initialize(const std::wstring &strName)
{
	CoCreateGuid(&m_guid);

	m_strName = strName;
	m_nChildFolders = 0;

	GetSystemTimeAsFileTime(&m_ftCreated);
	m_ftModified = m_ftCreated;
}

void FavoriteFolder::InitializeFromRegistry(const std::wstring &strKey)
{
	HKEY hKey;
	LONG lRes = RegOpenKeyEx(HKEY_CURRENT_USER,strKey.c_str(),0,KEY_READ,&hKey);

	if(lRes == ERROR_SUCCESS)
	{
		/* TODO: Write GUID. */
		//NRegistrySettings::ReadDwordFromRegistry(hKey,_T("ID"),reinterpret_cast<DWORD *>(&m_ID));
		NRegistrySettings::ReadStringFromRegistry(hKey,_T("Name"),m_strName);
		NRegistrySettings::ReadDwordFromRegistry(hKey,_T("DateCreatedLow"),&m_ftCreated.dwLowDateTime);
		NRegistrySettings::ReadDwordFromRegistry(hKey,_T("DateCreatedHigh"),&m_ftCreated.dwHighDateTime);
		NRegistrySettings::ReadDwordFromRegistry(hKey,_T("DateModifiedLow"),&m_ftModified.dwLowDateTime);
		NRegistrySettings::ReadDwordFromRegistry(hKey,_T("DateModifiedHigh"),&m_ftModified.dwHighDateTime);

		TCHAR szSubKeyName[256];
		DWORD dwSize = SIZEOF_ARRAY(szSubKeyName);
		int iIndex = 0;

		while(RegEnumKeyEx(hKey,iIndex,szSubKeyName,&dwSize,NULL,NULL,NULL,NULL) == ERROR_SUCCESS)
		{
			TCHAR szSubKey[256];
			StringCchPrintf(szSubKey,SIZEOF_ARRAY(szSubKey),_T("%s\\%s"),strKey.c_str(),szSubKeyName);

			if(CheckWildcardMatch(_T("FavoriteFolder_*"),szSubKeyName,FALSE))
			{
				FavoriteFolder FavoriteFolder = FavoriteFolder::UnserializeFromRegistry(szSubKey);
				m_ChildList.push_back(FavoriteFolder);
			}
			else if(CheckWildcardMatch(_T("Favorite_*"),szSubKeyName,FALSE))
			{
				/* TODO: Create Favorite. */
			}

			dwSize = SIZEOF_ARRAY(szSubKeyName);
			iIndex++;
		}

		RegCloseKey(hKey);
	}
}

void FavoriteFolder::SerializeToRegistry(const std::wstring &strKey)
{
	HKEY hKey;
	LONG lRes = RegCreateKeyEx(HKEY_CURRENT_USER,strKey.c_str(),
	0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey,NULL);

	if(lRes == ERROR_SUCCESS)
	{
		/* These details don't need to be saved for the root Favorite. */
		/* TODO: Read GUID. */
		//NRegistrySettings::SaveDwordToRegistry(hKey,_T("ID"),m_ID);
		NRegistrySettings::SaveStringToRegistry(hKey,_T("Name"),m_strName.c_str());
		NRegistrySettings::SaveDwordToRegistry(hKey,_T("DateCreatedLow"),m_ftCreated.dwLowDateTime);
		NRegistrySettings::SaveDwordToRegistry(hKey,_T("DateCreatedHigh"),m_ftCreated.dwHighDateTime);
		NRegistrySettings::SaveDwordToRegistry(hKey,_T("DateModifiedLow"),m_ftModified.dwLowDateTime);
		NRegistrySettings::SaveDwordToRegistry(hKey,_T("DateModifiedHigh"),m_ftModified.dwHighDateTime);

		int iItem = 0;

		for each(auto Variant in m_ChildList)
		{
			TCHAR szSubKey[256];

			if(FavoriteFolder *pFavoriteFolder = boost::get<FavoriteFolder>(&Variant))
			{
				StringCchPrintf(szSubKey,SIZEOF_ARRAY(szSubKey),_T("%s\\FavoriteFolder_%d"),strKey.c_str(),iItem);
				pFavoriteFolder->SerializeToRegistry(szSubKey);
			}
			else if(Favorite *pFavorite = boost::get<Favorite>(&Variant))
			{
				StringCchPrintf(szSubKey,SIZEOF_ARRAY(szSubKey),_T("%s\\Favorite_%d"),strKey.c_str(),iItem);

				/* TODO: Serialize. */
			}

			iItem++;
		}

		RegCloseKey(hKey);
	}
}

std::wstring FavoriteFolder::GetName() const
{
	return m_strName;
}

void FavoriteFolder::SetName(const std::wstring &strName)
{
	m_strName = strName;
}

GUID FavoriteFolder::GetGUID() const
{
	return m_guid;
}

FILETIME FavoriteFolder::GetDateCreated() const
{
	return m_ftCreated;
}

FILETIME FavoriteFolder::GetDateModified() const
{
	return m_ftModified;
}

void FavoriteFolder::InsertFavorite(const Favorite &bm)
{
	InsertFavorite(bm,m_ChildList.size());
}

void FavoriteFolder::InsertFavorite(const Favorite &bm,std::size_t Position)
{
	if(Position > (m_ChildList.size() - 1))
	{
		m_ChildList.push_back(bm);
	}
	else
	{
		auto itr = m_ChildList.begin();
		std::advance(itr,Position);
		m_ChildList.insert(itr,bm);
	}
	GetSystemTimeAsFileTime(&m_ftModified);
}

void FavoriteFolder::InsertFavoriteFolder(const FavoriteFolder &bf)
{
	InsertFavoriteFolder(bf,m_ChildList.size());
}

void FavoriteFolder::InsertFavoriteFolder(const FavoriteFolder &bf,std::size_t Position)
{
	if(Position > (m_ChildList.size() - 1))
	{
		m_ChildList.push_back(bf);
	}
	else
	{
		auto itr = m_ChildList.begin();
		std::advance(itr,Position);
		m_ChildList.insert(itr,bf);
	}

	m_nChildFolders++;
	GetSystemTimeAsFileTime(&m_ftModified);
}

std::list<boost::variant<FavoriteFolder,Favorite>>::iterator FavoriteFolder::begin()
{
	return m_ChildList.begin();
}

std::list<boost::variant<FavoriteFolder,Favorite>>::iterator FavoriteFolder::end()
{
	return m_ChildList.end();
}

bool FavoriteFolder::HasChildFolder() const
{
	if(m_nChildFolders > 0)
	{
		return true;
	}

	return false;
}