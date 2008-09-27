/*
** NetXMS - Network Management System
** Command line event sender
** Copyright (C) 2003, 2004 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** $module: nx_strict_tree.h
**
**/

#ifndef _nx_strict_tree_h_
#define _nx_strict_tree_h_

#include <nms_common.h>
#if !defined (__FreeBSD__) || __FreeBSD__ >= 5
#include <getopt.h>
#endif
#include <nxclapi.h>


struct TREE_ITEM
{
	TREE_ITEM 	*pParent;
	TREE_ITEM	**pChildList;   // pointer to child items array 
	DWORD			dwChildCount;  // size of child items array in elemtns.
	DWORD			dwObjLevel;
	NXC_OBJECT	*pObject;
};


struct TREE_INDEX_ITEM
{
	DWORD		dwObjId;
	TREE_ITEM	*pTreeItem;
};	


class StrictTree
{
	private:
		NXC_SESSION	m_hSession;
		TREE_ITEM 	*m_pRootItem;

		// Next 3 members are used for tree enumeration, using different algorithms.
		TREE_ITEM	*m_pCurrentItem;
		DWORD			m_dwCurrentChildNo;
		DWORD			m_dwCurrentItemIdx;
	
		// this is needed to simplify search in tree and to accelerate tree walking when properly sorted
		TREE_INDEX_ITEM	*m_pTreeIndex;
		DWORD					m_dwTreeIndexSize;
		
		BOOL Init(NXC_OBJECT *pRootObject);
		TREE_ITEM *FindObjectInTree(DWORD dwObjId);

	   // this makes a "tricky" comparison, resulting index to be sorted in "tree walking order"
		static int SortCompareForWalking(const void *Param1, const void *Param2);
		
	public:
		StrictTree(NXC_SESSION hSession);
		StrictTree(NXC_SESSION hSession, NXC_OBJECT *pRootObject);
		StrictTree(NXC_SESSION hSession, DWORD dwObjId);
		~StrictTree();

		BOOL LoadTree();
		BOOL AddItem(NXC_OBJECT *pObject);
		BOOL RemoveItem(NXC_OBJECT *pObject);
	
		const void ResetEnumeration()
		{ 
			m_dwCurrentChildNo = 0; m_pCurrentItem = m_pRootItem; m_dwCurrentItemIdx = 0;
		};
  	 	
		NXC_OBJECT *GetNextObject();
		NXC_OBJECT *GetNextObject(DWORD *dwLevel);
};

#endif
