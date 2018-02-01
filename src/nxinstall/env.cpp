/* 
** NetXMS - Network Management System
** NXSL-based installer tool collection
** Copyright (C) 2005-2011 Victor Kirhenshtein
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
** File: env.cpp
**
**/

#include "nxinstall.h"


/**
 * Additional NXSL functions
 */
static NXSL_ExtFunction s_nxslInstallerFunctions[] =
{
   { _T("chdir"), F_chdir, 1 },
   { _T("system"), F_system, 1 }
};

/**
 * Constructor for installer script environment
 */
NXSL_InstallerEnvironment::NXSL_InstallerEnvironment() : NXSL_Environment()
{
	registerIOFunctionSet();
	registerFunctionSet(sizeof(s_nxslInstallerFunctions) / sizeof(NXSL_ExtFunction), s_nxslInstallerFunctions);
}

/**
 * Script trace output
 */
void NXSL_InstallerEnvironment::trace(int level, const TCHAR *text)
{
	if (level <= g_traceLevel)
	{
		_tprintf(_T("%s\n"), text);
	}
}
