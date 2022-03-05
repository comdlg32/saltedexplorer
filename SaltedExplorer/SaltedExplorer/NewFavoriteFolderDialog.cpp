/******************************************************************
 *
 * Project: SaltedExplorer
 * File: NewFavoriteFolderDialog.cpp
 *
 * Handles the 'New Folder' dialog (for Favorites).
 *
 * Toiletflusher and XP Pro
 * www.saltedexplorer.ml
 *
 *****************************************************************/

#include "stdafx.h"
#include "SaltedExplorer.h"
#include "NewFavoriteFolderDialog.h"
#include "MainResource.h"


const TCHAR CNewFavoriteFolderDialogPersistentSettings::SETTINGS_KEY[] = _T("NewFavoriteFolder");

CNewFavoriteFolderDialog::CNewFavoriteFolderDialog(HINSTANCE hInstance,
	int iResource,HWND hParent) :
CBaseDialog(hInstance,iResource,hParent,true)
{
	m_pnbfdps = &CNewFavoriteFolderDialogPersistentSettings::GetInstance();
}

CNewFavoriteFolderDialog::~CNewFavoriteFolderDialog()
{
}

BOOL CNewFavoriteFolderDialog::OnInitDialog()
{
	return 0;
}

BOOL CNewFavoriteFolderDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
	switch(LOWORD(wParam))
	{
	case IDOK:
		OnOk();
		break;

	case IDCANCEL:
		OnCancel();
		break;
	}

	return 0;
}

void CNewFavoriteFolderDialog::OnOk()
{
	EndDialog(m_hDlg,1);
}

void CNewFavoriteFolderDialog::OnCancel()
{
	EndDialog(m_hDlg,0);
}

BOOL CNewFavoriteFolderDialog::OnClose()
{
	EndDialog(m_hDlg,0);
	return 0;
}

CNewFavoriteFolderDialogPersistentSettings::CNewFavoriteFolderDialogPersistentSettings() :
CDialogSettings(SETTINGS_KEY)
{
}

CNewFavoriteFolderDialogPersistentSettings::~CNewFavoriteFolderDialogPersistentSettings()
{

}

CNewFavoriteFolderDialogPersistentSettings& CNewFavoriteFolderDialogPersistentSettings::GetInstance()
{
	static CNewFavoriteFolderDialogPersistentSettings nbfdps;
	return nbfdps;
}