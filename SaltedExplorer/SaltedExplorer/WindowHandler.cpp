#include "stdafx.h"
#include "SaltedExplorer.h"
#include "SaltedExplorer_internal.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/FileContextMenuManager.h"
#include "../Helper/Macros.h"


LRESULT CALLBACK	TabBackingProcStub(HWND ListView,UINT msg,WPARAM wParam,LPARAM lParam);

DWORD MainToolbarStyles		=	WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
								TBSTYLE_TOOLTIPS | TBSTYLE_LIST | TBSTYLE_TRANSPARENT |
								TBSTYLE_FLAT | CCS_NODIVIDER | CCS_NORESIZE | CCS_ADJUSTABLE;

DWORD MenuBarStyles			=	WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
								TBSTYLE_TOOLTIPS | TBSTYLE_LIST | TBSTYLE_TRANSPARENT |
								TBSTYLE_FLAT | CCS_NODIVIDER | CCS_NORESIZE;

DWORD GoToolbarStyles		=	WS_CHILD |WS_VISIBLE |WS_CLIPSIBLINGS |WS_CLIPCHILDREN |
								TBSTYLE_TOOLTIPS | TBSTYLE_LIST | TBSTYLE_TRANSPARENT |
								TBSTYLE_FLAT | CCS_NODIVIDER| CCS_NORESIZE;

DWORD FavoriteToolbarStyles	=	WS_CHILD |WS_VISIBLE |WS_CLIPSIBLINGS |WS_CLIPCHILDREN |
								TBSTYLE_TOOLTIPS | TBSTYLE_LIST | TBSTYLE_TRANSPARENT |
								TBSTYLE_FLAT | CCS_NODIVIDER| CCS_NORESIZE;

DWORD TreeViewStyles		=	WS_CHILD | WS_VISIBLE | TVS_SHOWSELALWAYS | TVS_HASBUTTONS |
								TVS_EDITLABELS | TVS_HASLINES | TVS_TRACKSELECT;

DWORD ComboBoxStyles		=	WS_CHILD|WS_VISIBLE|WS_TABSTOP|
								CBS_DROPDOWN|CBS_AUTOHSCROLL|WS_CLIPSIBLINGS|
								WS_CLIPCHILDREN;

UINT TabToolbarStyles		=	WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
								TBSTYLE_TOOLTIPS | TBSTYLE_LIST | TBSTYLE_TRANSPARENT |
								TBSTYLE_FLAT | CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE;

DWORD RebarStyles			=	WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|
								WS_BORDER|CCS_NODIVIDER|CCS_TOP|CCS_NOPARENTALIGN|
								RBS_BANDBORDERS|RBS_VARHEIGHT;

void SaltedExplorer::CreateFolderControls(void)
{
	TCHAR szTemp[32];
	UINT uStyle = WS_CHILD|WS_CLIPSIBLINGS|WS_CLIPCHILDREN;

	if(m_bShowFolders)
		uStyle |= WS_VISIBLE;

	m_hHolder = CreateHolderWindow(m_hContainer,_T("Folders"),uStyle);
	SetWindowSubclass(m_hHolder,TreeViewHolderProcStub,0,(DWORD_PTR)this);

	m_hTreeView = CreateTreeView(m_hHolder,TreeViewStyles);

	if(m_bVistaControls)
	{
		SetWindowTheme(m_hTreeView,L"Explorer",NULL);
	}

	SetWindowLongPtr(m_hTreeView,GWL_EXSTYLE,WS_EX_CLIENTEDGE);
	m_pMyTreeView = new CMyTreeView(m_hTreeView,m_hContainer,m_pDirMon,m_hTreeViewIconThread);

	/* Now, subclass the treeview again. This is needed for messages
	such as WM_MOUSEWHEEL, which need to be intercepted before they
	reach the window procedure provided by CMyTreeView. */
	SetWindowSubclass(m_hTreeView,TreeViewSubclassStub,1,(DWORD_PTR)this);

	LoadString(g_hLanguageModule,IDS_HIDEFOLDERSPANE,szTemp,SIZEOF_ARRAY(szTemp));

	m_hFoldersToolbar = CreateTabToolbar(m_hHolder,FOLDERS_TOOLBAR_CLOSE,szTemp);
}

void SaltedExplorer::CreateMainControls(void)
{
	SIZE	sz;
	DWORD	ToolbarSize;
	TCHAR	szBandText[32];
	int		i = 0;

	/* If the rebar is locked, prevent bands from
	been rearranged. */
	if(m_bLockToolbars)
		RebarStyles |= RBS_FIXEDORDER;

	/* Create and subclass the main rebar control. */
	m_hMainRebar = CreateWindowEx(0,REBARCLASSNAME,EMPTY_STRING,RebarStyles,
		0,0,0,0,m_hContainer,NULL,GetModuleHandle(0),NULL);
	SetWindowSubclass(m_hMainRebar,RebarSubclassStub,0,(DWORD_PTR)this);

	for(i = 0;i < NUM_MAIN_TOOLBARS;i++)
	{
		switch(m_ToolbarInformation[i].wID)
		{
		case ID_MENUBAR:
			CreateMenuBar();
			ToolbarSize = (DWORD)SendMessage(m_hMenuBar,TB_GETBUTTONSIZE,0,0);
			m_ToolbarInformation[i].cyMinChild = HIWORD(ToolbarSize);
			m_ToolbarInformation[i].cyMaxChild = HIWORD(ToolbarSize);
			m_ToolbarInformation[i].cyChild = HIWORD(ToolbarSize);
			SendMessage(m_hMenuBar,TB_GETMAXSIZE,0,(LPARAM)&sz);

			if(m_ToolbarInformation[i].cx == 0)
				m_ToolbarInformation[i].cx = sz.cx;

			m_ToolbarInformation[i].cxIdeal = sz.cx;
			m_ToolbarInformation[i].hwndChild = m_hMenuBar;
			break;
		case ID_MAINTOOLBAR:
			CreateMainToolbar();
			ToolbarSize = (DWORD)SendMessage(m_hMainToolbar,TB_GETBUTTONSIZE,0,0);
			m_ToolbarInformation[i].cyMinChild = HIWORD(ToolbarSize);
			m_ToolbarInformation[i].cyMaxChild = HIWORD(ToolbarSize);
			m_ToolbarInformation[i].cyChild = HIWORD(ToolbarSize);
			SendMessage(m_hMainToolbar,TB_GETMAXSIZE,0,(LPARAM)&sz);

			if(m_ToolbarInformation[i].cx == 0)
				m_ToolbarInformation[i].cx = sz.cx;

			m_ToolbarInformation[i].cxIdeal = sz.cx;
			m_ToolbarInformation[i].hwndChild = m_hMainToolbar;
			break;

		case ID_ADDRESSTOOLBAR:
			CreateAddressToolbar();
			CreateAddressBar();
			LoadString(g_hLanguageModule,IDS_ADDRESSBAR,szBandText,SIZEOF_ARRAY(szBandText));
			ToolbarSize = (DWORD)SendMessage(m_hAddressToolbar,TB_GETBUTTONSIZE,0,0);
			m_ToolbarInformation[i].cyMinChild = HIWORD(ToolbarSize);
			m_ToolbarInformation[i].lpText = szBandText;
			m_ToolbarInformation[i].hwndChild = m_hAddressToolbar;
			break;

		case ID_FAVORITESTOOLBAR:
			CreateFAVORITESToolbar();
			ToolbarSize = (DWORD)SendMessage(m_hFavoritesToolbar,TB_GETBUTTONSIZE,0,0);
			m_ToolbarInformation[i].cyMinChild = HIWORD(ToolbarSize);
			m_ToolbarInformation[i].cyMaxChild = HIWORD(ToolbarSize);
			m_ToolbarInformation[i].cyChild = HIWORD(ToolbarSize);
			SendMessage(m_hFavoritesToolbar,TB_GETMAXSIZE,0,(LPARAM)&sz);

			if(m_ToolbarInformation[i].cx == 0)
				m_ToolbarInformation[i].cx = sz.cx;

			m_ToolbarInformation[i].cxIdeal = sz.cx;
			m_ToolbarInformation[i].hwndChild = m_hFavoritesToolbar;
			break;

		case ID_DRIVESTOOLBAR:
			CreateDrivesToolbar();
			ToolbarSize = (DWORD)SendMessage(m_hDrivesToolbar,TB_GETBUTTONSIZE,0,0);
			m_ToolbarInformation[i].cyMinChild = HIWORD(ToolbarSize);
			m_ToolbarInformation[i].cyMaxChild = HIWORD(ToolbarSize);
			m_ToolbarInformation[i].cyChild = HIWORD(ToolbarSize);
			SendMessage(m_hDrivesToolbar,TB_GETMAXSIZE,0,(LPARAM)&sz);

			if(m_ToolbarInformation[i].cx == 0)
				m_ToolbarInformation[i].cx = sz.cx;

			m_ToolbarInformation[i].cxIdeal = sz.cx;
			m_ToolbarInformation[i].hwndChild = m_hDrivesToolbar;
			break;

		case ID_APPLICATIONSTOOLBAR:
			CreateApplicationToolbar();
			ToolbarSize = (DWORD)SendMessage(m_hApplicationToolbar,TB_GETBUTTONSIZE,0,0);
			m_ToolbarInformation[i].cyMinChild = HIWORD(ToolbarSize);
			m_ToolbarInformation[i].cyMaxChild = HIWORD(ToolbarSize);
			m_ToolbarInformation[i].cyChild = HIWORD(ToolbarSize);
			SendMessage(m_hApplicationToolbar,TB_GETMAXSIZE,0,(LPARAM)&sz);

			if(m_ToolbarInformation[i].cx == 0)
				m_ToolbarInformation[i].cx = sz.cx;

			m_ToolbarInformation[i].cxIdeal = sz.cx;
			m_ToolbarInformation[i].hwndChild = m_hApplicationToolbar;
			break;
		}

		m_ToolbarInformation[i].cbSize = sizeof(REBARBANDINFO);
		SendMessage(m_hMainRebar,RB_INSERTBAND,(WPARAM)-1,(LPARAM)&m_ToolbarInformation[i]);
	}
}

void SaltedExplorer::CreateMenuBar(void)
{
	const int numButtons     = 9;

	const DWORD buttonStyles = BTNS_DROPDOWN | BTNS_SHOWTEXT | BTNS_AUTOSIZE;

	m_hMenuBar = CreateToolbar(m_hMainRebar,MenuBarStyles,
		TBSTYLE_EX_MIXEDBUTTONS|TBSTYLE_DROPDOWN|
		TBSTYLE_EX_DOUBLEBUFFER|TBSTYLE_EX_HIDECLIPPEDBUTTONS);

    LONG_PTR iMenuItems = SendMessage(m_hMenuBar, TB_ADDSTRING, 
                           (WPARAM)GetModuleHandle(NULL), (LPARAM)IDR_SUBMENU_START);
	
	TBBUTTON tbButtons[numButtons] = 
    {
        { I_IMAGENONE, MENUBAR_FILE, TBSTATE_ENABLED, buttonStyles, {0}, 0, iMenuItems },
        { I_IMAGENONE, MENUBAR_EDIT, TBSTATE_ENABLED, buttonStyles, {0}, 0, iMenuItems + 1 },
        { I_IMAGENONE, MENUBAR_SELC, TBSTATE_ENABLED, buttonStyles, {0}, 0, iMenuItems + 2 },
		{ I_IMAGENONE, MENUBAR_VIEW, TBSTATE_ENABLED, buttonStyles, {0}, 0, iMenuItems + 3 },
		{ I_IMAGENONE, MENUBAR_ACTI, TBSTATE_ENABLED, buttonStyles, {0}, 0, iMenuItems + 4 },
		{ I_IMAGENONE, MENUBAR_GO, TBSTATE_ENABLED, buttonStyles, {0}, 0, iMenuItems + 5 },
		{ I_IMAGENONE, MENUBAR_FAVO, TBSTATE_ENABLED, buttonStyles, {0}, 0, iMenuItems + 6 },
		{ I_IMAGENONE, MENUBAR_TOOLS, TBSTATE_ENABLED, buttonStyles, {0}, 0, iMenuItems + 7 },
		{ I_IMAGENONE, MENUBAR_HELP, TBSTATE_ENABLED, buttonStyles, {0}, 0, iMenuItems + 8 }
    };

	SendMessage(m_hMenuBar,TB_BUTTONSTRUCTSIZE,(WPARAM)sizeof(TBBUTTON),0);
	SendMessage(m_hMenuBar,TB_ADDBUTTONS,(WPARAM)numButtons,(LPARAM)&tbButtons);

}
void SaltedExplorer::CreateMainToolbar(void)
{

	m_hMainToolbar = CreateToolbar(m_hMainRebar,MainToolbarStyles,
		TBSTYLE_EX_MIXEDBUTTONS|TBSTYLE_EX_DRAWDDARROWS|
		TBSTYLE_EX_DOUBLEBUFFER|TBSTYLE_EX_HIDECLIPPEDBUTTONS);

	HIMAGELIST *phiml = NULL;
	HIMAGELIST *phiml1 = NULL;
	/* HIMAGELIST *phiml2 = NULL; */
	int cx;
	int cy;

	if(m_bLargeToolbarIcons)
	{
		cx = TOOLBAR_IMAGE_SIZE_LARGE_X;
		cy = TOOLBAR_IMAGE_SIZE_LARGE_Y;
		phiml = &m_himlToolbarLarge;
		phiml1 = &m_himlToolbarLargeInact;
		/* phiml2 = &m_himlToolbarLargeDisabled; */
	}
	else
	{
		cx = TOOLBAR_IMAGE_SIZE_SMALL_X;
		cy = TOOLBAR_IMAGE_SIZE_SMALL_Y;
		phiml = &m_himlToolbarSmall;
		phiml1 = &m_himlToolbarSmallInact;
		/* phiml2 = &m_himlToolbarSmallDisabled; */
	}

	SendMessage(m_hMainToolbar,TB_SETBITMAPSIZE,0,MAKELONG(cx,cy));	
	SendMessage(m_hMainToolbar,TB_BUTTONSTRUCTSIZE,(WPARAM)sizeof(TBBUTTON),0);

	/* Add the custom buttons to the toolbars image list. */
	SendMessage(m_hMainToolbar,TB_SETHOTIMAGELIST,0,(LPARAM)*phiml);
	SendMessage(m_hMainToolbar,TB_SETIMAGELIST,0,(LPARAM)*phiml1);
	/* SendMessage(m_hMainToolbar,TB_SETDISABLEDIMAGELIST,0,(LPARAM)*phiml2); */

	AddStringsToMainToolbar();
	InsertToolbarButtons();

	if(!m_bLoadSettingsFromXML)
	{
		if(m_bAttemptToolbarRestore)
		{
			TBSAVEPARAMS	tbSave;

			tbSave.hkr			= HKEY_CURRENT_USER;
			tbSave.pszSubKey	= REG_SETTINGS_KEY;
			tbSave.pszValueName	= _T("ToolbarState");

			SendMessage(m_hMainToolbar,TB_SAVERESTORE,FALSE,(LPARAM)&tbSave);
		}
	}
}

void SaltedExplorer::CreateAddressToolbar(void)
{
	HIMAGELIST	himl;
	HIMAGELIST	himl1;
	HBITMAP		hb;
	TBBUTTON	tbButton[2];
	TCHAR		szGoText[32];
	int			iCurrent = 0;

	m_hAddressToolbar = CreateToolbar(m_hMainRebar,GoToolbarStyles,
		TBSTYLE_EX_MIXEDBUTTONS|TBSTYLE_EX_DOUBLEBUFFER);

	SendMessage(m_hAddressToolbar,TB_SETBITMAPSIZE,0,MAKELONG(18,16));
	SendMessage(m_hAddressToolbar,TB_BUTTONSTRUCTSIZE,(WPARAM)sizeof(TBBUTTON),0);

	himl = ImageList_Create(18,16,ILC_COLOR32|ILC_MASK,0,1);
	hb = LoadBitmap(GetModuleHandle(0),MAKEINTRESOURCE(IDB_SHELL_GO_2000));
	ImageList_Add(himl,hb,NULL);
	DeleteObject(hb);

	himl1 = ImageList_Create(18,16,ILC_COLOR32|ILC_MASK,0,1);
	hb = LoadBitmap(GetModuleHandle(0),MAKEINTRESOURCE(IDB_SHELL_GO_2000_INA));
	ImageList_Add(himl1,hb,NULL);

	/* Add the custom buttons to the toolbars image list. */
	SendMessage(m_hAddressToolbar,TB_SETIMAGELIST,0,(LPARAM)himl1);
	SendMessage(m_hAddressToolbar,TB_SETHOTIMAGELIST,0,(LPARAM)himl);

	tbButton[iCurrent].iBitmap		= 0;
	tbButton[iCurrent].idCommand	= 0;
	tbButton[iCurrent].fsState		= TBSTATE_ENABLED;
	tbButton[iCurrent].fsStyle		= BTNS_SEP;
	tbButton[iCurrent].dwData		= 0;
	tbButton[iCurrent].iString		= 0;
	iCurrent++;

	LoadString(g_hLanguageModule,IDS_GO,szGoText,SIZEOF_ARRAY(szGoText));

	tbButton[iCurrent].iBitmap		= 0;
	tbButton[iCurrent].idCommand	= TOOLBAR_ADDRESSBAR_GO;
	tbButton[iCurrent].fsState		= TBSTATE_ENABLED;
	tbButton[iCurrent].fsStyle		= BTNS_SHOWTEXT | BTNS_BUTTON | BTNS_AUTOSIZE;
	tbButton[iCurrent].dwData		= 0;
	tbButton[iCurrent].iString		= (INT_PTR)szGoText;
	iCurrent++;

	/* Add the buttons to the toolbar. */
	SendMessage(m_hAddressToolbar,TB_ADDBUTTONS,(WPARAM)iCurrent,(LPARAM)tbButton);
}

void SaltedExplorer::CreateAddressBar(void)
{
	HWND		hEdit;
	HIMAGELIST	SmallIcons;

	m_hAddressBar = CreateComboBox(m_hAddressToolbar,ComboBoxStyles);

	/* Retrieve the small and large versions of the system image list. */
	Shell_GetImageLists(NULL,&SmallIcons);
	SendMessage(m_hAddressBar,CBEM_SETIMAGELIST,0,(LPARAM)SmallIcons);

	hEdit = (HWND)SendMessage(m_hAddressBar,CBEM_GETEDITCONTROL,0,0);

	SetWindowSubclass(hEdit,EditSubclassStub,0,(DWORD_PTR)this);

	/* Turn on auto complete for the edit control within the combobox.
	This will let the os complete paths as they are typed. */
	SHAutoComplete(hEdit,SHACF_FILESYSTEM|SHACF_AUTOSUGGEST_FORCE_ON);
}

void SaltedExplorer::CreateFAVORITESToolbar(void)
{
	m_hFavoritesToolbar = CreateToolbar(m_hMainRebar,FavoriteToolbarStyles,
		TBSTYLE_EX_MIXEDBUTTONS|TBSTYLE_EX_DRAWDDARROWS|
		TBSTYLE_EX_DOUBLEBUFFER|TBSTYLE_EX_HIDECLIPPEDBUTTONS);

	SetWindowSubclass(m_hFavoritesToolbar,FavoritesToolbarSubclassStub,0,(DWORD_PTR)this);

	SendMessage(m_hFavoritesToolbar,TB_SETBITMAPSIZE,0,MAKELONG(16,16));
	SendMessage(m_hFavoritesToolbar,TB_BUTTONSTRUCTSIZE,(WPARAM)sizeof(TBBUTTON),0);

	/* TODO: [FAVORITESs] Register toolbar for drag and drop. */
}

void SaltedExplorer::CreateDrivesToolbar(void)
{
	m_hDrivesToolbar = CreateToolbar(m_hMainRebar,FavoriteToolbarStyles,
		TBSTYLE_EX_DOUBLEBUFFER|TBSTYLE_EX_HIDECLIPPEDBUTTONS);

	SetWindowSubclass(m_hDrivesToolbar,DrivesToolbarSubclassStub,0,(DWORD_PTR)this);

	SendMessage(m_hDrivesToolbar,TB_SETBITMAPSIZE,0,MAKELONG(16,16));
	SendMessage(m_hDrivesToolbar,TB_BUTTONSTRUCTSIZE,(WPARAM)sizeof(TBBUTTON),0);

	InsertDrivesIntoDrivesToolbar();
}

HWND SaltedExplorer::CreateTabToolbar(HWND hParent,int idCommand,TCHAR *szTip)
{
	HWND TabToolbar;
	TBBUTTON tbButton[1];
	HBITMAP hb;
	HIMAGELIST himl;
	int ImageSize;
	int iIndex;

	TabToolbar = CreateToolbar(hParent,TabToolbarStyles,
		TBSTYLE_EX_MIXEDBUTTONS|TBSTYLE_EX_DOUBLEBUFFER);

	ImageSize = 7;
	
	SendMessage(TabToolbar,TB_SETBITMAPSIZE,0,MAKELONG(ImageSize,ImageSize));
	
	SendMessage(TabToolbar,TB_BUTTONSTRUCTSIZE,(WPARAM)sizeof(TBBUTTON),0);

	himl = ImageList_Create(ImageSize,ImageSize,ILC_COLOR32|ILC_MASK,0,2);

	hb = LoadBitmap(GetModuleHandle(0),MAKEINTRESOURCE(IDB_TABTOOLBAR_CLOSE));

	iIndex = ImageList_Add(himl,hb,NULL);

	DeleteObject(hb);

	SendMessage(TabToolbar,TB_SETIMAGELIST,0,(LPARAM)himl);

	/* Add the close button, used to close tabs. */
	tbButton[0].iBitmap		= iIndex;
	tbButton[0].idCommand	= idCommand;
	tbButton[0].fsState		= TBSTATE_ENABLED;
	tbButton[0].fsStyle		= TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE;
	tbButton[0].dwData		= 0;
	tbButton[0].iString		= (INT_PTR)szTip;
	SendMessage(TabToolbar,TB_ADDBUTTONS,(WPARAM)1,(LPARAM)&tbButton);

	SendMessage(TabToolbar,TB_SETBUTTONSIZE,0,MAKELPARAM(16,16));

	SendMessage(TabToolbar,TB_AUTOSIZE,0,0);

	return TabToolbar;
}

LRESULT CALLBACK EditSubclassStub(HWND hwnd,UINT uMsg,
WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	SaltedExplorer *pContainer = (SaltedExplorer *)dwRefData;

	return pContainer->EditSubclass(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK SaltedExplorer::EditSubclass(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
		case WM_KEYDOWN:
			switch(wParam)
			{
				case VK_RETURN:
					SendMessage(m_hContainer,CBN_KEYDOWN,VK_RETURN,0);
					return 0;
					break;
			}
			break;

		case WM_SETFOCUS:
			HandleToolbarItemStates();
			break;

		case WM_MOUSEWHEEL:
			if(OnMouseWheel(MOUSEWHEEL_SOURCE_OTHER,wParam,lParam))
			{
				return 0;
			}
			break;
	}

	return DefSubclassProc(hwnd,msg,wParam,lParam);
}

LRESULT CALLBACK RebarSubclassStub(HWND hwnd,UINT uMsg,
WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	SaltedExplorer *pContainer = (SaltedExplorer *)dwRefData;

	return pContainer->RebarSubclass(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK SaltedExplorer::RebarSubclass(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
		case WM_INITMENU:
			SendMessage(m_hContainer,WM_INITMENU,wParam,lParam);
			break;

		case WM_MENUSELECT:
			SendMessage(m_hContainer,WM_MENUSELECT,wParam,lParam);
			break;
	
		case WM_NOTIFY:
			switch(((LPNMHDR)lParam)->code)
			{
				case NM_RCLICK:
					{
						LPNMMOUSE	pnmm;
						LPNMHDR		pnmh;

						pnmm = (LPNMMOUSE)lParam;
						pnmh = &pnmm->hdr;

						if(pnmh->hwndFrom == m_hFavoritesToolbar)
						{
							/* TODO: [Favorites] Show favorites menu. */
						}
						else if(pnmh->hwndFrom == m_hDrivesToolbar)
						{
							if(pnmm->dwItemSpec != -1)
							{
								TBBUTTON	tbButton;
								TCHAR		*pszDrivePath;
								LONG		lResult;
								int			iIndex;

								iIndex = (int)SendMessage(m_hDrivesToolbar,TB_COMMANDTOINDEX,pnmm->dwItemSpec,0);

								if(iIndex != -1)
								{
									lResult = (int)SendMessage(m_hDrivesToolbar,TB_GETBUTTON,iIndex,(LPARAM)&tbButton);

									if(lResult)
									{
										pszDrivePath = (TCHAR *)tbButton.dwData;

										LPITEMIDLIST pidlItem = NULL;

										HRESULT hr = GetIdlFromParsingName(pszDrivePath,&pidlItem);

										if(SUCCEEDED(hr))
										{
											ClientToScreen(m_hDrivesToolbar,&pnmm->pt);

											std::list<LPITEMIDLIST> pidlItemList;

											CFileContextMenuManager fcmm(m_hDrivesToolbar,pidlItem,
												pidlItemList);

											FileContextMenuInfo_t fcmi;
											fcmi.uFrom = FROM_DRIVEBAR;

											CStatusBar StatusBar(m_hStatusBar);

											fcmm.ShowMenu(this,MIN_SHELL_MENU_ID,MAX_SHELL_MENU_ID,&pnmm->pt,&StatusBar,
												reinterpret_cast<DWORD_PTR>(&fcmi),FALSE,GetKeyState(VK_SHIFT) & 0x80);

											CoTaskMemFree(pidlItem);
										}
									}
								}
							}
							else
							{
								OnMainToolbarRClick();
							}
						}
						else if(pnmh->hwndFrom == m_hApplicationToolbar)
						{
							if(pnmm->dwItemSpec != -1)
							{
								int iItem;

								iItem = (int)SendMessage(m_hApplicationToolbar,
									TB_COMMANDTOINDEX,pnmm->dwItemSpec,0);

								m_iSelectedRClick = iItem;

								POINT ptCursor;
								DWORD dwPos;

								SetFocus(m_hApplicationToolbar);
								dwPos = GetMessagePos();
								ptCursor.x = GET_X_LPARAM(dwPos);
								ptCursor.y = GET_Y_LPARAM(dwPos);

								TrackPopupMenu(m_hApplicationRightClickMenu,TPM_LEFTALIGN,
									ptCursor.x,ptCursor.y,0,m_hMainRebar,NULL);
							}
							else
							{
								OnApplicationToolbarRClick();
							}
						}
						else
						{
							OnMainToolbarRClick();
						}
					}
					return TRUE;
					break;
			}
			break;
	}

	return DefSubclassProc(hwnd,msg,wParam,lParam);
}

void SaltedExplorer::SetStatusBarParts(int width)
{
	int Parts[3];

	Parts[0] = (int)(0.50 * width);
	Parts[1] = (int)(0.75 * width);
	Parts[2] = width;

	SendMessage(m_hStatusBar,SB_SETPARTS,3,(LPARAM)Parts);
}

void SaltedExplorer::ResizeWindows(void)
{
	RECT rc;

	GetClientRect(m_hContainer,&rc);
	SendMessage(m_hContainer,WM_SIZE,
	SIZE_RESTORED,(LPARAM)MAKELPARAM(rc.right,rc.bottom));
}

/* TODO: This should be linked to OnSize(). */
void SaltedExplorer::SetListViewInitialPosition(HWND hListView)
{
	RECT			rc;
	int				MainWindowWidth;
	int				MainWindowHeight;
	int				IndentBottom = 0;
	int				IndentTop = 0;
	int				IndentRight = 0;
	int				IndentLeft = 0;
	int				iIndentRebar = 0;

	GetClientRect(m_hContainer,&rc);

	MainWindowWidth = GetRectWidth(&rc);
	MainWindowHeight = GetRectHeight(&rc);

	if(m_hMainRebar)
	{
		GetWindowRect(m_hMainRebar,&rc);
		iIndentRebar += GetRectHeight(&rc);
	}

	if(m_bShowStatusBar)
	{
		GetWindowRect(m_hStatusBar,&rc);
		IndentBottom += GetRectHeight(&rc);
	}

	if(m_bShowDisplayWindow)
	{
		if (m_DisplayWindowVertical)
		{
			IndentRight += m_DisplayWindowWidth;
		}
		else
		{
			IndentBottom += m_DisplayWindowHeight;
		}
	}

	if(m_bShowFolders)
	{
		GetClientRect(m_hHolder,&rc);
		IndentLeft = GetRectWidth(&rc);
	}

	IndentTop = iIndentRebar;

	if(m_bShowTabBar)
	{
		if(!m_bShowTabBarAtBottom)
		{
			IndentTop += TAB_WINDOW_HEIGHT;
		}
	}

	if(!m_bShowTabBarAtBottom)
	{
		SetWindowPos(hListView,NULL,IndentLeft,IndentTop,
			MainWindowWidth - IndentLeft,MainWindowHeight -
			IndentBottom - IndentTop,
			SWP_HIDEWINDOW|SWP_NOZORDER);
	}
	else
	{
		SetWindowPos(hListView,NULL,IndentLeft,IndentTop,
			MainWindowWidth - IndentLeft,MainWindowHeight -
			IndentBottom - IndentTop - TAB_WINDOW_HEIGHT,
			SWP_HIDEWINDOW|SWP_NOZORDER);
	}
}

void SaltedExplorer::ToggleFolders(void)
{
	m_bShowFolders = !m_bShowFolders;
	lShowWindow(m_hHolder,m_bShowFolders);
	lShowWindow(m_hTreeView,m_bShowFolders);

	SendMessage(m_hMainToolbar,TB_CHECKBUTTON,(WPARAM)TOOLBAR_FOLDERS,(LPARAM)m_bShowFolders);
	ResizeWindows();
}

void SaltedExplorer::InsertToolbarButtons(void)
{
	std::list<ToolbarButton_t>::iterator	itr;
	TBBUTTON						*ptbButton = NULL;
	BYTE							StandardStyle;
	unsigned int					iCurrent = 0;

	/* Standard style that all toolbar buttons will have. */
	StandardStyle = BTNS_BUTTON | BTNS_AUTOSIZE;

	ptbButton = (TBBUTTON *)malloc(m_tbInitial.size() * sizeof(TBBUTTON));

	/* Fill out the button information... */
	for(itr = m_tbInitial.begin();itr != m_tbInitial.end();itr++)
	{
		if(iCurrent < m_tbInitial.size())
		{
			if(itr->iItemID == TOOLBAR_SEPARATOR)
			{
				ptbButton[iCurrent].iBitmap		= 0;
				ptbButton[iCurrent].idCommand	= 0;
				ptbButton[iCurrent].fsState		= TBSTATE_ENABLED;
				ptbButton[iCurrent].fsStyle		= BTNS_SEP;
				ptbButton[iCurrent].dwData		= 0;
				ptbButton[iCurrent].iString		= 0;
			}
			else
			{
				ptbButton[iCurrent].iBitmap		= LookupToolbarButtonImage(itr->iItemID);
				ptbButton[iCurrent].idCommand	= itr->iItemID;
				ptbButton[iCurrent].fsState		= TBSTATE_ENABLED;
				ptbButton[iCurrent].fsStyle		= StandardStyle | LookupToolbarButtonExtraStyles(itr->iItemID);
				ptbButton[iCurrent].dwData		= 0;
				ptbButton[iCurrent].iString		= (INT_PTR)itr->iItemID - TOOLBAR_BACK;
			}

			iCurrent++;
		}
	}

	/* Add the buttons to the toolbar. */
	SendMessage(m_hMainToolbar,TB_ADDBUTTONS,(WPARAM)iCurrent,(LPARAM)ptbButton);

	free(ptbButton);
}

void SaltedExplorer::InsertToolbarButton(ToolbarButton_t *ptb,int iPos)
{
	TBBUTTON	tbButton;
	BYTE		StandardStyle;
	TCHAR		*szText = NULL;

	StandardStyle = BTNS_BUTTON | BTNS_AUTOSIZE;

	if(ptb->iItemID == TOOLBAR_SEPARATOR)
	{
		tbButton.iBitmap	= 0;
		tbButton.idCommand	= 0;
		tbButton.fsState	= TBSTATE_ENABLED;
		tbButton.fsStyle	= BTNS_SEP;
		tbButton.dwData		= 0;
		tbButton.iString	= 0;
	}
	else
	{
		szText = (TCHAR *)malloc(64 * sizeof(TCHAR));

		LoadString(g_hLanguageModule,LookupToolbarButtonTextID(ptb->iItemID),
			szText,64);

		tbButton.iBitmap	= LookupToolbarButtonImage(ptb->iItemID);
		tbButton.idCommand	= ptb->iItemID;
		tbButton.fsState	= TBSTATE_ENABLED;
		tbButton.fsStyle	= StandardStyle | LookupToolbarButtonExtraStyles(ptb->iItemID);
		tbButton.dwData		= 0;
		tbButton.iString	= (INT_PTR)szText;
	}

	/* Add the button to the toolbar. */
	SendMessage(m_hMainToolbar,TB_INSERTBUTTON,(WPARAM)iPos,(LPARAM)&tbButton);

	HandleToolbarItemStates();
}

void SaltedExplorer::DeleteToolbarButton(int iButton)
{
	SendMessage(m_hMainToolbar,TB_DELETEBUTTON,iButton,0);
}

BOOL SaltedExplorer::OnTBQueryInsert(LPARAM lParam)
{
	return TRUE;
}

BOOL SaltedExplorer::OnTBQueryDelete(LPARAM lParam)
{
	/* All buttons can be deleted. */
	return TRUE;
}

BOOL SaltedExplorer::OnTBGetButtonInfo(LPARAM lParam)
{
	NMTOOLBAR		*pnmtb = NULL;
	static TCHAR	szText[64];
	int				id;

	pnmtb = (NMTOOLBAR *)lParam;

	/* The cast below is to fix C4018 (signed/unsigned mismatch). */
	if((pnmtb->iItem >= 0) && ((unsigned int)pnmtb->iItem < sizeof(ToolbarButtonSet) / sizeof(ToolbarButtonSet[0])))
	{
		id = ToolbarButtonSet[pnmtb->iItem];

		pnmtb->tbButton.fsState		= TBSTATE_ENABLED;
		pnmtb->tbButton.fsStyle		= BTNS_BUTTON | BTNS_AUTOSIZE | LookupToolbarButtonExtraStyles(id);
		pnmtb->tbButton.idCommand	= id;
		pnmtb->tbButton.iBitmap		= LookupToolbarButtonImage(id);
		pnmtb->tbButton.iString		= (INT_PTR)id - TOOLBAR_BACK;
		pnmtb->tbButton.dwData		= 0;

		StringCchCopy(pnmtb->pszText,pnmtb->cchText,szText);

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void SaltedExplorer::OnTBSave(LPARAM lParam)
{
	/* Can add custom information here. */
}

BOOL SaltedExplorer::OnTBRestore(LPARAM lParam)
{
	return 0;
}

void SaltedExplorer::OnTBReset(void)
{
	int nButtons;
	int i = 0;

	nButtons = (int)SendMessage(m_hMainToolbar,TB_BUTTONCOUNT,0,0);

	for(i = nButtons - 1;i >= 0;i--)
		SendMessage(m_hMainToolbar,TB_DELETEBUTTON,i,0);

	InsertToolbarButtons();
	HandleToolbarItemStates();
}

void SaltedExplorer::OnTBGetInfoTip(LPARAM lParam)
{
	NMTBGETINFOTIP	*ptbgit = NULL;
	LPITEMIDLIST	pidl = NULL;
	TCHAR			szInfoTip[1024];
	TCHAR			szPath[MAX_PATH];

	ptbgit = (NMTBGETINFOTIP *)lParam;

	StringCchCopy(ptbgit->pszText,ptbgit->cchTextMax,EMPTY_STRING);

	if(ptbgit->iItem == TOOLBAR_BACK)
	{
		if(m_pActiveShellBrowser->IsBackHistory())
		{
			pidl = m_pActiveShellBrowser->RetrieveHistoryItemWithoutUpdate(-1);

			GetDisplayName(pidl,szPath,SHGDN_INFOLDER);
			StringCchPrintf(szInfoTip,SIZEOF_ARRAY(szInfoTip),
				_T("Back to %s"),szPath);

			StringCchCopy(ptbgit->pszText,ptbgit->cchTextMax,szInfoTip);
		}
	}
	else if(ptbgit->iItem == TOOLBAR_FORWARD)
	{
		if(m_pActiveShellBrowser->IsForwardHistory())
		{
			pidl = m_pActiveShellBrowser->RetrieveHistoryItemWithoutUpdate(1);

			GetDisplayName(pidl,szPath,SHGDN_INFOLDER);
			StringCchPrintf(szInfoTip,SIZEOF_ARRAY(szInfoTip),
				_T("Forward to %s"),szPath);

			StringCchCopy(ptbgit->pszText,ptbgit->cchTextMax,szInfoTip);
		}
	}
	else if(ptbgit->iItem >= TOOLBAR_APPLICATIONS_ID_START)
	{
		TBBUTTON			tbButton;
		ApplicationButton_t *pab = NULL;
		LRESULT				lResult;
		int					iIndex;

		iIndex = (int)SendMessage(m_hApplicationToolbar,
			TB_COMMANDTOINDEX,ptbgit->iItem,0);

		if(iIndex != -1)
		{
			lResult = SendMessage(m_hApplicationToolbar,
				TB_GETBUTTON,iIndex,(LPARAM)&tbButton);

			if(lResult)
			{
				pab = (ApplicationButton_t *)tbButton.dwData;

				StringCchPrintf(ptbgit->pszText,ptbgit->cchTextMax,_T("%s\n%s"),
					pab->szName,pab->szCommand);
			}
		}
	}
	else if(ptbgit->iItem >= TOOLBAR_DRIVES_ID_START)
	{
		TBBUTTON	tbButton;
		TCHAR		*pszDrivePath;
		LRESULT		lResult;
		HRESULT		hr;
		int			iIndex;

		iIndex = (int)SendMessage(m_hDrivesToolbar,TB_COMMANDTOINDEX,ptbgit->iItem,0);

		if(iIndex != -1)
		{
			lResult = SendMessage(m_hDrivesToolbar,TB_GETBUTTON,iIndex,(LPARAM)&tbButton);

			if(lResult)
			{
				pszDrivePath = (TCHAR *)tbButton.dwData;

				/* Get the infotip for the drive, and use it as
				the items tooltip. */
				hr = GetItemInfoTip(pszDrivePath,szInfoTip,SIZEOF_ARRAY(szInfoTip));

				if(SUCCEEDED(hr))
				{
					StringCchCopy(ptbgit->pszText,ptbgit->cchTextMax,szInfoTip);
				}
			}
		}
	}
	else if(ptbgit->iItem >= TOOLBAR_FAVORITE_START)
	{
		int			iIndex;

		iIndex = (int)SendMessage(m_hFavoritesToolbar,TB_COMMANDTOINDEX,ptbgit->iItem,0);

		if(iIndex != -1)
		{
			/* TODO: [Favorites] Show info tip. */
		}
	}
}

void SaltedExplorer::OnAddressBarBeginDrag(void)
{
	IDragSourceHelper *pDragSourceHelper = NULL;
	IDropSource *pDropSource = NULL;
	HRESULT hr;

	hr = CoCreateInstance(CLSID_DragDropHelper,NULL,CLSCTX_ALL,
		IID_IDragSourceHelper,(LPVOID *)&pDragSourceHelper);

	if(SUCCEEDED(hr))
	{
		hr = CreateDropSource(&pDropSource,DRAG_TYPE_LEFTCLICK);

		if(SUCCEEDED(hr))
		{
			LPITEMIDLIST pidlDirectory = m_pActiveShellBrowser->QueryCurrentDirectoryIdl();

			FORMATETC ftc[2];
			STGMEDIUM stg[2];

			SetFORMATETC(&ftc[0],(CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR),
				NULL,DVASPECT_CONTENT,-1,TYMED_HGLOBAL);

			HGLOBAL hglb = GlobalAlloc(GMEM_MOVEABLE,1000);

			FILEGROUPDESCRIPTOR *pfgd = static_cast<FILEGROUPDESCRIPTOR *>(GlobalLock(hglb));

			pfgd->cItems = 1;

			FILEDESCRIPTOR *pfd = (FILEDESCRIPTOR *)((LPBYTE)pfgd + sizeof(UINT));

			/* File information (name, size, date created, etc). */
			pfd[0].dwFlags			= FD_ATTRIBUTES|FD_FILESIZE;
			pfd[0].dwFileAttributes	= FILE_ATTRIBUTE_NORMAL;
			pfd[0].nFileSizeLow		= 16384;
			pfd[0].nFileSizeHigh	= 0;

			/* The name of the file will be the folder name, followed by .lnk. */
			TCHAR szDisplayName[MAX_PATH];
			GetDisplayName(pidlDirectory,szDisplayName,SHGDN_INFOLDER);
			StringCchCat(szDisplayName,SIZEOF_ARRAY(szDisplayName),_T(".lnk"));
			StringCchCopy(pfd[0].cFileName,SIZEOF_ARRAY(pfd[0].cFileName),szDisplayName);

			GlobalUnlock(hglb);

			stg[0].pUnkForRelease	= 0;
			stg[0].hGlobal			= hglb;
			stg[0].tymed			= TYMED_HGLOBAL;

			/* File contents. */
			SetFORMATETC(&ftc[1],(CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILECONTENTS),
				NULL,DVASPECT_CONTENT,-1,TYMED_HGLOBAL);

			hglb = GlobalAlloc(GMEM_MOVEABLE,16384);

			IShellLink *pShellLink = NULL;
			IPersistStream *pPersistStream = NULL;
			HRESULT hr;

			hr = CoCreateInstance(CLSID_ShellLink,NULL,CLSCTX_INPROC_SERVER,
				IID_IShellLink,(LPVOID*)&pShellLink);

			if(SUCCEEDED(hr))
			{
				TCHAR szPath[MAX_PATH];

				GetDisplayName(pidlDirectory,szPath,SHGDN_FORPARSING);

				pShellLink->SetPath(szPath);

				hr = pShellLink->QueryInterface(IID_IPersistStream,(LPVOID*)&pPersistStream);

				if(SUCCEEDED(hr))
				{
					IStream *pStream = NULL;

					CreateStreamOnHGlobal(hglb,FALSE,&pStream);

					hr = pPersistStream->Save(pStream,TRUE);
				}
			}

			GlobalUnlock(hglb);

			stg[1].pUnkForRelease	= 0;
			stg[1].hGlobal			= hglb;
			stg[1].tymed			= TYMED_HGLOBAL;

			IDataObject *pDataObject = NULL;
			POINT pt = {0,0};

			hr = CreateDataObject(ftc,stg,&pDataObject,2);

			pDragSourceHelper->InitializeFromWindow(m_hAddressBar,&pt,pDataObject);

			DWORD dwEffect;

			DoDragDrop(pDataObject,pDropSource,DROPEFFECT_LINK,&dwEffect);

			CoTaskMemFree(pidlDirectory);

			pDataObject->Release();
			pDropSource->Release();
		}

		pDragSourceHelper->Release();
	}
}

void SaltedExplorer::AdjustMainToolbarSize(void)
{
	HIMAGELIST *phiml = NULL;
	HIMAGELIST *phiml1 = NULL;
	/* HIMAGELIST *phiml2 = NULL; */
	int cx;
	int cy;

	if(m_bLargeToolbarIcons)
	{
		cx = TOOLBAR_IMAGE_SIZE_LARGE_X;
		cy = TOOLBAR_IMAGE_SIZE_LARGE_Y;
		phiml = &m_himlToolbarLarge;
		phiml1 = &m_himlToolbarLargeInact;
		/* phiml2 = &m_himlToolbarLargeDisabled; */
	}
	else
	{
		cx = TOOLBAR_IMAGE_SIZE_SMALL_X;
		cy = TOOLBAR_IMAGE_SIZE_SMALL_Y;
		phiml = &m_himlToolbarSmall;
		phiml1 = &m_himlToolbarSmallInact;
		/* phiml2 = &m_himlToolbarSmallDisabled; */
	}

	/* Switch the image list. */
	SendMessage(m_hMainToolbar,TB_SETHOTIMAGELIST,0,(LPARAM)*phiml);
	SendMessage(m_hMainToolbar,TB_SETIMAGELIST,0,(LPARAM)*phiml1);
	/* SendMessage(m_hMainToolbar,TB_SETDISABLEDIMAGELIST,0,(LPARAM)*phiml2); */
	SendMessage(m_hMainToolbar,TB_SETBUTTONSIZE,0,MAKELPARAM(cx,cy));
	SendMessage(m_hMainToolbar,TB_AUTOSIZE,0,0);

	REBARBANDINFO rbi;
	DWORD dwSize;

	dwSize = (DWORD)SendMessage(m_hMainToolbar,TB_GETBUTTONSIZE,0,0);

	rbi.cbSize		= sizeof(rbi);
	rbi.fMask		= RBBIM_CHILDSIZE;
	rbi.cxMinChild	= 0;
	rbi.cyMinChild	= HIWORD(dwSize);
	rbi.cyChild		= HIWORD(dwSize);
	rbi.cyMaxChild	= HIWORD(dwSize);

	SendMessage(m_hMainRebar,RB_SETBANDINFO,0,(LPARAM)&rbi);
}