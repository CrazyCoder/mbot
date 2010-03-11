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
#include "sync.h"

CSyncList::CSyncList()
{
	m_count = 0;
	m_head = m_tail = NULL;
	InitializeCriticalSectionAndSpinCount(&m_cts,0x80000100);
}

CSyncList::~CSyncList()
{
	m_count = 0;
	m_head = m_tail = NULL;
	DeleteCriticalSection(&m_cts);
}

int CSyncList::Add(sSync* s)
{
	Lock();
	if(!m_head){
		m_head = m_tail = s;
	}else{
		m_tail->next = s;
		s->prev = m_tail;
		m_tail = s;
	}
	m_count++;
	Unlock();
	return 1;
}

int CSyncList::AddLocked(sSync* s)
{
	if(!m_head){
		m_head = m_tail = s;
	}else{
		m_tail->next = s;
		s->prev = m_tail;
		m_tail = s;
	}
	m_count++;
	return 1;
}
int CSyncList::Del(sSync* s)
{
	Lock();
	if(m_count == 1)
	{
		m_tail = m_head = NULL;
	}else{
		if(!s->next && !s->prev)goto Skip;

		if(m_head == s){
			m_head = s->next;
		}else if(m_tail == s){
			m_tail = s->prev;
		}

		if(s->prev){
			s->prev->next = s->next;
		}
		if(s->next){
			s->next->prev = s->prev;
		}
	}
	s->next = s->prev = NULL;
	m_count--;
Skip:
	Unlock();
	return 1;
}
int CSyncList::DelLocked(sSync* s)
{
	if(m_count == 1)
	{
		if(m_head != s)return 1;
		m_tail = m_head = NULL;
	}else{
		if(!s->next && !s->prev)return 1;

		if(m_head == s){
			m_head = s->next;
		}else if(m_tail == s){
			m_tail = s->prev;
		}

		if(s->prev){
			s->prev->next = s->next;
		}
		if(s->next){
			s->next->prev = s->prev;
		}
	}

	s->next = s->prev = NULL;
	m_count--;
	return 1;
}

int CSyncList::Release(SYNC_RELEASE fp)
{
	Lock();
	sSync* tmp;
	sSync* cur = m_head;
	while(cur)
	{
		tmp = cur->next;
		fp(cur);
		cur = tmp;
	}
	m_head = m_tail = NULL;
	Unlock();
	return TRUE;
}

int CSyncList::Lock()
{
	try{
		EnterCriticalSectionX(&m_cts);
	}catch(...){
		MBLOGEX("Error entering critical section!");
		return FALSE;
	}
	return TRUE;
}
int CSyncList::Unlock()
{
	try{
		LeaveCriticalSectionX(&m_cts);
	}catch(...){
		MBLOGEX("Error leaving critical section!");
		return FALSE;
	}
	return TRUE;
}