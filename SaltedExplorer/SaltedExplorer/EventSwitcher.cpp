/******************************************************************
 *
 * Project: SaltedExplorer
 * File: EvenSwitcher.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Switches events based on the currently selected window
 * (principally the listview and treeview).
 *
 
 * www.saltedexplorer.ml
 *
 *****************************************************************/

#include "stdafx.h"
#include "SaltedExplorer.h"

void SaltedExplorer::OnCopyItemPath(void)
{
	HWND hFocus;

	hFocus = GetFocus();

	if(hFocus == m_hActiveListView)
	{
		OnListViewCopyItemPath();
	}
	else if(hFocus == m_hTreeView)
	{
		OnTreeViewCopyItemPath();
	}
}

void SaltedExplorer::OnCopyUniversalPaths(void)
{
	HWND hFocus;

	hFocus = GetFocus();

	if(hFocus == m_hActiveListView)
	{
		OnListViewCopyUniversalPaths();
	}
	else if(hFocus == m_hTreeView)
	{
		OnTreeViewCopyUniversalPaths();
	}
}

void SaltedExplorer::OnCopy(BOOL bCopy)
{
	HWND hFocus;

	hFocus = GetFocus();

	if(hFocus == m_hActiveListView)
	{
		OnListViewCopy(bCopy);
	}
	else if(hFocus == m_hTreeView)
	{
		OnTreeViewCopy(bCopy);
	}
}

void SaltedExplorer::OnFileRename(void)
{
	HWND	hFocus;

	if(m_bListViewRenaming)
	{
		SendMessage(ListView_GetEditControl(m_hActiveListView),
			WM_USER_KEYDOWN,VK_F2,0);
	}
	else
	{
		hFocus = GetFocus();

		if(hFocus == m_hActiveListView)
		{
			OnListViewFileRename();
		}
		else if(hFocus == m_hTreeView)
		{
			OnTreeViewFileRename();
		}
	}
}

void SaltedExplorer::OnFileDelete(BOOL bPermanent)
{
	HWND hFocus;

	hFocus = GetFocus();

	if(hFocus == m_hActiveListView)
	{
		OnListViewFileDelete(bPermanent);
	}
	else if(hFocus == m_hTreeView)
	{
		OnTreeViewFileDelete(bPermanent);
	}
}

void SaltedExplorer::OnSetFileAttributes(void)
{
	HWND hFocus;

	hFocus = GetFocus();

	if(hFocus == m_hActiveListView)
	{
		OnListViewSetFileAttributes();
	}
	else if(hFocus == m_hTreeView)
	{
		OnTreeViewSetFileAttributes();
	}
}

void SaltedExplorer::OnShowFileProperties(void)
{
	HWND hFocus;

	hFocus = GetFocus();

	if(hFocus == m_hActiveListView)
	{
		OnListViewShowFileProperties();
	}
	else if(hFocus == m_hTreeView)
	{
		OnTreeViewShowFileProperties();
	}
}

void SaltedExplorer::OnRightClick(NMHDR *nmhdr)
{
	if(nmhdr->hwndFrom == m_hActiveListView)
	{
		POINT CursorPos;
		DWORD dwPos;

		dwPos = GetMessagePos();
		CursorPos.x = GET_X_LPARAM(dwPos);
		CursorPos.y = GET_Y_LPARAM(dwPos);

		OnListViewRClick(m_hActiveListView,&CursorPos);
	}
	else if(nmhdr->hwndFrom == ListView_GetHeader(m_hActiveListView))
	{
		/* The header on the active listview was right-clicked. */
		POINT CursorPos;
		DWORD dwPos;

		dwPos = GetMessagePos();
		CursorPos.x = GET_X_LPARAM(dwPos);
		CursorPos.y = GET_Y_LPARAM(dwPos);

		OnListViewHeaderRClick(&CursorPos);
	}
	else if(nmhdr->hwndFrom == m_hMainToolbar)
	{
		OnMainToolbarRClick();
	}
}

void SaltedExplorer::OnPaste(void)
{
	HWND hFocus;

	hFocus = GetFocus();

	if(hFocus == m_hActiveListView)
	{
		OnListViewPaste();
	}
	else if(hFocus == m_hTreeView)
	{
		OnTreeViewPaste();
	}
}