/******************************************************************
 *
 * Project: SaltedExplorer
 * File: FavoritesHandler.cpp
 *
 * Handles tasks associated with Favorites,
 * such as creating a Favorites menu, and
 * adding Favorites to a toolbar.
 *
 * www.saltedexplorer.ml
 *
 *****************************************************************/

#include "stdafx.h"
#include "SaltedExplorer.h"
#include "AddFavoritesDialog.h"
#include "NewFavoriteFolderDialog.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/Macros.h"

LRESULT CALLBACK FavoritesToolbarSubclassStub(HWND hwnd,UINT uMsg,
WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	SaltedExplorer *pContainer = reinterpret_cast<SaltedExplorer *>(dwRefData);

	return pContainer->FavoritesToolbarSubclass(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK SaltedExplorer::FavoritesToolbarSubclass(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_MBUTTONUP:
		{
			DWORD dwPos = GetMessagePos();

			POINT ptCursor;
			ptCursor.x = GET_X_LPARAM(dwPos);
			ptCursor.y = GET_Y_LPARAM(dwPos);
			MapWindowPoints(HWND_DESKTOP,m_hFavoritesToolbar,&ptCursor,1);

			int iIndex = static_cast<int>(SendMessage(m_hFavoritesToolbar,TB_HITTEST,0,
				reinterpret_cast<LPARAM>(&ptCursor)));

			if(iIndex >= 0)
			{
				TBBUTTON tbButton;
				SendMessage(m_hFavoritesToolbar,TB_GETBUTTON,iIndex,reinterpret_cast<LPARAM>(&tbButton));

				/* TODO: [FAVORITES] If this is a Favorite, open it in a new tab. */
			}
		}
		break;
	}

	return DefSubclassProc(hwnd,uMsg,wParam,lParam);
}

void SaltedExplorer::InsertFavoritesIntoMenu(void)
{
	/* TODO: [FAVORITES] Rewrite. */
}

void SaltedExplorer::InsertFavoriteToolbarButtons(void)
{
	HIMAGELIST himl = ImageList_Create(16,16,ILC_COLOR32 | ILC_MASK,0,1);
	HBITMAP hb = LoadBitmap(GetModuleHandle(0),MAKEINTRESOURCE(IDB_SHELLIMAGES_2000));
	ImageList_Add(himl,hb,NULL);
	SendMessage(m_hFavoritesToolbar,TB_SETIMAGELIST,0,reinterpret_cast<LPARAM>(himl));
	DeleteObject(hb);

	/* TODO: [FAVORITES] Rewrite. */
}

void SaltedExplorer::FavoriteToolbarNewFavorite(int iItem)
{

	if(iItem != -1)
	{
		/* TODO: Need to retrieve Favorite details. */
		/*TBBUTTON tbButton;
		SendMessage(m_hFavoritesToolbar,TB_GETBUTTON,iItem,(LPARAM)&tbButton);*/

		CFavorite Favorite(EMPTY_STRING,EMPTY_STRING,EMPTY_STRING);

		CAddFavoritesDialog AddFavoritesDialog(g_hLanguageModule,IDD_ADD_FAVORITES,m_hContainer,*m_bfAllFavorites,Favorite);
		AddFavoritesDialog.ShowModalDialog();
	}
}

void SaltedExplorer::FavoriteToolbarNewFolder(int iItem)
{
	CNewFavoriteFolderDialog NewFavoriteFolderDialog(g_hLanguageModule,IDD_NEWFAVORITEFOLDER,m_hContainer);
	NewFavoriteFolderDialog.ShowModalDialog();
}

HRESULT SaltedExplorer::ExpandAndBrowsePath(TCHAR *szPath)
{
	return ExpandAndBrowsePath(szPath,FALSE,FALSE);
}

/* Browses to the specified path. The path may
have any environment variables expanded (if
necessary). */
HRESULT SaltedExplorer::ExpandAndBrowsePath(TCHAR *szPath,BOOL bOpenInNewTab,BOOL bSwitchToNewTab)
{
	TCHAR szExpandedPath[MAX_PATH];

	MyExpandEnvironmentStrings(szPath,
		szExpandedPath,SIZEOF_ARRAY(szExpandedPath));

	return BrowseFolder(szExpandedPath,SBSP_ABSOLUTE,bOpenInNewTab,bSwitchToNewTab,FALSE);
}