// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Explorer++_internal.h"
#include "Tab.h"

namespace Plugins
{
	class TabsApi
	{
	public:
		struct Tab
		{
			std::wstring location;

			Tab(const TabInfo_t &tabInternal)
			{
				TCHAR path[MAX_PATH];
				tabInternal.shellBrower->QueryCurrentDirectory(SIZEOF_ARRAY(path), path);

				location = path;
			}

			std::wstring toString()
			{
				return _T("location = ") + location;
			}
		};

		TabsApi(IExplorerplusplus *pexpp);
		~TabsApi();

		void create(std::wstring path);
		boost::optional<Tab> get(int tabId);

	private:

		IExplorerplusplus *m_pexpp;
	};
}