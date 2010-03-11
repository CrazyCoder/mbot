/*

Miranda Scripting Plugin for Miranda-IM
Copyright 2004-2006 Piotr Pawluczuk (www.pawluczuk.info)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#ifndef _DIALOGS_H__
#define _DIALOGS_H__

#include "mbot.h"
#include "helpers.h"
#include "sync.h"
#include "smanager.h"
#pragma once

#define DLG_NUM_CONTROLS 32


struct sDlgControl
{
	char pszCallback[24];
	char type;/*0-button*/;
	char dummy;
	HWND hWnd;
	long param;
	unsigned long cs1;
	unsigned long cs2;
};

struct sDialog
{
	char pszCallback[24];
	char pszWmCommand[24];
	char pszWmNotify[24];
	char pszWmTimer[24];
	HWND hDlg;
	unsigned long lFlags;
	unsigned long lDepth;
	unsigned long lNum;
	long param;
	unsigned long ltc;
	unsigned long lid;
	PPHP php;
	sDlgControl* table[DLG_NUM_CONTROLS];
	//callbacks
};

struct sStdInDlg{
	enum FLAGS{NONE=0,NO_CANCEL=0x02,MUST_FILL=0x04,NUMBER_ONLY=0x80};
	char* title;
	char* info;
	char* def;
	char  buffer[2048];
	long  x,y;
	long  flags;
};

BOOL CALLBACK DlgProcedure(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK DlgStdInProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
VOID DlgFree(sDialog* dlg);
LPCSTR DlgGetCtrlClass(long type);

#endif //_DIALOGS_H__