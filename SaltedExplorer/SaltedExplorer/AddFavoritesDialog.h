#ifndef ADDFAVORITESDIALOG_INCLUDED
#define ADDFAVORITESDIALOG_INCLUDED

#include <unordered_set>
#include "FavoritesHelper.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/ResizableDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/Favorites.h"

class CAddFavoritesDialog;

class CAddFavoritesDialogPersistentSettings : public CDialogSettings
{
public:

	~CAddFavoritesDialogPersistentSettings();

	static CAddFavoritesDialogPersistentSettings &GetInstance();

private:

	friend CAddFavoritesDialog;

	static const TCHAR SETTINGS_KEY[];

	CAddFavoritesDialogPersistentSettings();

	CAddFavoritesDialogPersistentSettings(const CAddFavoritesDialogPersistentSettings &);
	CAddFavoritesDialogPersistentSettings & operator=(const CAddFavoritesDialogPersistentSettings &);

	bool							m_bInitialized;
	GUID							m_guidSelected;
	NFavoritesHelper::setExpansion_t	m_setExpansion;
};

class CAddFavoritesDialog : public CBaseDialog
{
public:

	CAddFavoritesDialog(HINSTANCE hInstance,int iResource,HWND hParent,CFavoriteFolder &AllFavorites,CFavorite &Favorite);
	~CAddFavoritesDialog();

	LRESULT CALLBACK	TreeViewEditProc(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam);

protected:

	BOOL	OnInitDialog();
	BOOL	OnCommand(WPARAM wParam,LPARAM lParam);
	BOOL	OnNotify(NMHDR *pnmhdr);
	BOOL	OnClose();
	BOOL	OnDestroy();
	BOOL	OnNcDestroy();

	void	SaveState();

	void	GetResizableControlInformation(CBaseDialog::DialogSizeConstraint &dsc,std::list<CResizableDialog::Control_t> &ControlList);

private:

	CAddFavoritesDialog & operator = (const CAddFavoritesDialog &mbd);

	void		SetDialogIcon();

	void		OnRClick(NMHDR *pnmhdr);

	void		OnNewFolder();

	void		OnTvnBeginLabelEdit();
	BOOL		OnTvnEndLabelEdit(NMTVDISPINFO *pnmtvdi);
	void		OnTvnKeyDown(NMTVKEYDOWN *pnmtvkd);

	void		OnTreeViewRename();

	void		OnOk();
	void		OnCancel();

	void		SaveTreeViewState();
	void		SaveTreeViewExpansionState(HWND hTreeView,HTREEITEM hItem);

	HICON			m_hDialogIcon;

	CFavoriteFolder	&m_AllFavorites;
	CFavorite		&m_Favorite;

	CFavoritesTreeView	*m_pFavoritesTreeView;

	CAddFavoritesDialogPersistentSettings	*m_pabdps;
};

#endif