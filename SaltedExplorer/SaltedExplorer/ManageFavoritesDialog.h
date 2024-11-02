#ifndef MANAGEFAVORITESSDIALOG_INCLUDED
#define MANAGEFAVORITESSDIALOG_INCLUDED

#include "FavoritesHelper.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/ResizableDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/Favorites.h"

class CManageFavoritesDialog;

class CManageFavoritesDialogPersistentSettings : public CDialogSettings
{
public:

	~CManageFavoritesDialogPersistentSettings();

	static CManageFavoritesDialogPersistentSettings &GetInstance();

private:

	friend CManageFavoritesDialog;

	enum ColumnType_t
	{
		COLUMN_TYPE_NAME = 1,
		COLUMN_TYPE_LOCATION = 2,
		COLUMN_TYPE_VISIT_DATE = 3,
		COLUMN_TYPE_VISIT_COUNT = 4,
		COLUMN_TYPE_ADDED = 5,
		COLUMN_TYPE_LAST_MODIFIED = 6
	};

	struct ColumnInfo_t
	{
		ColumnType_t	ColumnType;
		int				iWidth;
		bool			bActive;
	};

	static const TCHAR SETTINGS_KEY[];
	static const int DEFAULT_MANAGE_FAVORITES_COLUMN_WIDTH = 180;

	CManageFavoritesDialogPersistentSettings();

	CManageFavoritesDialogPersistentSettings(const CManageFavoritesDialogPersistentSettings &);
	CManageFavoritesDialogPersistentSettings & operator=(const CManageFavoritesDialogPersistentSettings &);

	void SetupDefaultColumns();

	std::vector<ColumnInfo_t>		m_vectorColumnInfo;

	bool							m_bInitialized;
	GUID							m_guidSelected;
	NFavoritesHelper::setExpansion_t	m_setExpansion;

	NFavoritesHelper::SortMode_t	m_SortMode;
	bool							m_bSortAscending;
};

class CManageFavoritesDialog : public CBaseDialog, public NFavorite::IFavoriteItemNotification
{
public:

	CManageFavoritesDialog(HINSTANCE hInstance,int iResource,HWND hParent,CFavoriteFolder &AllFavorites);
	~CManageFavoritesDialog();

	int CALLBACK		SortFavorites(LPARAM lParam1,LPARAM lParam2);
	LRESULT CALLBACK	EditSearchProc(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam);

	void	OnFavoriteItemModified(const GUID &guid);
	void	OnFavoriteAdded(const CFavoriteFolder &ParentFavoriteFolder,const CFavorite &Favorite);
	void	OnFavoriteFolderAdded(const CFavoriteFolder &ParentFavoriteFolder,const CFavoriteFolder &FavoriteFolder);
	void	OnFavoriteRemoved(const GUID &guid);
	void	OnFavoriteFolderRemoved(const GUID &guid);

protected:

	BOOL	OnInitDialog();
	INT_PTR	OnCtlColorEdit(HWND hwnd,HDC hdc);
	BOOL	OnAppCommand(HWND hwnd,UINT uCmd,UINT uDevice,DWORD dwKeys);
	BOOL	OnCommand(WPARAM wParam,LPARAM lParam);
	BOOL	OnNotify(NMHDR *pnmhdr);
	BOOL	OnClose();
	BOOL	OnDestroy();
	BOOL	OnNcDestroy();

	void	SaveState();

private:


	static const COLORREF SEARCH_TEXT_COLOR = RGB(120,120,120);

	static const int TOOLBAR_ID_BACK			= 10000;
	static const int TOOLBAR_ID_FORWARD			= 10001;
	static const int TOOLBAR_ID_ORGANIZE		= 10002;
	static const int TOOLBAR_ID_VIEWS			= 10003;
	static const int TOOLBAR_ID_IMPORTEXPORT	= 10004;

	CManageFavoritesDialog & operator = (const CManageFavoritesDialog &mbd);

	void		SetupSearchField();
	void		SetupToolbar();
	void		SetupTreeView();
	void		SetupListView();

	void		SortListViewItems(NFavoritesHelper::SortMode_t SortMode);

	void		GetColumnString(CManageFavoritesDialogPersistentSettings::ColumnType_t ColumnType,TCHAR *szColumn,UINT cchBuf);
	void		GetFavoriteItemColumnInfo(const NFavoritesHelper::variantFavorite_t variantFavorite,CManageFavoritesDialogPersistentSettings::ColumnType_t ColumnType,TCHAR *szColumn,size_t cchBuf);
	void		GetFavoriteColumnInfo(const CFavorite &Favorite,CManageFavoritesDialogPersistentSettings::ColumnType_t ColumnType,TCHAR *szColumn,size_t cchBuf);
	void		GetFavoriteFolderColumnInfo(const CFavoriteFolder &FavoriteFolder,CManageFavoritesDialogPersistentSettings::ColumnType_t ColumnType,TCHAR *szColumn,size_t cchBuf);

	void		SetSearchFieldDefaultState();
	void		RemoveSearchFieldDefaultState();

	void		BrowseBack();
	void		BrowseForward();
	void		BrowseFavoriteFolder(const CFavoriteFolder &FavoriteFolder);

	void		UpdateToolbarState();

	void		OnNewFolder();

	void		OnEnChange(HWND hEdit);
	void		OnDblClk(NMHDR *pnmhdr);
	void		OnRClick(NMHDR *pnmhdr);
	void		OnTbnDropDown(NMTOOLBAR *nmtb);
	void		ShowViewMenu();
	void		ShowOrganizeMenu();
	void		OnTvnSelChanged(NMTREEVIEW *pnmtv);
	void		OnListViewRClick();
	void		OnListViewHeaderRClick();
	BOOL		OnLvnEndLabelEdit(NMLVDISPINFO *pnmlvdi);
	void		OnLvnKeyDown(NMLVKEYDOWN *pnmlvkd);
	void		OnListViewRename();

	void		OnOk();
	void		OnCancel();

	HWND						m_hToolbar;
	HIMAGELIST					m_himlToolbar;

	CFavoriteFolder				&m_AllFavorites;

	GUID						m_guidCurrentFolder;

	bool						m_bNewFolderAdded;
	GUID						m_guidNewFolder;

	std::stack<GUID>			m_stackBack;
	std::stack<GUID>			m_stackForward;

	CFavoritesTreeView			*m_pFavoritesTreeView;

	bool						m_bListViewInitialized;
	CFavoritesListView			*m_pFavoritesListView;

	HFONT						m_hEditSearchFont;
	bool						m_bSearchFieldBlank;
	bool						m_bEditingSearchField;

	CManageFavoritesDialogPersistentSettings	*m_pmbdps;
};

#endif