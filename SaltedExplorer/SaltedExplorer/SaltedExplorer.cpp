/*****************************************************************
 * Project: SaltedExplorer
 * File: SaltedExplorer.cpp
 *****************************************************************/

#include "stdafx.h"
#include "SaltedExplorer.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/SetDefaultFileManager.h"


CRITICAL_SECTION	g_csDirMonCallback;

/* IUnknown interface members. */
HRESULT __stdcall SaltedExplorer::QueryInterface(REFIID iid, void **ppvObject)
{
	*ppvObject = NULL;

	if(iid == IID_IServiceProvider)
	{
		*ppvObject = static_cast<IServiceProvider *>(this);
	}

	if(*ppvObject)
	{
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG __stdcall SaltedExplorer::AddRef(void)
{
	return ++m_iRefCount;
}

ULONG __stdcall SaltedExplorer::Release(void)
{
	m_iRefCount--;
	
	if(m_iRefCount == 0)
	{
		delete this;
		return 0;
	}

	return m_iRefCount;
}

SaltedExplorer::SaltedExplorer(HWND hwnd)
{
	m_iRefCount = 1;

	m_hContainer					= hwnd;

	/* When the 'open new tabs next to
	current' option is activated, the
	first tab will open at the index
	m_iTabSelectedItem + 1 - therefore
	this variable must be initialized. */
	m_iTabSelectedItem				= 0;

	/* Initial state. */
	m_nSelected						= 0;
	m_iObjectIndex					= 0;
	m_iMaxArrangeMenuItem			= 0;
	m_bCountingUp					= FALSE;
	m_bCountingDown					= FALSE;
	m_bInverted						= FALSE;
	m_bSelectionFromNowhere			= FALSE;
	m_bSelectingTreeViewDirectory	= FALSE;
	m_bTreeViewRightClick			= FALSE;
	m_bTabBeenDragged				= FALSE;
	m_bTreeViewDelayEnabled			= FALSE;
	m_bSavePreferencesToXMLFile		= FALSE;
	m_bAttemptToolbarRestore		= FALSE;
	m_bLanguageLoaded				= FALSE;
	m_bListViewRenaming				= FALSE;
	m_bDragging						= FALSE;
	m_bDragCancelled				= FALSE;
	m_bDragAllowed					= FALSE;
	m_pActiveShellBrowser			= NULL;
	g_hwndSearch					= NULL;
	g_hwndPreferences					= NULL;
	g_hwndManageFavorites			= NULL;
	m_ListViewMButtonItem			= -1;
	m_nDrivesInToolbar				= 0;
	m_zDeltaTotal					= 0;

	/* Dialog states. */
	m_bDisplayColorsDlgStateSaved	= FALSE;

	m_pTaskbarList3					= NULL;

	m_bBlockNext = FALSE;

	InitializeColorRules();

	SetDefaultValues();
	SetAllDefaultColumns();

	InitializeTabMap();

	/* Default folder (i.e. My Computer). */
	GetVirtualFolderParsingPath(CSIDL_DRIVES,m_DefaultTabDirectoryStatic);
	GetVirtualFolderParsingPath(CSIDL_DRIVES,m_DefaultTabDirectory);

	InitializeMainToolbars();
	InitializeApplicationToolbar();

	InitializeCriticalSection(&g_csDirMonCallback);

	m_iDWFolderSizeUniqueId = 0;

	m_pClipboardDataObject	= NULL;
	m_iCutTabInternal		= 0;
	m_hCutTreeViewItem		= NULL;

	/* View modes. */
	OSVERSIONINFO VersionInfo;
	ViewMode_t ViewMode;

	VersionInfo.dwOSVersionInfoSize	= sizeof(OSVERSIONINFO);

	if(GetVersionEx(&VersionInfo) != 0)
	{
		m_dwMajorVersion = VersionInfo.dwMajorVersion;
		m_dwMinorVersion = VersionInfo.dwMinorVersion;

		if(VersionInfo.dwMajorVersion >= WINDOWS_VISTA_SEVEN_MAJORVERSION)
		{
			ViewMode.uViewMode = VM_THUMBNAILS;
			m_ViewModes.push_back(ViewMode);

			ViewMode.uViewMode = VM_TILES;
			m_ViewModes.push_back(ViewMode);

			ViewMode.uViewMode = VM_ICONS;
			m_ViewModes.push_back(ViewMode);

			ViewMode.uViewMode = VM_SMALLICONS;
			m_ViewModes.push_back(ViewMode);

			ViewMode.uViewMode = VM_LIST;
			m_ViewModes.push_back(ViewMode);

			ViewMode.uViewMode = VM_DETAILS;
			m_ViewModes.push_back(ViewMode);
		}
		else
		{
			ViewMode.uViewMode = VM_THUMBNAILS;
			m_ViewModes.push_back(ViewMode);

			ViewMode.uViewMode = VM_TILES;
			m_ViewModes.push_back(ViewMode);

			ViewMode.uViewMode = VM_ICONS;
			m_ViewModes.push_back(ViewMode);

			ViewMode.uViewMode = VM_SMALLICONS;
			m_ViewModes.push_back(ViewMode);

			ViewMode.uViewMode = VM_LIST;
			m_ViewModes.push_back(ViewMode);

			ViewMode.uViewMode = VM_DETAILS;
			m_ViewModes.push_back(ViewMode);
		}
	}

	m_hDwmapi = LoadLibrary(_T("dwmapi.dll"));

	if(m_hDwmapi != NULL)
	{
		DwmInvalidateIconicBitmaps = (DwmInvalidateIconicBitmapsProc)GetProcAddress(m_hDwmapi,"DwmInvalidateIconicBitmaps");
	}
	else
	{
		DwmInvalidateIconicBitmaps = NULL;
	}
}

SaltedExplorer::~SaltedExplorer()
{
	/* Favorites teardown. */
	delete m_pipbin;

	m_pDirMon->Release();

	if(m_hDwmapi != NULL)
	{
		FreeLibrary(m_hDwmapi);
	}
}

void SaltedExplorer::InitializeMainToolbars(void)
{
	/* Initialize the main toolbar styles and settings here. The visibility and gripper
	styles will be set after the settings have been loaded (needed to keep compatibility
	with versions older than 0.9.5.4). */

	m_ToolbarInformation[0].wID			= ID_MENUBAR;
	m_ToolbarInformation[0].fMask		= RBBIM_ID|RBBIM_CHILD|RBBIM_CHILDSIZE|RBBIM_SIZE|RBBIM_IDEALSIZE|RBBIM_STYLE;
	m_ToolbarInformation[0].fStyle		= RBBS_BREAK|RBBS_USECHEVRON;
	m_ToolbarInformation[0].cx			= 0;
	m_ToolbarInformation[0].cxIdeal		= 0;
	m_ToolbarInformation[0].cxMinChild	= 0;
	m_ToolbarInformation[0].cyIntegral	= 0;
	m_ToolbarInformation[0].cxHeader	= 0;
	m_ToolbarInformation[0].lpText		= NULL;

	m_ToolbarInformation[1].wID			= ID_MAINTOOLBAR;
	m_ToolbarInformation[1].fMask		= RBBIM_ID|RBBIM_CHILD|RBBIM_CHILDSIZE|RBBIM_SIZE|RBBIM_IDEALSIZE|RBBIM_STYLE;
	m_ToolbarInformation[1].fStyle		= RBBS_BREAK|RBBS_USECHEVRON;
	m_ToolbarInformation[1].cx			= 0;
	m_ToolbarInformation[1].cxIdeal		= 0;
	m_ToolbarInformation[1].cxMinChild	= 0;
	m_ToolbarInformation[1].cyIntegral	= 0;
	m_ToolbarInformation[1].cxHeader	= 0;
	m_ToolbarInformation[1].lpText		= NULL;

	m_ToolbarInformation[2].wID			= ID_ADDRESSTOOLBAR;
	m_ToolbarInformation[2].fMask		= RBBIM_ID|RBBIM_CHILD|RBBIM_CHILDSIZE|RBBIM_SIZE|RBBIM_STYLE|RBBIM_TEXT;
	m_ToolbarInformation[2].fStyle		= RBBS_BREAK;
	m_ToolbarInformation[2].cx			= 0;
	m_ToolbarInformation[2].cxIdeal		= 0;
	m_ToolbarInformation[2].cxMinChild	= 0;
	m_ToolbarInformation[2].cyIntegral	= 0;
	m_ToolbarInformation[2].cxHeader	= 0;
	m_ToolbarInformation[2].lpText		= NULL;

	m_ToolbarInformation[3].wID			= ID_FAVORITESTOOLBAR;
	m_ToolbarInformation[3].fMask		= RBBIM_ID|RBBIM_CHILD|RBBIM_CHILDSIZE|RBBIM_SIZE|RBBIM_IDEALSIZE|RBBIM_STYLE;
	m_ToolbarInformation[3].fStyle		= RBBS_BREAK|RBBS_USECHEVRON;
	m_ToolbarInformation[3].cx			= 0;
	m_ToolbarInformation[3].cxIdeal		= 0;
	m_ToolbarInformation[3].cxMinChild	= 0;
	m_ToolbarInformation[3].cyIntegral	= 0;
	m_ToolbarInformation[3].cxHeader	= 0;
	m_ToolbarInformation[3].lpText		= NULL;

	m_ToolbarInformation[4].wID			= ID_DRIVESTOOLBAR;
	m_ToolbarInformation[4].fMask		= RBBIM_ID|RBBIM_CHILD|RBBIM_CHILDSIZE|RBBIM_SIZE|RBBIM_IDEALSIZE|RBBIM_STYLE;
	m_ToolbarInformation[4].fStyle		= RBBS_BREAK|RBBS_USECHEVRON;
	m_ToolbarInformation[4].cx			= 0;
	m_ToolbarInformation[4].cxIdeal		= 0;
	m_ToolbarInformation[4].cxMinChild	= 0;
	m_ToolbarInformation[4].cyIntegral	= 0;
	m_ToolbarInformation[4].cxHeader	= 0;
	m_ToolbarInformation[4].lpText		= NULL;

	m_ToolbarInformation[5].wID			= ID_APPLICATIONSTOOLBAR;
	m_ToolbarInformation[5].fMask		= RBBIM_ID|RBBIM_CHILD|RBBIM_CHILDSIZE|RBBIM_SIZE|RBBIM_IDEALSIZE|RBBIM_STYLE;
	m_ToolbarInformation[5].fStyle		= RBBS_BREAK|RBBS_USECHEVRON;
	m_ToolbarInformation[5].cx			= 0;
	m_ToolbarInformation[5].cxIdeal		= 0;
	m_ToolbarInformation[5].cxMinChild	= 0;
	m_ToolbarInformation[5].cyIntegral	= 0;
	m_ToolbarInformation[5].cxHeader	= 0;
	m_ToolbarInformation[5].lpText		= NULL;

}

/*
 * Sets the default values used within the program.
 */
void SaltedExplorer::SetDefaultValues(void)
{
	/* User options. */
	m_bOpenNewTabNextToCurrent		= FALSE;
	m_bConfirmCloseTabs				= FALSE;
	m_bStickySelection				= TRUE;
	m_bShowFullTitlePath			= FALSE;
	m_bAlwaysOpenNewTab				= FALSE;
	m_bShowFolderSizes				= FALSE;
	m_bDisableFolderSizesNetworkRemovable	 = FALSE;
	m_bUnlockFolders				= TRUE;
	m_StartupMode					= STARTUP_PREVIOUSTABS;
	m_WebViewOptions				= OPTION_WEBVIEW;
	m_bExtendTabControl				= FALSE;
	m_bShowUserNameInTitleBar		= FALSE;
	m_bShowPrivilegeLevelInTitleBar	= FALSE;
	m_bShowFilePreviews				= TRUE;
	m_ReplaceExplorerMode			= NDefaultFileManager::REPLACEEXPLORER_NONE;
	m_bOneClickActivate				= FALSE;
	m_OneClickActivateHoverTime		= DEFAULT_LISTVIEW_HOVER_TIME;
	m_bAllowMultipleInstances		= TRUE;
	m_bForceSameTabWidth			= FALSE;
	m_bDoubleClickTabClose			= TRUE;
	m_bHandleZipFiles				= FALSE;
	m_bInsertSorted					= TRUE;
	m_bOverwriteExistingFilesConfirmation	= TRUE;
	m_bCheckBoxSelection			= FALSE;
	m_bForceSize					= FALSE;
	m_SizeDisplayFormat				= SIZE_FORMAT_BYTES;
	m_bSynchronizeTreeview			= TRUE;
	m_bTVAutoExpandSelected			= FALSE;
	m_bCloseMainWindowOnTabClose	= TRUE;
	m_bLargeToolbarIcons			= FALSE;
	m_bToolbarTitleButtons			= FALSE;
	m_bShowTaskbarThumbnails		= TRUE;
	m_bPlayNavigationSound			= TRUE;

	/* Infotips (user options). */
	m_bShowInfoTips					= TRUE;
	m_InfoTipType					= INFOTIP_SYSTEM;

	/* Window states. */
	m_bShellMode					= FALSE;
	m_bVistaControls				= FALSE;
	m_bShowStatusBar				= TRUE;
	m_bShowFolders					= FALSE;
	m_bShowMenuBar					= TRUE;
	m_bShowAddressBar				= TRUE;
	m_bShowMainToolbar				= TRUE;
	m_bShowFavoritesToolbar			= FALSE;
	m_bShowDisplayWindow			= FALSE;
	m_bShowDrivesToolbar			= FALSE;
	m_bShowApplicationToolbar		= FALSE;
	m_bAlwaysShowTabBar				= FALSE;
	m_bShowTabBar					= FALSE;
	m_bLockToolbars					= FALSE;
	m_DisplayWindowWidth			= DEFAULT_DISPLAYWINDOW_WIDTH;
	m_DisplayWindowHeight			= DEFAULT_DISPLAYWINDOW_HEIGHT;
	m_DisplayWindowVertical			= FALSE;
	m_TreeViewWidth					= DEFAULT_TREEVIEW_WIDTH;
	m_bUseFullRowSelect				= FALSE;
	m_bShowTabBarAtBottom			= FALSE;

	/* Global options. */
	m_ViewModeGlobal				= VM_ICONS;
	m_bShowHiddenGlobal				= TRUE;
	m_bShowExtensionsGlobal			= TRUE;
	m_bShowInGroupsGlobal			= FALSE;
	m_bAutoArrangeGlobal			= TRUE;
	m_bSortAscendingGlobal			= TRUE;
	m_bShowGridlinesGlobal			= FALSE;
	m_bShowFriendlyDatesGlobal		= FALSE;
	m_bHideSystemFilesGlobal		= FALSE;
	m_bHideLinkExtensionGlobal		= FALSE;
}

HMENU SaltedExplorer::CreateRebarHistoryMenu(BOOL bBack)
{
	HMENU hSubMenu = NULL;
	std::list<LPITEMIDLIST> lHistory;
	std::list<LPITEMIDLIST>::iterator itr;
	MENUITEMINFO mii;
	TCHAR szDisplayName[MAX_PATH];
	int iBase;
	int i = 0;

	if(bBack)
	{
		m_pActiveShellBrowser->GetBackHistory(&lHistory);

		iBase = ID_REBAR_MENU_BACK_START;
	}
	else
	{
		m_pActiveShellBrowser->GetForwardHistory(&lHistory);

		iBase = ID_REBAR_MENU_FORWARD_START;
	}

	if(lHistory.size() > 0)
	{
		hSubMenu = CreateMenu();

		for(itr = lHistory.begin();itr != lHistory.end();itr++)
		{
			GetDisplayName(*itr,szDisplayName,SHGDN_INFOLDER);

			mii.cbSize		= sizeof(mii);
			mii.fMask		= MIIM_ID|MIIM_STRING;
			mii.wID			= iBase + i + 1;
			mii.dwTypeData	= szDisplayName;
			InsertMenuItem(hSubMenu,i,TRUE,&mii);

			i++;

			CoTaskMemFree(*itr);
		}

		lHistory.clear();

		SetMenuOwnerDraw(hSubMenu);
	}

	return hSubMenu;
}

SaltedExplorer::CLoadSaveRegistry::CLoadSaveRegistry(SaltedExplorer *pContainer)
{
	m_iRefCount = 1;

	m_pContainer = pContainer;
}

SaltedExplorer::CLoadSaveRegistry::~CLoadSaveRegistry()
{

}

/* IUnknown interface members. */
HRESULT __stdcall SaltedExplorer::CLoadSaveRegistry::QueryInterface(REFIID iid, void **ppvObject)
{
	*ppvObject = NULL;

	if(*ppvObject)
	{
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG __stdcall SaltedExplorer::CLoadSaveRegistry::AddRef(void)
{
	return ++m_iRefCount;
}

ULONG __stdcall SaltedExplorer::CLoadSaveRegistry::Release(void)
{
	m_iRefCount--;
	
	if(m_iRefCount == 0)
	{
		delete this;
		return 0;
	}

	return m_iRefCount;
}

HRESULT SaltedExplorer::QueryService(REFGUID guidService,REFIID riid,void **ppv)
{
	*ppv = NULL;

	if(riid == IID_IShellView2)
	{
		*ppv = static_cast<IShellView2 *>(this);
	}
	else if(riid == IID_INewMenuClient)
	{
		*ppv = static_cast<INewMenuClient *>(this);
	}

	if(*ppv)
	{
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

HRESULT SaltedExplorer::CreateViewWindow2(LPSV2CVW2_PARAMS lpParams)
{
	return S_OK;
}

HRESULT SaltedExplorer::GetView(SHELLVIEWID *pvid,ULONG uView)
{
	return S_OK;
}

HRESULT SaltedExplorer::HandleRename(LPCITEMIDLIST pidlNew)
{
	return S_OK;
}

HRESULT SaltedExplorer::SelectAndPositionItem(LPCITEMIDLIST pidlItem,UINT uFlags,POINT *ppt)
{
	LPITEMIDLIST pidlComplete = NULL;
	LPITEMIDLIST pidlDirectory = NULL;

	/* The idlist passed is only a relative (child) one. Combine
	it with the tabs' current directory to get a full idlist. */
	pidlDirectory = m_pActiveShellBrowser->QueryCurrentDirectoryIdl();
	pidlComplete = ILCombine(pidlDirectory,pidlItem);

	m_pActiveShellBrowser->QueueRename((LPITEMIDLIST)pidlComplete);

	CoTaskMemFree(pidlDirectory);
	CoTaskMemFree(pidlComplete);

	return S_OK;
}

HRESULT SaltedExplorer::GetWindow(HWND *)
{
	return S_OK;
}

HRESULT SaltedExplorer::ContextSensitiveHelp(BOOL bHelp)
{
	return S_OK;
}

HRESULT SaltedExplorer::TranslateAccelerator(MSG *msg)
{
	return S_OK;
}

HRESULT SaltedExplorer::EnableModeless(BOOL fEnable)
{
	return S_OK;
}

HRESULT SaltedExplorer::UIActivate(UINT uActivate)
{
	return S_OK;
}

HRESULT SaltedExplorer::Refresh(void)
{
	return S_OK;
}

HRESULT SaltedExplorer::CreateViewWindow(IShellView *psvPrevious,LPCFOLDERSETTINGS pfs,IShellBrowser *psb,RECT *prcView,HWND *phWnd)
{
	return S_OK;
}

HRESULT SaltedExplorer::DestroyViewWindow(void)
{
	return S_OK;
}

HRESULT SaltedExplorer::GetCurrentInfo(LPFOLDERSETTINGS pfs)
{
	return S_OK;
}

HRESULT SaltedExplorer::AddPropertySheetPages(DWORD dwReserved,LPFNSVADDPROPSHEETPAGE pfn,LPARAM lparam)
{
	return S_OK;
}

HRESULT SaltedExplorer::SaveViewState(void)
{
	return S_OK;
}

HRESULT SaltedExplorer::SelectItem(LPCITEMIDLIST pidlItem,SVSIF uFlags)
{
	return S_OK;
}

HRESULT SaltedExplorer::GetItemObject(UINT uItem,REFIID riid,void **ppv)
{
	return S_OK;
}

HRESULT SaltedExplorer::IncludeItems(NMCII_FLAGS *pFlags)
{
	/* pFlags will be one of:
	NMCII_ITEMS
	NMCII_FOLDERS
	Which one of these is selected determines which
	items are shown on the 'new' menu.
	If NMCII_ITEMS is selected, only files will be
	shown (meaning 'New Folder' will NOT appear).
	If NMCII_FOLDERS is selected, only folders will
	be shown (this means that in most cases, only
	'New Folder' and 'New Briefcase' will be shown).
	Therefore, to get all the items, OR the two flags
	together to show files and folders. */

	*pFlags = NMCII_ITEMS|NMCII_FOLDERS;

	return S_OK;
}

HRESULT SaltedExplorer::SelectAndEditItem(PCIDLIST_ABSOLUTE pidlItem,NMCSAEI_FLAGS flags)
{
	switch(flags)
	{
		/* This would usually cause the
		item to be selected first, then
		renamed. In this case however,
		the item is selected and renamed
		in one operation, so this state
		can be ignored. */
		case NMCSAEI_SELECT:
			break;

		/* Now, start an in-place rename
		of the item. */
		case NMCSAEI_EDIT:
			m_pActiveShellBrowser->QueueRename((LPITEMIDLIST)pidlItem);
			break;
	}

	return S_OK;
}
