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
#ifndef _SMANAGER_H__
#define _SMANAGER_H__

#include "helpers.h"
#include "sync.h"
#pragma once

typedef struct sScript : public sSync
{
	char*	szFilePath;
	char*	szBuffered;
	char*	szDescription;
	long	lRefCount;
	long	lUsgCount;
	long	lFlags;
}SPHP,*PPHP;

typedef struct sHandler
{
	char*	szFcnName;
	long	lPriority;
	long	lFlags;
	PPHP	php;

	sHandler* next;
	sHandler* prev;
}SHANDLER,*PHANDLER;

typedef CRITICAL_SECTION CSECTION;

void	sman_unregister(PPHP s);
void	sman_inc(PPHP s);
void	sman_dec(PPHP s);
void	sman_incref(PPHP s);
void	sman_decref(PPHP s);
long	sman_recache(PPHP s);
long	sman_uncache(PPHP s);
long	sman_uninstall(PPHP s,long del);
PPHP	sman_register(const char* szPathname,long cache);
PPHP	sman_getbyfile(const char* szPathname);

void	sman_startup();
void	sman_shutdown();

PHANDLER sman_handler_add(PPHP s,long event_id,long priority,long flags);
PHANDLER sman_handler_get(long event_id);
PHANDLER sman_handler_find(PPHP s,long event_id);
void	 sman_handler_enable(PHANDLER h);
void	 sman_handler_disable(PHANDLER h);

#define sman_lock() EnterCriticalSectionX(&sm_sync)
#define sman_unlock() LeaveCriticalSectionX(&sm_sync)

#endif //_SMANAGER_H__