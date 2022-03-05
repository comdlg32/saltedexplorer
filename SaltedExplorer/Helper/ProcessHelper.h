#pragma once

DWORD			GetProcessImageName(DWORD dwProcessId, TCHAR *szImageName, DWORD nSize);
BOOL			SetProcessTokenPrivilege(DWORD ProcessId,TCHAR *PrivilegeName,BOOL bEnablePrivilege);
BOOL			GetProcessOwner(TCHAR *szOwner,DWORD BufSize);