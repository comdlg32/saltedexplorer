/******************************************************************
 *
 * Project: SaltedExplorer
 * File: TabHandler.cpp
 *
 * Provides tab management as well as the
 * handling of messages associated with the tabs.
 *
 * Toiletflusher and XP Pro
 * www.saltedexplorer.ml
 *
 *****************************************************************/

#include "stdafx.h"
#include <list>
#include "SaltedExplorer.h"
#include "RenameTabDialog.h"
#include "../Helper/FileOperations.h"
#include "../Helper/Helper.h"
#include "../Helper/Controls.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Macros.h"
#include "MainResource.h"


DWORD ListViewStyles		=	WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|
								LVS_ICON|LVS_EDITLABELS|LVS_SHOWSELALWAYS|LVS_SHAREIMAGELISTS|
								LVS_AUTOARRANGE|WS_TABSTOP|LVS_ALIGNTOP;

UINT TabCtrlStyles			=	WS_VISIBLE|WS_CHILD|TCS_FOCUSNEVER|TCS_SINGLELINE|
								TCS_TOOLTIPS|WS_CLIPSIBLINGS|WS_CLIPCHILDREN;

extern LRESULT CALLBACK	ListViewSubclassProcStub(HWND ListView,UINT msg,WPARAM wParam,LPARAM lParam);
extern LRESULT	(CALLBACK *DefaultListViewProc)(HWND,UINT,WPARAM,LPARAM);

std::wstring SaltedExplorer::GetTabName(int iTab)
{
	TCITEM tcItem;
	tcItem.mask = TCIF_PARAM;
	TabCtrl_GetItem(m_hTabCtrl,iTab,&tcItem);

	return std::wstring(m_TabInfo[static_cast<int>(tcItem.lParam)].szName);
}

void SaltedExplorer::SetTabName(int iTab,std::wstring strName,BOOL bUseCustomName)
{
	TCITEM tcItem;
	tcItem.mask = TCIF_PARAM;
	TabCtrl_GetItem(m_hTabCtrl,iTab,&tcItem);

	StringCchCopy(m_TabInfo[static_cast<int>(tcItem.lParam)].szName,
		SIZEOF_ARRAY(m_TabInfo[static_cast<int>(tcItem.lParam)].szName),strName.c_str());
	m_TabInfo[static_cast<int>(tcItem.lParam)].bUseCustomName = bUseCustomName;

	TCHAR szName[256];
	StringCchCopy(szName,SIZEOF_ARRAY(szName),strName.c_str());

	tcItem.mask = TCIF_TEXT;
	tcItem.pszText = szName;
	TabCtrl_SetItem(m_hTabCtrl,iTab,&tcItem);
}

void SaltedExplorer::InitializeTabMap(void)
{
	int i = 0;

	for(i = 0;i < MAX_TABS;i++)
	{
		m_uTabMap[i] = 0;
	}
}

void SaltedExplorer::ReleaseTabId(int iTabId)
{
	m_uTabMap[iTabId] = 0;
}

BOOL SaltedExplorer::CheckTabIdStatus(int iTabId)
{
	if(m_uTabMap[iTabId] == 0)
		return FALSE;

	return TRUE;
}

int SaltedExplorer::GenerateUniqueTabId(void)
{
	BOOL	bFound = FALSE;
	int		i = 0;

	for(i = 0;i < MAX_TABS;i++)
	{
		if(m_uTabMap[i] == 0)
		{
			m_uTabMap[i] = 1;
			bFound = TRUE;
			break;
		}
	}

	if(bFound)
		return i;
	else
		return -1;
}

HRESULT SaltedExplorer::CreateNewTab(TCHAR *TabDirectory,
InitialSettings_t *pSettings,TabInfo_t *pTabInfo,BOOL bSwitchToNewTab,
int *pTabObjectIndex)
{
	LPITEMIDLIST	pidl = NULL;
	TCHAR			szExpandedPath[MAX_PATH];
	HRESULT			hr;
	BOOL			bRet;

	/* Attempt to expand the path (in the event that
	it contains embedded environment variables). */
	bRet = MyExpandEnvironmentStrings(TabDirectory,
		szExpandedPath,SIZEOF_ARRAY(szExpandedPath));

	if(!bRet)
	{
		StringCchCopy(szExpandedPath,
			SIZEOF_ARRAY(szExpandedPath),TabDirectory);
	}

	if(!SUCCEEDED(GetIdlFromParsingName(szExpandedPath,&pidl)))
		return E_FAIL;

	hr = CreateNewTab(pidl,pSettings,pTabInfo,bSwitchToNewTab,pTabObjectIndex);

	CoTaskMemFree(pidl);

	return hr;
}

/* Creates a new tab. If the settings argument is NULL,
the global settings will be used. */
HRESULT SaltedExplorer::CreateNewTab(LPITEMIDLIST pidlDirectory,
InitialSettings_t *pSettings,TabInfo_t *pTabInfo,BOOL bSwitchToNewTab,
int *pTabObjectIndex)
{
	UINT				uFlags;
	HRESULT				hr;
	InitialSettings_t	is;
	int					iNewTabIndex;
	int					iTabId;

	if(!CheckIdl(pidlDirectory) || !IsIdlDirectory(pidlDirectory))
		return E_FAIL;

	if(m_bOpenNewTabNextToCurrent)
		iNewTabIndex = m_iTabSelectedItem + 1;
	else
		iNewTabIndex = TabCtrl_GetItemCount(m_hTabCtrl);

	iTabId = GenerateUniqueTabId();

	if(iTabId == -1)
		return E_FAIL;

	if(pTabInfo == NULL)
	{
		m_TabInfo[iTabId].bLocked			= FALSE;
		m_TabInfo[iTabId].bAddressLocked	= FALSE;
		m_TabInfo[iTabId].bUseCustomName	= FALSE;
	}
	else
	{
		m_TabInfo[iTabId] = *pTabInfo;
	}

	m_hListView[iTabId]	= CreateAndSubclassListView(m_hContainer,ListViewStyles);

	if(m_hListView[iTabId] == NULL)
		return E_FAIL;

	NListView::ListView_ActivateOneClickSelect(m_hListView[iTabId],m_bOneClickActivate,m_OneClickActivateHoverTime);

	/* Set the listview to its initial size. */
	SetListViewInitialPosition(m_hListView[iTabId]);

	/* If no explicit settings are specified, use the
	global ones. */
	if(pSettings == NULL)
	{
		BOOL bFound = FALSE;

		/* These settings are program-wide. */
		is.bGridlinesActive		= m_bShowGridlinesGlobal;
		is.bShowHidden			= m_bShowHiddenGlobal;
		is.bShowInGroups		= m_bShowInGroupsGlobal;
		is.bSortAscending		= m_bSortAscendingGlobal;
		is.bAutoArrange			= m_bAutoArrangeGlobal;
		is.bShowFolderSizes		= m_bShowFolderSizes;
		is.bDisableFolderSizesNetworkRemovable = m_bDisableFolderSizesNetworkRemovable;
		is.bHideSystemFiles		= m_bHideSystemFilesGlobal;
		is.bHideLinkExtension	= m_bHideLinkExtensionGlobal;

		/* Check if there are any specific settings saved
		for the specified directory. */
		for each(auto ds in m_DirectorySettingsList)
		{
			if(CompareIdls(pidlDirectory,ds.pidlDirectory))
			{
				/* TODO: */
				//bFound = TRUE;

				is.SortMode				= ds.dsi.SortMode;
				is.ViewMode				= ds.dsi.ViewMode;
				is.bApplyFilter			= FALSE;
				is.bFilterCaseSensitive	= FALSE;

				is.pControlPanelColumnList			= &ds.dsi.ControlPanelColumnList;
				is.pMyComputerColumnList			= &ds.dsi.MyComputerColumnList;
				is.pMyNetworkPlacesColumnList		= &ds.dsi.MyNetworkPlacesColumnList;
				is.pNetworkConnectionsColumnList	= &ds.dsi.NetworkConnectionsColumnList;
				is.pPrintersColumnList				= &ds.dsi.PrintersColumnList;
				is.pRealFolderColumnList			= &ds.dsi.RealFolderColumnList;
				is.pRecycleBinColumnList			= &ds.dsi.RecycleBinColumnList;
			}
		}

		if(bFound)
		{
			/* There are existing settings for this directory,
			so use those, rather than the defaults. */
		}
		else
		{
			is.SortMode				= DEFAULT_SORT_MODE;
			is.ViewMode				= m_ViewModeGlobal;
			is.bApplyFilter			= FALSE;
			is.bFilterCaseSensitive	= FALSE;

			StringCchCopy(is.szFilter,SIZEOF_ARRAY(is.szFilter),EMPTY_STRING);

			is.pControlPanelColumnList			= &m_ControlPanelColumnList;
			is.pMyComputerColumnList			= &m_MyComputerColumnList;
			is.pMyNetworkPlacesColumnList		= &m_MyNetworkPlacesColumnList;
			is.pNetworkConnectionsColumnList	= &m_NetworkConnectionsColumnList;
			is.pPrintersColumnList				= &m_PrintersColumnList;
			is.pRealFolderColumnList			= &m_RealFolderColumnList;
			is.pRecycleBinColumnList			= &m_RecycleBinColumnList;
		}

		pSettings = &is;
	}

	pSettings->bForceSize	= m_bForceSize;
	pSettings->sdf			= m_SizeDisplayFormat;

	InitializeFolderView(m_hContainer,m_hListView[iTabId],
	&m_pFolderView[iTabId],pSettings,m_hIconThread,m_hFolderSizeThread);

	if(pSettings->bApplyFilter)
		NListView::ListView_SetBackgroundImage(m_hListView[iTabId],IDB_FILTERINGAPPLIED);

	ListViewInfo_t	*plvi = NULL;

	plvi = (ListViewInfo_t *)malloc(sizeof(ListViewInfo_t));

	plvi->pContainer	= this;
	plvi->iObjectIndex	= iTabId;

	SetWindowLongPtr(m_hListView[iTabId],GWLP_USERDATA,(LONG_PTR)plvi);

	/* Subclass the window. */
	DefaultListViewProc = (WNDPROC)SetWindowLongPtr(m_hListView[iTabId],GWLP_WNDPROC,(LONG_PTR)ListViewSubclassProcStub);

	m_pFolderView[iTabId]->QueryInterface(IID_IShellBrowser,
	(void **)&m_pShellBrowser[iTabId]);

	m_pFolderView[iTabId]->SetId(iTabId);
	m_pFolderView[iTabId]->SetResourceModule(g_hLanguageModule);

	m_pShellBrowser[iTabId]->SetHideSystemFiles(m_bHideSystemFilesGlobal);
	m_pShellBrowser[iTabId]->SetShowExtensions(m_bShowExtensionsGlobal);
	m_pShellBrowser[iTabId]->SetHideLinkExtension(m_bHideLinkExtensionGlobal);
	m_pShellBrowser[iTabId]->SetShowFolderSizes(m_bShowFolderSizes);
	m_pShellBrowser[iTabId]->SetShowFriendlyDates(m_bShowFriendlyDatesGlobal);
	m_pShellBrowser[iTabId]->SetInsertSorted(m_bInsertSorted);

	/* Browse folder sends a message back to the main window, which
	attempts to contact the new tab (needs to be created before browsing
	the folder). */
	InsertNewTab(pidlDirectory,iNewTabIndex,iTabId);

	if(bSwitchToNewTab)
	{
		/* Select the newly created tab. */
		TabCtrl_SetCurSel(m_hTabCtrl,iNewTabIndex);

		/* Hide the previously active tab, and show the
		newly created one. */
		ShowWindow(m_hActiveListView,SW_HIDE);
		ShowWindow(m_hListView[iTabId],SW_SHOW);

		m_iObjectIndex			= iTabId;
		m_iTabSelectedItem		= iNewTabIndex;

		m_hActiveListView		= m_hListView[m_iObjectIndex];
		m_pActiveShellBrowser	= m_pShellBrowser[m_iObjectIndex];

		SetFocus(m_hListView[iTabId]);
	}

	/* SBSP_SAMEBROWSER is used internally. Ignored
	by the shellbrowser. */
	uFlags = SBSP_ABSOLUTE;

	/* These settings are applied to all tabs (i.e. they
	are not tab specific). Send them to the browser
	regardless of whether it loads its own settings or not. */
	PushGlobalSettingsToTab(iTabId);

	hr = m_pShellBrowser[iTabId]->BrowseFolder(pidlDirectory,uFlags);

	if(bSwitchToNewTab)
		m_pShellBrowser[iTabId]->QueryCurrentDirectory(MAX_PATH,m_CurrentDirectory);

	if(hr != S_OK)
	{
		/* Folder was not browsed. Likely that the path does not exist
		(or is locked, cannot be found, etc). */
		return E_FAIL;
	}

	if(pTabObjectIndex != NULL)
		*pTabObjectIndex = iTabId;

	/* If we're running on Windows 7, we'll create
	a proxy window for each tab. This proxy window
	will create the taskbar thumbnail for that tab. */
	CreateTabProxy(pidlDirectory,iTabId,bSwitchToNewTab);

	return S_OK;
}

void SaltedExplorer::OnTabChangeInternal(BOOL bSetFocus)
{
	TCITEM tcItem;

	tcItem.mask = TCIF_PARAM;
	TabCtrl_GetItem(m_hTabCtrl,m_iTabSelectedItem,&tcItem);

	/* Hide the old listview. */
	ShowWindow(m_hActiveListView,SW_HIDE);

	m_iObjectIndex = (int)tcItem.lParam;

	m_hActiveListView		= m_hListView[m_iObjectIndex];
	m_pActiveShellBrowser	= m_pShellBrowser[m_iObjectIndex];

	/* The selected tab has changed, so update the current
	directory. Although this is not needed internally, context
	menu extensions may need the current directory to be
	set correctly. */
	m_pActiveShellBrowser->QueryCurrentDirectory(SIZEOF_ARRAY(m_CurrentDirectory),
		m_CurrentDirectory);
	SetCurrentDirectory(m_CurrentDirectory);

	m_nSelected = m_pActiveShellBrowser->QueryNumSelected();

	SetActiveArrangeMenuItems();
	UpdateArrangeMenuItems();

	UpdateWindowStates();

	/* Show the new listview. */
	ShowWindow(m_hActiveListView,SW_SHOW);

	/* Inform the taskbar that this tab has become active. */
	if(m_bTaskbarInitialised)
	{
		std::list<TabProxyInfo_t>::iterator itr;

		for(itr = m_TabProxyList.begin();itr != m_TabProxyList.end();itr++)
		{
			if(itr->iTabId == m_iObjectIndex)
			{
				TCITEM tcItem;
				int nTabs;

				nTabs = TabCtrl_GetItemCount(m_hTabCtrl);

				/* POtentially the tab may have swapped position, so
				tell the taskbar to reposition it. */
				if(m_iTabSelectedItem == (nTabs - 1))
				{
					m_pTaskbarList3->SetTabOrder(itr->hProxy,NULL);
				}
				else
				{
					std::list<TabProxyInfo_t>::iterator itrNext;

					tcItem.mask = TCIF_PARAM;
					TabCtrl_GetItem(m_hTabCtrl,m_iTabSelectedItem + 1,&tcItem);

					for(itrNext = m_TabProxyList.begin();itrNext != m_TabProxyList.end();itrNext++)
					{
						if(itrNext->iTabId == (int)tcItem.lParam)
						{
							m_pTaskbarList3->SetTabOrder(itr->hProxy,itrNext->hProxy);
						}
					}
				}

				m_pTaskbarList3->SetTabActive(itr->hProxy,m_hContainer,0);
				break;
			}
		}
	}

	if(bSetFocus)
	{
		SetFocus(m_hActiveListView);
	}
}

void SaltedExplorer::RefreshAllTabs(void)
{
	int i = 0;
	int NumTabs;
	TCITEM tcItem;
	int iIndex;

	NumTabs = TabCtrl_GetItemCount(m_hTabCtrl);

	for(i = 0;i < NumTabs;i++)
	{
		tcItem.mask = TCIF_PARAM;
		TabCtrl_GetItem(m_hTabCtrl,i,&tcItem);
		iIndex = (int)tcItem.lParam;

		RefreshTab(iIndex);
	}
}

void SaltedExplorer::CloseOtherTabs(int iTab)
{
	int nTabs;
	int i = 0;

	nTabs = TabCtrl_GetItemCount(m_hTabCtrl);

	/* Close all tabs except the
	specified one. */
	for(i = nTabs - 1;i >= 0; i--)
	{
		if(i != iTab)
		{
			CloseTab(i);
		}
	}
}

void SaltedExplorer::SelectAdjacentTab(BOOL bNextTab)
{
	int nTabs;

	nTabs = TabCtrl_GetItemCount(m_hTabCtrl);

	if(bNextTab)
	{
		/* If this is the last tab in the order,
		wrap the selection back to the start. */
		if(m_iTabSelectedItem == (nTabs - 1))
			m_iTabSelectedItem = 0;
		else
			m_iTabSelectedItem++;
	}
	else
	{
		/* If this is the first tab in the order,
		wrap the selection back to the end. */
		if(m_iTabSelectedItem == 0)
			m_iTabSelectedItem = nTabs - 1;
		else
			m_iTabSelectedItem--;
	}

	TabCtrl_SetCurSel(m_hTabCtrl,m_iTabSelectedItem);

	OnTabChangeInternal(TRUE);
}

void SaltedExplorer::OnSelectTab(int iTab)
{
	return OnSelectTab(iTab,TRUE);
}

void SaltedExplorer::OnSelectTab(int iTab,BOOL bSetFocus)
{
	int nTabs;

	nTabs = TabCtrl_GetItemCount(m_hTabCtrl);

	if(iTab == -1)
	{
		m_iTabSelectedItem = nTabs - 1;
	}
	else
	{
		if(iTab < nTabs)
			m_iTabSelectedItem = iTab;
		else
			m_iTabSelectedItem = nTabs - 1;
	}

	TabCtrl_SetCurSel(m_hTabCtrl,m_iTabSelectedItem);

	OnTabChangeInternal(bSetFocus);
}

HRESULT SaltedExplorer::OnCloseTab(void)
{
	int iCurrentTab;

	iCurrentTab = TabCtrl_GetCurSel(m_hTabCtrl);

	return CloseTab(iCurrentTab);
}

HRESULT SaltedExplorer::CloseTab(int TabIndex)
{
	TCITEM	tcItem;
	int		NumTabs;
	int		m_iLastSelectedTab;
	int		ListViewIndex;
	int		iRemoveImage;

	m_iLastSelectedTab = TabIndex;

	NumTabs = TabCtrl_GetItemCount(m_hTabCtrl);

	if((NumTabs == 1))
	{
		if(m_bCloseMainWindowOnTabClose)
		{
			SendMessage(m_hContainer,WM_CLOSE,0,0);
		}

		return S_OK;
	}

	tcItem.mask = TCIF_IMAGE|TCIF_PARAM;
	TabCtrl_GetItem(m_hTabCtrl,TabIndex,&tcItem);
	iRemoveImage = tcItem.iImage;

	/* The tab is locked. Don't close it. */
	if(m_TabInfo[(int)tcItem.lParam].bLocked || m_TabInfo[(int)tcItem.lParam].bAddressLocked)
		return S_FALSE;

	ListViewIndex = (int)tcItem.lParam;

	EnterCriticalSection(&g_csDirMonCallback);
	ReleaseTabId(ListViewIndex);
	LeaveCriticalSection(&g_csDirMonCallback);

	NumTabs--;

	/*
	Cases:
	If the tab been closed is the active tab:
	 - If the first tab is been closed, then the selected
	   tab will still be the first tab.
	 - If the last tab is closed, then the selected tab
	   will be one less then the index of the previously
	   selected tab.
	 - Otherwise, the index of the selected tab will remain
	   unchanged (as a tab will be pushed down).
   If the tab been closed is not the active tab:
	 - If the index of the closed tab is less than the index
	   of the active tab, the index of the active tab will
	   decrease by one (as all higher tabs are pushed down
	   one space).
	*/
	if(TabIndex == m_iTabSelectedItem)
	{
		if(TabIndex == NumTabs)
		{
			m_iTabSelectedItem--;
			TabCtrl_SetCurSel(m_hTabCtrl,m_iTabSelectedItem);
			TabCtrl_DeleteItem(m_hTabCtrl,TabIndex);
		}
		else
		{
			TabCtrl_DeleteItem(m_hTabCtrl,TabIndex);
			TabCtrl_SetCurSel(m_hTabCtrl,m_iTabSelectedItem);
		}

		OnTabChangeInternal(TRUE);
	}
	else
	{
		TabCtrl_DeleteItem(m_hTabCtrl,TabIndex);

		if(TabIndex < m_iTabSelectedItem)
			m_iTabSelectedItem--;
	}

	/* Remove the tabs image from the image list. */
	TabCtrl_RemoveImage(m_hTabCtrl,iRemoveImage);

	std::list<TabProxyInfo_t>::iterator itr;

	if(m_bTaskbarInitialised)
	{
		for(itr = m_TabProxyList.begin();itr != m_TabProxyList.end();itr++)
		{
			if(itr->iTabId == ListViewIndex)
			{
				HICON hIcon;

				m_pTaskbarList3->UnregisterTab(itr->hProxy);

				TabProxy_t *ptp = (TabProxy_t *)GetWindowLongPtr(itr->hProxy,GWLP_USERDATA);
				DestroyWindow(itr->hProxy);
				free(ptp);

				hIcon = (HICON)GetClassLongPtr(itr->hProxy,GCLP_HICONSM);
				UnregisterClass((LPCWSTR)MAKEWORD(itr->atomClass,0),GetModuleHandle(0));
				DestroyIcon(hIcon);

				m_TabProxyList.erase(itr);
				break;
			}
		}
	}

	m_pDirMon->StopDirectoryMonitor(m_pShellBrowser[ListViewIndex]->GetDirMonitorId());

	m_pFolderView[ListViewIndex]->SetTerminationStatus();
	DestroyWindow(m_hListView[ListViewIndex]);

	m_pShellBrowser[ListViewIndex]->Release();
	m_pShellBrowser[ListViewIndex] = NULL;
	m_pFolderView[ListViewIndex]->Release();
	m_pFolderView[ListViewIndex] = NULL;

	HandleTabToolbarItemStates();

	if(!m_bAlwaysShowTabBar)
	{
		if(TabCtrl_GetItemCount(m_hTabCtrl) == 1)
		{
			RECT rc;

			m_bShowTabBar = FALSE;

			GetClientRect(m_hContainer,&rc);

			SendMessage(m_hContainer,WM_SIZE,SIZE_RESTORED,
				(LPARAM)MAKELPARAM(rc.right,rc.bottom));
		}
	}

	return S_OK;
}

void SaltedExplorer::RefreshTab(int iTabId)
{
	LPITEMIDLIST pidlDirectory = NULL;
	HRESULT hr;

	pidlDirectory = m_pShellBrowser[iTabId]->QueryCurrentDirectoryIdl();

	hr = m_pShellBrowser[iTabId]->BrowseFolder(pidlDirectory,
		SBSP_SAMEBROWSER|SBSP_ABSOLUTE|SBSP_WRITENOHISTORY);

	if(SUCCEEDED(hr))
		OnDirChanged(iTabId);

	CoTaskMemFree(pidlDirectory);
}

void SaltedExplorer::OnTabSelectionChange(void)
{
	m_iTabSelectedItem = TabCtrl_GetCurSel(m_hTabCtrl);

	OnTabChangeInternal(TRUE);
}

LRESULT CALLBACK TabSubclassProcStub(HWND hwnd,UINT uMsg,
WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	SaltedExplorer *pContainer = (SaltedExplorer *)dwRefData;

	return pContainer->TabSubclassProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK SaltedExplorer::TabSubclassProc(HWND hTab,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
		case WM_INITMENU:
			OnInitTabMenu(wParam);
			SendMessage(m_hContainer,WM_INITMENU,wParam,lParam);
			break;

		case WM_MENUSELECT:
			/* Forward the message to the main window so it can
			handle menu help. */
			SendMessage(m_hContainer,WM_MENUSELECT,wParam,lParam);
			break;

		case WM_MEASUREITEM:
			SendMessage(m_hContainer,WM_MEASUREITEM,wParam,lParam);
			break;

		case WM_DRAWITEM:
			SendMessage(m_hContainer,WM_DRAWITEM,wParam,lParam);
			break;

		case WM_LBUTTONDOWN:
			{
				POINT pt;
				POINTSTOPOINT(pt, MAKEPOINTS(lParam));
				OnTabCtrlLButtonDown(&pt);
			}
			break;

		case WM_LBUTTONUP:
			OnTabCtrlLButtonUp();
			break;

		case WM_MOUSEMOVE:
			{
				POINT pt;
				POINTSTOPOINT(pt, MAKEPOINTS(lParam));
				OnTabCtrlMouseMove(&pt);
			}
			break;

		case WM_MBUTTONUP:
			SendMessage(m_hContainer,WM_USER_TABMCLICK,wParam,lParam);
			break;

		case WM_RBUTTONUP:
			{
				POINT pt;
				POINTSTOPOINT(pt, MAKEPOINTS(lParam));
				OnTabCtrlRButtonUp(&pt);
			}
			break;

		case WM_CAPTURECHANGED:
			{
				if((HWND)lParam != hTab)
					ReleaseCapture();

				m_bTabBeenDragged = FALSE;
			}
			break;

		case WM_LBUTTONDBLCLK:
			{
				TCHITTESTINFO info;
				int ItemNum;
				DWORD dwPos;
				POINT MousePos;

				dwPos = GetMessagePos();
				MousePos.x = GET_X_LPARAM(dwPos);
				MousePos.y = GET_Y_LPARAM(dwPos);
				ScreenToClient(hTab,&MousePos);

				/* The cursor position will be tested to see if
				there is a tab beneath it. */
				info.pt.x	= LOWORD(lParam);
				info.pt.y	= HIWORD(lParam);

				ItemNum = TabCtrl_HitTest(m_hTabCtrl,&info);

				if(info.flags != TCHT_NOWHERE && m_bDoubleClickTabClose)
				{
					CloseTab(ItemNum);
				}
			}
			break;
	}

	return DefSubclassProc(hTab,msg,wParam,lParam);
}

void SaltedExplorer::OnInitTabMenu(WPARAM wParam)
{
	HMENU hTabMenu;
	TCITEM tcItem;

	tcItem.mask = TCIF_PARAM;
	TabCtrl_GetItem(m_hTabCtrl,m_iTabMenuItem,&tcItem);

	hTabMenu = (HMENU)wParam;

	lCheckMenuItem(hTabMenu,IDM_TAB_LOCKTAB,m_TabInfo[(int)tcItem.lParam].bLocked);
	lCheckMenuItem(hTabMenu,IDM_TAB_LOCKTABANDADDRESS,m_TabInfo[(int)tcItem.lParam].bAddressLocked);
	lEnableMenuItem(hTabMenu,IDM_TAB_CLOSETAB,
		!(m_TabInfo[(int)tcItem.lParam].bLocked || m_TabInfo[(int)tcItem.lParam].bAddressLocked));
}

void SaltedExplorer::OnTabCtrlLButtonDown(POINT *pt)
{
	TCHITTESTINFO info;
	info.pt = *pt;
	int ItemNum = TabCtrl_HitTest(m_hTabCtrl,&info);

	if(info.flags != TCHT_NOWHERE)
	{
		/* Save the bounds of the dragged tab. */
		TabCtrl_GetItemRect(m_hTabCtrl,ItemNum,&m_rcDraggedTab);

		/* Capture mouse movement exclusively until
		the mouse button is released. */
		SetCapture(m_hTabCtrl);

		m_bTabBeenDragged = TRUE;
	}
}

void SaltedExplorer::OnTabCtrlLButtonUp(void)
{
	if(GetCapture() == m_hTabCtrl)
		ReleaseCapture();

	m_bTabBeenDragged = FALSE;
}

void SaltedExplorer::OnTabCtrlMouseMove(POINT *pt)
{
	/* Is a tab currently been dragged? */
	if(m_bTabBeenDragged)
	{
		/* Dragged tab. */
		int iSelected = TabCtrl_GetCurFocus(m_hTabCtrl);

		TCHITTESTINFO HitTestInfo;
		HitTestInfo.pt = *pt;
		int iSwap = TabCtrl_HitTest(m_hTabCtrl,&HitTestInfo);

		/* Check:
		- If the cursor is over an item.
		- If the cursor is not over the dragged item itself.
		- If the cursor has passed to the left of the dragged tab, or
		- If the cursor has passed to the right of the dragged tab. */
		if(HitTestInfo.flags != TCHT_NOWHERE &&
			iSwap != iSelected &&
			(pt->x < m_rcDraggedTab.left ||
			pt->x > m_rcDraggedTab.right))
		{
			RECT rcSwap;

			TabCtrl_GetItemRect(m_hTabCtrl,iSwap,&rcSwap);

			/* These values need to be adjusted, since
			tabs are adjusted whenever the dragged tab
			passes a boundary, not when the cursor is
			released. */
			if(pt->x > m_rcDraggedTab.right)
			{
				/* Cursor has gone past the right edge of
				the dragged tab. */
				m_rcDraggedTab.left		= m_rcDraggedTab.right;
				m_rcDraggedTab.right	= rcSwap.right;
			}
			else
			{
				/* Cursor has gone past the left edge of
				the dragged tab. */
				m_rcDraggedTab.right	= m_rcDraggedTab.left;
				m_rcDraggedTab.left		= rcSwap.left;
			}

			/* Swap the dragged tab with the tab the cursor
			finished up on. */
			TabCtrl_SwapItems(m_hTabCtrl,iSelected,iSwap);

			/* The index of the selected tab has now changed
			(but the actual tab/browser selected remains the
			same). */
			m_iTabSelectedItem = iSwap;
			TabCtrl_SetCurFocus(m_hTabCtrl,iSwap);
		}
	}
}

void SaltedExplorer::OnTabCtrlRButtonUp(POINT *pt)
{
	TCHITTESTINFO tcHitTest;
	tcHitTest.pt = *pt;
	int iTabHit = TabCtrl_HitTest(m_hTabCtrl,&tcHitTest);

	if(tcHitTest.flags != TCHT_NOWHERE)
	{
		POINT ptCopy = *pt;
		ClientToScreen(m_hTabCtrl,&ptCopy);

		m_iTabMenuItem = iTabHit;

		UINT Command = TrackPopupMenu(m_hTabRightClickMenu,
		TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_VERTICAL | TPM_RETURNCMD,
		ptCopy.x,ptCopy.y,0,m_hTabCtrl,NULL);

		ProcessTabCommand(Command,iTabHit);
	}
}

void SaltedExplorer::ProcessTabCommand(UINT uMenuID,int iTabHit)
{
	switch(uMenuID)
	{
		case IDM_TAB_DUPLICATETAB:
			OnDuplicateTab(iTabHit);
			break;

		case IDM_TAB_OPENPARENTINNEWTAB:
			{
				TCITEM tcItem;
				tcItem.mask = TCIF_PARAM;
				TabCtrl_GetItem(m_hTabCtrl,iTabHit,&tcItem);

				LPITEMIDLIST pidlCurrent = m_pShellBrowser[static_cast<int>(tcItem.lParam)]->QueryCurrentDirectoryIdl();

				LPITEMIDLIST pidlParent = NULL;
				HRESULT hr = GetVirtualParentPath(pidlCurrent,&pidlParent);

				if(SUCCEEDED(hr))
				{
					BrowseFolder(pidlParent,SBSP_ABSOLUTE,TRUE,TRUE,FALSE);
					CoTaskMemFree(pidlParent);
				}

				CoTaskMemFree(pidlCurrent);
			}
			break;

		case IDM_TAB_REFRESH:
			RefreshTab(iTabHit);
			break;

		case IDM_TAB_REFRESHALL:
			RefreshAllTabs();
			break;

		case IDM_TAB_RENAMETAB:
			{
				CRenameTabDialog RenameTabDialog(g_hLanguageModule,IDD_RENAMETAB,m_hContainer,iTabHit,this);

				RenameTabDialog.ShowModalDialog();
			}
			break;

		case IDM_TAB_LOCKTAB:
			OnLockTab(iTabHit);
			break;

		case IDM_TAB_LOCKTABANDADDRESS:
			OnLockTabAndAddress(iTabHit);
			break;

		case IDM_TAB_CLOSEOTHERTABS:
			CloseOtherTabs(iTabHit);
			break;

		case IDM_TAB_CLOSETABSTORIGHT:
			{
				int nTabs = TabCtrl_GetItemCount(m_hTabCtrl);

				for(int i = nTabs - 1;i > iTabHit;i--)
				{
					CloseTab(i);
				}
			}
			break;

		case IDM_TAB_CLOSETAB:
			CloseTab(iTabHit);
			break;

		default:
			/* Send the resulting command back to the main window for processing. */
			SendMessage(m_hContainer,WM_COMMAND,MAKEWPARAM(uMenuID,iTabHit),0);
			break;
	}
}

void SaltedExplorer::InitializeTabs(void)
{
	HIMAGELIST	himlSmall;
	TCHAR		szTabCloseTip[64];
	HRESULT		hr;

	/* The tab backing will hold the tab window. */
	CreateTabBacking();

	if(m_bForceSameTabWidth)
		TabCtrlStyles |= TCS_FIXEDWIDTH;

	m_hTabCtrl = CreateTabControl(m_hTabBacking,TabCtrlStyles);

	himlSmall = ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,0,100);
	AddDefaultTabIcons(himlSmall);
	TabCtrl_SetImageList(m_hTabCtrl,himlSmall);

	/* Initialize the drag source helper, and use it to initialize the drop target helper. */
	hr = CoCreateInstance(CLSID_DragDropHelper,NULL,CLSCTX_INPROC_SERVER,
	IID_IDragSourceHelper,(LPVOID *)&m_pDragSourceHelper);

	if(SUCCEEDED(hr))
	{
		hr = m_pDragSourceHelper->QueryInterface(IID_IDropTargetHelper,(LPVOID *)&m_pDropTargetHelper);

		if(SUCCEEDED(hr))
		{
			/* Indicate that the tab control supports the dropping of items. */
			RegisterDragDrop(m_hTabCtrl,this);
		}
	}

	/* Subclass the tab control. */
	SetWindowSubclass(m_hTabCtrl,TabSubclassProcStub,0,(DWORD_PTR)this);

	/* Create the toolbar that will appear on the tab control.
	Only contains the close buton used to close tabs. */
	LoadString(g_hLanguageModule,IDS_TAB_CLOSE_TIP,
		szTabCloseTip,SIZEOF_ARRAY(szTabCloseTip));
	m_hTabWindowToolbar	= CreateTabToolbar(m_hTabBacking,TABTOOLBAR_CLOSE,szTabCloseTip);
}

void SaltedExplorer::AddDefaultTabIcons(HIMAGELIST himlTab)
{
	HIMAGELIST himlTemp;
	HBITMAP hBitmap;
	ICONINFO IconInfo;

	himlTemp = ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,0,48);

	hBitmap = LoadBitmap(GetModuleHandle(0),MAKEINTRESOURCE(IDB_SHELLIMAGES_2000));

	ImageList_Add(himlTemp,hBitmap,NULL);
	GetIconInfo(ImageList_GetIcon(himlTemp,SHELLIMAGES_LOCK,
		ILD_TRANSPARENT),&IconInfo);
	ImageList_Add(himlTab,IconInfo.hbmColor,IconInfo.hbmMask);

	DeleteObject(IconInfo.hbmColor);
	DeleteObject(IconInfo.hbmMask);
	ImageList_Destroy(himlTemp);
}

void SaltedExplorer::InsertNewTab(LPITEMIDLIST pidlDirectory,int iNewTabIndex,int iTabId)
{
	TCITEM		tcItem;
	TCHAR		szTabText[MAX_PATH];
	TCHAR		szExpandedTabText[MAX_PATH];

	/* If no custom name is set, use the folders name. */
	if(!m_TabInfo[iTabId].bUseCustomName)
	{
		GetDisplayName(pidlDirectory,szTabText,SHGDN_INFOLDER);

		StringCchCopy(m_TabInfo[iTabId].szName,
			SIZEOF_ARRAY(m_TabInfo[iTabId].szName),szTabText);
	}

	ReplaceCharacterWithString(m_TabInfo[iTabId].szName,szExpandedTabText,
		SIZEOF_ARRAY(szExpandedTabText),'&',_T("&&"));

	/* Tab control insertion information. The folders name will be used
	as the tab text. */
	tcItem.mask			= TCIF_TEXT|TCIF_PARAM;
	tcItem.pszText		= szExpandedTabText;
	tcItem.lParam		= iTabId;

	SendMessage(m_hTabCtrl,TCM_INSERTITEM,(WPARAM)iNewTabIndex,(LPARAM)&tcItem);

	SetTabIcon(iNewTabIndex,iTabId,pidlDirectory);

	if(!m_bAlwaysShowTabBar)
	{
		if(TabCtrl_GetItemCount(m_hTabCtrl) > 1)
		{
			RECT rc;

			m_bShowTabBar = TRUE;

			GetClientRect(m_hContainer,&rc);

			SendMessage(m_hContainer,WM_SIZE,SIZE_RESTORED,
				(LPARAM)MAKELPARAM(rc.right,rc.bottom));
		}
	}
}

void SaltedExplorer::OnDuplicateTab(int iTab)
{
	TCITEM tcItem;

	tcItem.mask = TCIF_PARAM;
	TabCtrl_GetItem(m_hTabCtrl,iTab,&tcItem);

	DuplicateTab((int)tcItem.lParam);
}

void SaltedExplorer::OnLockTab(int iTab)
{
	TCITEM tcItem;

	tcItem.mask = TCIF_PARAM;
	TabCtrl_GetItem(m_hTabCtrl,iTab,&tcItem);

	OnLockTabInternal(iTab,(int)tcItem.lParam);
}

void SaltedExplorer::OnLockTabInternal(int iTab,int iTabId)
{
	m_TabInfo[iTabId].bLocked = !m_TabInfo[iTabId].bLocked;

	/* The "Lock Tab" and "Lock Tab and Address" options
	are mutually exclusive. */
	if(m_TabInfo[iTabId].bLocked)
	{
		m_TabInfo[iTabId].bAddressLocked = FALSE;
	}

	SetTabIcon(iTab,iTabId);

	/* If the tab that was locked/unlocked is the
	currently selected tab, then the tab close
	button on the toolbar will need to be updated. */
	if(iTabId == m_iObjectIndex)
		HandleTabToolbarItemStates();
}

void SaltedExplorer::OnLockTabAndAddress(int iTab)
{
	TCITEM tcItem;

	tcItem.mask = TCIF_PARAM;
	TabCtrl_GetItem(m_hTabCtrl,iTab,&tcItem);

	m_TabInfo[(int)tcItem.lParam].bAddressLocked = !m_TabInfo[(int)tcItem.lParam].bAddressLocked;

	if(m_TabInfo[(int)tcItem.lParam].bAddressLocked)
	{
		m_TabInfo[(int)tcItem.lParam].bLocked = FALSE;
	}

	SetTabIcon(iTab,(int)tcItem.lParam);

	/* If the tab that was locked/unlocked is the
	currently selected tab, then the tab close
	button on the toolbar will need to be updated. */
	if((int)tcItem.lParam == m_iObjectIndex)
		HandleTabToolbarItemStates();
}

void SaltedExplorer::HandleTabToolbarItemStates(void)
{
	int nTabs;

	nTabs = TabCtrl_GetItemCount(m_hTabCtrl);

	if(nTabs > 1 && !(m_TabInfo[m_iObjectIndex].bLocked || m_TabInfo[m_iObjectIndex].bAddressLocked))
	{
		/* Enable the tab close button. */
		SendMessage(m_hTabWindowToolbar,TB_SETSTATE,
		TABTOOLBAR_CLOSE,TBSTATE_ENABLED);
	}
	else
	{
		/* Disable the tab close toolbar button. */
		SendMessage(m_hTabWindowToolbar,TB_SETSTATE,
		TABTOOLBAR_CLOSE,TBSTATE_INDETERMINATE);
	}
}

BOOL SaltedExplorer::OnMouseWheel(MousewheelSource_t MousewheelSource,WPARAM wParam,LPARAM lParam)
{
	short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	m_zDeltaTotal += zDelta;

	DWORD dwCursorPos = GetMessagePos();
	POINTS pts = MAKEPOINTS(dwCursorPos);

	POINT pt;
	pt.x = pts.x;
	pt.y = pts.y;

	HWND hwnd = WindowFromPoint(pt);

	BOOL bMessageHandled = FALSE;

	/* Normally, mouse wheel messages will be sent
	to the window with focus. We want to be able to
	scroll windows even if they do not have focus,
	so we'll capture the mouse wheel message and
	and forward it to the window currently underneath
	the mouse. */
	if(hwnd == m_hActiveListView)
	{
		if(wParam & MK_CONTROL)
		{
			/* Switch listview views. For each wheel delta
			(notch) the wheel is scrolled through, switch
			the view once. */
			for(int i = 0;i < abs(m_zDeltaTotal / WHEEL_DELTA);i++)
			{
				CycleViewState((m_zDeltaTotal > 0));
			}
		}
		else if(wParam & MK_SHIFT)
		{
			if(m_zDeltaTotal < 0)
			{
				for(int i = 0;i < abs(m_zDeltaTotal / WHEEL_DELTA);i++)
				{
					OnBrowseBack();
				}
			}
			else
			{
				for(int i = 0;i < abs(m_zDeltaTotal / WHEEL_DELTA);i++)
				{
					OnBrowseForward();
				}
			}
		}
		else
		{
			if(MousewheelSource != MOUSEWHEEL_SOURCE_LISTVIEW)
			{
				bMessageHandled = TRUE;
				SendMessage(m_hActiveListView,WM_MOUSEWHEEL,wParam,lParam);
			}
		}
	}
	else if(hwnd == m_hTreeView)
	{
		if(MousewheelSource != MOUSEWHEEL_SOURCE_TREEVIEW)
		{
			bMessageHandled = TRUE;
			SendMessage(m_hTreeView,WM_MOUSEWHEEL,wParam,lParam);
		}
	}
	else if(hwnd == m_hTabCtrl)
	{
		bMessageHandled = TRUE;

		HWND hUpDown = FindWindowEx(m_hTabCtrl,NULL,UPDOWN_CLASS,NULL);

		if(hUpDown != NULL)
		{
			BOOL bSuccess;
			int iPos = static_cast<int>(SendMessage(hUpDown,UDM_GETPOS32,0,reinterpret_cast<LPARAM>(&bSuccess)));

			if(bSuccess)
			{
				int iScrollPos = iPos;

				int iLow;
				int iHigh;
				SendMessage(hUpDown,UDM_GETRANGE32,reinterpret_cast<WPARAM>(&iLow),reinterpret_cast<LPARAM>(&iHigh));

				if(m_zDeltaTotal < 0)
				{
					if(iScrollPos < iHigh)
					{
						iScrollPos++;
					}
				}
				else
				{
					if(iScrollPos > iLow)
					{
						iScrollPos--;
					}
				}

				SendMessage(m_hTabCtrl,WM_HSCROLL,MAKEWPARAM(SB_THUMBPOSITION,iScrollPos),NULL);
			}
		}
	}

	if(abs(m_zDeltaTotal) >= WHEEL_DELTA)
	{
		m_zDeltaTotal = m_zDeltaTotal % WHEEL_DELTA;
	}

	return bMessageHandled;
}

void SaltedExplorer::DuplicateTab(int iTabInternal)
{
	TCHAR szTabDirectory[MAX_PATH];

	m_pShellBrowser[iTabInternal]->QueryCurrentDirectory(SIZEOF_ARRAY(szTabDirectory),
		szTabDirectory);

	BrowseFolder(szTabDirectory,SBSP_ABSOLUTE,TRUE,FALSE,FALSE);
}

void SaltedExplorer::SetTabProxyIcon(int iTabId,HICON hIcon)
{
	std::list<TabProxyInfo_t>::iterator itr;

	for(itr = m_TabProxyList.begin();itr != m_TabProxyList.end();itr++)
	{
		if(itr->iTabId == iTabId)
		{
			HICON hIconTemp;

			hIconTemp = (HICON)GetClassLongPtr(itr->hProxy,GCLP_HICONSM);
			DestroyIcon(hIconTemp);

			hIconTemp = CopyIcon(hIcon);

			SetClassLongPtr(itr->hProxy,GCLP_HICONSM,(LONG_PTR)hIconTemp);
			break;
		}
	}
}

int SaltedExplorer::GetCurrentTabId()
{
	return m_iObjectIndex;
}