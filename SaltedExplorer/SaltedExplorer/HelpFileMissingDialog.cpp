/******************************************************************
 *
 * Project: SaltedExplorer
 * File: HelpFileMissingDialog.cpp
 *
 * Manages the dialog shown when the help file is not found.
 *
 * Toiletflusher and XP Pro
 * www.saltedexplorer.ml
 *
 *****************************************************************/

#include "stdafx.h"
#include "HelpFileMissingDialog.h"
#include "MainResource.h"
#include "../Helper/Helper.h"
#include "../Helper/WindowHelper.h"

CHelpFileMissingDialog::CHelpFileMissingDialog(HINSTANCE hInstance,int iResource,HWND hParent) :
CBaseDialog(hInstance,iResource,hParent,false)
{
	
}

CHelpFileMissingDialog::~CHelpFileMissingDialog()
{

}

BOOL CHelpFileMissingDialog::OnInitDialog()
{
	CenterWindow(GetParent(m_hDlg),m_hDlg);

	return TRUE;
}

BOOL CHelpFileMissingDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
	switch(LOWORD(wParam))
	{
	case IDOK:
		EndDialog(m_hDlg,1);
		break;

	case IDCANCEL:
		EndDialog(m_hDlg,0);
		break;
	}

	return 0;
}

BOOL CHelpFileMissingDialog::OnNotify(NMHDR *pnmhdr)
{
	switch(pnmhdr->code)
	{
	case NM_CLICK:
	case NM_RETURN:
		{
			if(pnmhdr->hwndFrom == GetDlgItem(m_hDlg,IDC_SYSLINK_DOWNLOAD))
			{
				PNMLINK pnmlink = reinterpret_cast<PNMLINK>(pnmhdr);
				ShellExecute(NULL,L"open",pnmlink->item.szUrl,
					NULL,NULL,SW_SHOW);
			}
		}
		break;
	}

	return 0;
}

BOOL CHelpFileMissingDialog::OnClose()
{
	EndDialog(m_hDlg,0);
	return 0;
}