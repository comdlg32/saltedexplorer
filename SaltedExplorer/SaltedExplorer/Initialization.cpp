/******************************************************************
 *
 * Project: SaltedExplorer
 * File: Initialization.cpp
 *
 * Includes miscellaneous functions related to
 * the top-level GUI component.
 *
 * Toiletflusher and XP Pro
 *
 * www.saltedexplorer.ml
 *
 *****************************************************************/

#include "stdafx.h"
#include <list>
#include "SaltedExplorer.h"
#include "CustomizeColorsDialog.h"
#include "FavoritesHelper.h"
#include "../Helper/FileOperations.h"
#include "../Helper/Helper.h"
#include "../Helper/Controls.h"
#include "../Helper/Macros.h"
#include "MainResource.h"
#include "gdiplus.h"

extern HIMAGELIST himlMenu;

void SaltedExplorer::InitializeFavorites(void)
{
	TCHAR szTemp[64];

	LoadString(g_hLanguageModule,IDS_FAVORITES_ALLFAVORITES,szTemp,SIZEOF_ARRAY(szTemp));
	m_bfAllFavorites = CFavoriteFolder::CreateNew(szTemp);

	/* Set up the 'Favorites Toolbar' and 'Favorites Menu' folders. */
	LoadString(g_hLanguageModule,IDS_FAVORITES_FAVORITESTOOLBAR,szTemp,SIZEOF_ARRAY(szTemp));
	CFavoriteFolder bfFavoritesToolbar = CFavoriteFolder::Create(szTemp);
	m_bfAllFavorites->InsertFavoriteFolder(bfFavoritesToolbar);

	LoadString(g_hLanguageModule,IDS_FAVORITES_FAVORITESMENU,szTemp,SIZEOF_ARRAY(szTemp));
	CFavoriteFolder bfFavoritesMenu = CFavoriteFolder::Create(szTemp);
	m_bfAllFavorites->InsertFavoriteFolder(bfFavoritesMenu);

	m_pipbin = new CIPFavoriteItemNotifier(m_hContainer);
	CFavoriteItemNotifier::GetInstance().AddObserver(m_pipbin);

}

void SaltedExplorer::InitializeDisplayWindow(void)
{
	DWInitialSettings_t	InitialSettings;

	InitialSettings.CentreColor		= m_DisplayWindowCentreColor;
	InitialSettings.SurroundColor	= m_DisplayWindowSurroundColor;
	InitialSettings.TextColor		= m_DisplayWindowTextColor;
	InitialSettings.hFont			= m_DisplayWindowFont;
	InitialSettings.hIcon			= (HICON)LoadImage(GetModuleHandle(0),
		MAKEINTRESOURCE(IDI_DISPLAYWINDOW),IMAGE_ICON,
		0,0,LR_CREATEDIBSECTION);

	m_hDisplayWindow = CreateDisplayWindow(m_hContainer,
		&m_pDisplayMain,&InitialSettings);
}

void SaltedExplorer::InitializeMenus(void)
{
	HMENU	hMenu;
	HBITMAP	hBitmap;
	int		nTopLevelMenus;
	int		i = 0;

	hMenu = m_hMenu;

	/* Insert the view mode (icons, small icons, details, etc) menus in. */
	MENUITEMINFO mii;
	std::list<ViewMode_t>::iterator itr;
	TCHAR szText[64];

	for(itr = m_ViewModes.begin();itr != m_ViewModes.end();itr++)
	{
		LoadString(g_hLanguageModule,GetViewModeMenuStringId(itr->uViewMode),
			szText,SIZEOF_ARRAY(szText));

		mii.cbSize		= sizeof(mii);
		mii.fMask		= MIIM_ID|MIIM_STRING;
		mii.wID			= GetViewModeMenuId(itr->uViewMode);
		mii.dwTypeData	= szText;
		InsertMenuItem(hMenu,IDM_VIEW_PLACEHOLDER,FALSE,&mii);

		InsertMenuItem(m_hViewsMenu,IDM_VIEW_PLACEHOLDER,FALSE,&mii);
	}

	/* Delete the placeholder menu. */
	DeleteMenu(hMenu,IDM_VIEW_PLACEHOLDER,MF_BYCOMMAND);
	DeleteMenu(m_hViewsMenu,IDM_VIEW_PLACEHOLDER,MF_BYCOMMAND);

	nTopLevelMenus = GetMenuItemCount(hMenu);

	/* Loop through each of the top level menus, setting
	all sub menus to owner drawn. Don't set top-level
	parent menus to owner drawn. */
	for(i = 0;i < nTopLevelMenus;i++)
	{
		SetMenuOwnerDraw(GetSubMenu(hMenu,i));
	}

	himlMenu = ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,0,48);
	
	/* Contains all images used on the menus. */
	hBitmap = LoadBitmap(GetModuleHandle(0),MAKEINTRESOURCE(IDB_SHELLIMAGES_2000));

	ImageList_Add(himlMenu,hBitmap,NULL);

	/* <---- Tab right click menu ----> */
	SetMenuOwnerDraw(m_hTabRightClickMenu);

	/* CCustomMenu will handle the drawing of all owner drawn menus. */
	m_pCustomMenu = new CCustomMenu(m_hContainer,hMenu,himlMenu);

	SetGoMenuName(hMenu,IDM_GO_MYCOMPUTER,CSIDL_DRIVES);
	SetGoMenuName(hMenu,IDM_GO_MYDOCUMENTS,CSIDL_PERSONAL);
	SetGoMenuName(hMenu,IDM_GO_MYMUSIC,CSIDL_MYMUSIC);
	SetGoMenuName(hMenu,IDM_GO_MYPICTURES,CSIDL_MYPICTURES);
	SetGoMenuName(hMenu,IDM_GO_DESKTOP,CSIDL_DESKTOP);
	SetGoMenuName(hMenu,IDM_GO_RECYCLEBIN,CSIDL_BITBUCKET);
	SetGoMenuName(hMenu,IDM_GO_CONTROLPANEL,CSIDL_CONTROLS);
	SetGoMenuName(hMenu,IDM_GO_PRINTERS,CSIDL_PRINTERS);
	SetGoMenuName(hMenu,IDM_GO_CDBURNING,CSIDL_CDBURN_AREA);
	SetGoMenuName(hMenu,IDM_GO_MYNETWORKPLACES,CSIDL_NETWORK);
	SetGoMenuName(hMenu,IDM_GO_NETWORKCONNECTIONS,CSIDL_CONNECTIONS);

	DeleteObject(hBitmap);

	LANGID language = GetUserDefaultUILanguage();
	HMODULE baseBrd = LoadMUILibrary(TEXT("C:\\Windows\\Branding\\Basebrd\\basebrd.dll"), MUI_LANGUAGE_NAME, language);
	const DWORD bufferSize = 256;
	TCHAR brandingString[bufferSize];
	LoadString(baseBrd, 10, brandingString, bufferSize);
	
	TCHAR buffer[bufferSize];
	mii.cbSize	     = sizeof(mii);
	GetMenuItemInfo(hMenu, IDM_HELP_LEGAL, FALSE, &mii);
	_stprintf(buffer, bufferSize, mii.dwTypeData, brandingString);

	mii.cbSize	    = sizeof(mii);
	mii.fMask		= MIIM_STRING;
	mii.dwTypeData	= buffer;
	SetMenuItemInfo(hMenu, IDM_HELP_LEGAL, FALSE, &mii);

	/* Arrange submenu. */
	SetMenuOwnerDraw(m_hArrangeSubMenu);

	/* Group by submenu. */
	SetMenuOwnerDraw(m_hGroupBySubMenu);
}

void SaltedExplorer::SetDefaultTabSettings(TabInfo_t *pTabInfo)
{
	pTabInfo->bLocked			= FALSE;
	pTabInfo->bAddressLocked	= FALSE;
	pTabInfo->bUseCustomName	= FALSE;
	StringCchCopy(pTabInfo->szName,
		SIZEOF_ARRAY(pTabInfo->szName),EMPTY_STRING);
}

void SaltedExplorer::InitializeColorRules(void)
{
	ColorRule_t ColorRule;
	TCHAR szTemp[64];

	LoadString(g_hLanguageModule,IDS_GENERAL_COLOR_RULE_COMPRESSED,szTemp,SIZEOF_ARRAY(szTemp));
	ColorRule.strDescription		= szTemp;
	ColorRule.rgbColour				= CF_COMPRESSED;
	ColorRule.dwFilterAttributes	= FILE_ATTRIBUTE_COMPRESSED;
	m_ColorRuleList.push_back(ColorRule);

	LoadString(g_hLanguageModule,IDS_GENERAL_COLOR_RULE_ENCRYPTED,szTemp,SIZEOF_ARRAY(szTemp));
	ColorRule.strDescription		= szTemp;
	ColorRule.rgbColour				= CF_ENCRYPTED;
	ColorRule.dwFilterAttributes	= FILE_ATTRIBUTE_ENCRYPTED;
	m_ColorRuleList.push_back(ColorRule);
}