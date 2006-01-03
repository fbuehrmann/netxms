/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2005, 2006 Victor Kirhenshtein
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
** $module: class.cpp
**
**/

#include "libnxsl.h"


//
// Class constructor
//

NXSL_Class::NXSL_Class(void)
{
}


//
// Class destructor
//

NXSL_Class::~NXSL_Class()
{
}


//
// Get attribute
// Default implementation - always returns error
//

NXSL_Value *NXSL_Class::GetAttr(NXSL_Object *pObject, char *pszAttr)
{
   return NULL;
}


//
// Set attribute
// Default implementation - always returns error
//

BOOL NXSL_Class::SetAttr(NXSL_Object *pObject, char *pszAttr, NXSL_Value *pValue)
{
   return FALSE;
}
