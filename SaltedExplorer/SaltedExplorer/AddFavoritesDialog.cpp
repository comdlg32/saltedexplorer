/******************************************************************
 *
 * Project: SaltedExplorer
 * File: AddFavoritesDialog.cpp
 *
 * Handles the 'Add Favorites' dialog.
 *
 * Toiletflusher and XP Pro
 * www.saltedexplorer.ml
 *
 *****************************************************************/

#include "stdafx.h"
#include <stack>
#include "SaltedExplorer_internal.h"
#include "AddFavoritesDialog.h"
#include "FavoritesHelper.h"
#include "MainResource.h"
#include "../Helper/Macros.h"

namespace NAddFavoritesDialog
{
	LRESULT CALLBACK TreeViewEditProcStub(HWND hwnd,UINT uMsg,
		WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);
}

const TCHAR CAddFavoritesDialogPersistentSettings::SETTINGS_KEY[] = _T("AddFavorite");

CAddFavoritesDialog::CAddFavoritesDialog(HINSTANCE hInstance,int iResource,HWND hParent,
	FavoriteFolder *pAllFavorites,Favorite *pFavorite) :
m_pAllFavorites(pAllFavorites),
m_pFavorite(pFavorite),
CBaseDialog(hInstance,iResource,hParent,true)
{
	m_pabdps = &CAddFavoritesDialogPersistentSettings::GetInstance();

	/* If the singleton settings class has not been initialized
	yet, mark the root Favorite as selected and expanded. This
	is only needed the first time this dialog is shown, as
	selection and expansion info will be saved each time after
	that. */
	if(!m_pabdps->m_bInitialized)
	{
		m_pabdps->m_guidSelected = pAllFavorites->GetGUID();
		m_pabdps->m_setExpansion.insert(pAllFavorites->GetGUID());

		m_pabdps->m_bInitialized = true;
	}
}

CAddFavoritesDialog::~CAddFavoritesDialog()
{
}

BOOL CAddFavoritesDialog::OnInitDialog()
{
	SetDialogIcon();

	SetDlgItemText(m_hDlg,IDC_FAVORITES_NAME,m_pFavorite->GetName().c_str());
	SetDlgItemText(m_hDlg,IDC_FAVORITES_LOCATION,m_pFavorite->GetLocation().c_str());

	if(m_pFavorite->GetName().size() == 0 ||
		m_pFavorite->GetLocation().size() == 0)
	{
		EnableWindow(GetDlgItem(m_hDlg,IDOK),FALSE);
	}

	HWND hTreeView = GetDlgItem(m_hDlg,IDC_FAVORITES_TREEVIEW);

	m_pFavoritesTreeView = new FavoritesTreeView(hTreeView);
	m_pFavoritesTreeView->InsertFoldersIntoTreeView(m_pAllFavorites,
		m_pabdps->m_guidSelected,m_pabdps->m_setExpansion);

	HWND hEditName = GetDlgItem(m_hDlg,IDC_FAVORITES_NAME);
	SendMessage(hEditName,EM_SETSEL,0,-1);
	SetFocus(hEditName);

	m_pabdps->RestoreDialogPosition(m_hDlg,true);

	return 0;
}

void CAddFavoritesDialog::SetDialogIcon()
{
	HIMAGELIST himl = ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,0,48);
	HBITMAP hBitmap = LoadBitmap(GetInstance(),MAKEINTRESOURCE(IDB_SHELLIMAGES_2000));
	ImageList_Add(himl,hBitmap,NULL);

	m_hDialogIcon = ImageList_GetIcon(himl,SHELLIMAGES_ADDFAV,ILD_NORMAL);
	SetClassLongPtr(m_hDlg,GCLP_HICONSM,reinterpret_cast<LONG_PTR>(m_hDialogIcon));

	DeleteObject(hBitmap);
	ImageList_Destroy(himl);
}

void CAddFavoritesDialog::GetResizableControlInformation(CBaseDialog::DialogSizeConstraint &dsc,
	std::list<CResizableDialog::Control_t> &ControlList)
{
	dsc = CBaseDialog::DIALOG_SIZE_CONSTRAINT_NONE;

	CResizableDialog::Control_t Control;

	Control.iID = IDC_FAVORITES_NAME;
	Control.Type = CResizableDialog::TYPE_RESIZE;
	Control.Constraint = CResizableDialog::CONSTRAINT_X;
	ControlList.push_back(Control);

	Control.iID = IDC_FAVORITES_LOCATION;
	Control.Type = CResizableDialog::TYPE_RESIZE;
	Control.Constraint = CResizableDialog::CONSTRAINT_X;
	ControlList.push_back(Control);

	Control.iID = IDC_FAVORITES_TREEVIEW;
	Control.Type = CResizableDialog::TYPE_RESIZE;
	Control.Constraint = CResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);

	Control.iID = IDC_FAVORITES_NEWFOLDER;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_Y;
	ControlList.push_back(Control);

	Control.iID = IDOK;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);

	Control.iID = IDCANCEL;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);

	Control.iID = IDC_GRIPPER;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);
}

BOOL CAddFavoritesDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
	if(HIWORD(wParam) != 0)
	{

		switch(HIWORD(wParam))
		{
		case EN_CHANGE:
			/* If either the name or location fields are empty,
			disable the ok button. */
			BOOL bEnable = (GetWindowTextLength(GetDlgItem(m_hDlg,IDC_FAVORITES_NAME)) != 0 &&
				GetWindowTextLength(GetDlgItem(m_hDlg,IDC_FAVORITES_LOCATION)) != 0);
			EnableWindow(GetDlgItem(m_hDlg,IDOK),bEnable);
			break;
		}
	}
	else
	{
		switch(LOWORD(wParam))
		{
		case IDM_ADDFAVORITES_RLICK_RENAME:
			OnTreeViewRename();
			break;

		case IDM_ADDFAVORITES_RLICK_NEWFOLDER:
		case IDC_FAVORITES_NEWFOLDER:
			OnNewFolder();
			break;

		case IDOK:
			OnOk();
			break;

		case IDCANCEL:
			OnCancel();
			break;
		}
	}

	return 0;
}

BOOL CAddFavoritesDialog::OnNotify(NMHDR *pnmhdr)
{
	switch(pnmhdr->code)
	{
	case NM_RCLICK:
		OnRClick(pnmhdr);
		break;
	case TVN_BEGINLABELEDIT:
		OnTvnBeginLabelEdit();
		break;

	case TVN_ENDLABELEDIT:
		return OnTvnEndLabelEdit(reinterpret_cast<NMTVDISPINFO *>(pnmhdr));
		break;

	case TVN_KEYDOWN:
		OnTvnKeyDown(reinterpret_cast<NMTVKEYDOWN *>(pnmhdr));
		break;
	}

	return 0;
}

void CAddFavoritesDialog::OnNewFolder()
{
	HWND hTreeView = GetDlgItem(m_hDlg,IDC_FAVORITES_TREEVIEW);
	HTREEITEM hSelectedItem = TreeView_GetSelection(hTreeView);

	assert(hSelectedItem != NULL);

	TCHAR szTemp[64];
	LoadString(GetInstance(),IDS_FAVORITES_NEWFAVORITEFOLDER,szTemp,SIZEOF_ARRAY(szTemp));
	FavoriteFolder NewFavoriteFolder = FavoriteFolder::Create(szTemp);

	FavoriteFolder *pParentFavoriteFolder = m_pFavoritesTreeView->GetFavoriteFolderFromTreeView(
		hSelectedItem,m_pAllFavorites);
	pParentFavoriteFolder->InsertFavoriteFolder(NewFavoriteFolder);
	HTREEITEM hNewItem = m_pFavoritesTreeView->InsertFolderIntoTreeView(hSelectedItem,
		&NewFavoriteFolder,m_pabdps->m_guidSelected,m_pabdps->m_setExpansion);

	TVITEM tvi;
	tvi.mask		= TVIF_CHILDREN;
	tvi.hItem		= hSelectedItem;
	tvi.cChildren	= 1;
	TreeView_SetItem(hTreeView,&tvi);
	TreeView_Expand(hTreeView,hSelectedItem,TVE_EXPAND);

	/* The item will be selected, as it is assumed that if
	the user creates a new folder, they intend to place any
	new Favorite within that folder. */
	SetFocus(hTreeView);
	TreeView_SelectItem(hTreeView,hNewItem);
	TreeView_EditLabel(hTreeView,hNewItem);
}

void CAddFavoritesDialog::OnRClick(NMHDR *pnmhdr)
{
	if(pnmhdr->idFrom == IDC_FAVORITES_TREEVIEW)
	{
		HWND hTreeView = GetDlgItem(m_hDlg,IDC_FAVORITES_TREEVIEW);

		DWORD dwCursorPos = GetMessagePos();

		POINT ptCursor;
		ptCursor.x = GET_X_LPARAM(dwCursorPos);
		ptCursor.y = GET_Y_LPARAM(dwCursorPos);

		TVHITTESTINFO tvhti;
		tvhti.pt = ptCursor;
		ScreenToClient(hTreeView,&tvhti.pt);
		HTREEITEM hItem = TreeView_HitTest(hTreeView,&tvhti);

		if(hItem != NULL)
		{
			TreeView_SelectItem(hTreeView,hItem);

			HMENU hMenu = LoadMenu(GetInstance(),MAKEINTRESOURCE(IDR_ADDFAVORITES_RCLICK_MENU));
			TrackPopupMenu(GetSubMenu(hMenu,0),TPM_LEFTALIGN,ptCursor.x,ptCursor.y,0,m_hDlg,NULL);
			DestroyMenu(hMenu);
		}
	}
}

void CAddFavoritesDialog::OnTvnBeginLabelEdit()
{
	HWND hEdit = reinterpret_cast<HWND>(SendDlgItemMessage(m_hDlg,
		IDC_FAVORITES_TREEVIEW,TVM_GETEDITCONTROL,0,0));
	SetWindowSubclass(hEdit,NAddFavoritesDialog::TreeViewEditProcStub,0,
		reinterpret_cast<DWORD_PTR>(this));
}

BOOL CAddFavoritesDialog::OnTvnEndLabelEdit(NMTVDISPINFO *pnmtvdi)
{
	HWND hEdit = reinterpret_cast<HWND>(SendDlgItemMessage(m_hDlg,
		IDC_FAVORITES_TREEVIEW,TVM_GETEDITCONTROL,0,0));
	RemoveWindowSubclass(hEdit,NAddFavoritesDialog::TreeViewEditProcStub,0);

	if(pnmtvdi->item.pszText != NULL &&
		lstrlen(pnmtvdi->item.pszText) > 0)
	{
		FavoriteFolder *pFavoriteFolder = m_pFavoritesTreeView->GetFavoriteFolderFromTreeView(
			pnmtvdi->item.hItem,m_pAllFavorites);
		pFavoriteFolder->SetName(pnmtvdi->item.pszText);

		SetWindowLongPtr(m_hDlg,DWLP_MSGRESULT,TRUE);
		return TRUE;
	}

	SetWindowLongPtr(m_hDlg,DWLP_MSGRESULT,FALSE);
	return FALSE;
}

LRESULT CALLBACK NAddFavoritesDialog::TreeViewEditProcStub(HWND hwnd,UINT uMsg,
	WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	CAddFavoritesDialog *pabd = reinterpret_cast<CAddFavoritesDialog *>(dwRefData);

	return pabd->TreeViewEditProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK CAddFavoritesDialog::TreeViewEditProc(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	switch(Msg)
	{
	case WM_GETDLGCODE:
		switch(wParam)
		{
		case VK_RETURN:
			return DLGC_WANTALLKEYS;
			break;
		}
		break;
	}

	return DefSubclassProc(hwnd,Msg,wParam,lParam);
}

void CAddFavoritesDialog::OnTvnKeyDown(NMTVKEYDOWN *pnmtvkd)
{
	switch(pnmtvkd->wVKey)
	{
	case VK_F2:
		OnTreeViewRename();
		break;
	}
}

void CAddFavoritesDialog::OnTreeViewRename()
{
	HWND hTreeView = GetDlgItem(m_hDlg,IDC_FAVORITES_TREEVIEW);
	HTREEITEM hSelectedItem = TreeView_GetSelection(hTreeView);
	TreeView_EditLabel(hTreeView,hSelectedItem);
}

void CAddFavoritesDialog::OnOk()
{

	HWND hName = GetDlgItem(m_hDlg,IDC_FAVORITES_NAME);
	std::wstring strName;
	GetWindowString(hName,strName);

	HWND hLocation = GetDlgItem(m_hDlg,IDC_FAVORITES_LOCATION);
	std::wstring strLocation;
	GetWindowString(hLocation,strLocation);

	if(strName.size() > 0 &&
		strLocation.size() > 0)
	{
		HWND hTreeView = GetDlgItem(m_hDlg,IDC_FAVORITES_TREEVIEW);
		HTREEITEM hSelected = TreeView_GetSelection(hTreeView);
		FavoriteFolder *pFavoriteFolder = m_pFavoritesTreeView->GetFavoriteFolderFromTreeView(
			hSelected,m_pAllFavorites);

		Favorite Favorite(strName,strLocation,_T(""));
		pFavoriteFolder->InsertFavorite(Favorite);
	}

	EndDialog(m_hDlg,1);
}
void CAddFavoritesDialog::OnCancel()
{
	EndDialog(m_hDlg,0);
}

void CAddFavoritesDialog::SaveState()
{
	m_pabdps->SaveDialogPosition(m_hDlg);

	SaveTreeViewState();

	m_pabdps->m_bStateSaved = TRUE;
}

void CAddFavoritesDialog::SaveTreeViewState()
{
	HWND hTreeView = GetDlgItem(m_hDlg,IDC_FAVORITES_TREEVIEW);

	HTREEITEM hSelected = TreeView_GetSelection(hTreeView);
	FavoriteFolder *pFavoriteFolder = m_pFavoritesTreeView->GetFavoriteFolderFromTreeView(hSelected,m_pAllFavorites);
	m_pabdps->m_guidSelected  = pFavoriteFolder->GetGUID();

	m_pabdps->m_setExpansion.clear();
	SaveTreeViewExpansionState(hTreeView,TreeView_GetRoot(hTreeView));
}

void CAddFavoritesDialog::SaveTreeViewExpansionState(HWND hTreeView,HTREEITEM hItem)
{
	UINT uState = TreeView_GetItemState(hTreeView,hItem,TVIS_EXPANDED);

	if(uState & TVIS_EXPANDED)
	{
		FavoriteFolder *pFavoriteFolder = m_pFavoritesTreeView->GetFavoriteFolderFromTreeView(hItem,m_pAllFavorites);
		m_pabdps->m_setExpansion.insert(pFavoriteFolder->GetGUID());

		HTREEITEM hChild = TreeView_GetChild(hTreeView,hItem);
		SaveTreeViewExpansionState(hTreeView,hChild);

		while((hChild = TreeView_GetNextSibling(hTreeView,hChild)) != NULL)
		{
			SaveTreeViewExpansionState(hTreeView,hChild);
		}
	}
}

BOOL CAddFavoritesDialog::OnClose()
{
	EndDialog(m_hDlg,0);
	return 0;
}

BOOL CAddFavoritesDialog::OnDestroy()
{
	DestroyIcon(m_hDialogIcon);
	return 0;
}

BOOL CAddFavoritesDialog::OnNcDestroy()
{
	delete m_pFavoritesTreeView;

	return 0;
}

CAddFavoritesDialogPersistentSettings::CAddFavoritesDialogPersistentSettings() :
CDialogSettings(SETTINGS_KEY)
{
	m_bInitialized = false;
}

CAddFavoritesDialogPersistentSettings::~CAddFavoritesDialogPersistentSettings()
{
}

CAddFavoritesDialogPersistentSettings& CAddFavoritesDialogPersistentSettings::GetInstance()
{
	static CAddFavoritesDialogPersistentSettings abdps;
	return abdps;
}