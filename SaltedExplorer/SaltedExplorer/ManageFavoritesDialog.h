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
};

class CManageFavoritesDialog : public CBaseDialog
{
public:

	CManageFavoritesDialog(HINSTANCE hInstance,int iResource,HWND hParent,FavoriteFolder *pAllFavorites);
	~CManageFavoritesDialog();

	int CALLBACK		SortFavorites(LPARAM lParam1,LPARAM lParam2);
	LRESULT CALLBACK	EditSearchProc(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam);

protected:

	BOOL	OnInitDialog();
	INT_PTR	OnCtlColorEdit(HWND hwnd,HDC hdc);
	BOOL	OnCommand(WPARAM wParam,LPARAM lParam);
	BOOL	OnNotify(NMHDR *pnmhdr);
	BOOL	OnClose();
	BOOL	OnDestroy();

	void	SaveState();

private:


	static const COLORREF SEARCH_TEXT_COLOR = RGB(120,120,120);

	static const int TOOLBAR_ID_BACK		= 10000;
	static const int TOOLBAR_ID_FORWARD		= 10001;
	static const int TOOLBAR_ID_ORGANIZE	= 10002;
	static const int TOOLBAR_ID_VIEWS		= 10003;

	void		SetupSearchField();
	void		SetupToolbar();
	void		SetupTreeView();
	void		SetupListView();

	void		GetColumnString(CManageFavoritesDialogPersistentSettings::ColumnType_t ColumnType,TCHAR *szColumn,UINT cchBuf);
	void		GetFavoriteItemColumnInfo(boost::variant<FavoriteFolder,Favorite> *pFavoriteVariant,CManageFavoritesDialogPersistentSettings::ColumnType_t ColumnType,TCHAR *szColumn,size_t cchBuf);
	void		GetFavoriteColumnInfo(Favorite *pFavorite,CManageFavoritesDialogPersistentSettings::ColumnType_t ColumnType,TCHAR *szColumn,size_t cchBuf);
	void		GetFavoriteFolderColumnInfo(FavoriteFolder *pFavoriteFolder,CManageFavoritesDialogPersistentSettings::ColumnType_t ColumnType,TCHAR *szColumn,size_t cchBuf);

	void		SetSearchFieldDefaultState();
	void		RemoveSearchFieldDefaultState();

	void		BrowseBack();
	void		BrowseForward();
	void		BrowseFavoriteFolder(const FavoriteFolder &FavoriteFolder);

	void		OnEnChange(HWND hEdit);
	void		OnDblClk(NMHDR *pnmhdr);
	void		OnRClick(NMHDR *pnmhdr);
	void		OnTbnDropDown(NMTOOLBAR *nmtb);
	void		OnTvnSelChanged(NMTREEVIEW *pnmtv);
	void		OnListViewRClick();
	void		OnListViewHeaderRClick();
	void		OnLvnKeyDown(NMLVKEYDOWN *pnmlvkd);


	void		OnOk();
	void		OnCancel();

	HWND						m_hToolbar;
	HIMAGELIST					m_himlToolbar;

	FavoriteFolder				*m_pAllFavorites;

	std::stack<GUID>			m_stackBack;
	std::stack<GUID>			m_stackForward;

	FavoritesTreeView			*m_pFavoritesTreeView;

	NFavoritesHelper::SortMode_t	m_SortMode;
	bool						m_bSortAscending;
	FavoritesListView			*m_pFavoritesListView;

	HFONT						m_hEditSearchFont;
	bool						m_bSearchFieldBlank;
	bool						m_bEditingSearchField;

	CManageFavoritesDialogPersistentSettings	*m_pmbdps;
};

#endif