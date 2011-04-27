// CDHeaderCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "CDListCtrl.h"
#include "CDHeaderCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCDHeaderCtrl

CCDHeaderCtrl::CCDHeaderCtrl()
{
	m_cr3DHighLight   = ::GetSysColor(COLOR_3DHIGHLIGHT);
	m_cr3DShadow      = ::GetSysColor(COLOR_3DSHADOW);
	m_cr3DFace        = ::GetSysColor(COLOR_3DFACE);
	m_crBtnText       = ::GetSysColor(COLOR_BTNTEXT);
	
	m_pListCtrl       = NULL;			//+++
	m_nFormat         = DT_DEFAULT;		//+++
	m_rgbText         = m_crBtnText;	//+++
	m_bDividerLines   = TRUE;			//+++
	m_bDoubleBuffer   = TRUE;
	m_iSpacing        = 6;
	m_sizeArrow.cx    = 8;
	m_sizeArrow.cy    = 8;
	m_sizeImage.cx    = 0;
	m_sizeImage.cy    = 0;
	m_bStaticBorder   = FALSE;
	m_nDontDropCursor = 0;
	m_bResizing       = FALSE;
	m_nClickFlags     = 0;
}

CCDHeaderCtrl::~CCDHeaderCtrl()
{
}


BEGIN_MESSAGE_MAP(CCDHeaderCtrl, CHeaderCtrl)
	//{{AFX_MSG_MAP(CCDHeaderCtrl)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDBLCLK()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCDHeaderCtrl message handlers

void CCDHeaderCtrl::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	// TODO: Add your message handler code here
	DrawCtrl(&dc);
	// Do not call CHeaderCtrl::OnPaint() for painting messages
}

BOOL CCDHeaderCtrl::OnEraseBkgnd(CDC* pDC) 
{
	return TRUE;
}
void CCDHeaderCtrl::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	
	SendMessage(WM_LBUTTONDOWN, nFlags, MAKELPARAM(point.x, point.y));
	
	//CHeaderCtrl::OnLButtonDown(nFlags, point);
}


///////////////////////////////////////////////////////////////////////////////
// DrawCtrl
void CCDHeaderCtrl::DrawCtrl(CDC* pDC)
{
	CRect rectClip;
	if (pDC->GetClipBox(&rectClip) == ERROR)
		return;

	CRect rectClient, rectItem;
	GetClientRect(&rectClient);

    pDC->FillSolidRect(rectClip, m_cr3DFace);

	int iItems = GetItemCount();
	ASSERT(iItems >= 0);

	CPen penHighLight(PS_SOLID, 1, m_cr3DHighLight);
	CPen penShadow(PS_SOLID, 1, m_cr3DShadow);
	CPen* pPen = pDC->GetCurrentPen();

	CFont* pFont = pDC->SelectObject(GetFont());

	pDC->SetBkColor(m_cr3DFace);
	pDC->SetTextColor(m_crBtnText);

	int iWidth = 0;

	for (int i = 0; i < iItems; i++)
	{
		int iItem = OrderToIndex(i);

		TCHAR szText[FLATHEADER_TEXT_MAX];

		HDITEM hditem;
		hditem.mask = HDI_WIDTH|HDI_FORMAT|HDI_TEXT|HDI_IMAGE|HDI_BITMAP;
		hditem.pszText = szText;
		hditem.cchTextMax = sizeof(szText);
		VERIFY(GetItem(iItem, &hditem));

		VERIFY(GetItemRect(iItem, rectItem));

		if (rectItem.right >= rectClip.left || rectItem.left <= rectClip.right)
		{
			if (hditem.fmt & HDF_OWNERDRAW)
			{
				DRAWITEMSTRUCT disItem;
				disItem.CtlType = ODT_BUTTON;
				disItem.CtlID = GetDlgCtrlID();
				disItem.itemID = iItem;
				disItem.itemAction = ODA_DRAWENTIRE;
				disItem.itemState = 0;
				disItem.hwndItem = m_hWnd;
				disItem.hDC = pDC->m_hDC;
				disItem.rcItem = rectItem;
				disItem.itemData = 0;

				DrawItem(&disItem);
			}
			else
			{
				rectItem.DeflateRect(m_iSpacing, 0);
				DrawItem(pDC, rectItem, &hditem);
				rectItem.InflateRect(m_iSpacing, 0);

				//if (m_nClickFlags & MK_LBUTTON && m_iHotIndex == iItem && m_hdhtiHotItem.flags & HHT_ONHEADER)
				//	pDC->InvertRect(rectItem);
			}

			if (i < iItems-1)
			{
				// draw divider lines
				if (m_bDividerLines)		//+++
				{
					pDC->SelectObject(&penShadow);
					pDC->MoveTo(rectItem.right-1, rectItem.top+2);
					pDC->LineTo(rectItem.right-1, rectItem.bottom-2);

					pDC->SelectObject(&penHighLight);
					pDC->MoveTo(rectItem.right, rectItem.top+2);
					pDC->LineTo(rectItem.right, rectItem.bottom-2);
				}
			}
		}

		iWidth += hditem.cxy;
	}

	if (iWidth > 0)
	{
		rectClient.right = rectClient.left + iWidth;
		pDC->Draw3dRect(rectClient, m_cr3DHighLight, m_cr3DShadow);
	}

	pDC->SelectObject(pFont);
	pDC->SelectObject(pPen);

	penHighLight.DeleteObject();
	penShadow.DeleteObject();
}

///////////////////////////////////////////////////////////////////////////////
// DrawItem
void CCDHeaderCtrl::DrawItem(LPDRAWITEMSTRUCT)
{
	ASSERT(FALSE);  // must override for self draw header controls
}

///////////////////////////////////////////////////////////////////////////////
// DrawItem
void CCDHeaderCtrl::DrawItem(CDC* pDC, CRect rect, LPHDITEM lphdi)
{
	ASSERT(lphdi->mask & HDI_FORMAT);

	int iWidth = 0;

	CBitmap* pBitmap = NULL;
	BITMAP BitmapInfo;

	if (lphdi->fmt & HDF_BITMAP)
	{
		ASSERT(lphdi->mask & HDI_BITMAP);
		ASSERT(lphdi->hbm);

		pBitmap = CBitmap::FromHandle(lphdi->hbm);
		if (pBitmap)
			VERIFY(pBitmap->GetObject(sizeof(BITMAP), &BitmapInfo));
	}

	rect.left += ((iWidth = DrawImage(pDC, rect, lphdi, FALSE)) != 0) ? 
		iWidth + m_iSpacing : 0;

	rect.right -= ((iWidth = DrawBitmap(pDC, rect, lphdi, pBitmap, &BitmapInfo, TRUE)) != 0) ? 
		iWidth + m_iSpacing : 0;

	DrawText(pDC, rect, lphdi);
}

///////////////////////////////////////////////////////////////////////////////
// DrawImage
int CCDHeaderCtrl::DrawImage(CDC* pDC, CRect rect, LPHDITEM lphdi, BOOL bRight)
{
	int iWidth = 0;

	if (lphdi->iImage != XHEADERCTRL_NO_IMAGE)
	{
		CImageList* pImageList = GetImageList();

		if (pImageList && (rect.Width() > 0))
		{
			POINT point;

			point.y = rect.CenterPoint().y - ((m_sizeImage.cy+1) >> 1);

			if (bRight)
				point.x = rect.right - m_sizeImage.cx;
			else
				point.x = rect.left;

			SIZE size;
			size.cx = rect.Width() < m_sizeImage.cx ? rect.Width() : m_sizeImage.cx;
			size.cy = m_sizeImage.cy;

			// save image list background color
			COLORREF rgb = pImageList->GetBkColor();

			// set image list background color to same as header control
			pImageList->SetBkColor(pDC->GetBkColor());
			pImageList->DrawIndirect(pDC, lphdi->iImage, point, size, CPoint(0, 0));
			pImageList->SetBkColor(rgb);

			iWidth = m_sizeImage.cx;
		}
		else if (rect.Width() > 0)
		{
			// no image list, just draw checkbox

			CRect chkboxrect = rect;

			// center the checkbox

			int h = 13;	// height and width are the same
			chkboxrect.right = chkboxrect.left + h;
			chkboxrect.top = rect.top + (rect.Height() - h) / 2;
			chkboxrect.bottom = chkboxrect.top + h;

			// fill rect inside checkbox with white
			COLORREF rgbBackground = pDC->SetBkColor(::GetSysColor(COLOR_WINDOW));
			pDC->FillSolidRect(&chkboxrect, ::GetSysColor(COLOR_WINDOW));

			// draw border
			CBrush brush(RGB(51,102,153));
			pDC->FrameRect(&chkboxrect, &brush);

			if (lphdi->iImage == XHEADERCTRL_CHECKED_IMAGE)
			{
				CPen blackpen(PS_SOLID, 1, RGB(51,153,51));
				CPen * pOldPen = pDC->SelectObject(&blackpen);

				// draw the checkmark
				int x = chkboxrect.left + 9;
				ASSERT(x < chkboxrect.right);
				int y = chkboxrect.top + 3;
				int i = 0;
				for (i = 0; i < 4; i++)
				{
					pDC->MoveTo(x, y);
					pDC->LineTo(x, y+3);
					x--;
					y++;
				}
				for (i = 0; i < 3; i++)
				{
					pDC->MoveTo(x, y);
					pDC->LineTo(x, y+3);
					x--;
					y--;
				}

				if (pOldPen)
					pDC->SelectObject(pOldPen);
			}

			pDC->SetBkColor(rgbBackground);

			iWidth = chkboxrect.Width();
		}
		else
		{
			// width = 0, do nothing
		}
	}

	return iWidth;
}

///////////////////////////////////////////////////////////////////////////////
// DrawBitmap
int CCDHeaderCtrl::DrawBitmap(CDC* pDC, 
								CRect rect, 
								LPHDITEM lphdi, 
								CBitmap* pBitmap, 
								BITMAP* pBitmapInfo, 
								BOOL bRight)
{
	UNUSED_ALWAYS(lphdi);

	int iWidth = 0;

	if (pBitmap)
	{
		iWidth = pBitmapInfo->bmWidth;
		if (iWidth <= rect.Width() && rect.Width() > 0)
		{
			POINT point;

			point.y = rect.CenterPoint().y - (pBitmapInfo->bmHeight >> 1);

			if (bRight)
				point.x = rect.right - iWidth;
			else
				point.x = rect.left;

			CDC dc;
			if (dc.CreateCompatibleDC(pDC) == TRUE) 
			{
				CBitmap * pOldBitmap = dc.SelectObject(pBitmap);
				iWidth = pDC->BitBlt(
					point.x, point.y, 
					pBitmapInfo->bmWidth, pBitmapInfo->bmHeight, 
					&dc, 
					0, 0, 
					SRCCOPY
				) ? iWidth:0;
				dc.SelectObject(pOldBitmap);
			}
			else 
				iWidth = 0;
		}
		else
			iWidth = 0;
	}

	return iWidth;
}

///////////////////////////////////////////////////////////////////////////////
// DrawText
int CCDHeaderCtrl::DrawText(CDC* pDC, CRect rect, LPHDITEM lphdi)
{
	CSize size = pDC->GetTextExtent(lphdi->pszText);

#if 0  // -----------------------------------------------------------
	pDC->SetTextColor(RGB(0,0,255));

	if (rect.Width() > 0 && lphdi->mask & HDI_TEXT && lphdi->fmt & HDF_STRING)
	{
		size = pDC->GetTextExtent(lphdi->pszText);

		// always center column headers
		pDC->DrawText(lphdi->pszText, -1, rect, 
			DT_CENTER|DT_END_ELLIPSIS|DT_SINGLELINE|DT_VCENTER);
	}
#endif // -----------------------------------------------------------

	//+++

	if (rect.Width() > 0 && lphdi->mask & HDI_TEXT && lphdi->fmt & HDF_STRING)
	{
		pDC->SetTextColor(m_rgbText);

		UINT nFormat = m_nFormat;

		if (nFormat == DT_DEFAULT)
		{
			// default to whatever alignment the column is set to

			if (lphdi->fmt & LVCFMT_CENTER)
				nFormat = DT_CENTER;
			else if (lphdi->fmt & LVCFMT_RIGHT)
				nFormat = DT_RIGHT;
			else
				nFormat = DT_LEFT;
		}

		pDC->DrawText(lphdi->pszText, -1, rect, 
			nFormat | DT_END_ELLIPSIS | DT_SINGLELINE | DT_VCENTER);
	}

	size.cx = rect.Width() > size.cx ? size.cx : rect.Width();

	return size.cx > 0 ? size.cx : 0;
}
