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
#include "mbot.h"
#include "helpers.h"
#include "functions.h"
#include "smanager.h"

CSyncList	sm_list;
PHANDLER	sm_handlers[32]={0};
CSECTION	sm_sync = {0};
int			sm_maxe = fcn_log2(MB_EVENT_FILE_IN) + 1;

void sman_startup()
{
	InitializeCriticalSectionAndSpinCount(&sm_sync,0x80000100);
}
void sman_shutdown()
{
	if(!hConsole)return;

	sman_lock();
	for(int i=0;i<sm_maxe;i++)
	{
		PHANDLER ph = sm_handlers[i];
		PHANDLER tmp;
		while(ph)
		{
			tmp = ph->next;
			my_memfree(ph);
			ph = tmp;
		}
	}
	sman_unlock();

	while(1)
	{
		PPHP php;
		PPHP tmp;

		sm_list.Lock();
		php = (PPHP)sm_list.m_head;
		while(php)
		{
			tmp = (PPHP)php->next;
			if(php->szBuffered){
				my_memfree(php->szBuffered);
			}
			if(php->szFilePath){
				my_memfree(php->szFilePath);
			}
			my_memfree(php);
			php = tmp;
		}
		sm_list.Unlock();
		break;
	}
	DeleteCriticalSection(&sm_sync);
}

void sman_insertafter(PHANDLER a,PHANDLER b)
{
	if(a->next){
		a->next->prev = b;
		b->next = a->next;
	}
	b->prev = a;
	a->next = b;
}

void sman_inserthead(PHANDLER a,PHANDLER b)
{
	a->prev = b;
	b->next = a;
	b->prev = NULL;
}

PHANDLER sman_handler_add(PPHP s,long event_id,long priority,long flags)
{
	sman_lock();

	PHANDLER*	head;
	PHANDLER	f;
	PHANDLER	nhandler;
	char*		szFunction = (char*)help_get_funct_name(event_id);

	head = &sm_handlers[fcn_log2(event_id)];

	if(*head){
		f = *head;
		while(f)
		{
			if(f->php == s){
				f->lFlags &= ~(MBOT_FLAG_INACTIVE);
				sman_unlock();
				return f;
			}
			f = (PHANDLER)f->next;
		}
	}

	if(!szFunction || !(nhandler=(PHANDLER)my_malloc(sizeof(SHANDLER)))){
		goto Error;
	}
	memset(nhandler,0,sizeof(SHANDLER));

	nhandler->php = s;
	nhandler->lPriority = priority;
	nhandler->szFcnName = szFunction;

	head = &sm_handlers[fcn_log2(event_id)];

	if(*head == NULL){
		*head = nhandler;
	}else{
		f = *head;

		if(f->lPriority < priority){
			sman_inserthead(f,nhandler);
			*head = nhandler;
		}else{
			while(1){
				if(f->next == NULL || f->next->lPriority < priority){
					sman_insertafter(f,nhandler);
					break;
				}
				f = f->next;
			}
		}
	}
	sman_unlock();
	return nhandler;
Error:
	sman_unlock();
	return NULL;
}

PHANDLER sman_handler_get(long event_id)
{
	return sm_handlers[fcn_log2(event_id)];
}

PHANDLER sman_handler_find(PPHP s,long event_id)
{
	PHANDLER first;
	
	sman_lock();
	first = sm_handlers[fcn_log2(event_id)];

	if(!first)
	{
		sman_unlock();
		return NULL;
	}

	while(first)
	{
		if(first->php == s)break;
		first = first->next;
	}
	sman_unlock();
	return first;
}

void sman_handler_disable(PHANDLER h)
{
	sman_lock();
	h->lFlags |= MBOT_FLAG_INACTIVE;
	sman_unlock();
}

void sman_handler_enable(PHANDLER h)
{
	sman_lock();
	h->lFlags &= ~(MBOT_FLAG_INACTIVE);
	sman_unlock();
}

PPHP sman_register(const char* szPathname,long cache)
{
	static char* body;
	static char* path;
	PPHP  php;

	for(char* a = (char*)strchr(szPathname,'\\');a;a=strchr(a,'\\'))*a = '/';

	sm_list.Lock();
	php = (PPHP)sm_list.m_head;
	while(php){
		if(strcmp(php->szFilePath,szPathname)==0){
			php->lRefCount++;
			goto End;
		}
		php = (PPHP)php->next;
	}
	//cache if necessary
	if(cache){
		if(!(body = (char*)help_cache_php_file(szPathname))){
			goto End;
		}
	}else{
		body = NULL;
	}
	path = my_strdup(szPathname);
	if(!path){
		goto Error;
	}
	php = (PPHP)my_malloc(sizeof(SPHP));
	if(!php){
		goto Error;
	}
	memset(php,0,sizeof(SPHP));
	php->lRefCount = 1;
	php->szBuffered = body;
	php->szFilePath = path;
	sm_list.AddLocked(php);
	goto End;
Error:
	if(cache && body)my_memfree((void*)body);
	if(path)my_memfree((void*)path);
End:
	sm_list.Unlock();
	return php;
}

PPHP sman_getbyfile(const char* szPathname)
{
	PPHP  php;

	strlwr((char*)szPathname);
	for(char* a = (char*)strchr(szPathname,'\\');a;a=strchr(a,'\\'))*a = '/';

	sm_list.Lock();
	php = (PPHP)sm_list.m_head;
	while(php)
	{
		if(!stricmp(php->szFilePath,szPathname)){
			php->lRefCount++;
			goto End;
		}
		php = (PPHP)php->next;
	}
	End:
	sm_list.Unlock();
	return php;
}

long sman_uninstall(PPHP s,long del)
{
	extern CSyncList g_mlist;
	extern CSyncList g_svlist;

	PHANDLER ph;
	sMFSync *mfs;
	sHESync *hfs;

	sm_list.Lock();
	if(s->lUsgCount != 0){
		s->lFlags |= MBOT_FLAG_SHALLDIE | (del==1?MBOT_FLAG_DELETE:0);
		sm_list.Unlock();
		return 0;
	}
	sman_lock();

	for(int i=0;i<32 && s->lRefCount;i++)
	{
		ph = sm_handlers[i];
		while(ph && s->lRefCount)
		{
			if(ph->php == s){
				ph->lFlags |= MBOT_FLAG_INACTIVE | MBOT_FLAG_DELETE;
				s->lRefCount--;
				break;
			}
			ph = ph->next;
		}
	}

	//check if not using menu stuff
	g_mlist.Lock();
	mfs = (sMFSync*)g_mlist.m_head;
	while(mfs)
	{
		if(mfs->php == s){
			CLISTMENUITEM mi={0};
			mi.cbSize = sizeof(mi);
			mi.flags = CMIM_FLAGS | CMIF_GRAYED | CMIF_HIDDEN;
			CallService(MS_CLIST_MODIFYMENUITEM,(LPARAM)mfs->hMenu,(LPARAM)&mi);
		}
		mfs = (sMFSync*)mfs->next;
	}
	g_mlist.Unlock();
	//check if not using service stuff
	g_svlist.Lock();
	hfs = (sHESync*)g_svlist.m_head;
	while(hfs)
	{
		if(hfs->php == s)
		{
			sHESync* tmp = (sHESync*)hfs->next;
			if(hfs->hFunction){
				DestroyServiceFunction((HANDLE)hfs->hFunction);
			}else if(hfs->hHook){
				UnhookEvent((HANDLE)hfs->hHook);
			}
			my_memfree(hfs->pCode);
			g_svlist.DelLocked(tmp);
			my_memfree(tmp);
			hfs = tmp;
		}else{
			hfs = (sHESync*)hfs->next;
		}
	}
	g_svlist.Unlock();

	//check if not using cron stuff
	cron_unregister_param(s);

	if(s->szBuffered){
		my_memfree(s->szBuffered);
		s->szBuffered = NULL;
	}
	sm_list.Unlock();
	sman_unlock();
	return (!del)?1:DeleteFile(s->szFilePath);
}

void sman_unregister(PPHP s)
{
	sm_list.Lock();
	s->lRefCount--;
	if(s->lRefCount == 0)
	{
		if(s->szBuffered){
			my_memfree((void*)s->szBuffered);
		}
		if(s->szFilePath){
			my_memfree((void*)s->szFilePath);
		}
		sm_list.DelLocked(s);
	}
	sm_list.Unlock();
}

void sman_inc(PPHP s)
{
	sm_list.Lock();
	s->lUsgCount++;
	sm_list.Unlock();
}
void sman_dec(PPHP s)
{
	sm_list.Lock();
	s->lUsgCount--;
	if((s->lFlags & MBOT_FLAG_SHALLDIE) && s->lUsgCount == 0){
		sm_list.Unlock();
		sman_uninstall(s,(s->lFlags & MBOT_FLAG_DELETE)!=0);
	}else{
		sm_list.Unlock();
	}
}

void sman_incref(PPHP s)
{
	sm_list.Lock();
	s->lRefCount++;
	sm_list.Unlock();
}
void sman_decref(PPHP s)
{
	sm_list.Lock();
	s->lRefCount--;
	if(s->lRefCount <= 0)
	{
		if(s->szBuffered){
			my_memfree((void*)s->szBuffered);
		}
		if(s->szFilePath){
			my_memfree((void*)s->szFilePath);
		}
		sm_list.DelLocked(s);
	}
	sm_list.Unlock();
}

long sman_recache(PPHP s)
{
	long result=0;
	sm_list.Lock();
	if(s->lFlags)s->lFlags = 0;
	if(s->lUsgCount == 0)
	{
		if(s->szBuffered){
			my_memfree(s->szBuffered);
		}
		s->szBuffered = (char*)help_cache_php_file(s->szFilePath);
		result = (s->szBuffered != NULL);
	}
	sm_list.Unlock();
	return result;
}
long sman_uncache(PPHP s)
{
	long result=0;
	sm_list.Lock();
	if(s->lFlags)s->lFlags = 0;
	if(s->lUsgCount == 0)
	{
		if(s->szBuffered){
			my_memfree((void*)s->szBuffered);
			s->szBuffered = NULL;
		}
		result = 1;
	}
	sm_list.Unlock();
	return result;
}