/////////////////////////////////////////////////////////////////////////////
// DropWnd.h : header file
// 
// CAdvComboBox Control
// Version: 2.1
// Date: September 2002
// Author: Mathias Tunared
// Email: Mathias@inorbit.com
// Copyright (c) 2002. All Rights Reserved.
//
// This code, in compiled form or as source code, may be redistributed 
// unmodified PROVIDING it is not sold for profit without the authors 
// written consent, and providing that this notice and the authors name 
// and all copyright notices remains intact.
//
// This file is provided "as is" with no expressed or implied warranty.
// The author accepts no liability for any damage/loss of business that
// this product may cause.
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_DROPWND_H__F873AF0A_BD7D_4B20_BDF6_1EBB973AA348__INCLUDED_)
#define AFX_DROPWND_H__F873AF0A_BD7D_4B20_BDF6_1EBB973AA348__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DropWnd.h : header file
//
#pragma warning( disable : 4251 )
#pragma warning( disable : 4786 )

#include "AdvComboBoxDef.h"
#include <vector>

class CAdvComboBox;
class CDropListBox;
class CDropScrollBar;

/////////////////////////////////////////////////////////////////////////////
// CDropWnd window

class CDropWnd : public CWnd
{
public:
	CDropWnd( CAdvComboBox* pComboParent, std::vector<PLIST_ITEM> &itemlist );
	virtual ~CDropWnd();

	CDropListBox*	GetListBoxPtr() { return m_listbox; }
	CDropScrollBar* GetScrollBarPtr() { return m_scrollbar; }
	PLIST_ITEM GetListBoxItem(int nPos);
	PLIST_ITEM GetDropWndItem(int nPos);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDropWnd)
	public:
	virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	virtual BOOL DestroyWindow();
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

	// Generated message map functions
protected:
	//{{AFX_MSG(CDropWnd)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPaint();
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnActivateApp(BOOL bActive, HTASK hTask);
	//}}AFX_MSG
	afx_msg LONG OnSetCapture( WPARAM wParam, LPARAM lParam );
	afx_msg LONG OnReleaseCapture( WPARAM wParam, LPARAM lParam );
	DECLARE_MESSAGE_MAP()

private:
	CAdvComboBox* m_pComboParent;
	CDropListBox* m_listbox;
	CDropScrollBar* m_scrollbar;

	CRect m_rcList;
	CRect m_rcScroll;
	CRect m_rcSizeHandle;

	//list<PLIST_ITEM> m_pList;
	std::vector<PLIST_ITEM> m_pList;

	bool m_bResizing;
	int m_nMouseDiffX;
	int m_nMouseDiffY;

	int m_nVScrollW;
	int m_nVScrollH;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DROPWND_H__F873AF0A_BD7D_4B20_BDF6_1EBB973AA348__INCLUDED_)
