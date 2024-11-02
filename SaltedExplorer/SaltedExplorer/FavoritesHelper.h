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
	typedef boost::variant<CFavoriteFolder &,CFavorite &> variantFavorite_t;

	enum SortMode_t
	{
		SM_NAME = 1,
		SM_LOCATION = 2,
		SM_VISIT_DATE = 3,
		SM_VISIT_COUNT = 4,
		SM_ADDED = 5,
		SM_LAST_MODIFIED = 6
	};

	int CALLBACK		SortByName(const variantFavorite_t FavoriteItem1,const variantFavorite_t FavoriteItem2);
	int CALLBACK		SortByLocation(const variantFavorite_t FavoriteItem1,const variantFavorite_t FavoriteItem2);
	int CALLBACK		SortByVisitDate(const variantFavorite_t FavoriteItem1,const variantFavorite_t FavoriteItem2);
	int CALLBACK		SortByVisitCount(const variantFavorite_t FavoriteItem1,const variantFavorite_t FavoriteItem2);
	int CALLBACK		SortByAdded(const variantFavorite_t FavoriteItem1,const variantFavorite_t FavoriteItem2);
	int CALLBACK		SortByLastModified(const variantFavorite_t FavoriteItem1,const variantFavorite_t FavoriteItem2);

	variantFavorite_t	GetFavoriteItem(CFavoriteFolder &ParentFavoriteFolder,const GUID &guid);
}

class CFavoritesTreeView
{
	friend LRESULT CALLBACK FavoritesTreeViewProcStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);

public:

	CFavoritesTreeView(HWND hTreeView,CFavoriteFolder *pAllFavorites,const GUID &guidSelected,const NFavoritesHelper::setExpansion_t &setExpansion);
	~CFavoritesTreeView();

	CFavoriteFolder		&GetFavoriteFolderFromTreeView(HTREEITEM hItem);

	void				FavoriteFolderAdded(const CFavoriteFolder &ParentFavoriteFolder,const CFavoriteFolder &FavoriteFolder);
	void				FavoriteFolderModified(const GUID &guid);

	void				SelectFolder(const GUID &guid);

private:

	typedef std::unordered_map<GUID,HTREEITEM,NFavoritesHelper::GuidHash,NFavoritesHelper::GuidEq> ItemMap_t;

	LRESULT CALLBACK	TreeViewProc(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam);

	void				SetupTreeView(const GUID &guidSelected,const NFavoritesHelper::setExpansion_t &setExpansion);

	HTREEITEM			InsertFolderIntoTreeView(HTREEITEM hParent,const CFavoriteFolder &FavoriteFolder);
	void				InsertFoldersIntoTreeViewRecursive(HTREEITEM hParent,const CFavoriteFolder &FavoriteFolder);

	void				OnTvnDeleteItem(NMTREEVIEW *pnmtv);

	HWND							m_hTreeView;
	HIMAGELIST						m_himl;

	CFavoriteFolder					*m_pAllFavorites;

	std::unordered_map<UINT,GUID>	m_mapID;
	ItemMap_t						m_mapItem;
	UINT							m_uIDCounter;
};

class CFavoritesListView
{
public:
	CFavoritesListView(HWND hListView);
	~CFavoritesListView();

	void	InsertFavoritesIntoListView(const CFavoriteFolder &FavoriteFolder);
	int		InsertFavoriteFolderIntoListView(const CFavoriteFolder &FavoriteFolder);
	int		InsertFavoriteFolderIntoListView(const CFavoriteFolder &FavoriteFolder,int iPosition);
	int		InsertFavoriteIntoListView(const CFavorite &Favorite);
	int		InsertFavoriteIntoListView(const CFavorite &Favorite,int iPosition);
	NFavoritesHelper::variantFavorite_t	GetFavoriteItemFromListView(CFavoriteFolder &ParentFavoriteFolder,int iItem);
	NFavoritesHelper::variantFavorite_t	GetFavoriteItemFromListViewlParam(CFavoriteFolder &ParentFavoriteFolder,LPARAM lParam);

private:

	int		InsertFavoriteItemIntoListView(const std::wstring &strName,const GUID &guid,bool bFolder,int iPosition);

	HWND							m_hListView;
	HIMAGELIST						m_himl;

	std::unordered_map<UINT,GUID>	m_mapID;
	UINT							m_uIDCounter;
};

/* Receives low-level favorites notifications, and rebroadcasts them
via IPC to other SaltedExplorer processes. */
class CIPFavoriteItemNotifier : public NFavorite::IFavoriteItemNotification
{
	friend BOOL CALLBACK FavoriteNotifierEnumWindowsStub(HWND hwnd,LPARAM lParam);

public:

	CIPFavoriteItemNotifier(HWND hTopLevelWnd);
	~CIPFavoriteItemNotifier();

	void	OnFavoriteItemModified(const GUID &guid);
	void	OnFavoriteAdded(const CFavoriteFolder &ParentFavoriteFolder,const CFavorite &Favorite);
	void	OnFavoriteFolderAdded(const CFavoriteFolder &ParentFavoriteFolder,const CFavoriteFolder &FavoriteFolder);
	void	OnFavoriteRemoved(const GUID &guid);
	void	OnFavoriteFolderRemoved(const GUID &guid);

private:

	BOOL CALLBACK	BookmarkNotifierEnumWindows(HWND hwnd);

	HWND	m_hTopLevelWnd;
};

/* Receives favorites notifications via IPC from other SaltedExplorer process,
and rebroadcasts those notifications internally.
This class will have to emulate all favorite notifications. That is, upon
receiving a modification, addition, etc notification, this class will
have to reconstruct the changes locally. This will then cause the changes
to be rebroadcast internally.
While reconstructing the changes, this class will have to set a flag indicating
that the changes are not to rebroadcast. */
class CIPFavoriteObserver
{
public:

	CIPFavoriteObserver();
	~CIPFavoriteObserver();

	void	OnFavoriteItemModified(const GUID &guid);


private:
};

#endif