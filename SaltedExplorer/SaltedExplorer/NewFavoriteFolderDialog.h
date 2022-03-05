#ifndef NEWFAVORITEFOLDERDIALOG_INCLUDED
#define NEWFAVORITEFOLDERDIALOG_INCLUDED

#include "../Helper/BaseDialog.h"
#include "../Helper/ResizableDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/Favorites.h"

class CNewFavoriteFolderDialog;

class CNewFavoriteFolderDialogPersistentSettings : public CDialogSettings
{
public:

	~CNewFavoriteFolderDialogPersistentSettings();

	static CNewFavoriteFolderDialogPersistentSettings &GetInstance();

private:

	friend CNewFavoriteFolderDialog;

	static const TCHAR SETTINGS_KEY[];

	CNewFavoriteFolderDialogPersistentSettings();

	CNewFavoriteFolderDialogPersistentSettings(const CNewFavoriteFolderDialogPersistentSettings &);
	CNewFavoriteFolderDialogPersistentSettings & operator=(const CNewFavoriteFolderDialogPersistentSettings &);
};

class CNewFavoriteFolderDialog : public CBaseDialog
{
public:

	CNewFavoriteFolderDialog(HINSTANCE hInstance,int iResource,HWND hParent);
	~CNewFavoriteFolderDialog();

protected:

	BOOL	OnInitDialog();
	BOOL	OnCommand(WPARAM wParam,LPARAM lParam);
	BOOL	OnClose();

private:

	void	OnOk();
	void	OnCancel();

	CNewFavoriteFolderDialogPersistentSettings	*m_pnbfdps;
};

#endif