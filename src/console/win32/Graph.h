#if !defined(AFX_GRAPH_H__021ACC81_A267_4464_889B_5718D2985D19__INCLUDED_)
#define AFX_GRAPH_H__021ACC81_A267_4464_889B_5718D2985D19__INCLUDED_

#include "..\..\..\INCLUDE\nxclapi.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Graph.h : header file
//

#define MAX_GRAPH_ITEMS    16

/////////////////////////////////////////////////////////////////////////////
// CGraph window

class CGraph : public CWnd
{
// Construction
public:
	CGraph();

// Attributes
public:
	BOOL m_bShowGrid;
	BOOL m_bAutoScale;
	COLORREF m_rgbLabelTextColor;
	COLORREF m_rgbLabelBkColor;
	COLORREF m_rgbLineColors[MAX_GRAPH_ITEMS];
	COLORREF m_rgbGridColor;
	COLORREF m_rgbBkColor;
	COLORREF m_rgbAxisColor;
	COLORREF m_rgbTextColor;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGraph)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	void SetData(DWORD dwIndex, NXC_DCI_DATA *pData);
	void SetTimeFrame(DWORD dwTimeFrom, DWORD dwTimeTo);
	BOOL Create(DWORD dwStyle, const RECT &rect, CWnd *pwndParent, int nId);
	virtual ~CGraph();

	// Generated message map functions
protected:
	//{{AFX_MSG(CGraph)
	afx_msg void OnPaint();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	//}}AFX_MSG
   afx_msg int OnMouseHover(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
private:
	CPoint m_ptLastHoverPoint;
	RECT m_rectGraph;
	RECT m_rectInfo;
	void DrawInfoRect(POINTS pt);
	BOOL m_bIsActive;
	void SetMouseTracking(void);
	double m_dCurrMaxValue;
	double m_dSecondsPerPixel;
	void DrawLineGraph(CDC &dc, NXC_DCI_DATA *pData, COLORREF rgbColor);
	DWORD m_dwTimeTo;
	DWORD m_dwTimeFrom;
	DWORD m_dwNumItems;
	double m_dMaxValue;
	int m_iGridSize;
	NXC_DCI_DATA *m_pData[MAX_GRAPH_ITEMS];
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRAPH_H__021ACC81_A267_4464_889B_5718D2985D19__INCLUDED_)
