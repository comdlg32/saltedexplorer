/******************************************************************
 *
 * Project: SaltedExplorer
 * File: ManageFavoritesDialog.cpp
 *
 * Handles the 'Manage Favorites' dialog.
 *
 * Toiletflusher and XP Pro
 * www.saltedexplorer.ml
 *
 *****************************************************************/

#include "stdafx.h"
#include <stack>
#include "SaltedExplorer_internal.h"
#include "ManageFavoritesDialog.h"
#include "MainResource.h"
#include "FavoritesHelper.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/ListViewHelper.h"

namespace NManageFavoritesDialog
{
	int CALLBACK		SortFavoritesStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);

	LRESULT CALLBACK	EditSearchProcStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);
}

const TCHAR CManageFavoritesDialogPersistentSettings::SETTINGS_KEY[] = _T("ManageFavorites");

CManageFavoritesDialog::CManageFavoritesDialog(HINSTANCE hInstance,int iResource,HWND hParent,
	CFavoriteFolder &AllFavorites) :
m_AllFavorites(AllFavorites),
m_guidCurrentFolder(AllFavorites.GetGUID()),
m_bNewFolderAdded(false),
m_bSearchFieldBlank(true),
m_bEditingSearchField(false),
m_hEditSearchFont(NULL),
CBaseDialog(hInstance,iResource,hParent,true)
{
	m_pmbdps = &CManageFavoritesDialogPersistentSettings::GetInstance();

	if(!m_pmbdps->m_bInitialized)
	{
		m_pmbdps->m_guidSelected = AllFavorites.GetGUID();
		m_pmbdps->m_setExpansion.insert(AllFavorites.GetGUID());

		m_pmbdps->m_bInitialized = true;
	}
}

CManageFavoritesDialog::~CManageFavoritesDialog()
{
	delete m_pFavoritesTreeView;
	delete m_pFavoritesListView;
}

BOOL CManageFavoritesDialog::OnInitDialog()
{
	SetupSearchField();
	SetupToolbar();
	SetupTreeView();
	SetupListView();

	CFavoriteItemNotifier::GetInstance().AddObserver(this);

	UpdateToolbarState();

	SetFocus(GetDlgItem(m_hDlg,IDC_MANAGEFAVORITES_LISTVIEW));

	return 0;
}

void CManageFavoritesDialog::SetupSearchField()
{
	HWND hEdit = GetDlgItem(m_hDlg,IDC_MANAGEFAVORITES_EDITSEARCH);
	SetWindowSubclass(hEdit,NManageFavoritesDialog::EditSearchProcStub,
		0,reinterpret_cast<DWORD_PTR>(this));
	SetSearchFieldDefaultState();
}

void CManageFavoritesDialog::SetupToolbar()
{
	m_hToolbar = CreateToolbar(m_hDlg,
		WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|
		TBSTYLE_TOOLTIPS|TBSTYLE_LIST|TBSTYLE_TRANSPARENT|
		TBSTYLE_FLAT|CCS_NODIVIDER|CCS_NORESIZE,
		TBSTYLE_EX_MIXEDBUTTONS|TBSTYLE_EX_DRAWDDARROWS|
		TBSTYLE_EX_DOUBLEBUFFER|TBSTYLE_EX_HIDECLIPPEDBUTTONS);

	SendMessage(m_hToolbar,TB_SETBITMAPSIZE,0,MAKELONG(16,16));
	SendMessage(m_hToolbar,TB_BUTTONSTRUCTSIZE,static_cast<WPARAM>(sizeof(TBBUTTON)),0);

	m_himlToolbar = ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,0,48);
	HBITMAP hBitmap = LoadBitmap(GetInstance(),MAKEINTRESOURCE(IDB_SHELLIMAGES_2000));
	ImageList_Add(m_himlToolbar,hBitmap,NULL);
	DeleteObject(hBitmap);
	SendMessage(m_hToolbar,TB_SETIMAGELIST,0,reinterpret_cast<LPARAM>(m_himlToolbar));

	TBBUTTON tbb;
	TCHAR szTemp[64];

	tbb.iBitmap		= SHELLIMAGES_BACK;
	tbb.idCommand	= TOOLBAR_ID_BACK;
	tbb.fsState		= TBSTATE_ENABLED;
	tbb.fsStyle		= BTNS_BUTTON|BTNS_AUTOSIZE;
	tbb.dwData		= 0;
	SendMessage(m_hToolbar,TB_INSERTBUTTON,0,reinterpret_cast<LPARAM>(&tbb));

	tbb.iBitmap		= SHELLIMAGES_FORWARD;
	tbb.idCommand	= TOOLBAR_ID_FORWARD;
	tbb.fsState		= TBSTATE_ENABLED;
	tbb.fsStyle		= BTNS_BUTTON|BTNS_AUTOSIZE;
	tbb.dwData		= 0;
	SendMessage(m_hToolbar,TB_INSERTBUTTON,1,reinterpret_cast<LPARAM>(&tbb));

	LoadString(GetInstance(),IDS_MANAGE_FAVORITES_TOOLBAR_ORGANIZE,szTemp,SIZEOF_ARRAY(szTemp));

	tbb.iBitmap		= SHELLIMAGES_COPY;
	tbb.idCommand	= TOOLBAR_ID_ORGANIZE;
	tbb.fsState		= TBSTATE_ENABLED;
	tbb.fsStyle		= BTNS_BUTTON|BTNS_AUTOSIZE|BTNS_SHOWTEXT|BTNS_DROPDOWN;
	tbb.dwData		= 0;
	tbb.iString		= reinterpret_cast<INT_PTR>(szTemp);
	SendMessage(m_hToolbar,TB_INSERTBUTTON,2,reinterpret_cast<LPARAM>(&tbb));

	LoadString(GetInstance(),IDS_MANAGE_FAVORITES_TOOLBAR_VIEWS,szTemp,SIZEOF_ARRAY(szTemp));

	tbb.iBitmap		= SHELLIMAGES_VIEWS;
	tbb.idCommand	= TOOLBAR_ID_VIEWS;
	tbb.fsState		= TBSTATE_ENABLED;
	tbb.fsStyle		= BTNS_BUTTON|BTNS_AUTOSIZE|BTNS_SHOWTEXT|BTNS_DROPDOWN;
	tbb.dwData		= 0;
	tbb.iString		= reinterpret_cast<INT_PTR>(szTemp);
	SendMessage(m_hToolbar,TB_INSERTBUTTON,3,reinterpret_cast<LPARAM>(&tbb));

	LoadString(GetInstance(),IDS_MANAGE_FAVORITES_TOOLBAR_IMPORTEXPORT,szTemp,SIZEOF_ARRAY(szTemp));

	tbb.iBitmap		= SHELLIMAGES_PROPERTIES;
	tbb.idCommand	= TOOLBAR_ID_IMPORTEXPORT;
	tbb.fsState		= TBSTATE_ENABLED;
	tbb.fsStyle		= BTNS_BUTTON|BTNS_AUTOSIZE|BTNS_SHOWTEXT|BTNS_DROPDOWN;
	tbb.dwData		= 0;
	tbb.iString		= reinterpret_cast<INT_PTR>(szTemp);
	SendMessage(m_hToolbar,TB_INSERTBUTTON,4,reinterpret_cast<LPARAM>(&tbb));

	RECT rcTreeView;
	GetWindowRect(GetDlgItem(m_hDlg,IDC_MANAGEFAVORITES_TREEVIEW),&rcTreeView);
	MapWindowPoints(HWND_DESKTOP,m_hDlg,reinterpret_cast<LPPOINT>(&rcTreeView),2);

	RECT rcSearch;
	GetWindowRect(GetDlgItem(m_hDlg,IDC_MANAGEFAVORITES_EDITSEARCH),&rcSearch);
	MapWindowPoints(HWND_DESKTOP,m_hDlg,reinterpret_cast<LPPOINT>(&rcSearch),2);

	DWORD dwButtonSize = static_cast<DWORD>(SendMessage(m_hToolbar,TB_GETBUTTONSIZE,0,0));
	SetWindowPos(m_hToolbar,NULL,rcTreeView.left,rcSearch.top - (HIWORD(dwButtonSize) - GetRectHeight(&rcSearch)) / 2,
		rcSearch.left - rcTreeView.left - 10,HIWORD(dwButtonSize),0);
}

void CManageFavoritesDialog::SetupTreeView()
{
	HWND hTreeView = GetDlgItem(m_hDlg,IDC_MANAGEFAVORITES_TREEVIEW);

	m_pFavoritesTreeView = new CFavoritesTreeView(hTreeView,&m_AllFavorites,
		m_pmbdps->m_guidSelected,m_pmbdps->m_setExpansion);
}
void CManageFavoritesDialog::SetupListView()
{
	HWND hListView = GetDlgItem(m_hDlg,IDC_MANAGEFAVORITES_LISTVIEW);

	m_pFavoritesListView = new CFavoritesListView(hListView);

	int iColumn = 0;

	for each(auto ci in m_pmbdps->m_vectorColumnInfo)
	{
		if(ci.bActive)
		{
			LVCOLUMN lvCol;
			TCHAR szTemp[128];

			GetColumnString(ci.ColumnType,szTemp,SIZEOF_ARRAY(szTemp));
			lvCol.mask		= LVCF_TEXT|LVCF_WIDTH;
			lvCol.pszText	= szTemp;
			lvCol.cx		= ci.iWidth;
			ListView_InsertColumn(hListView,iColumn,&lvCol);

			++iColumn;
		}
	}

	m_pFavoritesListView->InsertFavoritesIntoListView(m_AllFavorites);
	int iItem = 0;

	/* Update the data for each of the sub-items. */
	for(auto itr = m_AllFavorites.begin();itr != m_AllFavorites.end();++itr)
	{
		int iSubItem = 1;

		for each(auto ci in m_pmbdps->m_vectorColumnInfo)
		{
			/* The name column will always appear first in
			the set of columns and can be skipped here. */
			if(ci.bActive && ci.ColumnType != CManageFavoritesDialogPersistentSettings::COLUMN_TYPE_NAME)
			{
				TCHAR szColumn[256];
				GetFavoriteItemColumnInfo(*itr,ci.ColumnType,szColumn,SIZEOF_ARRAY(szColumn));
				ListView_SetItemText(hListView,iItem,iSubItem,szColumn);

				++iSubItem;
			}
		}

		++iItem;
	}

	ListView_SortItems(hListView,NManageFavoritesDialog::SortFavoritesStub,reinterpret_cast<LPARAM>(this));
	NListView::ListView_SelectItem(hListView,0,TRUE);

	m_bListViewInitialized = true;
}

void CManageFavoritesDialog::SortListViewItems(NFavoritesHelper::SortMode_t SortMode)
{
	m_pmbdps->m_SortMode = SortMode;

	HWND hListView = GetDlgItem(m_hDlg,IDC_MANAGEFAVORITES_LISTVIEW);
	ListView_SortItems(hListView,NManageFavoritesDialog::SortFavoritesStub,reinterpret_cast<LPARAM>(this));
}

int CALLBACK NManageFavoritesDialog::SortFavoritesStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	assert(lParamSort != NULL);

	CManageFavoritesDialog *pmbd = reinterpret_cast<CManageFavoritesDialog *>(lParamSort);

	return pmbd->SortFavorites(lParam1,lParam2);
}

int CALLBACK CManageFavoritesDialog::SortFavorites(LPARAM lParam1,LPARAM lParam2)
{
	/* TODO: Need to be able to retrieve items using their lParam
	value. */
	HWND hTreeView = GetDlgItem(m_hDlg,IDC_MANAGEFAVORITES_TREEVIEW);
	HTREEITEM hSelected = TreeView_GetSelection(hTreeView);
	CFavoriteFolder &FavoriteFolder = m_pFavoritesTreeView->GetFavoriteFolderFromTreeView(hSelected);

	NFavoritesHelper::variantFavorite_t variantFavorite1 = m_pFavoritesListView->GetFavoriteItemFromListViewlParam(FavoriteFolder,lParam1);
	NFavoritesHelper::variantFavorite_t variantFavorite2 = m_pFavoritesListView->GetFavoriteItemFromListViewlParam(FavoriteFolder,lParam2);

	int iRes = 0;

	switch(m_pmbdps->m_SortMode)
	{
	case NFavoritesHelper::SM_NAME:
		iRes = NFavoritesHelper::SortByName(variantFavorite1,variantFavorite2);
		break;

	case NFavoritesHelper::SM_LOCATION:
		iRes = NFavoritesHelper::SortByLocation(variantFavorite1,variantFavorite2);
		break;

	case NFavoritesHelper::SM_VISIT_DATE:
		iRes = NFavoritesHelper::SortByVisitDate(variantFavorite1,variantFavorite2);
		break;

	case NFavoritesHelper::SM_VISIT_COUNT:
		iRes = NFavoritesHelper::SortByVisitCount(variantFavorite1,variantFavorite2);
		break;

	case NFavoritesHelper::SM_ADDED:
		iRes = NFavoritesHelper::SortByAdded(variantFavorite1,variantFavorite2);
		break;

	case NFavoritesHelper::SM_LAST_MODIFIED:
		iRes = NFavoritesHelper::SortByLastModified(variantFavorite1,variantFavorite2);
		break;
	}

	if(!m_pmbdps->m_bSortAscending)
	{
		iRes = -iRes;
	}

	return iRes;
}

/* Changes the font within the search edit
control, and sets the default text. */
void CManageFavoritesDialog::SetSearchFieldDefaultState()
{
	HWND hEdit = GetDlgItem(m_hDlg,IDC_MANAGEFAVORITES_EDITSEARCH);

	LOGFONT lf;
	HFONT hCurentFont = reinterpret_cast<HFONT>(SendMessage(hEdit,WM_GETFONT,0,0));
	GetObject(hCurentFont,sizeof(lf),reinterpret_cast<LPVOID>(&lf));

	HFONT hPrevEditSearchFont = m_hEditSearchFont;

	lf.lfItalic = TRUE;
	m_hEditSearchFont = CreateFontIndirect(&lf);
	SendMessage(hEdit,WM_SETFONT,reinterpret_cast<WPARAM>(m_hEditSearchFont),MAKEWORD(TRUE,0));

	if(hPrevEditSearchFont != NULL)
	{
		DeleteFont(hPrevEditSearchFont);
	}

	TCHAR szTemp[64];
	LoadString(GetInstance(),IDS_MANAGE_FAVORITES_DEFAULT_SEARCH_TEXT,szTemp,SIZEOF_ARRAY(szTemp));
	SetWindowText(hEdit,szTemp);
}

/* Resets the font within the search
field and removes any text. */
void CManageFavoritesDialog::RemoveSearchFieldDefaultState()
{
	HWND hEdit = GetDlgItem(m_hDlg,IDC_MANAGEFAVORITES_EDITSEARCH);

	LOGFONT lf;
	HFONT hCurentFont = reinterpret_cast<HFONT>(SendMessage(hEdit,WM_GETFONT,0,0));
	GetObject(hCurentFont,sizeof(lf),reinterpret_cast<LPVOID>(&lf));

	HFONT hPrevEditSearchFont = m_hEditSearchFont;

	lf.lfItalic = FALSE;
	m_hEditSearchFont = CreateFontIndirect(&lf);
	SendMessage(hEdit,WM_SETFONT,reinterpret_cast<WPARAM>(m_hEditSearchFont),MAKEWORD(TRUE,0));

	DeleteFont(hPrevEditSearchFont);

	SetWindowText(hEdit,EMPTY_STRING);
}

LRESULT CALLBACK NManageFavoritesDialog::EditSearchProcStub(HWND hwnd,UINT uMsg,
	WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	CManageFavoritesDialog *pmbd = reinterpret_cast<CManageFavoritesDialog *>(dwRefData);

	return pmbd->EditSearchProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK CManageFavoritesDialog::EditSearchProc(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	switch(Msg)
	{
	case WM_SETFOCUS:
		if(m_bSearchFieldBlank)
		{
			RemoveSearchFieldDefaultState();
		}

		m_bEditingSearchField = true;
		break;

	case WM_KILLFOCUS:
		if(GetWindowTextLength(hwnd) == 0)
		{
			m_bSearchFieldBlank = true;
			SetSearchFieldDefaultState();
		}
		else
		{
			m_bSearchFieldBlank = false;
		}

		m_bEditingSearchField = false;
		break;
	}

	return DefSubclassProc(hwnd,Msg,wParam,lParam);
}

INT_PTR CManageFavoritesDialog::OnCtlColorEdit(HWND hwnd,HDC hdc)
{
	if(hwnd == GetDlgItem(m_hDlg,IDC_MANAGEFAVORITES_EDITSEARCH))
	{
		if(m_bSearchFieldBlank &&
			!m_bEditingSearchField)
		{
			SetTextColor(hdc,SEARCH_TEXT_COLOR);
			SetBkMode(hdc,TRANSPARENT);
			return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_WINDOW));
		}
		else
		{
			SetTextColor(hdc,GetSysColor(COLOR_WINDOWTEXT));
			SetBkMode(hdc,TRANSPARENT);
			return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_WINDOW));
		}
	}

	return 0;
}

BOOL CManageFavoritesDialog::OnAppCommand(HWND hwnd,UINT uCmd,UINT uDevice,DWORD dwKeys)
{
	switch(uCmd)
	{
	case APPCOMMAND_BROWSER_BACKWARD:
		BrowseBack();
		break;

	case APPCOMMAND_BROWSER_FORWARD:
		BrowseForward();
		break;
	}

	return 0;
}

BOOL CManageFavoritesDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
	if(HIWORD(wParam) != 0)
	{
		switch(HIWORD(wParam))
		{
		case EN_CHANGE:
			OnEnChange(reinterpret_cast<HWND>(lParam));
			break;
		}
	}
	else
	{
		switch(LOWORD(wParam))
		{
		case TOOLBAR_ID_ORGANIZE:
			ShowOrganizeMenu();
			break;

		case TOOLBAR_ID_VIEWS:
			ShowViewMenu();
			break;

		case IDM_MB_ORGANIZE_NEWFOLDER:
			OnNewFolder();
			break;

		case IDM_MB_VIEW_SORTBYNAME:
			SortListViewItems(NFavoritesHelper::SM_NAME);
			break;

		case IDM_MB_VIEW_SORTBYLOCATION:
			SortListViewItems(NFavoritesHelper::SM_LOCATION);
			break;

		case IDM_MB_VIEW_SORTBYVISITDATE:
			SortListViewItems(NFavoritesHelper::SM_VISIT_DATE);
			break;

		case IDM_MB_VIEW_SORTBYVISITCOUNT:
			SortListViewItems(NFavoritesHelper::SM_VISIT_COUNT);
			break;

		case IDM_MB_VIEW_SORTBYADDED:
			SortListViewItems(NFavoritesHelper::SM_ADDED);
			break;

		case IDM_MB_VIEW_SORTBYLASTMODIFIED:
			SortListViewItems(NFavoritesHelper::SM_LAST_MODIFIED);
			break;

		case IDM_MB_VIEW_SORTASCENDING:
			m_pmbdps->m_bSortAscending = true;
			SortListViewItems(m_pmbdps->m_SortMode);
			break;

		case IDM_MB_VIEW_SORTDESCENDING:
			m_pmbdps->m_bSortAscending = false;
			SortListViewItems(m_pmbdps->m_SortMode);
			break;

			/* TODO: */
		case IDM_MB_FAVORITE_OPEN:
			break;

		case IDM_MB_FAVORITE_OPENINNEWTAB:
			break;

		case IDM_MB_FAVORITE_OPENINNEWWINDOW:
			break;

		case IDM_MB_FAVORITE_CUT:
			break;

		case IDM_MB_FAVORITE_COPY:
			break;

		case IDM_MB_FAVORITE_DELETE:
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

BOOL CManageFavoritesDialog::OnNotify(NMHDR *pnmhdr)
{
	switch(pnmhdr->code)
	{
	case NM_DBLCLK:
		OnDblClk(pnmhdr);
		break;

	case NM_RCLICK:
		OnRClick(pnmhdr);
		break;

	case TVN_SELCHANGED:
		OnTvnSelChanged(reinterpret_cast<NMTREEVIEW *>(pnmhdr));
		break;

	case LVN_ENDLABELEDIT:
		return OnLvnEndLabelEdit(reinterpret_cast<NMLVDISPINFO *>(pnmhdr));
		break;

	case LVN_KEYDOWN:
		OnLvnKeyDown(reinterpret_cast<NMLVKEYDOWN *>(pnmhdr));
		break;
	}

	return 0;
}

void CManageFavoritesDialog::OnNewFolder()
{
	TCHAR szTemp[64];
	LoadString(GetInstance(),IDS_FAVORITES_NEWFAVORITEFOLDER,szTemp,SIZEOF_ARRAY(szTemp));
	CFavoriteFolder NewFavoriteFolder = CFavoriteFolder::Create(szTemp);

	/* Save the folder GUID, so that it can be selected and
	placed into edit mode once the Favorite notification
	comes through. */
	m_bNewFolderAdded = true;
	m_guidNewFolder = NewFavoriteFolder.GetGUID();

	HWND hTreeView = GetDlgItem(m_hDlg,IDC_FAVORITES_TREEVIEW);
	HTREEITEM hSelectedItem = TreeView_GetSelection(hTreeView);

	assert(hSelectedItem != NULL);

	CFavoriteFolder &ParentFavoriteFolder = m_pFavoritesTreeView->GetFavoriteFolderFromTreeView(
		hSelectedItem);
	ParentFavoriteFolder.InsertFavoriteFolder(NewFavoriteFolder);
}

void CManageFavoritesDialog::OnEnChange(HWND hEdit)
{
	if(hEdit != GetDlgItem(m_hDlg,IDC_MANAGEFAVORITES_EDITSEARCH))
	{
		return;
	}

	std::wstring strSearch;
	GetWindowString(hEdit,strSearch);

	if(strSearch.size() > 0)
	{

	}
}

void CManageFavoritesDialog::OnListViewRClick()
{
	DWORD dwCursorPos = GetMessagePos();

	POINT ptCursor;
	ptCursor.x = GET_X_LPARAM(dwCursorPos);
	ptCursor.y = GET_Y_LPARAM(dwCursorPos);

	HWND hListView = GetDlgItem(m_hDlg,IDC_MANAGEFAVORITES_LISTVIEW);

	LVHITTESTINFO lvhti;
	lvhti.pt = ptCursor;
	ScreenToClient(hListView,&lvhti.pt);
	int iItem = ListView_HitTest(hListView,&lvhti);

	if(iItem == -1)
	{
		return;
	}

	HMENU hMenu = LoadMenu(GetInstance(),MAKEINTRESOURCE(IDR_MANAGEFAVORITES_FAVORITE_RCLICK_MENU));
	SetMenuDefaultItem(GetSubMenu(hMenu,0),IDM_MB_FAVORITE_OPEN,FALSE);

	TrackPopupMenu(GetSubMenu(hMenu,0),TPM_LEFTALIGN,ptCursor.x,ptCursor.y,0,m_hDlg,NULL);
	DestroyMenu(hMenu);
}

void CManageFavoritesDialog::OnListViewHeaderRClick()
{
	DWORD dwCursorPos = GetMessagePos();

	POINT ptCursor;
	ptCursor.x = GET_X_LPARAM(dwCursorPos);
	ptCursor.y = GET_Y_LPARAM(dwCursorPos);

	HMENU hMenu = CreatePopupMenu();
	int iItem = 0;

	for each(auto ci in m_pmbdps->m_vectorColumnInfo)
	{
		TCHAR szColumn[128];
		GetColumnString(ci.ColumnType,szColumn,SIZEOF_ARRAY(szColumn));

		MENUITEMINFO mii;
		mii.cbSize		= sizeof(mii);
		mii.fMask		= MIIM_ID|MIIM_STRING|MIIM_STATE;
		mii.wID			= ci.ColumnType;
		mii.dwTypeData	= szColumn;
		mii.fState		= 0;

		if(ci.bActive)
		{
			mii.fState |= MFS_CHECKED;
		}

		/* The name column cannot be removed. */
		if(ci.ColumnType == CManageFavoritesDialogPersistentSettings::COLUMN_TYPE_NAME)
		{
			mii.fState |= MFS_DISABLED;
		}

		InsertMenuItem(hMenu,iItem,TRUE,&mii);

		++iItem;
	}

	int iCmd = TrackPopupMenu(hMenu,TPM_LEFTALIGN|TPM_RETURNCMD,ptCursor.x,ptCursor.y,0,m_hDlg,NULL);
	DestroyMenu(hMenu);

	int iColumn = 0;

	for(auto itr = m_pmbdps->m_vectorColumnInfo.begin();itr != m_pmbdps->m_vectorColumnInfo.end();++itr)
	{
		if(itr->ColumnType == iCmd)
		{
			HWND hListView = GetDlgItem(m_hDlg,IDC_MANAGEFAVORITES_LISTVIEW);

			if(itr->bActive)
			{
				itr->iWidth = ListView_GetColumnWidth(hListView,iColumn);
				ListView_DeleteColumn(hListView,iColumn);
			}
			else
			{
				LVCOLUMN lvCol;
				TCHAR szTemp[128];

				GetColumnString(itr->ColumnType,szTemp,SIZEOF_ARRAY(szTemp));
				lvCol.mask		= LVCF_TEXT|LVCF_WIDTH;
				lvCol.pszText	= szTemp;
				lvCol.cx		= itr->iWidth;
				ListView_InsertColumn(hListView,iColumn,&lvCol);

				HWND hTreeView = GetDlgItem(m_hDlg,IDC_MANAGEFAVORITES_TREEVIEW);
				HTREEITEM hSelected = TreeView_GetSelection(hTreeView);
				CFavoriteFolder &FavoriteFolder = m_pFavoritesTreeView->GetFavoriteFolderFromTreeView(hSelected);

				int iItem = 0;

				for(auto itrFavorites = FavoriteFolder.begin();itrFavorites != FavoriteFolder.end();++itrFavorites)
				{
					TCHAR szColumn[256];
					GetFavoriteItemColumnInfo(*itrFavorites,itr->ColumnType,szColumn,SIZEOF_ARRAY(szColumn));
					ListView_SetItemText(hListView,iItem,iColumn,szColumn);

					++iItem;
				}
			}

			itr->bActive = !itr->bActive;

			break;
		}
		else
		{
			if(itr->bActive)
			{
				++iColumn;
			}
		}
	}
}

BOOL CManageFavoritesDialog::OnLvnEndLabelEdit(NMLVDISPINFO *pnmlvdi)
{
	if(pnmlvdi->item.pszText != NULL &&
		lstrlen(pnmlvdi->item.pszText) > 0)
	{
		HWND hTreeView = GetDlgItem(m_hDlg,IDC_FAVORITES_TREEVIEW);
		HTREEITEM hSelectedItem = TreeView_GetSelection(hTreeView);

		assert(hSelectedItem != NULL);

		CFavoriteFolder &ParentFavoriteFolder = m_pFavoritesTreeView->GetFavoriteFolderFromTreeView(hSelectedItem);
		NFavoritesHelper::variantFavorite_t variantFavorite = m_pFavoritesListView->GetFavoriteItemFromListView(
			ParentFavoriteFolder,pnmlvdi->item.iItem);

		if(variantFavorite.type() == typeid(CFavoriteFolder))
		{
			CFavoriteFolder &FavoriteFolder = boost::get<CFavoriteFolder>(variantFavorite);
			FavoriteFolder.SetName(pnmlvdi->item.pszText);
		}
		else if(variantFavorite.type() == typeid(CFavorite))
		{
			CFavorite &Favorite = boost::get<CFavorite>(variantFavorite);
			Favorite.SetName(pnmlvdi->item.pszText);
		}

		SetWindowLongPtr(m_hDlg,DWLP_MSGRESULT,TRUE);
		return TRUE;
	}

	SetWindowLongPtr(m_hDlg,DWLP_MSGRESULT,FALSE);
	return FALSE;
}

void CManageFavoritesDialog::OnLvnKeyDown(NMLVKEYDOWN *pnmlvkd)
{
	switch(pnmlvkd->wVKey)
	{
	case VK_F2:
		OnListViewRename();
		break;

	case 'A':
		if((GetKeyState(VK_CONTROL) & 0x80) &&
			!(GetKeyState(VK_SHIFT) & 0x80) &&
			!(GetKeyState(VK_MENU) & 0x80))
		{
			HWND hListView = GetDlgItem(m_hDlg,IDC_MANAGEFAVORITES_LISTVIEW);
			NListView::ListView_SelectAllItems(hListView,TRUE);
			SetFocus(hListView);
		}
		break;
	}
}

void CManageFavoritesDialog::OnListViewRename()
{
	HWND hListView = GetDlgItem(m_hDlg,IDC_MANAGEFAVORITES_LISTVIEW);
	int iItem = ListView_GetNextItem(hListView,-1,LVNI_SELECTED);

	if(iItem != -1)
	{
		ListView_EditLabel(hListView,iItem);
	}
}

void CManageFavoritesDialog::GetColumnString(CManageFavoritesDialogPersistentSettings::ColumnType_t ColumnType,
	TCHAR *szColumn,UINT cchBuf)
{
	UINT uResourceID = 0;

	switch(ColumnType)
	{
	case CManageFavoritesDialogPersistentSettings::COLUMN_TYPE_NAME:
		uResourceID = IDS_MANAGE_FAVORITES_COLUMN_NAME;
		break;

	case CManageFavoritesDialogPersistentSettings::COLUMN_TYPE_LOCATION:
		uResourceID = IDS_MANAGE_FAVORITES_COLUMN_LOCATION;
		break;

	case CManageFavoritesDialogPersistentSettings::COLUMN_TYPE_VISIT_DATE:
		uResourceID = IDS_MANAGE_FAVORITES_COLUMN_VISIT_DATE;
		break;

	case CManageFavoritesDialogPersistentSettings::COLUMN_TYPE_VISIT_COUNT:
		uResourceID = IDS_MANAGE_FAVORITES_COLUMN_VISIT_COUNT;
		break;

	case CManageFavoritesDialogPersistentSettings::COLUMN_TYPE_ADDED:
		uResourceID = IDS_MANAGE_FAVORITES_COLUMN_ADDED;
		break;

	case CManageFavoritesDialogPersistentSettings::COLUMN_TYPE_LAST_MODIFIED:
		uResourceID = IDS_MANAGE_FAVORITES_COLUMN_LAST_MODIFIED;
		break;

	default:
		assert(FALSE);
		break;
	}

	LoadString(GetInstance(),uResourceID,szColumn,cchBuf);
}

void CManageFavoritesDialog::GetFavoriteItemColumnInfo(const NFavoritesHelper::variantFavorite_t variantFavorite,
	CManageFavoritesDialogPersistentSettings::ColumnType_t ColumnType,TCHAR *szColumn,size_t cchBuf)
{
	if(variantFavorite.type() == typeid(CFavoriteFolder))
	{
		const CFavoriteFolder &FavoriteFolder = boost::get<CFavoriteFolder>(variantFavorite);
		GetFavoriteFolderColumnInfo(FavoriteFolder,ColumnType,szColumn,cchBuf);
	}
	else
	{
		const CFavorite &Favorite = boost::get<CFavorite>(variantFavorite);
		GetFavoriteColumnInfo(Favorite,ColumnType,szColumn,cchBuf);
	}
}

void CManageFavoritesDialog::GetFavoriteColumnInfo(const CFavorite &Favorite,
	CManageFavoritesDialogPersistentSettings::ColumnType_t ColumnType,
	TCHAR *szColumn,size_t cchBuf)
{
	switch(ColumnType)
	{
	case CManageFavoritesDialogPersistentSettings::COLUMN_TYPE_NAME:
		StringCchCopy(szColumn,cchBuf,Favorite.GetName().c_str());
		break;

	case CManageFavoritesDialogPersistentSettings::COLUMN_TYPE_LOCATION:
		StringCchCopy(szColumn,cchBuf,Favorite.GetLocation().c_str());
		break;

	case CManageFavoritesDialogPersistentSettings::COLUMN_TYPE_VISIT_DATE:
		{
			/* TODO: Friendly dates. */
			FILETIME ftLastVisited = Favorite.GetDateLastVisited();
			CreateFileTimeString(&ftLastVisited,szColumn,static_cast<int>(cchBuf),FALSE);
		}
		break;

	case CManageFavoritesDialogPersistentSettings::COLUMN_TYPE_VISIT_COUNT:
		StringCchPrintf(szColumn,cchBuf,_T("%d"),Favorite.GetVisitCount());
		break;

	case CManageFavoritesDialogPersistentSettings::COLUMN_TYPE_ADDED:
		{
			/* TODO: Friendly dates. */
			FILETIME ftCreated = Favorite.GetDateCreated();
			CreateFileTimeString(&ftCreated,szColumn,static_cast<int>(cchBuf),FALSE);
		}
		break;

	case CManageFavoritesDialogPersistentSettings::COLUMN_TYPE_LAST_MODIFIED:
		{
			/* TODO: Friendly dates. */
			FILETIME ftModified = Favorite.GetDateModified();
			CreateFileTimeString(&ftModified,szColumn,static_cast<int>(cchBuf),FALSE);
		}
		break;

	default:
		assert(FALSE);
		break;
	}
}

void CManageFavoritesDialog::GetFavoriteFolderColumnInfo(const CFavoriteFolder &FavoriteFolder,
	CManageFavoritesDialogPersistentSettings::ColumnType_t ColumnType,TCHAR *szColumn,size_t cchBuf)
{
	switch(ColumnType)
	{
	case CManageFavoritesDialogPersistentSettings::COLUMN_TYPE_NAME:
		StringCchCopy(szColumn,cchBuf,FavoriteFolder.GetName().c_str());
		break;

	case CManageFavoritesDialogPersistentSettings::COLUMN_TYPE_LOCATION:
		StringCchCopy(szColumn,cchBuf,EMPTY_STRING);
		break;

	case CManageFavoritesDialogPersistentSettings::COLUMN_TYPE_VISIT_DATE:
		StringCchCopy(szColumn,cchBuf,EMPTY_STRING);
		break;

	case CManageFavoritesDialogPersistentSettings::COLUMN_TYPE_VISIT_COUNT:
		StringCchCopy(szColumn,cchBuf,EMPTY_STRING);
		break;

	case CManageFavoritesDialogPersistentSettings::COLUMN_TYPE_ADDED:
		{
			/* TODO: Friendly dates. */
			FILETIME ftCreated = FavoriteFolder.GetDateCreated();
			CreateFileTimeString(&ftCreated,szColumn,static_cast<int>(cchBuf),FALSE);
		}
		break;

	case CManageFavoritesDialogPersistentSettings::COLUMN_TYPE_LAST_MODIFIED:
		{
			/* TODO: Friendly dates. */
			FILETIME ftModified = FavoriteFolder.GetDateModified();
			CreateFileTimeString(&ftModified,szColumn,static_cast<int>(cchBuf),FALSE);
		}
		break;

	default:
		assert(FALSE);
		break;
	}
}

void CManageFavoritesDialog::OnTbnDropDown(NMTOOLBAR *nmtb)
{
	switch(nmtb->iItem)
	{
	case TOOLBAR_ID_VIEWS:
		ShowViewMenu();
		break;

	case TOOLBAR_ID_ORGANIZE:
		ShowOrganizeMenu();
		break;
	}
}

void CManageFavoritesDialog::ShowViewMenu()
{
	DWORD dwButtonState = static_cast<DWORD>(SendMessage(m_hToolbar,TB_GETSTATE,TOOLBAR_ID_VIEWS,MAKEWORD(TBSTATE_PRESSED,0)));
	SendMessage(m_hToolbar,TB_SETSTATE,TOOLBAR_ID_VIEWS,MAKEWORD(dwButtonState|TBSTATE_PRESSED,0));

	HMENU hMenu = LoadMenu(GetInstance(),MAKEINTRESOURCE(IDR_MANAGEFAVORITES_VIEW_MENU));

	UINT uCheck;

	if(m_pmbdps->m_bSortAscending)
	{
		uCheck = IDM_MB_VIEW_SORTASCENDING;
	}
	else
	{
		uCheck = IDM_MB_VIEW_SORTDESCENDING;
	}

	CheckMenuRadioItem(hMenu,IDM_MB_VIEW_SORTASCENDING,IDM_MB_VIEW_SORTDESCENDING,uCheck,MF_BYCOMMAND);

	switch(m_pmbdps->m_SortMode)
	{
	case NFavoritesHelper::SM_NAME:
		uCheck = IDM_MB_VIEW_SORTBYNAME;
		break;

	case NFavoritesHelper::SM_LOCATION:
		uCheck = IDM_MB_VIEW_SORTBYLOCATION;
		break;

	case NFavoritesHelper::SM_VISIT_DATE:
		uCheck = IDM_MB_VIEW_SORTBYVISITDATE;
		break;

	case NFavoritesHelper::SM_VISIT_COUNT:
		uCheck = IDM_MB_VIEW_SORTBYVISITCOUNT;
		break;

	case NFavoritesHelper::SM_ADDED:
		uCheck = IDM_MB_VIEW_SORTBYADDED;
		break;

	case NFavoritesHelper::SM_LAST_MODIFIED:
		uCheck = IDM_MB_VIEW_SORTBYLASTMODIFIED;
		break;
	}

	CheckMenuRadioItem(hMenu,IDM_MB_VIEW_SORTBYNAME,IDM_MB_VIEW_SORTBYLASTMODIFIED,uCheck,MF_BYCOMMAND);

	RECT rcButton;
	SendMessage(m_hToolbar,TB_GETRECT,TOOLBAR_ID_VIEWS,reinterpret_cast<LPARAM>(&rcButton));

	POINT pt;
	pt.x = rcButton.left;
	pt.y = rcButton.bottom;
	ClientToScreen(m_hToolbar,&pt);

	TrackPopupMenu(GetSubMenu(hMenu,0),TPM_LEFTALIGN,pt.x,pt.y,0,m_hDlg,NULL);
	DestroyMenu(hMenu);

	SendMessage(m_hToolbar,TB_SETSTATE,TOOLBAR_ID_VIEWS,MAKEWORD(dwButtonState,0));
}

void CManageFavoritesDialog::ShowOrganizeMenu()
{
	DWORD dwButtonState = static_cast<DWORD>(SendMessage(m_hToolbar,TB_GETSTATE,TOOLBAR_ID_ORGANIZE,MAKEWORD(TBSTATE_PRESSED,0)));
	SendMessage(m_hToolbar,TB_SETSTATE,TOOLBAR_ID_ORGANIZE,MAKEWORD(dwButtonState|TBSTATE_PRESSED,0));

	HMENU hMenu = LoadMenu(GetInstance(),MAKEINTRESOURCE(IDR_MANAGEFAVORITES_ORGANIZE_MENU));

	RECT rcButton;
	SendMessage(m_hToolbar,TB_GETRECT,TOOLBAR_ID_ORGANIZE,reinterpret_cast<LPARAM>(&rcButton));

	POINT pt;
	pt.x = rcButton.left;
	pt.y = rcButton.bottom;
	ClientToScreen(m_hToolbar,&pt);

	TrackPopupMenu(GetSubMenu(hMenu,0),TPM_LEFTALIGN,pt.x,pt.y,0,m_hDlg,NULL);
	DestroyMenu(hMenu);

	SendMessage(m_hToolbar,TB_SETSTATE,TOOLBAR_ID_ORGANIZE,MAKEWORD(dwButtonState,0));
}

void CManageFavoritesDialog::OnTvnSelChanged(NMTREEVIEW *pnmtv)
{
	/* This message will come in once before the listview has been
	properly initialized (due to the selection been set in
	the treeview), and can be ignored. */
	if(!m_bListViewInitialized)
	{
		return;
	}

	CFavoriteFolder &FavoriteFolder = m_pFavoritesTreeView->GetFavoriteFolderFromTreeView(pnmtv->itemNew.hItem);

	if(IsEqualGUID(FavoriteFolder.GetGUID(),m_guidCurrentFolder))
	{
		return;
	}

	BrowseFavoriteFolder(FavoriteFolder);
}

void CManageFavoritesDialog::OnDblClk(NMHDR *pnmhdr)
{
	HWND hListView = GetDlgItem(m_hDlg,IDC_MANAGEFAVORITES_LISTVIEW);

	if(pnmhdr->hwndFrom == hListView)
	{
		NMITEMACTIVATE *pnmia = reinterpret_cast<NMITEMACTIVATE *>(pnmhdr);

		HWND hTreeView = GetDlgItem(m_hDlg,IDC_MANAGEFAVORITES_TREEVIEW);
		HTREEITEM hSelected = TreeView_GetSelection(hTreeView);
		CFavoriteFolder &ParentFavoriteFolder = m_pFavoritesTreeView->GetFavoriteFolderFromTreeView(hSelected);
		NFavoritesHelper::variantFavorite_t variantFavorite = m_pFavoritesListView->GetFavoriteItemFromListView(
			ParentFavoriteFolder,pnmia->iItem);

		if(variantFavorite.type() == typeid(CFavoriteFolder))
		{
			CFavoriteFolder &FavoriteFolder = boost::get<CFavoriteFolder>(variantFavorite);
			m_pFavoritesListView->InsertFavoritesIntoListView(FavoriteFolder);

			/* TODO: Change treeview selection. */
		}
		else if(variantFavorite.type() == typeid(CFavorite))
		{
			/* TODO: Send the Favorite back to the main
			window to open. */
		}
	}
}

void CManageFavoritesDialog::BrowseFavoriteFolder(const CFavoriteFolder &FavoriteFolder)
{
	m_stackBack.push(m_guidCurrentFolder);

	m_guidCurrentFolder = FavoriteFolder.GetGUID();
	m_pFavoritesTreeView->SelectFolder(FavoriteFolder.GetGUID());
	m_pFavoritesListView->InsertFavoritesIntoListView(FavoriteFolder);

	UpdateToolbarState();
}

void CManageFavoritesDialog::BrowseBack()
{
	if(m_stackBack.size() == 0)
	{
		return;
	}

	GUID guid = m_stackBack.top();
	m_stackBack.pop();
	m_stackForward.push(guid);

	/* TODO: Browse back. */
}

void CManageFavoritesDialog::BrowseForward()
{
	if(m_stackForward.size() == 0)
	{
		return;
	}
}

void CManageFavoritesDialog::UpdateToolbarState()
{
	SendMessage(m_hToolbar,TB_ENABLEBUTTON,TOOLBAR_ID_BACK,m_stackBack.size() != 0);
	SendMessage(m_hToolbar,TB_ENABLEBUTTON,TOOLBAR_ID_FORWARD,m_stackForward.size() != 0);
}

void CManageFavoritesDialog::OnFavoriteItemModified(const GUID &guid)
{
	m_pFavoritesTreeView->FavoriteFolderModified(guid);
}

void CManageFavoritesDialog::OnFavoriteAdded(const CFavoriteFolder &ParentFavoriteFolder,const CFavorite &Favorite)
{
}

void CManageFavoritesDialog::OnFavoriteFolderAdded(const CFavoriteFolder &ParentFavoriteFolder,const CFavoriteFolder &FavoriteFolder)
{
	m_pFavoritesTreeView->FavoriteFolderAdded(ParentFavoriteFolder,FavoriteFolder);

	if(IsEqualGUID(ParentFavoriteFolder.GetGUID(),m_guidCurrentFolder))
	{
		int iItem = m_pFavoritesListView->InsertFavoriteFolderIntoListView(FavoriteFolder);

		if(IsEqualGUID(FavoriteFolder.GetGUID(),m_guidNewFolder))
		{
			HWND hListView = GetDlgItem(m_hDlg,IDC_MANAGEFAVORITES_LISTVIEW);

			SetFocus(hListView);
			NListView::ListView_SelectAllItems(hListView,FALSE);
			NListView::ListView_SelectItem(hListView,iItem,TRUE);
			ListView_EditLabel(hListView,iItem);

			m_bNewFolderAdded = false;
		}
}

void CManageFavoritesDialog::OnFavoriteRemoved(const GUID &guid)
{

}

void CManageFavoritesDialog::OnFavoriteFolderRemoved(const GUID &guid)
{

}

void CManageFavoritesDialog::OnRClick(NMHDR *pnmhdr)
{
	HWND hListView = GetDlgItem(m_hDlg,IDC_MANAGEFAVORITES_LISTVIEW);
	HWND hTreeView = GetDlgItem(m_hDlg,IDC_MANAGEFAVORITES_TREEVIEW);

	if(pnmhdr->hwndFrom == hListView)
	{
		OnListViewRClick();
	}
	else if(pnmhdr->hwndFrom == ListView_GetHeader(hListView))
	{
		OnListViewHeaderRClick();
	}
	else if(pnmhdr->hwndFrom == hTreeView)
	{

	}
}

void CManageFavoritesDialog::OnOk()
{
	EndDialog(m_hDlg,1);
}

void CManageFavoritesDialog::OnCancel()
{
	EndDialog(m_hDlg,0);
}

BOOL CManageFavoritesDialog::OnClose()
{
	EndDialog(m_hDlg,0);
	return 0;
}

BOOL CManageFavoritesDialog::OnDestroy()
{
	CFavoriteItemNotifier::GetInstance().RemoveObserver(this);
	DeleteFont(m_hEditSearchFont);
	ImageList_Destroy(m_himlToolbar);
	return 0;
}

BOOL CManageFavoritesDialog::OnNcDestroy()
{
	delete this;

	return 0;
}

void CManageFavoritesDialog::SaveState()
{
	m_pmbdps->SaveDialogPosition(m_hDlg);

	HWND hListView = GetDlgItem(m_hDlg,IDC_MANAGEFAVORITES_LISTVIEW);
	int iColumn = 0;

	for(auto itr = m_pmbdps->m_vectorColumnInfo.begin();itr != m_pmbdps->m_vectorColumnInfo.end();++itr)
	{
		if(itr->bActive)
		{
			itr->iWidth = ListView_GetColumnWidth(hListView,iColumn);
			++iColumn;
		}
	}

	m_pmbdps->m_bStateSaved = TRUE;
}

CManageFavoritesDialogPersistentSettings::CManageFavoritesDialogPersistentSettings() :
m_bInitialized(false),
m_SortMode(NFavoritesHelper::SM_NAME),
m_bSortAscending(true),
CDialogSettings(SETTINGS_KEY)
{
	SetupDefaultColumns();

	/* TODO: Save listview selection information. */
}

CManageFavoritesDialogPersistentSettings::~CManageFavoritesDialogPersistentSettings()
{

}

CManageFavoritesDialogPersistentSettings& CManageFavoritesDialogPersistentSettings::GetInstance()
{
	static CManageFavoritesDialogPersistentSettings mbdps;
	return mbdps;
}

void CManageFavoritesDialogPersistentSettings::SetupDefaultColumns()
{
	ColumnInfo_t ci;

	ci.ColumnType	= COLUMN_TYPE_NAME;
	ci.iWidth		= DEFAULT_MANAGE_FAVORITES_COLUMN_WIDTH;
	ci.bActive		= TRUE;
	m_vectorColumnInfo.push_back(ci);

	ci.ColumnType	= COLUMN_TYPE_LOCATION;
	ci.iWidth		= DEFAULT_MANAGE_FAVORITES_COLUMN_WIDTH;
	ci.bActive		= TRUE;
	m_vectorColumnInfo.push_back(ci);

	ci.ColumnType	= COLUMN_TYPE_VISIT_DATE;
	ci.iWidth		= DEFAULT_MANAGE_FAVORITES_COLUMN_WIDTH;
	ci.bActive		= FALSE;
	m_vectorColumnInfo.push_back(ci);

	ci.ColumnType	= COLUMN_TYPE_VISIT_COUNT;
	ci.iWidth		= DEFAULT_MANAGE_FAVORITES_COLUMN_WIDTH;
	ci.bActive		= FALSE;
	m_vectorColumnInfo.push_back(ci);

	ci.ColumnType	= COLUMN_TYPE_ADDED;
	ci.iWidth		= DEFAULT_MANAGE_FAVORITES_COLUMN_WIDTH;
	ci.bActive		= FALSE;
	m_vectorColumnInfo.push_back(ci);

	ci.ColumnType	= COLUMN_TYPE_LAST_MODIFIED;
	ci.iWidth		= DEFAULT_MANAGE_FAVORITES_COLUMN_WIDTH;
	ci.bActive		= FALSE;
	m_vectorColumnInfo.push_back(ci);
} 