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
#ifndef _WINDOW_H_
#define _WINDOW_H_

INT_PTR WINAPI MBotDlgProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
INT_PTR WINAPI MBotDlgProcOption(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

void MBotConsoleAppendText(const char* txt,long formatted = 0);
int  MBotGetPrefString(const char *szSetting, char* out, int len, char* def);
void MBotConsoleClear();

#endif //_WINDOW_H_