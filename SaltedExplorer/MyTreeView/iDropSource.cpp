/******************************************************************
 *
 * Project: MyTreeView
 * File: iDropSource.cpp
 *
 * Provides support for acting as a drop source.
 *
 * Toiletflusher and XP Pro
 * www.saltedexplorer.ml
 *
 *****************************************************************/

#include "stdafx.h"
#include "MyTreeView.h"
#include "MyTreeViewInternal.h"


HRESULT _stdcall CMyTreeView::QueryContinueDrag(BOOL fEscapePressed,DWORD grfKeyState)
{
	DWORD dwStopButton = 0;

	if(m_DragType == DRAG_TYPE_LEFTCLICK)
	{
		if((grfKeyState & MK_LBUTTON) == 0)
			return DRAGDROP_S_DROP;

		dwStopButton = MK_RBUTTON;
	}
	else if(m_DragType == DRAG_TYPE_RIGHTCLICK)
	{
		if((grfKeyState & MK_RBUTTON) == 0)
			return DRAGDROP_S_DROP;

		dwStopButton = MK_LBUTTON;
	}

	if(fEscapePressed == TRUE || grfKeyState & dwStopButton)
		return DRAGDROP_S_CANCEL;

	return S_OK;
}

HRESULT _stdcall CMyTreeView::GiveFeedback(DWORD dwEffect)
{
	return DRAGDROP_S_USEDEFAULTCURSORS;
}