#if !defined(AFX_ADDTASKDLG_H__6A6C94A4_690C_4FE0_A1ED_D8858B0F742D__INCLUDED_)
#define AFX_ADDTASKDLG_H__6A6C94A4_690C_4FE0_A1ED_D8858B0F742D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AddTaskDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAddTaskDlg dialog

class CAddTaskDlg : public CDialog
{
// Construction
public:
	CAddTaskDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAddTaskDlg)
	enum { IDD = IDD_ADD_TASK_DIALOG };
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddTaskDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddTaskDlg)
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeNewAddress();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDTASKDLG_H__6A6C94A4_690C_4FE0_A1ED_D8858B0F742D__INCLUDED_)
