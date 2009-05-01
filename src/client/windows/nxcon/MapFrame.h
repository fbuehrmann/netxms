#if !defined(AFX_MAPFRAME_H__8A4682BA_0A30_4BE2_BDBE_16ED918E0D46__INCLUDED_)
#define AFX_MAPFRAME_H__8A4682BA_0A30_4BE2_BDBE_16ED918E0D46__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MapFrame.h : header file
//

#include "MapView.h"
#include "netxms_maps.h"
#include "MapToolbox.h"	// Added by ClassView

#define OBJECT_HISTORY_SIZE      512


/////////////////////////////////////////////////////////////////////////////
// CMapFrame frame

class CMapFrame : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CMapFrame)
protected:
	CMapFrame();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMapFrame)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	CString m_strTitle;
	int m_nScaleShift;
	DWORD m_dwMapId;
	NXC_OBJECT * GetFirstSelectedObject(void);
	CStatusBarCtrl m_wndStatusBar;
	BOOL CurrObjectIsNode(void);
	void RedoLayout(void);
	BOOL m_bShowToolBox;
	BOOL m_bShowStatusBar;
	BOOL m_bShowToolBar;
	CImageList m_imageList;
	CMapToolbox m_wndToolBox;
	CToolBar m_wndToolBar;
	CMapView m_wndMapView;
	virtual ~CMapFrame();

	// Generated message map functions
	//{{AFX_MSG(CMapFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnViewRefresh();
	afx_msg void OnMapZoomin();
	afx_msg void OnUpdateMapZoomin(CCmdUI* pCmdUI);
	afx_msg void OnMapZoomout();
	afx_msg void OnUpdateMapZoomout(CCmdUI* pCmdUI);
	afx_msg void OnObjectOpenparent();
	afx_msg void OnUpdateObjectOpenparent(CCmdUI* pCmdUI);
	afx_msg void OnMapBack();
	afx_msg void OnUpdateMapBack(CCmdUI* pCmdUI);
	afx_msg void OnMapForward();
	afx_msg void OnUpdateMapForward(CCmdUI* pCmdUI);
	afx_msg void OnMapHome();
	afx_msg void OnMapSave();
	afx_msg void OnMapRedolayout();
	afx_msg void OnPaint();
	afx_msg void OnMapShowStatusbar();
	afx_msg void OnUpdateMapShowStatusbar(CCmdUI* pCmdUI);
	afx_msg void OnMapShowToolbar();
	afx_msg void OnUpdateMapShowToolbar(CCmdUI* pCmdUI);
	afx_msg void OnMapShowToolbox();
	afx_msg void OnUpdateMapShowToolbox(CCmdUI* pCmdUI);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnObjectProperties();
	afx_msg void OnUpdateObjectProperties(CCmdUI* pCmdUI);
	afx_msg void OnObjectLastdcivalues();
	afx_msg void OnUpdateObjectLastdcivalues(CCmdUI* pCmdUI);
	afx_msg void OnMapSetbackground();
	afx_msg void OnObjectDatacollection();
	afx_msg void OnUpdateObjectDatacollection(CCmdUI* pCmdUI);
	afx_msg void OnObjectManage();
	afx_msg void OnUpdateObjectManage(CCmdUI* pCmdUI);
	afx_msg void OnObjectUnbind();
	afx_msg void OnUpdateObjectUnbind(CCmdUI* pCmdUI);
	afx_msg void OnObjectUnmanage();
	afx_msg void OnUpdateObjectUnmanage(CCmdUI* pCmdUI);
	afx_msg void OnObjectBind();
	afx_msg void OnUpdateObjectBind(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMapSave(CCmdUI* pCmdUI);
	afx_msg void OnMapLink();
	afx_msg void OnUpdateMapLink(CCmdUI* pCmdUI);
	afx_msg void OnMapAutolayout();
	afx_msg void OnUpdateMapAutolayout(CCmdUI* pCmdUI);
	afx_msg void OnClose();
	afx_msg void OnMapUnlink();
	afx_msg void OnUpdateMapUnlink(CCmdUI* pCmdUI);
	afx_msg void OnMapShowconnectornames();
	afx_msg void OnUpdateMapShowconnectornames(CCmdUI* pCmdUI);
	afx_msg void OnMapEnsurevisible();
	afx_msg void OnUpdateMapEnsurevisible(CCmdUI* pCmdUI);
	afx_msg void OnMapObjectsizeNormal();
	afx_msg void OnMapObjectsizeSmall();
	afx_msg void OnMapObjectsizeTiny();
	afx_msg void OnUpdateMapObjectsizeNormal(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMapObjectsizeSmall(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMapObjectsizeTiny(CCmdUI* pCmdUI);
	afx_msg void OnObjectChangeipaddress();
	afx_msg void OnUpdateObjectChangeipaddress(CCmdUI* pCmdUI);
	afx_msg void OnObjectComments();
	afx_msg void OnUpdateObjectComments(CCmdUI* pCmdUI);
	afx_msg void OnObjectPollConfiguration();
	afx_msg void OnUpdateObjectPollConfiguration(CCmdUI* pCmdUI);
	afx_msg void OnObjectPollInterfaceNames();
	afx_msg void OnUpdateObjectPollInterfaceNames(CCmdUI* pCmdUI);
	afx_msg void OnObjectPollStatus();
	afx_msg void OnUpdateObjectPollStatus(CCmdUI* pCmdUI);
	afx_msg void OnObjectSetchildmgmt();
	afx_msg void OnUpdateObjectSetchildmgmt(CCmdUI* pCmdUI);
	//}}AFX_MSG
   afx_msg LRESULT OnObjectChange(WPARAM wParam, LPARAM lParam);
   afx_msg LRESULT OnSubmapChange(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
private:
	void SetScaleShift(int nShift);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAPFRAME_H__8A4682BA_0A30_4BE2_BDBE_16ED918E0D46__INCLUDED_)
