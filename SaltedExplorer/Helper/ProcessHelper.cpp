/******************************************************************
*
* Project: Helper
* File: ProcessHelper.cpp
*
* Process helper functionality.
*
* Toiletflusher and XP Pro
* www.saltedexplorer.ml
*
*****************************************************************/

#include "stdafx.h"
#include "Helper.h"
#include "ProcessHelper.h"
#include "Macros.h"


DWORD GetProcessImageName(DWORD dwProcessId, TCHAR *szImageName, DWORD nSize)
{
	DWORD dwRet = 0;

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcessId);

	if(hProcess != NULL)
	{
		dwRet = GetModuleFileNameEx(hProcess, NULL, szImageName, nSize);
		CloseHandle(hProcess);
	}

	return dwRet;
}

BOOL GetProcessOwner(TCHAR *szOwner,DWORD BufSize)
{
	HANDLE hProcess;
	HANDLE hToken;
	TOKEN_USER *pTokenUser = NULL;
	SID_NAME_USE eUse;
	LPTSTR StringSid;
	TCHAR szAccountName[512];
	DWORD dwAccountName = SIZEOF_ARRAY(szAccountName);
	TCHAR szDomainName[512];
	DWORD dwDomainName = SIZEOF_ARRAY(szDomainName);
	DWORD ReturnLength;
	DWORD dwSize = 0;
	BOOL bRes;
	BOOL bReturn = FALSE;

	hProcess = OpenProcess(PROCESS_ALL_ACCESS,FALSE,GetCurrentProcessId());

	if(hProcess != NULL)
	{
		bRes = OpenProcessToken(hProcess,TOKEN_ALL_ACCESS,&hToken);

		if(bRes)
		{
			GetTokenInformation(hToken,TokenUser,NULL,0,&dwSize);

			pTokenUser = (PTOKEN_USER)GlobalAlloc(GMEM_FIXED,dwSize);

			if(pTokenUser != NULL)
			{
				GetTokenInformation(hToken,TokenUser,(LPVOID)pTokenUser,dwSize,&ReturnLength);

				bRes = LookupAccountSid(NULL,pTokenUser->User.Sid,szAccountName,&dwAccountName,
					szDomainName,&dwDomainName,&eUse);

				/* LookupAccountSid failed. */
				if(bRes == 0)
				{
					bRes = ConvertSidToStringSid(pTokenUser->User.Sid,&StringSid);

					if(bRes != 0)
					{
						StringCchCopy(szOwner,BufSize,StringSid);

						LocalFree(StringSid);

						bReturn = TRUE;
					}
				}
				else
				{
					StringCchPrintf(szOwner,BufSize,_T("%s\\%s"),szDomainName,szAccountName);

					bReturn = TRUE;
				}

				GlobalFree(pTokenUser);
			}
		}
		CloseHandle(hProcess);
	}

	if(!bReturn)
		StringCchCopy(szOwner,BufSize,EMPTY_STRING);

	return bReturn;
}

BOOL SetProcessTokenPrivilege(DWORD ProcessId,TCHAR *PrivilegeName,BOOL bEnablePrivilege)
{
	HANDLE hProcess;
	HANDLE hToken;
	TOKEN_PRIVILEGES tp;
	LUID luid;

	hProcess = OpenProcess(PROCESS_ALL_ACCESS,FALSE,ProcessId);

	if(hProcess == NULL)
		return FALSE;

	OpenProcessToken(hProcess,TOKEN_ALL_ACCESS,&hToken);

	LookupPrivilegeValue(NULL,PrivilegeName,&luid);

	tp.PrivilegeCount				= 1;
	tp.Privileges[0].Luid			= luid;

	if(bEnablePrivilege)
		tp.Privileges[0].Attributes	= SE_PRIVILEGE_ENABLED;
	else
		tp.Privileges[0].Attributes	= 0;

	CloseHandle(hProcess);

	return AdjustTokenPrivileges(hToken,FALSE,&tp,0,NULL,NULL);
}