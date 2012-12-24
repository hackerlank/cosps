// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__9DA43909_97A3_4F96_B127_82C39BD5AE21__INCLUDED_)
#define AFX_STDAFX_H__9DA43909_97A3_4F96_B127_82C39BD5AE21__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#pragma warning( disable : 4786 )

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

//application header files
#include "CommonCmds.h"
#include "ResizableDialog.h"

//config log4cplus

/*
#ifdef _DEBUG

#ifndef ENABLE_LOG4CPLUS
#define ENABLE_LOG4CPLUS
#endif

#else

#ifdef ENABLE_LOG4CPLUS
#undef ENABLE_LOG4CPLUS
#endif

#endif
*/
#define ENABLE_LOG4CPLUS
#include "log4cplus_config.h"

#include "BCGCBProInc.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__9DA43909_97A3_4F96_B127_82C39BD5AE21__INCLUDED_)
