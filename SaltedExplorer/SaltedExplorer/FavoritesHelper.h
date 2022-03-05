#ifndef FAVORITESHELPER_INCLUDED
#define FAVORITESHELPER_INCLUDED

#include <unordered_set>
#include <unordered_map>
#include "../Helper/Favorites.h"

namespace NFavoritesHelper
{
	struct GuidEq
	{
		bool operator () (const GUID &guid1,const GUID &guid2) const
		{
			return (IsEqualGUID(guid1,guid2) == TRUE);
		}
	};

	struct GuidHash
	{
		size_t operator () (const GUID &guid) const
		{
			return guid.Data1;
		}
	};

	typedef std::unordered_set<GUID,GuidHash,GuidEq> setExpansion_t;
	typedef boost::variant<FavoriteFolder &,Favorite &> variantFavorite_t;

	enum SortMode_t
	{
		SM_NAME = 1
	};

	int CALLBACK		SortByName(const variantFavorite_t FavoriteItem1,const variantFavorite_t FavoriteItem2);
	variantFavorite_t	GetFavoriteItem(FavoriteFolder &ParentFavoriteFolder,const GUID &guid);
}

class FavoritesTreeView
{
public:

	FavoritesTreeView(HWND hTreeView);
	~FavoritesTreeView();

	void			InsertFoldersIntoTreeView(FavoriteFolder *pFavoriteFolder,const GUID &guidSelected,const NFavoritesHelper::setExpansion_t &setExpansion);
	HTREEITEM		InsertFolderIntoTreeView(HTREEITEM hParent,FavoriteFolder *pFavoriteFolder,const GUID &guidSelected,const NFavoritesHelper::setExpansion_t &setExpansion);
	FavoriteFolder	*GetFavoriteFolderFromTreeView(HTREEITEM hItem,FavoriteFolder *pRootFavoriteFolder);

private:

	void	InsertFoldersIntoTreeViewRecursive(HTREEITEM hParent,FavoriteFolder *pFavoriteFolder,const GUID &guidSelected,const NFavoritesHelper::setExpansion_t &setExpansion);

	HWND							m_hTreeView;
	HIMAGELIST						m_himl;

	std::unordered_map<UINT,GUID>	m_mapID;
	UINT							m_uIDCounter;
};

class FavoritesListView
{
public:
	FavoritesListView(HWND hListView);
	~FavoritesListView();

	void	InsertFavoritesIntoListView(FavoriteFolder *pFavoriteFolder);
	void	InsertFavoriteFolderIntoListView(FavoriteFolder *pFavoriteFolder,int iPosition);
	void	InsertFavoriteIntoListView(Favorite *pFavorite,int iPosition);
	NFavoritesHelper::variantFavorite_t	GetFavoriteItemFromListView(int iItem);

private:

	void	InsertFavoriteItemIntoListView(const std::wstring &strName,const GUID &guid,int iPosition);

	HWND							m_hListView;
	HIMAGELIST						m_himl;

	FavoriteFolder					*m_pParentFavoriteFolder;

	std::unordered_map<UINT,GUID>	m_mapID;
	UINT							m_uIDCounter;
};

#endif