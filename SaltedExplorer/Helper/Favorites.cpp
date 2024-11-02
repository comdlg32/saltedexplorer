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

CFavorite::CFavorite(const std::wstring &strName,const std::wstring &strLocation,const std::wstring &strDescription) :
	m_strName(strName),
	m_strLocation(strLocation),
	m_strDescription(strDescription),
	m_iVisitCount(0)
{
	CoCreateGuid(&m_guid);
	GetSystemTimeAsFileTime(&m_ftCreated);
}

CFavorite::~CFavorite()
{

}

std::wstring CFavorite::GetName() const
{
	return m_strName;
}

std::wstring CFavorite::GetLocation() const
{
	return m_strLocation;
}

std::wstring CFavorite::GetDescription() const
{
	return m_strDescription;
}

void CFavorite::SetName(const std::wstring &strName)
{
	m_strName = strName;

	CFavoriteItemNotifier::GetInstance().NotifyObserversFavoriteItemModified(m_guid);
}

void CFavorite::SetLocation(const std::wstring &strLocation)
{
	m_strLocation = strLocation;

	CFavoriteItemNotifier::GetInstance().NotifyObserversFavoriteItemModified(m_guid);
}

void CFavorite::SetDescription(const std::wstring &strDescription)
{
	m_strDescription = strDescription;

	CFavoriteItemNotifier::GetInstance().NotifyObserversFavoriteItemModified(m_guid);
}

GUID CFavorite::GetGUID() const
{
	return m_guid;
}

int CFavorite::GetVisitCount() const
{
	return m_iVisitCount;
}

FILETIME CFavorite::GetDateLastVisited() const
{
	return m_ftLastVisited;
}

void CFavorite::UpdateVisitCount()
{
	++m_iVisitCount;
	GetSystemTimeAsFileTime(&m_ftLastVisited);

	CFavoriteItemNotifier::GetInstance().NotifyObserversFavoriteItemModified(m_guid);
}

FILETIME CFavorite::GetDateCreated() const
{
	return m_ftCreated;
}

FILETIME CFavorite::GetDateModified() const
{
	return m_ftModified;
}

CFavoriteFolder CFavoriteFolder::Create(const std::wstring &strName)
{
	return CFavoriteFolder(strName,INITIALIZATION_TYPE_NORMAL);
}

CFavoriteFolder *CFavoriteFolder::CreateNew(const std::wstring &strName)
{
	return new CFavoriteFolder(strName,INITIALIZATION_TYPE_NORMAL);
}

CFavoriteFolder CFavoriteFolder::UnserializeFromRegistry(const std::wstring &strKey)
{
	return CFavoriteFolder(strKey,INITIALIZATION_TYPE_REGISTRY);
}

CFavoriteFolder::CFavoriteFolder(const std::wstring &str,InitializationType_t InitializationType)
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

CFavoriteFolder::~CFavoriteFolder()
{

}

void CFavoriteFolder::Initialize(const std::wstring &strName)
{
	CoCreateGuid(&m_guid);

	m_strName = strName;
	m_nChildFolders = 0;

	GetSystemTimeAsFileTime(&m_ftCreated);
	m_ftModified = m_ftCreated;
}

void CFavoriteFolder::InitializeFromRegistry(const std::wstring &strKey)
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
				CFavoriteFolder FavoriteFolder = CFavoriteFolder::UnserializeFromRegistry(szSubKey);
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

void CFavoriteFolder::SerializeToRegistry(const std::wstring &strKey)
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

			if(CFavoriteFolder *pFavoriteFolder = boost::get<CFavoriteFolder>(&Variant))
			{
				StringCchPrintf(szSubKey,SIZEOF_ARRAY(szSubKey),_T("%s\\FavoriteFolder_%d"),strKey.c_str(),iItem);
				pFavoriteFolder->SerializeToRegistry(szSubKey);
			}
			else if(CFavorite *pFavorite = boost::get<CFavorite>(&Variant))
			{
				StringCchPrintf(szSubKey,SIZEOF_ARRAY(szSubKey),_T("%s\\Favorite_%d"),strKey.c_str(),iItem);

				/* TODO: Serialize. */
			}

			iItem++;
		}

		RegCloseKey(hKey);
	}
}

std::wstring CFavoriteFolder::GetName() const
{
	return m_strName;
}

void CFavoriteFolder::SetName(const std::wstring &strName)
{
	m_strName = strName;

	CFavoriteItemNotifier::GetInstance().NotifyObserversFavoriteItemModified(m_guid);
}

GUID CFavoriteFolder::GetGUID() const
{
	return m_guid;
}

FILETIME CFavoriteFolder::GetDateCreated() const
{
	return m_ftCreated;
}

FILETIME CFavoriteFolder::GetDateModified() const
{
	return m_ftModified;
}

void CFavoriteFolder::InsertFavorite(const CFavorite &Favorite)
{
	InsertFavorite(Favorite,m_ChildList.size());
}

void CFavoriteFolder::InsertFavorite(const CFavorite &Favorite,std::size_t Position)
{
	if(Position > (m_ChildList.size() - 1))
	{
		m_ChildList.push_back(Favorite);
	}
	else
	{
		auto itr = m_ChildList.begin();
		std::advance(itr,Position);
		m_ChildList.insert(itr,Favorite);
	}
	GetSystemTimeAsFileTime(&m_ftModified);

	CFavoriteItemNotifier::GetInstance().NotifyObserversFavoriteAdded(*this,Favorite);
}

void CFavoriteFolder::InsertFavoriteFolder(const CFavoriteFolder &FavoriteFolder)
{
	InsertFavoriteFolder(FavoriteFolder,m_ChildList.size());
}

void CFavoriteFolder::InsertFavoriteFolder(const CFavoriteFolder &FavoriteFolder,std::size_t Position)
{
	if(Position > (m_ChildList.size() - 1))
	{
		m_ChildList.push_back(FavoriteFolder);
	}
	else
	{
		auto itr = m_ChildList.begin();
		std::advance(itr,Position);
		m_ChildList.insert(itr,FavoriteFolder);
	}

	m_nChildFolders++;
	GetSystemTimeAsFileTime(&m_ftModified);

	CFavoriteItemNotifier::GetInstance().NotifyObserversFavoriteFolderAdded(*this,FavoriteFolder);
}

std::list<boost::variant<CFavoriteFolder,CFavorite>>::iterator CFavoriteFolder::begin()
{
	return m_ChildList.begin();
}

std::list<boost::variant<CFavoriteFolder,CFavorite>>::iterator CFavoriteFolder::end()
{
	return m_ChildList.end();
}

std::list<boost::variant<CFavoriteFolder,CFavorite>>::const_iterator CFavoriteFolder::begin() const
{
	return m_ChildList.begin();
}

std::list<boost::variant<CFavoriteFolder,CFavorite>>::const_iterator CFavoriteFolder::end() const
{
	return m_ChildList.end();
}

bool CFavoriteFolder::HasChildFolder() const
{
	if(m_nChildFolders > 0)
	{
		return true;
	}

	return false;
}

CFavoriteItemNotifier::CFavoriteItemNotifier()
{

}

CFavoriteItemNotifier::~CFavoriteItemNotifier()
{

}

CFavoriteItemNotifier& CFavoriteItemNotifier::GetInstance()
{
	static CFavoriteItemNotifier bin;
	return bin;
}

void CFavoriteItemNotifier::AddObserver(NFavorite::IFavoriteItemNotification *pbin)
{
	m_listObservers.push_back(pbin);
}

void CFavoriteItemNotifier::RemoveObserver(NFavorite::IFavoriteItemNotification *pbin)
{
	auto itr = std::find_if(m_listObservers.begin(),m_listObservers.end(),
		[pbin](const NFavorite::IFavoriteItemNotification *pbinCurrent){return pbinCurrent == pbin;});

	if(itr != m_listObservers.end())
	{
		m_listObservers.erase(itr);
	}
}

void CFavoriteItemNotifier::NotifyObserversFavoriteItemModified(const GUID &guid)
{
	NotifyObservers(NOTIFY_FAVORITE_ITEM_MODIFIED,NULL,NULL,NULL,&guid);
}

void CFavoriteItemNotifier::NotifyObserversFavoriteAdded(const CFavoriteFolder &ParentFavoriteFolder,
	const CFavorite &Favorite)
{
	NotifyObservers(NOTIFY_FAVORITE_ADDED,&ParentFavoriteFolder,NULL,&Favorite,NULL);
}

void CFavoriteItemNotifier::NotifyObserversFavoriteFolderAdded(const CFavoriteFolder &ParentFavoriteFolder,
	const CFavoriteFolder &FavoriteFolder)
{
	NotifyObservers(NOTIFY_FAVORITE_FOLDER_ADDED,&ParentFavoriteFolder,&FavoriteFolder,NULL,NULL);
}

void CFavoriteItemNotifier::NotifyObserversFavoriteRemoved(const GUID &guid)
{
	NotifyObservers(NOTIFY_FAVORITE_REMOVED,NULL,NULL,NULL,&guid);
}

void CFavoriteItemNotifier::NotifyObserversFavoriteFolderRemoved(const GUID &guid)
{
	NotifyObservers(NOTIFY_FAVORITE_FOLDER_REMOVED,NULL,NULL,NULL,&guid);
}

void CFavoriteItemNotifier::NotifyObservers(NotificationType_t NotificationType,
	const CFavoriteFolder *pParentFavoriteFolder,const CFavoriteFolder *pFavoriteFolder,
	const CFavorite *pFavorite,const GUID *pguid)
{
	for each(auto pbin in m_listObservers)
	{
		switch(NotificationType)
		{
		case NOTIFY_FAVORITE_ITEM_MODIFIED:
			pbin->OnFavoriteItemModified(*pguid);
			break;

		case NOTIFY_FAVORITE_ADDED:
			pbin->OnFavoriteAdded(*pParentFavoriteFolder,*pFavorite);
			break;

		case NOTIFY_FAVORITE_FOLDER_ADDED:
			pbin->OnFavoriteFolderAdded(*pParentFavoriteFolder,*pFavoriteFolder);
			break;

		case NOTIFY_FAVORITE_REMOVED:
			pbin->OnFavoriteRemoved(*pguid);
			break;

		case NOTIFY_FAVORITE_FOLDER_REMOVED:
			pbin->OnFavoriteFolderRemoved(*pguid);
			break;
		}
	}
}