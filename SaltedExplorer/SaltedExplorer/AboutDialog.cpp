
#include "stdafx.h"
#include <list>
#include "SaltedExplorer_internal.h"
#include "AboutDialog.h"
#include "MainResource.h"
#include "Version.h"
#include "../Helper/Helper.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/Macros.h"


CAboutDialog::CAboutDialog(HINSTANCE hInstance,int iResource,HWND hParent) :
CBaseDialog(hInstance,iResource,hParent,false)
{
	
}

CAboutDialog::~CAboutDialog()
{

}

BOOL CAboutDialog::OnInitDialog()
{
	m_hIcon = reinterpret_cast<HICON>(LoadImage(GetModuleHandle(0),
		MAKEINTRESOURCE(IDI_MAIN),IMAGE_ICON,
		32,32,LR_VGACOLOR));

	SendMessage(m_hDlg,WM_SETICON,ICON_SMALL,reinterpret_cast<LPARAM>(m_hIcon));

	/* If the dialog has been loaded from a resource other than
	the one in the executable (which will be the case, for example,
	if a translation DLL has been loaded), then the image that
	normally appears won't be shown. This is because the static
	control will attempt to load it from its resource section (where
	the image doesn't exist). Manually set the image here. */
	if(GetInstance() != GetModuleHandle(0))
	{
		HBITMAP hbm = LoadBitmap(GetModuleHandle(0),MAKEINTRESOURCE(IDB_ABOUT));
		SendDlgItemMessage(m_hDlg,IDC_ABOUT_STATIC_IMAGE,STM_SETIMAGE,
			IMAGE_BITMAP,reinterpret_cast<LPARAM>(hbm));
	}

	TCHAR szVersion[64];
	TCHAR szBuild[64];
	TCHAR szBuildDate[64];
	TCHAR szBuildNumber[64];
	TCHAR szYearNumber[64];
	TCHAR szDebugMode[64];
	TCHAR szTemp[64];

	/* Indicate which architecture (32-bit or
	64-bit) we are building for in the version
	string.*/
#ifdef WIN64

#else

#endif




	GetDlgItemText(m_hDlg,IDC_STATIC_VERSIONNUMBER,szTemp,SIZEOF_ARRAY(szTemp));
	StringCchPrintf(szVersion,SIZEOF_ARRAY(szVersion),szTemp,VERSION_STRING_W,szBuild);
	SetDlgItemText(m_hDlg,IDC_STATIC_VERSIONNUMBER,szVersion);

	GetDlgItemText(m_hDlg,IDC_STATIC_BUILDDATE,szTemp,SIZEOF_ARRAY(szTemp));
	StringCchPrintf(szBuildDate,SIZEOF_ARRAY(szBuildDate),szTemp,BUILD_DATE);
	SetDlgItemText(m_hDlg,IDC_STATIC_BUILDDATE,szBuildDate);

	GetDlgItemText(m_hDlg,IDC_STATIC_BUILDNUMBER,szTemp,SIZEOF_ARRAY(szTemp));
	StringCchPrintf(szBuildNumber,SIZEOF_ARRAY(szBuildNumber),szTemp,BUILD_NUMBER);
	SetDlgItemText(m_hDlg,IDC_STATIC_BUILDNUMBER,szBuildNumber);

	GetDlgItemText(m_hDlg,IDC_STATIC_YEAR,szTemp,SIZEOF_ARRAY(szTemp));
	StringCchPrintf(szYearNumber,SIZEOF_ARRAY(szYearNumber),szTemp,YEAR_NUMBER);
	SetDlgItemText(m_hDlg,IDC_STATIC_YEAR,szYearNumber);

	GetDlgItemText(m_hDlg,IDC_STATIC_DEBUG,szTemp,SIZEOF_ARRAY(szTemp));
	StringCchPrintf(szDebugMode,SIZEOF_ARRAY(szDebugMode),szTemp,DEBUG_STRING);
	SetDlgItemText(m_hDlg,IDC_STATIC_DEBUG,szDebugMode);

	CenterWindow(GetParent(m_hDlg),m_hDlg);

	return TRUE;
}

BOOL CAboutDialog::OnCommand(WPARAM wParam,LPARAM lParam)
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

BOOL CAboutDialog::OnNotify(NMHDR *pnmhdr)
{
	switch(pnmhdr->code)
	{
	case NM_CLICK:
	case NM_RETURN:
		{
			if(pnmhdr->hwndFrom == GetDlgItem(m_hDlg,IDC_SITELINK))
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

BOOL CAboutDialog::OnClose()
{
	DestroyIcon(m_hIcon);

	EndDialog(m_hDlg,0);
	return 0;
}
