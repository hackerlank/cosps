// ProgressDlg.h : header file
// CG: This file was added by the Progress Dialog component

/////////////////////////////////////////////////////////////////////////////
// CProgressDlg dialog

#ifndef __PROGRESSDLG_H__
#define __PROGRESSDLG_H__

class CProgressDlg : public CDialog
{
// Construction / Destruction
public:
    CProgressDlg();   // standard constructor
    ~CProgressDlg();

    BOOL Create(CWnd *pParent=NULL);

    // Checking for Cancel button
    BOOL IsCancelled();
        
// Dialog Data
    //{{AFX_DATA(CProgressDlg)
	enum { IDD = CG_IDD_PROGRESS };
	//}}AFX_DATA
	CProgressCtrl* GetProgressCtrl();
	void UpdatePercent(int nCurrent);
	void SetTitle(LPCTSTR lpszTitle);
    void SetStatus(LPCTSTR lpszMessage);
	void SetCurObj(LPCTSTR lpszMessage);

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CProgressDlg)
    public:
    virtual BOOL DestroyWindow();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:  
    BOOL m_bCancel;
    BOOL m_bParentDisabled;
	CProgressCtrl m_Progress;
    void ReEnableParent();
    virtual void OnCancel();
    virtual void OnOK() {};

    // Generated message map functions
    //{{AFX_MSG(CProgressDlg)
    virtual BOOL OnInitDialog();
	afx_msg LRESULT OnProgressUpdate(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnProgressSetRange(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnProgressIsCancelled(WPARAM wParam, LPARAM lParam);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
private:
	CTimeCost m_timeCost;
};

#endif // __PROGRESSDLG_H__
