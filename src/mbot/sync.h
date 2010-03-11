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

#ifndef _SYNC_H_
#define _SYNC_H_

#define WINVER	0x0500
#define _WIN32_WINNT 0x0500
#define _WIN32_IE 0x0501
#define _WIN32_WINDOWS 0x0500
#include <windows.h>
#pragma once

typedef void (*SYNC_RELEASE)(void* data);

struct sSync
{
	sSync* next;
	sSync* prev;
public:
	sSync(){
		next = prev = NULL;
	}
};

class CSyncList
{
public:
	CSyncList();
	~CSyncList();
public:
	int Add(sSync* hs);
	int AddLocked(sSync* hs);
	int Del(sSync* hs);
	int DelLocked(sSync* s);
	int Lock();
	int Unlock();
	int Release(SYNC_RELEASE fp);
public:
	sSync* m_head;
	sSync* m_tail;
	unsigned long m_count;
	CRITICAL_SECTION m_cts;
};
#endif //_SYNC_H_