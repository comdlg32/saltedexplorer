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

FavoritesTreeView::FavoritesTreeView(HWND hTreeView) :
	m_hTreeView(hTreeView),
	m_uIDCounter(0)
{
	m_himl = ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,0,48);
	HBITMAP hBitmap = LoadBitmap(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_SHELLIMAGES_2000));
	ImageList_Add(m_himl,hBitmap,NULL);
	TreeView_SetImageList(hTreeView,m_himl,TVSIL_NORMAL);
	DeleteObject(hBitmap);

	TreeView_DeleteAllItems(hTreeView);
}

FavoritesTreeView::~FavoritesTreeView()
{
	ImageList_Destroy(m_himl);
}

void FavoritesTreeView::InsertFoldersIntoTreeView(FavoriteFolder *pFavoriteFolder,
	const GUID &guidSelected,const NFavoritesHelper::setExpansion_t &setExpansion)
{
	HTREEITEM hRoot = InsertFolderIntoTreeView(NULL,pFavoriteFolder,
		guidSelected,setExpansion);

	InsertFoldersIntoTreeViewRecursive(hRoot,pFavoriteFolder,
		guidSelected,setExpansion);
}

void FavoritesTreeView::InsertFoldersIntoTreeViewRecursive(HTREEITEM hParent,
	FavoriteFolder *pFavoriteFolder,const GUID &guidSelected,const NFavoritesHelper::setExpansion_t &setExpansion)
{
	for(auto itr = pFavoriteFolder->begin();itr != pFavoriteFolder->end();++itr)
	{
		if(FavoriteFolder *pFavoriteFolderChild = boost::get<FavoriteFolder>(&(*itr)))
		{
			HTREEITEM hCurrentItem = InsertFolderIntoTreeView(hParent,
				pFavoriteFolderChild,guidSelected,setExpansion);

			if(pFavoriteFolderChild->HasChildFolder())
			{
				InsertFoldersIntoTreeViewRecursive(hCurrentItem,
					pFavoriteFolderChild,guidSelected,setExpansion);
			}
		}
	}
}

HTREEITEM FavoritesTreeView::InsertFolderIntoTreeView(HTREEITEM hParent,
	FavoriteFolder *pFavoriteFolder,const GUID &guidSelected,const NFavoritesHelper::setExpansion_t &setExpansion)
{
	TCHAR szText[256];
	StringCchCopy(szText,SIZEOF_ARRAY(szText),pFavoriteFolder->GetName().c_str());

	int nChildren = 0;

	if(pFavoriteFolder->HasChildFolder())
	{
		nChildren = 1;
	}

	UINT uState = 0;
	UINT uStateMask = 0;

	auto itr = setExpansion.find(pFavoriteFolder->GetGUID());

	if(itr != setExpansion.end() &&
		pFavoriteFolder->HasChildFolder())
	{
		uState		|= TVIS_EXPANDED;
		uStateMask	|= TVIS_EXPANDED;
	}

	if(IsEqualGUID(pFavoriteFolder->GetGUID(),guidSelected))
	{
		uState		|= TVIS_SELECTED;
		uStateMask	|= TVIS_SELECTED;
	}

	TVITEMEX tviex;
	tviex.mask				= TVIF_TEXT|TVIF_IMAGE|TVIF_CHILDREN|TVIF_SELECTEDIMAGE|TVIF_PARAM|TVIF_STATE;
	tviex.pszText			= szText;
	tviex.iImage			= SHELLIMAGES_NEWTAB;
	tviex.iSelectedImage	= SHELLIMAGES_NEWTAB;
	tviex.cChildren			= nChildren;
	tviex.lParam			= m_uIDCounter;
	tviex.state				= uState;
	tviex.stateMask			= uStateMask;

	TVINSERTSTRUCT tvis;
	tvis.hParent			= hParent;
	tvis.hInsertAfter		= TVI_LAST;
	tvis.itemex				= tviex;
	HTREEITEM hItem = TreeView_InsertItem(m_hTreeView,&tvis);

	m_mapID.insert(std::make_pair<UINT,GUID>(m_uIDCounter,pFavoriteFolder->GetGUID()));
	++m_uIDCounter;

	return hItem;
}

FavoriteFolder *FavoritesTreeView::GetFavoriteFolderFromTreeView(HTREEITEM hItem,
	FavoriteFolder *pRootFavoriteFolder)
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

	FavoriteFolder *pFavoriteFolder = pRootFavoriteFolder;

	while(!stackIDs.empty())
	{
		UINT uID = stackIDs.top();
		auto itr = m_mapID.find(uID);

		NFavoritesHelper::variantFavorite_t variantFavorite = NFavoritesHelper::GetFavoriteItem(*pRootFavoriteFolder,itr->second);
		pFavoriteFolder = boost::get<FavoriteFolder>(&variantFavorite);

		stackIDs.pop();
	}

	return pFavoriteFolder;
}

FavoritesListView::FavoritesListView(HWND hListView) :
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

FavoritesListView::~FavoritesListView()
{
	ImageList_Destroy(m_himl);
}

void FavoritesListView::InsertFavoritesIntoListView(FavoriteFolder *pFavoriteFolder)
{
	m_pParentFavoriteFolder = pFavoriteFolder;

	ListView_DeleteAllItems(m_hListView);
	m_uIDCounter = 0;
	m_mapID.clear();

	int iItem = 0;

	for(auto itr = pFavoriteFolder->begin();itr != pFavoriteFolder->end();++itr)
	{
		if(FavoriteFolder *pFavoriteFolder = boost::get<FavoriteFolder>(&(*itr)))
		{
			InsertFavoriteFolderIntoListView(pFavoriteFolder,iItem);
		}
		else if(Favorite *pFavorite = boost::get<Favorite>(&(*itr)))
		{
			InsertFavoriteIntoListView(pFavorite,iItem);
		}

		++iItem;
	}
}

void FavoritesListView::InsertFavoriteFolderIntoListView(FavoriteFolder *pFavoriteFolder,int iPosition)
{
	InsertFavoriteItemIntoListView(pFavoriteFolder->GetName(),
		pFavoriteFolder->GetGUID(),iPosition);
}

void FavoritesListView::InsertFavoriteIntoListView(Favorite *pFavorite,int iPosition)
{
	InsertFavoriteItemIntoListView(pFavorite->GetName(),
		pFavorite->GetGUID(),iPosition);
}

void FavoritesListView::InsertFavoriteItemIntoListView(const std::wstring &strName,
	const GUID &guid,int iPosition)
{
	TCHAR szName[256];
	StringCchCopy(szName,SIZEOF_ARRAY(szName),strName.c_str());

	LVITEM lvi;
	lvi.mask		= LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
	lvi.iItem		= iPosition;
	lvi.iSubItem	= 0;
	lvi.iImage		= SHELLIMAGES_NEWTAB;
	lvi.pszText		= szName;
	lvi.lParam		= m_uIDCounter;
	ListView_InsertItem(m_hListView,&lvi);

	m_mapID.insert(std::make_pair<UINT,GUID>(m_uIDCounter,guid));
	++m_uIDCounter;
}

NFavoritesHelper::variantFavorite_t FavoritesListView::GetFavoriteItemFromListView(int iItem)
{
	LVITEM lvi;
	lvi.mask		= LVIF_PARAM;
	lvi.iItem		= iItem;
	lvi.iSubItem	= 0;
	ListView_GetItem(m_hListView,&lvi);

	auto itr = m_mapID.find(static_cast<UINT>(lvi.lParam));

	NFavoritesHelper::variantFavorite_t variantFavorite = NFavoritesHelper::GetFavoriteItem(*m_pParentFavoriteFolder,itr->second);

	return variantFavorite;
}

NFavoritesHelper::variantFavorite_t NFavoritesHelper::GetFavoriteItem(FavoriteFolder &ParentFavoriteFolder,
	const GUID &guid)
{
	auto itr = std::find_if(ParentFavoriteFolder.begin(),ParentFavoriteFolder.end(),
		[guid](boost::variant<FavoriteFolder,Favorite> &variantFavorite) -> BOOL
		{
			if(variantFavorite.type() == typeid(FavoriteFolder))
			{
				FavoriteFolder LFavoriteFolder = boost::get<FavoriteFolder>(variantFavorite);
				return IsEqualGUID(LFavoriteFolder.GetGUID(),guid);
			}
			else
			{
				Favorite LFavorite = boost::get<Favorite>(variantFavorite);
				return IsEqualGUID(LFavorite.GetGUID(),guid);
			}
		}
	);

	if(itr == ParentFavoriteFolder.end())
	{
		assert(false);
	}
	if(itr->type() == typeid(FavoriteFolder))
	{
		FavoriteFolder &LFavoriteFolder = boost::get<FavoriteFolder>(*itr);
		return LFavoriteFolder;
	}
	else
	{
		Favorite &LFavorite = boost::get<Favorite>(*itr);
		return LFavorite;
	}
}

int CALLBACK NFavoritesHelper::SortByName(const variantFavorite_t FavoriteItem1,
	const variantFavorite_t FavoriteItem2)
{
	if(FavoriteItem1.type() == typeid(FavoriteFolder) &&
		FavoriteItem2.type() == typeid(FavoriteFolder))
	{
		const FavoriteFolder &FavoriteFolder1 = boost::get<FavoriteFolder>(FavoriteItem1);
		const FavoriteFolder &FavoriteFolder2 = boost::get<FavoriteFolder>(FavoriteItem2);

		return FavoriteFolder1.GetName().compare(FavoriteFolder2.GetName());
	}
	else if(FavoriteItem1.type() == typeid(FavoriteFolder) &&
		FavoriteItem2.type() == typeid(Favorite))
	{
		return -1;
	}
	else if(FavoriteItem1.type() == typeid(Favorite) &&
		FavoriteItem2.type() == typeid(FavoriteFolder))
	{
		return 1;
	}
	else
	{
		const Favorite &Favorite1 = boost::get<Favorite>(FavoriteItem1);
		const Favorite &Favorite2 = boost::get<Favorite>(FavoriteItem1);

		return Favorite1.GetName().compare(Favorite2.GetName());
	}
}