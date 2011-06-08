#include "stdafx.h"
#include "MultiSelTreeCtrl.h"
#include "tinyxml.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

UINT ID_TREE_ITEM_SELECTED_EVENT = ::RegisterWindowMessage(_T("ID_TREE_ITEM_SELECTED_EVENT"));

#define XML_NM_ROOT			"root"
#define XML_NM_LANG			"language"
#define XML_NM_FILE			"file"

#define XML_ATTRIB_TEXT		"text"
#define XML_ATTRIB_TYPE		"type"
#define XML_ATTRIB_CHECKED	"checked"

typedef CMap<HTREEITEM, HTREEITEM, TiXmlNode*, TiXmlNode*> CMapHTreeItem2XmlNodePtr;

BOOL CALLBACK PrintProc(CMultiSelTreeCtrl* pTree, HTREEITEM hTreeItem, LPARAM lParam)
{
	CString sText = pTree->GetItemText(hTreeItem);
	AfxTrace("%s\n", sText);
	return TRUE;
}

BOOL CALLBACK ExpandProc(CMultiSelTreeCtrl* pTree, HTREEITEM hTreeItem, LPARAM lParam)
{
	UINT nCode = (UINT)lParam;
	if(pTree->ItemHasChildren(hTreeItem))
	{
		return pTree->Expand(hTreeItem, nCode);
	}
	return TRUE;
}

BOOL CALLBACK SetItemStateImageProc(CMultiSelTreeCtrl* pTree, HTREEITEM hTreeItem, LPARAM lParam)
{
	UINT nState = (UINT)lParam;
	pTree->SetItemState(hTreeItem, INDEXTOSTATEIMAGEMASK(nState), TVIS_STATEIMAGEMASK);
	return TRUE;
}

BOOL CALLBACK GetSelectedItemProc(CMultiSelTreeCtrl* pTree, HTREEITEM hTreeItem, LPARAM lParam)
{
	CList<HTREEITEM, HTREEITEM>* pTreeItemList = (CList<HTREEITEM, HTREEITEM>*)lParam;

	UINT nStateImage = pTree->GetItemStateImage(hTreeItem);
	//Only full-checked leaf node item is the valid selected items.
	if(nStateImage == TVIS_IMAGE_STATE_FULL_CHECK)
	{
		if(!pTree->ItemHasChildren(hTreeItem))
		{
			pTreeItemList->AddTail(hTreeItem);
		}
	}	
	return TRUE;
}

BOOL CALLBACK SaveTreeProc(CMultiSelTreeCtrl* pTree, HTREEITEM hTreeItem, LPARAM lParam)
{
	CMapHTreeItem2XmlNodePtr* pMap = (CMapHTreeItem2XmlNodePtr*)lParam;

	HTREEITEM hParent = pTree->GetParentItem(hTreeItem);
	TiXmlNode* pParent = NULL;
	BOOL bExist = pMap->Lookup(hParent, pParent);
	ASSERT(bExist);

	TiXmlNode* pChild = pTree->InsertXMLChild(pParent, hTreeItem);
	pMap->SetAt(hTreeItem, pChild);

	return TRUE;
}

CMultiSelTreeCtrl::CMultiSelTreeCtrl()
{
	m_uFlags = 0;
}

CMultiSelTreeCtrl::~CMultiSelTreeCtrl()
{
}


BEGIN_MESSAGE_MAP(CMultiSelTreeCtrl, CTreeCtrl)
	//{{AFX_MSG_MAP(CMutiTreeCtrl)
	ON_WM_DESTROY()
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONDOWN()
	ON_NOTIFY_REFLECT(TVN_KEYDOWN, OnKeydown)
	ON_NOTIFY_REFLECT(NM_CLICK, OnStateIconClick)	
	ON_NOTIFY_REFLECT(TVN_DELETEITEM, OnDeleteItem)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMutiTreeCtrl message handlers
void CMultiSelTreeCtrl::PreSubclassWindow() 
{
	CTreeCtrl::PreSubclassWindow();

	LPITEMIDLIST  pidl = NULL;
	SHGetSpecialFolderLocation(GetSafeHwnd(), (UINT)CSIDL_DESKTOP, &pidl);
	
    SHFILEINFO sfi;
	ZeroMemory(&sfi, sizeof(SHFILEINFO));
	HIMAGELIST hShellImageList = (HIMAGELIST) SHGetFileInfo((LPCTSTR)(LPCITEMIDLIST)pidl,
		0, &sfi, sizeof(SHFILEINFO), SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
	ImageList_SetBkColor(hShellImageList, CLR_NONE);
	
	SendMessage(TVM_SETIMAGELIST, TVSIL_NORMAL, (LPARAM)hShellImageList);
	
	IMalloc* pIface = NULL;
	SHGetMalloc(&pIface);
	
	if( SUCCEEDED(::SHGetMalloc(&pIface)) )
	{
		pIface->Free(pidl);
		pIface->Release();
		pIface = NULL;
	}
}
BOOL CMultiSelTreeCtrl::Init()
{
	const TCHAR* lpXMLFile = _T("filter_tree.xml");
	TiXmlDocument doc( CommonUtils::GetConfFilePath(lpXMLFile) );
	bool loadOkay = doc.LoadFile();	
	if ( !loadOkay )
	{
		AfxTrace(_T("Failed to load file %s. Error=%s.\n"), lpXMLFile, doc.ErrorDesc());
		return FALSE;
	}

	TiXmlNode *pNode = NULL;
	TiXmlElement *pElement = NULL;

	pNode = doc.FirstChild( XML_NM_ROOT );
	ASSERT( pNode );
	pElement = pNode->ToElement();
	ASSERT(pElement);

	HTREEITEM hRoot = NULL;
	hRoot = InsertSubItem(NULL, pElement);

	CList<TiXmlElement*, TiXmlElement*&> pElementList;
	CList<HTREEITEM, HTREEITEM&> hTreeItemList;

	pElementList.AddTail(pElement);
	hTreeItemList.AddTail(hRoot);

	HTREEITEM hParent = NULL, hTreeItem = NULL;
	while(!pElementList.IsEmpty())
	{
		pElement = pElementList.RemoveHead();
		hParent = hTreeItemList.RemoveHead();

		for(pNode = pElement->FirstChild(); pNode != NULL; pNode = pNode->NextSibling())
		{
			pElement = pNode->ToElement();
			ASSERT(pElement);

			hTreeItem = InsertSubItem(hParent, pElement);

			pElementList.AddTail(pElement);
			hTreeItemList.AddTail(hTreeItem);
		}
	}
	return TRUE;
}

HTREEITEM CMultiSelTreeCtrl::InsertSubItem(HTREEITEM hParent, TiXmlElement* pElement)
{
	const char* pBuffer = NULL;

	TVINSERTSTRUCT tvis;
	ZeroMemory(&tvis, sizeof(TVINSERTSTRUCT));
	tvis.hParent = hParent;
	tvis.hInsertAfter = TVI_LAST;

	// used fields
	const UINT nTVIFlags = TVIF_TEXT | TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
	tvis.item.mask = nTVIFlags;

	//Text
	// provide a buffer for the item text
	TCHAR szText[MAX_PATH];
	ZeroMemory(szText, MAX_PATH);
	tvis.item.pszText = szText;
	tvis.item.cchTextMax = MAX_PATH;	

	pBuffer = pElement->Attribute(XML_ATTRIB_TEXT);
	if(pBuffer != NULL)
	{
		lstrcpyn(tvis.item.pszText, pBuffer, tvis.item.cchTextMax);
	}
	
	//State
	pBuffer = pElement->Attribute(XML_ATTRIB_CHECKED);

	int nCheckStatus = TVIS_IMAGE_STATE_UNCHECK;
	if(pBuffer != NULL)
	{
		nCheckStatus = atoi(pBuffer);
	}
	tvis.item.state = INDEXTOSTATEIMAGEMASK( nCheckStatus );
	tvis.item.stateMask = TVIS_STATEIMAGEMASK;

	pBuffer = pElement->Value();
	if(pBuffer != NULL && strcmp(pBuffer, XML_NM_FILE) != 0)
	{
		tvis.item.state |= TVIS_EXPANDED;
		tvis.item.stateMask |= TVIS_EXPANDED;
	}
	
	//lparam : prepare item data
	TVITEMDATA* pData = new TVITEMDATA;
	pData->szName = pElement->Value();
	pBuffer = pElement->Attribute(XML_ATTRIB_TYPE);
	if(pBuffer != NULL)
	{
		pData->szType = pBuffer;	
	}
	tvis.item.lParam = (LPARAM)pData;

	//Branch node
	if(pData->szType.IsEmpty())
	{
		//non-selected state image
		tvis.item.iImage = CommonUtils::GetWindowsDirIconIndex();
		//selected image
		tvis.item.iSelectedImage = CommonUtils::GetWindowsDirOpenIconIndex();
	}
	//Leaf node
	else
	{
		//non-selected state image
		tvis.item.iImage = CommonUtils::GetIconIndex(pData->szType);
		//selected image
		tvis.item.iSelectedImage = tvis.item.iImage;
	}
	
	// then insert new item
	HTREEITEM hTreeItem = InsertItem(&tvis);
	return hTreeItem;
}

BOOL CMultiSelTreeCtrl::SaveTree(const char * filename, HTREEITEM hTreeItemRoot)
{
	if(hTreeItemRoot == NULL)
	{
		return FALSE;
	}

	BOOL bResult = FALSE;

	TiXmlDocument doc(filename);
	
	TiXmlDeclaration declaration("1.0", "UTF-8", "yes");
	doc.InsertEndChild(declaration);

	CMapHTreeItem2XmlNodePtr map;
	map.SetAt((HTREEITEM)NULL, &doc);
	BFSEnumItems(hTreeItemRoot, SaveTreeProc, (LPARAM)&map);
	map.RemoveAll();

	//Validation
	if(doc.Error())
	{
		AfxTrace("Error in %s: %s\n", doc.Value(), doc.ErrorDesc());
		return FALSE;
	}
	doc.SaveFile();
	return bResult;
}

TiXmlNode* CMultiSelTreeCtrl::InsertXMLChild(TiXmlNode* pParentNode, HTREEITEM hItem)
{
	TVITEMDATA* pTVIData = (TVITEMDATA*)GetItemData(hItem);
	ASSERT(pTVIData);

	TiXmlElement element(pTVIData->szName);
	element.SetAttribute(XML_ATTRIB_TEXT, GetItemText(hItem));
	element.SetAttribute(XML_ATTRIB_TYPE, pTVIData->szType);
	element.SetAttribute(XML_ATTRIB_CHECKED, GetItemStateImage(hItem));

	return pParentNode->InsertEndChild(element);
}

void CMultiSelTreeCtrl::OnDestroy() 
{
	SaveTree(CommonUtils::GetConfFilePath(_T("filter_tree.xml"), GCFP_USER), GetRootItem());
	CTreeCtrl::OnDestroy();
}

void CMultiSelTreeCtrl::OnStateIconClick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;
	if(m_uFlags & TVHT_ONITEMSTATEICON) 
	{
		*pResult = 1;
		CWnd* pOwner = GetOwner();
		
		if(pOwner != NULL)
		{
			pOwner->SendMessage(ID_TREE_ITEM_SELECTED_EVENT, 0, m_uFlags);
		}
	}
}

void CMultiSelTreeCtrl::OnLButtonDown(UINT nFlags, CPoint point) 
{
	HTREEITEM hItem = HitTest(point, &m_uFlags);
	if ( (m_uFlags & TVHT_ONITEMSTATEICON) != 0 )
	{
		UINT nStateImage = GetItemStateImage(hItem);
		nStateImage = (nStateImage == TVIS_IMAGE_STATE_FULL_CHECK) ? 
			TVIS_IMAGE_STATE_UNCHECK : TVIS_IMAGE_STATE_FULL_CHECK;
		
		SetItemStateImage(hItem, nStateImage);
		
		//Set this item selected
		Select(hItem, TVGN_CARET);
	}
	
	CTreeCtrl::OnLButtonDown(nFlags, point);
}

void CMultiSelTreeCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	//CHAR space
	if(nChar == VK_SPACE)
	{
		HTREEITEM hItem = GetSelectedItem();
		UINT nStateImage = GetItemStateImage(hItem);
		if(nStateImage != TVIS_IMAGE_STATE_NONE)
		{
			nStateImage = (nStateImage == TVIS_IMAGE_STATE_FULL_CHECK) ? 
				TVIS_IMAGE_STATE_UNCHECK : TVIS_IMAGE_STATE_FULL_CHECK;
			SetItemStateImage(hItem, nStateImage);
		}
	}
	else 
	{
		CTreeCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
	}
}

void CMultiSelTreeCtrl::OnKeydown(NMHDR* pNMHDR, LRESULT* pResult) 
{
	TV_KEYDOWN* pTVKeyDown = (TV_KEYDOWN*)pNMHDR;
	
	*pResult = 0;
}

void CMultiSelTreeCtrl::OnDeleteItem(NMHDR* pNMHDR, LRESULT* pResult) 
{
	TVITEM& item = ((LPNMTREEVIEW)pNMHDR)->itemOld;
	
	// free item data, ignore invalid items
	if (item.lParam != 0)
		delete (TVITEMDATA*)item.lParam;
	
	*pResult = 0;
}

BOOL CMultiSelTreeCtrl::BFSEnumItems(HTREEITEM hTreeItemRoot, ENUM_TREEITEMPROC enumProc, LPARAM lParam)
{
	if(hTreeItemRoot == NULL)
	{
		return TRUE;
	}
	CList<HTREEITEM, HTREEITEM> hTreeItemList;
	hTreeItemList.AddTail(hTreeItemRoot);	

	HTREEITEM hCurTreeItem = hTreeItemRoot;
	HTREEITEM hTreeItem = NULL;

	//Add all siblings to the list firstly
	for(hTreeItem = GetNextItem(hCurTreeItem, TVGN_NEXT); hTreeItem != NULL; hTreeItem = GetNextItem(hTreeItem, TVGN_NEXT))
	{
		hTreeItemList.AddTail(hTreeItem);
	}
	
	while(!hTreeItemList.IsEmpty()) 
	{
		hCurTreeItem = hTreeItemList.RemoveHead();

		//Add all the children items to the list
		for(hTreeItem = GetChildItem(hCurTreeItem); hTreeItem != NULL; hTreeItem = GetNextItem(hTreeItem, TVGN_NEXT))
		{
			hTreeItemList.AddTail(hTreeItem);
		}
		if(!enumProc(this, hCurTreeItem, lParam))
		{
			return FALSE;
		}
	}
	return TRUE;
}

//This algorithm is similar to the way of pre-order visiting binary tree.
//The first child   ==> left  child
//The first sibling ==> right child
BOOL CMultiSelTreeCtrl::DFSEnumItems(HTREEITEM hTreeItemRoot, ENUM_TREEITEMPROC enumProc, LPARAM lParam)
{
	if(hTreeItemRoot == NULL)
	{
		return TRUE;
	}
	//This list mimic the stack
	CList<HTREEITEM, HTREEITEM> hTreeItemList;

	HTREEITEM hTreeItem = hTreeItemRoot;
	while(hTreeItem != NULL || !hTreeItemList.IsEmpty())
	{
		while(hTreeItem != NULL)  
		{  
			if(!enumProc(this, hTreeItem, lParam))
			{
				return FALSE;
			}
			
			hTreeItemList.AddTail(hTreeItem);

			//The first child: mimic the left child of binary tree
			hTreeItem = GetChildItem(hTreeItem);
		}
		//FILO, Remove from tail to mimic Stack
		hTreeItem = hTreeItemList.RemoveTail();

		//The first sibling: mimic the right child of binary tree
		hTreeItem = GetNextItem(hTreeItem, TVGN_NEXT);
	}
	return TRUE;
}

void CMultiSelTreeCtrl::ExpandAllItems(UINT nCode)
{
	SetRedraw(FALSE);
	BOOL nResult = BFSEnumItems(GetRootItem(), ExpandProc, (LPARAM)nCode);
	if(!nResult)
	{
		AfxTrace("Failed to ExpandAllItems\n");
	}
	SetRedraw(TRUE);
	UpdateWindow();
}

BOOL CMultiSelTreeCtrl::GetSelectedItems(CList<HTREEITEM, HTREEITEM>& hItemList)
{
	BOOL bResult = DFSEnumItems(GetRootItem(), GetSelectedItemProc, (LPARAM)&hItemList);
	return bResult;
}

UINT CMultiSelTreeCtrl::GetItemStateImage(HTREEITEM hItem)
{
	UINT nState = GetItemState(hItem, TVIS_STATEIMAGEMASK) >> 12;
	return nState;
}

BOOL CMultiSelTreeCtrl::SetItemStateImage(HTREEITEM hItem, UINT nStateImage)
{
	SetRedraw(FALSE);
	BOOL bResult = CTreeCtrl::SetItemState(hItem, INDEXTOSTATEIMAGEMASK(nStateImage), TVIS_STATEIMAGEMASK);
	if(nStateImage != 0)
	{
		//Set the children's state image
		BFSEnumItems(GetChildItem(hItem), SetItemStateImageProc, nStateImage);

		//Set the state image of siblings and parent
		SetSiblingAndParentStateImage(hItem, nStateImage);
	}
	SetRedraw(TRUE);
	UpdateWindow();
	return bResult;
}

void CMultiSelTreeCtrl::SetSiblingAndParentStateImage(HTREEITEM hItem, int nState)
{
	HTREEITEM hParentItem = GetParentItem(hItem);
	while(hParentItem != NULL)
	{
		//all the siblings have the same state
		if(IsSameStateImageWithOtherSiblings(hItem, nState))
		{
			CTreeCtrl::SetItemState(hParentItem, INDEXTOSTATEIMAGEMASK(nState), TVIS_STATEIMAGEMASK);
			hItem = hParentItem;
			hParentItem = GetParentItem(hParentItem);
		}
		else
		{
			//No need to check other siblings, parent and ancestor node should all be in the partial-check state
			while(hParentItem != NULL)
			{
				CTreeCtrl::SetItemState(hParentItem, INDEXTOSTATEIMAGEMASK(TVIS_IMAGE_STATE_PARTIAL_CHECK), TVIS_STATEIMAGEMASK);
				hParentItem = GetParentItem(hParentItem);
			}
			//now, hParentItem should be NULL, it will exit the outer loop.
		}
	}
}
BOOL CMultiSelTreeCtrl::IsSameStateImageWithOtherSiblings(HTREEITEM hItem, int nState)
{
	int nSiblingState = nState;

	HTREEITEM hNextSiblingItem = GetNextSiblingItem(hItem);
	for( ; hNextSiblingItem != NULL; hNextSiblingItem = GetNextSiblingItem(hNextSiblingItem))
	{
		nSiblingState = GetItemStateImage(hNextSiblingItem);
		if(nSiblingState != 0 && nSiblingState != nState)
		{
			return FALSE;
		}
	}

	HTREEITEM hPrevSiblingItem = GetPrevSiblingItem(hItem);
	for( ; hPrevSiblingItem != NULL; hPrevSiblingItem = GetPrevSiblingItem(hPrevSiblingItem))
	{
		nSiblingState = GetItemStateImage(hPrevSiblingItem);
		if(nSiblingState != 0 && nSiblingState != nState)
		{
			return FALSE;
		}
	}
	
	return TRUE;
}