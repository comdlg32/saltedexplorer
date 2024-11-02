/******************************************************************
 *
 * Project: SaltedExplorer
 * File: FavoritesHelper.cpp
 *
 * Provides several helper functions for Favorites.
 *
 * Toiletflusher and XP Pro
 * www.saltedexplorer.ml
 *
 *****************************************************************/

#include "stdafx.h"
#include <stack>
#include <algorithm>
#include "SaltedExplorer_internal.h"
#include "FavoritesHelper.h"
#include "../Helper/Favorites.h"
#include "../Helper/Macros.h"

CFavoritesTreeView::CFavoritesTreeView(HWND hTreeView,CFavoriteFolder *pAllFavorites,
	const GUID &guidSelected,const NFavoritesHelper::setExpansion_t &setExpansion) :
	m_hTreeView(hTreeView),
	m_pAllFavorites(pAllFavorites),
	m_uIDCounter(0)
{
	SetWindowSubclass(hTreeView,FavoritesTreeViewProcStub,0,reinterpret_cast<DWORD_PTR>(this));

	m_himl = ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,0,48);
	HBITMAP hBitmap = LoadBitmap(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_SHELLIMAGES_2000));
	ImageList_Add(m_himl,hBitmap,NULL);
	TreeView_SetImageList(hTreeView,m_himl,TVSIL_NORMAL);
	DeleteObject(hBitmap);

	SetupTreeView(guidSelected,setExpansion);
}

CFavoritesTreeView::~CFavoritesTreeView()
{
	RemoveWindowSubclass(m_hTreeView,FavoritesTreeViewProcStub,0);

	ImageList_Destroy(m_himl);
}

LRESULT CALLBACK FavoritesTreeViewProcStub(HWND hwnd,UINT uMsg,
	WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	CFavoritesTreeView *pbtv = reinterpret_cast<CFavoritesTreeView *>(dwRefData);

	return pbtv->TreeViewProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK CFavoritesTreeView::TreeViewProc(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	switch(Msg)
	{
	case WM_NOTIFY:
		switch(reinterpret_cast<NMHDR *>(lParam)->code)
		{
		case TVN_DELETEITEM:
			OnTvnDeleteItem(reinterpret_cast<NMTREEVIEW *>(lParam));
			break;
		}
		break;
	}

	return DefSubclassProc(hwnd,Msg,wParam,lParam);
}

void CFavoritesTreeView::SetupTreeView(const GUID &guidSelected,const NFavoritesHelper::setExpansion_t &setExpansion)
{
	TreeView_DeleteAllItems(m_hTreeView);

	HTREEITEM hRoot = InsertFolderIntoTreeView(NULL,*m_pAllFavorites);
	InsertFoldersIntoTreeViewRecursive(hRoot,*m_pAllFavorites);

	for each(auto guidExpanded in setExpansion)
	{
		auto itrExpanded = m_mapItem.find(guidExpanded);

		if(itrExpanded != m_mapItem.end())
		{
			CFavoriteFolder &FavoriteFolder = GetFavoriteFolderFromTreeView(itrExpanded->second);

			if(FavoriteFolder.HasChildFolder())
			{
				TreeView_Expand(m_hTreeView,itrExpanded->second,TVE_EXPAND);
			}
		}
	}

	auto itrSelected = m_mapItem.find(guidSelected);
	
	if(itrSelected != m_mapItem.end())
	{
		TreeView_SelectItem(m_hTreeView,itrSelected->second);
	}
}

void CFavoritesTreeView::InsertFoldersIntoTreeViewRecursive(HTREEITEM hParent,const CFavoriteFolder &FavoriteFolder)
{
	for(auto itr = FavoriteFolder.begin();itr != FavoriteFolder.end();++itr)
	{
		if(itr->type() == typeid(CFavoriteFolder))
		{
			const CFavoriteFolder &FavoriteFolderChild = boost::get<CFavoriteFolder>(*itr);

			HTREEITEM hCurrentItem = InsertFolderIntoTreeView(hParent,
				FavoriteFolderChild);

			if(FavoriteFolderChild.HasChildFolder())
			{
				InsertFoldersIntoTreeViewRecursive(hCurrentItem,
					FavoriteFolderChild);
			}
		}
	}
}

HTREEITEM CFavoritesTreeView::InsertFolderIntoTreeView(HTREEITEM hParent,const CFavoriteFolder &FavoriteFolder)
{
	TCHAR szText[256];
	StringCchCopy(szText,SIZEOF_ARRAY(szText),FavoriteFolder.GetName().c_str());

	int nChildren = 0;

	if(FavoriteFolder.HasChildFolder())
	{
		nChildren = 1;
	}

	TVITEMEX tviex;
	tviex.mask				= TVIF_TEXT|TVIF_IMAGE|TVIF_CHILDREN|TVIF_SELECTEDIMAGE|TVIF_PARAM;
	tviex.pszText			= szText;
	tviex.iImage			= SHELLIMAGES_NEWTAB;
	tviex.iSelectedImage	= SHELLIMAGES_NEWTAB;
	tviex.cChildren			= nChildren;
	tviex.lParam			= m_uIDCounter;

	TVINSERTSTRUCT tvis;
	tvis.hParent			= hParent;
	tvis.hInsertAfter		= TVI_LAST;
	tvis.itemex				= tviex;
	HTREEITEM hItem = TreeView_InsertItem(m_hTreeView,&tvis);

	m_mapID.insert(std::make_pair<UINT,GUID>(m_uIDCounter,FavoriteFolder.GetGUID()));
	++m_uIDCounter;

	m_mapItem.insert(std::make_pair<GUID,HTREEITEM>(FavoriteFolder.GetGUID(),hItem));

	return hItem;
}

void CFavoritesTreeView::FavoriteFolderAdded(const CFavoriteFolder &ParentFavoriteFolder,const CFavoriteFolder &FavoriteFolder)
{
	/* Due to the fact that *all* bookmark folders will be inserted
	into the treeview (regardless of whether or not they are actually
	shown), any new folders will always need to be inserted. */
	auto itr = m_mapItem.find(ParentFavoriteFolder.GetGUID());
	assert(itr != m_mapItem.end());
	InsertFolderIntoTreeView(itr->second,FavoriteFolder);

	UINT uParentState = TreeView_GetItemState(m_hTreeView,itr->second,TVIS_EXPANDED);

	if((uParentState & TVIS_EXPANDED) != TVIS_EXPANDED)
	{
		TVITEM tvi;
		tvi.mask		= TVIF_CHILDREN;
		tvi.hItem		= itr->second;
		tvi.cChildren	= 1;
		TreeView_SetItem(m_hTreeView,&tvi);
	}
}

void CFavoritesTreeView::FavoriteFolderModified(const GUID &guid)
{
	auto itr = m_mapItem.find(guid);
	assert(itr != m_mapItem.end());

	CFavoriteFolder &FavoriteFolder = GetFavoriteFolderFromTreeView(itr->second);

	TCHAR szText[256];
	StringCchCopy(szText,SIZEOF_ARRAY(szText),FavoriteFolder.GetName().c_str());

	/* The only property of the bookmark folder shown
	within the treeview is its name, so that is all
	that needs to be updated here. */
	TVITEM tvi;
	tvi.mask		= TVIF_TEXT;
	tvi.hItem		= itr->second;
	tvi.pszText		= szText;
	TreeView_SetItem(m_hTreeView,&tvi);
}

void CFavoritesTreeView::OnTvnDeleteItem(NMTREEVIEW *pnmtv)
{
	auto itrID = m_mapID.find(static_cast<UINT>(pnmtv->itemOld.lParam));

	if(itrID == m_mapID.end())
	{
		assert(false);
	}

	auto itrItem = m_mapItem.find(itrID->second);

	if(itrItem == m_mapItem.end())
	{
		assert(false);
	}

	m_mapItem.erase(itrItem);
	m_mapID.erase(itrID);
}

void CFavoritesTreeView::SelectFolder(const GUID &guid)
{
	auto itr = m_mapItem.find(guid);

	assert(itr != m_mapItem.end());

	TreeView_SelectItem(m_hTreeView,itr->second);
}

CFavoriteFolder &CFavoritesTreeView::GetFavoriteFolderFromTreeView(HTREEITEM hItem)
{
	TVITEM tvi;
	tvi.mask	= TVIF_HANDLE|TVIF_PARAM;
	tvi.hItem	= hItem;
	TreeView_GetItem(m_hTreeView,&tvi);

	std::stack<UINT> stackIDs;
	HTREEITEM hParent;
	HTREEITEM hCurrentItem = hItem;

	while((hParent = TreeView_GetParent(m_hTreeView,hCurrentItem)) != NULL)
	{
		TVITEM tvi;
		tvi.mask	= TVIF_HANDLE|TVIF_PARAM;
		tvi.hItem	= hCurrentItem;
		TreeView_GetItem(m_hTreeView,&tvi);

		stackIDs.push(static_cast<UINT>(tvi.lParam));

		hCurrentItem = hParent;
	}

	CFavoriteFolder *pFavoriteFolder = m_pAllFavorites;

	while(!stackIDs.empty())
	{
		UINT uID = stackIDs.top();
		auto itr = m_mapID.find(uID);

		NFavoritesHelper::variantFavorite_t variantFavorite = NFavoritesHelper::GetFavoriteItem(*pFavoriteFolder,itr->second);
		pFavoriteFolder = boost::get<CFavoriteFolder>(&variantFavorite);

		stackIDs.pop();
	}

	return *pFavoriteFolder;
}

CFavoritesListView::CFavoritesListView(HWND hListView) :
m_hListView(hListView),
m_uIDCounter(0)
{
	ListView_SetExtendedListViewStyleEx(hListView,
		LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT,
		LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT);

	m_himl = ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,0,48);
	HBITMAP hBitmap = LoadBitmap(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_SHELLIMAGES_2000));
	ImageList_Add(m_himl,hBitmap,NULL);
	ListView_SetImageList(hListView,m_himl,LVSIL_SMALL);
	DeleteObject(hBitmap);
}

CFavoritesListView::~CFavoritesListView()
{
	ImageList_Destroy(m_himl);
}

void CFavoritesListView::InsertFavoritesIntoListView(const CFavoriteFolder &FavoriteFolder)
{
	ListView_DeleteAllItems(m_hListView);
	m_uIDCounter = 0;
	m_mapID.clear();

	int iItem = 0;

	for(auto itr = FavoriteFolder.begin();itr != FavoriteFolder.end();++itr)
	{
		if(itr->type() == typeid(CFavoriteFolder))
		{
			const CFavoriteFolder &CurrentFavoriteFolder = boost::get<CFavoriteFolder>(*itr);
			InsertFavoriteFolderIntoListView(CurrentFavoriteFolder,iItem);
		}
		else
		{
			const CFavorite &CurrentFavorite = boost::get<CFavorite>(*itr);
			InsertFavoriteIntoListView(CurrentFavorite,iItem);
		}

		++iItem;
	}
}

int CFavoritesListView::InsertFavoriteFolderIntoListView(const CFavoriteFolder &FavoriteFolder)
{
	int nItems = ListView_GetItemCount(m_hListView);
	return InsertFavoriteItemIntoListView(FavoriteFolder.GetName(),
		FavoriteFolder.GetGUID(),true,nItems);
}

int CFavoritesListView::InsertFavoriteFolderIntoListView(const CFavoriteFolder &FavoriteFolder,int iPosition)
{
	return InsertFavoriteItemIntoListView(FavoriteFolder.GetName(),
		FavoriteFolder.GetGUID(),true,iPosition);
}

int CFavoritesListView::InsertFavoriteIntoListView(const CFavorite &Favorite)
{
	int nItems = ListView_GetItemCount(m_hListView);
	return InsertFavoriteItemIntoListView(Favorite.GetName(),
		Favorite.GetGUID(),false,nItems);
}

int CFavoritesListView::InsertFavoriteIntoListView(const CFavorite &Favorite,int iPosition)
{
	return InsertFavoriteItemIntoListView(Favorite.GetName(),
		Favorite.GetGUID(),false,iPosition);
}

int CFavoritesListView::InsertFavoriteItemIntoListView(const std::wstring &strName,
	const GUID &guid,bool bFolder,int iPosition)
{
	TCHAR szName[256];
	StringCchCopy(szName,SIZEOF_ARRAY(szName),strName.c_str());

	int iImage;

	if(bFolder)
	{
		iImage = SHELLIMAGES_NEWTAB;
	}
	else
	{
		iImage = SHELLIMAGES_FAV;
	}

	LVITEM lvi;
	lvi.mask		= LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
	lvi.iItem		= iPosition;
	lvi.iSubItem	= 0;
	lvi.iImage		= iImage;
	lvi.pszText		= szName;
	lvi.lParam		= m_uIDCounter;
	int iItem = ListView_InsertItem(m_hListView,&lvi);

	m_mapID.insert(std::make_pair<UINT,GUID>(m_uIDCounter,guid));
	++m_uIDCounter;

	return iItem;
}

NFavoritesHelper::variantFavorite_t CFavoritesListView::GetFavoriteItemFromListView(CFavoriteFolder &ParentFavoriteFolder,int iItem)
{
	LVITEM lvi;
	lvi.mask		= LVIF_PARAM;
	lvi.iItem		= iItem;
	lvi.iSubItem	= 0;
	ListView_GetItem(m_hListView,&lvi);

	NFavoritesHelper::variantFavorite_t variantFavorite = GetFavoriteItemFromListViewlParam(ParentFavoriteFolder,lvi.lParam);

	return variantFavorite;
}

NFavoritesHelper::variantFavorite_t CFavoritesListView::GetFavoriteItemFromListViewlParam(CFavoriteFolder &ParentFavoriteFolder,LPARAM lParam)
{
	auto itr = m_mapID.find(static_cast<UINT>(lParam));
	NFavoritesHelper::variantFavorite_t variantFavorite = NFavoritesHelper::GetFavoriteItem(ParentFavoriteFolder,itr->second);

	return variantFavorite;
}

NFavoritesHelper::variantFavorite_t NFavoritesHelper::GetFavoriteItem(CFavoriteFolder &ParentFavoriteFolder,
	const GUID &guid)
{
	auto itr = std::find_if(ParentFavoriteFolder.begin(),ParentFavoriteFolder.end(),
		[guid](boost::variant<CFavoriteFolder,CFavorite> &variantFavorite) -> BOOL
		{
			if(variantFavorite.type() == typeid(CFavoriteFolder))
			{
				CFavoriteFolder FavoriteFolder = boost::get<CFavoriteFolder>(variantFavorite);
				return IsEqualGUID(FavoriteFolder.GetGUID(),guid);
			}
			else
			{
				CFavorite Favorite = boost::get<CFavorite>(variantFavorite);
				return IsEqualGUID(Favorite.GetGUID(),guid);
			}
		}
	);

	assert(itr != ParentFavoriteFolder.end());

	if(itr->type() == typeid(CFavoriteFolder))
	{
		CFavoriteFolder &FavoriteFolder = boost::get<CFavoriteFolder>(*itr);
		return FavoriteFolder;
	}
	else
	{
		CFavorite &Favorite = boost::get<CFavorite>(*itr);
		return Favorite;
	}
}

int CALLBACK NFavoritesHelper::SortByName(const variantFavorite_t FavoriteItem1,
	const variantFavorite_t FavoriteItem2)
{
	if(FavoriteItem1.type() == typeid(CFavoriteFolder) &&
		FavoriteItem2.type() == typeid(CFavoriteFolder))
	{
		const CFavoriteFolder &FavoriteFolder1 = boost::get<CFavoriteFolder>(FavoriteItem1);
		const CFavoriteFolder &FavoriteFolder2 = boost::get<CFavoriteFolder>(FavoriteItem2);

		return FavoriteFolder1.GetName().compare(FavoriteFolder2.GetName());
	}
	else if(FavoriteItem1.type() == typeid(CFavoriteFolder) &&
		FavoriteItem2.type() == typeid(CFavorite))
	{
		return -1;
	}
	else if(FavoriteItem1.type() == typeid(CFavorite) &&
		FavoriteItem2.type() == typeid(CFavoriteFolder))
	{
		return 1;
	}
	else
	{
		const CFavorite &Favorite1 = boost::get<CFavorite>(FavoriteItem1);
		const CFavorite &Favorite2 = boost::get<CFavorite>(FavoriteItem1);

		return Favorite1.GetName().compare(Favorite2.GetName());
	}
}

int CALLBACK NFavoritesHelper::SortByLocation(const variantFavorite_t FavoriteItem1,
	const variantFavorite_t FavoriteItem2)
{
	if(FavoriteItem1.type() == typeid(CFavoriteFolder) &&
		FavoriteItem2.type() == typeid(CFavoriteFolder))
	{
		return 0;
	}
	else if(FavoriteItem1.type() == typeid(CFavoriteFolder) &&
		FavoriteItem2.type() == typeid(CFavorite))
	{
		return -1;
	}
	else if(FavoriteItem1.type() == typeid(CFavorite) &&
		FavoriteItem2.type() == typeid(CFavoriteFolder))
	{
		return 1;
	}
	else
	{
		const CFavorite &Favorite1 = boost::get<CFavorite>(FavoriteItem1);
		const CFavorite &Favorite2 = boost::get<CFavorite>(FavoriteItem1);

		return Favorite1.GetLocation().compare(Favorite2.GetLocation());
	}
}

int CALLBACK NFavoritesHelper::SortByVisitDate(const variantFavorite_t FavoriteItem1,
	const variantFavorite_t FavoriteItem2)
{
	if(FavoriteItem1.type() == typeid(CFavoriteFolder) &&
		FavoriteItem2.type() == typeid(CFavoriteFolder))
	{
		return 0;
	}
	else if(FavoriteItem1.type() == typeid(CFavoriteFolder) &&
		FavoriteItem2.type() == typeid(CFavorite))
	{
		return -1;
	}
	else if(FavoriteItem1.type() == typeid(CFavorite) &&
		FavoriteItem2.type() == typeid(CFavoriteFolder))
	{
		return 1;
	}
	else
	{
		const CFavorite &Favorite1 = boost::get<CFavorite>(FavoriteItem1);
		const CFavorite &Favorite2 = boost::get<CFavorite>(FavoriteItem1);

		FILETIME ft1 = Favorite1.GetDateLastVisited();
		FILETIME ft2 = Favorite2.GetDateLastVisited();

		return CompareFileTime(&ft1,&ft2);
	}
}

int CALLBACK NFavoritesHelper::SortByVisitCount(const variantFavorite_t FavoriteItem1,
	const variantFavorite_t FavoriteItem2)
{
	if(FavoriteItem1.type() == typeid(CFavoriteFolder) &&
		FavoriteItem2.type() == typeid(CFavoriteFolder))
	{
		return 0;
	}
	else if(FavoriteItem1.type() == typeid(CFavoriteFolder) &&
		FavoriteItem2.type() == typeid(CFavorite))
	{
		return -1;
	}
	else if(FavoriteItem1.type() == typeid(CFavorite) &&
		FavoriteItem2.type() == typeid(CFavoriteFolder))
	{
		return 1;
	}
	else
	{
		const CFavorite &Favorite1 = boost::get<CFavorite>(FavoriteItem1);
		const CFavorite &Favorite2 = boost::get<CFavorite>(FavoriteItem1);

		return Favorite1.GetVisitCount() - Favorite2.GetVisitCount();
	}
}

int CALLBACK NFavoritesHelper::SortByAdded(const variantFavorite_t FavoriteItem1,
	const variantFavorite_t FavoriteItem2)
{
	if(FavoriteItem1.type() == typeid(CFavoriteFolder) &&
		FavoriteItem2.type() == typeid(CFavoriteFolder))
	{
		const CFavoriteFolder &FavoriteFolder1 = boost::get<CFavoriteFolder>(FavoriteItem1);
		const CFavoriteFolder &FavoriteFolder2 = boost::get<CFavoriteFolder>(FavoriteItem2);

		FILETIME ft1 = FavoriteFolder1.GetDateCreated();
		FILETIME ft2 = FavoriteFolder2.GetDateCreated();

		return CompareFileTime(&ft1,&ft2);
	}
	else if(FavoriteItem1.type() == typeid(CFavoriteFolder) &&
		FavoriteItem2.type() == typeid(CFavorite))
	{
		return -1;
	}
	else if(FavoriteItem1.type() == typeid(CFavorite) &&
		FavoriteItem2.type() == typeid(CFavoriteFolder))
	{
		return 1;
	}
	else
	{
		const CFavorite &Favorite1 = boost::get<CFavorite>(FavoriteItem1);
		const CFavorite &Favorite2 = boost::get<CFavorite>(FavoriteItem1);

		FILETIME ft1 = Favorite1.GetDateCreated();
		FILETIME ft2 = Favorite2.GetDateCreated();

		return CompareFileTime(&ft1,&ft2);
	}
}

int CALLBACK NFavoritesHelper::SortByLastModified(const variantFavorite_t FavoriteItem1,
	const variantFavorite_t FavoriteItem2)
{
	if(FavoriteItem1.type() == typeid(CFavoriteFolder) &&
		FavoriteItem2.type() == typeid(CFavoriteFolder))
	{
		const CFavoriteFolder &FavoriteFolder1 = boost::get<CFavoriteFolder>(FavoriteItem1);
		const CFavoriteFolder &FavoriteFolder2 = boost::get<CFavoriteFolder>(FavoriteItem2);

		FILETIME ft1 = FavoriteFolder1.GetDateModified();
		FILETIME ft2 = FavoriteFolder2.GetDateModified();

		return CompareFileTime(&ft1,&ft2);
	}
	else if(FavoriteItem1.type() == typeid(CFavoriteFolder) &&
		FavoriteItem2.type() == typeid(CFavorite))
	{
		return -1;
	}
	else if(FavoriteItem1.type() == typeid(CFavorite) &&
		FavoriteItem2.type() == typeid(CFavoriteFolder))
	{
		return 1;
	}
	else
	{
		const CFavorite &Favorite1 = boost::get<CFavorite>(FavoriteItem1);
		const CFavorite &Favorite2 = boost::get<CFavorite>(FavoriteItem1);

		FILETIME ft1 = Favorite1.GetDateModified();
		FILETIME ft2 = Favorite2.GetDateModified();

		return CompareFileTime(&ft1,&ft2);
	}
}

CIPFavoriteItemNotifier::CIPFavoriteItemNotifier(HWND hTopLevelWnd) :
m_hTopLevelWnd(hTopLevelWnd)
{

}

CIPFavoriteItemNotifier::~CIPFavoriteItemNotifier()
{

}

BOOL CALLBACK FavoriteNotifierEnumWindowsStub(HWND hwnd,LPARAM lParam)
{
	CIPFavoriteItemNotifier *pipbn = reinterpret_cast<CIPFavoriteItemNotifier *>(lParam);

	return pipbn->FavoriteNotifierEnumWindows(hwnd);
}

BOOL CALLBACK CIPFavoriteItemNotifier::FavoriteNotifierEnumWindows(HWND hwnd)
{
	TCHAR szClassName[256];
	int iRes = GetClassName(hwnd,szClassName,SIZEOF_ARRAY(szClassName));

	if(iRes != 0 &&
		lstrcmp(szClassName,SaltedExplorer::CLASS_NAME) == 0 &&
		hwnd != m_hTopLevelWnd)
	{
		SaltedExplorer::IPFavoriteNotification_t ipbn;
		ipbn.Type = SaltedExplorer::IP_NOTIFICATION_TYPE_FAVORITE_MODIFIED;

		COPYDATASTRUCT cds;
		cds.lpData = reinterpret_cast<PVOID>(&ipbn);
		cds.cbData = sizeof(ipbn);
		cds.dwData = NULL;
		SendMessage(hwnd,WM_COPYDATA,reinterpret_cast<WPARAM>(m_hTopLevelWnd),reinterpret_cast<LPARAM>(&cds));
	}

	return TRUE;
}

void CIPFavoriteItemNotifier::OnFavoriteItemModified(const GUID &guid)
{
	EnumWindows(FavoriteNotifierEnumWindowsStub,reinterpret_cast<LPARAM>(this));
}

void CIPFavoriteItemNotifier::OnFavoriteAdded(const CFavoriteFolder &ParentFavoriteFolder,const CFavorite &Favorite)
{

}

void CIPFavoriteItemNotifier::OnFavoriteFolderAdded(const CFavoriteFolder &ParentFavoriteFolder,const CFavoriteFolder &FavoriteFolder)
{

}

void CIPFavoriteItemNotifier::OnFavoriteRemoved(const GUID &guid)
{

}

void CIPFavoriteItemNotifier::OnFavoriteFolderRemoved(const GUID &guid)
{

}

CIPFavoriteItemNotifier::CIPFavoriteObserver()
{

}

CIPFavoriteItemNotifier::~CIPFavoriteObserver()
{

}

void CIPFavoriteItemNotifier::OnFavoriteItemModified(const GUID &guid)
{
	/* Find the bookmark with the specified GUID, and update its
	properties. This will need to update *each* of the bookmarks
	properties - i.e.. if a new public property is added, that property
	will need to be updated here.
	This may also need to update internal properties (i.e. visitor
	count, last visit date).
	Need to clone exact state of modified favorite, and broadcast
	a modification notification. */
}